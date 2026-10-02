#include "apptools.h"
#include "settingsstore.h"
#include "networkrequestutils.h"

#include <QClipboard>
#include <QGuiApplication>

AppTools::AppTools(QObject *parent)
    : QObject(parent)
{
}

QString AppTools::applicationVersion() const
{
    return QStringLiteral(SAILVAULT_APP_VERSION);
}

QString AppTools::packageVersion() const
{
    return QStringLiteral(SAILVAULT_APP_VERSION) + QLatin1Char('-')
        + QStringLiteral(SAILVAULT_PACKAGE_RELEASE);
}

QString AppTools::milestone() const
{
    return QStringLiteral(SAILVAULT_MILESTONE);
}

QString AppTools::buildLabel() const
{
    return QStringLiteral(SAILVAULT_BUILD_LABEL);
}

QString AppTools::walletCoreVersion() const
{
    return QStringLiteral(SAILVAULT_WALLET_CORE_VERSION);
}

void AppTools::copyText(const QString &text) const
{
    if (QClipboard *clipboard = QGuiApplication::clipboard())
        clipboard->setText(text);
}

QVariantMap AppTools::settingsDiagnostics() const
{
    return SailVaultSettings::diagnostics();
}

QVariantMap AppTools::rerunSettingsPersistenceProbe() const
{
    SailVaultSettings::runPersistenceProbe();
    SailVaultSettings::runStorageLifecycleProbe();
    return SailVaultSettings::diagnostics();
}

QVariantMap AppTools::networkSecurityDiagnostics() const
{
    struct Endpoint {
        const char *key;
        const char *fallback;
    };

    static const Endpoint endpoints[] = {
        { "network/ethereumRpcUrl", "https://ethereum-rpc.publicnode.com" },
        { "network/ethereumExplorerUrl", "https://eth.blockscout.com/api/v2" },
        { "network/bitcoinApiUrl", "https://blockstream.info/api" },
        { "network/solanaRpcUrl", "https://solana-rpc.publicnode.com" },
        { "network/solanaTokenRpcUrl", "https://api.mainnet.solana.com" }
    };

    int validEndpoints = 0;
    const int endpointCount = sizeof(endpoints) / sizeof(endpoints[0]);
    for (int i = 0; i < endpointCount; ++i) {
        const QString value = SailVaultSettings::value(
            QString::fromLatin1(endpoints[i].key),
            QString::fromLatin1(endpoints[i].fallback)).toString();
        if (SailVaultNetwork::isValidHttpsEndpoint(value))
            ++validEndpoints;
    }

    const bool policySelfTest =
        SailVaultNetwork::isValidHttpsEndpoint(
            QStringLiteral("https://example.com/rpc"))
        && !SailVaultNetwork::isValidHttpsEndpoint(
            QStringLiteral("http") + QStringLiteral("://example.com/rpc"))
        && !SailVaultNetwork::isValidHttpsEndpoint(
            QStringLiteral("https://user:secret@example.com/rpc"))
        && !SailVaultNetwork::isValidHttpsEndpoint(QStringLiteral("not a url"));

    const bool endpointPolicyPassed = validEndpoints == endpointCount;

    QNetworkRequest requestProbe(
        QUrl(QStringLiteral("https://example.com/request-policy-probe")));
    requestProbe.setAttribute(QNetworkRequest::FollowRedirectsAttribute, true);
    requestProbe.setMaximumRedirectsAllowed(8);
    requestProbe.setAttribute(QNetworkRequest::CookieLoadControlAttribute,
                              QNetworkRequest::Automatic);
    requestProbe.setAttribute(QNetworkRequest::CookieSaveControlAttribute,
                              QNetworkRequest::Automatic);
    requestProbe.setAttribute(QNetworkRequest::AuthenticationReuseAttribute,
                              QNetworkRequest::Automatic);
    requestProbe.setAttribute(QNetworkRequest::CacheLoadControlAttribute,
                              QNetworkRequest::PreferCache);
    requestProbe.setAttribute(QNetworkRequest::CacheSaveControlAttribute, true);
    requestProbe.setRawHeader("Cookie", "session=test");
    requestProbe.setRawHeader("Authorization", "Basic test");
    requestProbe.setRawHeader("Referer", "https://example.com/source");
    requestProbe.setRawHeader("Origin", "https://example.com");

    SailVaultNetwork::hardenRequest(requestProbe);

    const bool redirectPolicySelfTest =
        !requestProbe.attribute(
            QNetworkRequest::FollowRedirectsAttribute, true).toBool()
        && requestProbe.maximumRedirectsAllowed() == 0;

    const bool cookieIsolationSelfTest =
        requestProbe.attribute(
            QNetworkRequest::CookieLoadControlAttribute,
            QNetworkRequest::Automatic).toInt() == QNetworkRequest::Manual
        && requestProbe.attribute(
            QNetworkRequest::CookieSaveControlAttribute,
            QNetworkRequest::Automatic).toInt() == QNetworkRequest::Manual
        && !requestProbe.hasRawHeader("Cookie");

    const bool authIsolationSelfTest =
        requestProbe.attribute(
            QNetworkRequest::AuthenticationReuseAttribute,
            QNetworkRequest::Automatic).toInt() == QNetworkRequest::Manual
        && !requestProbe.hasRawHeader("Authorization");

    const bool cacheIsolationSelfTest =
        requestProbe.attribute(
            QNetworkRequest::CacheLoadControlAttribute,
            QNetworkRequest::PreferCache).toInt() == QNetworkRequest::AlwaysNetwork
        && !requestProbe.attribute(
            QNetworkRequest::CacheSaveControlAttribute, true).toBool()
        && requestProbe.rawHeader("Cache-Control").contains("no-store");

    const bool contextHeaderIsolationSelfTest =
        !requestProbe.hasRawHeader("Referer")
        && !requestProbe.hasRawHeader("Origin");

    const bool requestStatePolicySelfTest =
        cookieIsolationSelfTest
        && authIsolationSelfTest
        && cacheIsolationSelfTest
        && contextHeaderIsolationSelfTest;

    const bool jsonMediaTypeSelfTest =
        SailVaultNetwork::isJsonMediaType(QByteArray("application/json"))
        && SailVaultNetwork::isJsonMediaType(
            QByteArray("application/json; charset=utf-8"))
        && SailVaultNetwork::isJsonMediaType(
            QByteArray("application/problem+json"))
        && !SailVaultNetwork::isJsonMediaType(QByteArray("text/html"))
        && !SailVaultNetwork::isJsonMediaType(QByteArray());

    const bool plainTextMediaTypeSelfTest =
        SailVaultNetwork::isPlainTextMediaType(QByteArray("text/plain"))
        && SailVaultNetwork::isPlainTextMediaType(
            QByteArray("text/plain; charset=utf-8"))
        && !SailVaultNetwork::isPlainTextMediaType(
            QByteArray("application/json"));

    QJsonObject rpcResultProbe;
    rpcResultProbe.insert(QStringLiteral("jsonrpc"), QStringLiteral("2.0"));
    rpcResultProbe.insert(QStringLiteral("id"), 1);
    rpcResultProbe.insert(QStringLiteral("result"), QStringLiteral("ok"));

    QJsonObject rpcStringIdProbe;
    rpcStringIdProbe.insert(QStringLiteral("jsonrpc"), QStringLiteral("2.0"));
    rpcStringIdProbe.insert(QStringLiteral("id"), QStringLiteral("spl"));
    rpcStringIdProbe.insert(QStringLiteral("result"), QJsonObject());

    QJsonObject rpcWrongIdProbe = rpcResultProbe;
    rpcWrongIdProbe.insert(QStringLiteral("id"), 2);

    QJsonObject rpcWrongVersionProbe = rpcResultProbe;
    rpcWrongVersionProbe.insert(QStringLiteral("jsonrpc"), QStringLiteral("1.0"));

    QJsonObject rpcAmbiguousProbe = rpcResultProbe;
    QJsonObject rpcErrorObject;
    rpcErrorObject.insert(QStringLiteral("code"), -32000);
    rpcErrorObject.insert(QStringLiteral("message"), QStringLiteral("test"));
    rpcAmbiguousProbe.insert(QStringLiteral("error"), rpcErrorObject);

    QString rpcDetail;
    const bool jsonRpcEnvelopeSelfTest =
        SailVaultNetwork::validateJsonRpcEnvelope(
            rpcResultProbe, QJsonValue(1), &rpcDetail)
        && SailVaultNetwork::validateJsonRpcEnvelope(
            rpcStringIdProbe, QJsonValue(QStringLiteral("spl")), &rpcDetail)
        && !SailVaultNetwork::validateJsonRpcEnvelope(
            rpcWrongIdProbe, QJsonValue(1), &rpcDetail)
        && !SailVaultNetwork::validateJsonRpcEnvelope(
            rpcWrongVersionProbe, QJsonValue(1), &rpcDetail)
        && !SailVaultNetwork::validateJsonRpcEnvelope(
            rpcAmbiguousProbe, QJsonValue(1), &rpcDetail);

    const bool providerResponsePolicySelfTest =
        jsonMediaTypeSelfTest
        && plainTextMediaTypeSelfTest
        && jsonRpcEnvelopeSelfTest;

    QVariantMap result;
    result.insert(QStringLiteral("endpointCount"), endpointCount);
    result.insert(QStringLiteral("validEndpointCount"), validEndpoints);
    result.insert(QStringLiteral("endpointPolicyPassed"), endpointPolicyPassed);
    result.insert(QStringLiteral("policySelfTestPassed"), policySelfTest);
    result.insert(QStringLiteral("redirectPolicySelfTestPassed"),
                  redirectPolicySelfTest);
    result.insert(QStringLiteral("requestStatePolicySelfTestPassed"),
                  requestStatePolicySelfTest);
    result.insert(QStringLiteral("cookieIsolationPassed"),
                  cookieIsolationSelfTest);
    result.insert(QStringLiteral("authIsolationPassed"),
                  authIsolationSelfTest);
    result.insert(QStringLiteral("cacheIsolationPassed"),
                  cacheIsolationSelfTest);
    result.insert(QStringLiteral("contextHeaderIsolationPassed"),
                  contextHeaderIsolationSelfTest);
    result.insert(QStringLiteral("jsonMediaTypePolicyPassed"),
                  jsonMediaTypeSelfTest && plainTextMediaTypeSelfTest);
    result.insert(QStringLiteral("jsonRpcEnvelopePolicyPassed"),
                  jsonRpcEnvelopeSelfTest);
    result.insert(QStringLiteral("providerResponsePolicyPassed"),
                  providerResponsePolicySelfTest);
    result.insert(QStringLiteral("networkBoundaryPassed"),
                  endpointPolicyPassed && policySelfTest
                  && redirectPolicySelfTest
                  && requestStatePolicySelfTest
                  && providerResponsePolicySelfTest);
    result.insert(QStringLiteral("redirectsBlocked"), redirectPolicySelfTest);
    result.insert(QStringLiteral("responseLimitBytes"),
                  static_cast<qlonglong>(SailVaultNetwork::DefaultMaxResponseBytes));
    result.insert(QStringLiteral("responseLimitText"),
                  SailVaultNetwork::responseLimitText());
    result.insert(QStringLiteral("detail"),
                  QStringLiteral("%1/%2 endpoints pass HTTPS policy · stateless=%3 · responses validated=%4")
                      .arg(validEndpoints).arg(endpointCount)
                      .arg(requestStatePolicySelfTest
                           ? QStringLiteral("yes")
                           : QStringLiteral("no"))
                      .arg(providerResponsePolicySelfTest
                           ? QStringLiteral("yes")
                           : QStringLiteral("no")));
    return result;
}


QVariantMap AppTools::privacyDiagnostics() const
{
    return SailVaultSettings::privacyDiagnostics();
}

QVariantMap AppTools::clearCachedPublicData() const
{
    QString detail;
    const bool passed = SailVaultSettings::clearCachedPublicData(&detail);
    QVariantMap result = SailVaultSettings::privacyDiagnostics();
    result.insert(QStringLiteral("operationPassed"), passed);
    result.insert(QStringLiteral("operationDetail"), detail);
    return result;
}

QVariantMap AppTools::clearQuarantinedSettingsData() const
{
    QString detail;
    const bool passed = SailVaultSettings::clearQuarantinedSettingsData(&detail);
    QVariantMap result = SailVaultSettings::privacyDiagnostics();
    result.insert(QStringLiteral("operationPassed"), passed);
    result.insert(QStringLiteral("operationDetail"), detail);
    return result;
}
