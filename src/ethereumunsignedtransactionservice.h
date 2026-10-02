#pragma once

#include <QObject>
#include <QByteArray>
#include <QJsonArray>
#include <QNetworkAccessManager>
#include <QString>
#include <QVariantMap>

class QNetworkReply;

class EthereumUnsignedTransactionService : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool loading READ loading NOTIFY stateChanged)
    Q_PROPERTY(bool constructed READ constructed NOTIFY stateChanged)
    Q_PROPERTY(QString status READ status NOTIFY stateChanged)
    Q_PROPERTY(QString providerHost READ providerHost NOTIFY stateChanged)
    Q_PROPERTY(QString intentFingerprint READ intentFingerprint NOTIFY stateChanged)
    Q_PROPERTY(QString chainId READ chainId NOTIFY stateChanged)
    Q_PROPERTY(QString nonce READ nonce NOTIFY stateChanged)
    Q_PROPERTY(QString gasLimit READ gasLimit NOTIFY stateChanged)
    Q_PROPERTY(QString baseFeeGwei READ baseFeeGwei NOTIFY stateChanged)
    Q_PROPERTY(QString maxPriorityFeeGwei READ maxPriorityFeeGwei NOTIFY stateChanged)
    Q_PROPERTY(QString maxFeeGwei READ maxFeeGwei NOTIFY stateChanged)
    Q_PROPERTY(QString maximumNetworkFeeEth READ maximumNetworkFeeEth NOTIFY stateChanged)
    Q_PROPERTY(QString unsignedPayloadHex READ unsignedPayloadHex NOTIFY stateChanged)
    Q_PROPERTY(QString signingHashHex READ signingHashHex NOTIFY stateChanged)
    Q_PROPERTY(QString constructionFingerprint READ constructionFingerprint NOTIFY stateChanged)
    Q_PROPERTY(QString constructedAt READ constructedAt NOTIFY stateChanged)
    Q_PROPERTY(QString feeSource READ feeSource NOTIFY stateChanged)

public:
    explicit EthereumUnsignedTransactionService(QObject *parent = nullptr);

    bool loading() const;
    bool constructed() const;
    QString status() const;
    QString providerHost() const;
    QString intentFingerprint() const;
    QString chainId() const;
    QString nonce() const;
    QString gasLimit() const;
    QString baseFeeGwei() const;
    QString maxPriorityFeeGwei() const;
    QString maxFeeGwei() const;
    QString maximumNetworkFeeEth() const;
    QString unsignedPayloadHex() const;
    QString signingHashHex() const;
    QString constructionFingerprint() const;
    QString constructedAt() const;
    QString feeSource() const;

    Q_INVOKABLE void construct(const QVariantMap &intent,
                               const QString &ethereumRpcUrl);
    Q_INVOKABLE void reset();
    Q_INVOKABLE QVariantMap runSelfTest() const;

    // C++-only signing handoff. QML may trigger construction, but the
    // development signing boundary re-reads this immutable snapshot directly
    // from the constructed service instead of accepting QML-authored payloads.
    QVariantMap signingSnapshot() const;
    static bool validateSigningSnapshot(const QVariantMap &intent,
                                        const QVariantMap &snapshot,
                                        QString *error = nullptr);

signals:
    void stateChanged();

private:
    enum RpcKind {
        ChainIdentity,
        PendingNonce,
        LatestBlock,
        PriorityFee,
        PriorityFeeFallback,
        DestinationCode
    };

    static const int kRequestTimeoutMs = 15000;
    static const qint64 kMaxResponseBytes = 256LL * 1024LL;

    void clearState(bool preserveStatus = false);
    void fail(const QString &message, quint64 generation);
    void sendRpc(RpcKind kind,
                 const QString &method,
                 const QJsonArray &params,
                 int requestId,
                 quint64 generation,
                 int attempt = 0);
    void handleRpcResult(RpcKind kind,
                         const QJsonObject &object,
                         quint64 generation);
    void completeInput(RpcKind kind, quint64 generation);
    void finalizeConstruction(quint64 generation);

    static bool parseRpcQuantity(const QJsonValue &value,
                                 quint64 *number,
                                 QString *canonicalHex = nullptr);
    static bool amountToAtomicBytes(const QString &amount,
                                    int decimals,
                                    QByteArray *bytes,
                                    QString *atomicDecimal = nullptr);
    static QByteArray unsignedEip1559Payload(quint64 chainId,
                                             quint64 nonce,
                                             quint64 maxPriorityFeePerGas,
                                             quint64 maxFeePerGas,
                                             quint64 gasLimit,
                                             const QString &destinationAddress,
                                             const QByteArray &value,
                                             QString *error = nullptr);
    static QString keccak256Hex(const QByteArray &data);
    static QString formatGwei(quint64 wei);
    static QString formatWeiAsEth128(unsigned __int128 wei);
    static QString constructionFingerprintFor(const QString &intentFingerprint,
                                              const QByteArray &payload);

    QNetworkAccessManager m_network;
    quint64 m_generation = 0;
    int m_pendingInputs = 0;
    bool m_loading = false;
    bool m_constructed = false;

    QVariantMap m_intent;
    QString m_endpoint;
    QString m_providerHost;
    QString m_intentFingerprint;
    QString m_status;
    QString m_chainIdText;
    QString m_nonceText;
    QString m_gasLimitText;
    QString m_baseFeeGwei;
    QString m_maxPriorityFeeGwei;
    QString m_maxFeeGwei;
    QString m_maximumNetworkFeeEth;
    QString m_unsignedPayloadHex;
    QString m_signingHashHex;
    QString m_constructionFingerprint;
    QString m_constructedAt;
    QString m_feeSource;

    quint64 m_chainId = 0;
    quint64 m_nonce = 0;
    quint64 m_baseFeePerGas = 0;
    quint64 m_priorityFeePerGas = 0;
    quint64 m_fallbackGasPrice = 0;
    quint64 m_blockGasLimit = 0;
    quint64 m_gasLimit = 0;
    quint64 m_maxFeePerGas = 0;
    qint64 m_constructedEpochMs = 0;
    QByteArray m_valueBytes;
};
