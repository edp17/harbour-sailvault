#pragma once

#include <QObject>
#include <QString>
#include <QStringList>
#include <QVariantList>
#include <QNetworkAccessManager>

class QNetworkReply;

class TokenService : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool loading READ loading NOTIFY stateChanged)
    Q_PROPERTY(bool lastRefreshPassed READ lastRefreshPassed NOTIFY stateChanged)
    Q_PROPERTY(QString status READ status NOTIFY stateChanged)
    Q_PROPERTY(QString ethereumStatus READ ethereumStatus NOTIFY stateChanged)
    Q_PROPERTY(QString solanaStatus READ solanaStatus NOTIFY stateChanged)
    Q_PROPERTY(QVariantList entries READ entries NOTIFY stateChanged)
    Q_PROPERTY(int tokenCount READ tokenCount NOTIFY stateChanged)
    Q_PROPERTY(int ethereumTokenCount READ ethereumTokenCount NOTIFY stateChanged)
    Q_PROPERTY(int solanaTokenCount READ solanaTokenCount NOTIFY stateChanged)
    Q_PROPERTY(bool showHidden READ showHidden WRITE setShowHidden NOTIFY stateChanged)
    Q_PROPERTY(int hiddenTokenCount READ hiddenTokenCount NOTIFY stateChanged)
    Q_PROPERTY(QString lastUpdated READ lastUpdated NOTIFY stateChanged)

public:
    explicit TokenService(QObject *parent = nullptr);

    bool loading() const;
    bool lastRefreshPassed() const;
    QString status() const;
    QString ethereumStatus() const;
    QString solanaStatus() const;
    QVariantList entries() const;
    int tokenCount() const;
    int ethereumTokenCount() const;
    int solanaTokenCount() const;
    bool showHidden() const;
    void setShowHidden(bool show);
    int hiddenTokenCount() const;
    QString lastUpdated() const;

    Q_INVOKABLE void refresh(const QString &ethereumAddress,
                             const QString &solanaAddress);
    Q_INVOKABLE bool tokenHidden(const QString &chain,
                                 const QString &identifier) const;
    Q_INVOKABLE void setTokenHidden(const QString &chain,
                                    const QString &identifier,
                                    bool hidden);

signals:
    void stateChanged();

private:
    void refreshEthereum(const QString &address, quint64 generation);
    void refreshSolanaProgram(const QString &address,
                              const QString &programId,
                              const QString &standard,
                              quint64 generation);
    void finishSolanaRequest(bool passed,
                             const QString &message,
                             quint64 generation);
    void finishIfReady(quint64 generation);
    void rebuildVisibleEntries();
    static QString tokenStorageKey(const QString &chain,
                                   const QString &identifier);

    void addEntry(const QString &chain,
                  const QString &name,
                  const QString &symbol,
                  const QString &amount,
                  const QString &identifier,
                  const QString &standard,
                  const QString &extra);

    static QString setting(const char *key, const char *fallback);
    static QString normalizeBaseUrl(const QString &url);
    static QString networkErrorText(QNetworkReply *reply);
    static bool isZeroInteger(const QString &value);
    static QString formatIntegerAmount(const QString &rawAmount, int decimals);

    QNetworkAccessManager m_network;
    bool m_loading = false;
    bool m_lastRefreshPassed = false;
    bool m_ethRequested = false;
    bool m_solRequested = false;
    bool m_ethDone = false;
    bool m_solDone = false;
    bool m_ethOk = false;
    bool m_solOk = false;
    int m_solRequestsPending = 0;
    int m_solRequestsSucceeded = 0;
    int m_ethereumTokenCount = 0;
    int m_solanaTokenCount = 0;
    QStringList m_solErrors;
    QString m_status;
    QString m_ethereumStatus;
    QString m_solanaStatus;
    QVariantList m_allEntries;
    QVariantList m_entries;
    bool m_showHidden = false;
    int m_hiddenTokenCount = 0;
    QString m_lastUpdated;
    quint64 m_generation = 0;
};
