#include "networkhealthservice.h"
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
#include <QVariant>

namespace {
const int kRequestTimeoutMs = 12000;
const int kDnsRetryDelayMs = 900;
const int kMaximumDnsRetries = 1;

const char kEthereumMainnetChainId[] = "0x1";
const char kBitcoinMainnetGenesis[] =
    "000000000019d6689c085ae165831e934ff763ae46a2a6c172b3f1b60a8ce26f";
const char kSolanaMainnetGenesisPrefix[] = "5eykt4UsFv8P8NJdTREpY1vzqKqZKvdp";
const char kKrakenTickerBase[] = "https://api.kraken.com/0/public/Ticker?pair=XBT";

QString normalizedCurrency(const QString &currency)
{
    const QString value = currency.trimmed().toUpper();
    if (value == QStringLiteral("USD")
            || value == QStringLiteral("EUR")
            || value == QStringLiteral("GBP"))
        return value;
    return QStringLiteral("GBP");
}

QString stateLabel(const QString &state)
{
    if (state == QStringLiteral("pass"))
        return QStringLiteral("Verified");
    if (state == QStringLiteral("warning"))
        return QStringLiteral("Attention");
    if (state == QStringLiteral("fail"))
        return QStringLiteral("Failed");
    if (state == QStringLiteral("checking"))
        return QStringLiteral("Checking…");
    return QStringLiteral("Not checked");
}

qint64 elapsedSince(qint64 started)
{
    const qint64 elapsed = QDateTime::currentMSecsSinceEpoch() - started;
    return elapsed < 0 ? 0 : elapsed;
}
}

NetworkHealthService::NetworkHealthService(QObject *parent)
    : QObject(parent)
    , m_summary(QStringLiteral("Run a check to verify the configured public providers"))
{
    resetProvider(EthereumRpc,
                  QStringLiteral("Ethereum RPC"),
                  QStringLiteral("Chain identity"),
                  QString());
    resetProvider(EthereumExplorer,
                  QStringLiteral("Ethereum explorer"),
                  QStringLiteral("Activity and token API"),
                  QString());
    resetProvider(BitcoinEsplora,
                  QStringLiteral("Bitcoin Esplora"),
                  QStringLiteral("Chain identity"),
                  QString());
    resetProvider(SolanaRpc,
                  QStringLiteral("Solana RPC"),
                  QStringLiteral("Chain identity"),
                  QString());
    resetProvider(SolanaTokenRpc,
                  QStringLiteral("Solana token RPC"),
                  QStringLiteral("Token-account provider"),
                  QString());
    resetProvider(KrakenTicker,
                  QStringLiteral("Kraken ticker"),
                  QStringLiteral("Public fiat price API"),
                  QStringLiteral("https://api.kraken.com"));
}

bool NetworkHealthService::loading() const
{
    return m_loading;
}

QString NetworkHealthService::summary() const
{
    return m_summary;
}

QString NetworkHealthService::lastChecked() const
{
    return m_lastChecked;
}

QVariantList NetworkHealthService::providers() const
{
    QVariantList result;
    for (int i = 0; i < ProviderCount; ++i)
        result.append(m_provider[i]);
    return result;
}

void NetworkHealthService::resetProvider(ProviderIndex index,
                                         const QString &name,
                                         const QString &purpose,
                                         const QString &endpoint)
{
    QVariantMap item;
    item.insert(QStringLiteral("name"), name);
    item.insert(QStringLiteral("purpose"), purpose);
    item.insert(QStringLiteral("endpoint"), endpoint.trimmed());
    item.insert(QStringLiteral("state"), QStringLiteral("idle"));
    item.insert(QStringLiteral("status"), QStringLiteral("Not checked"));
    item.insert(QStringLiteral("detail"), QStringLiteral("No request sent"));
    item.insert(QStringLiteral("latency"), QString());
    m_provider[index] = item;
}

void NetworkHealthService::checkAll(const QString &ethereumRpc,
                                    const QString &ethereumExplorer,
                                    const QString &bitcoinApi,
                                    const QString &solanaRpc,
                                    const QString &solanaTokenRpc,
                                    const QString &fiatCurrency)
{
    ++m_generation;
    const quint64 generation = m_generation;
    SailVaultNetwork::cancelOutstanding(&m_network);

    if (SailVaultNetwork::offlineModeEnabled()) {
        m_loading = false;
        m_completed = 0;
        m_lastChecked.clear();
        m_summary = QStringLiteral("Offline mode enabled · provider checks are disabled");
        emit stateChanged();
        return;
    }

    m_loading = true;
    m_completed = 0;
    m_lastChecked.clear();

    resetProvider(EthereumRpc,
                  QStringLiteral("Ethereum RPC"),
                  QStringLiteral("Chain identity"),
                  ethereumRpc);
    resetProvider(EthereumExplorer,
                  QStringLiteral("Ethereum explorer"),
                  QStringLiteral("Activity and token API"),
                  ethereumExplorer);
    resetProvider(BitcoinEsplora,
                  QStringLiteral("Bitcoin Esplora"),
                  QStringLiteral("Chain identity"),
                  bitcoinApi);
    resetProvider(SolanaRpc,
                  QStringLiteral("Solana RPC"),
                  QStringLiteral("Chain identity"),
                  solanaRpc);
    resetProvider(SolanaTokenRpc,
                  QStringLiteral("Solana token RPC"),
                  QStringLiteral("Token-account provider"),
                  solanaTokenRpc);
    resetProvider(KrakenTicker,
                  QStringLiteral("Kraken ticker"),
                  QStringLiteral("Public XBT/") + normalizedCurrency(fiatCurrency)
                      + QStringLiteral(" price API"),
                  QStringLiteral("https://api.kraken.com"));

    for (int i = 0; i < ProviderCount; ++i) {
        m_provider[i].insert(QStringLiteral("state"), QStringLiteral("checking"));
        m_provider[i].insert(QStringLiteral("status"), QStringLiteral("Checking…"));
        m_provider[i].insert(QStringLiteral("detail"), QStringLiteral("Request in progress"));
        m_provider[i].insert(QStringLiteral("latency"), QString());
    }

    updateSummary();
    emit stateChanged();

    checkEthereumRpc(ethereumRpc, generation);
    checkEthereumExplorer(ethereumExplorer, generation);
    checkBitcoin(bitcoinApi, generation);
    checkSolana(solanaRpc, SolanaRpc, generation);
    checkSolana(solanaTokenRpc, SolanaTokenRpc, generation);
    checkKraken(fiatCurrency, generation);
}

bool NetworkHealthService::validHttpsUrl(const QString &text, QUrl *url)
{
    const QUrl candidate(text.trimmed());
    const bool valid = candidate.isValid()
        && candidate.scheme().compare(QStringLiteral("https"), Qt::CaseInsensitive) == 0
        && !candidate.host().isEmpty();

    if (valid && url)
        *url = candidate;
    return valid;
}

QString NetworkHealthService::appendPath(const QString &base,
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

QString NetworkHealthService::replyError(QNetworkReply *reply)
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

void NetworkHealthService::armTimeout(QNetworkReply *reply)
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

void NetworkHealthService::finishInvalidUrl(ProviderIndex index,
                                            const QString &detail,
                                            quint64 generation)
{
    finishProvider(index, QStringLiteral("fail"), detail, -1, generation);
}

void NetworkHealthService::finishProvider(ProviderIndex index,
                                          const QString &state,
                                          const QString &detail,
                                          qint64 elapsedMs,
                                          quint64 generation)
{
    if (generation != m_generation)
        return;

    if (m_provider[index].value(QStringLiteral("state")).toString()
            != QStringLiteral("checking"))
        return;

    m_provider[index].insert(QStringLiteral("state"), state);
    m_provider[index].insert(QStringLiteral("status"), stateLabel(state));
    m_provider[index].insert(QStringLiteral("detail"), detail);
    m_provider[index].insert(
        QStringLiteral("latency"),
        elapsedMs >= 0
            ? QStringLiteral("%1 ms").arg(elapsedMs)
            : QString());

    ++m_completed;
    if (m_completed >= ProviderCount) {
        m_loading = false;
        m_lastChecked = QDateTime::currentDateTime().toString(
            QStringLiteral("yyyy-MM-dd HH:mm:ss"));
    }

    updateSummary();
    emit stateChanged();
}

void NetworkHealthService::updateSummary()
{
    if (m_loading) {
        m_summary = QStringLiteral("Checking public providers · %1/%2 complete")
            .arg(m_completed).arg(ProviderCount);
        return;
    }

    int passed = 0;
    int warnings = 0;
    int failed = 0;

    for (int i = 0; i < ProviderCount; ++i) {
        const QString state =
            m_provider[i].value(QStringLiteral("state")).toString();
        if (state == QStringLiteral("pass"))
            ++passed;
        else if (state == QStringLiteral("warning"))
            ++warnings;
        else if (state == QStringLiteral("fail"))
            ++failed;
    }

    if (passed == ProviderCount) {
        m_summary = QStringLiteral("All %1 public providers verified")
            .arg(ProviderCount);
    } else if (failed == 0 && warnings > 0) {
        m_summary = QStringLiteral("%1 verified · %2 need attention")
            .arg(passed).arg(warnings);
    } else if (failed > 0) {
        m_summary = QStringLiteral("%1 verified · %2 attention · %3 failed")
            .arg(passed).arg(warnings).arg(failed);
    } else {
        m_summary = QStringLiteral("Run a check to verify the configured public providers");
    }
}

void NetworkHealthService::checkEthereumRpc(const QString &endpoint,
                                            quint64 generation,
                                            int attempt)
{
    QUrl url;
    if (!validHttpsUrl(endpoint, &url)) {
        finishInvalidUrl(EthereumRpc,
                         QStringLiteral("Invalid endpoint · HTTPS URL required"),
                         generation);
        return;
    }

    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader,
                      QStringLiteral("application/json"));
    request.setRawHeader("Accept", "application/json");

    QJsonObject body;
    body.insert(QStringLiteral("jsonrpc"), QStringLiteral("2.0"));
    body.insert(QStringLiteral("id"), 1);
    body.insert(QStringLiteral("method"), QStringLiteral("eth_chainId"));
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
                checkEthereumRpc(endpoint, generation, attempt + 1);
            });
            return;
        }

        if (!error.isEmpty()) {
            finishProvider(EthereumRpc, QStringLiteral("fail"),
                           error, elapsed, generation);
            return;
        }

        QJsonParseError parseError;
        const QJsonDocument document = QJsonDocument::fromJson(data, &parseError);
        if (parseError.error != QJsonParseError::NoError || !document.isObject()) {
            finishProvider(EthereumRpc, QStringLiteral("fail"),
                           QStringLiteral("Invalid JSON-RPC response"),
                           elapsed, generation);
            return;
        }

        const QJsonObject object = document.object();
        if (object.contains(QStringLiteral("error"))) {
            finishProvider(EthereumRpc, QStringLiteral("fail"),
                           QStringLiteral("JSON-RPC returned an error"),
                           elapsed, generation);
            return;
        }

        const QString chainId = object.value(QStringLiteral("result"))
            .toString().toLower();
        if (chainId == QString::fromLatin1(kEthereumMainnetChainId)) {
            finishProvider(EthereumRpc, QStringLiteral("pass"),
                           QStringLiteral("Ethereum mainnet · chain ID 1"),
                           elapsed, generation);
        } else if (!chainId.isEmpty()) {
            finishProvider(EthereumRpc, QStringLiteral("warning"),
                           QStringLiteral("Reachable, but chain ID is %1 · expected 0x1")
                               .arg(chainId),
                           elapsed, generation);
        } else {
            finishProvider(EthereumRpc, QStringLiteral("fail"),
                           QStringLiteral("JSON-RPC response has no chain ID"),
                           elapsed, generation);
        }
    });
}

void NetworkHealthService::checkEthereumExplorer(const QString &endpoint,
                                                 quint64 generation,
                                                 int attempt)
{
    const QString requestUrl = appendPath(endpoint, QStringLiteral("stats"));
    QUrl url;
    if (!validHttpsUrl(requestUrl, &url)) {
        finishInvalidUrl(EthereumExplorer,
                         QStringLiteral("Invalid endpoint · HTTPS URL required"),
                         generation);
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
                checkEthereumExplorer(endpoint, generation, attempt + 1);
            });
            return;
        }

        if (!error.isEmpty()) {
            finishProvider(EthereumExplorer, QStringLiteral("fail"),
                           error, elapsed, generation);
            return;
        }

        QJsonParseError parseError;
        const QJsonDocument document = QJsonDocument::fromJson(data, &parseError);
        if (parseError.error != QJsonParseError::NoError || !document.isObject()) {
            finishProvider(EthereumExplorer, QStringLiteral("fail"),
                           QStringLiteral("Explorer returned invalid JSON"),
                           elapsed, generation);
            return;
        }

        const QJsonObject object = document.object();
        if (object.isEmpty()) {
            finishProvider(EthereumExplorer, QStringLiteral("warning"),
                           QStringLiteral("API is reachable, but /stats returned no data"),
                           elapsed, generation);
            return;
        }

        QString detail = QStringLiteral("Blockscout-compatible API responded");
        const QJsonValue blocks = object.value(QStringLiteral("total_blocks"));
        if (!blocks.isUndefined() && !blocks.isNull()) {
            const QString blockText = blocks.isString()
                ? blocks.toString()
                : QString::number(blocks.toDouble(), 'f', 0);
            if (!blockText.isEmpty())
                detail += QStringLiteral(" · block count %1").arg(blockText);
        }

        finishProvider(EthereumExplorer, QStringLiteral("pass"),
                       detail, elapsed, generation);
    });
}

void NetworkHealthService::checkBitcoin(const QString &endpoint,
                                        quint64 generation,
                                        int attempt)
{
    const QString requestUrl = appendPath(endpoint, QStringLiteral("block-height/0"));
    QUrl url;
    if (!validHttpsUrl(requestUrl, &url)) {
        finishInvalidUrl(BitcoinEsplora,
                         QStringLiteral("Invalid endpoint · HTTPS URL required"),
                         generation);
        return;
    }

    QNetworkRequest request(url);
    request.setRawHeader("Accept", "text/plain");

    const qint64 started = QDateTime::currentMSecsSinceEpoch();
    QNetworkReply *reply = m_network.get(request);
    armTimeout(reply);

    connect(reply, &QNetworkReply::finished, this,
            [this, reply, endpoint, generation, attempt, started]() {
        const QNetworkReply::NetworkError errorCode = reply->error();
        const QString body = QString::fromUtf8(reply->readAll()).trimmed();
        const qint64 elapsed = elapsedSince(started);
        const QString error = replyError(reply);
        reply->deleteLater();

        if (generation != m_generation)
            return;

        if (errorCode == QNetworkReply::HostNotFoundError
                && attempt < kMaximumDnsRetries) {
            QTimer::singleShot(kDnsRetryDelayMs, this,
                               [this, endpoint, generation, attempt]() {
                checkBitcoin(endpoint, generation, attempt + 1);
            });
            return;
        }

        if (!error.isEmpty()) {
            finishProvider(BitcoinEsplora, QStringLiteral("fail"),
                           error, elapsed, generation);
            return;
        }

        if (body == QString::fromLatin1(kBitcoinMainnetGenesis)) {
            finishProvider(BitcoinEsplora, QStringLiteral("pass"),
                           QStringLiteral("Bitcoin mainnet genesis verified"),
                           elapsed, generation);
        } else if (!body.isEmpty()) {
            finishProvider(BitcoinEsplora, QStringLiteral("warning"),
                           QStringLiteral("Reachable, but genesis block does not match Bitcoin mainnet"),
                           elapsed, generation);
        } else {
            finishProvider(BitcoinEsplora, QStringLiteral("fail"),
                           QStringLiteral("Esplora returned an empty response"),
                           elapsed, generation);
        }
    });
}

void NetworkHealthService::checkSolana(const QString &endpoint,
                                       ProviderIndex index,
                                       quint64 generation,
                                       int attempt)
{
    QUrl url;
    if (!validHttpsUrl(endpoint, &url)) {
        finishInvalidUrl(index,
                         QStringLiteral("Invalid endpoint · HTTPS URL required"),
                         generation);
        return;
    }

    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader,
                      QStringLiteral("application/json"));
    request.setRawHeader("Accept", "application/json");

    QJsonObject body;
    body.insert(QStringLiteral("jsonrpc"), QStringLiteral("2.0"));
    body.insert(QStringLiteral("id"), 1);
    body.insert(QStringLiteral("method"), QStringLiteral("getGenesisHash"));
    body.insert(QStringLiteral("params"), QJsonArray());

    const qint64 started = QDateTime::currentMSecsSinceEpoch();
    QNetworkReply *reply = m_network.post(
        request, QJsonDocument(body).toJson(QJsonDocument::Compact));
    armTimeout(reply);

    connect(reply, &QNetworkReply::finished, this,
            [this, reply, endpoint, index, generation, attempt, started]() {
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
                               [this, endpoint, index, generation, attempt]() {
                checkSolana(endpoint, index, generation, attempt + 1);
            });
            return;
        }

        if (!error.isEmpty()) {
            finishProvider(index, QStringLiteral("fail"),
                           error, elapsed, generation);
            return;
        }

        QJsonParseError parseError;
        const QJsonDocument document = QJsonDocument::fromJson(data, &parseError);
        if (parseError.error != QJsonParseError::NoError || !document.isObject()) {
            finishProvider(index, QStringLiteral("fail"),
                           QStringLiteral("Invalid JSON-RPC response"),
                           elapsed, generation);
            return;
        }

        const QJsonObject object = document.object();
        if (object.contains(QStringLiteral("error"))) {
            finishProvider(index, QStringLiteral("fail"),
                           QStringLiteral("JSON-RPC returned an error"),
                           elapsed, generation);
            return;
        }

        const QString genesis = object.value(QStringLiteral("result")).toString();
        if (genesis.startsWith(QString::fromLatin1(kSolanaMainnetGenesisPrefix))) {
            finishProvider(index, QStringLiteral("pass"),
                           QStringLiteral("Solana mainnet genesis verified"),
                           elapsed, generation);
        } else if (!genesis.isEmpty()) {
            finishProvider(index, QStringLiteral("warning"),
                           QStringLiteral("Reachable, but genesis hash does not match Solana mainnet"),
                           elapsed, generation);
        } else {
            finishProvider(index, QStringLiteral("fail"),
                           QStringLiteral("JSON-RPC response has no genesis hash"),
                           elapsed, generation);
        }
    });
}

void NetworkHealthService::checkKraken(const QString &currency,
                                       quint64 generation,
                                       int attempt)
{
    const QString normalized = normalizedCurrency(currency);
    const QString endpoint = QString::fromLatin1(kKrakenTickerBase) + normalized;
    const QUrl url(endpoint);

    QNetworkRequest request(url);
    request.setRawHeader("Accept", "application/json");

    const qint64 started = QDateTime::currentMSecsSinceEpoch();
    QNetworkReply *reply = m_network.get(request);
    armTimeout(reply);

    connect(reply, &QNetworkReply::finished, this,
            [this, reply, normalized, generation, attempt, started]() {
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
                               [this, normalized, generation, attempt]() {
                checkKraken(normalized, generation, attempt + 1);
            });
            return;
        }

        if (!error.isEmpty()) {
            finishProvider(KrakenTicker, QStringLiteral("fail"),
                           error, elapsed, generation);
            return;
        }

        QJsonParseError parseError;
        const QJsonDocument document = QJsonDocument::fromJson(data, &parseError);
        if (parseError.error != QJsonParseError::NoError || !document.isObject()) {
            finishProvider(KrakenTicker, QStringLiteral("fail"),
                           QStringLiteral("Kraken returned invalid JSON"),
                           elapsed, generation);
            return;
        }

        const QJsonObject object = document.object();
        const QJsonArray errors = object.value(QStringLiteral("error")).toArray();
        const QJsonObject result = object.value(QStringLiteral("result")).toObject();

        if (!errors.isEmpty()) {
            finishProvider(KrakenTicker, QStringLiteral("fail"),
                           QStringLiteral("Kraken returned an API error"),
                           elapsed, generation);
        } else if (result.isEmpty()) {
            finishProvider(KrakenTicker, QStringLiteral("warning"),
                           QStringLiteral("API is reachable, but ticker data is empty"),
                           elapsed, generation);
        } else {
            finishProvider(KrakenTicker, QStringLiteral("pass"),
                           QStringLiteral("Public XBT/%1 ticker available").arg(normalized),
                           elapsed, generation);
        }
    });
}
