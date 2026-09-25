#include "portfolioservice.h"
#include "settingsstore.h"
#include "networkrequestutils.h"

#include <QCryptographicHash>
#include <QDateTime>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QStringList>
#include <QTimer>
#include <QUrl>

#include <boost/multiprecision/cpp_int.hpp>

namespace {

const char kEthereumRpcKey[] = "network/ethereumRpcUrl";
const char kEthereumExplorerKey[] = "network/ethereumExplorerUrl";
const char kBitcoinApiKey[] = "network/bitcoinApiUrl";
const char kSolanaRpcKey[] = "network/solanaRpcUrl";
const char kSolanaTokenRpcKey[] = "network/solanaTokenRpcUrl";
const char kAutoRefreshAfterUnlockKey[] = "portfolio/autoRefreshAfterUnlock";
const char kOfflineModeKey[] = "network/offlineMode";

const char kDefaultEthereumRpc[] = "https://ethereum-rpc.publicnode.com";
const char kDefaultEthereumExplorer[] = "https://eth.blockscout.com/api/v2";
const char kDefaultBitcoinApi[] = "https://blockstream.info/api";
const char kDefaultSolanaRpc[] = "https://solana-rpc.publicnode.com";
const char kDefaultSolanaTokenRpc[] = "https://api.mainnet.solana.com";

QString replyError(QNetworkReply *reply)
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

QString trimDecimalFraction(QString value)
{
    while (value.contains(QLatin1Char('.')) && value.endsWith(QLatin1Char('0')))
        value.chop(1);
    if (value.endsWith(QLatin1Char('.')))
        value.chop(1);
    return value;
}

} // namespace

PortfolioService::PortfolioService(QObject *parent)
    : QObject(parent)
    , m_status(QStringLiteral(
          "Load the development wallet, then refresh read-only balances."))
{
}

bool PortfolioService::loading() const
{
    return m_loading;
}

bool PortfolioService::lastRefreshPassed() const
{
    return m_lastRefreshPassed;
}

QString PortfolioService::status() const
{
    return m_status;
}

QString PortfolioService::ethereumBalance() const
{
    return m_ethereumBalance;
}

QString PortfolioService::bitcoinBalance() const
{
    return m_bitcoinBalance;
}

QString PortfolioService::solanaBalance() const
{
    return m_solanaBalance;
}

QString PortfolioService::ethereumStatus() const
{
    return m_ethereumStatus;
}

QString PortfolioService::bitcoinStatus() const
{
    return m_bitcoinStatus;
}

QString PortfolioService::solanaStatus() const
{
    return m_solanaStatus;
}

QString PortfolioService::ethereumUpdated() const
{
    return m_ethereumUpdated;
}

QString PortfolioService::bitcoinUpdated() const
{
    return m_bitcoinUpdated;
}

QString PortfolioService::solanaUpdated() const
{
    return m_solanaUpdated;
}

QString PortfolioService::lastUpdated() const
{
    return m_lastUpdated;
}

bool PortfolioService::usingCachedData() const
{
    return m_usingCachedData;
}

QString PortfolioService::ethereumRpcUrl() const
{
    return SailVaultSettings::value(
        QString::fromLatin1(kEthereumRpcKey),
        QString::fromLatin1(kDefaultEthereumRpc)).toString();
}

QString PortfolioService::ethereumExplorerUrl() const
{
    return SailVaultSettings::value(
        QString::fromLatin1(kEthereumExplorerKey),
        QString::fromLatin1(kDefaultEthereumExplorer)).toString();
}

QString PortfolioService::bitcoinApiUrl() const
{
    return SailVaultSettings::value(
        QString::fromLatin1(kBitcoinApiKey),
        QString::fromLatin1(kDefaultBitcoinApi)).toString();
}

QString PortfolioService::solanaRpcUrl() const
{
    return SailVaultSettings::value(
        QString::fromLatin1(kSolanaRpcKey),
        QString::fromLatin1(kDefaultSolanaRpc)).toString();
}

QString PortfolioService::solanaTokenRpcUrl() const
{
    return SailVaultSettings::value(
        QString::fromLatin1(kSolanaTokenRpcKey),
        QString::fromLatin1(kDefaultSolanaTokenRpc)).toString();
}

QString PortfolioService::snapshotId(const QString &ethereumAddress,
                                     const QString &bitcoinAddress,
                                     const QString &solanaAddress)
{
    const QByteArray source =
        ethereumAddress.trimmed().toUtf8()
        + QByteArray("|")
        + bitcoinAddress.trimmed().toUtf8()
        + QByteArray("|")
        + solanaAddress.trimmed().toUtf8();

    if (source == QByteArray("||"))
        return QString();

    return QString::fromLatin1(
        QCryptographicHash::hash(source, QCryptographicHash::Sha256).toHex());
}

QString PortfolioService::addressCacheBase(const QString &chain,
                                               const QString &address) const
{
    const QString normalizedChain = chain.trimmed();
    QString normalizedAddress = address.trimmed();
    if (normalizedChain.isEmpty() || normalizedAddress.isEmpty())
        return QString();

    // Ethereum hexadecimal addresses are case-insensitive at the protocol
    // level; normalize casing so checksum/lowercase representations share the
    // same exact-address cache entry. Bitcoin and Solana remain unchanged.
    if (normalizedChain == QStringLiteral("Ethereum"))
        normalizedAddress = normalizedAddress.toLower();

    QByteArray source = normalizedChain.toUtf8();
    source.append('|');
    source.append(normalizedAddress.toUtf8());
    const QString id = QString::fromLatin1(
        QCryptographicHash::hash(source, QCryptographicHash::Sha256).toHex());
    return QStringLiteral("publicBalanceCache/") + id + QLatin1Char('/');
}

void PortfolioService::saveAddressCache(const QString &chain,
                                        const QString &address,
                                        const QString &balance,
                                        const QString &updated)
{
    const QString base = addressCacheBase(chain, address);
    if (base.isEmpty() || updated.isEmpty()
            || balance.isEmpty() || balance == QStringLiteral("—")) {
        return;
    }

    SailVaultSettings::setValue(base + QStringLiteral("balance"), balance);
    SailVaultSettings::setValue(base + QStringLiteral("lastUpdated"), updated);
}

QString PortfolioService::cachedBalance(const QString &chain,
                                        const QString &address) const
{
    const QString normalizedChain = chain.trimmed();
    const QString normalizedAddress = address.trimmed();
    if (normalizedAddress.isEmpty())
        return QString();

    const QString addressBase = addressCacheBase(normalizedChain, normalizedAddress);
    if (!addressBase.isEmpty()) {
        const QString updated = SailVaultSettings::value(
            addressBase + QStringLiteral("lastUpdated"), QString()).toString();
        const QString balance = SailVaultSettings::value(
            addressBase + QStringLiteral("balance"), QString()).toString();
        if (!updated.isEmpty() && !balance.isEmpty()
                && balance != QStringLiteral("—")) {
            return balance;
        }
    }

    // Legacy M26/M27 single-address snapshot fallback.
    QString id;
    QString key;
    if (normalizedChain == QStringLiteral("Ethereum")) {
        id = snapshotId(normalizedAddress, QString(), QString());
        key = QStringLiteral("ethereumBalance");
    } else if (normalizedChain == QStringLiteral("Bitcoin")) {
        id = snapshotId(QString(), normalizedAddress, QString());
        key = QStringLiteral("bitcoinBalance");
    } else if (normalizedChain == QStringLiteral("Solana")) {
        id = snapshotId(QString(), QString(), normalizedAddress);
        key = QStringLiteral("solanaBalance");
    } else {
        return QString();
    }

    if (id.isEmpty())
        return QString();

    const QString base = QStringLiteral("portfolioCache/")
        + id + QLatin1Char('/');
    const QString updated = SailVaultSettings::value(
        base + QStringLiteral("lastUpdated"), QString()).toString();
    if (updated.isEmpty())
        return QString();

    const QString balance = SailVaultSettings::value(
        base + key, QString()).toString();
    return balance == QStringLiteral("—") ? QString() : balance;
}

QString PortfolioService::cachedBalanceUpdated(const QString &chain,
                                               const QString &address) const
{
    const QString normalizedChain = chain.trimmed();
    const QString normalizedAddress = address.trimmed();
    if (normalizedAddress.isEmpty())
        return QString();

    const QString addressBase = addressCacheBase(normalizedChain, normalizedAddress);
    if (!addressBase.isEmpty()) {
        const QString updated = SailVaultSettings::value(
            addressBase + QStringLiteral("lastUpdated"), QString()).toString();
        if (!updated.isEmpty())
            return updated;
    }

    // Legacy M26/M27 single-address snapshot fallback.
    QString id;
    if (normalizedChain == QStringLiteral("Ethereum"))
        id = snapshotId(normalizedAddress, QString(), QString());
    else if (normalizedChain == QStringLiteral("Bitcoin"))
        id = snapshotId(QString(), normalizedAddress, QString());
    else if (normalizedChain == QStringLiteral("Solana"))
        id = snapshotId(QString(), QString(), normalizedAddress);
    else
        return QString();

    if (id.isEmpty())
        return QString();

    const QString base = QStringLiteral("portfolioCache/")
        + id + QLatin1Char('/');
    return SailVaultSettings::value(
        base + QStringLiteral("lastUpdated"), QString()).toString();
}

void PortfolioService::loadSnapshot(const QString &ethereumAddress,
                                    const QString &bitcoinAddress,
                                    const QString &solanaAddress)
{
    m_snapshotId = snapshotId(ethereumAddress,
                              bitcoinAddress,
                              solanaAddress);

    if (m_snapshotId.isEmpty())
        return;

    const QString base = QStringLiteral("portfolioCache/")
        + m_snapshotId + QLatin1Char('/');

    const QString cachedUpdated =
        SailVaultSettings::value(base + QStringLiteral("lastUpdated"),
                                 QString()).toString();

    const QString eth = ethereumAddress.trimmed();
    const QString btc = bitcoinAddress.trimmed();
    const QString sol = solanaAddress.trimmed();

    m_ethAddress = eth;
    m_btcAddress = btc;
    m_solAddress = sol;

    // Prefer the exact-address cache. Since M31 each successful chain refresh
    // is persisted independently, even if another provider fails. Fall back to
    // the older grouped cache for upgrades from M30 and earlier.
    const QString exactEth = cachedBalance(QStringLiteral("Ethereum"), eth);
    const QString exactBtc = cachedBalance(QStringLiteral("Bitcoin"), btc);
    const QString exactSol = cachedBalance(QStringLiteral("Solana"), sol);

    m_ethereumBalance = eth.isEmpty()
        ? QStringLiteral("—")
        : (!exactEth.isEmpty() ? exactEth
            : SailVaultSettings::value(
                  base + QStringLiteral("ethereumBalance"),
                  QStringLiteral("—")).toString());

    m_bitcoinBalance = btc.isEmpty()
        ? QStringLiteral("—")
        : (!exactBtc.isEmpty() ? exactBtc
            : SailVaultSettings::value(
                  base + QStringLiteral("bitcoinBalance"),
                  QStringLiteral("—")).toString());

    m_solanaBalance = sol.isEmpty()
        ? QStringLiteral("—")
        : (!exactSol.isEmpty() ? exactSol
            : SailVaultSettings::value(
                  base + QStringLiteral("solanaBalance"),
                  QStringLiteral("—")).toString());

    m_ethereumUpdated = eth.isEmpty() ? QString()
        : cachedBalanceUpdated(QStringLiteral("Ethereum"), eth);
    m_bitcoinUpdated = btc.isEmpty() ? QString()
        : cachedBalanceUpdated(QStringLiteral("Bitcoin"), btc);
    m_solanaUpdated = sol.isEmpty() ? QString()
        : cachedBalanceUpdated(QStringLiteral("Solana"), sol);

    if (m_ethereumUpdated.isEmpty() && m_ethereumBalance != QStringLiteral("—"))
        m_ethereumUpdated = cachedUpdated;
    if (m_bitcoinUpdated.isEmpty() && m_bitcoinBalance != QStringLiteral("—"))
        m_bitcoinUpdated = cachedUpdated;
    if (m_solanaUpdated.isEmpty() && m_solanaBalance != QStringLiteral("—"))
        m_solanaUpdated = cachedUpdated;

    m_ethereumStatus = eth.isEmpty() ? QStringLiteral("Not configured")
        : (m_ethereumBalance == QStringLiteral("—")
           ? QStringLiteral("No cached value") : QStringLiteral("Cached"));
    m_bitcoinStatus = btc.isEmpty() ? QStringLiteral("Not configured")
        : (m_bitcoinBalance == QStringLiteral("—")
           ? QStringLiteral("No cached value") : QStringLiteral("Cached"));
    m_solanaStatus = sol.isEmpty() ? QStringLiteral("Not configured")
        : (m_solanaBalance == QStringLiteral("—")
           ? QStringLiteral("No cached value") : QStringLiteral("Cached"));

    QStringList updatedValues;
    if (!m_ethereumUpdated.isEmpty()) updatedValues << m_ethereumUpdated;
    if (!m_bitcoinUpdated.isEmpty()) updatedValues << m_bitcoinUpdated;
    if (!m_solanaUpdated.isEmpty()) updatedValues << m_solanaUpdated;
    updatedValues.sort();
    m_lastUpdated = updatedValues.isEmpty() ? cachedUpdated : updatedValues.last();

    const bool hasAny = m_ethereumBalance != QStringLiteral("—")
        || m_bitcoinBalance != QStringLiteral("—")
        || m_solanaBalance != QStringLiteral("—");
    if (!hasAny)
        return;

    const bool cacheComplete =
        (eth.isEmpty() || m_ethereumBalance != QStringLiteral("—"))
        && (btc.isEmpty() || m_bitcoinBalance != QStringLiteral("—"))
        && (sol.isEmpty() || m_solanaBalance != QStringLiteral("—"));
    m_lastRefreshPassed = cacheComplete;
    m_usingCachedData = true;
    m_status = cacheComplete
        ? QStringLiteral("Cached balances · refresh to verify current values")
        : QStringLiteral("Partial cached balances · refresh to fill missing values");

    // M28 migration/index: expose balances cached by a normal multi-chain
    // portfolio snapshot through the exact-address cache as well. M31 keeps
    // each chain's own verification timestamp instead of flattening them.
    saveAddressCache(QStringLiteral("Ethereum"), eth,
                     m_ethereumBalance, m_ethereumUpdated);
    saveAddressCache(QStringLiteral("Bitcoin"), btc,
                     m_bitcoinBalance, m_bitcoinUpdated);
    saveAddressCache(QStringLiteral("Solana"), sol,
                     m_solanaBalance, m_solanaUpdated);

    emit stateChanged();
}

void PortfolioService::saveSnapshot()
{
    if (m_snapshotId.isEmpty() || !m_lastRefreshPassed)
        return;

    const QString base = QStringLiteral("portfolioCache/")
        + m_snapshotId + QLatin1Char('/');

    SailVaultSettings::setValue(
        base + QStringLiteral("ethereumBalance"), m_ethereumBalance);
    SailVaultSettings::setValue(
        base + QStringLiteral("bitcoinBalance"), m_bitcoinBalance);
    SailVaultSettings::setValue(
        base + QStringLiteral("solanaBalance"), m_solanaBalance);
    SailVaultSettings::setValue(
        base + QStringLiteral("lastUpdated"), m_lastUpdated);

    // Keep an exact-address index in parallel with the grouped snapshot so
    // Address book can reuse already-known balances without requiring a
    // separate per-address refresh.
    saveAddressCache(QStringLiteral("Ethereum"), m_ethAddress,
                     m_ethereumBalance, m_lastUpdated);
    saveAddressCache(QStringLiteral("Bitcoin"), m_btcAddress,
                     m_bitcoinBalance, m_lastUpdated);
    saveAddressCache(QStringLiteral("Solana"), m_solAddress,
                     m_solanaBalance, m_lastUpdated);
}

QString PortfolioService::normalizeBaseUrl(const QString &url)
{
    QString normalized = url.trimmed();
    while (normalized.endsWith(QLatin1Char('/')))
        normalized.chop(1);
    return normalized;
}

static bool isValidHttpsEndpoint(const QString &url)
{
    const QUrl parsed(url);
    return parsed.isValid()
        && parsed.scheme().compare(QStringLiteral("https"), Qt::CaseInsensitive) == 0
        && !parsed.host().isEmpty();
}

void PortfolioService::setEthereumRpcUrl(const QString &url)
{
    const QString normalized = normalizeBaseUrl(url);
    if (!isValidHttpsEndpoint(normalized) || normalized == ethereumRpcUrl())
        return;

    SailVaultSettings::setValue(
        QString::fromLatin1(kEthereumRpcKey), normalized);
    emit settingsChanged();
}

void PortfolioService::setEthereumExplorerUrl(const QString &url)
{
    const QString normalized = normalizeBaseUrl(url);
    if (!isValidHttpsEndpoint(normalized) || normalized == ethereumExplorerUrl())
        return;

    SailVaultSettings::setValue(
        QString::fromLatin1(kEthereumExplorerKey), normalized);
    emit settingsChanged();
}

void PortfolioService::setBitcoinApiUrl(const QString &url)
{
    const QString normalized = normalizeBaseUrl(url);
    if (!isValidHttpsEndpoint(normalized) || normalized == bitcoinApiUrl())
        return;

    SailVaultSettings::setValue(
        QString::fromLatin1(kBitcoinApiKey), normalized);
    emit settingsChanged();
}

void PortfolioService::setSolanaRpcUrl(const QString &url)
{
    const QString normalized = normalizeBaseUrl(url);
    if (!isValidHttpsEndpoint(normalized) || normalized == solanaRpcUrl())
        return;

    SailVaultSettings::setValue(
        QString::fromLatin1(kSolanaRpcKey), normalized);
    emit settingsChanged();
}

void PortfolioService::setSolanaTokenRpcUrl(const QString &url)
{
    const QString normalized = normalizeBaseUrl(url);
    if (!isValidHttpsEndpoint(normalized) || normalized == solanaTokenRpcUrl())
        return;

    SailVaultSettings::setValue(
        QString::fromLatin1(kSolanaTokenRpcKey), normalized);
    emit settingsChanged();
}

bool PortfolioService::autoRefreshAfterUnlock() const
{
    return SailVaultSettings::value(
        QString::fromLatin1(kAutoRefreshAfterUnlockKey), true).toBool();
}

void PortfolioService::setAutoRefreshAfterUnlock(bool enabled)
{
    if (enabled == autoRefreshAfterUnlock())
        return;

    SailVaultSettings::setValue(
        QString::fromLatin1(kAutoRefreshAfterUnlockKey), enabled);
    emit settingsChanged();
}

bool PortfolioService::offlineMode() const
{
    return SailVaultSettings::value(
        QString::fromLatin1(kOfflineModeKey), false).toBool();
}

void PortfolioService::setOfflineMode(bool enabled)
{
    if (offlineMode() == enabled)
        return;

    SailVaultSettings::setValue(QString::fromLatin1(kOfflineModeKey), enabled);

    if (enabled) {
        ++m_generation;
        SailVaultNetwork::cancelOutstanding(&m_network);
        m_loading = false;
        m_lastRefreshPassed = false;
        m_usingCachedData = (m_ethereumBalance != QStringLiteral("—")
                             || m_bitcoinBalance != QStringLiteral("—")
                             || m_solanaBalance != QStringLiteral("—"));
        m_status = m_usingCachedData
            ? QStringLiteral("Offline mode enabled · using last-known balances")
            : QStringLiteral("Offline mode enabled · no provider requests will be sent");
        if (m_ethRequested)
            m_ethereumStatus = m_ethereumBalance != QStringLiteral("—")
                ? QStringLiteral("Offline · cached") : QStringLiteral("Offline");
        if (m_btcRequested)
            m_bitcoinStatus = m_bitcoinBalance != QStringLiteral("—")
                ? QStringLiteral("Offline · cached") : QStringLiteral("Offline");
        if (m_solRequested)
            m_solanaStatus = m_solanaBalance != QStringLiteral("—")
                ? QStringLiteral("Offline · cached") : QStringLiteral("Offline");
        emit stateChanged();
    }

    emit settingsChanged();
}

void PortfolioService::resetEndpoints()
{
    SailVaultSettings::remove(QStringList()
        << QString::fromLatin1(kEthereumRpcKey)
        << QString::fromLatin1(kEthereumExplorerKey)
        << QString::fromLatin1(kBitcoinApiKey)
        << QString::fromLatin1(kSolanaRpcKey)
        << QString::fromLatin1(kSolanaTokenRpcKey));
    emit settingsChanged();
}

QString PortfolioService::formatBitcoin(qint64 satoshis)
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

QString PortfolioService::formatSolana(qint64 lamports)
{
    const bool negative = lamports < 0;
    quint64 value = negative
        ? static_cast<quint64>(-(lamports + 1)) + 1
        : static_cast<quint64>(lamports);

    const quint64 whole = value / 1000000000ULL;
    const quint64 fraction = value % 1000000000ULL;

    QString result = QStringLiteral("%1.%2")
        .arg(whole)
        .arg(fraction, 9, 10, QLatin1Char('0'));

    result = trimDecimalFraction(result);
    if (!result.contains(QLatin1Char('.')))
        result += QStringLiteral(".0");
    if (negative)
        result.prepend(QLatin1Char('-'));

    return result + QStringLiteral(" SOL");
}

QString PortfolioService::formatEthereumWeiHex(const QString &hexWei)
{
    QString hex = hexWei.trimmed();
    if (hex.startsWith(QStringLiteral("0x"), Qt::CaseInsensitive))
        hex.remove(0, 2);

    if (hex.isEmpty())
        return QStringLiteral("0.0 ETH");

    boost::multiprecision::cpp_int wei = 0;
    for (const QChar ch : hex) {
        int digit = -1;
        if (ch >= QLatin1Char('0') && ch <= QLatin1Char('9'))
            digit = ch.unicode() - QLatin1Char('0').unicode();
        else if (ch >= QLatin1Char('a') && ch <= QLatin1Char('f'))
            digit = 10 + ch.unicode() - QLatin1Char('a').unicode();
        else if (ch >= QLatin1Char('A') && ch <= QLatin1Char('F'))
            digit = 10 + ch.unicode() - QLatin1Char('A').unicode();

        if (digit < 0)
            return QString();

        wei <<= 4;
        wei += digit;
    }

    const boost::multiprecision::cpp_int divisor(
        "1000000000000000000");
    const boost::multiprecision::cpp_int whole = wei / divisor;
    boost::multiprecision::cpp_int fraction = wei % divisor;

    std::string wholeString = whole.convert_to<std::string>();
    std::string fractionString = fraction.convert_to<std::string>();

    QString fractionQt = QString::fromStdString(fractionString)
        .rightJustified(18, QLatin1Char('0'));

    // Eight decimals is enough for the wallet overview while keeping the
    // calculation itself exact. Do not round upward from hidden digits.
    fractionQt = fractionQt.left(8);

    QString result = QString::fromStdString(wholeString)
        + QLatin1Char('.')
        + fractionQt;
    result = trimDecimalFraction(result);
    if (!result.contains(QLatin1Char('.')))
        result += QStringLiteral(".0");

    return result + QStringLiteral(" ETH");
}

void PortfolioService::refresh(const QString &ethereumAddress,
                               const QString &bitcoinAddress,
                               const QString &solanaAddress)
{
    const QString eth = ethereumAddress.trimmed();
    const QString btc = bitcoinAddress.trimmed();
    const QString sol = solanaAddress.trimmed();

    m_snapshotId = snapshotId(eth, btc, sol);
    m_ethAddress = eth;
    m_btcAddress = btc;
    m_solAddress = sol;

    m_ethRequested = !eth.isEmpty();
    m_btcRequested = !btc.isEmpty();
    m_solRequested = !sol.isEmpty();

    if (!m_ethRequested && !m_btcRequested && !m_solRequested) {
        m_lastRefreshPassed = false;
        m_status = QStringLiteral(
            "Provide at least one public address before refreshing balances.");
        emit stateChanged();
        return;
    }

    if (offlineMode()) {
        ++m_generation;
        SailVaultNetwork::cancelOutstanding(&m_network);
        m_loading = false;
        m_lastRefreshPassed = false;
        m_usingCachedData = (m_ethereumBalance != QStringLiteral("—")
                             || m_bitcoinBalance != QStringLiteral("—")
                             || m_solanaBalance != QStringLiteral("—"));
        m_ethereumStatus = m_ethRequested
            ? (m_ethereumBalance != QStringLiteral("—")
               ? QStringLiteral("Offline · cached") : QStringLiteral("Offline"))
            : QStringLiteral("Not configured");
        m_bitcoinStatus = m_btcRequested
            ? (m_bitcoinBalance != QStringLiteral("—")
               ? QStringLiteral("Offline · cached") : QStringLiteral("Offline"))
            : QStringLiteral("Not configured");
        m_solanaStatus = m_solRequested
            ? (m_solanaBalance != QStringLiteral("—")
               ? QStringLiteral("Offline · cached") : QStringLiteral("Offline"))
            : QStringLiteral("Not configured");
        m_status = m_usingCachedData
            ? QStringLiteral("Offline mode · showing last-known balances")
            : QStringLiteral("Offline mode · no cached balances available");
        emit stateChanged();
        return;
    }

    const QUrl ethUrl(ethereumRpcUrl());
    const QUrl btcUrl(bitcoinApiUrl());
    const QUrl solUrl(solanaRpcUrl());

    if ((m_ethRequested
         && (!ethUrl.isValid()
             || ethUrl.scheme() != QStringLiteral("https")))
            || (m_btcRequested
                && (!btcUrl.isValid()
                    || btcUrl.scheme() != QStringLiteral("https")))
            || (m_solRequested
                && (!solUrl.isValid()
                    || solUrl.scheme() != QStringLiteral("https")))) {
        m_lastRefreshPassed = false;
        m_status = QStringLiteral(
            "Required network endpoints must be valid HTTPS URLs.");
        emit stateChanged();
        return;
    }

    ++m_generation;
    const quint64 generation = m_generation;
    SailVaultNetwork::cancelOutstanding(&m_network);

    m_loading = true;
    m_lastRefreshPassed = false;

    // Values remain visible while a refresh is in flight. If a provider later
    // fails, M31 keeps the last-known value instead of replacing it with an
    // ambiguous dash.
    m_usingCachedData = (m_ethereumBalance != QStringLiteral("—")
                         || m_bitcoinBalance != QStringLiteral("—")
                         || m_solanaBalance != QStringLiteral("—"));

    m_ethereumStatus = m_ethRequested ? QStringLiteral("Refreshing…")
                                      : QStringLiteral("Not configured");
    m_bitcoinStatus = m_btcRequested ? QStringLiteral("Refreshing…")
                                     : QStringLiteral("Not configured");
    m_solanaStatus = m_solRequested ? QStringLiteral("Refreshing…")
                                    : QStringLiteral("Not configured");

    m_ethDone = !m_ethRequested;
    m_btcDone = !m_btcRequested;
    m_solDone = !m_solRequested;

    m_ethOk = !m_ethRequested;
    m_btcOk = !m_btcRequested;
    m_solOk = !m_solRequested;

    m_ethError.clear();
    m_btcError.clear();
    m_solError.clear();

    if (!m_ethRequested)
        m_ethereumBalance = QStringLiteral("—");

    if (!m_btcRequested)
        m_bitcoinBalance = QStringLiteral("—");

    if (!m_solRequested)
        m_solanaBalance = QStringLiteral("—");

    m_status = QStringLiteral("Refreshing read-only balances…");
    emit stateChanged();

    if (m_ethRequested)
        refreshEthereum(eth, generation);

    if (m_btcRequested)
        refreshBitcoin(btc, generation);

    if (m_solRequested)
        refreshSolana(sol, generation);
}

void PortfolioService::refreshEthereum(const QString &address,
                                       quint64 generation,
                                       int attempt)
{
    if (generation != m_generation)
        return;
    QNetworkRequest request{QUrl(ethereumRpcUrl())};
    request.setHeader(QNetworkRequest::ContentTypeHeader,
                      QStringLiteral("application/json"));
    request.setRawHeader("Accept", "application/json");

    QJsonObject body;
    body.insert(QStringLiteral("jsonrpc"), QStringLiteral("2.0"));
    body.insert(QStringLiteral("method"), QStringLiteral("eth_getBalance"));

    QJsonArray params;
    params.append(address);
    params.append(QStringLiteral("latest"));
    body.insert(QStringLiteral("params"), params);
    body.insert(QStringLiteral("id"), 1);

    QNetworkReply *reply =
        m_network.post(request, QJsonDocument(body).toJson(QJsonDocument::Compact));
    SailVaultNetwork::armTimeout(reply);

    connect(reply, &QNetworkReply::finished, this,
            [this, reply, address, generation, attempt]() {
        const QNetworkReply::NetworkError errorCode = reply->error();
        const QString networkError = replyError(reply);
        reply->deleteLater();

        if (generation != m_generation)
            return;

        if (errorCode == QNetworkReply::HostNotFoundError && attempt < 2) {
            m_status = QStringLiteral(
                "Ethereum DNS lookup failed · retrying portfolio refresh…");
            emit stateChanged();

            QTimer::singleShot(1200 * (attempt + 1), this,
                               [this, address, generation, attempt]() {
                refreshEthereum(address, generation, attempt + 1);
            });
            return;
        }

        if (!networkError.isEmpty()) {
            failRequest(QStringLiteral("Ethereum"), networkError, generation);
            return;
        }

        QJsonParseError parseError;
        const QJsonDocument doc =
            QJsonDocument::fromJson(reply->readAll(), &parseError);

        if (parseError.error != QJsonParseError::NoError || !doc.isObject()) {
            failRequest(QStringLiteral("Ethereum"),
                        QStringLiteral("invalid JSON response"),
                        generation);
            return;
        }

        const QJsonObject object = doc.object();
        if (object.contains(QStringLiteral("error"))) {
            const QJsonObject error =
                object.value(QStringLiteral("error")).toObject();
            failRequest(
                QStringLiteral("Ethereum"),
                error.value(QStringLiteral("message")).toString(
                    QStringLiteral("RPC error")),
                generation);
            return;
        }

        const QString formatted =
            formatEthereumWeiHex(object.value(QStringLiteral("result")).toString());

        if (formatted.isEmpty()) {
            failRequest(QStringLiteral("Ethereum"),
                        QStringLiteral("invalid eth_getBalance result"),
                        generation);
            return;
        }

        m_ethereumBalance = formatted;
        m_ethereumStatus = QStringLiteral("OK");
        m_ethereumUpdated = QDateTime::currentDateTime()
            .toString(QStringLiteral("yyyy-MM-dd hh:mm:ss"));
        saveAddressCache(QStringLiteral("Ethereum"), address,
                         m_ethereumBalance, m_ethereumUpdated);
        m_ethOk = true;
        m_ethDone = true;
        finishRequest(generation);
    });
}

void PortfolioService::refreshBitcoin(const QString &address,
                                      quint64 generation,
                                      int attempt)
{
    if (generation != m_generation)
        return;
    const QString endpoint =
        normalizeBaseUrl(bitcoinApiUrl())
        + QStringLiteral("/address/")
        + QString::fromUtf8(QUrl::toPercentEncoding(address));

    QNetworkRequest request{QUrl(endpoint)};
    request.setRawHeader("Accept", "application/json");

    QNetworkReply *reply = m_network.get(request);
    SailVaultNetwork::armTimeout(reply);

    connect(reply, &QNetworkReply::finished, this,
            [this, reply, address, generation, attempt]() {
        const QNetworkReply::NetworkError errorCode = reply->error();
        const QString networkError = replyError(reply);
        reply->deleteLater();

        if (generation != m_generation)
            return;

        if (errorCode == QNetworkReply::HostNotFoundError && attempt < 2) {
            m_status = QStringLiteral(
                "Bitcoin DNS lookup failed · retrying portfolio refresh…");
            emit stateChanged();

            QTimer::singleShot(1200 * (attempt + 1), this,
                               [this, address, generation, attempt]() {
                refreshBitcoin(address, generation, attempt + 1);
            });
            return;
        }

        if (!networkError.isEmpty()) {
            failRequest(QStringLiteral("Bitcoin"), networkError, generation);
            return;
        }

        QJsonParseError parseError;
        const QJsonDocument doc =
            QJsonDocument::fromJson(reply->readAll(), &parseError);

        if (parseError.error != QJsonParseError::NoError || !doc.isObject()) {
            failRequest(QStringLiteral("Bitcoin"),
                        QStringLiteral("invalid JSON response"),
                        generation);
            return;
        }

        const QJsonObject object = doc.object();
        const QJsonObject chain =
            object.value(QStringLiteral("chain_stats")).toObject();
        const QJsonObject mempool =
            object.value(QStringLiteral("mempool_stats")).toObject();

        if (chain.isEmpty() || mempool.isEmpty()) {
            failRequest(QStringLiteral("Bitcoin"),
                        QStringLiteral("missing Esplora address statistics"),
                        generation);
            return;
        }

        const qint64 confirmed =
            static_cast<qint64>(
                chain.value(QStringLiteral("funded_txo_sum")).toDouble())
            - static_cast<qint64>(
                chain.value(QStringLiteral("spent_txo_sum")).toDouble());

        const qint64 unconfirmed =
            static_cast<qint64>(
                mempool.value(QStringLiteral("funded_txo_sum")).toDouble())
            - static_cast<qint64>(
                mempool.value(QStringLiteral("spent_txo_sum")).toDouble());

        m_bitcoinBalance = formatBitcoin(confirmed + unconfirmed);
        m_bitcoinStatus = QStringLiteral("OK");
        m_bitcoinUpdated = QDateTime::currentDateTime()
            .toString(QStringLiteral("yyyy-MM-dd hh:mm:ss"));
        saveAddressCache(QStringLiteral("Bitcoin"), address,
                         m_bitcoinBalance, m_bitcoinUpdated);
        m_btcOk = true;
        m_btcDone = true;
        finishRequest(generation);
    });
}

void PortfolioService::refreshSolana(const QString &address,
                                      quint64 generation,
                                      int attempt)
{
    if (generation != m_generation)
        return;
    QNetworkRequest request{QUrl(solanaRpcUrl())};
    request.setHeader(QNetworkRequest::ContentTypeHeader,
                      QStringLiteral("application/json"));
    request.setRawHeader("Accept", "application/json");

    QJsonObject config;
    config.insert(QStringLiteral("commitment"), QStringLiteral("finalized"));

    QJsonArray params;
    params.append(address);
    params.append(config);

    QJsonObject body;
    body.insert(QStringLiteral("jsonrpc"), QStringLiteral("2.0"));
    body.insert(QStringLiteral("id"), 1);
    body.insert(QStringLiteral("method"), QStringLiteral("getBalance"));
    body.insert(QStringLiteral("params"), params);

    QNetworkReply *reply =
        m_network.post(request, QJsonDocument(body).toJson(QJsonDocument::Compact));
    SailVaultNetwork::armTimeout(reply);

    connect(reply, &QNetworkReply::finished, this,
            [this, reply, address, generation, attempt]() {
        const QNetworkReply::NetworkError errorCode = reply->error();
        const QString networkError = replyError(reply);
        reply->deleteLater();

        if (generation != m_generation)
            return;

        if (errorCode == QNetworkReply::HostNotFoundError && attempt < 2) {
            m_status = QStringLiteral(
                "Solana DNS lookup failed · retrying portfolio refresh…");
            emit stateChanged();

            QTimer::singleShot(1200 * (attempt + 1), this,
                               [this, address, generation, attempt]() {
                refreshSolana(address, generation, attempt + 1);
            });
            return;
        }

        if (!networkError.isEmpty()) {
            failRequest(QStringLiteral("Solana"), networkError, generation);
            return;
        }

        QJsonParseError parseError;
        const QJsonDocument doc =
            QJsonDocument::fromJson(reply->readAll(), &parseError);

        if (parseError.error != QJsonParseError::NoError || !doc.isObject()) {
            failRequest(QStringLiteral("Solana"),
                        QStringLiteral("invalid JSON response"),
                        generation);
            return;
        }

        const QJsonObject object = doc.object();
        if (object.contains(QStringLiteral("error"))) {
            const QJsonObject error =
                object.value(QStringLiteral("error")).toObject();
            failRequest(QStringLiteral("Solana"),
                        error.value(QStringLiteral("message")).toString(
                            QStringLiteral("RPC error")),
                        generation);
            return;
        }

        const QJsonObject result =
            object.value(QStringLiteral("result")).toObject();
        const QJsonValue value = result.value(QStringLiteral("value"));
        if (!value.isDouble()) {
            failRequest(QStringLiteral("Solana"),
                        QStringLiteral("invalid getBalance result"),
                        generation);
            return;
        }

        m_solanaBalance =
            formatSolana(static_cast<qint64>(value.toDouble()));
        m_solanaStatus = QStringLiteral("OK");
        m_solanaUpdated = QDateTime::currentDateTime()
            .toString(QStringLiteral("yyyy-MM-dd hh:mm:ss"));
        saveAddressCache(QStringLiteral("Solana"), address,
                         m_solanaBalance, m_solanaUpdated);
        m_solOk = true;
        m_solDone = true;
        finishRequest(generation);
    });
}

void PortfolioService::failRequest(const QString &chain,
                                   const QString &message,
                                   quint64 generation)
{
    if (generation != m_generation)
        return;

    if (chain == QStringLiteral("Ethereum")) {
        m_ethDone = true;
        m_ethOk = false;
        m_ethError = message;
        m_ethereumStatus = QStringLiteral("FAIL · %1").arg(message);
        if (m_ethereumBalance.isEmpty())
            m_ethereumBalance = QStringLiteral("—");
    } else if (chain == QStringLiteral("Bitcoin")) {
        m_btcDone = true;
        m_btcOk = false;
        m_btcError = message;
        m_bitcoinStatus = QStringLiteral("FAIL · %1").arg(message);
        if (m_bitcoinBalance.isEmpty())
            m_bitcoinBalance = QStringLiteral("—");
    } else {
        m_solDone = true;
        m_solOk = false;
        m_solError = message;
        m_solanaStatus = QStringLiteral("FAIL · %1").arg(message);
        if (m_solanaBalance.isEmpty())
            m_solanaBalance = QStringLiteral("—");
    }

    finishRequest(generation);
}

void PortfolioService::finishRequest(quint64 generation)
{
    if (generation != m_generation
            || !m_ethDone || !m_btcDone || !m_solDone) {
        return;
    }

    m_loading = false;

    m_lastRefreshPassed =
        (!m_ethRequested || m_ethOk)
        && (!m_btcRequested || m_btcOk)
        && (!m_solRequested || m_solOk);

    if (m_lastRefreshPassed) {
        m_lastUpdated =
            QDateTime::currentDateTime()
                .toString(QStringLiteral("yyyy-MM-dd hh:mm:ss"));
        m_usingCachedData = false;
        m_status = QStringLiteral("PASS · Read-only balances refreshed");
        saveSnapshot();
    } else {
        QStringList failures;

        if (m_ethRequested && !m_ethOk)
            failures << QStringLiteral("Ethereum: %1").arg(m_ethError);

        if (m_btcRequested && !m_btcOk)
            failures << QStringLiteral("Bitcoin: %1").arg(m_btcError);

        if (m_solRequested && !m_solOk)
            failures << QStringLiteral("Solana: %1").arg(m_solError);

        m_status = QStringLiteral("PARTIAL · %1")
            .arg(failures.join(QStringLiteral(" · ")));

        // A partial refresh can contain a mixture of newly verified and
        // retained last-known values. Keep that state explicit for QML.
        m_usingCachedData = (m_ethereumBalance != QStringLiteral("—")
                             || m_bitcoinBalance != QStringLiteral("—")
                             || m_solanaBalance != QStringLiteral("—"));

        QStringList updatedValues;
        if (!m_ethereumUpdated.isEmpty()) updatedValues << m_ethereumUpdated;
        if (!m_bitcoinUpdated.isEmpty()) updatedValues << m_bitcoinUpdated;
        if (!m_solanaUpdated.isEmpty()) updatedValues << m_solanaUpdated;
        updatedValues.sort();
        if (!updatedValues.isEmpty())
            m_lastUpdated = updatedValues.last();
    }

    emit stateChanged();
}
