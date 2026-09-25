#pragma once

#include <QObject>
#include <QNetworkAccessManager>
#include <QString>
#include <QVariantList>
#include <QVariantMap>

class QNetworkReply;
class QUrl;

class NetworkFeeService : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool loading READ loading NOTIFY stateChanged)
    Q_PROPERTY(QString summary READ summary NOTIFY stateChanged)
    Q_PROPERTY(QString lastUpdated READ lastUpdated NOTIFY stateChanged)
    Q_PROPERTY(QVariantList estimates READ estimates NOTIFY stateChanged)

public:
    explicit NetworkFeeService(QObject *parent = nullptr);

    bool loading() const;
    QString summary() const;
    QString lastUpdated() const;
    QVariantList estimates() const;

    Q_INVOKABLE void refresh(const QString &ethereumRpc,
                             const QString &bitcoinApi,
                             const QString &solanaRpc);

signals:
    void stateChanged();

private:
    enum ChainIndex {
        Ethereum = 0,
        Bitcoin,
        Solana,
        ChainCount
    };

    void resetEstimate(ChainIndex index,
                       const QString &name,
                       const QString &endpoint);
    void finishEstimate(ChainIndex index,
                        const QString &state,
                        const QString &headline,
                        const QString &detail,
                        qint64 elapsedMs,
                        quint64 generation);
    void finishInvalidUrl(ChainIndex index,
                          quint64 generation);
    void updateSummary();

    void refreshEthereum(const QString &endpoint,
                         quint64 generation,
                         int attempt = 0);
    void refreshBitcoin(const QString &endpoint,
                        quint64 generation,
                        int attempt = 0);
    void refreshSolana(const QString &endpoint,
                       quint64 generation,
                       int attempt = 0);

    static bool validHttpsUrl(const QString &text, QUrl *url = nullptr);
    static QString appendPath(const QString &base, const QString &path);
    static QString replyError(QNetworkReply *reply);
    static void armTimeout(QNetworkReply *reply);
    static QString compactNumber(double value, int maximumDecimals);

    QNetworkAccessManager m_network;
    QVariantMap m_estimate[ChainCount];
    QString m_summary;
    QString m_lastUpdated;
    bool m_loading = false;
    int m_completed = 0;
    quint64 m_generation = 0;
};
