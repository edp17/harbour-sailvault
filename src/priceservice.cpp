#include "priceservice.h"
#include "settingsstore.h"
#include "networkrequestutils.h"

#include <QDateTime>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QTimer>
#include <QStringList>
#include <QUrl>

namespace {
const char kFiatCurrencyKey[] = "portfolio/fiatCurrency";
const char kDefaultFiatCurrency[] = "GBP";
const char kKrakenTickerBase[] = "https://api.kraken.com/0/public/Ticker?pair=";

QString firstKrakenError(const QJsonArray &errors)
{
    if (errors.isEmpty())
        return QString();

    QStringList messages;
    for (const QJsonValue &value : errors) {
        const QString text = value.toString();
        if (!text.isEmpty())
            messages.append(text);
    }

    return messages.isEmpty()
        ? QStringLiteral("Kraken returned an API error")
        : messages.join(QStringLiteral(" · "));
}
}

PriceService::PriceService(QObject *parent)
    : QObject(parent)
    , m_status(QStringLiteral("Market prices not refreshed yet"))
    , m_bitcoinStatus(QStringLiteral("Not queried"))
    , m_ethereumStatus(QStringLiteral("Not queried"))
    , m_solanaStatus(QStringLiteral("Not queried"))
{
    loadCachedPrices();
}

bool PriceService::loading() const { return m_loading; }
bool PriceService::lastRefreshPassed() const { return m_lastRefreshPassed; }
QString PriceService::status() const { return m_status; }

double PriceService::bitcoinPriceValue() const { return m_bitcoinPrice; }
double PriceService::ethereumPriceValue() const { return m_ethereumPrice; }
double PriceService::solanaPriceValue() const { return m_solanaPrice; }
QString PriceService::bitcoinStatus() const { return m_bitcoinStatus; }
QString PriceService::ethereumStatus() const { return m_ethereumStatus; }
QString PriceService::solanaStatus() const { return m_solanaStatus; }
QString PriceService::bitcoinUpdated() const { return m_bitcoinUpdated; }
QString PriceService::ethereumUpdated() const { return m_ethereumUpdated; }
QString PriceService::solanaUpdated() const { return m_solanaUpdated; }
QString PriceService::lastUpdated() const { return m_lastUpdated; }
bool PriceService::usingCachedData() const { return m_usingCachedData; }

QString PriceService::normalizedCurrency(const QString &currency)
{
    const QString value = currency.trimmed().toUpper();
    if (value == QStringLiteral("USD")
            || value == QStringLiteral("EUR")
            || value == QStringLiteral("GBP"))
        return value;
    return QString::fromLatin1(kDefaultFiatCurrency);
}

QString PriceService::fiatCurrency() const
{
    return normalizedCurrency(
        SailVaultSettings::value(
            QString::fromLatin1(kFiatCurrencyKey),
            QString::fromLatin1(kDefaultFiatCurrency)).toString());
}

void PriceService::loadCachedPrices()
{
    const QString base = QStringLiteral("priceCache/")
        + fiatCurrency() + QLatin1Char('/');

    const QString cachedUpdated =
        SailVaultSettings::value(base + QStringLiteral("lastUpdated"),
                                 QString()).toString();

    m_bitcoinPrice =
        SailVaultSettings::value(base + QStringLiteral("bitcoin"), 0.0).toDouble();
    m_ethereumPrice =
        SailVaultSettings::value(base + QStringLiteral("ethereum"), 0.0).toDouble();
    m_solanaPrice =
        SailVaultSettings::value(base + QStringLiteral("solana"), 0.0).toDouble();

    m_bitcoinUpdated = SailVaultSettings::value(
        base + QStringLiteral("bitcoinUpdated"), cachedUpdated).toString();
    m_ethereumUpdated = SailVaultSettings::value(
        base + QStringLiteral("ethereumUpdated"), cachedUpdated).toString();
    m_solanaUpdated = SailVaultSettings::value(
        base + QStringLiteral("solanaUpdated"), cachedUpdated).toString();

    if (m_bitcoinPrice <= 0.0) m_bitcoinUpdated.clear();
    if (m_ethereumPrice <= 0.0) m_ethereumUpdated.clear();
    if (m_solanaPrice <= 0.0) m_solanaUpdated.clear();

    QStringList updatedValues;
    if (!m_bitcoinUpdated.isEmpty()) updatedValues << m_bitcoinUpdated;
    if (!m_ethereumUpdated.isEmpty()) updatedValues << m_ethereumUpdated;
    if (!m_solanaUpdated.isEmpty()) updatedValues << m_solanaUpdated;
    updatedValues.sort();
    m_lastUpdated = updatedValues.isEmpty() ? cachedUpdated : updatedValues.last();

    const bool hasAny = m_bitcoinPrice > 0.0
        || m_ethereumPrice > 0.0 || m_solanaPrice > 0.0;
    m_lastRefreshPassed = m_bitcoinPrice > 0.0
        && m_ethereumPrice > 0.0 && m_solanaPrice > 0.0;
    m_usingCachedData = hasAny;

    if (!hasAny) {
        m_status = QStringLiteral("Market prices not refreshed yet");
        m_bitcoinStatus = QStringLiteral("Not queried");
        m_ethereumStatus = QStringLiteral("Not queried");
        m_solanaStatus = QStringLiteral("Not queried");
        m_lastUpdated.clear();
        return;
    }

    m_status = m_lastRefreshPassed
        ? QStringLiteral("Cached market prices · refresh to verify current values")
        : QStringLiteral("Market-price cache incomplete · refresh required");
    m_bitcoinStatus = m_bitcoinPrice > 0.0
        ? QStringLiteral("Cached") : QStringLiteral("Not queried");
    m_ethereumStatus = m_ethereumPrice > 0.0
        ? QStringLiteral("Cached") : QStringLiteral("Not queried");
    m_solanaStatus = m_solanaPrice > 0.0
        ? QStringLiteral("Cached") : QStringLiteral("Not queried");
}

void PriceService::saveCachedPrices()
{
    if (!m_lastRefreshPassed)
        return;

    const QString base = QStringLiteral("priceCache/")
        + fiatCurrency() + QLatin1Char('/');

    SailVaultSettings::setValue(base + QStringLiteral("bitcoin"), m_bitcoinPrice);
    SailVaultSettings::setValue(base + QStringLiteral("ethereum"), m_ethereumPrice);
    SailVaultSettings::setValue(base + QStringLiteral("solana"), m_solanaPrice);
    SailVaultSettings::setValue(base + QStringLiteral("bitcoinUpdated"), m_bitcoinUpdated);
    SailVaultSettings::setValue(base + QStringLiteral("ethereumUpdated"), m_ethereumUpdated);
    SailVaultSettings::setValue(base + QStringLiteral("solanaUpdated"), m_solanaUpdated);
    SailVaultSettings::setValue(base + QStringLiteral("lastUpdated"), m_lastUpdated);
}

void PriceService::saveCachedPrice(const QString &asset,
                                   double price,
                                   const QString &updated)
{
    if (price <= 0.0 || updated.isEmpty())
        return;

    const QString base = QStringLiteral("priceCache/")
        + fiatCurrency() + QLatin1Char('/');

    QString key;
    QString updatedKey;
    if (asset == QStringLiteral("Bitcoin")) {
        key = QStringLiteral("bitcoin");
        updatedKey = QStringLiteral("bitcoinUpdated");
    } else if (asset == QStringLiteral("Ethereum")) {
        key = QStringLiteral("ethereum");
        updatedKey = QStringLiteral("ethereumUpdated");
    } else {
        key = QStringLiteral("solana");
        updatedKey = QStringLiteral("solanaUpdated");
    }

    SailVaultSettings::setValue(base + key, price);
    SailVaultSettings::setValue(base + updatedKey, updated);
}

void PriceService::setFiatCurrency(const QString &currency)
{
    const QString normalized = normalizedCurrency(currency);
    if (normalized == fiatCurrency())
        return;

    SailVaultSettings::setValue(
        QString::fromLatin1(kFiatCurrencyKey), normalized);

    loadCachedPrices();

    if (!m_usingCachedData)
        m_status = QStringLiteral("Fiat currency changed · refresh market prices");

    emit settingsChanged();
    emit stateChanged();
}

QString PriceService::fiatSymbol() const
{
    const QString currency = fiatCurrency();
    if (currency == QStringLiteral("GBP"))
        return QString::fromUtf8("£");
    if (currency == QStringLiteral("EUR"))
        return QString::fromUtf8("€");
    return QStringLiteral("$");
}

QString PriceService::formatFiat(double value) const
{
    if (value < 0.0)
        value = 0.0;
    int decimals = 2;
    if (value > 0.0 && value < 0.01)
        decimals = 4;
    return fiatSymbol() + QString::number(value, 'f', decimals)
        + QStringLiteral(" ") + fiatCurrency();
}

QString PriceService::bitcoinPrice() const
{
    return m_bitcoinPrice > 0.0 ? formatFiat(m_bitcoinPrice) : QStringLiteral("—");
}
QString PriceService::ethereumPrice() const
{
    return m_ethereumPrice > 0.0 ? formatFiat(m_ethereumPrice) : QStringLiteral("—");
}
QString PriceService::solanaPrice() const
{
    return m_solanaPrice > 0.0 ? formatFiat(m_solanaPrice) : QStringLiteral("—");
}

QString PriceService::pairFor(const QString &asset, const QString &currency)
{
    if (asset == QStringLiteral("Bitcoin"))
        return QStringLiteral("XBT") + currency;
    if (asset == QStringLiteral("Ethereum"))
        return QStringLiteral("ETH") + currency;
    return QStringLiteral("SOL") + currency;
}

QString PriceService::networkErrorText(QNetworkReply *reply)
{
    if (SailVaultNetwork::timedOut(reply))
        return QStringLiteral("Request timed out after 15 seconds");

    const int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    if (reply->error() != QNetworkReply::NoError) {
        return status > 0
            ? QStringLiteral("HTTP %1 · %2").arg(status).arg(reply->errorString())
            : reply->errorString();
    }
    if (status < 200 || status >= 300)
        return QStringLiteral("HTTP %1").arg(status);
    return QString();
}

void PriceService::refresh()
{
    ++m_generation;
    const quint64 generation = m_generation;
    SailVaultNetwork::cancelOutstanding(&m_network);
    const QString currency = fiatCurrency();

    if (SailVaultNetwork::offlineModeEnabled()) {
        m_loading = false;
        m_lastRefreshPassed = false;
        m_usingCachedData = (m_bitcoinPrice > 0.0
                             || m_ethereumPrice > 0.0
                             || m_solanaPrice > 0.0);
        m_bitcoinStatus = m_bitcoinPrice > 0.0
            ? QStringLiteral("Offline · cached") : QStringLiteral("Offline");
        m_ethereumStatus = m_ethereumPrice > 0.0
            ? QStringLiteral("Offline · cached") : QStringLiteral("Offline");
        m_solanaStatus = m_solanaPrice > 0.0
            ? QStringLiteral("Offline · cached") : QStringLiteral("Offline");
        m_status = m_usingCachedData
            ? QStringLiteral("Offline mode · using last-known market prices")
            : QStringLiteral("Offline mode · no cached market prices available");
        emit stateChanged();
        return;
    }

    m_loading = true;
    m_lastRefreshPassed = false;
    m_usingCachedData = (m_bitcoinPrice > 0.0
                         || m_ethereumPrice > 0.0
                         || m_solanaPrice > 0.0);
    m_btcDone = m_ethDone = m_solDone = false;
    m_btcOk = m_ethOk = m_solOk = false;
    m_bitcoinStatus = QStringLiteral("Refreshing…");
    m_ethereumStatus = QStringLiteral("Refreshing…");
    m_solanaStatus = QStringLiteral("Refreshing…");
    m_status = QStringLiteral("Refreshing Kraken market prices…");
    emit stateChanged();

    requestTicker(QStringLiteral("Bitcoin"), pairFor(QStringLiteral("Bitcoin"), currency), generation);
    requestTicker(QStringLiteral("Ethereum"), pairFor(QStringLiteral("Ethereum"), currency), generation);
    requestTicker(QStringLiteral("Solana"), pairFor(QStringLiteral("Solana"), currency), generation);
}

void PriceService::requestTicker(const QString &asset,
                                 const QString &pair,
                                 quint64 generation,
                                 int attempt)
{
    if (generation != m_generation)
        return;
    const QString endpoint = QString::fromLatin1(kKrakenTickerBase)
        + QString::fromUtf8(QUrl::toPercentEncoding(pair));

    QNetworkRequest request{QUrl(endpoint)};
    request.setRawHeader("Accept", "application/json");
    QNetworkReply *reply = m_network.get(request);
    SailVaultNetwork::armTimeout(reply);

    connect(reply, &QNetworkReply::finished, this,
            [this, reply, asset, pair, generation, attempt]() {
        const QByteArray payload = reply->readAll();
        const QNetworkReply::NetworkError errorCode = reply->error();
        const QString networkError = networkErrorText(reply);
        reply->deleteLater();

        if (generation != m_generation)
            return;

        if (errorCode == QNetworkReply::HostNotFoundError && attempt < 2) {
            m_status = QStringLiteral(
                "Market-price DNS lookup failed · retrying…");
            emit stateChanged();

            QTimer::singleShot(1200 * (attempt + 1), this,
                               [this, asset, pair, generation, attempt]() {
                requestTicker(asset, pair, generation, attempt + 1);
            });
            return;
        }

        if (!networkError.isEmpty()) {
            finishChain(asset, false, 0.0, networkError, generation);
            return;
        }

        QJsonParseError parseError;
        const QJsonDocument doc = QJsonDocument::fromJson(payload, &parseError);
        if (parseError.error != QJsonParseError::NoError || !doc.isObject()) {
            finishChain(asset, false, 0.0, QStringLiteral("invalid JSON response"), generation);
            return;
        }

        const QJsonObject root = doc.object();
        const QString apiError = firstKrakenError(root.value(QStringLiteral("error")).toArray());
        if (!apiError.isEmpty()) {
            finishChain(asset, false, 0.0, apiError, generation);
            return;
        }

        const QJsonObject result = root.value(QStringLiteral("result")).toObject();
        if (result.isEmpty()) {
            finishChain(asset, false, 0.0, QStringLiteral("empty ticker result"), generation);
            return;
        }

        QJsonObject ticker;
        for (QJsonObject::const_iterator it = result.constBegin(); it != result.constEnd(); ++it) {
            ticker = it.value().toObject();
            break;
        }

        const QJsonArray close = ticker.value(QStringLiteral("c")).toArray();
        if (close.isEmpty()) {
            finishChain(asset, false, 0.0, QStringLiteral("ticker has no last-trade price"), generation);
            return;
        }

        bool ok = false;
        const double price = close.at(0).toString().toDouble(&ok);
        if (!ok || price <= 0.0) {
            finishChain(asset, false, 0.0, QStringLiteral("invalid ticker price"), generation);
            return;
        }

        finishChain(asset, true, price, QString(), generation);
    });
}

void PriceService::finishChain(const QString &asset,
                               bool passed,
                               double price,
                               const QString &message,
                               quint64 generation)
{
    if (generation != m_generation)
        return;

    const QString state = passed ? QStringLiteral("OK")
                                 : QStringLiteral("FAIL · %1").arg(message);

    if (asset == QStringLiteral("Bitcoin")) {
        m_btcDone = true;
        m_btcOk = passed;
        m_bitcoinStatus = state;
        if (passed) {
            m_bitcoinPrice = price;
            m_bitcoinUpdated = QDateTime::currentDateTime()
                .toString(QStringLiteral("yyyy-MM-dd hh:mm:ss"));
            saveCachedPrice(asset, price, m_bitcoinUpdated);
        }
    } else if (asset == QStringLiteral("Ethereum")) {
        m_ethDone = true;
        m_ethOk = passed;
        m_ethereumStatus = state;
        if (passed) {
            m_ethereumPrice = price;
            m_ethereumUpdated = QDateTime::currentDateTime()
                .toString(QStringLiteral("yyyy-MM-dd hh:mm:ss"));
            saveCachedPrice(asset, price, m_ethereumUpdated);
        }
    } else {
        m_solDone = true;
        m_solOk = passed;
        m_solanaStatus = state;
        if (passed) {
            m_solanaPrice = price;
            m_solanaUpdated = QDateTime::currentDateTime()
                .toString(QStringLiteral("yyyy-MM-dd hh:mm:ss"));
            saveCachedPrice(asset, price, m_solanaUpdated);
        }
    }

    finishIfReady(generation);
}

void PriceService::finishIfReady(quint64 generation)
{
    if (generation != m_generation || !m_btcDone || !m_ethDone || !m_solDone)
        return;

    m_loading = false;
    m_lastRefreshPassed = m_btcOk && m_ethOk && m_solOk;
    const int successCount = (m_btcOk ? 1 : 0) + (m_ethOk ? 1 : 0) + (m_solOk ? 1 : 0);

    if (successCount == 3)
        m_status = QStringLiteral("PASS · BTC, ETH and SOL market prices refreshed");
    else if (successCount > 0)
        m_status = QStringLiteral("PARTIAL · %1 of 3 market prices refreshed").arg(successCount);
    else
        m_status = QStringLiteral("FAIL · Market prices could not be refreshed");

    if (m_lastRefreshPassed) {
        m_lastUpdated =
            QDateTime::currentDateTime()
                .toString(QStringLiteral("yyyy-MM-dd hh:mm:ss"));
        m_usingCachedData = false;
        saveCachedPrices();
    } else {
        m_usingCachedData = (m_bitcoinPrice > 0.0
                             || m_ethereumPrice > 0.0
                             || m_solanaPrice > 0.0);

        QStringList updatedValues;
        if (!m_bitcoinUpdated.isEmpty()) updatedValues << m_bitcoinUpdated;
        if (!m_ethereumUpdated.isEmpty()) updatedValues << m_ethereumUpdated;
        if (!m_solanaUpdated.isEmpty()) updatedValues << m_solanaUpdated;
        updatedValues.sort();
        if (!updatedValues.isEmpty())
            m_lastUpdated = updatedValues.last();
    }

    emit stateChanged();
}
