#pragma once

#include <QString>
#include <QStringList>
#include <QVariant>
#include <QVariantMap>

namespace SailVaultSettings
{
void initialize();
QString filePath();
QVariant value(const QString &key, const QVariant &defaultValue = QVariant());
bool contains(const QString &key);
bool setValue(const QString &key, const QVariant &value);
bool remove(const QString &key);
bool remove(const QStringList &keys);
void migrateLegacySettings();

// Move a malformed non-sensitive setting out of the active namespace while
// retaining a local copy for diagnostics/recovery. No Sailfish Secrets data is
// ever handled by this settings layer.
bool quarantineValue(const QString &key, const QString &reason);

int schemaVersion();
QVariantMap diagnostics();
QVariantMap privacyDiagnostics();
bool runPersistenceProbe(QString *detail = nullptr);
bool clearCachedPublicData(QString *detail = nullptr);
bool clearQuarantinedSettingsData(QString *detail = nullptr);
} // namespace SailVaultSettings
