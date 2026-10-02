#pragma once

#include <QObject>
#include <QByteArray>
#include <QString>
#include <QVariantMap>

struct TWPrivateKey;

class DevelopmentSigningService : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool signing READ signing NOTIFY stateChanged)
    Q_PROPERTY(bool signatureVerified READ signatureVerified NOTIFY stateChanged)
    Q_PROPERTY(QString status READ status NOTIFY stateChanged)
    Q_PROPERTY(QString signerAddress READ signerAddress NOTIFY stateChanged)
    Q_PROPERTY(QString signingHashHex READ signingHashHex NOTIFY stateChanged)
    Q_PROPERTY(QString constructionFingerprint READ constructionFingerprint NOTIFY stateChanged)
    Q_PROPERTY(QString signatureProofFingerprint READ signatureProofFingerprint NOTIFY stateChanged)
    Q_PROPERTY(int signatureSize READ signatureSize NOTIFY stateChanged)
    Q_PROPERTY(QString signedAt READ signedAt NOTIFY stateChanged)

public:
    explicit DevelopmentSigningService(QObject *parent = nullptr);

    bool signing() const;
    bool signatureVerified() const;
    QString status() const;
    QString signerAddress() const;
    QString signingHashHex() const;
    QString constructionFingerprint() const;
    QString signatureProofFingerprint() const;
    int signatureSize() const;
    QString signedAt() const;

    // QObject pointers keep the Qt 5.6/QML call boundary simple. They are
    // immediately qobject_cast to the concrete SailVault C++ types.
    Q_INVOKABLE void signEthereum(QObject *vaultObject,
                                  QObject *ethereumBuilderObject,
                                  const QVariantMap &intent);
    Q_INVOKABLE void reset();
    Q_INVOKABLE QVariantMap runSelfTest() const;

signals:
    void stateChanged();

private:
    static const qint64 kMaximumConstructionAgeMs = 120000;

    void clearState(bool preserveStatus = false);
    void fail(const QString &message);

    static bool parseHash32(const QString &hex, QByteArray *bytes);
    static bool signDigestWithDevelopmentKey(TWPrivateKey *key,
                                             const QByteArray &digest,
                                                const QString &expectedSigner,
                                                QByteArray *signature,
                                                int *recoveryId,
                                                QString *error);
    static QString signatureProofFor(const QString &intentFingerprint,
                                     const QString &constructionFingerprint,
                                     const QString &signingHashHex,
                                     const QString &signerAddress,
                                     const QByteArray &signature);
    static void secureErase(QByteArray *bytes);

    bool m_signing = false;
    bool m_signatureVerified = false;
    QString m_status;
    QString m_signerAddress;
    QString m_signingHashHex;
    QString m_constructionFingerprint;
    QString m_signatureProofFingerprint;
    int m_signatureSize = 0;
    QString m_signedAt;
};
