#pragma once

#include <QObject>
#include <QString>

class WatchOnlyService : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString label READ label NOTIFY stateChanged)
    Q_PROPERTY(QString ethereumAddress READ ethereumAddress NOTIFY stateChanged)
    Q_PROPERTY(QString bitcoinAddress READ bitcoinAddress NOTIFY stateChanged)
    Q_PROPERTY(QString solanaAddress READ solanaAddress NOTIFY stateChanged)
    Q_PROPERTY(bool hasProfile READ hasProfile NOTIFY stateChanged)
    Q_PROPERTY(QString status READ status NOTIFY stateChanged)

public:
    explicit WatchOnlyService(QObject *parent = nullptr);

    QString label() const;
    QString ethereumAddress() const;
    QString bitcoinAddress() const;
    QString solanaAddress() const;
    bool hasProfile() const;
    QString status() const;

    Q_INVOKABLE void reload();

    Q_INVOKABLE bool saveProfile(const QString &label,
                                 const QString &ethereumAddress,
                                 const QString &bitcoinAddress,
                                 const QString &solanaAddress);

    Q_INVOKABLE void clearProfile();

    Q_INVOKABLE bool validateEthereum(const QString &address) const;
    Q_INVOKABLE bool validateBitcoin(const QString &address) const;
    Q_INVOKABLE bool validateSolana(const QString &address) const;

signals:
    void stateChanged();

private:
    static bool validateAddress(const QString &address, int coinType);

    QString m_label;
    QString m_ethereumAddress;
    QString m_bitcoinAddress;
    QString m_solanaAddress;
    QString m_status;
};
