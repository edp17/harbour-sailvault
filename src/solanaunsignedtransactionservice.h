#pragma once

#include <QObject>
#include <QByteArray>
#include <QJsonArray>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QString>
#include <QVariantMap>

class QNetworkReply;

class SolanaUnsignedTransactionService : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool loading READ loading NOTIFY stateChanged)
    Q_PROPERTY(bool constructed READ constructed NOTIFY stateChanged)
    Q_PROPERTY(QString status READ status NOTIFY stateChanged)
    Q_PROPERTY(QString providerHost READ providerHost NOTIFY stateChanged)
    Q_PROPERTY(QString intentFingerprint READ intentFingerprint NOTIFY stateChanged)
    Q_PROPERTY(QString genesisHash READ genesisHash NOTIFY stateChanged)
    Q_PROPERTY(QString recentBlockhash READ recentBlockhash NOTIFY stateChanged)
    Q_PROPERTY(QString lastValidBlockHeight READ lastValidBlockHeight NOTIFY stateChanged)
    Q_PROPERTY(QString lamports READ lamports NOTIFY stateChanged)
    Q_PROPERTY(QString networkFeeLamports READ networkFeeLamports NOTIFY stateChanged)
    Q_PROPERTY(QString networkFeeSol READ networkFeeSol NOTIFY stateChanged)
    Q_PROPERTY(QString signerAddress READ signerAddress NOTIFY stateChanged)
    Q_PROPERTY(QString instructionSummary READ instructionSummary NOTIFY stateChanged)
    Q_PROPERTY(QString unsignedMessageHex READ unsignedMessageHex NOTIFY stateChanged)
    Q_PROPERTY(QString unsignedMessageBase64 READ unsignedMessageBase64 NOTIFY stateChanged)
    Q_PROPERTY(QString unsignedTransactionTemplateBase64 READ unsignedTransactionTemplateBase64 NOTIFY stateChanged)
    Q_PROPERTY(QString constructionFingerprint READ constructionFingerprint NOTIFY stateChanged)
    Q_PROPERTY(QString constructedAt READ constructedAt NOTIFY stateChanged)

public:
    explicit SolanaUnsignedTransactionService(QObject *parent = nullptr);

    bool loading() const { return m_loading; }
    bool constructed() const { return m_constructed; }
    QString status() const { return m_status; }
    QString providerHost() const { return m_providerHost; }
    QString intentFingerprint() const { return m_intentFingerprint; }
    QString genesisHash() const { return m_genesisHash; }
    QString recentBlockhash() const { return m_recentBlockhash; }
    QString lastValidBlockHeight() const { return m_lastValidBlockHeightText; }
    QString lamports() const { return m_lamportsText; }
    QString networkFeeLamports() const { return m_networkFeeLamportsText; }
    QString networkFeeSol() const { return m_networkFeeSol; }
    QString signerAddress() const { return m_signerAddress; }
    QString instructionSummary() const { return m_instructionSummary; }
    QString unsignedMessageHex() const { return m_unsignedMessageHex; }
    QString unsignedMessageBase64() const { return m_unsignedMessageBase64; }
    QString unsignedTransactionTemplateBase64() const { return m_unsignedTransactionTemplateBase64; }
    QString constructionFingerprint() const { return m_constructionFingerprint; }
    QString constructedAt() const { return m_constructedAt; }

    Q_INVOKABLE void construct(const QVariantMap &intent,
                               const QString &solanaRpcUrl);
    Q_INVOKABLE void reset();
    Q_INVOKABLE QVariantMap runSelfTest() const;

signals:
    void stateChanged();

private:
    enum RpcKind {
        GenesisHash,
        LatestBlockhash,
        FeeForMessage
    };

    struct PreSigningResult {
        bool ok = false;
        QString error;
        QByteArray signer;
        QByteArray message;
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
    void maybeBuildMessage(quint64 generation);
    void finalizeConstruction(quint64 generation, quint64 feeLamports);

    static bool amountToLamports(const QString &amount, quint64 *lamports);
    static bool decodeBase58_32(const QString &value, QByteArray *decoded = nullptr);
    static QByteArray encodeSolanaSigningInput(const QString &sender,
                                               const QString &recipient,
                                               quint64 lamports,
                                               const QString &recentBlockhash);
    static PreSigningResult walletCorePreSigningMessage(const QString &sender,
                                                        const QString &recipient,
                                                        quint64 lamports,
                                                        const QString &recentBlockhash);
    static QByteArray unsignedTransactionTemplate(const QByteArray &message);
    static QString formatLamportsAsSol(quint64 lamports);
    static QString constructionFingerprintFor(const QString &intentFingerprint,
                                              const QString &genesisHash,
                                              const QString &recentBlockhash,
                                              quint64 lastValidBlockHeight,
                                              quint64 feeLamports,
                                              const QByteArray &message);

    QNetworkAccessManager m_network;
    quint64 m_generation = 0;
    bool m_loading = false;
    bool m_constructed = false;
    bool m_haveGenesis = false;
    bool m_haveBlockhash = false;
    bool m_feeRequested = false;

    QVariantMap m_intent;
    QString m_endpoint;
    QString m_providerHost;
    QString m_intentFingerprint;
    QString m_status;
    QString m_genesisHash;
    QString m_recentBlockhash;
    QString m_lastValidBlockHeightText;
    QString m_lamportsText;
    QString m_networkFeeLamportsText;
    QString m_networkFeeSol;
    QString m_signerAddress;
    QString m_instructionSummary;
    QString m_unsignedMessageHex;
    QString m_unsignedMessageBase64;
    QString m_unsignedTransactionTemplateBase64;
    QString m_constructionFingerprint;
    QString m_constructedAt;

    quint64 m_lamports = 0;
    quint64 m_lastValidBlockHeight = 0;
    QByteArray m_unsignedMessage;
};
