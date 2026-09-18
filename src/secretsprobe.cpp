#include "secretsprobe.h"

#include <Secrets/createcollectionrequest.h>
#include <Secrets/deletesecretrequest.h>
#include <Secrets/result.h>
#include <Secrets/secret.h>
#include <Secrets/storesecretrequest.h>
#include <Secrets/storedsecretrequest.h>

namespace {

const char kPublicTestMnemonic[] =
    "abandon abandon abandon abandon abandon abandon abandon abandon "
    "abandon abandon abandon about";

// SQLCipher-backed Sailfish Secrets collections accept only alphanumeric
// Latin-1 collection names. Keep this identifier punctuation-free.
const QString kCollectionName = QStringLiteral("harboursailvault");
const QString kSecretName = QStringLiteral("m2-public-bip39-test-vector");

Sailfish::Secrets::Secret::Identifier testIdentifier()
{
    return Sailfish::Secrets::Secret::Identifier(
        kSecretName,
        kCollectionName,
        Sailfish::Secrets::SecretManager::DefaultEncryptedStoragePluginName);
}

bool isMissingError(Sailfish::Secrets::Result::ErrorCode code)
{
    return code == Sailfish::Secrets::Result::InvalidCollectionError
        || code == Sailfish::Secrets::Result::InvalidSecretError
        || code == Sailfish::Secrets::Result::InvalidSecretIdentifierError;
}

QString requestError(const Sailfish::Secrets::Result &result)
{
    return QStringLiteral("error %1: %2")
        .arg(static_cast<int>(result.errorCode()))
        .arg(result.errorMessage().isEmpty()
                 ? QStringLiteral("no error message returned")
                 : result.errorMessage());
}

} // namespace

SecretsProbe::SecretsProbe(QObject *parent)
    : QObject(parent)
    , m_status(QStringLiteral("Not tested yet"))
    , m_detail(QStringLiteral(
          "Uses Sailfish Secrets encrypted storage. Only the public BIP39 test vector is used."))
{
}

QString SecretsProbe::status() const
{
    return m_status;
}

QString SecretsProbe::detail() const
{
    return m_detail;
}

bool SecretsProbe::lastOperationPassed() const
{
    return m_lastOperationPassed;
}

bool SecretsProbe::testSecretPresent() const
{
    return m_testSecretPresent;
}

int SecretsProbe::operationCount() const
{
    return m_operationCount;
}

void SecretsProbe::setResult(bool passed,
                             bool present,
                             const QString &status,
                             const QString &detail)
{
    ++m_operationCount;
    m_lastOperationPassed = passed;
    m_testSecretPresent = present;
    m_status = status;
    m_detail = detail;
    emit stateChanged();
}

bool SecretsProbe::ensureCollection(QString *errorMessage)
{
    if (!m_manager.isInitialized()) {
        if (errorMessage) {
            *errorMessage = QStringLiteral(
                "Sailfish Secrets manager is not initialized. Try the operation again.");
        }
        return false;
    }

    Sailfish::Secrets::CreateCollectionRequest request;
    request.setManager(&m_manager);
    request.setCollectionLockType(
        Sailfish::Secrets::CreateCollectionRequest::DeviceLock);
    request.setDeviceLockUnlockSemantic(
        Sailfish::Secrets::SecretManager::DeviceLockKeepUnlocked);
    request.setAccessControlMode(
        Sailfish::Secrets::SecretManager::OwnerOnlyMode);
    request.setUserInteractionMode(
        Sailfish::Secrets::SecretManager::SystemInteraction);
    request.setCollectionName(kCollectionName);
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
        *errorMessage = requestError(result);
    return false;
}

SecretsProbe::FetchState SecretsProbe::fetchTestSecret(QString *errorMessage)
{
    if (!m_manager.isInitialized()) {
        if (errorMessage) {
            *errorMessage = QStringLiteral(
                "Sailfish Secrets manager is not initialized. Try the operation again.");
        }
        return FetchState::Error;
    }

    Sailfish::Secrets::StoredSecretRequest request;
    request.setManager(&m_manager);
    request.setUserInteractionMode(
        Sailfish::Secrets::SecretManager::SystemInteraction);
    request.setIdentifier(testIdentifier());
    request.startRequest();
    request.waitForFinished();

    const Sailfish::Secrets::Result result = request.result();
    if (result.code() == Sailfish::Secrets::Result::Succeeded) {
        const QByteArray expected(kPublicTestMnemonic);
        return request.secret().data() == expected
            ? FetchState::PresentAndMatches
            : FetchState::PresentButDifferent;
    }

    if (isMissingError(result.errorCode()))
        return FetchState::Missing;

    if (errorMessage)
        *errorMessage = requestError(result);
    return FetchState::Error;
}

void SecretsProbe::storeTestSecret()
{
    QString error;
    if (!ensureCollection(&error)) {
        setResult(false, false,
                  QStringLiteral("FAIL · Secure collection unavailable"),
                  error);
        return;
    }

    const FetchState existing = fetchTestSecret(&error);
    if (existing == FetchState::PresentAndMatches) {
        setResult(true, true,
                  QStringLiteral("PASS · Public test secret is already stored"),
                  QStringLiteral(
                      "Existing value was verified internally; recovery text was not exposed to QML."));
        return;
    }
    if (existing == FetchState::PresentButDifferent) {
        setResult(false, true,
                  QStringLiteral("FAIL · Refusing to overwrite unexpected secret data"),
                  QStringLiteral(
                      "The M2 test identifier already exists but does not contain the expected public test vector."));
        return;
    }
    if (existing == FetchState::Error) {
        setResult(false, false,
                  QStringLiteral("FAIL · Could not inspect secure storage"),
                  error);
        return;
    }

    Sailfish::Secrets::Secret secret(testIdentifier());
    secret.setType(Sailfish::Secrets::Secret::TypeBlob);
    secret.setData(QByteArray(kPublicTestMnemonic));

    Sailfish::Secrets::StoreSecretRequest request;
    request.setManager(&m_manager);
    request.setSecretStorageType(
        Sailfish::Secrets::StoreSecretRequest::CollectionSecret);
    request.setUserInteractionMode(
        Sailfish::Secrets::SecretManager::SystemInteraction);
    request.setSecret(secret);
    request.startRequest();
    request.waitForFinished();

    const Sailfish::Secrets::Result result = request.result();
    if (result.code() != Sailfish::Secrets::Result::Succeeded) {
        setResult(false, false,
                  QStringLiteral("FAIL · Secure store failed"),
                  requestError(result));
        return;
    }

    error.clear();
    const FetchState verification = fetchTestSecret(&error);
    if (verification == FetchState::PresentAndMatches) {
        setResult(true, true,
                  QStringLiteral("PASS · Public test secret stored securely"),
                  QStringLiteral(
                      "Stored in an owner-only, device-lock-protected encrypted Sailfish Secrets collection."));
    } else if (verification == FetchState::PresentButDifferent) {
        setResult(false, true,
                  QStringLiteral("FAIL · Stored data did not verify"),
                  QStringLiteral(
                      "Secure storage returned data different from the public test vector."));
    } else {
        setResult(false, false,
                  QStringLiteral("FAIL · Could not verify stored test secret"),
                  error.isEmpty()
                      ? QStringLiteral("Secret was not found after the store request.")
                      : error);
    }
}

void SecretsProbe::verifyTestSecret()
{
    QString error;
    const FetchState state = fetchTestSecret(&error);

    switch (state) {
    case FetchState::PresentAndMatches:
        setResult(true, true,
                  QStringLiteral("PASS · Stored public test secret verified"),
                  QStringLiteral(
                      "The value persisted in Sailfish Secrets and matches internally. No recovery text was returned to QML."));
        break;
    case FetchState::PresentButDifferent:
        setResult(false, true,
                  QStringLiteral("FAIL · Stored secret does not match"),
                  QStringLiteral(
                      "The M2 test identifier exists, but its data differs from the expected public test vector."));
        break;
    case FetchState::Missing:
        setResult(false, false,
                  QStringLiteral("No M2 test secret is currently stored"),
                  QStringLiteral(
                      "Press “Store public test secret”, then verify it. You may restart the app between those steps."));
        break;
    case FetchState::Error:
        setResult(false, false,
                  QStringLiteral("FAIL · Secure retrieval failed"),
                  error);
        break;
    }
}

void SecretsProbe::deleteTestSecret()
{
    if (!m_manager.isInitialized()) {
        setResult(false, false,
                  QStringLiteral("FAIL · Sailfish Secrets unavailable"),
                  QStringLiteral(
                      "Sailfish Secrets manager is not initialized. Try the operation again."));
        return;
    }

    Sailfish::Secrets::DeleteSecretRequest request;
    request.setManager(&m_manager);
    request.setUserInteractionMode(
        Sailfish::Secrets::SecretManager::SystemInteraction);
    request.setIdentifier(testIdentifier());
    request.startRequest();
    request.waitForFinished();

    const Sailfish::Secrets::Result result = request.result();
    if (result.code() == Sailfish::Secrets::Result::Succeeded) {
        QString error;
        const FetchState verification = fetchTestSecret(&error);
        if (verification == FetchState::Missing) {
            setResult(true, false,
                      QStringLiteral("PASS · Public test secret deleted"),
                      QStringLiteral(
                          "The encrypted collection remains available for later M2 tests, but the test secret is gone."));
        } else {
            setResult(false, verification != FetchState::Missing,
                      QStringLiteral("FAIL · Delete could not be verified"),
                      error.isEmpty()
                          ? QStringLiteral("The secret remained accessible after deletion.")
                          : error);
        }
        return;
    }

    if (isMissingError(result.errorCode())) {
        setResult(true, false,
                  QStringLiteral("PASS · Public test secret is already absent"),
                  QStringLiteral(
                      "There is no M2 test recovery material stored under the SailVault test identifier."));
        return;
    }

    setResult(false, m_testSecretPresent,
              QStringLiteral("FAIL · Secure delete failed"),
              requestError(result));
}
