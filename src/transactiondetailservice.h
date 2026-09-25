#pragma once

#include <QObject>
#include <QString>
#include <QNetworkAccessManager>

class QNetworkReply;

class TransactionDetailService : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool loading READ loading NOTIFY stateChanged)
    Q_PROPERTY(bool loaded READ loaded NOTIFY stateChanged)
    Q_PROPERTY(QString status READ status NOTIFY stateChanged)
    Q_PROPERTY(QString chain READ chain NOTIFY stateChanged)
    Q_PROPERTY(QString identifier READ identifier NOTIFY stateChanged)
    Q_PROPERTY(QString direction READ direction NOTIFY stateChanged)
    Q_PROPERTY(QString amount READ amount NOTIFY stateChanged)
    Q_PROPERTY(QString fee READ fee NOTIFY stateChanged)
    Q_PROPERTY(QString transactionState READ transactionState NOTIFY stateChanged)
    Q_PROPERTY(QString blockText READ blockText NOTIFY stateChanged)
    Q_PROPERTY(QString timeText READ timeText NOTIFY stateChanged)
    Q_PROPERTY(QString fromAddress READ fromAddress NOTIFY stateChanged)
    Q_PROPERTY(QString toAddress READ toAddress NOTIFY stateChanged)
    Q_PROPERTY(QString method READ method NOTIFY stateChanged)
    Q_PROPERTY(QString extraText READ extraText NOTIFY stateChanged)

public:
    explicit TransactionDetailService(QObject *parent = nullptr);

    bool loading() const;
    bool loaded() const;
    QString status() const;
    QString chain() const;
    QString identifier() const;
    QString direction() const;
    QString amount() const;
    QString fee() const;
    QString transactionState() const;
    QString blockText() const;
    QString timeText() const;
    QString fromAddress() const;
    QString toAddress() const;
    QString method() const;
    QString extraText() const;

    Q_INVOKABLE void load(const QString &chain,
                          const QString &identifier,
                          const QString &walletAddress);

signals:
    void stateChanged();

private:
    void loadEthereum(const QString &hash,
                      const QString &walletAddress,
                      quint64 generation);
    void loadBitcoin(const QString &txid,
                     const QString &walletAddress,
                     quint64 generation);
    void loadSolana(const QString &signature,
                    const QString &walletAddress,
                    quint64 generation);

    void fail(const QString &message, quint64 generation);
    void succeed(quint64 generation);

    static QString setting(const char *key, const char *fallback);
    static QString normalizeBaseUrl(const QString &url);
    static QString networkErrorText(QNetworkReply *reply);
    static QString addressHash(const QJsonValue &value);
    static QString jsonScalarString(const QJsonValue &value);
    static QString formatBitcoin(qint64 satoshis);
    static QString formatSolana(qint64 lamports);
    static QString formatEthereumWei(const QString &weiText);
    static QString isoTimeToLocal(const QString &timestamp);
    static QString unixTimeToLocal(qint64 timestamp);

    void clearDetails();

    QNetworkAccessManager m_network;

    bool m_loading = false;
    bool m_loaded = false;
    QString m_status;
    QString m_chain;
    QString m_identifier;
    QString m_walletAddress;
    QString m_direction;
    QString m_amount;
    QString m_fee;
    QString m_transactionState;
    QString m_blockText;
    QString m_timeText;
    QString m_fromAddress;
    QString m_toAddress;
    QString m_method;
    QString m_extraText;

    quint64 m_generation = 0;
};
