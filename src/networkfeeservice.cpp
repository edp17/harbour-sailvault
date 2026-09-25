#include "networkfeeservice.h"
#include "networkrequestutils.h"

#include <QDateTime>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QTimer>
#include <QUrl>
#include <QVector>
#include <QtGlobal>

#include <algorithm>
#include <cmath>

namespace {
const int kRequestTimeoutMs = 12000;
const int kDnsRetryDelayMs = 900;
const int kMaximumDnsRetries = 1;
const int kBitcoinExampleVBytes = 140;

qint64 elapsedSince(qint64 started)
{
    const qint64 elapsed = QDateTime::currentMSecsSinceEpoch() - started;
    return elapsed < 0 ? 0 : elapsed;
}

QString stateLabel(const QString &state)
{
    if (state == QStringLiteral("pass"))
        return QStringLiteral("Updated");
    if (state == QStringLiteral("fail"))
        return QStringLiteral("Unavailable");
    if (state == QStringLiteral("checking"))
        return QStringLiteral("Checking…");
    return QStringLiteral("Not checked");
}

double nearestFeeRate(const QJsonObject &object, int target, bool *ok)
{
    int bestDistance = 0;
    double bestValue = 0.0;
    bool found = false;

    for (QJsonObject::const_iterator it = object.constBegin();
         it != object.constEnd(); ++it) {
        bool targetOk = false;
        const int currentTarget = it.key().toInt(&targetOk);
        if (!targetOk)
            continue;

        double value = 0.0;
        bool valueOk = false;
        if (it.value().isDouble()) {
            value = it.value().toDouble();
            valueOk = std::isfinite(value);
        } else if (it.value().isString()) {
            value = it.value().toString().toDouble(&valueOk);
        }

        if (!valueOk || value < 0.0)
            continue;

        const int distance = std::abs(currentTarget - target);
        if (!found || distance < bestDistance) {
            found = true;
            bestDistance = distance;
            bestValue = value;
        }
    }

    if (ok)
        *ok = found;
    return bestValue;
}
}

NetworkFeeService::NetworkFeeService(QObject *parent)
    : QObject(parent)
    , m_summary(QStringLiteral("Refresh to estimate current public network fees"))
{
    resetEstimate(Ethereum, QStringLiteral("Ethereum"), QString());
    resetEstimate(Bitcoin, QStringLiteral("Bitcoin"), QString());
    resetEstimate(Solana, QStringLiteral("Solana"), QString());
}

bool NetworkFeeService::loading() const
{
    return m_loading;
}

QString NetworkFeeService::summary() const
{
    return m_summary;
}

QString NetworkFeeService::lastUpdated() const
{
    return m_lastUpdated;
}

QVariantList NetworkFeeService::estimates() const
{
    QVariantList result;
    for (int i = 0; i < ChainCount; ++i)
        result.append(m_estimate[i]);
    return result;
}

void NetworkFeeService::resetEstimate(ChainIndex index,
                                      const QString &name,
                                      const QString &endpoint)
{
    QVariantMap item;
    item.insert(QStringLiteral("name"), name);
    item.insert(QStringLiteral("endpoint"), endpoint.trimmed());
    item.insert(QStringLiteral("state"), QStringLiteral("idle"));
    item.insert(QStringLiteral("status"), QStringLiteral("Not checked"));
    item.insert(QStringLiteral("headline"), QStringLiteral("—"));
    item.insert(QStringLiteral("detail"), QStringLiteral("No request sent"));
    item.insert(QStringLiteral("latency"), QString());
    m_estimate[index] = item;
}

void NetworkFeeService::refresh(const QString &ethereumRpc,
                                const QString &bitcoinApi,
                                const QString &solanaRpc)
{
    ++m_generation;
    const quint64 generation = m_generation;
    SailVaultNetwork::cancelOutstanding(&m_network);

    if (SailVaultNetwork::offlineModeEnabled()) {
        m_loading = false;
        m_completed = 0;
        m_lastUpdated.clear();
        m_summary = QStringLiteral("Offline mode enabled · fee refresh is disabled");
        emit stateChanged();
        return;
    }

    m_loading = true;
    m_completed = 0;
    m_lastUpdated.clear();

    resetEstimate(Ethereum, QStringLiteral("Ethereum"), ethereumRpc);
    resetEstimate(Bitcoin, QStringLiteral("Bitcoin"), bitcoinApi);
    resetEstimate(Solana, QStringLiteral("Solana"), solanaRpc);

    for (int i = 0; i < ChainCount; ++i) {
        m_estimate[i].insert(QStringLiteral("state"), QStringLiteral("checking"));
        m_estimate[i].insert(QStringLiteral("status"), QStringLiteral("Checking…"));
        m_estimate[i].insert(QStringLiteral("headline"), QStringLiteral("Fetching estimate…"));
        m_estimate[i].insert(QStringLiteral("detail"), QStringLiteral("Request in progress"));
        m_estimate[i].insert(QStringLiteral("latency"), QString());
    }

    updateSummary();
    emit stateChanged();

    refreshEthereum(ethereumRpc, generation);
    refreshBitcoin(bitcoinApi, generation);
    refreshSolana(solanaRpc, generation);
}

bool NetworkFeeService::validHttpsUrl(const QString &text, QUrl *url)
{
    const QUrl candidate(text.trimmed());
    const bool valid = candidate.isValid()
        && candidate.scheme().compare(QStringLiteral("https"), Qt::CaseInsensitive) == 0
        && !candidate.host().isEmpty();

    if (valid && url)
        *url = candidate;
    return valid;
}

QString NetworkFeeService::appendPath(const QString &base,
                                      const QString &path)
{
    QString result = base.trimmed();
    while (result.endsWith(QLatin1Char('/')))
        result.chop(1);

    if (!path.startsWith(QLatin1Char('/')))
        result += QLatin1Char('/');
    result += path;
    return result;
}

QString NetworkFeeService::replyError(QNetworkReply *reply)
{
    if (reply->property("sailvaultTimedOut").toBool())
        return QStringLiteral("Request timed out");

    const int status = reply->attribute(
        QNetworkRequest::HttpStatusCodeAttribute).toInt();

    if (reply->error() != QNetworkReply::NoError) {
        if (status > 0)
            return QStringLiteral("HTTP %1 · %2")
                .arg(status).arg(reply->errorString());
        return reply->errorString();
    }

    if (status >= 400)
        return QStringLiteral("HTTP %1").arg(status);

    return QString();
}

void NetworkFeeService::armTimeout(QNetworkReply *reply)
{
    QTimer *timer = new QTimer(reply);
    timer->setSingleShot(true);
    timer->setInterval(kRequestTimeoutMs);

    QObject::connect(timer, &QTimer::timeout, reply, [reply]() {
        reply->setProperty("sailvaultTimedOut", true);
        reply->abort();
    });
    QObject::connect(reply, &QNetworkReply::finished,
                     timer, &QTimer::stop);

    timer->start();
}

QString NetworkFeeService::compactNumber(double value, int maximumDecimals)
{
    if (!std::isfinite(value))
        return QStringLiteral("—");

    QString text = QString::number(value, 'f', maximumDecimals);
    while (text.contains(QLatin1Char('.')) && text.endsWith(QLatin1Char('0')))
        text.chop(1);
    if (text.endsWith(QLatin1Char('.')))
        text.chop(1);
    return text;
}

void NetworkFeeService::finishInvalidUrl(ChainIndex index,
                                         quint64 generation)
{
    finishEstimate(index,
                   QStringLiteral("fail"),
                   QStringLiteral("Invalid endpoint"),
                   QStringLiteral("HTTPS URL required"),
                   -1,
                   generation);
}

void NetworkFeeService::finishEstimate(ChainIndex index,
                                       const QString &state,
                                       const QString &headline,
                                       const QString &detail,
                                       qint64 elapsedMs,
                                       quint64 generation)
{
    if (generation != m_generation)
        return;

    if (m_estimate[index].value(QStringLiteral("state")).toString()
            != QStringLiteral("checking"))
        return;

    m_estimate[index].insert(QStringLiteral("state"), state);
    m_estimate[index].insert(QStringLiteral("status"), stateLabel(state));
    m_estimate[index].insert(QStringLiteral("headline"), headline);
    m_estimate[index].insert(QStringLiteral("detail"), detail);
    m_estimate[index].insert(
        QStringLiteral("latency"),
        elapsedMs >= 0 ? QStringLiteral("%1 ms").arg(elapsedMs) : QString());

    ++m_completed;
    if (m_completed >= ChainCount) {
        m_loading = false;
        m_lastUpdated = QDateTime::currentDateTime().toString(
            QStringLiteral("yyyy-MM-dd HH:mm:ss"));
    }

    updateSummary();
    emit stateChanged();
}

void NetworkFeeService::updateSummary()
{
    if (m_loading) {
        m_summary = QStringLiteral("Fetching fee estimates · %1/%2 complete")
            .arg(m_completed).arg(ChainCount);
        return;
    }

    int passed = 0;
    int failed = 0;
    for (int i = 0; i < ChainCount; ++i) {
        const QString state = m_estimate[i].value(QStringLiteral("state")).toString();
        if (state == QStringLiteral("pass"))
            ++passed;
        else if (state == QStringLiteral("fail"))
            ++failed;
    }

    if (passed == ChainCount)
        m_summary = QStringLiteral("ETH, BTC and SOL fee estimates updated");
    else if (passed > 0)
        m_summary = QStringLiteral("%1 network estimates updated · %2 unavailable")
            .arg(passed).arg(failed);
    else if (failed == ChainCount)
        m_summary = QStringLiteral("Fee estimates are currently unavailable");
    else
        m_summary = QStringLiteral("Refresh to estimate current public network fees");
}

void NetworkFeeService::refreshEthereum(const QString &endpoint,
                                        quint64 generation,
                                        int attempt)
{
    QUrl url;
    if (!validHttpsUrl(endpoint, &url)) {
        finishInvalidUrl(Ethereum, generation);
        return;
    }

    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader,
                      QStringLiteral("application/json"));
    request.setRawHeader("Accept", "application/json");

    QJsonObject body;
    body.insert(QStringLiteral("jsonrpc"), QStringLiteral("2.0"));
    body.insert(QStringLiteral("id"), 1);
    body.insert(QStringLiteral("method"), QStringLiteral("eth_gasPrice"));
    body.insert(QStringLiteral("params"), QJsonArray());

    const qint64 started = QDateTime::currentMSecsSinceEpoch();
    QNetworkReply *reply = m_network.post(
        request, QJsonDocument(body).toJson(QJsonDocument::Compact));
    armTimeout(reply);

    connect(reply, &QNetworkReply::finished, this,
            [this, reply, endpoint, generation, attempt, started]() {
        const QNetworkReply::NetworkError errorCode = reply->error();
        const QByteArray data = reply->readAll();
        const qint64 elapsed = elapsedSince(started);
        const QString error = replyError(reply);
        reply->deleteLater();

        if (generation != m_generation)
            return;

        if (errorCode == QNetworkReply::HostNotFoundError
                && attempt < kMaximumDnsRetries) {
            QTimer::singleShot(kDnsRetryDelayMs, this,
                               [this, endpoint, generation, attempt]() {
                refreshEthereum(endpoint, generation, attempt + 1);
            });
            return;
        }

        if (!error.isEmpty()) {
            finishEstimate(Ethereum, QStringLiteral("fail"),
                           QStringLiteral("Ethereum estimate failed"),
                           error, elapsed, generation);
            return;
        }

        QJsonParseError parseError;
        const QJsonDocument document = QJsonDocument::fromJson(data, &parseError);
        if (parseError.error != QJsonParseError::NoError || !document.isObject()) {
            finishEstimate(Ethereum, QStringLiteral("fail"),
                           QStringLiteral("Ethereum estimate failed"),
                           QStringLiteral("Invalid JSON-RPC response"),
                           elapsed, generation);
            return;
        }

        const QJsonObject object = document.object();
        if (object.contains(QStringLiteral("error"))) {
            finishEstimate(Ethereum, QStringLiteral("fail"),
                           QStringLiteral("Ethereum estimate failed"),
                           QStringLiteral("eth_gasPrice returned an RPC error"),
                           elapsed, generation);
            return;
        }

        QString gasHex = object.value(QStringLiteral("result")).toString().trimmed();
        if (gasHex.startsWith(QStringLiteral("0x"), Qt::CaseInsensitive))
            gasHex.remove(0, 2);

        bool ok = false;
        const quint64 gasWei = gasHex.toULongLong(&ok, 16);
        if (!ok || gasHex.isEmpty()) {
            finishEstimate(Ethereum, QStringLiteral("fail"),
                           QStringLiteral("Ethereum estimate failed"),
                           QStringLiteral("RPC returned an invalid gas price"),
                           elapsed, generation);
            return;
        }

        const double gasGwei = static_cast<double>(gasWei) / 1000000000.0;
        const double simpleTransferEth =
            static_cast<double>(gasWei) * 21000.0 / 1000000000000000000.0;

        const QString headline = QStringLiteral("%1 Gwei suggested gas price")
            .arg(compactNumber(gasGwei, gasGwei < 10.0 ? 3 : 2));
        const QString detail = QStringLiteral(
            "Simple 21,000-gas ETH transfer ≈ %1 ETH · provider eth_gasPrice")
            .arg(compactNumber(simpleTransferEth, 8));

        finishEstimate(Ethereum, QStringLiteral("pass"),
                       headline, detail, elapsed, generation);
    });
}

void NetworkFeeService::refreshBitcoin(const QString &endpoint,
                                       quint64 generation,
                                       int attempt)
{
    const QString requestUrl = appendPath(endpoint, QStringLiteral("fee-estimates"));
    QUrl url;
    if (!validHttpsUrl(requestUrl, &url)) {
        finishInvalidUrl(Bitcoin, generation);
        return;
    }

    QNetworkRequest request(url);
    request.setRawHeader("Accept", "application/json");

    const qint64 started = QDateTime::currentMSecsSinceEpoch();
    QNetworkReply *reply = m_network.get(request);
    armTimeout(reply);

    connect(reply, &QNetworkReply::finished, this,
            [this, reply, endpoint, generation, attempt, started]() {
        const QNetworkReply::NetworkError errorCode = reply->error();
        const QByteArray data = reply->readAll();
        const qint64 elapsed = elapsedSince(started);
        const QString error = replyError(reply);
        reply->deleteLater();

        if (generation != m_generation)
            return;

        if (errorCode == QNetworkReply::HostNotFoundError
                && attempt < kMaximumDnsRetries) {
            QTimer::singleShot(kDnsRetryDelayMs, this,
                               [this, endpoint, generation, attempt]() {
                refreshBitcoin(endpoint, generation, attempt + 1);
            });
            return;
        }

        if (!error.isEmpty()) {
            finishEstimate(Bitcoin, QStringLiteral("fail"),
                           QStringLiteral("Bitcoin estimate failed"),
                           error, elapsed, generation);
            return;
        }

        QJsonParseError parseError;
        const QJsonDocument document = QJsonDocument::fromJson(data, &parseError);
        if (parseError.error != QJsonParseError::NoError || !document.isObject()) {
            finishEstimate(Bitcoin, QStringLiteral("fail"),
                           QStringLiteral("Bitcoin estimate failed"),
                           QStringLiteral("Esplora returned invalid JSON"),
                           elapsed, generation);
            return;
        }

        const QJsonObject object = document.object();
        bool fastOk = false;
        bool balancedOk = false;
        bool economyOk = false;
        const double fast = nearestFeeRate(object, 1, &fastOk);
        const double balanced = nearestFeeRate(object, 6, &balancedOk);
        const double economy = nearestFeeRate(object, 144, &economyOk);

        if (!fastOk || !balancedOk || !economyOk) {
            finishEstimate(Bitcoin, QStringLiteral("fail"),
                           QStringLiteral("Bitcoin estimate failed"),
                           QStringLiteral("Esplora returned incomplete fee targets"),
                           elapsed, generation);
            return;
        }

        const qint64 fastSat = qRound64(fast * kBitcoinExampleVBytes);
        const qint64 balancedSat = qRound64(balanced * kBitcoinExampleVBytes);
        const qint64 economySat = qRound64(economy * kBitcoinExampleVBytes);

        const QString headline = QStringLiteral(
            "1 block %1 · 6 blocks %2 · 144 blocks %3 sat/vB")
            .arg(compactNumber(fast, 1))
            .arg(compactNumber(balanced, 1))
            .arg(compactNumber(economy, 1));
        const QString detail = QStringLiteral(
            "~%1 vB example: %2 / %3 / %4 sat · fast / balanced / economy")
            .arg(kBitcoinExampleVBytes)
            .arg(fastSat)
            .arg(balancedSat)
            .arg(economySat);

        finishEstimate(Bitcoin, QStringLiteral("pass"),
                       headline, detail, elapsed, generation);
    });
}

void NetworkFeeService::refreshSolana(const QString &endpoint,
                                      quint64 generation,
                                      int attempt)
{
    QUrl url;
    if (!validHttpsUrl(endpoint, &url)) {
        finishInvalidUrl(Solana, generation);
        return;
    }

    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader,
                      QStringLiteral("application/json"));
    request.setRawHeader("Accept", "application/json");

    QJsonArray accountList;
    QJsonArray params;
    params.append(accountList);

    QJsonObject body;
    body.insert(QStringLiteral("jsonrpc"), QStringLiteral("2.0"));
    body.insert(QStringLiteral("id"), 1);
    body.insert(QStringLiteral("method"), QStringLiteral("getRecentPrioritizationFees"));
    body.insert(QStringLiteral("params"), params);

    const qint64 started = QDateTime::currentMSecsSinceEpoch();
    QNetworkReply *reply = m_network.post(
        request, QJsonDocument(body).toJson(QJsonDocument::Compact));
    armTimeout(reply);

    connect(reply, &QNetworkReply::finished, this,
            [this, reply, endpoint, generation, attempt, started]() {
        const QNetworkReply::NetworkError errorCode = reply->error();
        const QByteArray data = reply->readAll();
        const qint64 elapsed = elapsedSince(started);
        const QString error = replyError(reply);
        reply->deleteLater();

        if (generation != m_generation)
            return;

        if (errorCode == QNetworkReply::HostNotFoundError
                && attempt < kMaximumDnsRetries) {
            QTimer::singleShot(kDnsRetryDelayMs, this,
                               [this, endpoint, generation, attempt]() {
                refreshSolana(endpoint, generation, attempt + 1);
            });
            return;
        }

        if (!error.isEmpty()) {
            finishEstimate(Solana, QStringLiteral("fail"),
                           QStringLiteral("Solana estimate failed"),
                           error, elapsed, generation);
            return;
        }

        QJsonParseError parseError;
        const QJsonDocument document = QJsonDocument::fromJson(data, &parseError);
        if (parseError.error != QJsonParseError::NoError || !document.isObject()) {
            finishEstimate(Solana, QStringLiteral("fail"),
                           QStringLiteral("Solana estimate failed"),
                           QStringLiteral("Invalid JSON-RPC response"),
                           elapsed, generation);
            return;
        }

        const QJsonObject object = document.object();
        if (object.contains(QStringLiteral("error"))) {
            finishEstimate(Solana, QStringLiteral("fail"),
                           QStringLiteral("Solana estimate failed"),
                           QStringLiteral("getRecentPrioritizationFees returned an RPC error"),
                           elapsed, generation);
            return;
        }

        const QJsonValue resultValue = object.value(QStringLiteral("result"));
        if (!resultValue.isArray()) {
            finishEstimate(Solana, QStringLiteral("fail"),
                           QStringLiteral("Solana estimate failed"),
                           QStringLiteral("RPC returned no prioritization-fee sample"),
                           elapsed, generation);
            return;
        }

        const QJsonArray result = resultValue.toArray();
        QVector<quint64> fees;
        int nonZero = 0;

        for (int i = 0; i < result.size(); ++i) {
            const QJsonValue entryValue = result.at(i);
            if (!entryValue.isObject())
                continue;

            const QJsonValue feeValue = entryValue.toObject().value(
                QStringLiteral("prioritizationFee"));
            if (!feeValue.isDouble())
                continue;

            const double rawFee = feeValue.toDouble();
            if (!std::isfinite(rawFee) || rawFee < 0.0)
                continue;

            const quint64 fee = static_cast<quint64>(rawFee);
            fees.append(fee);
            if (fee > 0)
                ++nonZero;
        }

        if (fees.isEmpty()) {
            finishEstimate(Solana, QStringLiteral("fail"),
                           QStringLiteral("Solana estimate failed"),
                           QStringLiteral("RPC returned an empty fee sample"),
                           elapsed, generation);
            return;
        }

        std::sort(fees.begin(), fees.end());
        const int medianIndex = (fees.size() - 1) / 2;
        const int p75Index = ((fees.size() - 1) * 75) / 100;
        const quint64 median = fees.at(medianIndex);
        const quint64 p75 = fees.at(p75Index);
        const quint64 peak = fees.last();

        const QString headline = QStringLiteral(
            "Median %1 · P75 %2 micro-lamports/CU")
            .arg(median)
            .arg(p75);
        const QString detail = QStringLiteral(
            "Peak %1 · %2/%3 recent slots non-zero · base fee 5,000 lamports/signature")
            .arg(peak)
            .arg(nonZero)
            .arg(fees.size());

        finishEstimate(Solana, QStringLiteral("pass"),
                       headline, detail, elapsed, generation);
    });
}
