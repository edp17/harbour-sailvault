#pragma once

#include <QObject>
#include <QString>
#include <QNetworkAccessManager>

class QNetworkReply;

class PriceService : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool loading READ loading NOTIFY stateChanged)
    Q_PROPERTY(bool lastRefreshPassed READ lastRefreshPassed NOTIFY stateChanged)
    Q_PROPERTY(QString status READ status NOTIFY stateChanged)
    Q_PROPERTY(QString fiatCurrency READ fiatCurrency WRITE setFiatCurrency NOTIFY settingsChanged)
    Q_PROPERTY(QString fiatSymbol READ fiatSymbol NOTIFY settingsChanged)
    Q_PROPERTY(double bitcoinPriceValue READ bitcoinPriceValue NOTIFY stateChanged)
    Q_PROPERTY(double ethereumPriceValue READ ethereumPriceValue NOTIFY stateChanged)
    Q_PROPERTY(double solanaPriceValue READ solanaPriceValue NOTIFY stateChanged)
    Q_PROPERTY(QString bitcoinPrice READ bitcoinPrice NOTIFY stateChanged)
    Q_PROPERTY(QString ethereumPrice READ ethereumPrice NOTIFY stateChanged)
    Q_PROPERTY(QString solanaPrice READ solanaPrice NOTIFY stateChanged)
    Q_PROPERTY(QString bitcoinStatus READ bitcoinStatus NOTIFY stateChanged)
    Q_PROPERTY(QString ethereumStatus READ ethereumStatus NOTIFY stateChanged)
    Q_PROPERTY(QString solanaStatus READ solanaStatus NOTIFY stateChanged)
    Q_PROPERTY(QString bitcoinUpdated READ bitcoinUpdated NOTIFY stateChanged)
    Q_PROPERTY(QString ethereumUpdated READ ethereumUpdated NOTIFY stateChanged)
    Q_PROPERTY(QString solanaUpdated READ solanaUpdated NOTIFY stateChanged)
    Q_PROPERTY(QString lastUpdated READ lastUpdated NOTIFY stateChanged)
    Q_PROPERTY(bool usingCachedData READ usingCachedData NOTIFY stateChanged)

public:
    explicit PriceService(QObject *parent = nullptr);

    bool loading() const;
    bool lastRefreshPassed() const;
    QString status() const;
    QString fiatCurrency() const;
    void setFiatCurrency(const QString &currency);
    QString fiatSymbol() const;
    double bitcoinPriceValue() const;
    double ethereumPriceValue() const;
    double solanaPriceValue() const;
    QString bitcoinPrice() const;
    QString ethereumPrice() const;
    QString solanaPrice() const;
    QString bitcoinStatus() const;
    QString ethereumStatus() const;
    QString solanaStatus() const;
    QString bitcoinUpdated() const;
    QString ethereumUpdated() const;
    QString solanaUpdated() const;
    QString lastUpdated() const;
    bool usingCachedData() const;

    Q_INVOKABLE void refresh();
    Q_INVOKABLE QString formatFiat(double value) const;

signals:
    void stateChanged();
    void settingsChanged();

private:
    void requestTicker(const QString &asset, const QString &pair, quint64 generation, int attempt = 0);
    void finishChain(const QString &asset, bool passed, double price,
                     const QString &message, quint64 generation);
    void finishIfReady(quint64 generation);
    void loadCachedPrices();
    void saveCachedPrices();
    void saveCachedPrice(const QString &asset, double price, const QString &updated);

    static QString normalizedCurrency(const QString &currency);
    static QString pairFor(const QString &asset, const QString &currency);
    static QString networkErrorText(QNetworkReply *reply);

    QNetworkAccessManager m_network;
    bool m_loading = false;
    bool m_lastRefreshPassed = false;
    bool m_usingCachedData = false;
    bool m_btcDone = false;
    bool m_ethDone = false;
    bool m_solDone = false;
    bool m_btcOk = false;
    bool m_ethOk = false;
    bool m_solOk = false;
    double m_bitcoinPrice = 0.0;
    double m_ethereumPrice = 0.0;
    double m_solanaPrice = 0.0;
    QString m_status;
    QString m_bitcoinStatus;
    QString m_ethereumStatus;
    QString m_solanaStatus;
    QString m_bitcoinUpdated;
    QString m_ethereumUpdated;
    QString m_solanaUpdated;
    QString m_lastUpdated;
    quint64 m_generation = 0;
};
