#include "walletcoreprobe.h"

#include <QSysInfo>
#include <QStringList>
#include <QVariantMap>

#include <cstring>

#include <TrustWalletCore/TWAnyAddress.h>
#include <TrustWalletCore/TWCoinType.h>
#include <TrustWalletCore/TWCryptoBox.h>
#include <TrustWalletCore/TWCryptoBoxPublicKey.h>
#include <TrustWalletCore/TWCryptoBoxSecretKey.h>
#include <TrustWalletCore/TWCurve.h>
#include <TrustWalletCore/TWDerivation.h>
#include <TrustWalletCore/TWData.h>
#include <TrustWalletCore/TWHDWallet.h>
#include <TrustWalletCore/TWMnemonic.h>
#include <TrustWalletCore/TWPrivateKey.h>
#include <TrustWalletCore/TWPublicKey.h>
#include <TrustWalletCore/TWString.h>

namespace {

const char *kTestMnemonic =
    "abandon abandon abandon abandon abandon abandon abandon abandon "
    "abandon abandon abandon about";

// Wallet Core's derivation identifiers are shared with the Rust registry and are
// append-only ABI values. M40 had to reproduce this table for Sailfish; keep a
// compile-time tripwire in the application so drift cannot silently ship.
static_assert(TWDerivationDefault == 0, "Wallet Core derivation ABI drift");
static_assert(TWDerivationCustom == 1, "Wallet Core derivation ABI drift");
static_assert(TWDerivationBitcoinSegwit == 2, "Wallet Core derivation ABI drift");
static_assert(TWDerivationBitcoinLegacy == 3, "Wallet Core derivation ABI drift");
static_assert(TWDerivationBitcoinTestnet == 4, "Wallet Core derivation ABI drift");
static_assert(TWDerivationLitecoinLegacy == 5, "Wallet Core derivation ABI drift");
static_assert(TWDerivationSolanaSolana == 6, "Wallet Core derivation ABI drift");
static_assert(TWDerivationStratisSegwit == 7, "Wallet Core derivation ABI drift");
static_assert(TWDerivationBitcoinTaproot == 8, "Wallet Core derivation ABI drift");
static_assert(TWDerivationPactusMainnet == 9, "Wallet Core derivation ABI drift");
static_assert(TWDerivationPactusTestnet == 10, "Wallet Core derivation ABI drift");
static_assert(TWDerivationSmartChainStableAccount == 11, "Wallet Core derivation ABI drift");

static_assert(TWCoinTypeBitcoin == 0, "Wallet Core Bitcoin coin-type ABI drift");
static_assert(TWCoinTypeEthereum == 60, "Wallet Core Ethereum coin-type ABI drift");
static_assert(TWCoinTypeSolana == 501, "Wallet Core Solana coin-type ABI drift");

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

class TWAnyAddressGuard
{
public:
    explicit TWAnyAddressGuard(TWAnyAddress *value = nullptr) : m_value(value) {}
    ~TWAnyAddressGuard() { if (m_value) TWAnyAddressDelete(m_value); }
    TWAnyAddress *get() const { return m_value; }
private:
    TWAnyAddress *m_value;
};

class TWCryptoBoxSecretKeyGuard
{
public:
    explicit TWCryptoBoxSecretKeyGuard(TWCryptoBoxSecretKey *value = nullptr) : m_value(value) {}
    ~TWCryptoBoxSecretKeyGuard() { if (m_value) TWCryptoBoxSecretKeyDelete(m_value); }
    TWCryptoBoxSecretKey *get() const { return m_value; }
private:
    TWCryptoBoxSecretKey *m_value;
};

class TWCryptoBoxPublicKeyGuard
{
public:
    explicit TWCryptoBoxPublicKeyGuard(TWCryptoBoxPublicKey *value = nullptr) : m_value(value) {}
    ~TWCryptoBoxPublicKeyGuard() { if (m_value) TWCryptoBoxPublicKeyDelete(m_value); }
    TWCryptoBoxPublicKey *get() const { return m_value; }
private:
    TWCryptoBoxPublicKey *m_value;
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

int WalletCoreProbe::runCount() const
{
    return m_runCount;
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
    ++m_runCount;
    m_results.clear();
    m_allPassed = false;

    m_buildInfo = QStringLiteral("Wallet Core %1 · %2 · %3-bit · self-check run #%4")
            .arg(QStringLiteral(SAILVAULT_WALLET_CORE_VERSION),
                 QSysInfo::currentCpuArchitecture())
            .arg(QSysInfo::WordSize)
            .arg(m_runCount);

    TWStringGuard mnemonic(TWStringCreateWithUTF8Bytes(kTestMnemonic));
    TWStringGuard passphrase(TWStringCreateWithUTF8Bytes(""));

    if (!mnemonic.get() || !passphrase.get()) {
        addResult(QStringLiteral("C ABI bootstrap"), false,
                  QStringLiteral("Could not create Wallet Core strings."));
        m_summary = QStringLiteral("FAIL · Wallet Core C ABI initialization failed");
        emit resultsChanged();
        return;
    }

    TWStringGuard invalidMnemonic(TWStringCreateWithUTF8Bytes(
        "abandon abandon abandon abandon abandon abandon abandon abandon "
        "abandon abandon abandon sailvault"));
    const bool mnemonicValidationPassed =
        TWMnemonicIsValid(mnemonic.get())
        && invalidMnemonic.get()
        && !TWMnemonicIsValid(invalidMnemonic.get());
    addResult(QStringLiteral("BIP-39 validation"),
              mnemonicValidationPassed,
              mnemonicValidationPassed
                  ? QStringLiteral("Valid test phrase accepted; invalid-word phrase rejected.")
                  : QStringLiteral("Mnemonic validation did not preserve positive/negative behavior."),
              QStringLiteral("valid=true; invalid=false"));

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

    const QString expectedSol =
        QStringLiteral("GjJyeC1r2RgkuoCWMyPYkCWSGSGLcz266EaAkLA27AhL");
    TWStringGuard solAddress(
        TWHDWalletGetAddressForCoin(wallet.get(), TWCoinTypeSolana));
    const QString actualSol = fromTWString(solAddress.get());
    addResult(QStringLiteral("Solana derivation"),
              actualSol == expectedSol,
              actualSol.isEmpty()
                  ? QStringLiteral("No Solana address returned.")
                  : actualSol,
              expectedSol);

    // Validate both the positive addresses and deliberately malformed/cross-chain
    // inputs. These strings are public test vectors only.
    TWStringGuard ethVector(TWStringCreateWithUTF8Bytes(expectedEth.toUtf8().constData()));
    TWStringGuard btcVector(TWStringCreateWithUTF8Bytes(expectedBtc.toUtf8().constData()));
    TWStringGuard solVector(TWStringCreateWithUTF8Bytes(expectedSol.toUtf8().constData()));
    TWStringGuard invalidAddress(TWStringCreateWithUTF8Bytes("sailvault-not-an-address"));

    const bool addressValidationPassed =
        ethVector.get() && btcVector.get() && solVector.get() && invalidAddress.get()
        && TWAnyAddressIsValid(ethVector.get(), TWCoinTypeEthereum)
        && TWAnyAddressIsValid(btcVector.get(), TWCoinTypeBitcoin)
        && TWAnyAddressIsValid(solVector.get(), TWCoinTypeSolana)
        && !TWAnyAddressIsValid(invalidAddress.get(), TWCoinTypeEthereum)
        && !TWAnyAddressIsValid(invalidAddress.get(), TWCoinTypeBitcoin)
        && !TWAnyAddressIsValid(invalidAddress.get(), TWCoinTypeSolana)
        && !TWAnyAddressIsValid(ethVector.get(), TWCoinTypeBitcoin)
        && !TWAnyAddressIsValid(btcVector.get(), TWCoinTypeSolana);
    addResult(QStringLiteral("Address validation hardening"),
              addressValidationPassed,
              addressValidationPassed
                  ? QStringLiteral("BTC/ETH/SOL vectors accepted; malformed and wrong-chain inputs rejected.")
                  : QStringLiteral("Address validation positive/negative gate failed."),
              QStringLiteral("valid vectors=true; malformed/cross-chain=false"));

    // Round-trip each default derived key through its public key and AnyAddress.
    // Private-key bytes never leave native C++ and are never placed in result/QML data.
    bool roundTripPassed = true;
    QStringList roundTripDetails;
    struct RoundTripVector {
        TWCoinType coin;
        QString name;
        QString expected;
    };
    const RoundTripVector roundTrips[] = {
        { TWCoinTypeBitcoin, QStringLiteral("BTC"), expectedBtc },
        { TWCoinTypeEthereum, QStringLiteral("ETH"), expectedEth },
        { TWCoinTypeSolana, QStringLiteral("SOL"), expectedSol }
    };
    for (const RoundTripVector &item : roundTrips) {
        TWPrivateKeyGuard derivedKey(TWHDWalletGetKeyForCoin(wallet.get(), item.coin));
        TWPublicKeyGuard derivedPublicKey(
            derivedKey.get() ? TWPrivateKeyGetPublicKey(derivedKey.get(), item.coin) : nullptr);
        TWAnyAddressGuard address(
            derivedPublicKey.get()
                ? TWAnyAddressCreateWithPublicKey(derivedPublicKey.get(), item.coin)
                : nullptr);
        TWStringGuard addressText(
            address.get() ? TWAnyAddressDescription(address.get()) : nullptr);
        const QString actual = fromTWString(addressText.get());
        const bool passed = !actual.isEmpty() && actual == item.expected;
        roundTripPassed = roundTripPassed && passed;
        roundTripDetails.append(item.name + QStringLiteral("=")
                                + (passed ? QStringLiteral("ok") : QStringLiteral("fail")));
    }
    addResult(QStringLiteral("Public-key/address round-trip"),
              roundTripPassed,
              roundTripDetails.join(QStringLiteral(" · ")),
              QStringLiteral("BTC=ok · ETH=ok · SOL=ok"));

    // Wallet Core 4.7.0 hardened derivation-index bounds and 4.8.0 made key
    // retrieval explicitly nullable on failure. Exercise both contracts.
    TWStringGuard invalidPath(
        TWStringCreateWithUTF8Bytes("m/44'/60'/2147483648'/0/0"));
    TWPrivateKeyGuard invalidPathKey(
        invalidPath.get()
            ? TWHDWalletGetKey(wallet.get(), TWCoinTypeEthereum, invalidPath.get())
            : nullptr);
    const bool invalidPathRejected = invalidPath.get() && !invalidPathKey.get();
    addResult(QStringLiteral("Derivation-path bounds"),
              invalidPathRejected,
              invalidPathRejected
                  ? QStringLiteral("Out-of-range hardened index rejected with null key result.")
                  : QStringLiteral("Out-of-range derivation path was not rejected."),
              QStringLiteral("null key"));

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

            uint8_t mutatedDigestBytes[sizeof(digestBytes)];
            std::memcpy(mutatedDigestBytes, digestBytes, sizeof(digestBytes));
            mutatedDigestBytes[sizeof(mutatedDigestBytes) - 1] ^= 0x01;
            TWDataGuard mutatedDigest(
                TWDataCreateWithBytes(mutatedDigestBytes, sizeof(mutatedDigestBytes)));
            const uint8_t shortSignatureByte = 0;
            TWDataGuard shortSignature(
                TWDataCreateWithBytes(&shortSignatureByte, 1));

            const bool rejectsMutatedDigest = mutatedDigest.get()
                && !TWPublicKeyVerify(publicKey.get(), signature.get(), mutatedDigest.get());
            const bool rejectsShortSignature = shortSignature.get()
                && !TWPublicKeyVerify(publicKey.get(), shortSignature.get(), digest.get());

            signPassed = (signatureSize == 65
                          && verifies
                          && rejectsMutatedDigest
                          && rejectsShortSignature);
            signDetail = QStringLiteral("%1-byte secp256k1 signature · verify=%2 · mutated-digest=%3 · short-signature=%4")
                    .arg(signatureSize)
                    .arg(verifies ? QStringLiteral("true") : QStringLiteral("false"))
                    .arg(rejectsMutatedDigest ? QStringLiteral("rejected") : QStringLiteral("accepted"))
                    .arg(rejectsShortSignature ? QStringLiteral("rejected") : QStringLiteral("accepted"));
        }
    }

    addResult(QStringLiteral("secp256k1 validation hardening"),
              signPassed,
              signDetail,
              QStringLiteral("65-byte signature verifies; mutated digest and short signature rejected"));

    // M40r9 restored Wallet Core 4.8.3's Rust-backed C++ codegen stage. Exercise
    // that generated ABI at runtime, including the maintained small-order-key
    // rejection behavior and a local-only encrypt/decrypt round-trip.
    bool cryptoBoxPassed = false;
    QString cryptoBoxDetail;
    const uint8_t knownSecretBytes[32] = {
        0xdd, 0x87, 0x00, 0x0d, 0x48, 0x05, 0xd6, 0xfb,
        0xd8, 0x9a, 0xe1, 0x35, 0x2f, 0x5e, 0x44, 0x45,
        0x64, 0x8b, 0x79, 0xd5, 0xe9, 0x01, 0xc9, 0x2a,
        0xeb, 0xcb, 0x61, 0x0e, 0x9b, 0xe4, 0x68, 0xe4
    };
    const uint8_t smallOrderPublicKey[32] = { 0 };
    const uint8_t messageBytes[] = {
        'S','a','i','l','V','a','u','l','t',' ','M','4','1'
    };

    TWDataGuard knownSecretData(
        TWDataCreateWithBytes(knownSecretBytes, sizeof(knownSecretBytes)));
    TWDataGuard smallOrderData(
        TWDataCreateWithBytes(smallOrderPublicKey, sizeof(smallOrderPublicKey)));
    TWDataGuard messageData(
        TWDataCreateWithBytes(messageBytes, sizeof(messageBytes)));

    if (!knownSecretData.get() || !smallOrderData.get() || !messageData.get()) {
        cryptoBoxDetail = QStringLiteral("Could not allocate local CryptoBox test data.");
    } else {
        const bool knownSecretValid = TWCryptoBoxSecretKeyIsValid(knownSecretData.get());
        const bool smallOrderRejected = !TWCryptoBoxPublicKeyIsValid(smallOrderData.get());
        TWCryptoBoxSecretKeyGuard mySecret(
            knownSecretValid
                ? TWCryptoBoxSecretKeyCreateWithData(knownSecretData.get())
                : nullptr);
        TWCryptoBoxSecretKeyGuard otherSecret(TWCryptoBoxSecretKeyCreate());
        TWCryptoBoxPublicKeyGuard myPublic(
            mySecret.get() ? TWCryptoBoxSecretKeyGetPublicKey(mySecret.get()) : nullptr);
        TWCryptoBoxPublicKeyGuard otherPublic(
            otherSecret.get() ? TWCryptoBoxSecretKeyGetPublicKey(otherSecret.get()) : nullptr);
        TWDataGuard importedSecret(
            mySecret.get() ? TWCryptoBoxSecretKeyData(mySecret.get()) : nullptr);
        TWDataGuard otherPublicData(
            otherPublic.get() ? TWCryptoBoxPublicKeyData(otherPublic.get()) : nullptr);

        const bool secretRoundTrip = importedSecret.get()
            && TWDataEqual(importedSecret.get(), knownSecretData.get());
        const bool publicValid = otherPublicData.get()
            && TWCryptoBoxPublicKeyIsValid(otherPublicData.get());

        TWDataGuard encrypted(
            mySecret.get() && otherPublic.get()
                ? TWCryptoBoxEncryptEasy(mySecret.get(), otherPublic.get(), messageData.get())
                : nullptr);
        TWDataGuard decrypted(
            encrypted.get() && otherSecret.get() && myPublic.get()
                ? TWCryptoBoxDecryptEasy(otherSecret.get(), myPublic.get(), encrypted.get())
                : nullptr);
        const bool decryptRoundTrip = decrypted.get()
            && TWDataEqual(decrypted.get(), messageData.get());

        cryptoBoxPassed = knownSecretValid
            && smallOrderRejected
            && mySecret.get()
            && otherSecret.get()
            && myPublic.get()
            && otherPublic.get()
            && secretRoundTrip
            && publicValid
            && decryptRoundTrip;
        cryptoBoxDetail = QStringLiteral("generated bridge=%1 · fixed-key round-trip=%2 · small-order key=%3 · encrypt/decrypt=%4")
                .arg(mySecret.get() && myPublic.get() && otherSecret.get() && otherPublic.get()
                         ? QStringLiteral("linked") : QStringLiteral("failed"))
                .arg(secretRoundTrip ? QStringLiteral("ok") : QStringLiteral("fail"))
                .arg(smallOrderRejected ? QStringLiteral("rejected") : QStringLiteral("accepted"))
                .arg(decryptRoundTrip ? QStringLiteral("ok") : QStringLiteral("fail"));
    }

    addResult(QStringLiteral("Rust/C++ CryptoBox bridge"),
              cryptoBoxPassed,
              cryptoBoxDetail,
              QStringLiteral("linked · fixed-key round-trip=ok · small-order key=rejected · encrypt/decrypt=ok"));

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
