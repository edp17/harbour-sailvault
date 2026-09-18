#pragma once

#include <QObject>
#include <QString>

#include <Secrets/secretmanager.h>

class SecretsProbe : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString status READ status NOTIFY stateChanged)
    Q_PROPERTY(QString detail READ detail NOTIFY stateChanged)
    Q_PROPERTY(bool lastOperationPassed READ lastOperationPassed NOTIFY stateChanged)
    Q_PROPERTY(bool testSecretPresent READ testSecretPresent NOTIFY stateChanged)
    Q_PROPERTY(int operationCount READ operationCount NOTIFY stateChanged)

public:
    explicit SecretsProbe(QObject *parent = nullptr);

    QString status() const;
    QString detail() const;
    bool lastOperationPassed() const;
    bool testSecretPresent() const;
    int operationCount() const;

    Q_INVOKABLE void storeTestSecret();
    Q_INVOKABLE void verifyTestSecret();
    Q_INVOKABLE void deleteTestSecret();

signals:
    void stateChanged();

private:
    enum class FetchState {
        PresentAndMatches,
        PresentButDifferent,
        Missing,
        Error
    };

    bool ensureCollection(QString *errorMessage);
    FetchState fetchTestSecret(QString *errorMessage);
    void setResult(bool passed,
                   bool present,
                   const QString &status,
                   const QString &detail);

    Sailfish::Secrets::SecretManager m_manager;
    QString m_status;
    QString m_detail;
    bool m_lastOperationPassed = false;
    bool m_testSecretPresent = false;
    int m_operationCount = 0;
};
