#include "developmentsigningservice.h"

#include "ethereumunsignedtransactionservice.h"
#include "unsignedtransactionservice.h"
#include "walletvault.h"

#include <QCryptographicHash>
#include <QDateTime>
#include <QRegularExpression>

#include <TrustWalletCore/TWAnyAddress.h>
#include <TrustWalletCore/TWCoinType.h>
#include <TrustWalletCore/TWCurve.h>
#include <TrustWalletCore/TWData.h>
#include <TrustWalletCore/TWHDWallet.h>
#include <TrustWalletCore/TWPrivateKey.h>
#include <TrustWalletCore/TWPublicKey.h>
#include <TrustWalletCore/TWString.h>

namespace {

const char kPublicTestMnemonic[] =
    "abandon abandon abandon abandon abandon abandon abandon abandon "
    "abandon abandon abandon about";

const QString kExpectedEthereum =
    QStringLiteral("0x9858EfFD232B4033E47d90003D41EC34EcaEda94");
const QString kExpectedBitcoin =
    QStringLiteral("bc1qcr8te4kr609gcawutmrza0j4xv80jy8z306fyu");
const QString kExpectedSolana =
    QStringLiteral("GjJyeC1r2RgkuoCWMyPYkCWSGSGLcz266EaAkLA27AhL");

QString fromTWString(TWString *value)
{
    if (!value)
        return QString();
    const char *bytes = TWStringUTF8Bytes(value);
    return bytes ? QString::fromUtf8(bytes) : QString();
}

class TWStringGuard
{
public:
    explicit TWStringGuard(TWString *value = nullptr) : m_value(value) {}
    ~TWStringGuard() { if (m_value) TWStringDelete(m_value); }
    TWString *get() const { return m_value; }
private:
    TWString *m_value;
};

class TWAnyAddressGuard
{
public:
    explicit TWAnyAddressGuard(TWAnyAddress *value = nullptr) : m_value(value) {}
    ~TWAnyAddressGuard() { if (m_value) TWAnyAddressDelete(m_value); }
    TWAnyAddress *get() const { return m_value; }
private:
    TWAnyAddress *m_value;
};

class TWDataGuard
{
public:
    explicit TWDataGuard(TWData *value = nullptr) : m_value(value) {}
    ~TWDataGuard() { if (m_value) TWDataDelete(m_value); }
    TWData *get() const { return m_value; }
private:
    TWData *m_value;
};

class SensitiveTWDataGuard
{
public:
    explicit SensitiveTWDataGuard(TWData *value = nullptr) : m_value(value) {}
    ~SensitiveTWDataGuard()
    {
        if (m_value) {
            TWDataReset(m_value);
            TWDataDelete(m_value);
        }
    }
    TWData *get() const { return m_value; }
private:
    TWData *m_value;
};

class TWPrivateKeyGuard
{
public:
    explicit TWPrivateKeyGuard(TWPrivateKey *value = nullptr) : m_value(value) {}
    ~TWPrivateKeyGuard() { if (m_value) TWPrivateKeyDelete(m_value); }
    TWPrivateKey *get() const { return m_value; }
private:
    TWPrivateKey *m_value;
};

class TWPublicKeyGuard
{
public:
    explicit TWPublicKeyGuard(TWPublicKey *value = nullptr) : m_value(value) {}
    ~TWPublicKeyGuard() { if (m_value) TWPublicKeyDelete(m_value); }
    TWPublicKey *get() const { return m_value; }
private:
    TWPublicKey *m_value;
};

class TWHDWalletGuard
{
public:
    explicit TWHDWalletGuard(TWHDWallet *value = nullptr) : m_value(value) {}
    ~TWHDWalletGuard() { if (m_value) TWHDWalletDelete(m_value); }
    TWHDWallet *get() const { return m_value; }
private:
    TWHDWallet *m_value;
};

} // namespace

DevelopmentSigningService::DevelopmentSigningService(QObject *parent)
    : QObject(parent)
    , m_status(QStringLiteral(
          "Development sign/verify has not run · production signing remains disabled"))
{
}

bool DevelopmentSigningService::signing() const { return m_signing; }
bool DevelopmentSigningService::signatureVerified() const { return m_signatureVerified; }
QString DevelopmentSigningService::status() const { return m_status; }
QString DevelopmentSigningService::signerAddress() const { return m_signerAddress; }
QString DevelopmentSigningService::signingHashHex() const { return m_signingHashHex; }
QString DevelopmentSigningService::constructionFingerprint() const { return m_constructionFingerprint; }
QString DevelopmentSigningService::signatureProofFingerprint() const { return m_signatureProofFingerprint; }
int DevelopmentSigningService::signatureSize() const { return m_signatureSize; }
QString DevelopmentSigningService::signedAt() const { return m_signedAt; }

void DevelopmentSigningService::secureErase(QByteArray *bytes)
{
    if (!bytes || bytes->isEmpty())
        return;
    bytes->detach();
    volatile char *p = bytes->data();
    for (int i = 0; i < bytes->size(); ++i)
        p[i] = 0;
    bytes->clear();
    bytes->squeeze();
}

void DevelopmentSigningService::clearState(bool preserveStatus)
{
    m_signing = false;
    m_signatureVerified = false;
    if (!preserveStatus) {
        m_status = QStringLiteral(
            "Development sign/verify has not run · production signing remains disabled");
    }
    m_signerAddress.clear();
    m_signingHashHex.clear();
    m_constructionFingerprint.clear();
    m_signatureProofFingerprint.clear();
    m_signatureSize = 0;
    m_signedAt.clear();
}

void DevelopmentSigningService::reset()
{
    clearState();
    emit stateChanged();
}

void DevelopmentSigningService::fail(const QString &message)
{
    clearState(true);
    m_status = message;
    emit stateChanged();
}

bool DevelopmentSigningService::parseHash32(const QString &hex, QByteArray *bytes)
{
    if (!bytes)
        return false;
    QString value = hex.trimmed();
    if (value.startsWith(QStringLiteral("0x"), Qt::CaseInsensitive))
        value.remove(0, 2);
    static const QRegularExpression pattern(QStringLiteral("^[0-9a-fA-F]{64}$"));
    if (!pattern.match(value).hasMatch())
        return false;
    const QByteArray decoded = QByteArray::fromHex(value.toLatin1());
    if (decoded.size() != 32)
        return false;
    *bytes = decoded;
    return true;
}

bool DevelopmentSigningService::signDigestWithDevelopmentKey(
        TWPrivateKey *key,
        const QByteArray &digest,
        const QString &expectedSigner,
        QByteArray *signature,
        int *recoveryId,
        QString *error)
{
    const auto fail = [error](const QString &message) {
        if (error)
            *error = message;
        return false;
    };

    if (!key || !signature || digest.size() != 32)
        return fail(QStringLiteral("Development signing input is incomplete."));

    TWPublicKeyGuard publicKey(TWPrivateKeyGetPublicKey(key, TWCoinTypeEthereum));
    TWAnyAddressGuard anyAddress(publicKey.get()
        ? TWAnyAddressCreateWithPublicKey(publicKey.get(), TWCoinTypeEthereum)
        : nullptr);
    TWStringGuard address(anyAddress.get()
        ? TWAnyAddressDescription(anyAddress.get())
        : nullptr);
    const QString derivedAddress = fromTWString(address.get());
    if (derivedAddress.isEmpty()
            || derivedAddress.compare(expectedSigner, Qt::CaseInsensitive) != 0
            || derivedAddress.compare(kExpectedEthereum, Qt::CaseInsensitive) != 0) {
        return fail(QStringLiteral(
            "Transient Ethereum key is not the published development signer."));
    }

    TWDataGuard digestData(TWDataCreateWithBytes(
        reinterpret_cast<const uint8_t *>(digest.constData()),
        static_cast<size_t>(digest.size())));
    if (!digestData.get())
        return fail(QStringLiteral("Wallet Core could not create the Ethereum signing digest."));

    SensitiveTWDataGuard signatureData(
        TWPrivateKeySign(key, digestData.get(), TWCurveSECP256k1));
    if (!signatureData.get() || !publicKey.get()
            || TWDataSize(signatureData.get()) != 65) {
        return fail(QStringLiteral("Wallet Core did not return a 65-byte recoverable secp256k1 signature."));
    }

    if (!TWPublicKeyVerify(publicKey.get(), signatureData.get(), digestData.get()))
        return fail(QStringLiteral("Wallet Core rejected its generated Ethereum signature."));

    const uint8_t *raw = TWDataBytes(signatureData.get());
    const size_t size = TWDataSize(signatureData.get());
    if (!raw || size != 65)
        return fail(QStringLiteral("Wallet Core signature bytes are unavailable."));

    QByteArray result(reinterpret_cast<const char *>(raw), static_cast<int>(size));
    const int recovery = static_cast<unsigned char>(result.at(64));
    if (recovery < 0 || recovery > 3) {
        secureErase(&result);
        return fail(QStringLiteral("Wallet Core returned an invalid secp256k1 recovery id."));
    }

    *signature = result;
    if (recoveryId)
        *recoveryId = recovery;
    if (error)
        error->clear();
    return true;
}

QString DevelopmentSigningService::signatureProofFor(
        const QString &intentFingerprint,
        const QString &constructionFingerprint,
        const QString &signingHashHex,
        const QString &signerAddress,
        const QByteArray &signature)
{
    const QByteArray canonical = QByteArray("sailvault-development-eth-signature-proof-v1\n")
        + intentFingerprint.toLatin1() + '\n'
        + constructionFingerprint.toLatin1() + '\n'
        + signingHashHex.toLower().toLatin1() + '\n'
        + signerAddress.toLower().toLatin1() + '\n'
        + signature.toHex();
    return QString::fromLatin1(QCryptographicHash::hash(
        canonical, QCryptographicHash::Sha256).toHex());
}

void DevelopmentSigningService::signEthereum(
        QObject *vaultObject,
        QObject *ethereumBuilderObject,
        const QVariantMap &intent)
{
    clearState();

    WalletVault *vault = qobject_cast<WalletVault *>(vaultObject);
    EthereumUnsignedTransactionService *builder =
        qobject_cast<EthereumUnsignedTransactionService *>(ethereumBuilderObject);
    if (!vault || !builder) {
        fail(QStringLiteral(
            "Development signing blocked · WalletVault/Ethereum constructor boundary is unavailable"));
        return;
    }

    if (!vault->walletLoaded()) {
        fail(QStringLiteral(
            "Development signing blocked · load the public development wallet first"));
        return;
    }

    // Defense in depth: the signer has its own copy of the published public
    // identity gate in addition to WalletVault's secret-side revalidation.
    if (vault->ethereumAddress().compare(kExpectedEthereum, Qt::CaseInsensitive) != 0
            || vault->bitcoinAddress() != kExpectedBitcoin
            || vault->solanaAddress() != kExpectedSolana) {
        fail(QStringLiteral(
            "Development signing blocked · loaded session is not the published ETH/BTC/SOL test wallet"));
        return;
    }

    if (!builder->constructed()) {
        fail(QStringLiteral(
            "Development signing blocked · construct and review the Ethereum payload first"));
        return;
    }

    const QVariantMap snapshot = builder->signingSnapshot();
    QString snapshotError;
    if (!EthereumUnsignedTransactionService::validateSigningSnapshot(
            intent, snapshot, &snapshotError)) {
        fail(QStringLiteral("Development signing blocked · %1").arg(snapshotError));
        return;
    }

    const QString source = intent.value(QStringLiteral("sourceAddress")).toString();
    if (source.compare(kExpectedEthereum, Qt::CaseInsensitive) != 0
            || source.compare(vault->ethereumAddress(), Qt::CaseInsensitive) != 0) {
        fail(QStringLiteral(
            "Development signing blocked · reviewed source is not the published test signer"));
        return;
    }

    const qint64 constructedEpochMs =
        snapshot.value(QStringLiteral("constructedEpochMs")).toLongLong();
    const qint64 nowMs = QDateTime::currentMSecsSinceEpoch();
    if (constructedEpochMs <= 0
            || constructedEpochMs > nowMs + 10000
            || nowMs - constructedEpochMs > kMaximumConstructionAgeMs) {
        fail(QStringLiteral(
            "Development signing blocked · unsigned network inputs are older than two minutes; reconstruct first"));
        return;
    }

    QByteArray digest;
    if (!parseHash32(snapshot.value(QStringLiteral("signingHashHex")).toString(),
                     &digest)) {
        fail(QStringLiteral(
            "Development signing blocked · signing hash is not exactly 32 bytes"));
        return;
    }

    m_signing = true;
    m_status = QStringLiteral(
        "Opening the revalidated public test wallet through Sailfish Secrets…");
    emit stateChanged();

    QByteArray signature;
    int recoveryId = -1;
    QString boundaryError;
    const bool signedOk = vault->withVerifiedDevelopmentEthereumKey(
        [this, &digest, &source, &signature, &recoveryId](
                TWPrivateKey *key, QString *error) {
            return signDigestWithDevelopmentKey(
                key, digest, source, &signature, &recoveryId, error);
        },
        &boundaryError);

    if (!signedOk) {
        secureErase(&digest);
        secureErase(&signature);
        fail(QStringLiteral("Development signing blocked · %1").arg(boundaryError));
        return;
    }

    if (signature.size() != 65 || recoveryId < 0 || recoveryId > 3) {
        secureErase(&digest);
        secureErase(&signature);
        fail(QStringLiteral(
            "Development signing failed · verified signature metadata is inconsistent"));
        return;
    }

    const QString proof = signatureProofFor(
        intent.value(QStringLiteral("fingerprint")).toString(),
        snapshot.value(QStringLiteral("constructionFingerprint")).toString(),
        snapshot.value(QStringLiteral("signingHashHex")).toString(),
        source,
        signature);

    m_signerAddress = source;
    m_signingHashHex = snapshot.value(QStringLiteral("signingHashHex")).toString();
    m_constructionFingerprint =
        snapshot.value(QStringLiteral("constructionFingerprint")).toString();
    m_signatureProofFingerprint = proof;
    m_signatureSize = signature.size();
    m_signedAt = QDateTime::currentDateTime().toString(
        QStringLiteral("yyyy-MM-dd HH:mm:ss"));

    // The raw signature is intentionally not retained or exposed to QML in
    // M52. This milestone proves Secrets -> Wallet Core sign/verify while
    // still preventing SailVault from assembling a broadcastable transaction.
    secureErase(&digest);
    secureErase(&signature);

    m_signing = false;
    m_signatureVerified = proof.size() == 64;
    m_status = m_signatureVerified
        ? QStringLiteral(
              "PASS · Public test-wallet Ethereum signature created, verified and discarded · no signed transaction assembled")
        : QStringLiteral("Development signing failed · signature proof fingerprint unavailable");
    emit stateChanged();
}

QVariantMap DevelopmentSigningService::runSelfTest() const
{
    QVariantMap result;

    QByteArray mnemonic(kPublicTestMnemonic);
    TWStringGuard mnemonicString(TWStringCreateWithUTF8Bytes(mnemonic.constData()));
    TWStringGuard passphrase(TWStringCreateWithUTF8Bytes(""));
    TWHDWalletGuard wallet(
        mnemonicString.get() && passphrase.get()
            ? TWHDWalletCreateWithMnemonic(mnemonicString.get(), passphrase.get())
            : nullptr);

    QString ethereum;
    QString bitcoin;
    QString solana;
    if (wallet.get()) {
        TWStringGuard eth(TWHDWalletGetAddressForCoin(wallet.get(), TWCoinTypeEthereum));
        TWStringGuard btc(TWHDWalletGetAddressForCoin(wallet.get(), TWCoinTypeBitcoin));
        TWStringGuard sol(TWHDWalletGetAddressForCoin(wallet.get(), TWCoinTypeSolana));
        ethereum = fromTWString(eth.get());
        bitcoin = fromTWString(btc.get());
        solana = fromTWString(sol.get());
    }

    const bool publishedIdentity = wallet.get()
        && ethereum.compare(kExpectedEthereum, Qt::CaseInsensitive) == 0
        && bitcoin == kExpectedBitcoin
        && solana == kExpectedSolana;

    QByteArray digest = QByteArray::fromHex(
        "ed5ffeca0540f40606b47a01aaf578365c2149d60220703eae796bc2a01da6d0");
    QByteArray signature;
    int recoveryId = -1;
    QString signingError;
    TWPrivateKeyGuard selfTestKey(
        wallet.get() ? TWHDWalletGetKeyForCoin(wallet.get(), TWCoinTypeEthereum) : nullptr);
    const bool signVerify = publishedIdentity && selfTestKey.get()
        && signDigestWithDevelopmentKey(
            selfTestKey.get(), digest, kExpectedEthereum,
            &signature, &recoveryId, &signingError)
        && signature.size() == 65
        && recoveryId >= 0 && recoveryId <= 3;

    bool mutatedDigestRejected = false;
    if (signVerify && selfTestKey.get()) {
        TWPublicKeyGuard publicKey(
            TWPrivateKeyGetPublicKey(selfTestKey.get(), TWCoinTypeEthereum));
        QByteArray mutated = digest;
        mutated[0] = static_cast<char>(
            static_cast<unsigned char>(mutated.at(0)) ^ 0x01U);
        TWDataGuard mutatedData(TWDataCreateWithBytes(
            reinterpret_cast<const uint8_t *>(mutated.constData()),
            static_cast<size_t>(mutated.size())));
        SensitiveTWDataGuard signatureData(TWDataCreateWithBytes(
            reinterpret_cast<const uint8_t *>(signature.constData()),
            static_cast<size_t>(signature.size())));
        mutatedDigestRejected = publicKey.get() && mutatedData.get() && signatureData.get()
            && !TWPublicKeyVerify(publicKey.get(), signatureData.get(), mutatedData.get());
        secureErase(&mutated);
    }

    const QString proofA = signVerify
        ? signatureProofFor(
              QString(64, QLatin1Char('a')),
              QString(64, QLatin1Char('b')),
              QStringLiteral("0x") + QString::fromLatin1(digest.toHex()),
              kExpectedEthereum,
              signature)
        : QString();
    QByteArray changedSignature = signature;
    if (!changedSignature.isEmpty())
        changedSignature[0] = static_cast<char>(
            static_cast<unsigned char>(changedSignature.at(0)) ^ 0x01U);
    const QString proofB = signVerify
        ? signatureProofFor(
              QString(64, QLatin1Char('a')),
              QString(64, QLatin1Char('b')),
              QStringLiteral("0x") + QString::fromLatin1(digest.toHex()),
              kExpectedEthereum,
              changedSignature)
        : QString();
    const bool proofBinding = proofA.size() == 64
        && proofB.size() == 64 && proofA != proofB;

    secureErase(&mnemonic);
    secureErase(&digest);
    secureErase(&signature);
    secureErase(&changedSignature);

    const bool passed = publishedIdentity && signVerify
        && mutatedDigestRejected && proofBinding;
    result.insert(QStringLiteral("passed"), passed);
    result.insert(QStringLiteral("publishedIdentityPassed"), publishedIdentity);
    result.insert(QStringLiteral("signVerifyPassed"), signVerify);
    result.insert(QStringLiteral("mutatedDigestRejected"), mutatedDigestRejected);
    result.insert(QStringLiteral("signatureProofBindingPassed"), proofBinding);
    result.insert(QStringLiteral("rawSignatureExported"), false);
    result.insert(QStringLiteral("detail"), passed
        ? QStringLiteral(
              "Published ETH/BTC/SOL test-wallet identity, transient Wallet Core secp256k1 sign/verify, mutated-digest rejection and non-exporting signature-proof binding passed")
        : QStringLiteral("Development signing-boundary self-test failed"));
    return result;
}
