#include "tokenservice.h"
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
#include <QVariantMap>

namespace {

const char kEthereumExplorerKey[] = "network/ethereumExplorerUrl";
const char kSolanaTokenRpcKey[] = "network/solanaTokenRpcUrl";
const char kHiddenTokensKey[] = "tokens/hiddenIds";

const char kDefaultEthereumExplorer[] = "https://eth.blockscout.com/api/v2";
const char kDefaultSolanaTokenRpc[] = "https://api.mainnet.solana.com";

const char kSplTokenProgram[] =
    "TokenkegQfeZyiNwAJbNbGKPFXCWuBvf9Ss623VQ5DA";
const char kToken2022Program[] =
    "TokenzQdBNbLqP5VEhdkAS6EPFLC1PHnBqCXEpPxuEb";

QString displayTokenName(const QString &name,
                         const QString &symbol,
                         const QString &fallback)
{
    if (!name.trimmed().isEmpty())
        return name.trimmed();
    if (!symbol.trimmed().isEmpty())
        return symbol.trimmed();
    return fallback;
}

} // namespace

TokenService::TokenService(QObject *parent)
    : QObject(parent)
    , m_status(QStringLiteral("Open Token holdings to query public token balances."))
    , m_ethereumStatus(QStringLiteral("Not queried"))
    , m_solanaStatus(QStringLiteral("Not queried"))
{
}

bool TokenService::loading() const
{
    return m_loading;
}

bool TokenService::lastRefreshPassed() const
{
    return m_lastRefreshPassed;
}

QString TokenService::status() const
{
    return m_status;
}

QString TokenService::ethereumStatus() const
{
    return m_ethereumStatus;
}

QString TokenService::solanaStatus() const
{
    return m_solanaStatus;
}

QVariantList TokenService::entries() const
{
    return m_entries;
}

int TokenService::tokenCount() const
{
    return m_entries.size();
}

int TokenService::ethereumTokenCount() const
{
    return m_ethereumTokenCount;
}

int TokenService::solanaTokenCount() const
{
    return m_solanaTokenCount;
}

bool TokenService::showHidden() const
{
    return m_showHidden;
}

void TokenService::setShowHidden(bool show)
{
    if (m_showHidden == show)
        return;

    m_showHidden = show;
    rebuildVisibleEntries();
    emit stateChanged();
}

int TokenService::hiddenTokenCount() const
{
    return m_hiddenTokenCount;
}

QString TokenService::lastUpdated() const
{
    return m_lastUpdated;
}

QString TokenService::setting(const char *key, const char *fallback)
{
    return SailVaultSettings::value(
        QString::fromLatin1(key),
        QString::fromLatin1(fallback)).toString();
}

QString TokenService::normalizeBaseUrl(const QString &url)
{
    QString value = url.trimmed();
    while (value.endsWith(QLatin1Char('/')))
        value.chop(1);
    return value;
}

QString TokenService::networkErrorText(QNetworkReply *reply)
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

bool TokenService::isZeroInteger(const QString &value)
{
    QString digits = value.trimmed();

    if (digits.startsWith(QLatin1Char('+'))
            || digits.startsWith(QLatin1Char('-'))) {
        digits.remove(0, 1);
    }

    if (digits.isEmpty())
        return true;

    for (const QChar ch : digits) {
        if (ch != QLatin1Char('0'))
            return false;
    }

    return true;
}

QString TokenService::formatIntegerAmount(const QString &rawAmount, int decimals)
{
    QString digits = rawAmount.trimmed();
    bool negative = false;

    if (digits.startsWith(QLatin1Char('-'))) {
        negative = true;
        digits.remove(0, 1);
    } else if (digits.startsWith(QLatin1Char('+'))) {
        digits.remove(0, 1);
    }

    if (digits.isEmpty())
        return QStringLiteral("0");

    for (const QChar ch : digits) {
        if (ch < QLatin1Char('0') || ch > QLatin1Char('9'))
            return rawAmount;
    }

    while (digits.size() > 1 && digits.startsWith(QLatin1Char('0')))
        digits.remove(0, 1);

    if (decimals <= 0) {
        if (negative && !isZeroInteger(digits))
            digits.prepend(QLatin1Char('-'));
        return digits;
    }

    while (digits.size() <= decimals)
        digits.prepend(QLatin1Char('0'));

    digits.insert(digits.size() - decimals, QLatin1Char('.'));

    while (digits.endsWith(QLatin1Char('0')))
        digits.chop(1);
    if (digits.endsWith(QLatin1Char('.')))
        digits.chop(1);

    if (digits.startsWith(QLatin1Char('.')))
        digits.prepend(QLatin1Char('0'));

    if (negative && !isZeroInteger(rawAmount))
        digits.prepend(QLatin1Char('-'));

    return digits;
}

QString TokenService::tokenStorageKey(const QString &chain,
                                      const QString &identifier)
{
    QString normalizedIdentifier = identifier.trimmed();

    if (chain.compare(QStringLiteral("Ethereum"),
                      Qt::CaseInsensitive) == 0) {
        normalizedIdentifier = normalizedIdentifier.toLower();
    }

    return chain.trimmed()
        + QLatin1Char('|')
        + normalizedIdentifier;
}

bool TokenService::tokenHidden(const QString &chain,
                               const QString &identifier) const
{
    const QString key = tokenStorageKey(chain, identifier);

    if (key.endsWith(QLatin1Char('|')))
        return false;

    const QStringList hidden =
        SailVaultSettings::value(
            QString::fromLatin1(kHiddenTokensKey),
            QStringList()).toStringList();

    return hidden.contains(key);
}

void TokenService::setTokenHidden(const QString &chain,
                                  const QString &identifier,
                                  bool hidden)
{
    const QString key = tokenStorageKey(chain, identifier);

    if (key.endsWith(QLatin1Char('|')))
        return;

    QStringList hiddenKeys =
        SailVaultSettings::value(
            QString::fromLatin1(kHiddenTokensKey),
            QStringList()).toStringList();

    bool changed = false;

    if (hidden) {
        if (!hiddenKeys.contains(key)) {
            hiddenKeys.append(key);
            changed = true;
        }
    } else {
        changed = hiddenKeys.removeAll(key) > 0;
    }

    if (!changed)
        return;

    SailVaultSettings::setValue(
        QString::fromLatin1(kHiddenTokensKey),
        hiddenKeys);

    for (int i = 0; i < m_allEntries.size(); ++i) {
        QVariantMap item = m_allEntries.at(i).toMap();

        if (tokenStorageKey(
                item.value(QStringLiteral("chain")).toString(),
                item.value(QStringLiteral("identifier")).toString()) == key) {
            item.insert(QStringLiteral("hidden"), hidden);
            m_allEntries[i] = item;
        }
    }

    rebuildVisibleEntries();
    emit stateChanged();
}

void TokenService::rebuildVisibleEntries()
{
    m_entries.clear();
    m_hiddenTokenCount = 0;

    for (const QVariant &value : m_allEntries) {
        const QVariantMap item = value.toMap();
        const bool hidden =
            item.value(QStringLiteral("hidden")).toBool();

        if (hidden)
            ++m_hiddenTokenCount;

        if (!hidden || m_showHidden)
            m_entries.append(item);
    }
}

void TokenService::addEntry(const QString &chain,
                            const QString &name,
                            const QString &symbol,
                            const QString &amount,
                            const QString &identifier,
                            const QString &standard,
                            const QString &extra)
{
    QVariantMap item;
    item.insert(QStringLiteral("chain"), chain);
    item.insert(QStringLiteral("name"), name);
    item.insert(QStringLiteral("symbol"), symbol);
    item.insert(QStringLiteral("amount"), amount);
    item.insert(QStringLiteral("identifier"), identifier);
    item.insert(QStringLiteral("standard"), standard);
    item.insert(QStringLiteral("extra"), extra);
    item.insert(QStringLiteral("hidden"),
                tokenHidden(chain, identifier));
    m_allEntries.append(item);
}

void TokenService::refresh(const QString &ethereumAddress,
                           const QString &solanaAddress)
{
    const QString eth = ethereumAddress.trimmed();
    const QString sol = solanaAddress.trimmed();

    m_ethRequested = !eth.isEmpty();
    m_solRequested = !sol.isEmpty();

    if (!m_ethRequested && !m_solRequested) {
        m_lastRefreshPassed = false;
        m_status = QStringLiteral(
            "Provide an Ethereum or Solana public address before refreshing token holdings.");
        emit stateChanged();
        return;
    }

    if (SailVaultNetwork::offlineModeEnabled()) {
        ++m_generation;
        SailVaultNetwork::cancelOutstanding(&m_network);
        m_loading = false;
        m_lastRefreshPassed = false;
        m_ethereumStatus = m_ethRequested ? QStringLiteral("Offline")
                                          : QStringLiteral("Not configured");
        m_solanaStatus = m_solRequested ? QStringLiteral("Offline")
                                        : QStringLiteral("Not configured");
        m_status = QStringLiteral(
            "Offline mode · token holdings require a provider request");
        emit stateChanged();
        return;
    }

    const QUrl ethUrl(setting(kEthereumExplorerKey, kDefaultEthereumExplorer));
    const QUrl solUrl(setting(kSolanaTokenRpcKey, kDefaultSolanaTokenRpc));

    if ((m_ethRequested
         && (!ethUrl.isValid()
             || ethUrl.scheme() != QStringLiteral("https")))
            || (m_solRequested
                && (!solUrl.isValid()
                    || solUrl.scheme() != QStringLiteral("https")))) {
        m_lastRefreshPassed = false;
        m_status = QStringLiteral(
            "Required token providers must be valid HTTPS URLs.");
        emit stateChanged();
        return;
    }

    ++m_generation;
    const quint64 generation = m_generation;
    SailVaultNetwork::cancelOutstanding(&m_network);

    m_loading = true;
    m_lastRefreshPassed = false;

    m_ethDone = !m_ethRequested;
    m_solDone = !m_solRequested;

    m_ethOk = !m_ethRequested;
    m_solOk = !m_solRequested;

    m_solRequestsPending = m_solRequested ? 2 : 0;
    m_solRequestsSucceeded = 0;

    m_ethereumTokenCount = 0;
    m_solanaTokenCount = 0;
    m_solErrors.clear();
    m_allEntries.clear();
    m_entries.clear();
    m_hiddenTokenCount = 0;

    m_ethereumStatus =
        m_ethRequested ? QStringLiteral("Refreshing…")
                       : QStringLiteral("Not configured");

    m_solanaStatus =
        m_solRequested ? QStringLiteral("Refreshing…")
                       : QStringLiteral("Not configured");

    m_status = QStringLiteral("Refreshing public token holdings…");
    emit stateChanged();

    if (m_ethRequested)
        refreshEthereum(eth, generation);

    if (m_solRequested) {
        refreshSolanaProgram(sol,
                             QString::fromLatin1(kSplTokenProgram),
                             QStringLiteral("SPL Token"),
                             generation);

        refreshSolanaProgram(sol,
                             QString::fromLatin1(kToken2022Program),
                             QStringLiteral("Token-2022"),
                             generation);
    }
}

void TokenService::refreshEthereum(const QString &address,
                                   quint64 generation)
{
    const QString endpoint =
        normalizeBaseUrl(setting(kEthereumExplorerKey, kDefaultEthereumExplorer))
        + QStringLiteral("/addresses/")
        + QString::fromUtf8(QUrl::toPercentEncoding(address))
        + QStringLiteral("/token-balances");

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
            m_ethDone = true;
            m_ethOk = false;
            m_ethereumStatus =
                QStringLiteral("FAIL · %1").arg(networkError);
            finishIfReady(generation);
            return;
        }

        QJsonParseError parseError;
        const QJsonDocument doc =
            QJsonDocument::fromJson(payload, &parseError);

        if (parseError.error != QJsonParseError::NoError || !doc.isArray()) {
            m_ethDone = true;
            m_ethOk = false;
            m_ethereumStatus =
                QStringLiteral("FAIL · invalid JSON response");
            finishIfReady(generation);
            return;
        }

        const QJsonArray balances = doc.array();

        for (const QJsonValue &value : balances) {
            const QJsonObject balance = value.toObject();
            const QJsonObject token =
                balance.value(QStringLiteral("token")).toObject();

            const QString type =
                token.value(QStringLiteral("type")).toString();

            if (type.compare(QStringLiteral("ERC-20"),
                             Qt::CaseInsensitive) != 0) {
                continue;
            }

            const QString raw =
                balance.value(QStringLiteral("value")).toVariant().toString();
            if (raw.isEmpty() || isZeroInteger(raw))
                continue;

            bool decimalsOk = false;
            int decimals =
                token.value(QStringLiteral("decimals"))
                    .toVariant().toString().toInt(&decimalsOk);
            if (!decimalsOk || decimals < 0)
                decimals = 0;

            const QString symbol =
                token.value(QStringLiteral("symbol")).toString();
            const QString name =
                displayTokenName(
                    token.value(QStringLiteral("name")).toString(),
                    symbol,
                    QStringLiteral("ERC-20 token"));
            const QString contract =
                token.value(QStringLiteral("address_hash")).toString();
            const QString amount =
                formatIntegerAmount(raw, decimals);
            const QString exchangeRate =
                token.value(QStringLiteral("exchange_rate"))
                    .toVariant().toString();

            QString extra = QStringLiteral("%1 decimals").arg(decimals);
            if (!exchangeRate.trimmed().isEmpty()
                    && exchangeRate != QStringLiteral("null")) {
                extra += QStringLiteral(" · explorer price %1")
                    .arg(exchangeRate);
            }

            addEntry(QStringLiteral("Ethereum"),
                     name,
                     symbol,
                     amount,
                     contract,
                     QStringLiteral("ERC-20"),
                     extra);
            ++m_ethereumTokenCount;
        }

        m_ethDone = true;
        m_ethOk = true;
        m_ethereumStatus =
            QStringLiteral("OK · %1 non-zero ERC-20")
                .arg(m_ethereumTokenCount);
        finishIfReady(generation);
    });
}

void TokenService::refreshSolanaProgram(const QString &address,
                                        const QString &programId,
                                        const QString &standard,
                                        quint64 generation)
{
    QNetworkRequest request{
        QUrl(setting(kSolanaTokenRpcKey, kDefaultSolanaTokenRpc))
    };
    request.setHeader(QNetworkRequest::ContentTypeHeader,
                      QStringLiteral("application/json"));
    request.setRawHeader("Accept", "application/json");

    QJsonObject filter;
    filter.insert(QStringLiteral("programId"), programId);

    QJsonObject config;
    config.insert(QStringLiteral("commitment"), QStringLiteral("finalized"));
    config.insert(QStringLiteral("encoding"), QStringLiteral("jsonParsed"));

    QJsonArray params;
    params.append(address);
    params.append(filter);
    params.append(config);

    QJsonObject body;
    body.insert(QStringLiteral("jsonrpc"), QStringLiteral("2.0"));
    body.insert(QStringLiteral("id"), standard);
    body.insert(QStringLiteral("method"),
                QStringLiteral("getTokenAccountsByOwner"));
    body.insert(QStringLiteral("params"), params);

    QNetworkReply *reply =
        m_network.post(request,
                       QJsonDocument(body).toJson(QJsonDocument::Compact));
    SailVaultNetwork::armTimeout(reply);

    connect(reply, &QNetworkReply::finished, this,
            [this, reply, standard, generation]() {
        const QByteArray payload = reply->readAll();
        const QString networkError = networkErrorText(reply);
        reply->deleteLater();

        if (generation != m_generation)
            return;

        if (!networkError.isEmpty()) {
            finishSolanaRequest(
                false,
                QStringLiteral("%1: %2").arg(standard, networkError),
                generation);
            return;
        }

        QJsonParseError parseError;
        const QJsonDocument doc =
            QJsonDocument::fromJson(payload, &parseError);

        if (parseError.error != QJsonParseError::NoError || !doc.isObject()) {
            finishSolanaRequest(
                false,
                QStringLiteral("%1: invalid JSON response").arg(standard),
                generation);
            return;
        }

        const QJsonObject object = doc.object();

        if (object.contains(QStringLiteral("error"))) {
            const QJsonObject error =
                object.value(QStringLiteral("error")).toObject();
            finishSolanaRequest(
                false,
                QStringLiteral("%1: %2")
                    .arg(standard,
                         error.value(QStringLiteral("message"))
                              .toString(QStringLiteral("RPC error"))),
                generation);
            return;
        }

        const QJsonArray accounts =
            object.value(QStringLiteral("result"))
                  .toObject()
                  .value(QStringLiteral("value"))
                  .toArray();

        for (const QJsonValue &value : accounts) {
            const QJsonObject account = value.toObject();
            const QJsonObject parsed =
                account.value(QStringLiteral("account"))
                       .toObject()
                       .value(QStringLiteral("data"))
                       .toObject()
                       .value(QStringLiteral("parsed"))
                       .toObject();

            const QJsonObject info =
                parsed.value(QStringLiteral("info")).toObject();
            const QJsonObject amountObject =
                info.value(QStringLiteral("tokenAmount")).toObject();

            const QString raw =
                amountObject.value(QStringLiteral("amount"))
                            .toVariant().toString();

            if (raw.isEmpty() || isZeroInteger(raw))
                continue;

            const QString mint =
                info.value(QStringLiteral("mint")).toString();
            const QString uiAmount =
                amountObject.value(QStringLiteral("uiAmountString"))
                            .toString();

            int decimals =
                amountObject.value(QStringLiteral("decimals")).toInt(0);

            const QString displayAmount =
                uiAmount.isEmpty()
                    ? formatIntegerAmount(raw, decimals)
                    : uiAmount;

            addEntry(QStringLiteral("Solana"),
                     standard == QStringLiteral("Token-2022")
                         ? QStringLiteral("Token-2022 asset")
                         : QStringLiteral("SPL token"),
                     QString(),
                     displayAmount,
                     mint,
                     standard,
                     QStringLiteral("%1 decimals").arg(decimals));
            ++m_solanaTokenCount;
        }

        finishSolanaRequest(true, QString(), generation);
    });
}

void TokenService::finishSolanaRequest(bool passed,
                                       const QString &message,
                                       quint64 generation)
{
    if (generation != m_generation || m_solRequestsPending <= 0)
        return;

    --m_solRequestsPending;

    if (passed)
        ++m_solRequestsSucceeded;
    else if (!message.isEmpty())
        m_solErrors.append(message);

    if (m_solRequestsPending > 0)
        return;

    m_solDone = true;
    m_solOk = (m_solRequestsSucceeded == 2);

    if (m_solOk) {
        m_solanaStatus =
            QStringLiteral("OK · %1 non-zero token account(s)")
                .arg(m_solanaTokenCount);
    } else if (m_solRequestsSucceeded > 0) {
        m_solanaStatus =
            QStringLiteral("PARTIAL · %1")
                .arg(m_solErrors.join(QStringLiteral(" · ")));
    } else {
        m_solanaStatus =
            QStringLiteral("FAIL · %1")
                .arg(m_solErrors.join(QStringLiteral(" · ")));
    }

    finishIfReady(generation);
}

void TokenService::finishIfReady(quint64 generation)
{
    if (generation != m_generation || !m_ethDone || !m_solDone)
        return;

    std::sort(m_allEntries.begin(), m_allEntries.end(),
              [](const QVariant &left, const QVariant &right) {
        const QVariantMap a = left.toMap();
        const QVariantMap b = right.toMap();

        const QString aChain = a.value(QStringLiteral("chain")).toString();
        const QString bChain = b.value(QStringLiteral("chain")).toString();

        if (aChain != bChain)
            return aChain < bChain;

        const QString aName = a.value(QStringLiteral("name")).toString();
        const QString bName = b.value(QStringLiteral("name")).toString();
        return aName.compare(bName, Qt::CaseInsensitive) < 0;
    });

    rebuildVisibleEntries();

    m_loading = false;

    m_lastRefreshPassed =
        (!m_ethRequested || m_ethOk)
        && (!m_solRequested || m_solOk);

    m_lastUpdated =
        QDateTime::currentDateTime()
            .toString(QStringLiteral("yyyy-MM-dd hh:mm:ss"));

    if (m_lastRefreshPassed) {
        m_status =
            QStringLiteral("PASS · Token holdings refreshed");
    } else if ((m_ethRequested && m_ethOk)
               || (m_solRequested && m_solRequestsSucceeded > 0)) {
        m_status =
            QStringLiteral("PARTIAL · Some configured token providers failed");
    } else {
        m_status =
            QStringLiteral("FAIL · Token holdings could not be refreshed");
    }

    emit stateChanged();
}
