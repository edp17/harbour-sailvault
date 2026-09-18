#include "walletcoreprobe.h"

#include <QSysInfo>
#include <QVariantMap>

#include <TrustWalletCore/TWCoinType.h>
#include <TrustWalletCore/TWCurve.h>
#include <TrustWalletCore/TWData.h>
#include <TrustWalletCore/TWHDWallet.h>
#include <TrustWalletCore/TWPrivateKey.h>
#include <TrustWalletCore/TWPublicKey.h>
#include <TrustWalletCore/TWString.h>

namespace {

const char *kTestMnemonic =
    "abandon abandon abandon abandon abandon abandon abandon abandon "
    "abandon abandon abandon about";

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

class TWDataGuard
{
public:
    explicit TWDataGuard(TWData *value = nullptr) : m_value(value) {}
    ~TWDataGuard() { if (m_value) TWDataDelete(m_value); }
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

WalletCoreProbe::WalletCoreProbe(QObject *parent)
    : QObject(parent)
{
}

QVariantList WalletCoreProbe::results() const
{
    return m_results;
}

QString WalletCoreProbe::summary() const
{
    return m_summary;
}

QString WalletCoreProbe::buildInfo() const
{
    return m_buildInfo;
}

bool WalletCoreProbe::allPassed() const
{
    return m_allPassed;
}

void WalletCoreProbe::addResult(const QString &name,
                                bool passed,
                                const QString &detail,
                                const QString &expected)
{
    QVariantMap item;
    item.insert(QStringLiteral("name"), name);
    item.insert(QStringLiteral("passed"), passed);
    item.insert(QStringLiteral("detail"), detail);
    item.insert(QStringLiteral("expected"), expected);
    m_results.append(item);
}

void WalletCoreProbe::run()
{
    m_results.clear();
    m_allPassed = false;

    m_buildInfo = QStringLiteral("Wallet Core %1 · %2 · %3-bit")
            .arg(QStringLiteral(SAILVAULT_WALLET_CORE_VERSION),
                 QSysInfo::currentCpuArchitecture())
            .arg(QSysInfo::WordSize);

    TWStringGuard mnemonic(TWStringCreateWithUTF8Bytes(kTestMnemonic));
    TWStringGuard passphrase(TWStringCreateWithUTF8Bytes(""));

    if (!mnemonic.get() || !passphrase.get()) {
        addResult(QStringLiteral("C ABI bootstrap"), false,
                  QStringLiteral("Could not create Wallet Core strings."));
        m_summary = QStringLiteral("FAIL · Wallet Core C ABI initialization failed");
        emit resultsChanged();
        return;
    }

    TWHDWalletGuard wallet(
        TWHDWalletCreateWithMnemonic(mnemonic.get(), passphrase.get()));

    if (!wallet.get()) {
        addResult(QStringLiteral("BIP-39 wallet"), false,
                  QStringLiteral("Wallet Core rejected the public BIP-39 test mnemonic."));
        m_summary = QStringLiteral("FAIL · HD wallet creation failed");
        emit resultsChanged();
        return;
    }

    addResult(QStringLiteral("BIP-39 wallet"), true,
              QStringLiteral("Public 12-word test vector accepted."));

    const QString expectedEth =
        QStringLiteral("0x9858EfFD232B4033E47d90003D41EC34EcaEda94");
    TWStringGuard ethAddress(
        TWHDWalletGetAddressForCoin(wallet.get(), TWCoinTypeEthereum));
    const QString actualEth = fromTWString(ethAddress.get());
    addResult(QStringLiteral("Ethereum derivation"),
              actualEth == expectedEth,
              actualEth.isEmpty()
                  ? QStringLiteral("No Ethereum address returned.")
                  : actualEth,
              expectedEth);

    const QString expectedBtc =
        QStringLiteral("bc1qcr8te4kr609gcawutmrza0j4xv80jy8z306fyu");
    TWStringGuard btcAddress(
        TWHDWalletGetAddressForCoin(wallet.get(), TWCoinTypeBitcoin));
    const QString actualBtc = fromTWString(btcAddress.get());
    addResult(QStringLiteral("Bitcoin BIP-84 derivation"),
              actualBtc == expectedBtc,
              actualBtc.isEmpty()
                  ? QStringLiteral("No Bitcoin address returned.")
                  : actualBtc,
              expectedBtc);

    // This is the same 32-byte digest used by Wallet Core's own HD-wallet
    // signing test. The test is local-only and never touches a network.
    const uint8_t digestBytes[32] = {
        0x3F, 0x89, 0x1F, 0xDA, 0x37, 0x04, 0xF0, 0x36,
        0x8D, 0xAB, 0x65, 0xFA, 0x81, 0xEB, 0xE6, 0x16,
        0xF4, 0xAA, 0x2A, 0x08, 0x54, 0x99, 0x5D, 0xA4,
        0xDC, 0x0B, 0x59, 0xD2, 0xCA, 0xDB, 0xD6, 0x4F
    };

    TWPrivateKeyGuard key(
        TWHDWalletGetKeyForCoin(wallet.get(), TWCoinTypeEthereum));
    TWDataGuard digest(TWDataCreateWithBytes(digestBytes, sizeof(digestBytes)));

    bool signPassed = false;
    QString signDetail;

    if (!key.get() || !digest.get()) {
        signDetail = QStringLiteral("Could not derive the Ethereum key or create the digest.");
    } else {
        TWDataGuard signature(
            TWPrivateKeySign(key.get(), digest.get(), TWCurveSECP256k1));
        TWPublicKeyGuard publicKey(
            TWPrivateKeyGetPublicKeySecp256k1(key.get(), false));

        if (!signature.get() || !publicKey.get()) {
            signDetail = QStringLiteral("Wallet Core did not return a signature/public key.");
        } else {
            const size_t signatureSize = TWDataSize(signature.get());
            const bool verifies =
                TWPublicKeyVerify(publicKey.get(), signature.get(), digest.get());
            signPassed = (signatureSize == 65 && verifies);
            signDetail = QStringLiteral("%1-byte secp256k1 signature; verify=%2")
                    .arg(signatureSize)
                    .arg(verifies ? QStringLiteral("true")
                                  : QStringLiteral("false"));
        }
    }

    addResult(QStringLiteral("secp256k1 sign + verify"),
              signPassed,
              signDetail,
              QStringLiteral("65-byte signature; verify=true"));

    m_allPassed = true;
    for (const QVariant &entry : m_results) {
        if (!entry.toMap().value(QStringLiteral("passed")).toBool()) {
            m_allPassed = false;
            break;
        }
    }

    m_summary = m_allPassed
        ? QStringLiteral("PASS · Wallet Core works on this Sailfish build")
        : QStringLiteral("FAIL · One or more Wallet Core checks failed");

    emit resultsChanged();
}
