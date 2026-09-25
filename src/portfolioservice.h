#pragma once

#include <QObject>
#include <QString>
#include <QNetworkAccessManager>

class QNetworkReply;

class PortfolioService : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool loading READ loading NOTIFY stateChanged)
    Q_PROPERTY(bool lastRefreshPassed READ lastRefreshPassed NOTIFY stateChanged)
    Q_PROPERTY(QString status READ status NOTIFY stateChanged)
    Q_PROPERTY(QString ethereumBalance READ ethereumBalance NOTIFY stateChanged)
    Q_PROPERTY(QString bitcoinBalance READ bitcoinBalance NOTIFY stateChanged)
    Q_PROPERTY(QString solanaBalance READ solanaBalance NOTIFY stateChanged)
    Q_PROPERTY(QString ethereumStatus READ ethereumStatus NOTIFY stateChanged)
    Q_PROPERTY(QString bitcoinStatus READ bitcoinStatus NOTIFY stateChanged)
    Q_PROPERTY(QString solanaStatus READ solanaStatus NOTIFY stateChanged)
    Q_PROPERTY(QString ethereumUpdated READ ethereumUpdated NOTIFY stateChanged)
    Q_PROPERTY(QString bitcoinUpdated READ bitcoinUpdated NOTIFY stateChanged)
    Q_PROPERTY(QString solanaUpdated READ solanaUpdated NOTIFY stateChanged)
    Q_PROPERTY(QString lastUpdated READ lastUpdated NOTIFY stateChanged)
    Q_PROPERTY(bool usingCachedData READ usingCachedData NOTIFY stateChanged)
    Q_PROPERTY(QString ethereumRpcUrl READ ethereumRpcUrl WRITE setEthereumRpcUrl NOTIFY settingsChanged)
    Q_PROPERTY(QString ethereumExplorerUrl READ ethereumExplorerUrl WRITE setEthereumExplorerUrl NOTIFY settingsChanged)
    Q_PROPERTY(QString bitcoinApiUrl READ bitcoinApiUrl WRITE setBitcoinApiUrl NOTIFY settingsChanged)
    Q_PROPERTY(QString solanaRpcUrl READ solanaRpcUrl WRITE setSolanaRpcUrl NOTIFY settingsChanged)
    Q_PROPERTY(QString solanaTokenRpcUrl READ solanaTokenRpcUrl WRITE setSolanaTokenRpcUrl NOTIFY settingsChanged)
    Q_PROPERTY(bool autoRefreshAfterUnlock READ autoRefreshAfterUnlock WRITE setAutoRefreshAfterUnlock NOTIFY settingsChanged)
    Q_PROPERTY(bool offlineMode READ offlineMode WRITE setOfflineMode NOTIFY settingsChanged)

public:
    explicit PortfolioService(QObject *parent = nullptr);

    bool loading() const;
    bool lastRefreshPassed() const;
    QString status() const;
    QString ethereumBalance() const;
    QString bitcoinBalance() const;
    QString solanaBalance() const;
    QString ethereumStatus() const;
    QString bitcoinStatus() const;
    QString solanaStatus() const;
    QString ethereumUpdated() const;
    QString bitcoinUpdated() const;
    QString solanaUpdated() const;
    QString lastUpdated() const;
    bool usingCachedData() const;

    QString ethereumRpcUrl() const;
    QString ethereumExplorerUrl() const;
    QString bitcoinApiUrl() const;
    QString solanaRpcUrl() const;
    QString solanaTokenRpcUrl() const;
    void setEthereumRpcUrl(const QString &url);
    void setEthereumExplorerUrl(const QString &url);
    void setBitcoinApiUrl(const QString &url);
    void setSolanaRpcUrl(const QString &url);
    void setSolanaTokenRpcUrl(const QString &url);
    bool autoRefreshAfterUnlock() const;
    void setAutoRefreshAfterUnlock(bool enabled);
    bool offlineMode() const;
    void setOfflineMode(bool enabled);

    Q_INVOKABLE void loadSnapshot(const QString &ethereumAddress,
                                  const QString &bitcoinAddress,
                                  const QString &solanaAddress);
    Q_INVOKABLE QString cachedBalance(const QString &chain,
                                      const QString &address) const;
    Q_INVOKABLE QString cachedBalanceUpdated(const QString &chain,
                                             const QString &address) const;
    Q_INVOKABLE void refresh(const QString &ethereumAddress,
                             const QString &bitcoinAddress,
                             const QString &solanaAddress);
    Q_INVOKABLE void resetEndpoints();

signals:
    void stateChanged();
    void settingsChanged();

private:
    void refreshEthereum(const QString &address, quint64 generation, int attempt = 0);
    void refreshBitcoin(const QString &address, quint64 generation, int attempt = 0);
    void refreshSolana(const QString &address, quint64 generation, int attempt = 0);
    void finishRequest(quint64 generation);
    void failRequest(const QString &chain,
                     const QString &message,
                     quint64 generation);
    static QString snapshotId(const QString &ethereumAddress,
                              const QString &bitcoinAddress,
                              const QString &solanaAddress);
    void saveSnapshot();
    void saveAddressCache(const QString &chain,
                          const QString &address,
                          const QString &balance,
                          const QString &updated);
    QString addressCacheBase(const QString &chain,
                             const QString &address) const;

    static QString normalizeBaseUrl(const QString &url);
    static QString formatBitcoin(qint64 satoshis);
    static QString formatEthereumWeiHex(const QString &hexWei);
    static QString formatSolana(qint64 lamports);

    QNetworkAccessManager m_network;
    bool m_loading = false;
    bool m_lastRefreshPassed = false;
    bool m_usingCachedData = false;
    bool m_ethRequested = false;
    bool m_btcRequested = false;
    bool m_solRequested = false;
    bool m_ethDone = false;
    bool m_btcDone = false;
    bool m_solDone = false;
    bool m_ethOk = false;
    bool m_btcOk = false;
    bool m_solOk = false;
    QString m_status;
    QString m_ethereumBalance = QStringLiteral("—");
    QString m_bitcoinBalance = QStringLiteral("—");
    QString m_solanaBalance = QStringLiteral("—");
    QString m_ethereumStatus = QStringLiteral("Not queried");
    QString m_bitcoinStatus = QStringLiteral("Not queried");
    QString m_solanaStatus = QStringLiteral("Not queried");
    QString m_ethereumUpdated;
    QString m_bitcoinUpdated;
    QString m_solanaUpdated;
    QString m_lastUpdated;
    QString m_snapshotId;
    QString m_ethAddress;
    QString m_btcAddress;
    QString m_solAddress;
    QString m_ethError;
    QString m_btcError;
    QString m_solError;
    quint64 m_generation = 0;
};
