#include "activityservice.h"
#include "settingsstore.h"
#include "networkrequestutils.h"

#include <algorithm>

#include <QDateTime>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QJsonValue>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QUrl>
#include <QUrlQuery>
#include <QVariantMap>

#include <boost/multiprecision/cpp_int.hpp>

namespace {

const char kEthereumExplorerKey[] = "network/ethereumExplorerUrl";
const char kBitcoinApiKey[] = "network/bitcoinApiUrl";
const char kSolanaRpcKey[] = "network/solanaRpcUrl";

const char kDefaultEthereumExplorer[] = "https://eth.blockscout.com/api/v2";
const char kDefaultBitcoinApi[] = "https://blockstream.info/api";
const char kDefaultSolanaRpc[] = "https://solana-rpc.publicnode.com";

const int kBitcoinPageSize = 25;
const int kSolanaPageSize = 25;

QString trimDecimalFraction(QString value)
{
    while (value.contains(QLatin1Char('.')) && value.endsWith(QLatin1Char('0')))
        value.chop(1);
    if (value.endsWith(QLatin1Char('.')))
        value.chop(1);
    return value;
}

QString addressHash(const QJsonValue &value)
{
    if (value.isString())
        return value.toString();
    if (value.isObject())
        return value.toObject().value(QStringLiteral("hash")).toString();
    return QString();
}

} // namespace

ActivityService::ActivityService(QObject *parent)
    : QObject(parent)
    , m_status(QStringLiteral("Open Recent activity to query public chain history."))
    , m_ethereumStatus(QStringLiteral("Not queried"))
    , m_bitcoinStatus(QStringLiteral("Not queried"))
    , m_solanaStatus(QStringLiteral("Not queried"))
{
}

bool ActivityService::loading() const
{
    return m_loading;
}

bool ActivityService::loadingOlder() const
{
    return m_loadingOlder;
}

bool ActivityService::canLoadOlder() const
{
    return m_ethHasMore || m_btcHasMore || m_solHasMore;
}

bool ActivityService::ethereumHasMore() const
{
    return m_ethHasMore;
}

bool ActivityService::bitcoinHasMore() const
{
    return m_btcHasMore;
}

bool ActivityService::solanaHasMore() const
{
    return m_solHasMore;
}

bool ActivityService::lastRefreshPassed() const
{
    return m_lastRefreshPassed;
}

QString ActivityService::status() const
{
    return m_status;
}

QString ActivityService::ethereumStatus() const
{
    return m_ethereumStatus;
}

QString ActivityService::bitcoinStatus() const
{
    return m_bitcoinStatus;
}

QString ActivityService::solanaStatus() const
{
    return m_solanaStatus;
}

QVariantList ActivityService::entries() const
{
    return m_entries;
}

int ActivityService::entryCount() const
{
    return m_entries.size();
}

QString ActivityService::lastUpdated() const
{
    return m_lastUpdated;
}

QString ActivityService::setting(const char *key, const char *fallback)
{
    return SailVaultSettings::value(
        QString::fromLatin1(key),
        QString::fromLatin1(fallback)).toString();
}

QString ActivityService::normalizeBaseUrl(const QString &url)
{
    QString value = url.trimmed();
    while (value.endsWith(QLatin1Char('/')))
        value.chop(1);
    return value;
}

QString ActivityService::networkErrorText(QNetworkReply *reply)
{
    if (SailVaultNetwork::timedOut(reply))
        return QStringLiteral("Request timed out after 15 seconds");

    const int status =
        reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();

    if (reply->error() != QNetworkReply::NoError) {
        return status > 0
            ? QStringLiteral("HTTP %1 · %2").arg(status).arg(reply->errorString())
            : reply->errorString();
    }

    if (status < 200 || status >= 300)
        return QStringLiteral("HTTP %1").arg(status);

    return QString();
}

QString ActivityService::formatBitcoin(qint64 satoshis)
{
    const bool negative = satoshis < 0;
    quint64 value = negative
        ? static_cast<quint64>(-(satoshis + 1)) + 1
        : static_cast<quint64>(satoshis);

    const quint64 whole = value / 100000000ULL;
    const quint64 fraction = value % 100000000ULL;

    QString result = QStringLiteral("%1.%2")
        .arg(whole)
        .arg(fraction, 8, 10, QLatin1Char('0'));

    result = trimDecimalFraction(result);
    if (!result.contains(QLatin1Char('.')))
        result += QStringLiteral(".0");
    if (negative)
        result.prepend(QLatin1Char('-'));

    return result + QStringLiteral(" BTC");
}

QString ActivityService::formatEthereumDecimalWei(const QString &weiText)
{
    QString digits = weiText.trimmed();
    if (digits.isEmpty())
        return QStringLiteral("0.0 ETH");

    bool negative = false;
    if (digits.startsWith(QLatin1Char('-'))) {
        negative = true;
        digits.remove(0, 1);
    }

    boost::multiprecision::cpp_int wei = 0;
    for (const QChar ch : digits) {
        if (ch < QLatin1Char('0') || ch > QLatin1Char('9'))
            return QStringLiteral("ETH");
        wei *= 10;
        wei += ch.unicode() - QLatin1Char('0').unicode();
    }

    const boost::multiprecision::cpp_int divisor("1000000000000000000");
    const boost::multiprecision::cpp_int whole = wei / divisor;
    const boost::multiprecision::cpp_int fraction = wei % divisor;

    QString result = QString::fromStdString(whole.convert_to<std::string>())
        + QLatin1Char('.')
        + QString::fromStdString(fraction.convert_to<std::string>())
              .rightJustified(18, QLatin1Char('0'))
              .left(8);

    result = trimDecimalFraction(result);
    if (!result.contains(QLatin1Char('.')))
        result += QStringLiteral(".0");
    if (negative)
        result.prepend(QLatin1Char('-'));

    return result + QStringLiteral(" ETH");
}

QString ActivityService::displayTime(qint64 timestamp)
{
    if (timestamp <= 0)
        return QStringLiteral("Pending / unknown time");

    return QDateTime::fromMSecsSinceEpoch(timestamp * 1000)
        .toLocalTime()
        .toString(QStringLiteral("yyyy-MM-dd hh:mm"));
}

QString ActivityService::jsonQueryValue(const QJsonValue &value)
{
    if (value.isString())
        return value.toString();
    if (value.isBool())
        return value.toBool() ? QStringLiteral("true") : QStringLiteral("false");
    if (value.isDouble())
        return QString::number(value.toDouble(), 'g', 16);
    return value.toVariant().toString();
}

bool ActivityService::addEntry(const QString &chain,
                               const QString &summary,
                               const QString &identifier,
                               const QString &state,
                               qint64 timestamp)
{
    const QString id = identifier.trimmed();
    if (id.isEmpty())
        return false;

    const QString key = chain + QLatin1Char(':') + id;
    if (m_seenEntries.contains(key))
        return false;

    m_seenEntries.insert(key);

    QVariantMap item;
    item.insert(QStringLiteral("chain"), chain);
    item.insert(QStringLiteral("summary"), summary);
    item.insert(QStringLiteral("identifier"), id);
    item.insert(QStringLiteral("state"), state);
    item.insert(QStringLiteral("timestamp"), timestamp);
    item.insert(QStringLiteral("timeText"), displayTime(timestamp));
    m_entries.append(item);
    return true;
}

void ActivityService::sortEntries()
{
    std::sort(m_entries.begin(), m_entries.end(),
              [](const QVariant &left, const QVariant &right) {
        const qint64 a =
            left.toMap().value(QStringLiteral("timestamp")).toLongLong();
        const qint64 b =
            right.toMap().value(QStringLiteral("timestamp")).toLongLong();
        return a > b;
    });
}

void ActivityService::updateChainStatus(const QString &chain)
{
    if (chain == QStringLiteral("Ethereum")) {
        m_ethereumStatus = QStringLiteral("OK · %1 loaded%2")
            .arg(m_ethLoaded)
            .arg(m_ethHasMore ? QStringLiteral(" · older available")
                              : QString());
    } else if (chain == QStringLiteral("Bitcoin")) {
        m_bitcoinStatus = QStringLiteral("OK · %1 loaded%2")
            .arg(m_btcLoaded)
            .arg(m_btcHasMore ? QStringLiteral(" · older available")
                              : QString());
    } else {
        m_solanaStatus = QStringLiteral("OK · %1 loaded%2")
            .arg(m_solLoaded)
            .arg(m_solHasMore ? QStringLiteral(" · older available")
                              : QString());
    }
}

int ActivityService::appendEthereumItems(const QJsonArray &items,
                                         const QString &address)
{
    int added = 0;

    for (int i = 0; i < items.size(); ++i) {
        const QJsonObject tx = items.at(i).toObject();

        const QString hash = tx.value(QStringLiteral("hash")).toString();
        const QString from = addressHash(tx.value(QStringLiteral("from")));
        const QString to = addressHash(tx.value(QStringLiteral("to")));
        const QString value = formatEthereumDecimalWei(
            tx.value(QStringLiteral("value")).toVariant().toString());

        QString direction = QStringLiteral("Ethereum transaction");
        if (to.compare(address, Qt::CaseInsensitive) == 0
                && from.compare(address, Qt::CaseInsensitive) != 0) {
            direction = QStringLiteral("Received %1").arg(value);
        } else if (from.compare(address, Qt::CaseInsensitive) == 0
                   && to.compare(address, Qt::CaseInsensitive) != 0) {
            direction = QStringLiteral("Sent %1").arg(value);
        } else {
            direction = QStringLiteral("Transaction · %1").arg(value);
        }

        const QString txStatus = tx.value(QStringLiteral("status")).toString();
        const QString state =
            txStatus.compare(QStringLiteral("error"), Qt::CaseInsensitive) == 0
                ? QStringLiteral("Failed")
                : QStringLiteral("Confirmed");

        qint64 timestamp = 0;
        const QString timestampText =
            tx.value(QStringLiteral("timestamp")).toString();
        if (!timestampText.isEmpty()) {
            const QDateTime dt = QDateTime::fromString(timestampText, Qt::ISODate);
            if (dt.isValid())
                timestamp = dt.toMSecsSinceEpoch() / 1000;
        }

        if (addEntry(QStringLiteral("Ethereum"),
                     direction,
                     hash,
                     state,
                     timestamp)) {
            ++added;
        }
    }

    m_ethLoaded += added;
    return added;
}

int ActivityService::appendBitcoinItems(const QJsonArray &items,
                                        const QString &address)
{
    int added = 0;

    for (int i = 0; i < items.size(); ++i) {
        const QJsonObject tx = items.at(i).toObject();
        qint64 received = 0;
        qint64 spent = 0;

        const QJsonArray outputs = tx.value(QStringLiteral("vout")).toArray();
        for (const QJsonValue &value : outputs) {
            const QJsonObject out = value.toObject();
            if (out.value(QStringLiteral("scriptpubkey_address")).toString()
                    == address) {
                received += static_cast<qint64>(
                    out.value(QStringLiteral("value")).toDouble());
            }
        }

        const QJsonArray inputs = tx.value(QStringLiteral("vin")).toArray();
        for (const QJsonValue &value : inputs) {
            const QJsonObject prevout =
                value.toObject().value(QStringLiteral("prevout")).toObject();
            if (prevout.value(QStringLiteral("scriptpubkey_address")).toString()
                    == address) {
                spent += static_cast<qint64>(
                    prevout.value(QStringLiteral("value")).toDouble());
            }
        }

        const qint64 net = received - spent;
        QString summary;
        if (net > 0)
            summary = QStringLiteral("Received %1").arg(formatBitcoin(net));
        else if (net < 0)
            summary = QStringLiteral("Sent %1").arg(formatBitcoin(-net));
        else
            summary = QStringLiteral("Bitcoin transaction");

        const QJsonObject status = tx.value(QStringLiteral("status")).toObject();
        const bool confirmed = status.value(QStringLiteral("confirmed")).toBool(false);
        const qint64 timestamp =
            confirmed
                ? static_cast<qint64>(
                      status.value(QStringLiteral("block_time")).toDouble())
                : 0;

        if (addEntry(QStringLiteral("Bitcoin"),
                     summary,
                     tx.value(QStringLiteral("txid")).toString(),
                     confirmed ? QStringLiteral("Confirmed")
                               : QStringLiteral("Pending"),
                     timestamp)) {
            ++added;
        }
    }

    m_btcLoaded += added;
    return added;
}

int ActivityService::appendSolanaItems(const QJsonArray &items)
{
    int added = 0;

    for (int i = 0; i < items.size(); ++i) {
        const QJsonObject tx = items.at(i).toObject();

        const bool failed =
            !tx.value(QStringLiteral("err")).isNull()
            && !tx.value(QStringLiteral("err")).isUndefined();

        QString summary = tx.value(QStringLiteral("memo")).toString();
        if (summary.isEmpty())
            summary = QStringLiteral("Solana transaction");

        const qint64 timestamp = static_cast<qint64>(
            tx.value(QStringLiteral("blockTime")).toDouble());

        QString state = failed
            ? QStringLiteral("Failed")
            : tx.value(QStringLiteral("confirmationStatus")).toString();
        if (state.isEmpty())
            state = QStringLiteral("Confirmed");

        if (addEntry(QStringLiteral("Solana"),
                     summary,
                     tx.value(QStringLiteral("signature")).toString(),
                     state,
                     timestamp)) {
            ++added;
        }
    }

    m_solLoaded += added;
    return added;
}

void ActivityService::refresh(const QString &ethereumAddress,
                              const QString &bitcoinAddress,
                              const QString &solanaAddress)
{
    const QString eth = ethereumAddress.trimmed();
    const QString btc = bitcoinAddress.trimmed();
    const QString sol = solanaAddress.trimmed();

    m_ethRequested = !eth.isEmpty();
    m_btcRequested = !btc.isEmpty();
    m_solRequested = !sol.isEmpty();

    if (!m_ethRequested && !m_btcRequested && !m_solRequested) {
        m_lastRefreshPassed = false;
        m_status = QStringLiteral(
            "Provide at least one public address before refreshing activity.");
        emit stateChanged();
        return;
    }

    if (SailVaultNetwork::offlineModeEnabled()) {
        ++m_generation;
        SailVaultNetwork::cancelOutstanding(&m_network);
        m_loading = false;
        m_loadingOlder = false;
        m_lastRefreshPassed = false;
        m_ethereumStatus = m_ethRequested ? QStringLiteral("Offline")
                                          : QStringLiteral("Not configured");
        m_bitcoinStatus = m_btcRequested ? QStringLiteral("Offline")
                                         : QStringLiteral("Not configured");
        m_solanaStatus = m_solRequested ? QStringLiteral("Offline")
                                        : QStringLiteral("Not configured");
        m_status = QStringLiteral(
            "Offline mode · recent activity requires a provider request");
        emit stateChanged();
        return;
    }

    const QUrl ethUrl(setting(kEthereumExplorerKey, kDefaultEthereumExplorer));
    const QUrl btcUrl(setting(kBitcoinApiKey, kDefaultBitcoinApi));
    const QUrl solUrl(setting(kSolanaRpcKey, kDefaultSolanaRpc));

    if ((m_ethRequested
         && (!ethUrl.isValid() || ethUrl.scheme() != QStringLiteral("https")))
            || (m_btcRequested
                && (!btcUrl.isValid()
                    || btcUrl.scheme() != QStringLiteral("https")))
            || (m_solRequested
                && (!solUrl.isValid()
                    || solUrl.scheme() != QStringLiteral("https")))) {
        m_lastRefreshPassed = false;
        m_status = QStringLiteral(
            "Required activity providers must be valid HTTPS URLs.");
        emit stateChanged();
        return;
    }

    ++m_generation;
    const quint64 generation = m_generation;
    SailVaultNetwork::cancelOutstanding(&m_network);

    m_loading = true;
    m_loadingOlder = false;
    m_lastRefreshPassed = false;

    m_ethDone = !m_ethRequested;
    m_btcDone = !m_btcRequested;
    m_solDone = !m_solRequested;

    m_ethOk = !m_ethRequested;
    m_btcOk = !m_btcRequested;
    m_solOk = !m_solRequested;

    m_ethAddress = eth;
    m_btcAddress = btc;
    m_solAddress = sol;

    m_entries.clear();
    m_seenEntries.clear();
    m_ethNextPageParams = QJsonObject();
    m_btcCursor.clear();
    m_solCursor.clear();
    m_ethHasMore = false;
    m_btcHasMore = false;
    m_solHasMore = false;
    m_ethLoaded = 0;
    m_btcLoaded = 0;
    m_solLoaded = 0;
    m_olderOutstanding = 0;
    m_olderRequested = 0;
    m_olderSucceeded = 0;
    m_olderAdded = 0;
    m_olderErrors.clear();

    m_ethereumStatus =
        m_ethRequested ? QStringLiteral("Refreshing…")
                       : QStringLiteral("Not configured");
    m_bitcoinStatus =
        m_btcRequested ? QStringLiteral("Refreshing…")
                       : QStringLiteral("Not configured");
    m_solanaStatus =
        m_solRequested ? QStringLiteral("Refreshing…")
                       : QStringLiteral("Not configured");

    m_status = QStringLiteral("Refreshing recent public-chain activity…");
    emit stateChanged();

    if (m_ethRequested)
        refreshEthereum(eth, generation);
    if (m_btcRequested)
        refreshBitcoin(btc, generation);
    if (m_solRequested)
        refreshSolana(sol, generation);
}

void ActivityService::refreshEthereum(const QString &address,
                                      quint64 generation)
{
    const QString endpoint =
        normalizeBaseUrl(setting(kEthereumExplorerKey, kDefaultEthereumExplorer))
        + QStringLiteral("/addresses/")
        + QString::fromUtf8(QUrl::toPercentEncoding(address))
        + QStringLiteral("/transactions");

    QNetworkRequest request{QUrl(endpoint)};
    request.setRawHeader("Accept", "application/json");

    QNetworkReply *reply = m_network.get(request);
    SailVaultNetwork::armTimeout(reply);

    connect(reply, &QNetworkReply::finished, this,
            [this, reply, address, generation]() {
        const QByteArray payload = reply->readAll();
        const QString networkError = networkErrorText(reply);
        reply->deleteLater();

        if (generation != m_generation)
            return;

        if (!networkError.isEmpty()) {
            setChainFailure(QStringLiteral("Ethereum"), networkError, generation);
            return;
        }

        QJsonParseError parseError;
        const QJsonDocument doc = QJsonDocument::fromJson(payload, &parseError);

        if (parseError.error != QJsonParseError::NoError || !doc.isObject()) {
            setChainFailure(QStringLiteral("Ethereum"),
                            QStringLiteral("invalid JSON response"),
                            generation);
            return;
        }

        const QJsonObject object = doc.object();
        appendEthereumItems(object.value(QStringLiteral("items")).toArray(),
                            address);

        const QJsonValue next = object.value(QStringLiteral("next_page_params"));
        m_ethNextPageParams = next.isObject() ? next.toObject() : QJsonObject();
        m_ethHasMore = !m_ethNextPageParams.isEmpty();

        m_ethOk = true;
        m_ethDone = true;
        updateChainStatus(QStringLiteral("Ethereum"));
        finishIfReady(generation);
    });
}

void ActivityService::refreshBitcoin(const QString &address,
                                     quint64 generation)
{
    const QString endpoint =
        normalizeBaseUrl(setting(kBitcoinApiKey, kDefaultBitcoinApi))
        + QStringLiteral("/address/")
        + QString::fromUtf8(QUrl::toPercentEncoding(address))
        + QStringLiteral("/txs");

    QNetworkRequest request{QUrl(endpoint)};
    request.setRawHeader("Accept", "application/json");

    QNetworkReply *reply = m_network.get(request);
    SailVaultNetwork::armTimeout(reply);

    connect(reply, &QNetworkReply::finished, this,
            [this, reply, address, generation]() {
        const QByteArray payload = reply->readAll();
        const QString networkError = networkErrorText(reply);
        reply->deleteLater();

        if (generation != m_generation)
            return;

        if (!networkError.isEmpty()) {
            setChainFailure(QStringLiteral("Bitcoin"), networkError, generation);
            return;
        }

        QJsonParseError parseError;
        const QJsonDocument doc = QJsonDocument::fromJson(payload, &parseError);

        if (parseError.error != QJsonParseError::NoError || !doc.isArray()) {
            setChainFailure(QStringLiteral("Bitcoin"),
                            QStringLiteral("invalid JSON response"),
                            generation);
            return;
        }

        const QJsonArray items = doc.array();
        appendBitcoinItems(items, address);

        int confirmedCount = 0;
        QString lastConfirmed;
        for (int i = 0; i < items.size(); ++i) {
            const QJsonObject tx = items.at(i).toObject();
            const bool confirmed = tx.value(QStringLiteral("status"))
                .toObject().value(QStringLiteral("confirmed")).toBool(false);
            if (confirmed) {
                ++confirmedCount;
                lastConfirmed = tx.value(QStringLiteral("txid")).toString();
            }
        }

        m_btcCursor = lastConfirmed;
        m_btcHasMore = confirmedCount >= kBitcoinPageSize
                       && !m_btcCursor.isEmpty();

        m_btcOk = true;
        m_btcDone = true;
        updateChainStatus(QStringLiteral("Bitcoin"));
        finishIfReady(generation);
    });
}

void ActivityService::refreshSolana(const QString &address,
                                    quint64 generation)
{
    QNetworkRequest request{QUrl(setting(kSolanaRpcKey, kDefaultSolanaRpc))};
    request.setHeader(QNetworkRequest::ContentTypeHeader,
                      QStringLiteral("application/json"));
    request.setRawHeader("Accept", "application/json");

    QJsonObject config;
    config.insert(QStringLiteral("commitment"), QStringLiteral("finalized"));
    config.insert(QStringLiteral("limit"), kSolanaPageSize);

    QJsonArray params;
    params.append(address);
    params.append(config);

    QJsonObject body;
    body.insert(QStringLiteral("jsonrpc"), QStringLiteral("2.0"));
    body.insert(QStringLiteral("id"), 1);
    body.insert(QStringLiteral("method"),
                QStringLiteral("getSignaturesForAddress"));
    body.insert(QStringLiteral("params"), params);

    QNetworkReply *reply =
        m_network.post(request,
                       QJsonDocument(body).toJson(QJsonDocument::Compact));
    SailVaultNetwork::armTimeout(reply);

    connect(reply, &QNetworkReply::finished, this,
            [this, reply, generation]() {
        const QByteArray payload = reply->readAll();
        const QString networkError = networkErrorText(reply);
        reply->deleteLater();

        if (generation != m_generation)
            return;

        if (!networkError.isEmpty()) {
            setChainFailure(QStringLiteral("Solana"), networkError, generation);
            return;
        }

        QJsonParseError parseError;
        const QJsonDocument doc = QJsonDocument::fromJson(payload, &parseError);

        if (parseError.error != QJsonParseError::NoError || !doc.isObject()) {
            setChainFailure(QStringLiteral("Solana"),
                            QStringLiteral("invalid JSON response"),
                            generation);
            return;
        }

        const QJsonObject object = doc.object();
        if (object.contains(QStringLiteral("error"))) {
            const QJsonObject error = object.value(QStringLiteral("error")).toObject();
            setChainFailure(
                QStringLiteral("Solana"),
                error.value(QStringLiteral("message")).toString(
                    QStringLiteral("RPC error")),
                generation);
            return;
        }

        const QJsonArray items = object.value(QStringLiteral("result")).toArray();
        appendSolanaItems(items);

        m_solCursor = items.isEmpty()
            ? QString()
            : items.at(items.size() - 1).toObject().value(QStringLiteral("signature")).toString();
        m_solHasMore = items.size() >= kSolanaPageSize && !m_solCursor.isEmpty();

        m_solOk = true;
        m_solDone = true;
        updateChainStatus(QStringLiteral("Solana"));
        finishIfReady(generation);
    });
}

void ActivityService::loadOlder(const QString &chain)
{
    if (SailVaultNetwork::offlineModeEnabled()) {
        m_lastRefreshPassed = false;
        m_status = QStringLiteral("Offline mode · older activity is unavailable");
        emit stateChanged();
        return;
    }

    if (m_loading || m_loadingOlder || !canLoadOlder())
        return;

    const QString selected = chain.trimmed();
    const bool loadAll = selected.isEmpty()
        || selected == QStringLiteral("All")
        || selected == QStringLiteral("All chains");

    const bool loadEthereum = m_ethHasMore
        && (loadAll || selected == QStringLiteral("Ethereum"));
    const bool loadBitcoin = m_btcHasMore
        && (loadAll || selected == QStringLiteral("Bitcoin"));
    const bool loadSolana = m_solHasMore
        && (loadAll || selected == QStringLiteral("Solana"));

    if (!loadEthereum && !loadBitcoin && !loadSolana)
        return;

    m_loadingOlder = true;
    m_lastRefreshPassed = false;
    m_olderOutstanding = 0;
    m_olderRequested = 0;
    m_olderSucceeded = 0;
    m_olderAdded = 0;
    m_olderErrors.clear();

    if (loadEthereum) {
        ++m_olderOutstanding;
        ++m_olderRequested;
    }
    if (loadBitcoin) {
        ++m_olderOutstanding;
        ++m_olderRequested;
    }
    if (loadSolana) {
        ++m_olderOutstanding;
        ++m_olderRequested;
    }

    const quint64 generation = m_generation;
    m_status = loadAll
        ? QStringLiteral("Loading older public-chain activity…")
        : QStringLiteral("Loading older %1 activity…").arg(selected);
    emit stateChanged();

    if (loadEthereum)
        loadOlderEthereum(generation);
    if (loadBitcoin)
        loadOlderBitcoin(generation);
    if (loadSolana)
        loadOlderSolana(generation);
}

void ActivityService::loadOlderEthereum(quint64 generation)
{
    QUrl url(normalizeBaseUrl(
                 setting(kEthereumExplorerKey, kDefaultEthereumExplorer))
             + QStringLiteral("/addresses/")
             + QString::fromUtf8(QUrl::toPercentEncoding(m_ethAddress))
             + QStringLiteral("/transactions"));

    QUrlQuery query;
    for (QJsonObject::const_iterator it = m_ethNextPageParams.constBegin();
         it != m_ethNextPageParams.constEnd(); ++it) {
        if (!it.value().isNull() && !it.value().isUndefined())
            query.addQueryItem(it.key(), jsonQueryValue(it.value()));
    }
    url.setQuery(query);

    QNetworkRequest request{url};
    request.setRawHeader("Accept", "application/json");
    QNetworkReply *reply = m_network.get(request);
    SailVaultNetwork::armTimeout(reply);

    connect(reply, &QNetworkReply::finished, this,
            [this, reply, generation]() {
        const QByteArray payload = reply->readAll();
        const QString networkError = networkErrorText(reply);
        reply->deleteLater();

        if (generation != m_generation)
            return;

        if (!networkError.isEmpty()) {
            finishOlderRequest(QStringLiteral("Ethereum"), false,
                               networkError, 0, generation);
            return;
        }

        QJsonParseError parseError;
        const QJsonDocument doc = QJsonDocument::fromJson(payload, &parseError);
        if (parseError.error != QJsonParseError::NoError || !doc.isObject()) {
            finishOlderRequest(QStringLiteral("Ethereum"), false,
                               QStringLiteral("invalid JSON response"),
                               0, generation);
            return;
        }

        const QJsonObject object = doc.object();
        const int added = appendEthereumItems(
            object.value(QStringLiteral("items")).toArray(), m_ethAddress);

        const QJsonValue next = object.value(QStringLiteral("next_page_params"));
        m_ethNextPageParams = next.isObject() ? next.toObject() : QJsonObject();
        m_ethHasMore = !m_ethNextPageParams.isEmpty();
        updateChainStatus(QStringLiteral("Ethereum"));
        finishOlderRequest(QStringLiteral("Ethereum"), true,
                           QString(), added, generation);
    });
}

void ActivityService::loadOlderBitcoin(quint64 generation)
{
    const QString endpoint =
        normalizeBaseUrl(setting(kBitcoinApiKey, kDefaultBitcoinApi))
        + QStringLiteral("/address/")
        + QString::fromUtf8(QUrl::toPercentEncoding(m_btcAddress))
        + QStringLiteral("/txs/chain/")
        + QString::fromUtf8(QUrl::toPercentEncoding(m_btcCursor));

    QNetworkRequest request{QUrl(endpoint)};
    request.setRawHeader("Accept", "application/json");
    QNetworkReply *reply = m_network.get(request);
    SailVaultNetwork::armTimeout(reply);

    connect(reply, &QNetworkReply::finished, this,
            [this, reply, generation]() {
        const QByteArray payload = reply->readAll();
        const QString networkError = networkErrorText(reply);
        reply->deleteLater();

        if (generation != m_generation)
            return;

        if (!networkError.isEmpty()) {
            finishOlderRequest(QStringLiteral("Bitcoin"), false,
                               networkError, 0, generation);
            return;
        }

        QJsonParseError parseError;
        const QJsonDocument doc = QJsonDocument::fromJson(payload, &parseError);
        if (parseError.error != QJsonParseError::NoError || !doc.isArray()) {
            finishOlderRequest(QStringLiteral("Bitcoin"), false,
                               QStringLiteral("invalid JSON response"),
                               0, generation);
            return;
        }

        const QJsonArray items = doc.array();
        const int added = appendBitcoinItems(items, m_btcAddress);

        if (!items.isEmpty())
            m_btcCursor = items.at(items.size() - 1).toObject().value(QStringLiteral("txid")).toString();
        m_btcHasMore = items.size() >= kBitcoinPageSize && !m_btcCursor.isEmpty();
        updateChainStatus(QStringLiteral("Bitcoin"));
        finishOlderRequest(QStringLiteral("Bitcoin"), true,
                           QString(), added, generation);
    });
}

void ActivityService::loadOlderSolana(quint64 generation)
{
    QNetworkRequest request{QUrl(setting(kSolanaRpcKey, kDefaultSolanaRpc))};
    request.setHeader(QNetworkRequest::ContentTypeHeader,
                      QStringLiteral("application/json"));
    request.setRawHeader("Accept", "application/json");

    QJsonObject config;
    config.insert(QStringLiteral("commitment"), QStringLiteral("finalized"));
    config.insert(QStringLiteral("limit"), kSolanaPageSize);
    config.insert(QStringLiteral("before"), m_solCursor);

    QJsonArray params;
    params.append(m_solAddress);
    params.append(config);

    QJsonObject body;
    body.insert(QStringLiteral("jsonrpc"), QStringLiteral("2.0"));
    body.insert(QStringLiteral("id"), 1);
    body.insert(QStringLiteral("method"), QStringLiteral("getSignaturesForAddress"));
    body.insert(QStringLiteral("params"), params);

    QNetworkReply *reply =
        m_network.post(request,
                       QJsonDocument(body).toJson(QJsonDocument::Compact));
    SailVaultNetwork::armTimeout(reply);

    connect(reply, &QNetworkReply::finished, this,
            [this, reply, generation]() {
        const QByteArray payload = reply->readAll();
        const QString networkError = networkErrorText(reply);
        reply->deleteLater();

        if (generation != m_generation)
            return;

        if (!networkError.isEmpty()) {
            finishOlderRequest(QStringLiteral("Solana"), false,
                               networkError, 0, generation);
            return;
        }

        QJsonParseError parseError;
        const QJsonDocument doc = QJsonDocument::fromJson(payload, &parseError);
        if (parseError.error != QJsonParseError::NoError || !doc.isObject()) {
            finishOlderRequest(QStringLiteral("Solana"), false,
                               QStringLiteral("invalid JSON response"),
                               0, generation);
            return;
        }

        const QJsonObject object = doc.object();
        if (object.contains(QStringLiteral("error"))) {
            const QJsonObject error = object.value(QStringLiteral("error")).toObject();
            finishOlderRequest(
                QStringLiteral("Solana"), false,
                error.value(QStringLiteral("message")).toString(
                    QStringLiteral("RPC error")),
                0, generation);
            return;
        }

        const QJsonArray items = object.value(QStringLiteral("result")).toArray();
        const int added = appendSolanaItems(items);

        if (!items.isEmpty())
            m_solCursor = items.at(items.size() - 1).toObject().value(QStringLiteral("signature")).toString();
        m_solHasMore = items.size() >= kSolanaPageSize && !m_solCursor.isEmpty();
        updateChainStatus(QStringLiteral("Solana"));
        finishOlderRequest(QStringLiteral("Solana"), true,
                           QString(), added, generation);
    });
}

void ActivityService::finishOlderRequest(const QString &chain,
                                         bool success,
                                         const QString &error,
                                         int added,
                                         quint64 generation)
{
    if (generation != m_generation || !m_loadingOlder)
        return;

    if (success) {
        ++m_olderSucceeded;
        m_olderAdded += added;
    } else {
        m_olderErrors.append(QStringLiteral("%1: %2").arg(chain, error));
    }

    --m_olderOutstanding;
    if (m_olderOutstanding > 0) {
        emit stateChanged();
        return;
    }

    sortEntries();
    m_loadingOlder = false;
    m_lastRefreshPassed = (m_olderSucceeded == m_olderRequested);
    m_lastUpdated = QDateTime::currentDateTime()
        .toString(QStringLiteral("yyyy-MM-dd hh:mm:ss"));

    if (m_olderSucceeded == m_olderRequested) {
        m_status = m_olderAdded > 0
            ? QStringLiteral("PASS · Loaded %1 older transaction%2")
                  .arg(m_olderAdded)
                  .arg(m_olderAdded == 1 ? QString() : QStringLiteral("s"))
            : QStringLiteral("PASS · No older activity remains on the queried pages");
    } else if (m_olderSucceeded > 0) {
        m_status = QStringLiteral("PARTIAL · Loaded %1 older · %2")
            .arg(m_olderAdded)
            .arg(m_olderErrors.join(QStringLiteral("; ")));
    } else {
        m_status = QStringLiteral("FAIL · Older activity could not be loaded · %1")
            .arg(m_olderErrors.join(QStringLiteral("; ")));
    }

    emit stateChanged();
}

void ActivityService::setChainFailure(const QString &chain,
                                      const QString &message,
                                      quint64 generation)
{
    if (generation != m_generation)
        return;

    const QString text = QStringLiteral("FAIL · %1").arg(message);

    if (chain == QStringLiteral("Ethereum")) {
        m_ethDone = true;
        m_ethOk = false;
        m_ethHasMore = false;
        m_ethereumStatus = text;
    } else if (chain == QStringLiteral("Bitcoin")) {
        m_btcDone = true;
        m_btcOk = false;
        m_btcHasMore = false;
        m_bitcoinStatus = text;
    } else {
        m_solDone = true;
        m_solOk = false;
        m_solHasMore = false;
        m_solanaStatus = text;
    }

    finishIfReady(generation);
}

void ActivityService::finishIfReady(quint64 generation)
{
    if (generation != m_generation
            || !m_ethDone || !m_btcDone || !m_solDone) {
        return;
    }

    sortEntries();
    m_loading = false;

    m_lastRefreshPassed =
        (!m_ethRequested || m_ethOk)
        && (!m_btcRequested || m_btcOk)
        && (!m_solRequested || m_solOk);

    m_lastUpdated = QDateTime::currentDateTime()
        .toString(QStringLiteral("yyyy-MM-dd hh:mm:ss"));

    const int requested =
        (m_ethRequested ? 1 : 0)
        + (m_btcRequested ? 1 : 0)
        + (m_solRequested ? 1 : 0);

    const int successes =
        (m_ethRequested && m_ethOk ? 1 : 0)
        + (m_btcRequested && m_btcOk ? 1 : 0)
        + (m_solRequested && m_solOk ? 1 : 0);

    if (m_lastRefreshPassed) {
        m_status = QStringLiteral("PASS · Activity history refreshed · %1 loaded")
            .arg(m_entries.size());
    } else if (successes > 0) {
        m_status =
            QStringLiteral("PARTIAL · Activity refreshed on %1 of %2 configured chains · %3 loaded")
                .arg(successes)
                .arg(requested)
                .arg(m_entries.size());
    } else {
        m_status = QStringLiteral(
            "FAIL · No configured activity provider completed successfully");
    }

    emit stateChanged();
}
