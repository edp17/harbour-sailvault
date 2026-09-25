#include "walletvault.h"

#include <QByteArray>
#include <QVector>

#include <Secrets/createcollectionrequest.h>
#include <Secrets/deletecollectionrequest.h>
#include <Secrets/findsecretsrequest.h>
#include <Secrets/result.h>
#include <Secrets/secret.h>
#include <Secrets/storesecretrequest.h>
#include <Secrets/storedsecretrequest.h>

#include <TrustWalletCore/TWCoinType.h>
#include <TrustWalletCore/TWCurve.h>
#include <TrustWalletCore/TWData.h>
#include <TrustWalletCore/TWHDWallet.h>
#include <TrustWalletCore/TWPrivateKey.h>
#include <TrustWalletCore/TWPublicKey.h>
#include <TrustWalletCore/TWString.h>

namespace {

// This development build accepts only this public BIP39 test vector. It must
// never be replaced with a personal recovery phrase during development.
const char kPublicTestMnemonic[] =
    "abandon abandon abandon abandon abandon abandon abandon abandon "
    "abandon abandon abandon about";

// SQLCipher-backed Sailfish Secrets collection names must be alphanumeric.
// This dedicated wallet collection intentionally does NOT reuse the early
// DeviceLockKeepUnlocked test collection.
const QString kWalletCollection = QStringLiteral("sailvaultwalletv1");
const QString kMnemonicSecretName = QStringLiteral("mnemonicv1");

const QString kExpectedEthereum =
    QStringLiteral("0x9858EfFD232B4033E47d90003D41EC34EcaEda94");
const QString kExpectedBitcoin =
    QStringLiteral("bc1qcr8te4kr609gcawutmrza0j4xv80jy8z306fyu");
const QString kExpectedSolana =
    QStringLiteral("GjJyeC1r2RgkuoCWMyPYkCWSGSGLcz266EaAkLA27AhL");

Sailfish::Secrets::Secret::Identifier walletSecretIdentifier()
{
    return Sailfish::Secrets::Secret::Identifier(
        kMnemonicSecretName,
        kWalletCollection,
        Sailfish::Secrets::SecretManager::DefaultEncryptedStoragePluginName);
}

bool missingSecretError(Sailfish::Secrets::Result::ErrorCode code)
{
    return code == Sailfish::Secrets::Result::InvalidCollectionError
        || code == Sailfish::Secrets::Result::InvalidSecretError
        || code == Sailfish::Secrets::Result::InvalidSecretIdentifierError;
}

bool interactionRequiredError(Sailfish::Secrets::Result::ErrorCode code)
{
    return code == Sailfish::Secrets::Result::OperationRequiresUserInteraction
        || code == Sailfish::Secrets::Result::OperationRequiresSystemUserInteraction
        || code == Sailfish::Secrets::Result::CollectionIsLockedError
        || code == Sailfish::Secrets::Result::SecretsPluginIsLockedError;
}

QString resultError(const Sailfish::Secrets::Result &result)
{
    return QStringLiteral("error %1: %2")
        .arg(static_cast<int>(result.errorCode()))
        .arg(result.errorMessage().isEmpty()
                 ? QStringLiteral("no error message returned")
                 : result.errorMessage());
}

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

WalletVault::WalletVault(QObject *parent)
    : QObject(parent)
    , m_backendReady(m_manager.isInitialized())
    , m_status(QStringLiteral("Waiting for Sailfish Secrets"))
    , m_detail(QStringLiteral(
          "Only the public development test wallet is accepted. Recovery material never enters normal QML/network services."))
{
    connect(&m_manager,
            &Sailfish::Secrets::SecretManager::isInitializedChanged,
            this,
            [this]() {
                m_backendReady = m_manager.isInitialized();
                if (m_backendReady) {
                    refreshStatus();
                } else {
                    m_storageKnown = false;
                    m_storageProtected = false;
                    m_status = QStringLiteral("Sailfish Secrets is not ready");
                    m_detail = QStringLiteral(
                        "The secure storage service has not initialized yet.");
                    emit stateChanged();
                }
            });
}

bool WalletVault::backendReady() const
{
    return m_backendReady;
}

bool WalletVault::storageKnown() const
{
    return m_storageKnown;
}

bool WalletVault::storageProtected() const
{
    return m_storageProtected;
}

bool WalletVault::walletStored() const
{
    return m_walletStored;
}

bool WalletVault::walletLoaded() const
{
    return m_walletLoaded;
}

bool WalletVault::lastOperationPassed() const
{
    return m_lastOperationPassed;
}

QString WalletVault::status() const
{
    return m_status;
}

QString WalletVault::detail() const
{
    return m_detail;
}

QString WalletVault::ethereumAddress() const
{
    return m_ethereumAddress;
}

QString WalletVault::bitcoinAddress() const
{
    return m_bitcoinAddress;
}

QString WalletVault::solanaAddress() const
{
    return m_solanaAddress;
}

int WalletVault::operationCount() const
{
    return m_operationCount;
}

QString WalletVault::developmentTestMnemonic() const
{
    return QString::fromLatin1(kPublicTestMnemonic);
}

void WalletVault::secureErase(QByteArray *bytes)
{
    if (!bytes || bytes->isEmpty())
        return;

    // Ensure this QByteArray owns its data before wiping it.
    bytes->detach();
    volatile char *p = bytes->data();
    for (int i = 0; i < bytes->size(); ++i)
        p[i] = 0;

    bytes->clear();
    bytes->squeeze();
}

void WalletVault::clearPublicSession()
{
    m_walletLoaded = false;
    m_ethereumAddress.clear();
    m_bitcoinAddress.clear();
    m_solanaAddress.clear();
}

void WalletVault::setResult(bool passed,
                            const QString &status,
                            const QString &detail)
{
    ++m_operationCount;
    m_lastOperationPassed = passed;
    m_status = status;
    m_detail = detail;
    emit stateChanged();
}

bool WalletVault::queryWalletPresence(bool allowInteraction,
                                      QString *errorMessage)
{
    if (!m_manager.isInitialized()) {
        m_backendReady = false;
        m_storageKnown = false;
        m_storageProtected = false;
        if (errorMessage) {
            *errorMessage = QStringLiteral(
                "Sailfish Secrets manager is not initialized yet.");
        }
        return false;
    }

    m_backendReady = true;

    Sailfish::Secrets::FindSecretsRequest request;
    request.setManager(&m_manager);
    request.setCollectionName(kWalletCollection);
    request.setStoragePluginName(
        Sailfish::Secrets::SecretManager::DefaultEncryptedStoragePluginName);
    request.setFilter(Sailfish::Secrets::Secret::FilterData());
    request.setFilterOperator(Sailfish::Secrets::SecretManager::OperatorAnd);
    request.setUserInteractionMode(
        allowInteraction
            ? Sailfish::Secrets::SecretManager::SystemInteraction
            : Sailfish::Secrets::SecretManager::PreventInteraction);
    request.startRequest();
    request.waitForFinished();

    const Sailfish::Secrets::Result result = request.result();

    if (result.code() == Sailfish::Secrets::Result::Succeeded) {
        m_storageKnown = true;
        m_storageProtected = false;
        m_walletStored = false;

        const QVector<Sailfish::Secrets::Secret::Identifier> ids =
            request.identifiers();
        for (const Sailfish::Secrets::Secret::Identifier &id : ids) {
            if (id.name() == kMnemonicSecretName) {
                m_walletStored = true;
                break;
            }
        }
        return true;
    }

    if (result.errorCode() == Sailfish::Secrets::Result::InvalidCollectionError) {
        m_storageKnown = true;
        m_storageProtected = false;
        m_walletStored = false;
        return true;
    }

    if (!allowInteraction && interactionRequiredError(result.errorCode())) {
        // DeviceLockRelock is intentional. A non-interactive status probe may
        // therefore be refused while secure storage is healthy and protected.
        // Keep that state distinct from an actual Secrets/backend failure so
        // release-readiness checks do not need to unlock the wallet.
        m_storageKnown = false;
        m_storageProtected = true;
        if (errorMessage) {
            *errorMessage = QStringLiteral(
                "Wallet storage is locked. Load the wallet to authenticate.");
        }
        return false;
    }

    m_storageKnown = false;
    m_storageProtected = false;
    if (errorMessage)
        *errorMessage = resultError(result);
    return false;
}

void WalletVault::refreshStatus()
{
    QString error;
    const bool queried = queryWalletPresence(false, &error);

    if (queried) {
        m_lastOperationPassed = true;
        m_status = m_walletStored
            ? QStringLiteral("Secure demo wallet is stored")
            : QStringLiteral("No secure demo wallet is stored");
        m_detail = m_walletStored
            ? QStringLiteral(
                  "Recovery material remains inside Sailfish Secrets until an explicit load operation.")
            : QStringLiteral(
                  "Create the public test wallet to exercise the secure development-wallet lifecycle.");
    } else {
        m_lastOperationPassed = false;
        m_status = m_backendReady && m_storageProtected
            ? QStringLiteral("Secure wallet is protected by device lock")
            : (m_backendReady
                ? QStringLiteral("Secure wallet status is unavailable")
                : QStringLiteral("Waiting for Sailfish Secrets"));
        m_detail = error;
    }

    emit stateChanged();
}

bool WalletVault::ensureWalletCollection(QString *errorMessage)
{
    if (!m_manager.isInitialized()) {
        if (errorMessage) {
            *errorMessage = QStringLiteral(
                "Sailfish Secrets manager is not initialized yet.");
        }
        return false;
    }

    Sailfish::Secrets::CreateCollectionRequest request;
    request.setManager(&m_manager);
    request.setCollectionLockType(
        Sailfish::Secrets::CreateCollectionRequest::DeviceLock);

    // Wallet recovery material should relock when the device locks.
    // Subsequent access is mediated by Sailfish system authentication.
    request.setDeviceLockUnlockSemantic(
        Sailfish::Secrets::SecretManager::DeviceLockRelock);
    request.setAccessControlMode(
        Sailfish::Secrets::SecretManager::OwnerOnlyMode);
    request.setUserInteractionMode(
        Sailfish::Secrets::SecretManager::SystemInteraction);
    request.setCollectionName(kWalletCollection);
    request.setStoragePluginName(
        Sailfish::Secrets::SecretManager::DefaultEncryptedStoragePluginName);
    request.setEncryptionPluginName(
        Sailfish::Secrets::SecretManager::DefaultEncryptedStoragePluginName);
    request.startRequest();
    request.waitForFinished();

    const Sailfish::Secrets::Result result = request.result();
    if (result.code() == Sailfish::Secrets::Result::Succeeded
            || result.errorCode()
               == Sailfish::Secrets::Result::CollectionAlreadyExistsError) {
        return true;
    }

    if (errorMessage)
        *errorMessage = resultError(result);
    return false;
}

WalletVault::SecretFetchState
WalletVault::fetchMnemonic(QByteArray *mnemonic, QString *errorMessage)
{
    if (mnemonic)
        mnemonic->clear();

    if (!m_manager.isInitialized()) {
        if (errorMessage) {
            *errorMessage = QStringLiteral(
                "Sailfish Secrets manager is not initialized yet.");
        }
        return SecretFetchState::Error;
    }

    Sailfish::Secrets::StoredSecretRequest request;
    request.setManager(&m_manager);
    request.setIdentifier(walletSecretIdentifier());
    request.setUserInteractionMode(
        Sailfish::Secrets::SecretManager::SystemInteraction);
    request.startRequest();
    request.waitForFinished();

    const Sailfish::Secrets::Result result = request.result();

    if (result.code() == Sailfish::Secrets::Result::Succeeded) {
        if (mnemonic)
            *mnemonic = request.secret().data();
        return SecretFetchState::Present;
    }

    if (missingSecretError(result.errorCode()))
        return SecretFetchState::Missing;

    if (errorMessage)
        *errorMessage = resultError(result);
    return SecretFetchState::Error;
}

WalletVault::DerivedWallet
WalletVault::deriveAndCheck(const QByteArray &mnemonic) const
{
    DerivedWallet result;

    if (mnemonic.isEmpty()) {
        result.error = QStringLiteral("Stored recovery material is empty.");
        return result;
    }

    TWStringGuard mnemonicString(
        TWStringCreateWithUTF8Bytes(mnemonic.constData()));
    TWStringGuard passphrase(TWStringCreateWithUTF8Bytes(""));

    if (!mnemonicString.get() || !passphrase.get()) {
        result.error = QStringLiteral(
            "Wallet Core could not create mnemonic/passphrase strings.");
        return result;
    }

    TWHDWalletGuard wallet(
        TWHDWalletCreateWithMnemonic(mnemonicString.get(), passphrase.get()));

    if (!wallet.get()) {
        result.error = QStringLiteral(
            "Wallet Core rejected the recovery material retrieved from Sailfish Secrets.");
        return result;
    }

    TWStringGuard ethAddress(
        TWHDWalletGetAddressForCoin(wallet.get(), TWCoinTypeEthereum));
    TWStringGuard btcAddress(
        TWHDWalletGetAddressForCoin(wallet.get(), TWCoinTypeBitcoin));
    TWStringGuard solAddress(
        TWHDWalletGetAddressForCoin(wallet.get(), TWCoinTypeSolana));

    result.ethereumAddress = fromTWString(ethAddress.get());
    result.bitcoinAddress = fromTWString(btcAddress.get());
    result.solanaAddress = fromTWString(solAddress.get());

    if (result.ethereumAddress != kExpectedEthereum
            || result.bitcoinAddress != kExpectedBitcoin
            || result.solanaAddress != kExpectedSolana) {
        result.error = QStringLiteral(
            "Stored wallet does not match the published development test wallet. "
            "SailVault refuses to expose or overwrite unexpected recovery material.");
        return result;
    }

    const uint8_t digestBytes[32] = {
        0x3F, 0x89, 0x1F, 0xDA, 0x37, 0x04, 0xF0, 0x36,
        0x8D, 0xAB, 0x65, 0xFA, 0x81, 0xEB, 0xE6, 0x16,
        0xF4, 0xAA, 0x2A, 0x08, 0x54, 0x99, 0x5D, 0xA4,
        0xDC, 0x0B, 0x59, 0xD2, 0xCA, 0xDB, 0xD6, 0x4F
    };

    TWPrivateKeyGuard key(
        TWHDWalletGetKeyForCoin(wallet.get(), TWCoinTypeEthereum));
    TWDataGuard digest(
        TWDataCreateWithBytes(digestBytes, sizeof(digestBytes)));

    if (!key.get() || !digest.get()) {
        result.error = QStringLiteral(
            "Could not derive the internal Ethereum key/signing digest.");
        return result;
    }

    TWDataGuard signature(
        TWPrivateKeySign(key.get(), digest.get(), TWCurveSECP256k1));
    TWPublicKeyGuard publicKey(
        TWPrivateKeyGetPublicKeySecp256k1(key.get(), false));

    if (!signature.get() || !publicKey.get()) {
        result.error = QStringLiteral(
            "Wallet Core did not return a signature/public key.");
        return result;
    }

    result.signingOk =
        TWDataSize(signature.get()) == 65
        && TWPublicKeyVerify(publicKey.get(), signature.get(), digest.get());

    if (!result.signingOk) {
        result.error = QStringLiteral(
            "Wallet loaded from Secrets, but its secp256k1 signing self-check failed.");
        return result;
    }

    result.ok = true;
    return result;
}

bool WalletVault::storeDevelopmentMnemonic(const QByteArray &mnemonic,
                                                 const QString &successStatus,
                                                 QString *errorMessage)
{
    const DerivedWallet preflight = deriveAndCheck(mnemonic);
    if (!preflight.ok) {
        if (errorMessage)
            *errorMessage = preflight.error;
        return false;
    }

    Sailfish::Secrets::Secret secret(walletSecretIdentifier());
    secret.setType(Sailfish::Secrets::Secret::TypeBlob);
    secret.setData(mnemonic);

    Sailfish::Secrets::StoreSecretRequest request;
    request.setManager(&m_manager);
    request.setSecretStorageType(
        Sailfish::Secrets::StoreSecretRequest::CollectionSecret);
    request.setUserInteractionMode(
        Sailfish::Secrets::SecretManager::SystemInteraction);
    request.setSecret(secret);
    request.startRequest();
    request.waitForFinished();

    const Sailfish::Secrets::Result storeResult = request.result();
    if (storeResult.code() != Sailfish::Secrets::Result::Succeeded) {
        if (errorMessage)
            *errorMessage = resultError(storeResult);
        return false;
    }

    QByteArray storedMnemonic;
    QString fetchError;
    const SecretFetchState stored =
        fetchMnemonic(&storedMnemonic, &fetchError);

    if (stored != SecretFetchState::Present) {
        secureErase(&storedMnemonic);
        if (errorMessage) {
            *errorMessage = fetchError.isEmpty()
                ? QStringLiteral("The wallet was not found after storage.")
                : fetchError;
        }
        return false;
    }

    const DerivedWallet verified = deriveAndCheck(storedMnemonic);
    secureErase(&storedMnemonic);

    if (!verified.ok) {
        if (errorMessage)
            *errorMessage = verified.error;
        return false;
    }

    m_storageKnown = true;
    m_storageProtected = false;
    m_walletStored = true;
    m_walletLoaded = true;
    m_ethereumAddress = verified.ethereumAddress;
    m_bitcoinAddress = verified.bitcoinAddress;
    m_solanaAddress = verified.solanaAddress;

    setResult(true,
              successStatus,
              QStringLiteral(
                  "Recovery material was stored in the relocking Sailfish Secrets wallet collection, "
                  "read back, used by Wallet Core, and wiped from the temporary application buffer."));
    return true;
}

void WalletVault::createDemoWallet()
{
    clearPublicSession();
    m_storageProtected = false;

    QString error;
    if (!ensureWalletCollection(&error)) {
        m_storageKnown = false;
        m_storageProtected = false;
        setResult(false,
                  QStringLiteral("FAIL · Secure wallet collection unavailable"),
                  error);
        return;
    }

    QByteArray existingMnemonic;
    const SecretFetchState existing =
        fetchMnemonic(&existingMnemonic, &error);

    if (existing == SecretFetchState::Present) {
        const DerivedWallet derived = deriveAndCheck(existingMnemonic);
        secureErase(&existingMnemonic);

        m_storageKnown = true;
        m_storageProtected = false;
        m_walletStored = true;

        if (!derived.ok) {
            setResult(false,
                      QStringLiteral("FAIL · Refusing to overwrite stored wallet"),
                      derived.error);
            return;
        }

        m_walletLoaded = true;
        m_ethereumAddress = derived.ethereumAddress;
        m_bitcoinAddress = derived.bitcoinAddress;
        m_solanaAddress = derived.solanaAddress;
        setResult(true,
                  QStringLiteral("PASS · Demo wallet already exists and was verified"),
                  QStringLiteral(
                      "Existing secure wallet matches the public test vector; no overwrite was performed."));
        return;
    }

    secureErase(&existingMnemonic);

    if (existing == SecretFetchState::Error) {
        m_storageKnown = false;
        m_storageProtected = false;
        setResult(false,
                  QStringLiteral("FAIL · Could not inspect wallet storage"),
                  error);
        return;
    }

    QByteArray testMnemonic(kPublicTestMnemonic);
    error.clear();
    const bool stored = storeDevelopmentMnemonic(
        testMnemonic,
        QStringLiteral("PASS · Secure demo wallet created"),
        &error);
    secureErase(&testMnemonic);

    if (!stored) {
        m_storageKnown = false;
        m_storageProtected = false;
        setResult(false,
                  QStringLiteral("FAIL · Could not create secure demo wallet"),
                  error);
    }
}

void WalletVault::restoreDevelopmentWallet(const QString &mnemonic)
{
    clearPublicSession();
    m_storageProtected = false;

    // The development build deliberately refuses every recovery phrase except
    // the published BIP39 test vector. This prevents accidental use of personal
    // wallet material while we exercise the restore UX.
    const QString normalized = mnemonic.simplified();
    const QString expected = QString::fromLatin1(kPublicTestMnemonic);

    if (normalized != expected) {
        setResult(false,
                  QStringLiteral("REFUSED · Development build accepts only the public test phrase"),
                  QStringLiteral(
                      "No data was written to Sailfish Secrets. "
                      "Do not enter a personal recovery phrase in this build."));
        return;
    }

    QString error;
    if (!ensureWalletCollection(&error)) {
        m_storageKnown = false;
        m_storageProtected = false;
        setResult(false,
                  QStringLiteral("FAIL · Secure wallet collection unavailable"),
                  error);
        return;
    }

    QByteArray existingMnemonic;
    const SecretFetchState existing =
        fetchMnemonic(&existingMnemonic, &error);

    if (existing == SecretFetchState::Present) {
        const DerivedWallet existingWallet = deriveAndCheck(existingMnemonic);
        secureErase(&existingMnemonic);

        m_storageKnown = true;
        m_storageProtected = false;
        m_walletStored = true;

        if (!existingWallet.ok) {
            setResult(false,
                      QStringLiteral("FAIL · Refusing to overwrite stored wallet"),
                      existingWallet.error);
            return;
        }

        m_walletLoaded = true;
        m_ethereumAddress = existingWallet.ethereumAddress;
        m_bitcoinAddress = existingWallet.bitcoinAddress;
        m_solanaAddress = existingWallet.solanaAddress;
        setResult(true,
                  QStringLiteral("PASS · Existing development wallet restored"),
                  QStringLiteral(
                      "The stored wallet already matches the public test phrase; no overwrite was required."));
        return;
    }

    secureErase(&existingMnemonic);

    if (existing == SecretFetchState::Error) {
        m_storageKnown = false;
        m_storageProtected = false;
        setResult(false,
                  QStringLiteral("FAIL · Could not inspect wallet storage"),
                  error);
        return;
    }

    QByteArray recovery = normalized.toUtf8();
    error.clear();
    const bool stored = storeDevelopmentMnemonic(
        recovery,
        QStringLiteral("PASS · Development wallet restored securely"),
        &error);
    secureErase(&recovery);

    if (!stored) {
        m_storageKnown = false;
        m_storageProtected = false;
        setResult(false,
                  QStringLiteral("FAIL · Development restore failed"),
                  error);
    }
}

void WalletVault::loadStoredWallet()
{
    clearPublicSession();
    m_storageProtected = false;

    QByteArray mnemonic;
    QString error;
    const SecretFetchState fetched = fetchMnemonic(&mnemonic, &error);

    if (fetched == SecretFetchState::Missing) {
        secureErase(&mnemonic);
        m_storageKnown = true;
        m_storageProtected = false;
        m_walletStored = false;
        setResult(false,
                  QStringLiteral("No secure demo wallet is stored"),
                  QStringLiteral(
                      "Create the public test wallet first."));
        return;
    }

    if (fetched == SecretFetchState::Error) {
        secureErase(&mnemonic);
        m_storageKnown = false;
        m_storageProtected = false;
        setResult(false,
                  QStringLiteral("FAIL · Could not load secure wallet"),
                  error);
        return;
    }

    const DerivedWallet derived = deriveAndCheck(mnemonic);
    secureErase(&mnemonic);

    m_storageKnown = true;
    m_storageProtected = false;
    m_walletStored = true;

    if (!derived.ok) {
        setResult(false,
                  QStringLiteral("FAIL · Stored wallet validation failed"),
                  derived.error);
        return;
    }

    m_walletLoaded = true;
    m_ethereumAddress = derived.ethereumAddress;
    m_bitcoinAddress = derived.bitcoinAddress;
    m_solanaAddress = derived.solanaAddress;

    setResult(true,
              QStringLiteral("PASS · Wallet loaded from Sailfish Secrets"),
              QStringLiteral(
                  "Wallet Core derived all three expected addresses and completed an internal secp256k1 sign/verify test. "
                  "The recovery bytes were then wiped from the temporary application buffer."));
}

void WalletVault::clearSession()
{
    clearPublicSession();

    setResult(true,
              QStringLiteral("PASS · Wallet session cleared"),
              QStringLiteral(
                  "Public derived addresses were cleared from the application state. "
                  "The encrypted wallet remains stored in Sailfish Secrets."));
}

void WalletVault::lockSession(const QString &reason)
{
    if (!m_walletLoaded)
        return;

    clearPublicSession();

    setResult(true,
              QStringLiteral("Wallet locked automatically"),
              reason.isEmpty()
                  ? QStringLiteral(
                        "The public wallet session was cleared automatically.")
                  : reason);
}

void WalletVault::deleteDemoWallet()
{
    clearPublicSession();
    m_storageProtected = false;

    if (!m_manager.isInitialized()) {
        m_storageKnown = false;
        m_storageProtected = false;
        setResult(false,
                  QStringLiteral("FAIL · Sailfish Secrets unavailable"),
                  QStringLiteral(
                      "The secure storage service is not initialized yet."));
        return;
    }

    Sailfish::Secrets::DeleteCollectionRequest request;
    request.setManager(&m_manager);
    request.setCollectionName(kWalletCollection);
    request.setStoragePluginName(
        Sailfish::Secrets::SecretManager::DefaultEncryptedStoragePluginName);
    request.setUserInteractionMode(
        Sailfish::Secrets::SecretManager::SystemInteraction);
    request.startRequest();
    request.waitForFinished();

    const Sailfish::Secrets::Result result = request.result();

    if (result.code() == Sailfish::Secrets::Result::Succeeded
            || result.errorCode()
               == Sailfish::Secrets::Result::InvalidCollectionError) {
        m_storageKnown = true;
        m_storageProtected = false;
        m_walletStored = false;
        setResult(true,
                  QStringLiteral("PASS · Secure demo wallet deleted"),
                  QStringLiteral(
                      "The dedicated encrypted wallet collection and its recovery material are no longer present."));
        return;
    }

    m_storageKnown = false;
    m_storageProtected = false;
    setResult(false,
              QStringLiteral("FAIL · Could not delete secure wallet"),
              resultError(result));
}
