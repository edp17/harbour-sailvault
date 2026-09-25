#include "addressbookservice.h"
#include "settingsstore.h"

#include <TrustWalletCore/TWAnyAddress.h>
#include <TrustWalletCore/TWCoinType.h>
#include <TrustWalletCore/TWString.h>

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

#include <algorithm>

namespace {

const char kContactsKey[] = "addressBook/contacts";

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

AddressBookService::AddressBookService(QObject *parent)
    : QObject(parent)
{
    loadEntries();
}

QVariantList AddressBookService::entries() const
{
    return m_entries;
}

int AddressBookService::count() const
{
    return m_entries.size();
}

QString AddressBookService::status() const
{
    return m_status;
}

QString AddressBookService::normalizedChain(const QString &chain)
{
    const QString value = chain.trimmed();

    if (value.compare(QStringLiteral("Ethereum"),
                      Qt::CaseInsensitive) == 0) {
        return QStringLiteral("Ethereum");
    }

    if (value.compare(QStringLiteral("Bitcoin"),
                      Qt::CaseInsensitive) == 0) {
        return QStringLiteral("Bitcoin");
    }

    if (value.compare(QStringLiteral("Solana"),
                      Qt::CaseInsensitive) == 0) {
        return QStringLiteral("Solana");
    }

    return QString();
}

QString AddressBookService::normalizedAddress(const QString &chain,
                                              const QString &address)
{
    const QString normalizedChainName = normalizedChain(chain);
    QString value = address.trimmed();

    if (normalizedChainName == QStringLiteral("Ethereum"))
        return value.toLower();

    return value;
}

int AddressBookService::coinTypeForChain(const QString &chain)
{
    const QString normalized = normalizedChain(chain);

    if (normalized == QStringLiteral("Ethereum"))
        return TWCoinTypeEthereum;

    if (normalized == QStringLiteral("Bitcoin"))
        return TWCoinTypeBitcoin;

    if (normalized == QStringLiteral("Solana"))
        return TWCoinTypeSolana;

    return -1;
}

bool AddressBookService::validateCoinAddress(const QString &address,
                                             int coinType)
{
    if (coinType < 0)
        return false;

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

bool AddressBookService::validateAddress(const QString &chain,
                                         const QString &address) const
{
    return validateCoinAddress(
        address,
        coinTypeForChain(chain));
}

void AddressBookService::loadEntries()
{
    m_entries.clear();

    const QString encoded =
        SailVaultSettings::value(
            QString::fromLatin1(kContactsKey),
            QStringLiteral("[]")).toString();

    QJsonParseError error;
    const QJsonDocument document =
        QJsonDocument::fromJson(encoded.toUtf8(), &error);

    if (error.error != QJsonParseError::NoError
            || !document.isArray()) {
        SailVaultSettings::quarantineValue(
            QString::fromLatin1(kContactsKey),
            QStringLiteral("Invalid Address book JSON"));
        SailVaultSettings::setValue(
            QString::fromLatin1(kContactsKey), QStringLiteral("[]"));
        m_status = QStringLiteral(
            "Address book storage was invalid; a local recovery copy was kept and the list was reset.");
        emit stateChanged();
        return;
    }

    const QJsonArray array = document.array();

    for (const QJsonValue &value : array) {
        if (!value.isObject())
            continue;

        const QJsonObject object = value.toObject();

        const QString label =
            object.value(QStringLiteral("label")).toString().trimmed();

        const QString chain =
            normalizedChain(
                object.value(QStringLiteral("chain")).toString());

        const QString address =
            object.value(QStringLiteral("address")).toString().trimmed();

        if (label.isEmpty()
                || chain.isEmpty()
                || address.isEmpty()
                || !validateAddress(chain, address)) {
            continue;
        }

        QVariantMap item;
        item.insert(QStringLiteral("label"), label);
        item.insert(QStringLiteral("chain"), chain);
        item.insert(QStringLiteral("address"), address);
        m_entries.append(item);
    }

    std::sort(
        m_entries.begin(),
        m_entries.end(),
        [](const QVariant &left, const QVariant &right) {
            const QVariantMap a = left.toMap();
            const QVariantMap b = right.toMap();

            const int labelCompare =
                a.value(QStringLiteral("label")).toString()
                    .compare(
                        b.value(QStringLiteral("label")).toString(),
                        Qt::CaseInsensitive);

            if (labelCompare != 0)
                return labelCompare < 0;

            return a.value(QStringLiteral("chain")).toString()
                < b.value(QStringLiteral("chain")).toString();
        });

    m_status = m_entries.isEmpty()
        ? QStringLiteral("Address book is empty")
        : QStringLiteral("%1 saved public address%2")
              .arg(m_entries.size())
              .arg(m_entries.size() == 1
                       ? QString()
                       : QStringLiteral("es"));

    emit stateChanged();
}

bool AddressBookService::persistEntries()
{
    QJsonArray array;

    for (const QVariant &value : m_entries) {
        const QVariantMap item = value.toMap();

        QJsonObject object;
        object.insert(
            QStringLiteral("label"),
            item.value(QStringLiteral("label")).toString());

        object.insert(
            QStringLiteral("chain"),
            item.value(QStringLiteral("chain")).toString());

        object.insert(
            QStringLiteral("address"),
            item.value(QStringLiteral("address")).toString());

        array.append(object);
    }

    const QString encoded =
        QString::fromUtf8(
            QJsonDocument(array).toJson(
                QJsonDocument::Compact));

    return SailVaultSettings::setValue(
        QString::fromLatin1(kContactsKey),
        encoded);
}

bool AddressBookService::saveContact(int index,
                                     const QString &label,
                                     const QString &chain,
                                     const QString &address)
{
    const QString normalizedChainName =
        normalizedChain(chain);

    const QString normalizedLabel =
        label.trimmed();

    const QString trimmedAddress =
        address.trimmed();

    if (normalizedLabel.isEmpty()) {
        m_status = QStringLiteral("Enter a contact label.");
        emit stateChanged();
        return false;
    }

    if (normalizedChainName.isEmpty()) {
        m_status = QStringLiteral("Choose a supported chain.");
        emit stateChanged();
        return false;
    }

    if (!validateAddress(normalizedChainName, trimmedAddress)) {
        m_status = QStringLiteral(
            "%1 address is not valid according to Wallet Core.")
            .arg(normalizedChainName);
        emit stateChanged();
        return false;
    }

    const QString identity =
        normalizedAddress(
            normalizedChainName,
            trimmedAddress);

    for (int i = 0; i < m_entries.size(); ++i) {
        if (i == index)
            continue;

        const QVariantMap existing =
            m_entries.at(i).toMap();

        if (existing.value(QStringLiteral("chain")).toString()
                == normalizedChainName
                && normalizedAddress(
                    normalizedChainName,
                    existing.value(
                        QStringLiteral("address")).toString())
                    == identity) {
            m_status = QStringLiteral(
                "That %1 address is already in the address book.")
                .arg(normalizedChainName);
            emit stateChanged();
            return false;
        }
    }

    QVariantMap item;
    item.insert(QStringLiteral("label"), normalizedLabel);
    item.insert(QStringLiteral("chain"), normalizedChainName);
    item.insert(QStringLiteral("address"), trimmedAddress);

    if (index >= 0 && index < m_entries.size())
        m_entries[index] = item;
    else
        m_entries.append(item);

    std::sort(
        m_entries.begin(),
        m_entries.end(),
        [](const QVariant &left, const QVariant &right) {
            const QVariantMap a = left.toMap();
            const QVariantMap b = right.toMap();

            const int labelCompare =
                a.value(QStringLiteral("label")).toString()
                    .compare(
                        b.value(QStringLiteral("label")).toString(),
                        Qt::CaseInsensitive);

            if (labelCompare != 0)
                return labelCompare < 0;

            return a.value(QStringLiteral("chain")).toString()
                < b.value(QStringLiteral("chain")).toString();
        });

    if (!persistEntries()) {
        m_status = QStringLiteral(
            "Failed to persist the address book.");
        emit stateChanged();
        return false;
    }

    m_status = QStringLiteral("Address book saved");
    emit stateChanged();
    return true;
}

void AddressBookService::removeContact(int index)
{
    if (index < 0 || index >= m_entries.size())
        return;

    m_entries.removeAt(index);

    if (!persistEntries()) {
        m_status = QStringLiteral(
            "Failed to persist the address book.");
        emit stateChanged();
        return;
    }

    m_status = m_entries.isEmpty()
        ? QStringLiteral("Address book is empty")
        : QStringLiteral("Address removed");

    emit stateChanged();
}

void AddressBookService::reload()
{
    loadEntries();
}
