#pragma once

#include <QObject>
#include <QString>
#include <QVariantList>

class AddressBookService : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QVariantList entries READ entries NOTIFY stateChanged)
    Q_PROPERTY(int count READ count NOTIFY stateChanged)
    Q_PROPERTY(QString status READ status NOTIFY stateChanged)

public:
    explicit AddressBookService(QObject *parent = nullptr);

    QVariantList entries() const;
    int count() const;
    QString status() const;

    Q_INVOKABLE bool validateAddress(const QString &chain,
                                     const QString &address) const;

    Q_INVOKABLE bool saveContact(int index,
                                 const QString &label,
                                 const QString &chain,
                                 const QString &address);

    Q_INVOKABLE void removeContact(int index);
    Q_INVOKABLE void reload();

signals:
    void stateChanged();

private:
    static bool validateCoinAddress(const QString &address,
                                    int coinType);
    static int coinTypeForChain(const QString &chain);
    static QString normalizedChain(const QString &chain);
    static QString normalizedAddress(const QString &chain,
                                     const QString &address);

    void loadEntries();
    bool persistEntries();

    QVariantList m_entries;
    QString m_status;
};
