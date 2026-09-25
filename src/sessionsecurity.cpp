#include "sessionsecurity.h"
#include "settingsstore.h"

#include <QCoreApplication>
#include <QDateTime>
#include <QEvent>
#include <QGuiApplication>

namespace {

const char kEnabledKey[] = "security/autoLockEnabled";
const char kBackgroundDelayKey[] = "security/backgroundDelaySeconds";
const char kInactivityKey[] = "security/inactivityMinutes";

} // namespace

SessionSecurity::SessionSecurity(QObject *parent)
    : QObject(parent)
{
    SailVaultSettings::migrateLegacySettings();

    m_enabled = SailVaultSettings::value(
        QString::fromLatin1(kEnabledKey), true).toBool();
    m_backgroundDelaySeconds = normalizedBackgroundDelay(
        SailVaultSettings::value(
            QString::fromLatin1(kBackgroundDelayKey), 0).toInt());
    m_inactivityMinutes = normalizedInactivityMinutes(
        SailVaultSettings::value(
            QString::fromLatin1(kInactivityKey), 5).toInt());

    m_inactivityTimer.setSingleShot(true);
    m_armTimer.setSingleShot(true);
    m_armTimer.setInterval(750);

    connect(&m_armTimer, &QTimer::timeout,
            this, [this]() {
        if (!m_enabled || !m_sessionActive || m_lockPending)
            return;

        QGuiApplication *app =
            qobject_cast<QGuiApplication *>(QCoreApplication::instance());

        if (app && app->applicationState() != Qt::ApplicationActive)
            return;

        m_sessionArmed = true;
        m_backgroundSinceMs = 0;
        restartInactivityTimer();
    });

    connect(&m_inactivityTimer, &QTimer::timeout,
            this, [this]() {
        if (m_enabled && m_sessionActive) {
            requestLock(QStringLiteral(
                "Wallet locked automatically after %1 of inactivity.")
                .arg(inactivityText()));
        }
    });

    QCoreApplication *core = QCoreApplication::instance();
    if (core)
        core->installEventFilter(this);

    QGuiApplication *app =
        qobject_cast<QGuiApplication *>(QCoreApplication::instance());

    if (app) {
        connect(app, &QGuiApplication::applicationStateChanged,
                this, &SessionSecurity::handleApplicationState);
    }
}

SessionSecurity::~SessionSecurity()
{
    QCoreApplication *core = QCoreApplication::instance();
    if (core)
        core->removeEventFilter(this);
}

bool SessionSecurity::enabled() const
{
    return m_enabled;
}

void SessionSecurity::setEnabled(bool enabled)
{
    if (m_enabled == enabled)
        return;

    m_enabled = enabled;

    SailVaultSettings::setValue(
        QString::fromLatin1(kEnabledKey), enabled);

    if (!m_enabled) {
        m_inactivityTimer.stop();
        m_armTimer.stop();
        m_backgroundSinceMs = 0;
        m_lockPending = false;
        m_sessionArmed = false;
    } else if (m_sessionActive) {
        m_sessionArmed = false;
        scheduleSessionArming();
    }

    emit settingsChanged();
}

bool SessionSecurity::sessionActive() const
{
    return m_sessionActive;
}

void SessionSecurity::setSessionActive(bool active)
{
    if (m_sessionActive == active)
        return;

    m_sessionActive = active;

    if (!m_sessionActive) {
        m_inactivityTimer.stop();
        m_armTimer.stop();
        m_backgroundSinceMs = 0;
        m_lockPending = false;
        m_sessionArmed = false;
    } else {
        // Sailfish Secrets may temporarily move the application through
        // inactive/background states while an unlock request is completing.
        // Do not interpret those transitions as the user backgrounding an
        // already-open wallet.  Arm auto-lock only after SailVault has been
        // stably active for a short moment.
        m_lockPending = false;
        m_backgroundSinceMs = 0;
        m_sessionArmed = false;
        scheduleSessionArming();
    }

    emit sessionActiveChanged();
}

int SessionSecurity::normalizedBackgroundDelay(int seconds)
{
    switch (seconds) {
    case 0:
    case 30:
    case 60:
    case 300:
        return seconds;
    default:
        return 0;
    }
}

int SessionSecurity::normalizedInactivityMinutes(int minutes)
{
    switch (minutes) {
    case 0:
    case 1:
    case 5:
    case 15:
        return minutes;
    default:
        return 5;
    }
}

int SessionSecurity::backgroundDelaySeconds() const
{
    return m_backgroundDelaySeconds;
}

void SessionSecurity::setBackgroundDelaySeconds(int seconds)
{
    const int normalized = normalizedBackgroundDelay(seconds);
    if (m_backgroundDelaySeconds == normalized)
        return;

    m_backgroundDelaySeconds = normalized;

    SailVaultSettings::setValue(
        QString::fromLatin1(kBackgroundDelayKey), normalized);

    emit settingsChanged();
}

int SessionSecurity::inactivityMinutes() const
{
    return m_inactivityMinutes;
}

void SessionSecurity::setInactivityMinutes(int minutes)
{
    const int normalized = normalizedInactivityMinutes(minutes);
    if (m_inactivityMinutes == normalized)
        return;

    m_inactivityMinutes = normalized;

    SailVaultSettings::setValue(
        QString::fromLatin1(kInactivityKey), normalized);

    restartInactivityTimer();
    emit settingsChanged();
}

QString SessionSecurity::backgroundDelayText() const
{
    switch (m_backgroundDelaySeconds) {
    case 30:
        return QStringLiteral("30 seconds");
    case 60:
        return QStringLiteral("1 minute");
    case 300:
        return QStringLiteral("5 minutes");
    default:
        return QStringLiteral("Immediately");
    }
}

QString SessionSecurity::inactivityText() const
{
    switch (m_inactivityMinutes) {
    case 0:
        return QStringLiteral("Never");
    case 1:
        return QStringLiteral("1 minute");
    case 15:
        return QStringLiteral("15 minutes");
    default:
        return QStringLiteral("5 minutes");
    }
}

void SessionSecurity::requestLock(const QString &reason)
{
    if (!m_enabled || !m_sessionActive || m_lockPending)
        return;

    m_lockPending = true;
    m_inactivityTimer.stop();
    m_backgroundSinceMs = 0;
    emit lockRequested(reason);
}

void SessionSecurity::restartInactivityTimer()
{
    m_inactivityTimer.stop();

    if (!m_enabled || !m_sessionActive || !m_sessionArmed
            || m_inactivityMinutes <= 0)
        return;

    QGuiApplication *app =
        qobject_cast<QGuiApplication *>(QCoreApplication::instance());

    if (app && app->applicationState() != Qt::ApplicationActive)
        return;

    m_inactivityTimer.start(m_inactivityMinutes * 60 * 1000);
}

void SessionSecurity::scheduleSessionArming()
{
    m_armTimer.stop();

    if (!m_enabled || !m_sessionActive || m_lockPending)
        return;

    QGuiApplication *app =
        qobject_cast<QGuiApplication *>(QCoreApplication::instance());

    if (app && app->applicationState() != Qt::ApplicationActive)
        return;

    m_armTimer.start();
}

void SessionSecurity::noteUserActivity()
{
    if (!m_enabled || !m_sessionActive || m_lockPending || !m_sessionArmed)
        return;

    restartInactivityTimer();
}

bool SessionSecurity::eventFilter(QObject *watched, QEvent *event)
{
    Q_UNUSED(watched);

    if (!event)
        return false;

    switch (event->type()) {
    case QEvent::MouseButtonPress:
    case QEvent::KeyPress:
    case QEvent::TouchBegin:
        noteUserActivity();
        break;
    default:
        break;
    }

    return false;
}

void SessionSecurity::handleApplicationState(Qt::ApplicationState state)
{
    if (!m_enabled || !m_sessionActive || m_lockPending) {
        if (state == Qt::ApplicationActive)
            m_backgroundSinceMs = 0;
        return;
    }

    // During the final part of a Sailfish Secrets unlock, the application can
    // still receive transient inactive/background states.  Until the session
    // has been stably foregrounded and armed, ignore those transitions.
    if (!m_sessionArmed) {
        if (state == Qt::ApplicationActive) {
            m_backgroundSinceMs = 0;
            scheduleSessionArming();
        } else {
            m_armTimer.stop();
            m_backgroundSinceMs = 0;
        }
        return;
    }

    const qint64 now = QDateTime::currentMSecsSinceEpoch();

    if (state == Qt::ApplicationActive) {
        if (m_backgroundSinceMs > 0) {
            const qint64 elapsedMs = now - m_backgroundSinceMs;
            const qint64 requiredMs =
                static_cast<qint64>(m_backgroundDelaySeconds) * 1000;

            if (m_backgroundDelaySeconds == 0
                    || elapsedMs >= requiredMs) {
                requestLock(QStringLiteral(
                    "Wallet locked automatically after SailVault left the foreground."));
                return;
            }
        }

        m_backgroundSinceMs = 0;
        restartInactivityTimer();
        return;
    }

    m_inactivityTimer.stop();

    if (m_backgroundSinceMs == 0)
        m_backgroundSinceMs = now;

    if (m_backgroundDelaySeconds == 0) {
        requestLock(QStringLiteral(
            "Wallet locked automatically when SailVault left the foreground."));
    }
}
