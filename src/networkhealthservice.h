#pragma once

#include <QObject>
#include <QNetworkAccessManager>
#include <QString>
#include <QVariantList>
#include <QVariantMap>

class QNetworkReply;
class QUrl;

class NetworkHealthService : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool loading READ loading NOTIFY stateChanged)
    Q_PROPERTY(QString summary READ summary NOTIFY stateChanged)
    Q_PROPERTY(QString lastChecked READ lastChecked NOTIFY stateChanged)
    Q_PROPERTY(QVariantList providers READ providers NOTIFY stateChanged)

public:
    explicit NetworkHealthService(QObject *parent = nullptr);

    bool loading() const;
    QString summary() const;
    QString lastChecked() const;
    QVariantList providers() const;

    Q_INVOKABLE void checkAll(const QString &ethereumRpc,
                              const QString &ethereumExplorer,
                              const QString &bitcoinApi,
                              const QString &solanaRpc,
                              const QString &solanaTokenRpc,
                              const QString &fiatCurrency);

signals:
    void stateChanged();

private:
    enum ProviderIndex {
        EthereumRpc = 0,
        EthereumExplorer,
        BitcoinEsplora,
        SolanaRpc,
        SolanaTokenRpc,
        KrakenTicker,
        ProviderCount
    };

    void resetProvider(ProviderIndex index,
                       const QString &name,
                       const QString &purpose,
                       const QString &endpoint);
    void finishProvider(ProviderIndex index,
                        const QString &state,
                        const QString &detail,
                        qint64 elapsedMs,
                        quint64 generation);
    void finishInvalidUrl(ProviderIndex index,
                          const QString &detail,
                          quint64 generation);
    void updateSummary();

    void checkEthereumRpc(const QString &endpoint,
                          quint64 generation,
                          int attempt = 0);
    void checkEthereumExplorer(const QString &endpoint,
                               quint64 generation,
                               int attempt = 0);
    void checkBitcoin(const QString &endpoint,
                      quint64 generation,
                      int attempt = 0);
    void checkSolana(const QString &endpoint,
                     ProviderIndex index,
                     quint64 generation,
                     int attempt = 0);
    void checkKraken(const QString &currency,
                     quint64 generation,
                     int attempt = 0);

    static bool validHttpsUrl(const QString &text, QUrl *url = nullptr);
    static QString appendPath(const QString &base, const QString &path);
    static QString replyError(QNetworkReply *reply);
    static void armTimeout(QNetworkReply *reply);

    QNetworkAccessManager m_network;
    QVariantMap m_provider[ProviderCount];
    QString m_summary;
    QString m_lastChecked;
    bool m_loading = false;
    int m_completed = 0;
    quint64 m_generation = 0;
};
