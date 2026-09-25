#include "watchonlyservice.h"
#include "settingsstore.h"

#include <TrustWalletCore/TWAnyAddress.h>
#include <TrustWalletCore/TWCoinType.h>
#include <TrustWalletCore/TWString.h>

namespace {

const char kLabelKey[] = "watchOnly/label";
const char kEthereumKey[] = "watchOnly/ethereumAddress";
const char kBitcoinKey[] = "watchOnly/bitcoinAddress";
const char kSolanaKey[] = "watchOnly/solanaAddress";

class TWStringGuard
{
public:
    explicit TWStringGuard(TWString *value = nullptr)
        : m_value(value)
    {
    }

    ~TWStringGuard()
    {
        if (m_value)
            TWStringDelete(m_value);
    }

    TWString *get() const
    {
        return m_value;
    }

private:
    TWString *m_value;
};

} // namespace

WatchOnlyService::WatchOnlyService(QObject *parent)
    : QObject(parent)
{
    reload();
}

QString WatchOnlyService::label() const
{
    return m_label;
}

QString WatchOnlyService::ethereumAddress() const
{
    return m_ethereumAddress;
}

QString WatchOnlyService::bitcoinAddress() const
{
    return m_bitcoinAddress;
}

QString WatchOnlyService::solanaAddress() const
{
    return m_solanaAddress;
}

bool WatchOnlyService::hasProfile() const
{
    return !m_ethereumAddress.isEmpty()
        || !m_bitcoinAddress.isEmpty()
        || !m_solanaAddress.isEmpty();
}

QString WatchOnlyService::status() const
{
    return m_status;
}

bool WatchOnlyService::validateAddress(const QString &address, int coinType)
{
    const QString trimmed = address.trimmed();

    if (trimmed.isEmpty())
        return false;

    const QByteArray utf8 = trimmed.toUtf8();

    TWStringGuard string(
        TWStringCreateWithUTF8Bytes(utf8.constData()));

    if (!string.get())
        return false;

    return TWAnyAddressIsValid(
        string.get(),
        static_cast<TWCoinType>(coinType));
}

bool WatchOnlyService::validateEthereum(const QString &address) const
{
    return validateAddress(address, TWCoinTypeEthereum);
}

bool WatchOnlyService::validateBitcoin(const QString &address) const
{
    return validateAddress(address, TWCoinTypeBitcoin);
}

bool WatchOnlyService::validateSolana(const QString &address) const
{
    return validateAddress(address, TWCoinTypeSolana);
}

void WatchOnlyService::reload()
{
    m_label =
        SailVaultSettings::value(
            QString::fromLatin1(kLabelKey),
            QStringLiteral("Watch-only")).toString();

    m_ethereumAddress =
        SailVaultSettings::value(
            QString::fromLatin1(kEthereumKey),
            QString()).toString().trimmed();

    m_bitcoinAddress =
        SailVaultSettings::value(
            QString::fromLatin1(kBitcoinKey),
            QString()).toString().trimmed();

    m_solanaAddress =
        SailVaultSettings::value(
            QString::fromLatin1(kSolanaKey),
            QString()).toString().trimmed();

    bool recoveredInvalidValue = false;
    if (!m_ethereumAddress.isEmpty() && !validateEthereum(m_ethereumAddress)) {
        SailVaultSettings::quarantineValue(
            QString::fromLatin1(kEthereumKey),
            QStringLiteral("Invalid persisted watch-only Ethereum address"));
        m_ethereumAddress.clear();
        recoveredInvalidValue = true;
    }
    if (!m_bitcoinAddress.isEmpty() && !validateBitcoin(m_bitcoinAddress)) {
        SailVaultSettings::quarantineValue(
            QString::fromLatin1(kBitcoinKey),
            QStringLiteral("Invalid persisted watch-only Bitcoin address"));
        m_bitcoinAddress.clear();
        recoveredInvalidValue = true;
    }
    if (!m_solanaAddress.isEmpty() && !validateSolana(m_solanaAddress)) {
        SailVaultSettings::quarantineValue(
            QString::fromLatin1(kSolanaKey),
            QStringLiteral("Invalid persisted watch-only Solana address"));
        m_solanaAddress.clear();
        recoveredInvalidValue = true;
    }

    if (recoveredInvalidValue) {
        m_status = hasProfile()
            ? QStringLiteral("Watch-only profile loaded; invalid saved address data was quarantined")
            : QStringLiteral("Invalid saved watch-only address data was quarantined");
    } else {
        m_status = hasProfile()
            ? QStringLiteral("Watch-only profile loaded")
            : QStringLiteral("No watch-only profile saved");
    }

    emit stateChanged();
}

bool WatchOnlyService::saveProfile(const QString &label,
                                   const QString &ethereumAddress,
                                   const QString &bitcoinAddress,
                                   const QString &solanaAddress)
{
    const QString eth = ethereumAddress.trimmed();
    const QString btc = bitcoinAddress.trimmed();
    const QString sol = solanaAddress.trimmed();

    if (eth.isEmpty() && btc.isEmpty() && sol.isEmpty()) {
        m_status = QStringLiteral(
            "Enter at least one public Ethereum, Bitcoin or Solana address.");
        emit stateChanged();
        return false;
    }

    if (!eth.isEmpty() && !validateEthereum(eth)) {
        m_status = QStringLiteral(
            "Ethereum address is not valid according to Wallet Core.");
        emit stateChanged();
        return false;
    }

    if (!btc.isEmpty() && !validateBitcoin(btc)) {
        m_status = QStringLiteral(
            "Bitcoin address is not valid according to Wallet Core.");
        emit stateChanged();
        return false;
    }

    if (!sol.isEmpty() && !validateSolana(sol)) {
        m_status = QStringLiteral(
            "Solana address is not valid according to Wallet Core.");
        emit stateChanged();
        return false;
    }

    const QString storedLabel =
        label.trimmed().isEmpty()
            ? QStringLiteral("Watch-only")
            : label.trimmed();

    bool ok = true;

    ok = SailVaultSettings::setValue(
             QString::fromLatin1(kLabelKey),
             storedLabel) && ok;

    ok = SailVaultSettings::setValue(
             QString::fromLatin1(kEthereumKey),
             eth) && ok;

    ok = SailVaultSettings::setValue(
             QString::fromLatin1(kBitcoinKey),
             btc) && ok;

    ok = SailVaultSettings::setValue(
             QString::fromLatin1(kSolanaKey),
             sol) && ok;

    if (!ok) {
        m_status = QStringLiteral(
            "Failed to persist the watch-only profile.");
        emit stateChanged();
        return false;
    }

    m_label = storedLabel;
    m_ethereumAddress = eth;
    m_bitcoinAddress = btc;
    m_solanaAddress = sol;
    m_status = QStringLiteral("Watch-only profile saved");

    emit stateChanged();
    return true;
}

void WatchOnlyService::clearProfile()
{
    SailVaultSettings::remove(QStringList()
        << QString::fromLatin1(kLabelKey)
        << QString::fromLatin1(kEthereumKey)
        << QString::fromLatin1(kBitcoinKey)
        << QString::fromLatin1(kSolanaKey));

    m_label = QStringLiteral("Watch-only");
    m_ethereumAddress.clear();
    m_bitcoinAddress.clear();
    m_solanaAddress.clear();
    m_status = QStringLiteral("Watch-only profile cleared");

    emit stateChanged();
}
