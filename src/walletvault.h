#pragma once

#include <QObject>
#include <QString>

#include <Secrets/secretmanager.h>

class WalletVault : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool backendReady READ backendReady NOTIFY stateChanged)
    Q_PROPERTY(bool storageKnown READ storageKnown NOTIFY stateChanged)
    Q_PROPERTY(bool walletStored READ walletStored NOTIFY stateChanged)
    Q_PROPERTY(bool walletLoaded READ walletLoaded NOTIFY stateChanged)
    Q_PROPERTY(bool lastOperationPassed READ lastOperationPassed NOTIFY stateChanged)
    Q_PROPERTY(QString status READ status NOTIFY stateChanged)
    Q_PROPERTY(QString detail READ detail NOTIFY stateChanged)
    Q_PROPERTY(QString ethereumAddress READ ethereumAddress NOTIFY stateChanged)
    Q_PROPERTY(QString bitcoinAddress READ bitcoinAddress NOTIFY stateChanged)
    Q_PROPERTY(int operationCount READ operationCount NOTIFY stateChanged)

public:
    explicit WalletVault(QObject *parent = nullptr);

    bool backendReady() const;
    bool storageKnown() const;
    bool walletStored() const;
    bool walletLoaded() const;
    bool lastOperationPassed() const;
    QString status() const;
    QString detail() const;
    QString ethereumAddress() const;
    QString bitcoinAddress() const;
    int operationCount() const;

    Q_INVOKABLE void refreshStatus();
    Q_INVOKABLE void createDemoWallet();
    Q_INVOKABLE void loadStoredWallet();
    Q_INVOKABLE void clearSession();
    Q_INVOKABLE void deleteDemoWallet();

signals:
    void stateChanged();

private:
    struct DerivedWallet {
        bool ok = false;
        bool signingOk = false;
        QString ethereumAddress;
        QString bitcoinAddress;
        QString error;
    };

    enum class SecretFetchState {
        Present,
        Missing,
        Error
    };

    bool ensureWalletCollection(QString *errorMessage);
    SecretFetchState fetchMnemonic(QByteArray *mnemonic, QString *errorMessage);
    DerivedWallet deriveAndCheck(const QByteArray &mnemonic) const;
    bool queryWalletPresence(bool allowInteraction, QString *errorMessage);
    void setResult(bool passed, const QString &status, const QString &detail);
    void clearPublicSession();
    static void secureErase(QByteArray *bytes);

    Sailfish::Secrets::SecretManager m_manager;
    bool m_backendReady = false;
    bool m_storageKnown = false;
    bool m_walletStored = false;
    bool m_walletLoaded = false;
    bool m_lastOperationPassed = false;
    QString m_status;
    QString m_detail;
    QString m_ethereumAddress;
    QString m_bitcoinAddress;
    int m_operationCount = 0;
};
