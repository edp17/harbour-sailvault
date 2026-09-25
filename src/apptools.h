#pragma once

#include <QObject>
#include <QString>
#include <QVariantMap>

class AppTools : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString applicationVersion READ applicationVersion CONSTANT)
    Q_PROPERTY(QString packageVersion READ packageVersion CONSTANT)
    Q_PROPERTY(QString milestone READ milestone CONSTANT)
    Q_PROPERTY(QString buildLabel READ buildLabel CONSTANT)
    Q_PROPERTY(QString walletCoreVersion READ walletCoreVersion CONSTANT)

public:
    explicit AppTools(QObject *parent = nullptr);

    QString applicationVersion() const;
    QString packageVersion() const;
    QString milestone() const;
    QString buildLabel() const;
    QString walletCoreVersion() const;

    Q_INVOKABLE void copyText(const QString &text) const;
    Q_INVOKABLE QVariantMap settingsDiagnostics() const;
    Q_INVOKABLE QVariantMap rerunSettingsPersistenceProbe() const;
    Q_INVOKABLE QVariantMap privacyDiagnostics() const;
    Q_INVOKABLE QVariantMap clearCachedPublicData() const;
    Q_INVOKABLE QVariantMap clearQuarantinedSettingsData() const;
};
