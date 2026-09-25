#include "portfoliohistoryservice.h"
#include "settingsstore.h"

#include <QCryptographicHash>
#include <QDateTime>
#include <QJsonDocument>
#include <QJsonObject>
#include <QList>
#include <QtMath>

namespace {
const int kMaximumSnapshots = 120;
const int kDuplicateWindowSeconds = 60;

bool sameNumber(double left, double right)
{
    return qAbs(left - right) <= 0.00000001;
}
}

PortfolioHistoryService::PortfolioHistoryService(QObject *parent)
    : QObject(parent)
{
    rebuildView();
}

QString PortfolioHistoryService::fiatCurrency() const
{
    return m_fiatCurrency;
}

void PortfolioHistoryService::setFiatCurrency(const QString &currency)
{
    const QString normalized = normalizedCurrency(currency);
    if (normalized == m_fiatCurrency)
        return;

    m_fiatCurrency = normalized;
    rebuildView();
    emit stateChanged();
}

QVariantList PortfolioHistoryService::entries() const
{
    return m_entries;
}

int PortfolioHistoryService::count() const
{
    return m_entries.size();
}

QString PortfolioHistoryService::status() const
{
    return m_status;
}

double PortfolioHistoryService::latestTotalValue() const
{
    return m_latestTotalValue;
}

double PortfolioHistoryService::oldestTotalValue() const
{
    return m_oldestTotalValue;
}

double PortfolioHistoryService::changeValue() const
{
    return m_changeValue;
}

double PortfolioHistoryService::changePercent() const
{
    return m_changePercent;
}

QString PortfolioHistoryService::latestTimestamp() const
{
    return m_latestTimestamp;
}

double PortfolioHistoryService::latestBitcoinValue() const
{
    return m_latestBitcoinValue;
}

double PortfolioHistoryService::latestEthereumValue() const
{
    return m_latestEthereumValue;
}

double PortfolioHistoryService::latestSolanaValue() const
{
    return m_latestSolanaValue;
}

int PortfolioHistoryService::analysisWindowDays() const
{
    return m_analysisWindowDays;
}

void PortfolioHistoryService::setAnalysisWindowDays(int days)
{
    int normalized = 0;
    if (days == 1 || days == 7 || days == 30)
        normalized = days;

    if (normalized == m_analysisWindowDays)
        return;

    m_analysisWindowDays = normalized;
    rebuildView();
    emit stateChanged();
}

QString PortfolioHistoryService::analysisSeries() const
{
    return m_analysisSeries;
}

void PortfolioHistoryService::setAnalysisSeries(const QString &series)
{
    const QString normalized = normalizedAnalysisSeries(series);
    if (normalized == m_analysisSeries)
        return;

    m_analysisSeries = normalized;
    rebuildView();
    emit stateChanged();
}

QVariantList PortfolioHistoryService::analysisEntries() const
{
    return m_analysisEntries;
}

int PortfolioHistoryService::analysisCount() const
{
    return m_analysisEntries.size();
}

QString PortfolioHistoryService::analysisStatus() const
{
    return m_analysisStatus;
}

double PortfolioHistoryService::analysisStartValue() const
{
    return m_analysisStartValue;
}

double PortfolioHistoryService::analysisEndValue() const
{
    return m_analysisEndValue;
}

double PortfolioHistoryService::analysisLowValue() const
{
    return m_analysisLowValue;
}

double PortfolioHistoryService::analysisHighValue() const
{
    return m_analysisHighValue;
}

double PortfolioHistoryService::analysisAverageValue() const
{
    return m_analysisAverageValue;
}

double PortfolioHistoryService::analysisChangeValue() const
{
    return m_analysisChangeValue;
}

double PortfolioHistoryService::analysisChangePercent() const
{
    return m_analysisChangePercent;
}

QString PortfolioHistoryService::analysisStartTimestamp() const
{
    return m_analysisStartTimestamp;
}

QString PortfolioHistoryService::analysisEndTimestamp() const
{
    return m_analysisEndTimestamp;
}

QString PortfolioHistoryService::normalizedCurrency(const QString &currency)
{
    const QString normalized = currency.trimmed().toUpper();
    if (normalized == QStringLiteral("USD")
            || normalized == QStringLiteral("EUR")
            || normalized == QStringLiteral("GBP")) {
        return normalized;
    }
    return QStringLiteral("GBP");
}

QString PortfolioHistoryService::normalizedAnalysisSeries(const QString &series)
{
    const QString normalized = series.trimmed().toLower();
    if (normalized == QStringLiteral("bitcoin")
            || normalized == QStringLiteral("ethereum")
            || normalized == QStringLiteral("solana")) {
        return normalized;
    }
    return QStringLiteral("total");
}

QString PortfolioHistoryService::contextId(const QString &ethereumAddress,
                                           const QString &bitcoinAddress,
                                           const QString &solanaAddress)
{
    QString eth = ethereumAddress.trimmed().toLower();
    const QString btc = bitcoinAddress.trimmed();
    const QString sol = solanaAddress.trimmed();

    if (eth.isEmpty() && btc.isEmpty() && sol.isEmpty())
        return QString();

    QByteArray source = eth.toUtf8();
    source.append('|');
    source.append(btc.toUtf8());
    source.append('|');
    source.append(sol.toUtf8());

    return QString::fromLatin1(
        QCryptographicHash::hash(source, QCryptographicHash::Sha256).toHex());
}

double PortfolioHistoryService::numericBalance(const QString &text)
{
    bool ok = false;
    const double value = text.trimmed().section(QLatin1Char(' '), 0, 0).toDouble(&ok);
    return ok && value >= 0.0 ? value : 0.0;
}

double PortfolioHistoryService::analysisValue(const QVariantMap &item) const
{
    if (m_analysisSeries == QStringLiteral("bitcoin"))
        return item.value(QStringLiteral("bitcoinValue")).toDouble();
    if (m_analysisSeries == QStringLiteral("ethereum"))
        return item.value(QStringLiteral("ethereumValue")).toDouble();
    if (m_analysisSeries == QStringLiteral("solana"))
        return item.value(QStringLiteral("solanaValue")).toDouble();
    return item.value(QStringLiteral("totalValue")).toDouble();
}

QString PortfolioHistoryService::storageKey() const
{
    if (m_contextId.isEmpty())
        return QString();
    return QStringLiteral("portfolioHistory/v1/") + m_contextId;
}

void PortfolioHistoryService::setContext(const QString &ethereumAddress,
                                         const QString &bitcoinAddress,
                                         const QString &solanaAddress)
{
    const QString nextId = contextId(ethereumAddress, bitcoinAddress, solanaAddress);
    if (nextId == m_contextId)
        return;

    m_contextId = nextId;
    loadHistory();
    emit stateChanged();
}

void PortfolioHistoryService::loadHistory()
{
    m_allEntries = QJsonArray();

    const QString key = storageKey();
    if (!key.isEmpty()) {
        const QByteArray encoded = SailVaultSettings::value(key, QString()).toString().toUtf8();
        if (!encoded.isEmpty()) {
            QJsonParseError error;
            const QJsonDocument document = QJsonDocument::fromJson(encoded, &error);
            if (error.error == QJsonParseError::NoError && document.isArray()) {
                m_allEntries = document.array();
            } else {
                SailVaultSettings::quarantineValue(
                    key, QStringLiteral("Invalid portfolio history JSON"));
            }
        }
    }

    rebuildView();
}

void PortfolioHistoryService::saveHistory()
{
    const QString key = storageKey();
    if (key.isEmpty())
        return;

    SailVaultSettings::setValue(
        key,
        QString::fromUtf8(QJsonDocument(m_allEntries).toJson(QJsonDocument::Compact)));
}

bool PortfolioHistoryService::recordSnapshot(const QString &ethereumBalance,
                                             const QString &bitcoinBalance,
                                             const QString &solanaBalance,
                                             double ethereumPrice,
                                             double bitcoinPrice,
                                             double solanaPrice,
                                             const QString &currency,
                                             const QString &balanceUpdated,
                                             const QString &priceUpdated)
{
    if (m_contextId.isEmpty())
        return false;

    const QString normalizedFiat = normalizedCurrency(currency);
    const double ethBalance = numericBalance(ethereumBalance);
    const double btcBalance = numericBalance(bitcoinBalance);
    const double solBalance = numericBalance(solanaBalance);

    const double ethValue = ethereumPrice > 0.0 ? ethBalance * ethereumPrice : 0.0;
    const double btcValue = bitcoinPrice > 0.0 ? btcBalance * bitcoinPrice : 0.0;
    const double solValue = solanaPrice > 0.0 ? solBalance * solanaPrice : 0.0;
    const double total = ethValue + btcValue + solValue;

    // A zero-value wallet is still a valid snapshot as long as market prices
    // were available for at least one configured native asset.
    if (ethereumPrice <= 0.0 && bitcoinPrice <= 0.0 && solanaPrice <= 0.0)
        return false;

    const QDateTime balanceTime = QDateTime::fromString(
        balanceUpdated, QStringLiteral("yyyy-MM-dd hh:mm:ss"));
    const QDateTime priceTime = QDateTime::fromString(
        priceUpdated, QStringLiteral("yyyy-MM-dd hh:mm:ss"));
    if (!balanceTime.isValid() || !priceTime.isValid()
            || qAbs(balanceTime.secsTo(priceTime)) > 300) {
        return false;
    }

    const QDateTime now = QDateTime::currentDateTime();

    if (!m_allEntries.isEmpty()) {
        const QJsonObject previous =
            m_allEntries.at(m_allEntries.size() - 1).toObject();
        const QDateTime previousTime = QDateTime::fromString(
            previous.value(QStringLiteral("recordedAtIso")).toString(), Qt::ISODate);

        const bool sameSnapshot =
            previous.value(QStringLiteral("fiatCurrency")).toString() == normalizedFiat
            && previous.value(QStringLiteral("ethereumBalance")).toString() == ethereumBalance
            && previous.value(QStringLiteral("bitcoinBalance")).toString() == bitcoinBalance
            && previous.value(QStringLiteral("solanaBalance")).toString() == solanaBalance
            && sameNumber(previous.value(QStringLiteral("ethereumPrice")).toDouble(), ethereumPrice)
            && sameNumber(previous.value(QStringLiteral("bitcoinPrice")).toDouble(), bitcoinPrice)
            && sameNumber(previous.value(QStringLiteral("solanaPrice")).toDouble(), solanaPrice);

        if (sameSnapshot && previousTime.isValid()
                && previousTime.secsTo(now) >= 0
                && previousTime.secsTo(now) < kDuplicateWindowSeconds) {
            return false;
        }
    }

    QJsonObject snapshot;
    snapshot.insert(QStringLiteral("recordedAtIso"), now.toString(Qt::ISODate));
    snapshot.insert(QStringLiteral("recordedAt"),
                    now.toString(QStringLiteral("yyyy-MM-dd hh:mm:ss")));
    snapshot.insert(QStringLiteral("balanceUpdated"), balanceUpdated);
    snapshot.insert(QStringLiteral("priceUpdated"), priceUpdated);
    snapshot.insert(QStringLiteral("fiatCurrency"), normalizedFiat);
    snapshot.insert(QStringLiteral("ethereumBalance"), ethereumBalance);
    snapshot.insert(QStringLiteral("bitcoinBalance"), bitcoinBalance);
    snapshot.insert(QStringLiteral("solanaBalance"), solanaBalance);
    snapshot.insert(QStringLiteral("ethereumPrice"), ethereumPrice);
    snapshot.insert(QStringLiteral("bitcoinPrice"), bitcoinPrice);
    snapshot.insert(QStringLiteral("solanaPrice"), solanaPrice);
    snapshot.insert(QStringLiteral("ethereumValue"), ethValue);
    snapshot.insert(QStringLiteral("bitcoinValue"), btcValue);
    snapshot.insert(QStringLiteral("solanaValue"), solValue);
    snapshot.insert(QStringLiteral("totalValue"), total);

    m_allEntries.append(snapshot);
    while (m_allEntries.size() > kMaximumSnapshots)
        m_allEntries.removeAt(0);

    saveHistory();
    rebuildView();
    emit stateChanged();
    return true;
}

void PortfolioHistoryService::clearHistory()
{
    if (m_contextId.isEmpty())
        return;

    QJsonArray retained;
    for (int i = 0; i < m_allEntries.size(); ++i) {
        const QJsonObject object = m_allEntries.at(i).toObject();
        if (object.value(QStringLiteral("fiatCurrency")).toString() != m_fiatCurrency)
            retained.append(object);
    }

    m_allEntries = retained;
    saveHistory();
    rebuildView();
    emit stateChanged();
}

void PortfolioHistoryService::rebuildView()
{
    m_entries.clear();
    m_latestTotalValue = 0.0;
    m_oldestTotalValue = 0.0;
    m_changeValue = 0.0;
    m_changePercent = 0.0;
    m_latestTimestamp.clear();
    m_latestBitcoinValue = 0.0;
    m_latestEthereumValue = 0.0;
    m_latestSolanaValue = 0.0;

    m_analysisEntries.clear();
    m_analysisStatus.clear();
    m_analysisStartValue = 0.0;
    m_analysisEndValue = 0.0;
    m_analysisLowValue = 0.0;
    m_analysisHighValue = 0.0;
    m_analysisAverageValue = 0.0;
    m_analysisChangeValue = 0.0;
    m_analysisChangePercent = 0.0;
    m_analysisStartTimestamp.clear();
    m_analysisEndTimestamp.clear();

    QList<QVariantMap> filtered;
    for (int i = 0; i < m_allEntries.size(); ++i) {
        const QJsonObject object = m_allEntries.at(i).toObject();
        if (object.value(QStringLiteral("fiatCurrency")).toString() != m_fiatCurrency)
            continue;

        QVariantMap item;
        item.insert(QStringLiteral("recordedAtIso"),
                    object.value(QStringLiteral("recordedAtIso")).toString());
        item.insert(QStringLiteral("recordedAt"),
                    object.value(QStringLiteral("recordedAt")).toString());
        item.insert(QStringLiteral("balanceUpdated"),
                    object.value(QStringLiteral("balanceUpdated")).toString());
        item.insert(QStringLiteral("priceUpdated"),
                    object.value(QStringLiteral("priceUpdated")).toString());
        item.insert(QStringLiteral("ethereumBalance"),
                    object.value(QStringLiteral("ethereumBalance")).toString());
        item.insert(QStringLiteral("bitcoinBalance"),
                    object.value(QStringLiteral("bitcoinBalance")).toString());
        item.insert(QStringLiteral("solanaBalance"),
                    object.value(QStringLiteral("solanaBalance")).toString());
        item.insert(QStringLiteral("ethereumValue"),
                    object.value(QStringLiteral("ethereumValue")).toDouble());
        item.insert(QStringLiteral("bitcoinValue"),
                    object.value(QStringLiteral("bitcoinValue")).toDouble());
        item.insert(QStringLiteral("solanaValue"),
                    object.value(QStringLiteral("solanaValue")).toDouble());
        item.insert(QStringLiteral("totalValue"),
                    object.value(QStringLiteral("totalValue")).toDouble());
        filtered.append(item);
    }

    if (filtered.isEmpty()) {
        if (m_contextId.isEmpty())
            m_status = QStringLiteral("No portfolio context is loaded");
        else
            m_status = QStringLiteral("No local %1 history yet").arg(m_fiatCurrency);
        m_analysisStatus = m_status;
        return;
    }

    const QVariantMap oldest = filtered.first();
    const QVariantMap latest = filtered.last();

    m_oldestTotalValue = oldest.value(QStringLiteral("totalValue")).toDouble();
    m_latestTotalValue = latest.value(QStringLiteral("totalValue")).toDouble();
    m_changeValue = m_latestTotalValue - m_oldestTotalValue;
    if (m_oldestTotalValue > 0.0)
        m_changePercent = (m_changeValue / m_oldestTotalValue) * 100.0;
    m_latestTimestamp = latest.value(QStringLiteral("recordedAt")).toString();
    m_latestBitcoinValue = latest.value(QStringLiteral("bitcoinValue")).toDouble();
    m_latestEthereumValue = latest.value(QStringLiteral("ethereumValue")).toDouble();
    m_latestSolanaValue = latest.value(QStringLiteral("solanaValue")).toDouble();

    for (int i = filtered.size() - 1; i >= 0; --i)
        m_entries.append(filtered.at(i));

    m_status = QStringLiteral("%1 local snapshot%2 · stored on device only")
        .arg(filtered.size())
        .arg(filtered.size() == 1 ? QString() : QStringLiteral("s"));

    const QDateTime cutoff = m_analysisWindowDays > 0
        ? QDateTime::currentDateTime().addSecs(-m_analysisWindowDays * 86400)
        : QDateTime();
    double sum = 0.0;

    for (int i = 0; i < filtered.size(); ++i) {
        const QVariantMap source = filtered.at(i);
        const QDateTime recorded = QDateTime::fromString(
            source.value(QStringLiteral("recordedAtIso")).toString(), Qt::ISODate);
        if (m_analysisWindowDays > 0
                && (!recorded.isValid() || recorded < cutoff)) {
            continue;
        }

        const double value = analysisValue(source);
        QVariantMap point;
        point.insert(QStringLiteral("recordedAt"),
                     source.value(QStringLiteral("recordedAt")));
        point.insert(QStringLiteral("recordedAtIso"),
                     source.value(QStringLiteral("recordedAtIso")));
        point.insert(QStringLiteral("recordedEpoch"),
                     recorded.toMSecsSinceEpoch());
        point.insert(QStringLiteral("value"), value);
        m_analysisEntries.append(point);

        if (m_analysisEntries.size() == 1) {
            m_analysisStartValue = value;
            m_analysisLowValue = value;
            m_analysisHighValue = value;
            m_analysisStartTimestamp = source.value(QStringLiteral("recordedAt")).toString();
        } else {
            m_analysisLowValue = qMin(m_analysisLowValue, value);
            m_analysisHighValue = qMax(m_analysisHighValue, value);
        }

        m_analysisEndValue = value;
        m_analysisEndTimestamp = source.value(QStringLiteral("recordedAt")).toString();
        sum += value;
    }

    if (m_analysisEntries.isEmpty()) {
        m_analysisStatus = QStringLiteral("No snapshots in the selected history window");
        return;
    }

    m_analysisAverageValue = sum / static_cast<double>(m_analysisEntries.size());
    m_analysisChangeValue = m_analysisEndValue - m_analysisStartValue;
    if (m_analysisStartValue > 0.0)
        m_analysisChangePercent = (m_analysisChangeValue / m_analysisStartValue) * 100.0;

    m_analysisStatus = QStringLiteral("%1 chart point%2 · local history only")
        .arg(m_analysisEntries.size())
        .arg(m_analysisEntries.size() == 1 ? QString() : QStringLiteral("s"));
}
