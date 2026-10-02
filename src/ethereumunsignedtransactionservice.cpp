#include "ethereumunsignedtransactionservice.h"

#include "networkrequestutils.h"
#include "unsignedtransactionservice.h"

#include <QCryptographicHash>
#include <QDateTime>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QRegularExpression>
#include <QTimer>
#include <QUrl>

#include <TrustWalletCore/TWData.h>
#include <TrustWalletCore/TWHash.h>

#include <algorithm>
#include <limits>

namespace {
const int kMaximumDnsRetries = 1;
const int kDnsRetryDelayMs = 900;

class TWDataGuard
{
public:
    explicit TWDataGuard(TWData *value = nullptr) : m_value(value) {}
    ~TWDataGuard() { if (m_value) TWDataDelete(m_value); }
    TWData *get() const { return m_value; }
private:
    TWData *m_value;
};

QByteArray minimalBigEndian(quint64 value)
{
    QByteArray bytes;
    while (value > 0) {
        bytes.prepend(static_cast<char>(value & 0xffU));
        value >>= 8U;
    }
    return bytes;
}

QByteArray rlpLengthPrefix(int length, unsigned char shortBase, unsigned char longBase)
{
    if (length <= 55)
        return QByteArray(1, static_cast<char>(shortBase + length));

    const QByteArray lengthBytes = minimalBigEndian(static_cast<quint64>(length));
    QByteArray prefix(1, static_cast<char>(longBase + lengthBytes.size()));
    prefix += lengthBytes;
    return prefix;
}

QByteArray rlpBytes(const QByteArray &bytes)
{
    if (bytes.size() == 1
            && static_cast<unsigned char>(bytes.at(0)) < 0x80U) {
        return bytes;
    }
    return rlpLengthPrefix(bytes.size(), 0x80U, 0xb7U) + bytes;
}

QByteArray rlpList(const QList<QByteArray> &encodedItems)
{
    QByteArray payload;
    for (const QByteArray &item : encodedItems)
        payload += item;
    return rlpLengthPrefix(payload.size(), 0xc0U, 0xf7U) + payload;
}

QString stripLeadingZeros(const QString &value)
{
    int first = 0;
    while (first + 1 < value.size() && value.at(first) == QLatin1Char('0'))
        ++first;
    return value.mid(first);
}

QByteArray decimalDigitsToBytes(QString decimal)
{
    decimal = stripLeadingZeros(decimal);
    if (decimal.isEmpty() || decimal == QStringLiteral("0"))
        return QByteArray();

    QByteArray littleEndian;
    while (decimal != QStringLiteral("0")) {
        QString quotient;
        quotient.reserve(decimal.size());
        int carry = 0;
        bool emitted = false;

        for (int i = 0; i < decimal.size(); ++i) {
            const int digit = decimal.at(i).unicode() - QLatin1Char('0').unicode();
            const int current = carry * 10 + digit;
            const int q = current / 256;
            carry = current % 256;
            if (q != 0 || emitted) {
                quotient.append(QChar(QLatin1Char('0').unicode() + q));
                emitted = true;
            }
        }

        littleEndian.append(static_cast<char>(carry));
        decimal = quotient.isEmpty() ? QStringLiteral("0") : quotient;
    }

    std::reverse(littleEndian.begin(), littleEndian.end());
    return littleEndian;
}

QString uint128ToDecimal(unsigned __int128 value)
{
    if (value == 0)
        return QStringLiteral("0");

    QString result;
    while (value > 0) {
        const int digit = static_cast<int>(value % 10U);
        result.prepend(QChar(QLatin1Char('0').unicode() + digit));
        value /= 10U;
    }
    return result;
}

QString decimalUnitsToFixed(QString digits, int decimals, int maximumFractionDigits)
{
    digits = stripLeadingZeros(digits);
    if (digits.isEmpty())
        digits = QStringLiteral("0");

    if (decimals <= 0)
        return digits;

    if (digits.size() <= decimals)
        digits.prepend(QString(decimals + 1 - digits.size(), QLatin1Char('0')));

    QString whole = digits.left(digits.size() - decimals);
    QString fraction = digits.right(decimals);
    if (maximumFractionDigits >= 0 && fraction.size() > maximumFractionDigits)
        fraction = fraction.left(maximumFractionDigits);
    while (!fraction.isEmpty() && fraction.endsWith(QLatin1Char('0')))
        fraction.chop(1);

    return fraction.isEmpty() ? whole : whole + QLatin1Char('.') + fraction;
}

QString rpcErrorMessage(const QJsonObject &object, const QString &fallback)
{
    const QJsonObject error = object.value(QStringLiteral("error")).toObject();
    return error.value(QStringLiteral("message")).toString(fallback);
}

} // namespace

EthereumUnsignedTransactionService::EthereumUnsignedTransactionService(QObject *parent)
    : QObject(parent)
{
    m_status = QStringLiteral("Ethereum transaction not constructed");
}

bool EthereumUnsignedTransactionService::loading() const { return m_loading; }
bool EthereumUnsignedTransactionService::constructed() const { return m_constructed; }
QString EthereumUnsignedTransactionService::status() const { return m_status; }
QString EthereumUnsignedTransactionService::providerHost() const { return m_providerHost; }
QString EthereumUnsignedTransactionService::intentFingerprint() const { return m_intentFingerprint; }
QString EthereumUnsignedTransactionService::chainId() const { return m_chainIdText; }
QString EthereumUnsignedTransactionService::nonce() const { return m_nonceText; }
QString EthereumUnsignedTransactionService::gasLimit() const { return m_gasLimitText; }
QString EthereumUnsignedTransactionService::baseFeeGwei() const { return m_baseFeeGwei; }
QString EthereumUnsignedTransactionService::maxPriorityFeeGwei() const { return m_maxPriorityFeeGwei; }
QString EthereumUnsignedTransactionService::maxFeeGwei() const { return m_maxFeeGwei; }
QString EthereumUnsignedTransactionService::maximumNetworkFeeEth() const { return m_maximumNetworkFeeEth; }
QString EthereumUnsignedTransactionService::unsignedPayloadHex() const { return m_unsignedPayloadHex; }
QString EthereumUnsignedTransactionService::signingHashHex() const { return m_signingHashHex; }
QString EthereumUnsignedTransactionService::constructionFingerprint() const { return m_constructionFingerprint; }
QString EthereumUnsignedTransactionService::constructedAt() const { return m_constructedAt; }
QString EthereumUnsignedTransactionService::feeSource() const { return m_feeSource; }

void EthereumUnsignedTransactionService::clearState(bool preserveStatus)
{
    m_loading = false;
    m_constructed = false;
    m_pendingInputs = 0;
    m_intent.clear();
    m_endpoint.clear();
    m_providerHost.clear();
    m_intentFingerprint.clear();
    if (!preserveStatus)
        m_status = QStringLiteral("Ethereum transaction not constructed");
    m_chainIdText.clear();
    m_nonceText.clear();
    m_gasLimitText.clear();
    m_baseFeeGwei.clear();
    m_maxPriorityFeeGwei.clear();
    m_maxFeeGwei.clear();
    m_maximumNetworkFeeEth.clear();
    m_unsignedPayloadHex.clear();
    m_signingHashHex.clear();
    m_constructionFingerprint.clear();
    m_constructedAt.clear();
    m_feeSource.clear();
    m_chainId = 0;
    m_nonce = 0;
    m_baseFeePerGas = 0;
    m_priorityFeePerGas = 0;
    m_fallbackGasPrice = 0;
    m_blockGasLimit = 0;
    m_gasLimit = 0;
    m_maxFeePerGas = 0;
    m_constructedEpochMs = 0;
    m_valueBytes.clear();
}

void EthereumUnsignedTransactionService::reset()
{
    ++m_generation;
    SailVaultNetwork::cancelOutstanding(&m_network);
    clearState();
    emit stateChanged();
}

void EthereumUnsignedTransactionService::construct(const QVariantMap &intent,
                                                    const QString &ethereumRpcUrl)
{
    ++m_generation;
    const quint64 generation = m_generation;
    SailVaultNetwork::cancelOutstanding(&m_network);
    clearState();

    if (SailVaultNetwork::offlineModeEnabled()) {
        m_status = QStringLiteral("Offline mode enabled · Ethereum construction needs fresh public network inputs");
        emit stateChanged();
        return;
    }

    const QUrl endpoint(ethereumRpcUrl.trimmed());
    if (!SailVaultNetwork::isValidHttpsEndpoint(endpoint)) {
        m_status = QStringLiteral("Configured Ethereum RPC endpoint is not a valid HTTPS URL");
        emit stateChanged();
        return;
    }

    QString intentError;
    if (!UnsignedTransactionService::validateReviewModel(intent, &intentError)
            || intent.value(QStringLiteral("chain")).toString() != QStringLiteral("Ethereum")) {
        m_status = intentError.isEmpty()
            ? QStringLiteral("M49 constructs Ethereum intents only")
            : QStringLiteral("Intent validation failed · %1").arg(intentError);
        emit stateChanged();
        return;
    }

    if (!amountToAtomicBytes(intent.value(QStringLiteral("normalizedAmount")).toString(),
                             18, &m_valueBytes)) {
        m_status = QStringLiteral("Ethereum amount cannot be represented as a 256-bit wei value");
        emit stateChanged();
        return;
    }

    m_intent = intent;
    m_endpoint = endpoint.toString();
    m_providerHost = endpoint.host();
    m_intentFingerprint = intent.value(QStringLiteral("fingerprint")).toString();
    m_loading = true;
    m_status = QStringLiteral("Fetching fresh Ethereum nonce, fee and recipient-account inputs…");
    m_pendingInputs = 5;
    emit stateChanged();

    sendRpc(ChainIdentity, QStringLiteral("eth_chainId"), QJsonArray(),
            4901, generation);

    QJsonArray nonceParams;
    nonceParams.append(intent.value(QStringLiteral("sourceAddress")).toString());
    nonceParams.append(QStringLiteral("pending"));
    sendRpc(PendingNonce, QStringLiteral("eth_getTransactionCount"),
            nonceParams, 4902, generation);

    QJsonArray blockParams;
    blockParams.append(QStringLiteral("latest"));
    blockParams.append(false);
    sendRpc(LatestBlock, QStringLiteral("eth_getBlockByNumber"),
            blockParams, 4903, generation);

    sendRpc(PriorityFee, QStringLiteral("eth_maxPriorityFeePerGas"),
            QJsonArray(), 4904, generation);

    QJsonArray codeParams;
    codeParams.append(intent.value(QStringLiteral("destinationAddress")).toString());
    codeParams.append(QStringLiteral("latest"));
    sendRpc(DestinationCode, QStringLiteral("eth_getCode"),
            codeParams, 4905, generation);
}

void EthereumUnsignedTransactionService::fail(const QString &message,
                                               quint64 generation)
{
    if (generation != m_generation)
        return;
    ++m_generation;
    SailVaultNetwork::cancelOutstanding(&m_network);
    m_loading = false;
    m_constructed = false;
    m_pendingInputs = 0;
    m_status = message;
    emit stateChanged();
}

void EthereumUnsignedTransactionService::sendRpc(RpcKind kind,
                                                  const QString &method,
                                                  const QJsonArray &params,
                                                  int requestId,
                                                  quint64 generation,
                                                  int attempt)
{
    if (generation != m_generation)
        return;

    QNetworkRequest request{QUrl(m_endpoint)};
    request.setHeader(QNetworkRequest::ContentTypeHeader,
                      QStringLiteral("application/json"));
    request.setRawHeader("Accept", "application/json");

    QJsonObject body;
    body.insert(QStringLiteral("jsonrpc"), QStringLiteral("2.0"));
    body.insert(QStringLiteral("id"), requestId);
    body.insert(QStringLiteral("method"), method);
    body.insert(QStringLiteral("params"), params);

    SailVaultNetwork::hardenRequest(request);
    QNetworkReply *reply = m_network.post(
        request, QJsonDocument(body).toJson(QJsonDocument::Compact));
    SailVaultNetwork::armTimeout(reply, kRequestTimeoutMs, kMaxResponseBytes);

    connect(reply, &QNetworkReply::finished, this,
            [this, reply, kind, method, params, requestId, generation, attempt]() {
        const QNetworkReply::NetworkError errorCode = reply->error();
        const QString networkError = SailVaultNetwork::jsonErrorText(
            reply, kRequestTimeoutMs, kMaxResponseBytes);
        const QByteArray data = reply->readAll();
        reply->deleteLater();

        if (generation != m_generation)
            return;

        if (errorCode == QNetworkReply::HostNotFoundError
                && attempt < kMaximumDnsRetries) {
            QTimer::singleShot(kDnsRetryDelayMs, this,
                               [this, kind, method, params, requestId,
                                generation, attempt]() {
                sendRpc(kind, method, params, requestId,
                        generation, attempt + 1);
            });
            return;
        }

        if (!networkError.isEmpty()) {
            fail(QStringLiteral("Ethereum construction failed · %1").arg(networkError),
                 generation);
            return;
        }

        QJsonParseError parseError;
        const QJsonDocument document = QJsonDocument::fromJson(data, &parseError);
        if (parseError.error != QJsonParseError::NoError || !document.isObject()) {
            fail(QStringLiteral("Ethereum construction failed · invalid JSON-RPC response"),
                 generation);
            return;
        }

        const QJsonObject object = document.object();
        QString envelopeError;
        if (!SailVaultNetwork::validateJsonRpcEnvelope(
                object, QJsonValue(requestId), &envelopeError)) {
            fail(QStringLiteral("Ethereum construction failed · %1").arg(envelopeError),
                 generation);
            return;
        }

        if (object.contains(QStringLiteral("error"))) {
            if (kind == PriorityFee) {
                // eth_maxPriorityFeePerGas is widely implemented but is not
                // required by JSON-RPC itself. Fall back to eth_gasPrice and
                // derive the tip from the latest block base fee.
                sendRpc(PriorityFeeFallback, QStringLiteral("eth_gasPrice"),
                        QJsonArray(), 4906, generation);
                return;
            }
            fail(QStringLiteral("Ethereum construction failed · %1")
                     .arg(rpcErrorMessage(object,
                          QStringLiteral("provider returned an RPC error"))),
                 generation);
            return;
        }

        handleRpcResult(kind, object, generation);
    });
}

void EthereumUnsignedTransactionService::handleRpcResult(
        RpcKind kind, const QJsonObject &object, quint64 generation)
{
    if (generation != m_generation)
        return;

    quint64 value = 0;
    QString canonical;

    if (kind == ChainIdentity) {
        if (!parseRpcQuantity(object.value(QStringLiteral("result")),
                              &value, &canonical)
                || value != 1) {
            fail(QStringLiteral("Ethereum construction blocked · RPC is not Ethereum mainnet chain ID 1"),
                 generation);
            return;
        }
        m_chainId = value;
        m_chainIdText = QStringLiteral("1 (%1)").arg(canonical);
    } else if (kind == PendingNonce) {
        if (!parseRpcQuantity(object.value(QStringLiteral("result")),
                              &value, &canonical)) {
            fail(QStringLiteral("Ethereum construction failed · invalid pending nonce"),
                 generation);
            return;
        }
        m_nonce = value;
        m_nonceText = QStringLiteral("%1 (%2)").arg(value).arg(canonical);
    } else if (kind == LatestBlock) {
        const QJsonValue result = object.value(QStringLiteral("result"));
        if (!result.isObject()) {
            fail(QStringLiteral("Ethereum construction failed · latest block is not an object"),
                 generation);
            return;
        }
        const QJsonObject block = result.toObject();
        if (!parseRpcQuantity(block.value(QStringLiteral("baseFeePerGas")),
                              &m_baseFeePerGas)
                || !parseRpcQuantity(block.value(QStringLiteral("gasLimit")),
                                     &m_blockGasLimit)
                || m_blockGasLimit == 0) {
            fail(QStringLiteral("Ethereum construction failed · latest block lacks valid EIP-1559 fee/gas data"),
                 generation);
            return;
        }
        m_baseFeeGwei = formatGwei(m_baseFeePerGas);
    } else if (kind == PriorityFee) {
        if (!parseRpcQuantity(object.value(QStringLiteral("result")),
                              &m_priorityFeePerGas)) {
            fail(QStringLiteral("Ethereum construction failed · invalid priority fee"),
                 generation);
            return;
        }
        m_feeSource = QStringLiteral("eth_maxPriorityFeePerGas + latest base fee");
        m_maxPriorityFeeGwei = formatGwei(m_priorityFeePerGas);
    } else if (kind == PriorityFeeFallback) {
        if (!parseRpcQuantity(object.value(QStringLiteral("result")),
                              &m_fallbackGasPrice)) {
            fail(QStringLiteral("Ethereum construction failed · invalid gas-price fallback"),
                 generation);
            return;
        }
        m_feeSource = QStringLiteral("eth_gasPrice fallback + latest base fee");
    } else if (kind == DestinationCode) {
        if (!object.value(QStringLiteral("result")).isString()) {
            fail(QStringLiteral("Ethereum construction failed · invalid eth_getCode response"),
                 generation);
            return;
        }
        const QString code = object.value(QStringLiteral("result")).toString().trimmed();
        static const QRegularExpression codePattern(QStringLiteral("^0x[0-9a-fA-F]*$"));
        if (!codePattern.match(code).hasMatch()) {
            fail(QStringLiteral("Ethereum construction failed · malformed destination code"),
                 generation);
            return;
        }
        if (code.compare(QStringLiteral("0x"), Qt::CaseInsensitive) != 0) {
            fail(QStringLiteral("M49 blocks contract/delegated-code recipients · plain EOA ETH transfers only"),
                 generation);
            return;
        }
        m_gasLimit = 21000;
        m_gasLimitText = QStringLiteral("21000 · plain EOA transfer");
    }

    completeInput(kind, generation);
}

void EthereumUnsignedTransactionService::completeInput(RpcKind kind,
                                                        quint64 generation)
{
    if (generation != m_generation)
        return;

    if (kind == PriorityFeeFallback) {
        // The failed priority-fee request and this fallback together represent
        // one logical input, so do not decrement twice.
        }

    if (m_pendingInputs > 0)
        --m_pendingInputs;

    if (m_pendingInputs == 0)
        finalizeConstruction(generation);
}

void EthereumUnsignedTransactionService::finalizeConstruction(quint64 generation)
{
    if (generation != m_generation)
        return;

    if (m_chainId != 1 || m_blockGasLimit == 0 || m_gasLimit != 21000) {
        fail(QStringLiteral("Ethereum construction failed · incomplete network snapshot"),
             generation);
        return;
    }

    if (m_gasLimit > m_blockGasLimit) {
        fail(QStringLiteral("Ethereum construction blocked · estimated gas exceeds latest block gas limit"),
             generation);
        return;
    }

    if (m_feeSource.startsWith(QStringLiteral("eth_gasPrice"))) {
        m_priorityFeePerGas = m_fallbackGasPrice > m_baseFeePerGas
            ? m_fallbackGasPrice - m_baseFeePerGas : 0;
        m_maxPriorityFeeGwei = formatGwei(m_priorityFeePerGas);
    }

    if (m_baseFeePerGas > (std::numeric_limits<quint64>::max()
                           - m_priorityFeePerGas) / 2U) {
        fail(QStringLiteral("Ethereum construction blocked · fee arithmetic overflow"),
             generation);
        return;
    }
    const quint64 maxFeePerGas = m_baseFeePerGas * 2U + m_priorityFeePerGas;
    m_maxFeePerGas = maxFeePerGas;
    m_maxFeeGwei = formatGwei(maxFeePerGas);

    QString encodeError;
    const QByteArray payload = unsignedEip1559Payload(
        m_chainId,
        m_nonce,
        m_priorityFeePerGas,
        maxFeePerGas,
        m_gasLimit,
        m_intent.value(QStringLiteral("destinationAddress")).toString(),
        m_valueBytes,
        &encodeError);
    if (payload.isEmpty()) {
        fail(QStringLiteral("Ethereum construction failed · %1").arg(encodeError),
             generation);
        return;
    }

    const QString signingHash = keccak256Hex(payload);
    if (signingHash.size() != 64) {
        fail(QStringLiteral("Ethereum construction failed · Wallet Core Keccak-256 unavailable"),
             generation);
        return;
    }

    const unsigned __int128 maximumFeeWei =
        static_cast<unsigned __int128>(m_gasLimit)
        * static_cast<unsigned __int128>(maxFeePerGas);

    m_unsignedPayloadHex = QStringLiteral("0x")
        + QString::fromLatin1(payload.toHex());
    m_signingHashHex = QStringLiteral("0x") + signingHash;
    m_constructionFingerprint = constructionFingerprintFor(
        m_intentFingerprint, payload);
    m_maximumNetworkFeeEth = formatWeiAsEth128(maximumFeeWei);
    m_constructedEpochMs = QDateTime::currentMSecsSinceEpoch();
    m_constructedAt = QDateTime::fromMSecsSinceEpoch(m_constructedEpochMs).toString(
        QStringLiteral("yyyy-MM-dd HH:mm:ss"));
    m_loading = false;
    m_constructed = true;
    m_status = QStringLiteral(
        "Unsigned EIP-1559 signing payload constructed · signing and broadcasting remain disabled");
    emit stateChanged();
}

bool EthereumUnsignedTransactionService::parseRpcQuantity(
        const QJsonValue &value, quint64 *number, QString *canonicalHex)
{
    if (!value.isString() || !number)
        return false;

    QString text = value.toString().trimmed();
    static const QRegularExpression quantityPattern(
        QStringLiteral("^0x(?:0|[1-9a-fA-F][0-9a-fA-F]*)$"));
    if (!quantityPattern.match(text).hasMatch())
        return false;

    bool ok = false;
    const QString digits = text.mid(2);
    const quint64 parsed = digits.toULongLong(&ok, 16);
    if (!ok)
        return false;

    *number = parsed;
    if (canonicalHex)
        *canonicalHex = QStringLiteral("0x") + QString::number(parsed, 16);
    return true;
}

bool EthereumUnsignedTransactionService::amountToAtomicBytes(
        const QString &amount, int decimals, QByteArray *bytes,
        QString *atomicDecimal)
{
    if (!bytes || decimals < 0)
        return false;

    const QString value = amount.trimmed();
    static const QRegularExpression amountPattern(
        QStringLiteral("^[0-9]+(?:\\.[0-9]+)?$"));
    if (!amountPattern.match(value).hasMatch())
        return false;

    const int point = value.indexOf(QLatin1Char('.'));
    QString whole = point >= 0 ? value.left(point) : value;
    QString fraction = point >= 0 ? value.mid(point + 1) : QString();
    if (fraction.size() > decimals)
        return false;

    whole = stripLeadingZeros(whole);
    fraction += QString(decimals - fraction.size(), QLatin1Char('0'));
    QString digits = stripLeadingZeros(whole + fraction);
    if (digits.isEmpty())
        digits = QStringLiteral("0");
    // 2^256 - 1 has 78 decimal digits. Refuse obviously oversized
    // user input before running the decimal division loop.
    if (digits.size() > 78)
        return false;

    QByteArray result = decimalDigitsToBytes(digits);
    if (result.size() > 32 || result.isEmpty())
        return false;

    *bytes = result;
    if (atomicDecimal)
        *atomicDecimal = digits;
    return true;
}

QByteArray EthereumUnsignedTransactionService::unsignedEip1559Payload(
        quint64 chainId,
        quint64 nonce,
        quint64 maxPriorityFeePerGas,
        quint64 maxFeePerGas,
        quint64 gasLimit,
        const QString &destinationAddress,
        const QByteArray &value,
        QString *error)
{
    QString address = destinationAddress.trimmed();
    if (address.startsWith(QStringLiteral("0x"), Qt::CaseInsensitive))
        address.remove(0, 2);
    static const QRegularExpression addressPattern(
        QStringLiteral("^[0-9a-fA-F]{40}$"));
    if (!addressPattern.match(address).hasMatch()) {
        if (error)
            *error = QStringLiteral("destination address is not 20 bytes");
        return QByteArray();
    }

    if (chainId == 0 || gasLimit == 0 || maxFeePerGas < maxPriorityFeePerGas
            || value.isEmpty() || value.size() > 32) {
        if (error)
            *error = QStringLiteral("invalid EIP-1559 transaction fields");
        return QByteArray();
    }

    const QByteArray to = QByteArray::fromHex(address.toLatin1());
    if (to.size() != 20) {
        if (error)
            *error = QStringLiteral("destination address decoding failed");
        return QByteArray();
    }

    QList<QByteArray> fields;
    fields << rlpBytes(minimalBigEndian(chainId));
    fields << rlpBytes(minimalBigEndian(nonce));
    fields << rlpBytes(minimalBigEndian(maxPriorityFeePerGas));
    fields << rlpBytes(minimalBigEndian(maxFeePerGas));
    fields << rlpBytes(minimalBigEndian(gasLimit));
    fields << rlpBytes(to);
    fields << rlpBytes(value);
    fields << rlpBytes(QByteArray()); // data = 0x
    fields << rlpList(QList<QByteArray>()); // empty access list

    QByteArray payload(1, static_cast<char>(0x02));
    payload += rlpList(fields);
    if (error)
        error->clear();
    return payload;
}

QString EthereumUnsignedTransactionService::keccak256Hex(const QByteArray &data)
{
    if (data.isEmpty())
        return QString();

    TWDataGuard input(TWDataCreateWithBytes(
        reinterpret_cast<const uint8_t *>(data.constData()),
        static_cast<size_t>(data.size())));
    if (!input.get())
        return QString();

    TWDataGuard digest(TWHashKeccak256(input.get()));
    if (!digest.get() || TWDataSize(digest.get()) != 32)
        return QString();

    const QByteArray bytes(
        reinterpret_cast<const char *>(TWDataBytes(digest.get())),
        static_cast<int>(TWDataSize(digest.get())));
    return QString::fromLatin1(bytes.toHex());
}

QString EthereumUnsignedTransactionService::formatGwei(quint64 wei)
{
    const QString digits = QString::number(wei);
    return decimalUnitsToFixed(digits, 9, 9) + QStringLiteral(" Gwei");
}

QString EthereumUnsignedTransactionService::formatWeiAsEth128(
        unsigned __int128 wei)
{
    return decimalUnitsToFixed(uint128ToDecimal(wei), 18, 18)
        + QStringLiteral(" ETH");
}

QString EthereumUnsignedTransactionService::constructionFingerprintFor(
        const QString &intentFingerprint, const QByteArray &payload)
{
    const QByteArray canonical = QByteArray("sailvault-eth-unsigned-v1\n")
        + intentFingerprint.toLatin1() + '\n'
        + payload.toHex();
    return QString::fromLatin1(QCryptographicHash::hash(
        canonical, QCryptographicHash::Sha256).toHex());
}

QVariantMap EthereumUnsignedTransactionService::signingSnapshot() const
{
    QVariantMap snapshot;
    if (!m_constructed)
        return snapshot;

    snapshot.insert(QStringLiteral("snapshotVersion"), 1);
    snapshot.insert(QStringLiteral("chain"), QStringLiteral("Ethereum"));
    snapshot.insert(QStringLiteral("network"),
                    QStringLiteral("Ethereum mainnet · chain ID 1"));
    snapshot.insert(QStringLiteral("intentFingerprint"), m_intentFingerprint);
    snapshot.insert(QStringLiteral("chainId"), QString::number(m_chainId));
    snapshot.insert(QStringLiteral("nonce"), QString::number(m_nonce));
    snapshot.insert(QStringLiteral("maxPriorityFeePerGas"),
                    QString::number(m_priorityFeePerGas));
    snapshot.insert(QStringLiteral("maxFeePerGas"), QString::number(m_maxFeePerGas));
    snapshot.insert(QStringLiteral("gasLimit"), QString::number(m_gasLimit));
    snapshot.insert(QStringLiteral("unsignedPayloadHex"), m_unsignedPayloadHex);
    snapshot.insert(QStringLiteral("signingHashHex"), m_signingHashHex);
    snapshot.insert(QStringLiteral("constructionFingerprint"),
                    m_constructionFingerprint);
    snapshot.insert(QStringLiteral("constructedEpochMs"), m_constructedEpochMs);
    return snapshot;
}

bool EthereumUnsignedTransactionService::validateSigningSnapshot(
        const QVariantMap &intent,
        const QVariantMap &snapshot,
        QString *error)
{
    const auto fail = [error](const QString &message) {
        if (error)
            *error = message;
        return false;
    };

    QString intentError;
    if (!UnsignedTransactionService::validateReviewModel(intent, &intentError))
        return fail(QStringLiteral("reviewed intent rejected: %1").arg(intentError));

    if (intent.value(QStringLiteral("chain")).toString() != QStringLiteral("Ethereum")
            || snapshot.value(QStringLiteral("snapshotVersion")).toInt() != 1
            || snapshot.value(QStringLiteral("chain")).toString() != QStringLiteral("Ethereum")
            || snapshot.value(QStringLiteral("network")).toString()
               != QStringLiteral("Ethereum mainnet · chain ID 1")) {
        return fail(QStringLiteral("snapshot is not SailVault Ethereum mainnet v1"));
    }

    if (snapshot.value(QStringLiteral("intentFingerprint")).toString()
            != intent.value(QStringLiteral("fingerprint")).toString()) {
        return fail(QStringLiteral("construction is not bound to this reviewed intent"));
    }

    const auto parseDecimalU64 = [](const QVariant &value, quint64 *out) {
        if (!out)
            return false;
        const QString text = value.toString();
        static const QRegularExpression pattern(QStringLiteral("^(?:0|[1-9][0-9]*)$"));
        if (!pattern.match(text).hasMatch())
            return false;
        bool ok = false;
        const quint64 parsed = text.toULongLong(&ok, 10);
        if (!ok)
            return false;
        *out = parsed;
        return true;
    };

    quint64 chainId = 0;
    quint64 nonce = 0;
    quint64 priorityFee = 0;
    quint64 maxFee = 0;
    quint64 gasLimit = 0;
    if (!parseDecimalU64(snapshot.value(QStringLiteral("chainId")), &chainId)
            || !parseDecimalU64(snapshot.value(QStringLiteral("nonce")), &nonce)
            || !parseDecimalU64(snapshot.value(QStringLiteral("maxPriorityFeePerGas")), &priorityFee)
            || !parseDecimalU64(snapshot.value(QStringLiteral("maxFeePerGas")), &maxFee)
            || !parseDecimalU64(snapshot.value(QStringLiteral("gasLimit")), &gasLimit)
            || chainId != 1 || gasLimit != 21000 || maxFee < priorityFee) {
        return fail(QStringLiteral("snapshot fee/nonce/mainnet fields are invalid"));
    }

    QByteArray valueBytes;
    if (!amountToAtomicBytes(intent.value(QStringLiteral("normalizedAmount")).toString(),
                             18, &valueBytes)) {
        return fail(QStringLiteral("reviewed ETH amount cannot be converted to wei"));
    }

    QString encodeError;
    const QByteArray expectedPayload = unsignedEip1559Payload(
        chainId, nonce, priorityFee, maxFee, gasLimit,
        intent.value(QStringLiteral("destinationAddress")).toString(),
        valueBytes, &encodeError);
    if (expectedPayload.isEmpty())
        return fail(QStringLiteral("snapshot payload reconstruction failed: %1").arg(encodeError));

    QString payloadHex = snapshot.value(QStringLiteral("unsignedPayloadHex")).toString().trimmed();
    if (payloadHex.startsWith(QStringLiteral("0x"), Qt::CaseInsensitive))
        payloadHex.remove(0, 2);
    static const QRegularExpression payloadPattern(QStringLiteral("^[0-9a-fA-F]+$"));
    if (!payloadPattern.match(payloadHex).hasMatch()
            || (payloadHex.size() % 2) != 0
            || QByteArray::fromHex(payloadHex.toLatin1()) != expectedPayload) {
        return fail(QStringLiteral("unsigned payload does not match reviewed destination/amount and network inputs"));
    }

    const QString expectedHash = QStringLiteral("0x") + keccak256Hex(expectedPayload);
    if (expectedHash.size() != 66
            || snapshot.value(QStringLiteral("signingHashHex")).toString()
               .compare(expectedHash, Qt::CaseInsensitive) != 0) {
        return fail(QStringLiteral("Wallet Core signing hash does not match the unsigned payload"));
    }

    const QString expectedConstruction = constructionFingerprintFor(
        intent.value(QStringLiteral("fingerprint")).toString(), expectedPayload);
    if (snapshot.value(QStringLiteral("constructionFingerprint")).toString()
            != expectedConstruction) {
        return fail(QStringLiteral("construction fingerprint does not match reviewed intent/payload"));
    }

    if (snapshot.value(QStringLiteral("constructedEpochMs")).toLongLong() <= 0)
        return fail(QStringLiteral("construction timestamp is missing"));

    if (error)
        error->clear();
    return true;
}

QVariantMap EthereumUnsignedTransactionService::runSelfTest() const
{
    QVariantMap result;

    quint64 parsed = 0;
    const bool quantityValidation =
        parseRpcQuantity(QJsonValue(QStringLiteral("0x0")), &parsed)
        && parsed == 0
        && parseRpcQuantity(QJsonValue(QStringLiteral("0x5208")), &parsed)
        && parsed == 21000
        && !parseRpcQuantity(QJsonValue(QStringLiteral("0x00")), &parsed)
        && !parseRpcQuantity(QJsonValue(QStringLiteral("5208")), &parsed)
        && !parseRpcQuantity(QJsonValue(QStringLiteral("0xgg")), &parsed);

    QByteArray value;
    QString atomic;
    const bool amountValidation = amountToAtomicBytes(
            QStringLiteral("0.01"), 18, &value, &atomic)
        && atomic == QStringLiteral("10000000000000000")
        && QString::fromLatin1(value.toHex()) == QStringLiteral("2386f26fc10000")
        && !amountToAtomicBytes(QStringLiteral("0"), 18, &value)
        && !amountToAtomicBytes(QStringLiteral("0.0000000000000000001"), 18, &value);

    QByteArray vectorValue;
    const bool vectorAmount = amountToAtomicBytes(
        QStringLiteral("0.01"), 18, &vectorValue);
    QString encodeError;
    const QByteArray payload = vectorAmount
        ? unsignedEip1559Payload(
              1,
              7,
              1500000000ULL,
              30000000000ULL,
              21000,
              QStringLiteral("0x1111111111111111111111111111111111111111"),
              vectorValue,
              &encodeError)
        : QByteArray();

    const QString expectedPayload = QStringLiteral(
        "02ef01078459682f008506fc23ac00825208941111111111111111111111111111111111111111872386f26fc1000080c0");
    const QString expectedHash = QStringLiteral(
        "ed5ffeca0540f40606b47a01aaf578365c2149d60220703eae796bc2a01da6d0");
    const bool rlpVector = !payload.isEmpty()
        && QString::fromLatin1(payload.toHex()) == expectedPayload;
    const bool keccakVector = rlpVector && keccak256Hex(payload) == expectedHash;

    UnsignedTransactionService intentService;
    intentService.configure(
        QStringLiteral("Ethereum"),
        QStringLiteral("0x9858EfFD232B4033E47d90003D41EC34EcaEda94"),
        QStringLiteral("2 ETH"));
    intentService.setDestinationAddress(
        QStringLiteral("0x1111111111111111111111111111111111111111"));
    intentService.setAmountText(QStringLiteral("0.01"));
    QVariantMap reviewedIntent = intentService.reviewModel();
    const QVariantMap validReviewedIntentModel = reviewedIntent;
    QString intentError;
    const bool validReviewedIntent = UnsignedTransactionService::validateReviewModel(
        reviewedIntent, &intentError);
    reviewedIntent.insert(QStringLiteral("normalizedAmount"), QStringLiteral("0.02"));
    const bool mutatedIntentRejected = !UnsignedTransactionService::validateReviewModel(
        reviewedIntent, &intentError);
    const bool intentBinding = validReviewedIntent && mutatedIntentRejected;

    QVariantMap signingSnapshotVector;
    signingSnapshotVector.insert(QStringLiteral("snapshotVersion"), 1);
    signingSnapshotVector.insert(QStringLiteral("chain"), QStringLiteral("Ethereum"));
    signingSnapshotVector.insert(QStringLiteral("network"),
                                 QStringLiteral("Ethereum mainnet · chain ID 1"));
    signingSnapshotVector.insert(QStringLiteral("intentFingerprint"),
                                 validReviewedIntentModel.value(QStringLiteral("fingerprint")));
    signingSnapshotVector.insert(QStringLiteral("chainId"), QStringLiteral("1"));
    signingSnapshotVector.insert(QStringLiteral("nonce"), QStringLiteral("7"));
    signingSnapshotVector.insert(QStringLiteral("maxPriorityFeePerGas"),
                                 QStringLiteral("1500000000"));
    signingSnapshotVector.insert(QStringLiteral("maxFeePerGas"),
                                 QStringLiteral("30000000000"));
    signingSnapshotVector.insert(QStringLiteral("gasLimit"), QStringLiteral("21000"));
    signingSnapshotVector.insert(QStringLiteral("unsignedPayloadHex"),
                                 QStringLiteral("0x") + expectedPayload);
    signingSnapshotVector.insert(QStringLiteral("signingHashHex"),
                                 QStringLiteral("0x") + expectedHash);
    signingSnapshotVector.insert(QStringLiteral("constructionFingerprint"),
                                 constructionFingerprintFor(
                                     validReviewedIntentModel.value(QStringLiteral("fingerprint")).toString(),
                                     payload));
    signingSnapshotVector.insert(QStringLiteral("constructedEpochMs"), 1LL);
    QString signingSnapshotError;
    const bool signingSnapshotAccepted = validateSigningSnapshot(
        validReviewedIntentModel, signingSnapshotVector, &signingSnapshotError);
    QVariantMap mutatedSigningSnapshot = signingSnapshotVector;
    mutatedSigningSnapshot.insert(QStringLiteral("nonce"), QStringLiteral("8"));
    const bool mutatedSigningSnapshotRejected = !validateSigningSnapshot(
        validReviewedIntentModel, mutatedSigningSnapshot, &signingSnapshotError);
    const bool signingSnapshotBinding = signingSnapshotAccepted
        && mutatedSigningSnapshotRejected;

    const QString constructionA = constructionFingerprintFor(
        QStringLiteral("aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"),
        payload);
    QByteArray mutated = payload;
    if (!mutated.isEmpty())
        mutated[mutated.size() - 1] = static_cast<char>(
            static_cast<unsigned char>(mutated.at(mutated.size() - 1)) ^ 0x01U);
    const QString constructionB = constructionFingerprintFor(
        QStringLiteral("aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"),
        mutated);
    const bool fingerprintBinding = constructionA.size() == 64
        && constructionB.size() == 64
        && constructionA != constructionB;

    const bool passed = quantityValidation && amountValidation
        && rlpVector && keccakVector && intentBinding && fingerprintBinding
        && signingSnapshotBinding;

    result.insert(QStringLiteral("passed"), passed);
    result.insert(QStringLiteral("quantityValidationPassed"), quantityValidation);
    result.insert(QStringLiteral("amountToWeiPassed"), amountValidation);
    result.insert(QStringLiteral("eip1559RlpVectorPassed"), rlpVector);
    result.insert(QStringLiteral("keccakVectorPassed"), keccakVector);
    result.insert(QStringLiteral("intentBindingPassed"), intentBinding);
    result.insert(QStringLiteral("constructionFingerprintPassed"), fingerprintBinding);
    result.insert(QStringLiteral("signingSnapshotBindingPassed"), signingSnapshotBinding);
    result.insert(QStringLiteral("detail"), passed
        ? QStringLiteral("Ethereum quantity parsing, exact wei conversion, reviewed-intent validation, EIP-1559 RLP, Wallet Core Keccak-256, construction binding and C++ signing-snapshot validation passed")
        : QStringLiteral("Ethereum unsigned-construction self-test failed"));
    return result;
}
