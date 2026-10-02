#pragma once

#include <QObject>
#include <QString>
#include <QVariantMap>

class UnsignedTransactionService : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString chain READ chain NOTIFY stateChanged)
    Q_PROPERTY(QString network READ network NOTIFY stateChanged)
    Q_PROPERTY(QString symbol READ symbol NOTIFY stateChanged)
    Q_PROPERTY(int decimals READ decimals NOTIFY stateChanged)
    Q_PROPERTY(QString sourceAddress READ sourceAddress NOTIFY stateChanged)
    Q_PROPERTY(QString destinationAddress READ destinationAddress WRITE setDestinationAddress NOTIFY stateChanged)
    Q_PROPERTY(QString amountText READ amountText WRITE setAmountText NOTIFY stateChanged)
    Q_PROPERTY(QString normalizedAmount READ normalizedAmount NOTIFY stateChanged)
    Q_PROPERTY(QString availableBalance READ availableBalance NOTIFY stateChanged)
    Q_PROPERTY(bool destinationValid READ destinationValid NOTIFY stateChanged)
    Q_PROPERTY(bool amountValid READ amountValid NOTIFY stateChanged)
    Q_PROPERTY(bool balanceKnown READ balanceKnown NOTIFY stateChanged)
    Q_PROPERTY(bool amountWithinBalance READ amountWithinBalance NOTIFY stateChanged)
    Q_PROPERTY(bool readyForReview READ readyForReview NOTIFY stateChanged)
    Q_PROPERTY(bool selfTransfer READ selfTransfer NOTIFY stateChanged)
    Q_PROPERTY(QString status READ status NOTIFY stateChanged)
    Q_PROPERTY(QString balanceWarning READ balanceWarning NOTIFY stateChanged)

public:
    explicit UnsignedTransactionService(QObject *parent = nullptr);

    QString chain() const;
    QString network() const;
    QString symbol() const;
    int decimals() const;
    QString sourceAddress() const;
    QString destinationAddress() const;
    QString amountText() const;
    QString normalizedAmount() const;
    QString availableBalance() const;
    bool destinationValid() const;
    bool amountValid() const;
    bool balanceKnown() const;
    bool amountWithinBalance() const;
    bool readyForReview() const;
    bool selfTransfer() const;
    QString status() const;
    QString balanceWarning() const;

    void setDestinationAddress(const QString &address);
    void setAmountText(const QString &amount);

    Q_INVOKABLE void configure(const QString &chain,
                               const QString &sourceAddress,
                               const QString &availableBalance);
    Q_INVOKABLE QVariantMap reviewModel() const;
    Q_INVOKABLE QVariantMap runSelfTest() const;

    static bool validateReviewModel(const QVariantMap &model,
                                    QString *error = nullptr);

signals:
    void stateChanged();

private:
    struct ChainInfo {
        bool valid = false;
        QString chain;
        QString network;
        QString symbol;
        int decimals = 0;
        int coinType = -1;
    };

    struct DecimalValue {
        bool valid = false;
        bool zero = true;
        QString normalized;
        QString digits;
        int decimals = 0;
    };

    static ChainInfo chainInfo(const QString &chain);
    static bool validateAddress(const QString &address, int coinType);
    static DecimalValue parseDecimal(const QString &text, int maximumDecimals);
    static DecimalValue parseBalance(const QString &text,
                                     const QString &symbol,
                                     int maximumDecimals);
    static int compareDecimal(const DecimalValue &left,
                              const DecimalValue &right,
                              int scale);
    static QString canonicalAddress(const QString &chain,
                                    const QString &address);
    static QString fingerprintFor(const QVariantMap &model);
    void updateState();

    ChainInfo m_info;
    QString m_sourceAddress;
    QString m_destinationAddress;
    QString m_amountText;
    QString m_normalizedAmount;
    QString m_availableBalance;
    bool m_destinationValid = false;
    bool m_amountValid = false;
    bool m_balanceKnown = false;
    bool m_amountWithinBalance = false;
    bool m_readyForReview = false;
    bool m_selfTransfer = false;
    QString m_status;
    QString m_balanceWarning;
};
