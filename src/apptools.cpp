#include "apptools.h"
#include "settingsstore.h"

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
