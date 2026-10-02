#pragma once

#include <QObject>
#include <QByteArray>
#include <QNetworkAccessManager>
#include <QString>
#include <QVariantMap>
#include <QVector>

class QNetworkReply;
class QUrl;

class BitcoinUnsignedTransactionService : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool loading READ loading NOTIFY stateChanged)
    Q_PROPERTY(bool constructed READ constructed NOTIFY stateChanged)
    Q_PROPERTY(QString status READ status NOTIFY stateChanged)
    Q_PROPERTY(QString providerHost READ providerHost NOTIFY stateChanged)
    Q_PROPERTY(QString intentFingerprint READ intentFingerprint NOTIFY stateChanged)
    Q_PROPERTY(QString feeTarget READ feeTarget NOTIFY stateChanged)
    Q_PROPERTY(QString providerFeeEstimate READ providerFeeEstimate NOTIFY stateChanged)
    Q_PROPERTY(QString feeRateSatVb READ feeRateSatVb NOTIFY stateChanged)
    Q_PROPERTY(int confirmedUtxoCount READ confirmedUtxoCount NOTIFY stateChanged)
    Q_PROPERTY(int selectedInputCount READ selectedInputCount NOTIFY stateChanged)
    Q_PROPERTY(QString selectedInputsSummary READ selectedInputsSummary NOTIFY stateChanged)
    Q_PROPERTY(QString totalInputBtc READ totalInputBtc NOTIFY stateChanged)
    Q_PROPERTY(QString amountBtc READ amountBtc NOTIFY stateChanged)
    Q_PROPERTY(QString networkFeeBtc READ networkFeeBtc NOTIFY stateChanged)
    Q_PROPERTY(QString changeBtc READ changeBtc NOTIFY stateChanged)
    Q_PROPERTY(QString estimatedVbytes READ estimatedVbytes NOTIFY stateChanged)
    Q_PROPERTY(QString changePolicy READ changePolicy NOTIFY stateChanged)
    Q_PROPERTY(QString unsignedTransactionHex READ unsignedTransactionHex NOTIFY stateChanged)
    Q_PROPERTY(QString unsignedTxid READ unsignedTxid NOTIFY stateChanged)
    Q_PROPERTY(QString psbtBase64 READ psbtBase64 NOTIFY stateChanged)
    Q_PROPERTY(QString constructionFingerprint READ constructionFingerprint NOTIFY stateChanged)
    Q_PROPERTY(QString constructedAt READ constructedAt NOTIFY stateChanged)

public:
    explicit BitcoinUnsignedTransactionService(QObject *parent = nullptr);

    bool loading() const { return m_loading; }
    bool constructed() const { return m_constructed; }
    QString status() const { return m_status; }
    QString providerHost() const { return m_providerHost; }
    QString intentFingerprint() const { return m_intentFingerprint; }
    QString feeTarget() const { return m_feeTarget; }
    QString providerFeeEstimate() const { return m_providerFeeEstimate; }
    QString feeRateSatVb() const { return m_feeRateSatVb; }
    int confirmedUtxoCount() const { return m_confirmedUtxoCount; }
    int selectedInputCount() const { return m_selectedInputCount; }
    QString selectedInputsSummary() const { return m_selectedInputsSummary; }
    QString totalInputBtc() const { return m_totalInputBtc; }
    QString amountBtc() const { return m_amountBtc; }
    QString networkFeeBtc() const { return m_networkFeeBtc; }
    QString changeBtc() const { return m_changeBtc; }
    QString estimatedVbytes() const { return m_estimatedVbytes; }
    QString changePolicy() const { return m_changePolicy; }
    QString unsignedTransactionHex() const { return m_unsignedTransactionHex; }
    QString unsignedTxid() const { return m_unsignedTxid; }
    QString psbtBase64() const { return m_psbtBase64; }
    QString constructionFingerprint() const { return m_constructionFingerprint; }
    QString constructedAt() const { return m_constructedAt; }

    Q_INVOKABLE void construct(const QVariantMap &intent,
                               const QString &bitcoinApiUrl);
    Q_INVOKABLE void reset();
    Q_INVOKABLE QVariantMap runSelfTest() const;

signals:
    void stateChanged();

private:
    struct Utxo {
        QString txid;
        quint32 vout = 0;
        quint64 value = 0;
    };

    struct BuildResult {
        bool ok = false;
        QString error;
        QVector<Utxo> selected;
        quint64 totalInput = 0;
        quint64 amount = 0;
        quint64 fee = 0;
        quint64 change = 0;
        quint64 estimatedVbytes = 0;
        bool changeOutput = false;
        QByteArray unsignedTx;
        QByteArray psbt;
        QString txid;
    };

    enum RequestKind { GenesisRequest, UtxosRequest, FeeEstimatesRequest };
    static const int kRequestTimeoutMs = 15000;
    static const qint64 kMaxResponseBytes = 512LL * 1024LL;

    void clearState(bool preserveStatus = false);
    void fail(const QString &message, quint64 generation);
    void sendGet(RequestKind kind, const QUrl &url, quint64 generation, int attempt = 0);
    void handleJson(RequestKind kind, const QByteArray &data, quint64 generation);
    void maybeFinalize(quint64 generation);

    static bool amountToSats(const QString &amount, quint64 *sats);
    static bool lockScriptForAddress(const QString &address,
                                     QByteArray *script,
                                     bool *isP2wpkh = nullptr);
    static BuildResult buildTransaction(const QVector<Utxo> &utxos,
                                        quint64 amount,
                                        quint64 feeRateSatVb,
                                        const QByteArray &sourceScript,
                                        const QByteArray &destinationScript);
    static QByteArray compactSize(quint64 value);
    static int compactSizeLength(quint64 value);
    static QString formatBtc(quint64 sats);
    static QString txidFor(const QByteArray &unsignedTx);
    static QString constructionFingerprintFor(const QString &intentFingerprint,
                                              const QByteArray &unsignedTx,
                                              const QByteArray &psbt);

    QNetworkAccessManager m_network;
    quint64 m_generation = 0;
    int m_pending = 0;
    bool m_loading = false;
    bool m_constructed = false;
    bool m_haveGenesis = false;
    bool m_haveUtxos = false;
    bool m_haveFee = false;

    QVariantMap m_intent;
    QString m_baseUrl;
    QString m_providerHost;
    QString m_intentFingerprint;
    QString m_status;
    QString m_feeTarget;
    QString m_providerFeeEstimate;
    QString m_feeRateSatVb;
    int m_confirmedUtxoCount = 0;
    int m_selectedInputCount = 0;
    QString m_selectedInputsSummary;
    QString m_totalInputBtc;
    QString m_amountBtc;
    QString m_networkFeeBtc;
    QString m_changeBtc;
    QString m_estimatedVbytes;
    QString m_changePolicy;
    QString m_unsignedTransactionHex;
    QString m_unsignedTxid;
    QString m_psbtBase64;
    QString m_constructionFingerprint;
    QString m_constructedAt;

    QVector<Utxo> m_utxos;
    quint64 m_amountSats = 0;
    quint64 m_feeRate = 0;
    QByteArray m_sourceScript;
    QByteArray m_destinationScript;
};
