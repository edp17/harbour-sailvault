#pragma once

#include <QObject>
#include <QTimer>
#include <Qt>

class QEvent;

class SessionSecurity : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool enabled READ enabled WRITE setEnabled NOTIFY settingsChanged)
    Q_PROPERTY(bool sessionActive READ sessionActive WRITE setSessionActive NOTIFY sessionActiveChanged)
    Q_PROPERTY(int backgroundDelaySeconds READ backgroundDelaySeconds WRITE setBackgroundDelaySeconds NOTIFY settingsChanged)
    Q_PROPERTY(int inactivityMinutes READ inactivityMinutes WRITE setInactivityMinutes NOTIFY settingsChanged)
    Q_PROPERTY(QString backgroundDelayText READ backgroundDelayText NOTIFY settingsChanged)
    Q_PROPERTY(QString inactivityText READ inactivityText NOTIFY settingsChanged)

public:
    explicit SessionSecurity(QObject *parent = nullptr);
    ~SessionSecurity() override;

    bool enabled() const;
    void setEnabled(bool enabled);

    bool sessionActive() const;
    void setSessionActive(bool active);

    int backgroundDelaySeconds() const;
    void setBackgroundDelaySeconds(int seconds);

    int inactivityMinutes() const;
    void setInactivityMinutes(int minutes);

    QString backgroundDelayText() const;
    QString inactivityText() const;

signals:
    void settingsChanged();
    void sessionActiveChanged();
    void lockRequested(const QString &reason);

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;

private:
    void handleApplicationState(Qt::ApplicationState state);
    void noteUserActivity();
    void restartInactivityTimer();
    void scheduleSessionArming();
    void requestLock(const QString &reason);
    static int normalizedBackgroundDelay(int seconds);
    static int normalizedInactivityMinutes(int minutes);

    bool m_enabled = true;
    bool m_sessionActive = false;
    bool m_lockPending = false;
    bool m_sessionArmed = false;
    int m_backgroundDelaySeconds = 0;
    int m_inactivityMinutes = 5;
    qint64 m_backgroundSinceMs = 0;
    QTimer m_inactivityTimer;
    QTimer m_armTimer;
};
