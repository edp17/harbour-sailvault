#include "bitcoinunsignedtransactionservice.h"

#include "networkrequestutils.h"
#include "unsignedtransactionservice.h"

#include <QCryptographicHash>
#include <QDateTime>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QRegularExpression>
#include <QSet>
#include <QTimer>
#include <QUrl>

#include <TrustWalletCore/TWBitcoinScript.h>
#include <TrustWalletCore/TWCoinType.h>
#include <TrustWalletCore/TWData.h>
#include <TrustWalletCore/TWHash.h>
#include <TrustWalletCore/TWString.h>

#include <algorithm>
#include <cmath>
#include <limits>

namespace {
const int kMaximumDnsRetries = 1;
const int kDnsRetryDelayMs = 900;
const quint64 kMaximumBitcoinSats = 21000000ULL * 100000000ULL;
const quint64 kDustThresholdSats = 546ULL;
const int kMaximumInputs = 100;
const quint64 kSequenceRbf = 0xfffffffdULL;

class TWStringGuard {
public:
    explicit TWStringGuard(TWString *v = nullptr) : value(v) {}
    ~TWStringGuard() { if (value) TWStringDelete(value); }
    TWString *get() const { return value; }
private:
    TWString *value;
};

class TWScriptGuard {
public:
    explicit TWScriptGuard(TWBitcoinScript *v = nullptr) : value(v) {}
    ~TWScriptGuard() { if (value) TWBitcoinScriptDelete(value); }
    TWBitcoinScript *get() const { return value; }
private:
    TWBitcoinScript *value;
};

class TWDataGuard {
public:
    explicit TWDataGuard(TWData *v = nullptr) : value(v) {}
    ~TWDataGuard() { if (value) TWDataDelete(value); }
    TWData *get() const { return value; }
private:
    TWData *value;
};

void appendLe32(QByteArray &out, quint32 value)
{
    for (int i = 0; i < 4; ++i)
        out.append(static_cast<char>((value >> (8 * i)) & 0xffU));
}

void appendLe64(QByteArray &out, quint64 value)
{
    for (int i = 0; i < 8; ++i)
        out.append(static_cast<char>((value >> (8 * i)) & 0xffU));
}

bool isHex64(const QString &value)
{
    static const QRegularExpression pattern(QStringLiteral("^[0-9a-fA-F]{64}$"));
    return pattern.match(value).hasMatch();
}

int compactSizeLenLocal(quint64 value)
{
    if (value < 0xfdULL) return 1;
    if (value <= 0xffffULL) return 3;
    if (value <= 0xffffffffULL) return 5;
    return 9;
}

quint64 outputBytes(const QByteArray &script)
{
    return 8ULL + static_cast<quint64>(compactSizeLenLocal(
        static_cast<quint64>(script.size()))) + static_cast<quint64>(script.size());
}

} // namespace

BitcoinUnsignedTransactionService::BitcoinUnsignedTransactionService(QObject *parent)
    : QObject(parent)
{
    clearState();
}

void BitcoinUnsignedTransactionService::clearState(bool preserveStatus)
{
    const QString oldStatus = m_status;
    m_loading = false;
    m_constructed = false;
    m_pending = 0;
    m_haveGenesis = false;
    m_haveUtxos = false;
    m_haveFee = false;
    m_providerHost.clear();
    m_intentFingerprint.clear();
    m_feeTarget.clear();
    m_providerFeeEstimate.clear();
    m_feeRateSatVb.clear();
    m_confirmedUtxoCount = 0;
    m_selectedInputCount = 0;
    m_selectedInputsSummary.clear();
    m_totalInputBtc.clear();
    m_amountBtc.clear();
    m_networkFeeBtc.clear();
    m_changeBtc.clear();
    m_estimatedVbytes.clear();
    m_changePolicy.clear();
    m_unsignedTransactionHex.clear();
    m_unsignedTxid.clear();
    m_psbtBase64.clear();
    m_constructionFingerprint.clear();
    m_constructedAt.clear();
    m_utxos.clear();
    m_amountSats = 0;
    m_feeRate = 0;
    m_sourceScript.clear();
    m_destinationScript.clear();
    if (!preserveStatus)
        m_status.clear();
    else
        m_status = oldStatus;
}

void BitcoinUnsignedTransactionService::reset()
{
    ++m_generation;
    SailVaultNetwork::cancelOutstanding(&m_network);
    clearState();
    emit stateChanged();
}

bool BitcoinUnsignedTransactionService::amountToSats(const QString &amount,
                                                       quint64 *sats)
{
    if (!sats)
        return false;
    const QString value = amount.trimmed();
    static const QRegularExpression pattern(QStringLiteral("^[0-9]+(?:\\.[0-9]{1,8})?$"));
    if (!pattern.match(value).hasMatch())
        return false;

    const int point = value.indexOf(QLatin1Char('.'));
    QString whole = point >= 0 ? value.left(point) : value;
    QString fraction = point >= 0 ? value.mid(point + 1) : QString();
    while (fraction.size() < 8)
        fraction.append(QLatin1Char('0'));
    QString digits = whole + fraction;
    int first = 0;
    while (first + 1 < digits.size() && digits.at(first) == QLatin1Char('0'))
        ++first;
    digits = digits.mid(first);

    quint64 result = 0;
    for (int i = 0; i < digits.size(); ++i) {
        const int d = digits.at(i).unicode() - QLatin1Char('0').unicode();
        if (result > (std::numeric_limits<quint64>::max() - static_cast<quint64>(d)) / 10ULL)
            return false;
        result = result * 10ULL + static_cast<quint64>(d);
    }
    if (result == 0 || result > kMaximumBitcoinSats)
        return false;
    *sats = result;
    return true;
}

bool BitcoinUnsignedTransactionService::lockScriptForAddress(const QString &address,
                                                              QByteArray *script,
                                                              bool *isP2wpkh)
{
    if (!script)
        return false;
    const QByteArray utf8 = address.trimmed().toUtf8();
    TWStringGuard string(TWStringCreateWithUTF8Bytes(utf8.constData()));
    if (!string.get())
        return false;
    TWScriptGuard lock(TWBitcoinScriptLockScriptForAddress(string.get(), TWCoinTypeBitcoin));
    if (!lock.get())
        return false;
    if (isP2wpkh)
        *isP2wpkh = TWBitcoinScriptIsPayToWitnessPublicKeyHash(lock.get());
    TWDataGuard data(TWBitcoinScriptData(lock.get()));
    if (!data.get())
        return false;
    *script = QByteArray(reinterpret_cast<const char *>(TWDataBytes(data.get())),
                         static_cast<int>(TWDataSize(data.get())));
    return !script->isEmpty();
}

QByteArray BitcoinUnsignedTransactionService::compactSize(quint64 value)
{
    QByteArray out;
    if (value < 0xfdULL) {
        out.append(static_cast<char>(value));
    } else if (value <= 0xffffULL) {
        out.append(static_cast<char>(0xfd));
        out.append(static_cast<char>(value & 0xffU));
        out.append(static_cast<char>((value >> 8) & 0xffU));
    } else if (value <= 0xffffffffULL) {
        out.append(static_cast<char>(0xfe));
        appendLe32(out, static_cast<quint32>(value));
    } else {
        out.append(static_cast<char>(0xff));
        appendLe64(out, value);
    }
    return out;
}

int BitcoinUnsignedTransactionService::compactSizeLength(quint64 value)
{
    if (value < 0xfdULL) return 1;
    if (value <= 0xffffULL) return 3;
    if (value <= 0xffffffffULL) return 5;
    return 9;
}

QString BitcoinUnsignedTransactionService::formatBtc(quint64 sats)
{
    const quint64 whole = sats / 100000000ULL;
    quint64 fractionValue = sats % 100000000ULL;
    QString fraction = QStringLiteral("%1").arg(fractionValue, 8, 10, QLatin1Char('0'));
    while (!fraction.isEmpty() && fraction.endsWith(QLatin1Char('0')))
        fraction.chop(1);
    return fraction.isEmpty()
        ? QStringLiteral("%1 BTC").arg(whole)
        : QStringLiteral("%1.%2 BTC").arg(whole).arg(fraction);
}

QString BitcoinUnsignedTransactionService::txidFor(const QByteArray &unsignedTx)
{
    TWDataGuard data(TWDataCreateWithBytes(
        reinterpret_cast<const uint8_t *>(unsignedTx.constData()),
        static_cast<size_t>(unsignedTx.size())));
    if (!data.get())
        return QString();
    TWDataGuard hash(TWHashSHA256SHA256(data.get()));
    if (!hash.get() || TWDataSize(hash.get()) != 32)
        return QString();
    QByteArray bytes(reinterpret_cast<const char *>(TWDataBytes(hash.get())), 32);
    std::reverse(bytes.begin(), bytes.end());
    return QString::fromLatin1(bytes.toHex());
}

QString BitcoinUnsignedTransactionService::constructionFingerprintFor(
        const QString &intentFingerprint,
        const QByteArray &unsignedTx,
        const QByteArray &psbt)
{
    QByteArray canonical("SailVault M50 Bitcoin construction v1\nintent=");
    canonical += intentFingerprint.toLatin1();
    canonical += "\nunsigned=";
    canonical += unsignedTx.toHex();
    canonical += "\npsbt=";
    canonical += psbt.toBase64();
    return QString::fromLatin1(QCryptographicHash::hash(
        canonical, QCryptographicHash::Sha256).toHex());
}

BitcoinUnsignedTransactionService::BuildResult
BitcoinUnsignedTransactionService::buildTransaction(
        const QVector<Utxo> &utxos,
        quint64 amount,
        quint64 feeRateSatVb,
        const QByteArray &sourceScript,
        const QByteArray &destinationScript)
{
    BuildResult result;
    result.amount = amount;
    if (amount < kDustThresholdSats) {
        result.error = QStringLiteral("Bitcoin amount is below the conservative 546 sat dust floor");
        return result;
    }
    if (feeRateSatVb == 0 || feeRateSatVb > 10000ULL) {
        result.error = QStringLiteral("Bitcoin fee rate is outside the safety range");
        return result;
    }
    if (sourceScript.isEmpty() || destinationScript.isEmpty()) {
        result.error = QStringLiteral("Bitcoin output script is unavailable");
        return result;
    }

    QVector<Utxo> sorted = utxos;
    std::sort(sorted.begin(), sorted.end(), [](const Utxo &a, const Utxo &b) {
        if (a.value != b.value) return a.value > b.value;
        if (a.txid != b.txid) return a.txid < b.txid;
        return a.vout < b.vout;
    });

    quint64 total = 0;
    const auto estimateVbytes = [&](int inputs, bool withChange) -> quint64 {
        const int outputs = withChange ? 2 : 1;
        quint64 weight = 16ULL; // version
        weight += 2ULL; // segwit marker+flag in final signed transaction
        weight += static_cast<quint64>(compactSizeLength(inputs)) * 4ULL;
        weight += static_cast<quint64>(inputs) * (41ULL * 4ULL + 109ULL);
        weight += static_cast<quint64>(compactSizeLength(outputs)) * 4ULL;
        weight += outputBytes(destinationScript) * 4ULL;
        if (withChange)
            weight += outputBytes(sourceScript) * 4ULL;
        weight += 16ULL; // locktime
        return (weight + 3ULL) / 4ULL;
    };

    for (int i = 0; i < sorted.size() && i < kMaximumInputs; ++i) {
        const Utxo &u = sorted.at(i);
        if (u.value == 0 || !isHex64(u.txid))
            continue;
        if (total > kMaximumBitcoinSats - u.value) {
            result.error = QStringLiteral("Bitcoin UTXO total overflow");
            return result;
        }
        total += u.value;
        result.selected.append(u);

        const quint64 vbTwo = estimateVbytes(result.selected.size(), true);
        if (vbTwo > std::numeric_limits<quint64>::max() / feeRateSatVb)
            break;
        const quint64 feeTwo = vbTwo * feeRateSatVb;
        if (total >= amount && total - amount >= feeTwo) {
            const quint64 changeCandidate = total - amount - feeTwo;
            if (changeCandidate >= kDustThresholdSats) {
                result.fee = feeTwo;
                result.change = changeCandidate;
                result.changeOutput = true;
                result.estimatedVbytes = vbTwo;
                break;
            }

            const quint64 vbOne = estimateVbytes(result.selected.size(), false);
            const quint64 feeOne = vbOne * feeRateSatVb;
            if (total - amount >= feeOne) {
                result.fee = total - amount; // residual is intentionally fee
                result.change = 0;
                result.changeOutput = false;
                result.estimatedVbytes = vbOne;
                break;
            }
        }
    }

    if (result.fee == 0 || result.selected.isEmpty()) {
        result.error = QStringLiteral("Insufficient confirmed UTXOs for amount plus fee");
        return result;
    }
    result.totalInput = total;

    QByteArray tx;
    appendLe32(tx, 2U);
    tx += compactSize(static_cast<quint64>(result.selected.size()));
    for (const Utxo &u : result.selected) {
        QByteArray txidBytes = QByteArray::fromHex(u.txid.toLatin1());
        if (txidBytes.size() != 32) {
            result.error = QStringLiteral("Invalid UTXO transaction ID");
            return result;
        }
        std::reverse(txidBytes.begin(), txidBytes.end());
        tx += txidBytes;
        appendLe32(tx, u.vout);
        tx += compactSize(0);
        appendLe32(tx, static_cast<quint32>(kSequenceRbf));
    }
    tx += compactSize(result.changeOutput ? 2ULL : 1ULL);
    appendLe64(tx, amount);
    tx += compactSize(static_cast<quint64>(destinationScript.size()));
    tx += destinationScript;
    if (result.changeOutput) {
        appendLe64(tx, result.change);
        tx += compactSize(static_cast<quint64>(sourceScript.size()));
        tx += sourceScript;
    }
    appendLe32(tx, 0U);
    result.unsignedTx = tx;
    result.txid = txidFor(tx);
    if (result.txid.size() != 64) {
        result.error = QStringLiteral("Wallet Core SHA256d transaction ID failed");
        return result;
    }

    QByteArray psbt("psbt\xff", 5);
    psbt += compactSize(1);
    psbt.append(static_cast<char>(0x00));
    psbt += compactSize(static_cast<quint64>(tx.size()));
    psbt += tx;
    psbt.append(static_cast<char>(0x00));

    for (const Utxo &u : result.selected) {
        QByteArray witnessUtxo;
        appendLe64(witnessUtxo, u.value);
        witnessUtxo += compactSize(static_cast<quint64>(sourceScript.size()));
        witnessUtxo += sourceScript;

        psbt += compactSize(1);
        psbt.append(static_cast<char>(0x01)); // PSBT_IN_WITNESS_UTXO
        psbt += compactSize(static_cast<quint64>(witnessUtxo.size()));
        psbt += witnessUtxo;

        psbt += compactSize(1);
        psbt.append(static_cast<char>(0x03)); // PSBT_IN_SIGHASH_TYPE
        psbt += compactSize(4);
        appendLe32(psbt, 1U); // SIGHASH_ALL
        psbt.append(static_cast<char>(0x00));
    }
    psbt.append(static_cast<char>(0x00));
    if (result.changeOutput)
        psbt.append(static_cast<char>(0x00));

    result.psbt = psbt;
    result.ok = true;
    return result;
}

void BitcoinUnsignedTransactionService::construct(const QVariantMap &intent,
                                                    const QString &bitcoinApiUrl)
{
    ++m_generation;
    const quint64 generation = m_generation;
    SailVaultNetwork::cancelOutstanding(&m_network);
    clearState();

    QString intentError;
    if (!UnsignedTransactionService::validateReviewModel(intent, &intentError)) {
        m_status = QStringLiteral("Bitcoin construction blocked · %1").arg(intentError);
        emit stateChanged();
        return;
    }
    if (intent.value(QStringLiteral("chain")).toString() != QStringLiteral("Bitcoin")) {
        m_status = QStringLiteral("Bitcoin construction blocked · reviewed intent is not Bitcoin");
        emit stateChanged();
        return;
    }
    if (!amountToSats(intent.value(QStringLiteral("normalizedAmount")).toString(), &m_amountSats)) {
        m_status = QStringLiteral("Bitcoin construction blocked · invalid exact BTC amount");
        emit stateChanged();
        return;
    }
    if (m_amountSats < kDustThresholdSats) {
        m_status = QStringLiteral("Bitcoin construction blocked · amount is below conservative 546 sat dust floor");
        emit stateChanged();
        return;
    }

    bool sourceP2wpkh = false;
    if (!lockScriptForAddress(intent.value(QStringLiteral("sourceAddress")).toString(),
                              &m_sourceScript, &sourceP2wpkh) || !sourceP2wpkh) {
        m_status = QStringLiteral("Bitcoin construction blocked · M50 requires the development BIP84 P2WPKH source");
        emit stateChanged();
        return;
    }
    if (!lockScriptForAddress(intent.value(QStringLiteral("destinationAddress")).toString(),
                              &m_destinationScript)) {
        m_status = QStringLiteral("Bitcoin construction blocked · destination lock script is unavailable");
        emit stateChanged();
        return;
    }

    if (SailVaultNetwork::offlineModeEnabled()) {
        m_status = QStringLiteral("Bitcoin construction blocked · Offline mode is enabled");
        emit stateChanged();
        return;
    }

    QUrl base(bitcoinApiUrl.trimmed());
    if (!SailVaultNetwork::isValidHttpsEndpoint(base)) {
        m_status = QStringLiteral("Bitcoin construction blocked · configured API endpoint is not safe HTTPS");
        emit stateChanged();
        return;
    }
    QString normalized = base.toString(QUrl::RemoveQuery | QUrl::RemoveFragment);
    while (normalized.endsWith(QLatin1Char('/')))
        normalized.chop(1);

    m_intent = intent;
    m_baseUrl = normalized;
    m_providerHost = base.host();
    m_intentFingerprint = intent.value(QStringLiteral("fingerprint")).toString();
    m_loading = true;
    m_status = QStringLiteral("Verifying Bitcoin mainnet and fetching confirmed UTXOs / fee estimates…");
    m_pending = 3;
    emit stateChanged();

    const QString source = intent.value(QStringLiteral("sourceAddress")).toString();
    sendGet(GenesisRequest, QUrl(m_baseUrl + QStringLiteral("/block-height/0")), generation);
    sendGet(UtxosRequest, QUrl(m_baseUrl + QStringLiteral("/address/") + source
                               + QStringLiteral("/utxo")), generation);
    sendGet(FeeEstimatesRequest, QUrl(m_baseUrl + QStringLiteral("/fee-estimates")), generation);
}

void BitcoinUnsignedTransactionService::fail(const QString &message, quint64 generation)
{
    if (generation != m_generation)
        return;
    ++m_generation;
    SailVaultNetwork::cancelOutstanding(&m_network);
    m_loading = false;
    m_constructed = false;
    m_pending = 0;
    m_status = message;
    emit stateChanged();
}

void BitcoinUnsignedTransactionService::sendGet(RequestKind kind,
                                                 const QUrl &url,
                                                 quint64 generation,
                                                 int attempt)
{
    if (generation != m_generation)
        return;
    QNetworkRequest request{url};
    request.setRawHeader("Accept", "application/json");
    SailVaultNetwork::hardenRequest(request);
    QNetworkReply *reply = m_network.get(request);
    SailVaultNetwork::armTimeout(reply, kRequestTimeoutMs, kMaxResponseBytes);
    connect(reply, &QNetworkReply::finished, this,
            [this, reply, kind, url, generation, attempt]() {
        const QNetworkReply::NetworkError code = reply->error();
        const QString error = kind == GenesisRequest
            ? SailVaultNetwork::plainTextErrorText(reply, kRequestTimeoutMs, kMaxResponseBytes)
            : SailVaultNetwork::jsonErrorText(reply, kRequestTimeoutMs, kMaxResponseBytes);
        const QByteArray data = reply->readAll();
        reply->deleteLater();
        if (generation != m_generation)
            return;
        if (code == QNetworkReply::HostNotFoundError && attempt < kMaximumDnsRetries) {
            QTimer::singleShot(kDnsRetryDelayMs, this,
                               [this, kind, url, generation, attempt]() {
                sendGet(kind, url, generation, attempt + 1);
            });
            return;
        }
        if (!error.isEmpty()) {
            fail(QStringLiteral("Bitcoin construction failed · %1").arg(error), generation);
            return;
        }
        handleJson(kind, data, generation);
    });
}

void BitcoinUnsignedTransactionService::handleJson(RequestKind kind,
                                                    const QByteArray &data,
                                                    quint64 generation)
{
    if (generation != m_generation)
        return;
    if (kind == GenesisRequest) {
        const QString genesis = QString::fromLatin1(data).trimmed().toLower();
        if (genesis != QStringLiteral("000000000019d6689c085ae165831e934ff763ae46a2a6c172b3f1b60a8ce26f")) {
            fail(QStringLiteral("Bitcoin construction blocked · API is not Bitcoin mainnet genesis"), generation);
            return;
        }
        m_haveGenesis = true;
        if (m_pending > 0) --m_pending;
        maybeFinalize(generation);
        return;
    }

    QJsonParseError parseError;
    const QJsonDocument document = QJsonDocument::fromJson(data, &parseError);
    if (parseError.error != QJsonParseError::NoError) {
        fail(QStringLiteral("Bitcoin construction failed · invalid provider JSON"), generation);
        return;
    }

    if (kind == UtxosRequest) {
        if (!document.isArray()) {
            fail(QStringLiteral("Bitcoin construction failed · UTXO response is not an array"), generation);
            return;
        }
        const QJsonArray array = document.array();
        if (array.size() > 200) {
            fail(QStringLiteral("Bitcoin construction blocked · more than 200 UTXOs require a later coin-selection policy"), generation);
            return;
        }
        QVector<Utxo> confirmed;
        QSet<QString> seenOutpoints;
        for (const QJsonValue &entry : array) {
            if (!entry.isObject())
                continue;
            const QJsonObject object = entry.toObject();
            const QString txid = object.value(QStringLiteral("txid")).toString();
            const double voutDouble = object.value(QStringLiteral("vout")).toDouble(-1);
            const double valueDouble = object.value(QStringLiteral("value")).toDouble(-1);
            const QJsonObject status = object.value(QStringLiteral("status")).toObject();
            if (!isHex64(txid) || std::floor(voutDouble) != voutDouble
                    || voutDouble < 0 || voutDouble > 4294967295.0
                    || std::floor(valueDouble) != valueDouble
                    || valueDouble <= 0 || valueDouble > static_cast<double>(kMaximumBitcoinSats)
                    || !status.value(QStringLiteral("confirmed")).toBool(false)) {
                continue;
            }
            Utxo u;
            u.txid = txid.toLower();
            u.vout = static_cast<quint32>(voutDouble);
            u.value = static_cast<quint64>(valueDouble);
            const QString outpoint = u.txid + QLatin1Char(':') + QString::number(u.vout);
            if (seenOutpoints.contains(outpoint))
                continue;
            seenOutpoints.insert(outpoint);
            confirmed.append(u);
        }
        m_utxos = confirmed;
        m_confirmedUtxoCount = confirmed.size();
        m_haveUtxos = true;
    } else {
        if (!document.isObject()) {
            fail(QStringLiteral("Bitcoin construction failed · fee-estimate response is not an object"), generation);
            return;
        }
        const QJsonObject fees = document.object();
        QString selectedKey;
        if (fees.contains(QStringLiteral("6")))
            selectedKey = QStringLiteral("6");
        else if (fees.contains(QStringLiteral("3")))
            selectedKey = QStringLiteral("3");
        else if (fees.contains(QStringLiteral("1")))
            selectedKey = QStringLiteral("1");
        else {
            int best = std::numeric_limits<int>::max();
            for (auto it = fees.constBegin(); it != fees.constEnd(); ++it) {
                bool ok = false;
                const int target = it.key().toInt(&ok);
                if (ok && target > 0 && target < best && it.value().isDouble()
                        && it.value().toDouble() > 0) {
                    best = target;
                    selectedKey = it.key();
                }
            }
        }
        if (selectedKey.isEmpty() || !fees.value(selectedKey).isDouble()) {
            fail(QStringLiteral("Bitcoin construction failed · provider supplied no usable fee estimate"), generation);
            return;
        }
        const double estimate = fees.value(selectedKey).toDouble();
        if (!std::isfinite(estimate) || estimate <= 0.0 || estimate > 10000.0) {
            fail(QStringLiteral("Bitcoin construction blocked · fee estimate is outside the safety range"), generation);
            return;
        }
        m_feeRate = static_cast<quint64>(std::ceil(estimate));
        if (m_feeRate < 1) m_feeRate = 1;
        m_feeTarget = selectedKey + QStringLiteral(" blocks");
        m_providerFeeEstimate = QStringLiteral("%1 sat/vB").arg(
            QString::number(estimate, 'f', 3));
        m_feeRateSatVb = QStringLiteral("%1 sat/vB · conservative ceiling").arg(m_feeRate);
        m_haveFee = true;
    }

    if (m_pending > 0)
        --m_pending;
    maybeFinalize(generation);
}

void BitcoinUnsignedTransactionService::maybeFinalize(quint64 generation)
{
    if (generation != m_generation || m_pending != 0 || !m_haveGenesis || !m_haveUtxos || !m_haveFee)
        return;
    if (m_utxos.isEmpty()) {
        fail(QStringLiteral("Bitcoin construction blocked · no confirmed spendable UTXOs for this public development address"), generation);
        return;
    }

    const BuildResult result = buildTransaction(m_utxos, m_amountSats, m_feeRate,
                                                m_sourceScript, m_destinationScript);
    if (!result.ok) {
        fail(QStringLiteral("Bitcoin construction blocked · %1").arg(result.error), generation);
        return;
    }

    m_selectedInputCount = result.selected.size();
    QStringList lines;
    for (const Utxo &u : result.selected) {
        lines << QStringLiteral("%1:%2 · %3").arg(u.txid).arg(u.vout).arg(formatBtc(u.value));
    }
    m_selectedInputsSummary = lines.join(QLatin1Char('\n'));
    m_totalInputBtc = formatBtc(result.totalInput);
    m_amountBtc = formatBtc(result.amount);
    m_networkFeeBtc = formatBtc(result.fee);
    m_changeBtc = formatBtc(result.change);
    m_estimatedVbytes = QStringLiteral("%1 vB · conservative P2WPKH estimate")
        .arg(result.estimatedVbytes);
    m_changePolicy = result.changeOutput
        ? QStringLiteral("Development-only change returns to the public source address; dedicated secure change derivation is a later signing prerequisite")
        : QStringLiteral("No change output · sub-dust residual added to fee");
    m_unsignedTransactionHex = QString::fromLatin1(result.unsignedTx.toHex());
    m_unsignedTxid = result.txid;
    m_psbtBase64 = QString::fromLatin1(result.psbt.toBase64());
    m_constructionFingerprint = constructionFingerprintFor(
        m_intentFingerprint, result.unsignedTx, result.psbt);
    m_constructedAt = QDateTime::currentDateTimeUtc().toString(Qt::ISODate);
    m_loading = false;
    m_constructed = true;
    m_status = QStringLiteral("Unsigned Bitcoin transaction and PSBT constructed · signing remains disabled");
    emit stateChanged();
}

QVariantMap BitcoinUnsignedTransactionService::runSelfTest() const
{
    QVariantMap out;
    quint64 sats = 0;
    const bool amountPassed = amountToSats(QStringLiteral("0.00100000"), &sats)
        && sats == 100000ULL
        && !amountToSats(QStringLiteral("1e-3"), &sats)
        && !amountToSats(QStringLiteral("0.000000001"), &sats);

    QByteArray sourceScript;
    QByteArray destinationScript;
    bool sourceP2wpkh = false;
    const bool scriptsPassed = lockScriptForAddress(
            QStringLiteral("bc1qcr8te4kr609gcawutmrza0j4xv80jy8z306fyu"),
            &sourceScript, &sourceP2wpkh)
        && sourceP2wpkh
        && sourceScript.toHex() == QByteArray("0014c0cebcd6c3d3ca8c75dc5ec62ebe55330ef910e2")
        && lockScriptForAddress(
            QStringLiteral("bc1qnjg0jd8228aq7egyzacy8cys3knf9xvrerkf9g"),
            &destinationScript)
        && destinationScript.toHex() == QByteArray("00149c90f934ea51fa0f6504177043e0908da6929983");

    QVector<Utxo> sample;
    Utxo a; a.txid = QString(63, QLatin1Char('0')) + QLatin1Char('1'); a.vout = 0; a.value = 120000ULL; sample << a;
    Utxo b; b.txid = QString(63, QLatin1Char('0')) + QLatin1Char('2'); b.vout = 1; b.value = 70000ULL; sample << b;
    Utxo c; c.txid = QString(63, QLatin1Char('0')) + QLatin1Char('3'); c.vout = 2; c.value = 30000ULL; sample << c;

    const BuildResult built = scriptsPassed
        ? buildTransaction(sample, 100000ULL, 2ULL, sourceScript, destinationScript)
        : BuildResult();
    const bool selectionPassed = built.ok && built.selected.size() == 1
        && built.totalInput == 120000ULL && built.fee == 282ULL
        && built.change == 19718ULL && built.estimatedVbytes == 141ULL;
    const bool serializationPassed = built.ok
        && built.unsignedTx.toHex() == QByteArray("020000000101000000000000000000000000000000000000000000000000000000000000000000000000fdffffff02a0860100000000001600149c90f934ea51fa0f6504177043e0908da6929983064d000000000000160014c0cebcd6c3d3ca8c75dc5ec62ebe55330ef910e200000000")
        && built.txid == QStringLiteral("f18ce6a56e98470ea777d3d16d963c8f6ffc1cc2f2095948e45e109a179bbff9");
    const bool psbtPassed = built.ok && built.psbt.startsWith(QByteArray("psbt\xff", 5))
        && built.psbt.toBase64() == QByteArray("cHNidP8BAHECAAAAAQEAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAD9////AqCGAQAAAAAAFgAUnJD5NOpR+g9lBBdwQ+CQjaaSmYMGTQAAAAAAABYAFMDOvNbD08qMddxexi6+VTMO+RDiAAAAAAABAR/A1AEAAAAAABYAFMDOvNbD08qMddxexi6+VTMO+RDiAQMEAQAAAAAAAA==")
        && QByteArray::fromBase64(built.psbt.toBase64()) == built.psbt;

    const QString fp = built.ok ? constructionFingerprintFor(
        QString(64, QLatin1Char('a')), built.unsignedTx, built.psbt) : QString();
    QByteArray mutated = built.unsignedTx;
    if (!mutated.isEmpty()) { const int i = mutated.size() - 1; mutated[i] = static_cast<char>(mutated.at(i) ^ 0x01); }
    const bool bindingPassed = fp == QStringLiteral("835d224f71e950d9b4126b59033610caac7bb0100ce3d114af168b83e0c7a46a")
        && fp != constructionFingerprintFor(
            QString(64, QLatin1Char('a')), mutated, built.psbt);

    const bool passed = amountPassed && scriptsPassed && selectionPassed
        && serializationPassed && psbtPassed && bindingPassed;
    out.insert(QStringLiteral("passed"), passed);
    out.insert(QStringLiteral("exactAmountPassed"), amountPassed);
    out.insert(QStringLiteral("walletCoreScriptPassed"), scriptsPassed);
    out.insert(QStringLiteral("coinSelectionPassed"), selectionPassed);
    out.insert(QStringLiteral("serializationVectorPassed"), serializationPassed);
    out.insert(QStringLiteral("psbtPassed"), psbtPassed);
    out.insert(QStringLiteral("constructionFingerprintPassed"), bindingPassed);
    out.insert(QStringLiteral("detail"), passed
        ? QStringLiteral("Exact sats, Wallet Core scripts, deterministic largest-first selection, conservative P2WPKH fee sizing, unsigned serialization, PSBT v0 and SHA-256 binding passed")
        : QStringLiteral("One or more Bitcoin unsigned-construction checks failed"));
    return out;
}
