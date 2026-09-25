#pragma once

#include <QObject>
#include <QString>
#include <QVariantList>
#include <QNetworkAccessManager>
#include <QJsonObject>
#include <QJsonArray>
#include <QSet>
#include <QStringList>

class ActivityService : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool loading READ loading NOTIFY stateChanged)
    Q_PROPERTY(bool loadingOlder READ loadingOlder NOTIFY stateChanged)
    Q_PROPERTY(bool canLoadOlder READ canLoadOlder NOTIFY stateChanged)
    Q_PROPERTY(bool ethereumHasMore READ ethereumHasMore NOTIFY stateChanged)
    Q_PROPERTY(bool bitcoinHasMore READ bitcoinHasMore NOTIFY stateChanged)
    Q_PROPERTY(bool solanaHasMore READ solanaHasMore NOTIFY stateChanged)
    Q_PROPERTY(bool lastRefreshPassed READ lastRefreshPassed NOTIFY stateChanged)
    Q_PROPERTY(QString status READ status NOTIFY stateChanged)
    Q_PROPERTY(QString ethereumStatus READ ethereumStatus NOTIFY stateChanged)
    Q_PROPERTY(QString bitcoinStatus READ bitcoinStatus NOTIFY stateChanged)
    Q_PROPERTY(QString solanaStatus READ solanaStatus NOTIFY stateChanged)
    Q_PROPERTY(QVariantList entries READ entries NOTIFY stateChanged)
    Q_PROPERTY(int entryCount READ entryCount NOTIFY stateChanged)
    Q_PROPERTY(QString lastUpdated READ lastUpdated NOTIFY stateChanged)

public:
    explicit ActivityService(QObject *parent = nullptr);

    bool loading() const;
    bool loadingOlder() const;
    bool canLoadOlder() const;
    bool ethereumHasMore() const;
    bool bitcoinHasMore() const;
    bool solanaHasMore() const;
    bool lastRefreshPassed() const;
    QString status() const;
    QString ethereumStatus() const;
    QString bitcoinStatus() const;
    QString solanaStatus() const;
    QVariantList entries() const;
    int entryCount() const;
    QString lastUpdated() const;

    Q_INVOKABLE void refresh(const QString &ethereumAddress,
                             const QString &bitcoinAddress,
                             const QString &solanaAddress);
    Q_INVOKABLE void loadOlder(const QString &chain);

signals:
    void stateChanged();

private:
    void refreshEthereum(const QString &address, quint64 generation);
    void refreshBitcoin(const QString &address, quint64 generation);
    void refreshSolana(const QString &address, quint64 generation);

    void loadOlderEthereum(quint64 generation);
    void loadOlderBitcoin(quint64 generation);
    void loadOlderSolana(quint64 generation);

    int appendEthereumItems(const QJsonArray &items, const QString &address);
    int appendBitcoinItems(const QJsonArray &items, const QString &address);
    int appendSolanaItems(const QJsonArray &items);

    void finishIfReady(quint64 generation);
    void finishOlderRequest(const QString &chain,
                            bool success,
                            const QString &error,
                            int added,
                            quint64 generation);
    void setChainFailure(const QString &chain,
                         const QString &message,
                         quint64 generation);
    bool addEntry(const QString &chain,
                  const QString &summary,
                  const QString &identifier,
                  const QString &state,
                  qint64 timestamp);
    void sortEntries();
    void updateChainStatus(const QString &chain);

    static QString setting(const char *key, const char *fallback);
    static QString normalizeBaseUrl(const QString &url);
    static QString networkErrorText(QNetworkReply *reply);
    static QString formatBitcoin(qint64 satoshis);
    static QString formatEthereumDecimalWei(const QString &wei);
    static QString displayTime(qint64 timestamp);
    static QString jsonQueryValue(const QJsonValue &value);

    QNetworkAccessManager m_network;
    bool m_loading = false;
    bool m_loadingOlder = false;
    bool m_lastRefreshPassed = false;
    bool m_ethRequested = false;
    bool m_btcRequested = false;
    bool m_solRequested = false;
    bool m_ethDone = false;
    bool m_btcDone = false;
    bool m_solDone = false;
    bool m_ethOk = false;
    bool m_btcOk = false;
    bool m_solOk = false;
    bool m_ethHasMore = false;
    bool m_btcHasMore = false;
    bool m_solHasMore = false;
    QString m_status;
    QString m_ethereumStatus;
    QString m_bitcoinStatus;
    QString m_solanaStatus;
    QVariantList m_entries;
    QSet<QString> m_seenEntries;
    QString m_lastUpdated;
    QString m_ethAddress;
    QString m_btcAddress;
    QString m_solAddress;
    QJsonObject m_ethNextPageParams;
    QString m_btcCursor;
    QString m_solCursor;
    int m_ethLoaded = 0;
    int m_btcLoaded = 0;
    int m_solLoaded = 0;
    int m_olderOutstanding = 0;
    int m_olderRequested = 0;
    int m_olderSucceeded = 0;
    int m_olderAdded = 0;
    QStringList m_olderErrors;
    quint64 m_generation = 0;
};
