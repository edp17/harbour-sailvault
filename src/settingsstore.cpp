#include "settingsstore.h"

#include <QCryptographicHash>
#include <QDateTime>
#include <QDir>
#include <QFileInfo>
#include <QSettings>
#include <QStandardPaths>
#include <QUuid>
#include <QUrl>

namespace {
const char kLegacyOrganization[] = "harbour-sailvault";
const char kLegacyApplication[] = "harbour-sailvault";
const char kMigrationMarker[] = "meta/legacySettingsMigratedV1";
const char kSchemaKey[] = "meta/schemaVersion";
const char kInitializedKey[] = "meta/initialized";
const char kInstallIdKey[] = "meta/installId";
const char kFirstStartedAtKey[] = "meta/firstStartedAt";
const char kLastStartedAtKey[] = "meta/lastStartedAt";
const char kLaunchCountKey[] = "meta/launchCount";
const char kLastAppVersionKey[] = "meta/lastAppVersion";
const char kLastMilestoneKey[] = "meta/lastMilestone";
const int kCurrentSchemaVersion = 4;


bool hasPrefix(const QString &key, const QStringList &prefixes)
{
    for (const QString &prefix : prefixes) {
        if (key.startsWith(prefix))
            return true;
    }
    return false;
}

QStringList publicCachePrefixes()
{
    return QStringList()
        << QStringLiteral("portfolioCache/")
        << QStringLiteral("publicBalanceCache/")
        << QStringLiteral("priceCache/")
        << QStringLiteral("portfolioHistory/v1/");
}

QStringList quarantinedPrefixes()
{
    return QStringList()
        << QStringLiteral("recovery/corrupt/");
}

bool g_initialized = false;
bool g_initializing = false;
bool g_persistenceProbePassed = false;
QString g_persistenceProbeDetail = QStringLiteral("Not run");
QString g_initializationStatus = QStringLiteral("Not initialized");

QString settingsDirectory()
{
    QString base = QStandardPaths::writableLocation(QStandardPaths::ConfigLocation);
    if (base.isEmpty())
        base = QDir::homePath() + QStringLiteral("/.config");
    return QDir(base).filePath(QStringLiteral("harbour-sailvault"));
}

QStringList legacyKnownKeys()
{
    return QStringList()
        << QStringLiteral("security/autoLockEnabled")
        << QStringLiteral("security/backgroundDelaySeconds")
        << QStringLiteral("security/inactivityMinutes")
        << QStringLiteral("network/ethereumRpcUrl")
        << QStringLiteral("network/ethereumExplorerUrl")
        << QStringLiteral("network/bitcoinApiUrl")
        << QStringLiteral("network/solanaRpcUrl")
        << QStringLiteral("network/solanaTokenRpcUrl")
        << QStringLiteral("network/offlineMode")
        << QStringLiteral("portfolio/fiatCurrency")
        << QStringLiteral("portfolio/autoRefreshAfterUnlock")
        << QStringLiteral("tokens/hiddenIds")
        << QStringLiteral("watchOnly/label")
        << QStringLiteral("watchOnly/ethereumAddress")
        << QStringLiteral("watchOnly/bitcoinAddress")
        << QStringLiteral("watchOnly/solanaAddress")
        << QStringLiteral("addressBook/contacts");
}

bool validHttpsEndpoint(const QString &text)
{
    const QUrl url(text.trimmed());
    return url.isValid()
        && url.scheme().compare(QStringLiteral("https"), Qt::CaseInsensitive) == 0
        && !url.host().isEmpty();
}

void migrateLegacy(QSettings &target)
{
    if (target.value(QString::fromLatin1(kMigrationMarker), false).toBool())
        return;

    QSettings legacy(QString::fromLatin1(kLegacyOrganization),
                     QString::fromLatin1(kLegacyApplication));
    const QStringList keys = legacyKnownKeys();
    for (const QString &key : keys) {
        if (!target.contains(key) && legacy.contains(key))
            target.setValue(key, legacy.value(key));
    }
    target.setValue(QString::fromLatin1(kMigrationMarker), true);
}

void sanitizePreferenceSettings(QSettings &settings)
{
    const QStringList endpointKeys = QStringList()
        << QStringLiteral("network/ethereumRpcUrl")
        << QStringLiteral("network/ethereumExplorerUrl")
        << QStringLiteral("network/bitcoinApiUrl")
        << QStringLiteral("network/solanaRpcUrl")
        << QStringLiteral("network/solanaTokenRpcUrl");

    for (const QString &key : endpointKeys) {
        if (settings.contains(key)
                && !validHttpsEndpoint(settings.value(key).toString())) {
            // Missing endpoints deliberately fall back to compiled HTTPS
            // defaults. Do not preserve malformed or plaintext endpoints.
            settings.remove(key);
        }
    }

    if (settings.contains(QStringLiteral("portfolio/fiatCurrency"))) {
        const QString currency = settings.value(
            QStringLiteral("portfolio/fiatCurrency")).toString().trimmed().toUpper();
        if (currency == QStringLiteral("GBP")
                || currency == QStringLiteral("USD")
                || currency == QStringLiteral("EUR")) {
            settings.setValue(QStringLiteral("portfolio/fiatCurrency"), currency);
        } else {
            settings.setValue(QStringLiteral("portfolio/fiatCurrency"),
                              QStringLiteral("GBP"));
        }
    }

    if (settings.contains(QStringLiteral("security/backgroundDelaySeconds"))) {
        const int value = settings.value(
            QStringLiteral("security/backgroundDelaySeconds")).toInt();
        if (value != 0 && value != 30 && value != 60 && value != 300)
            settings.setValue(QStringLiteral("security/backgroundDelaySeconds"), 0);
    }

    if (settings.contains(QStringLiteral("security/inactivityMinutes"))) {
        const int value = settings.value(
            QStringLiteral("security/inactivityMinutes")).toInt();
        if (value != 0 && value != 1 && value != 5 && value != 15)
            settings.setValue(QStringLiteral("security/inactivityMinutes"), 5);
    }
}

bool performPersistenceProbe(QString *detail)
{
    const QString path = settingsDirectory() + QStringLiteral("/settings.ini");
    QDir dir(settingsDirectory());
    if (!dir.exists() && !dir.mkpath(QStringLiteral("."))) {
        if (detail)
            *detail = QStringLiteral("Cannot create configuration directory");
        return false;
    }

    const QString key = QStringLiteral("meta/persistenceProbe");
    const QString token = QUuid::createUuid().toString();

    QSettings writer(path, QSettings::IniFormat);
    writer.setValue(key, token);
    writer.sync();
    if (writer.status() != QSettings::NoError) {
        if (detail)
            *detail = QStringLiteral("INI write failed (%1)")
                .arg(static_cast<int>(writer.status()));
        return false;
    }

    QSettings reader(path, QSettings::IniFormat);
    const bool matched = reader.value(key).toString() == token;

    QSettings cleanup(path, QSettings::IniFormat);
    cleanup.remove(key);
    cleanup.sync();

    if (detail) {
        *detail = matched
            ? QStringLiteral("Write/sync/reopen/read round-trip passed")
            : QStringLiteral("Reopened INI did not return the value just written");
    }
    return matched;
}

void ensureInitialized()
{
    if (g_initialized || g_initializing)
        return;

    g_initializing = true;

    QDir dir(settingsDirectory());
    if (!dir.exists() && !dir.mkpath(QStringLiteral("."))) {
        g_initializationStatus = QStringLiteral(
            "Cannot create SailVault configuration directory");
        g_persistenceProbePassed = false;
        g_persistenceProbeDetail = g_initializationStatus;
        g_initialized = true;
        g_initializing = false;
        return;
    }

    const QString path = dir.filePath(QStringLiteral("settings.ini"));
    QSettings target(path, QSettings::IniFormat);

    migrateLegacy(target);

    // Preferences are cheap to validate on every start. This also recovers
    // gracefully from a hand-edited or partially-written INI file even after
    // the current schema has already been recorded.
    sanitizePreferenceSettings(target);

    if (!target.contains(QString::fromLatin1(kInstallIdKey))) {
        target.setValue(QString::fromLatin1(kInstallIdKey),
                        QUuid::createUuid().toString());
    }

    const QString startedAt = QDateTime::currentDateTimeUtc().toString(Qt::ISODate);
    if (!target.contains(QString::fromLatin1(kFirstStartedAtKey)))
        target.setValue(QString::fromLatin1(kFirstStartedAtKey), startedAt);
    target.setValue(QString::fromLatin1(kLastStartedAtKey), startedAt);
    target.setValue(QString::fromLatin1(kLaunchCountKey),
                    target.value(QString::fromLatin1(kLaunchCountKey), 0).toInt() + 1);
    target.setValue(QString::fromLatin1(kLastAppVersionKey),
                    QStringLiteral(SAILVAULT_APP_VERSION));
    target.setValue(QString::fromLatin1(kLastMilestoneKey),
                    QStringLiteral(SAILVAULT_MILESTONE));

    target.setValue(QString::fromLatin1(kSchemaKey), kCurrentSchemaVersion);
    target.setValue(QString::fromLatin1(kInitializedKey), true);
    target.sync();

    if (target.status() != QSettings::NoError) {
        g_initializationStatus = QStringLiteral("INI initialization failed (%1)")
            .arg(static_cast<int>(target.status()));
        g_persistenceProbePassed = false;
        g_persistenceProbeDetail = g_initializationStatus;
    } else {
        g_persistenceProbePassed = performPersistenceProbe(
            &g_persistenceProbeDetail);
        g_initializationStatus = g_persistenceProbePassed
            ? QStringLiteral("Persistent INI storage ready")
            : QStringLiteral("Persistent INI storage probe failed");
    }

    g_initialized = true;
    g_initializing = false;
}
} // namespace

namespace SailVaultSettings
{
void initialize()
{
    ensureInitialized();
}

QString filePath()
{
    return settingsDirectory() + QStringLiteral("/settings.ini");
}

void migrateLegacySettings()
{
    ensureInitialized();
}

QVariant value(const QString &key, const QVariant &defaultValue)
{
    ensureInitialized();
    QSettings settings(filePath(), QSettings::IniFormat);
    return settings.value(key, defaultValue);
}

bool contains(const QString &key)
{
    ensureInitialized();
    QSettings settings(filePath(), QSettings::IniFormat);
    return settings.contains(key);
}

bool setValue(const QString &key, const QVariant &newValue)
{
    ensureInitialized();
    QSettings settings(filePath(), QSettings::IniFormat);
    settings.setValue(key, newValue);
    settings.sync();
    return settings.status() == QSettings::NoError;
}

bool remove(const QString &key)
{
    ensureInitialized();
    QSettings settings(filePath(), QSettings::IniFormat);
    settings.remove(key);
    settings.sync();
    return settings.status() == QSettings::NoError;
}

bool remove(const QStringList &keys)
{
    ensureInitialized();
    QSettings settings(filePath(), QSettings::IniFormat);
    for (const QString &key : keys)
        settings.remove(key);
    settings.sync();
    return settings.status() == QSettings::NoError;
}

bool quarantineValue(const QString &key, const QString &reason)
{
    ensureInitialized();
    QSettings settings(filePath(), QSettings::IniFormat);
    if (!settings.contains(key))
        return true;

    const QByteArray digest = QCryptographicHash::hash(
        key.toUtf8(), QCryptographicHash::Sha256).toHex();
    const QString base = QStringLiteral("recovery/corrupt/")
        + QString::fromLatin1(digest) + QLatin1Char('/');

    settings.setValue(base + QStringLiteral("originalKey"), key);
    settings.setValue(base + QStringLiteral("reason"), reason);
    settings.setValue(base + QStringLiteral("savedAt"),
                      QDateTime::currentDateTimeUtc().toString(Qt::ISODate));
    settings.setValue(base + QStringLiteral("value"), settings.value(key));
    settings.remove(key);
    settings.sync();
    return settings.status() == QSettings::NoError;
}

int schemaVersion()
{
    ensureInitialized();
    QSettings settings(filePath(), QSettings::IniFormat);
    return settings.value(QString::fromLatin1(kSchemaKey), 0).toInt();
}

bool runPersistenceProbe(QString *detail)
{
    ensureInitialized();
    g_persistenceProbePassed = performPersistenceProbe(
        &g_persistenceProbeDetail);
    if (detail)
        *detail = g_persistenceProbeDetail;
    return g_persistenceProbePassed;
}

QVariantMap diagnostics()
{
    ensureInitialized();

    QSettings settings(filePath(), QSettings::IniFormat);
    const QFileInfo info(filePath());

    QVariantMap result;
    result.insert(QStringLiteral("path"), filePath());
    result.insert(QStringLiteral("directory"), settingsDirectory());
    result.insert(QStringLiteral("schemaVersion"),
                  settings.value(QString::fromLatin1(kSchemaKey), 0).toInt());
    result.insert(QStringLiteral("initialized"),
                  settings.value(QString::fromLatin1(kInitializedKey), false).toBool());
    result.insert(QStringLiteral("legacyMigrated"),
                  settings.value(QString::fromLatin1(kMigrationMarker), false).toBool());
    result.insert(QStringLiteral("fileExists"), info.exists());
    result.insert(QStringLiteral("fileWritable"), info.exists()
        ? info.isWritable() : QFileInfo(settingsDirectory()).isWritable());
    result.insert(QStringLiteral("fileSize"),
                  info.exists() ? static_cast<qlonglong>(info.size()) : 0);
    result.insert(QStringLiteral("keyCount"), settings.allKeys().size());
    result.insert(QStringLiteral("settingsStatus"),
                  static_cast<int>(settings.status()));
    result.insert(QStringLiteral("persistenceProbePassed"),
                  g_persistenceProbePassed);
    result.insert(QStringLiteral("persistenceProbeDetail"),
                  g_persistenceProbeDetail);
    result.insert(QStringLiteral("initializationStatus"),
                  g_initializationStatus);
    result.insert(QStringLiteral("firstStartedAt"),
                  settings.value(QString::fromLatin1(kFirstStartedAtKey)).toString());
    result.insert(QStringLiteral("lastStartedAt"),
                  settings.value(QString::fromLatin1(kLastStartedAtKey)).toString());
    result.insert(QStringLiteral("launchCount"),
                  settings.value(QString::fromLatin1(kLaunchCountKey), 0).toInt());
    result.insert(QStringLiteral("lastAppVersion"),
                  settings.value(QString::fromLatin1(kLastAppVersionKey)).toString());
    result.insert(QStringLiteral("lastMilestone"),
                  settings.value(QString::fromLatin1(kLastMilestoneKey)).toString());
    return result;
}


QVariantMap privacyDiagnostics()
{
    ensureInitialized();

    QSettings settings(filePath(), QSettings::IniFormat);
    const QStringList keys = settings.allKeys();

    int cachedPublicKeys = 0;
    int historyContexts = 0;
    int quarantinedKeys = 0;
    int quarantinedRecords = 0;

    for (const QString &key : keys) {
        if (hasPrefix(key, publicCachePrefixes()))
            ++cachedPublicKeys;
        if (key.startsWith(QStringLiteral("portfolioHistory/v1/")))
            ++historyContexts;
        if (key.startsWith(QStringLiteral("recovery/corrupt/"))) {
            ++quarantinedKeys;
            if (key.endsWith(QStringLiteral("/originalKey")))
                ++quarantinedRecords;
        }
    }

    const bool watchOnlyStored =
        !settings.value(QStringLiteral("watchOnly/ethereumAddress")).toString().trimmed().isEmpty()
        || !settings.value(QStringLiteral("watchOnly/bitcoinAddress")).toString().trimmed().isEmpty()
        || !settings.value(QStringLiteral("watchOnly/solanaAddress")).toString().trimmed().isEmpty();

    const bool addressBookStored =
        !settings.value(QStringLiteral("addressBook/contacts")).toString().trimmed().isEmpty();

    QVariantMap result;
    result.insert(QStringLiteral("cachedPublicKeys"), cachedPublicKeys);
    result.insert(QStringLiteral("historyContexts"), historyContexts);
    result.insert(QStringLiteral("quarantinedKeys"), quarantinedKeys);
    result.insert(QStringLiteral("quarantinedRecords"), quarantinedRecords);
    result.insert(QStringLiteral("watchOnlyStored"), watchOnlyStored);
    result.insert(QStringLiteral("addressBookStored"), addressBookStored);
    result.insert(QStringLiteral("hiddenTokenPreferencesStored"),
                  settings.contains(QStringLiteral("tokens/hiddenIds")));
    result.insert(QStringLiteral("settingsPath"), filePath());
    return result;
}

bool clearCachedPublicData(QString *detail)
{
    ensureInitialized();

    QSettings settings(filePath(), QSettings::IniFormat);
    const QStringList keys = settings.allKeys();
    int removed = 0;

    for (const QString &key : keys) {
        if (!hasPrefix(key, publicCachePrefixes()))
            continue;
        settings.remove(key);
        ++removed;
    }

    settings.sync();
    const bool ok = settings.status() == QSettings::NoError;
    if (detail) {
        *detail = ok
            ? QStringLiteral("Removed %1 cached public-data values").arg(removed)
            : QStringLiteral("Failed while clearing cached public data");
    }
    return ok;
}

bool clearQuarantinedSettingsData(QString *detail)
{
    ensureInitialized();

    QSettings settings(filePath(), QSettings::IniFormat);
    const QStringList keys = settings.allKeys();
    int removed = 0;

    for (const QString &key : keys) {
        if (!hasPrefix(key, quarantinedPrefixes()))
            continue;
        settings.remove(key);
        ++removed;
    }

    settings.sync();
    const bool ok = settings.status() == QSettings::NoError;
    if (detail) {
        *detail = ok
            ? QStringLiteral("Removed %1 quarantined non-sensitive values").arg(removed)
            : QStringLiteral("Failed while clearing quarantined settings data");
    }
    return ok;
}

} // namespace SailVaultSettings
