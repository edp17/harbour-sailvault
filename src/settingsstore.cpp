#include "settingsstore.h"

#include <QCryptographicHash>
#include <QDateTime>
#include <QDir>
#include <QFileInfo>
#include <QSettings>
#include <QStandardPaths>
#include <QTemporaryDir>
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
const char kProfileOriginKey[] = "meta/profileOrigin";
const char kFirstRecordedAppVersionKey[] = "meta/firstRecordedAppVersion";
const char kLastUpgradeFromVersionKey[] = "meta/lastUpgradeFromVersion";
const char kLastUpgradeFromMilestoneKey[] = "meta/lastUpgradeFromMilestone";
const char kLastUpgradeAtKey[] = "meta/lastUpgradeAt";
const char kLegacyImportCountKey[] = "meta/legacyImportedKeyCount";
const int kCurrentSchemaVersion = 5;


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
bool g_storageLifecycleProbePassed = false;
QString g_storageLifecycleProbeDetail = QStringLiteral("Not run");
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
        && !url.host().isEmpty()
        && url.userInfo().isEmpty();
}

int importLegacyKnownKeys(QSettings &source, QSettings &target)
{
    int imported = 0;
    const QStringList keys = legacyKnownKeys();
    for (const QString &key : keys) {
        if (!target.contains(key) && source.contains(key)) {
            target.setValue(key, source.value(key));
            ++imported;
        }
    }
    return imported;
}

int migrateLegacy(QSettings &target)
{
    if (target.value(QString::fromLatin1(kMigrationMarker), false).toBool()) {
        return target.value(QString::fromLatin1(kLegacyImportCountKey), 0).toInt();
    }

    QSettings legacy(QString::fromLatin1(kLegacyOrganization),
                     QString::fromLatin1(kLegacyApplication));
    const int imported = importLegacyKnownKeys(legacy, target);
    target.setValue(QString::fromLatin1(kMigrationMarker), true);
    target.setValue(QString::fromLatin1(kLegacyImportCountKey), imported);
    return imported;
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

void applyStartupMetadata(QSettings &target,
                          const QString &startedAt,
                          const QString &appVersion,
                          const QString &milestone,
                          int legacyImportedCount)
{
    const bool hadInstallIdentity =
        !target.value(QString::fromLatin1(kInstallIdKey)).toString().trimmed().isEmpty();
    const QString previousVersion =
        target.value(QString::fromLatin1(kLastAppVersionKey)).toString().trimmed();
    const QString previousMilestone =
        target.value(QString::fromLatin1(kLastMilestoneKey)).toString().trimmed();

    if (!hadInstallIdentity) {
        target.setValue(QString::fromLatin1(kInstallIdKey),
                        QUuid::createUuid().toString());
    }

    if (!target.contains(QString::fromLatin1(kProfileOriginKey))) {
        const QString origin = legacyImportedCount > 0
            ? QStringLiteral("legacy-import")
            : (hadInstallIdentity
               ? QStringLiteral("existing")
               : QStringLiteral("fresh"));
        target.setValue(QString::fromLatin1(kProfileOriginKey), origin);
    }

    if (!target.contains(QString::fromLatin1(kFirstRecordedAppVersionKey))) {
        target.setValue(QString::fromLatin1(kFirstRecordedAppVersionKey),
                        previousVersion.isEmpty() ? appVersion : previousVersion);
    }

    if (!target.contains(QString::fromLatin1(kFirstStartedAtKey)))
        target.setValue(QString::fromLatin1(kFirstStartedAtKey), startedAt);

    if (!previousVersion.isEmpty() && previousVersion != appVersion) {
        target.setValue(QString::fromLatin1(kLastUpgradeFromVersionKey),
                        previousVersion);
        target.setValue(QString::fromLatin1(kLastUpgradeFromMilestoneKey),
                        previousMilestone);
        target.setValue(QString::fromLatin1(kLastUpgradeAtKey), startedAt);
    }

    target.setValue(QString::fromLatin1(kLastStartedAtKey), startedAt);
    target.setValue(QString::fromLatin1(kLaunchCountKey),
                    target.value(QString::fromLatin1(kLaunchCountKey), 0).toInt() + 1);
    target.setValue(QString::fromLatin1(kLastAppVersionKey), appVersion);
    target.setValue(QString::fromLatin1(kLastMilestoneKey), milestone);
    target.setValue(QString::fromLatin1(kLegacyImportCountKey), legacyImportedCount);
    target.setValue(QString::fromLatin1(kSchemaKey), kCurrentSchemaVersion);
    target.setValue(QString::fromLatin1(kInitializedKey), true);
}

bool storageMetadataCoherent(QSettings &settings, QString *detail)
{
    const QString origin =
        settings.value(QString::fromLatin1(kProfileOriginKey)).toString();
    const bool originValid = origin == QStringLiteral("fresh")
        || origin == QStringLiteral("existing")
        || origin == QStringLiteral("legacy-import");
    const bool passed =
        settings.value(QString::fromLatin1(kSchemaKey), 0).toInt()
            == kCurrentSchemaVersion
        && settings.value(QString::fromLatin1(kInitializedKey), false).toBool()
        && !settings.value(QString::fromLatin1(kInstallIdKey)).toString().trimmed().isEmpty()
        && !settings.value(QString::fromLatin1(kFirstStartedAtKey)).toString().trimmed().isEmpty()
        && !settings.value(QString::fromLatin1(kLastStartedAtKey)).toString().trimmed().isEmpty()
        && settings.value(QString::fromLatin1(kLaunchCountKey), 0).toInt() >= 1
        && settings.value(QString::fromLatin1(kLastAppVersionKey)).toString()
            == QStringLiteral(SAILVAULT_APP_VERSION)
        && settings.value(QString::fromLatin1(kLastMilestoneKey)).toString()
            == QStringLiteral(SAILVAULT_MILESTONE)
        && !settings.value(QString::fromLatin1(kFirstRecordedAppVersionKey))
                .toString().trimmed().isEmpty()
        && originValid;

    if (detail) {
        *detail = passed
            ? QStringLiteral("Current profile metadata is coherent")
            : QStringLiteral("Current profile metadata is incomplete or inconsistent");
    }
    return passed;
}

bool performStorageLifecycleProbe(QString *detail)
{
    QTemporaryDir tempDir;
    if (!tempDir.isValid()) {
        if (detail)
            *detail = QStringLiteral("Cannot create isolated storage test directory");
        return false;
    }

    const QString currentVersion = QStringLiteral(SAILVAULT_APP_VERSION);
    const QString currentMilestone = QStringLiteral(SAILVAULT_MILESTONE);
    const QString testTime = QStringLiteral("2030-01-02T03:04:05Z");

    // Fresh profile: exact startup metadata path on a completely empty INI.
    const QString freshPath = tempDir.path() + QStringLiteral("/fresh.ini");
    {
        QSettings fresh(freshPath, QSettings::IniFormat);
        sanitizePreferenceSettings(fresh);
        applyStartupMetadata(fresh, testTime, currentVersion, currentMilestone, 0);
        fresh.sync();
        const bool freshPassed = fresh.status() == QSettings::NoError
            && fresh.value(QString::fromLatin1(kProfileOriginKey)).toString()
                == QStringLiteral("fresh")
            && !fresh.value(QString::fromLatin1(kInstallIdKey)).toString().isEmpty()
            && fresh.value(QString::fromLatin1(kLaunchCountKey)).toInt() == 1
            && fresh.value(QString::fromLatin1(kFirstRecordedAppVersionKey)).toString()
                == currentVersion
            && fresh.value(QString::fromLatin1(kLastAppVersionKey)).toString()
                == currentVersion
            && fresh.value(QString::fromLatin1(kSchemaKey)).toInt()
                == kCurrentSchemaVersion;
        if (!freshPassed) {
            if (detail)
                *detail = QStringLiteral("Fresh-profile simulation failed");
            return false;
        }
    }

    // Upgrade profile: preserve identity, first-start metadata and user settings
    // while recording the source build and advancing the schema/build identity.
    const QString upgradePath = tempDir.path() + QStringLiteral("/upgrade.ini");
    {
        QSettings upgrade(upgradePath, QSettings::IniFormat);
        const QString installId = QStringLiteral("m47-upgrade-probe-install-id");
        const QString firstStarted = QStringLiteral("2029-12-01T00:00:00Z");
        upgrade.setValue(QString::fromLatin1(kInstallIdKey), installId);
        upgrade.setValue(QString::fromLatin1(kFirstStartedAtKey), firstStarted);
        upgrade.setValue(QString::fromLatin1(kLaunchCountKey), 7);
        upgrade.setValue(QString::fromLatin1(kLastAppVersionKey), QStringLiteral("0.1.0"));
        upgrade.setValue(QString::fromLatin1(kLastMilestoneKey), QStringLiteral("1"));
        upgrade.setValue(QString::fromLatin1(kProfileOriginKey), QStringLiteral("existing"));
        upgrade.setValue(QString::fromLatin1(kFirstRecordedAppVersionKey),
                         QStringLiteral("0.1.0"));
        upgrade.setValue(QStringLiteral("portfolio/fiatCurrency"), QStringLiteral("EUR"));
        upgrade.setValue(QStringLiteral("network/offlineMode"), true);
        upgrade.setValue(QStringLiteral("security/inactivityMinutes"), 15);
        upgrade.setValue(QStringLiteral("tokens/hiddenIds"),
                         QStringList() << QStringLiteral("eth:test-token"));
        upgrade.setValue(QString::fromLatin1(kSchemaKey), 4);
        upgrade.setValue(QString::fromLatin1(kInitializedKey), true);
        upgrade.sync();

        sanitizePreferenceSettings(upgrade);
        applyStartupMetadata(upgrade, testTime, currentVersion, currentMilestone, 0);
        upgrade.sync();

        const bool upgradePassed = upgrade.status() == QSettings::NoError
            && upgrade.value(QString::fromLatin1(kInstallIdKey)).toString() == installId
            && upgrade.value(QString::fromLatin1(kFirstStartedAtKey)).toString() == firstStarted
            && upgrade.value(QString::fromLatin1(kLaunchCountKey)).toInt() == 8
            && upgrade.value(QString::fromLatin1(kLastUpgradeFromVersionKey)).toString()
                == QStringLiteral("0.1.0")
            && upgrade.value(QString::fromLatin1(kLastUpgradeFromMilestoneKey)).toString()
                == QStringLiteral("1")
            && upgrade.value(QString::fromLatin1(kLastUpgradeAtKey)).toString() == testTime
            && upgrade.value(QString::fromLatin1(kLastAppVersionKey)).toString()
                == currentVersion
            && upgrade.value(QStringLiteral("portfolio/fiatCurrency")).toString()
                == QStringLiteral("EUR")
            && upgrade.value(QStringLiteral("network/offlineMode")).toBool()
            && upgrade.value(QStringLiteral("security/inactivityMinutes")).toInt() == 15
            && upgrade.value(QStringLiteral("tokens/hiddenIds")).toStringList()
                == (QStringList() << QStringLiteral("eth:test-token"));
        if (!upgradePassed) {
            if (detail)
                *detail = QStringLiteral("Upgrade-preservation simulation failed");
            return false;
        }
    }

    // Legacy import: exercise the same known-key copier and sanitizer in an
    // isolated pair of INI files. No real legacy/user settings are modified.
    const QString legacyPath = tempDir.path() + QStringLiteral("/legacy.ini");
    const QString importedPath = tempDir.path() + QStringLiteral("/imported.ini");
    {
        QSettings legacy(legacyPath, QSettings::IniFormat);
        legacy.setValue(QStringLiteral("portfolio/fiatCurrency"), QStringLiteral("USD"));
        legacy.setValue(QStringLiteral("watchOnly/label"), QStringLiteral("Probe watch"));
        legacy.setValue(QStringLiteral("network/ethereumRpcUrl"),
                        QStringLiteral("http") + QStringLiteral("://unsafe.example/rpc"));
        legacy.sync();

        QSettings imported(importedPath, QSettings::IniFormat);
        const int importedCount = importLegacyKnownKeys(legacy, imported);
        imported.setValue(QString::fromLatin1(kMigrationMarker), true);
        imported.setValue(QString::fromLatin1(kLegacyImportCountKey), importedCount);
        sanitizePreferenceSettings(imported);
        applyStartupMetadata(imported, testTime, currentVersion, currentMilestone,
                             importedCount);
        imported.sync();

        const bool legacyPassed = imported.status() == QSettings::NoError
            && importedCount == 3
            && imported.value(QString::fromLatin1(kProfileOriginKey)).toString()
                == QStringLiteral("legacy-import")
            && imported.value(QStringLiteral("portfolio/fiatCurrency")).toString()
                == QStringLiteral("USD")
            && imported.value(QStringLiteral("watchOnly/label")).toString()
                == QStringLiteral("Probe watch")
            && !imported.contains(QStringLiteral("network/ethereumRpcUrl"));
        if (!legacyPassed) {
            if (detail)
                *detail = QStringLiteral("Legacy-import/sanitization simulation failed");
            return false;
        }
    }

    if (detail) {
        *detail = QStringLiteral(
            "Fresh initialization, upgrade preservation and legacy import simulations passed");
    }
    return true;
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
        g_storageLifecycleProbePassed = false;
        g_storageLifecycleProbeDetail = g_initializationStatus;
        g_initialized = true;
        g_initializing = false;
        return;
    }

    const QString path = dir.filePath(QStringLiteral("settings.ini"));
    QSettings target(path, QSettings::IniFormat);

    const int legacyImportedCount = migrateLegacy(target);

    // Preferences are cheap to validate on every start. This also recovers
    // gracefully from a hand-edited or partially-written INI file even after
    // the current schema has already been recorded.
    sanitizePreferenceSettings(target);

    const QString startedAt = QDateTime::currentDateTimeUtc().toString(Qt::ISODate);
    applyStartupMetadata(target, startedAt,
                         QStringLiteral(SAILVAULT_APP_VERSION),
                         QStringLiteral(SAILVAULT_MILESTONE),
                         legacyImportedCount);
    target.sync();

    if (target.status() != QSettings::NoError) {
        g_initializationStatus = QStringLiteral("INI initialization failed (%1)")
            .arg(static_cast<int>(target.status()));
        g_persistenceProbePassed = false;
        g_persistenceProbeDetail = g_initializationStatus;
        g_storageLifecycleProbePassed = false;
        g_storageLifecycleProbeDetail = g_initializationStatus;
    } else {
        g_persistenceProbePassed = performPersistenceProbe(
            &g_persistenceProbeDetail);
        g_storageLifecycleProbePassed = performStorageLifecycleProbe(
            &g_storageLifecycleProbeDetail);
        g_initializationStatus = g_persistenceProbePassed
                && g_storageLifecycleProbePassed
            ? QStringLiteral("Persistent INI storage and lifecycle checks ready")
            : QStringLiteral("Persistent storage release checks failed");
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

bool runStorageLifecycleProbe(QString *detail)
{
    ensureInitialized();
    g_storageLifecycleProbePassed = performStorageLifecycleProbe(
        &g_storageLifecycleProbeDetail);
    if (detail)
        *detail = g_storageLifecycleProbeDetail;
    return g_storageLifecycleProbePassed;
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

    QString metadataDetail;
    const bool metadataCoherent = storageMetadataCoherent(settings, &metadataDetail);
    result.insert(QStringLiteral("storageLifecycleProbePassed"),
                  g_storageLifecycleProbePassed);
    result.insert(QStringLiteral("storageLifecycleProbeDetail"),
                  g_storageLifecycleProbeDetail);
    result.insert(QStringLiteral("metadataCoherent"), metadataCoherent);
    result.insert(QStringLiteral("metadataDetail"), metadataDetail);
    result.insert(QStringLiteral("storageReleaseGatePassed"),
                  g_persistenceProbePassed
                  && g_storageLifecycleProbePassed
                  && metadataCoherent);
    result.insert(QStringLiteral("installIdentityPresent"),
                  !settings.value(QString::fromLatin1(kInstallIdKey))
                       .toString().trimmed().isEmpty());
    result.insert(QStringLiteral("profileOrigin"),
                  settings.value(QString::fromLatin1(kProfileOriginKey)).toString());
    result.insert(QStringLiteral("firstRecordedAppVersion"),
                  settings.value(QString::fromLatin1(kFirstRecordedAppVersionKey)).toString());
    result.insert(QStringLiteral("lastUpgradeFromVersion"),
                  settings.value(QString::fromLatin1(kLastUpgradeFromVersionKey)).toString());
    result.insert(QStringLiteral("lastUpgradeFromMilestone"),
                  settings.value(QString::fromLatin1(kLastUpgradeFromMilestoneKey)).toString());
    result.insert(QStringLiteral("lastUpgradeAt"),
                  settings.value(QString::fromLatin1(kLastUpgradeAtKey)).toString());
    result.insert(QStringLiteral("legacyImportedKeyCount"),
                  settings.value(QString::fromLatin1(kLegacyImportCountKey), 0).toInt());
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
