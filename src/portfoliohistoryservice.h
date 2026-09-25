#pragma once

#include <QObject>
#include <QString>
#include <QVariant>
#include <QJsonArray>

class PortfolioHistoryService : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString fiatCurrency READ fiatCurrency WRITE setFiatCurrency NOTIFY stateChanged)
    Q_PROPERTY(QVariantList entries READ entries NOTIFY stateChanged)
    Q_PROPERTY(int count READ count NOTIFY stateChanged)
    Q_PROPERTY(QString status READ status NOTIFY stateChanged)
    Q_PROPERTY(double latestTotalValue READ latestTotalValue NOTIFY stateChanged)
    Q_PROPERTY(double oldestTotalValue READ oldestTotalValue NOTIFY stateChanged)
    Q_PROPERTY(double changeValue READ changeValue NOTIFY stateChanged)
    Q_PROPERTY(double changePercent READ changePercent NOTIFY stateChanged)
    Q_PROPERTY(QString latestTimestamp READ latestTimestamp NOTIFY stateChanged)
    Q_PROPERTY(double latestBitcoinValue READ latestBitcoinValue NOTIFY stateChanged)
    Q_PROPERTY(double latestEthereumValue READ latestEthereumValue NOTIFY stateChanged)
    Q_PROPERTY(double latestSolanaValue READ latestSolanaValue NOTIFY stateChanged)

    // M30 local analytics. These properties are derived exclusively from the
    // already stored local history and never trigger network access.
    Q_PROPERTY(int analysisWindowDays READ analysisWindowDays WRITE setAnalysisWindowDays NOTIFY stateChanged)
    Q_PROPERTY(QString analysisSeries READ analysisSeries WRITE setAnalysisSeries NOTIFY stateChanged)
    Q_PROPERTY(QVariantList analysisEntries READ analysisEntries NOTIFY stateChanged)
    Q_PROPERTY(int analysisCount READ analysisCount NOTIFY stateChanged)
    Q_PROPERTY(QString analysisStatus READ analysisStatus NOTIFY stateChanged)
    Q_PROPERTY(double analysisStartValue READ analysisStartValue NOTIFY stateChanged)
    Q_PROPERTY(double analysisEndValue READ analysisEndValue NOTIFY stateChanged)
    Q_PROPERTY(double analysisLowValue READ analysisLowValue NOTIFY stateChanged)
    Q_PROPERTY(double analysisHighValue READ analysisHighValue NOTIFY stateChanged)
    Q_PROPERTY(double analysisAverageValue READ analysisAverageValue NOTIFY stateChanged)
    Q_PROPERTY(double analysisChangeValue READ analysisChangeValue NOTIFY stateChanged)
    Q_PROPERTY(double analysisChangePercent READ analysisChangePercent NOTIFY stateChanged)
    Q_PROPERTY(QString analysisStartTimestamp READ analysisStartTimestamp NOTIFY stateChanged)
    Q_PROPERTY(QString analysisEndTimestamp READ analysisEndTimestamp NOTIFY stateChanged)

public:
    explicit PortfolioHistoryService(QObject *parent = nullptr);

    QString fiatCurrency() const;
    void setFiatCurrency(const QString &currency);
    QVariantList entries() const;
    int count() const;
    QString status() const;
    double latestTotalValue() const;
    double oldestTotalValue() const;
    double changeValue() const;
    double changePercent() const;
    QString latestTimestamp() const;
    double latestBitcoinValue() const;
    double latestEthereumValue() const;
    double latestSolanaValue() const;

    int analysisWindowDays() const;
    void setAnalysisWindowDays(int days);
    QString analysisSeries() const;
    void setAnalysisSeries(const QString &series);
    QVariantList analysisEntries() const;
    int analysisCount() const;
    QString analysisStatus() const;
    double analysisStartValue() const;
    double analysisEndValue() const;
    double analysisLowValue() const;
    double analysisHighValue() const;
    double analysisAverageValue() const;
    double analysisChangeValue() const;
    double analysisChangePercent() const;
    QString analysisStartTimestamp() const;
    QString analysisEndTimestamp() const;

    Q_INVOKABLE void setContext(const QString &ethereumAddress,
                                const QString &bitcoinAddress,
                                const QString &solanaAddress);
    Q_INVOKABLE bool recordSnapshot(const QString &ethereumBalance,
                                    const QString &bitcoinBalance,
                                    const QString &solanaBalance,
                                    double ethereumPrice,
                                    double bitcoinPrice,
                                    double solanaPrice,
                                    const QString &currency,
                                    const QString &balanceUpdated,
                                    const QString &priceUpdated);
    Q_INVOKABLE void clearHistory();

signals:
    void stateChanged();

private:
    static QString normalizedCurrency(const QString &currency);
    static QString normalizedAnalysisSeries(const QString &series);
    static QString contextId(const QString &ethereumAddress,
                             const QString &bitcoinAddress,
                             const QString &solanaAddress);
    static double numericBalance(const QString &text);
    double analysisValue(const QVariantMap &item) const;
    QString storageKey() const;
    void loadHistory();
    void saveHistory();
    void rebuildView();

    QString m_contextId;
    QString m_fiatCurrency = QStringLiteral("GBP");
    QJsonArray m_allEntries;
    QVariantList m_entries;
    QString m_status;
    double m_latestTotalValue = 0.0;
    double m_oldestTotalValue = 0.0;
    double m_changeValue = 0.0;
    double m_changePercent = 0.0;
    QString m_latestTimestamp;
    double m_latestBitcoinValue = 0.0;
    double m_latestEthereumValue = 0.0;
    double m_latestSolanaValue = 0.0;

    int m_analysisWindowDays = 0;
    QString m_analysisSeries = QStringLiteral("total");
    QVariantList m_analysisEntries;
    QString m_analysisStatus;
    double m_analysisStartValue = 0.0;
    double m_analysisEndValue = 0.0;
    double m_analysisLowValue = 0.0;
    double m_analysisHighValue = 0.0;
    double m_analysisAverageValue = 0.0;
    double m_analysisChangeValue = 0.0;
    double m_analysisChangePercent = 0.0;
    QString m_analysisStartTimestamp;
    QString m_analysisEndTimestamp;
};
