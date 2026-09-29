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
    result.insert(QStringLiteral("networkBoundaryPassed"),
                  endpointPolicyPassed && policySelfTest
                  && redirectPolicySelfTest
                  && requestStatePolicySelfTest);
    result.insert(QStringLiteral("redirectsBlocked"), redirectPolicySelfTest);
    result.insert(QStringLiteral("responseLimitBytes"),
                  static_cast<qlonglong>(SailVaultNetwork::DefaultMaxResponseBytes));
    result.insert(QStringLiteral("responseLimitText"),
                  SailVaultNetwork::responseLimitText());
    result.insert(QStringLiteral("detail"),
                  QStringLiteral("%1/%2 endpoints pass HTTPS policy · requests stateless=%3")
                      .arg(validEndpoints).arg(endpointCount)
                      .arg(requestStatePolicySelfTest
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
