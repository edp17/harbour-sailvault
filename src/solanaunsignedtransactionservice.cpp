#include "solanaunsignedtransactionservice.h"

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

#include <TrustWalletCore/TWBase58.h>
#include <TrustWalletCore/TWCoinType.h>
#include <TrustWalletCore/TWData.h>
#include <TrustWalletCore/TWString.h>
#include <TrustWalletCore/TWTransactionCompiler.h>

#include <cmath>
#include <limits>

namespace {
const int kMaximumDnsRetries = 1;
const int kDnsRetryDelayMs = 900;
const char kSolanaMainnetGenesis[] = "5eykt4UsFv8P8NJdTREpY1vzqKqZKvdpKuc147dw2N9d";
const quint64 kMaximumQuotedFeeLamports = 100000000ULL; // 0.1 SOL defensive ceiling.

class TWStringGuard
{
public:
    explicit TWStringGuard(TWString *value = nullptr) : m_value(value) {}
    ~TWStringGuard() { if (m_value) TWStringDelete(m_value); }
    TWString *get() const { return m_value; }
private:
    TWString *m_value;
};

class TWDataGuard
{
public:
    explicit TWDataGuard(TWData *value = nullptr) : m_value(value) {}
    ~TWDataGuard() { if (m_value) TWDataDelete(m_value); }
    TWData *get() const { return m_value; }
private:
    TWData *m_value;
};

void appendVarint(QByteArray &out, quint64 value)
{
    do {
        unsigned char byte = static_cast<unsigned char>(value & 0x7fU);
        value >>= 7U;
        if (value != 0)
            byte |= 0x80U;
        out.append(static_cast<char>(byte));
    } while (value != 0);
}

void appendKey(QByteArray &out, int fieldNumber, int wireType)
{
    appendVarint(out, (static_cast<quint64>(fieldNumber) << 3U)
                       | static_cast<quint64>(wireType));
}

void appendLengthDelimited(QByteArray &out, int fieldNumber,
                           const QByteArray &value)
{
    appendKey(out, fieldNumber, 2);
    appendVarint(out, static_cast<quint64>(value.size()));
    out += value;
}

bool readVarint(const QByteArray &data, int *offset, quint64 *value)
{
    if (!offset || !value || *offset < 0 || *offset > data.size())
        return false;

    quint64 result = 0;
    int shift = 0;
    while (*offset < data.size() && shift <= 63) {
        const unsigned char byte = static_cast<unsigned char>(data.at((*offset)++));
        if (shift == 63 && (byte & 0xfeU) != 0)
            return false;
        result |= static_cast<quint64>(byte & 0x7fU) << shift;
        if ((byte & 0x80U) == 0) {
            *value = result;
            return true;
        }
        shift += 7;
    }
    return false;
}

bool parsePreSigningOutput(const QByteArray &data,
                           QByteArray *signer,
                           QByteArray *message,
                           QString *error)
{
    QByteArray foundSigner;
    QByteArray foundMessage;
    quint64 errorCode = 0;
    QString errorMessage;
    int offset = 0;

    while (offset < data.size()) {
        quint64 key = 0;
        if (!readVarint(data, &offset, &key) || key == 0) {
            if (error) *error = QStringLiteral("Wallet Core returned malformed Solana pre-signing protobuf");
            return false;
        }
        const int field = static_cast<int>(key >> 3U);
        const int wire = static_cast<int>(key & 0x07U);

        if (wire == 0) {
            quint64 value = 0;
            if (!readVarint(data, &offset, &value)) {
                if (error) *error = QStringLiteral("Wallet Core returned malformed Solana varint");
                return false;
            }
            if (field == 3)
                errorCode = value;
            continue;
        }

        if (wire == 2) {
            quint64 length = 0;
            if (!readVarint(data, &offset, &length)
                    || length > static_cast<quint64>(data.size() - offset)) {
                if (error) *error = QStringLiteral("Wallet Core returned malformed Solana length field");
                return false;
            }
            const QByteArray value = data.mid(offset, static_cast<int>(length));
            offset += static_cast<int>(length);
            if (field == 1) {
                if (!foundSigner.isEmpty()) {
                    if (error) *error = QStringLiteral("Solana construction expected exactly one signer");
                    return false;
                }
                foundSigner = value;
            } else if (field == 2) {
                foundMessage = value;
            } else if (field == 4) {
                errorMessage = QString::fromUtf8(value);
            }
            continue;
        }

        if (error) *error = QStringLiteral("Wallet Core returned unsupported Solana protobuf wire type");
        return false;
    }

    if (errorCode != 0) {
        if (error) {
            *error = errorMessage.isEmpty()
                ? QStringLiteral("Wallet Core Solana pre-signing error %1").arg(errorCode)
                : QStringLiteral("Wallet Core Solana error %1 · %2").arg(errorCode).arg(errorMessage);
        }
        return false;
    }
    if (foundSigner.isEmpty() || foundMessage.isEmpty()) {
        if (error) *error = QStringLiteral("Wallet Core Solana pre-signing output is incomplete");
        return false;
    }

    if (signer) *signer = foundSigner;
    if (message) *message = foundMessage;
    if (error) error->clear();
    return true;
}

QString rpcErrorMessage(const QJsonObject &object, const QString &fallback)
{
    const QJsonObject error = object.value(QStringLiteral("error")).toObject();
    return error.value(QStringLiteral("message")).toString(fallback);
}

QString stripLeadingZeros(const QString &value)
{
    int first = 0;
    while (first + 1 < value.size() && value.at(first) == QLatin1Char('0'))
        ++first;
    return value.mid(first);
}

} // namespace

SolanaUnsignedTransactionService::SolanaUnsignedTransactionService(QObject *parent)
    : QObject(parent)
{
    m_status = QStringLiteral("Solana transaction not constructed");
}

void SolanaUnsignedTransactionService::clearState(bool preserveStatus)
{
    m_loading = false;
    m_constructed = false;
    m_haveGenesis = false;
    m_haveBlockhash = false;
    m_feeRequested = false;
    m_intent.clear();
    m_endpoint.clear();
    m_providerHost.clear();
    m_intentFingerprint.clear();
    if (!preserveStatus)
        m_status = QStringLiteral("Solana transaction not constructed");
    m_genesisHash.clear();
    m_recentBlockhash.clear();
    m_lastValidBlockHeightText.clear();
    m_lamportsText.clear();
    m_networkFeeLamportsText.clear();
    m_networkFeeSol.clear();
    m_signerAddress.clear();
    m_instructionSummary.clear();
    m_unsignedMessageHex.clear();
    m_unsignedMessageBase64.clear();
    m_unsignedTransactionTemplateBase64.clear();
    m_constructionFingerprint.clear();
    m_constructedAt.clear();
    m_lamports = 0;
    m_lastValidBlockHeight = 0;
    m_unsignedMessage.clear();
}

void SolanaUnsignedTransactionService::reset()
{
    ++m_generation;
    SailVaultNetwork::cancelOutstanding(&m_network);
    clearState();
    emit stateChanged();
}

bool SolanaUnsignedTransactionService::amountToLamports(const QString &amount,
                                                        quint64 *lamports)
{
    if (!lamports)
        return false;
    const QString value = amount.trimmed();
    static const QRegularExpression pattern(QStringLiteral("^[0-9]+(?:\\.[0-9]+)?$"));
    if (!pattern.match(value).hasMatch())
        return false;

    const int point = value.indexOf(QLatin1Char('.'));
    QString whole = point >= 0 ? value.left(point) : value;
    QString fraction = point >= 0 ? value.mid(point + 1) : QString();
    if (fraction.size() > 9)
        return false;
    whole = stripLeadingZeros(whole);
    while (fraction.size() < 9)
        fraction.append(QLatin1Char('0'));

    QString digits = stripLeadingZeros(whole + fraction);
    if (digits.isEmpty())
        digits = QStringLiteral("0");

    quint64 result = 0;
    for (int i = 0; i < digits.size(); ++i) {
        const QChar ch = digits.at(i);
        if (ch < QLatin1Char('0') || ch > QLatin1Char('9'))
            return false;
        const quint64 digit = static_cast<quint64>(ch.unicode() - QLatin1Char('0').unicode());
        if (result > (std::numeric_limits<quint64>::max() - digit) / 10ULL)
            return false;
        result = result * 10ULL + digit;
    }
    if (result == 0)
        return false;
    *lamports = result;
    return true;
}

bool SolanaUnsignedTransactionService::decodeBase58_32(const QString &value,
                                                        QByteArray *decoded)
{
    const QByteArray utf8 = value.trimmed().toUtf8();
    if (utf8.isEmpty())
        return false;
    TWStringGuard string(TWStringCreateWithUTF8Bytes(utf8.constData()));
    if (!string.get())
        return false;
    TWDataGuard data(TWBase58DecodeNoCheck(string.get()));
    if (!data.get() || TWDataSize(data.get()) != 32)
        return false;
    if (decoded) {
        *decoded = QByteArray(
            reinterpret_cast<const char *>(TWDataBytes(data.get())),
            static_cast<int>(TWDataSize(data.get())));
    }
    return true;
}

QByteArray SolanaUnsignedTransactionService::encodeSolanaSigningInput(
        const QString &sender,
        const QString &recipient,
        quint64 lamports,
        const QString &recentBlockhash)
{
    QByteArray transfer;
    appendLengthDelimited(transfer, 1, recipient.toUtf8());
    appendKey(transfer, 2, 0);
    appendVarint(transfer, lamports);

    QByteArray input;
    appendLengthDelimited(input, 2, recentBlockhash.toUtf8());
    appendLengthDelimited(input, 4, transfer);
    appendLengthDelimited(input, 14, sender.toUtf8());
    return input;
}

SolanaUnsignedTransactionService::PreSigningResult
SolanaUnsignedTransactionService::walletCorePreSigningMessage(
        const QString &sender,
        const QString &recipient,
        quint64 lamports,
        const QString &recentBlockhash)
{
    PreSigningResult result;
    if (!decodeBase58_32(sender) || !decodeBase58_32(recipient)
            || !decodeBase58_32(recentBlockhash)) {
        result.error = QStringLiteral("Solana address or recent blockhash is not a 32-byte base58 value");
        return result;
    }

    const QByteArray inputBytes = encodeSolanaSigningInput(
        sender, recipient, lamports, recentBlockhash);
    TWDataGuard input(TWDataCreateWithBytes(
        reinterpret_cast<const uint8_t *>(inputBytes.constData()),
        static_cast<size_t>(inputBytes.size())));
    if (!input.get()) {
        result.error = QStringLiteral("Unable to create Wallet Core Solana input");
        return result;
    }

    TWDataGuard output(TWTransactionCompilerPreImageHashes(
        TWCoinTypeSolana, input.get()));
    if (!output.get()) {
        result.error = QStringLiteral("Wallet Core Solana transaction compiler returned no output");
        return result;
    }

    const QByteArray outputBytes(
        reinterpret_cast<const char *>(TWDataBytes(output.get())),
        static_cast<int>(TWDataSize(output.get())));
    if (!parsePreSigningOutput(outputBytes, &result.signer,
                               &result.message, &result.error)) {
        return result;
    }
    if (result.signer != sender.toUtf8()) {
        result.error = QStringLiteral("Wallet Core Solana signer does not match reviewed source address");
        result.signer.clear();
        result.message.clear();
        return result;
    }

    result.ok = true;
    return result;
}

QByteArray SolanaUnsignedTransactionService::unsignedTransactionTemplate(
        const QByteArray &message)
{
    if (message.isEmpty())
        return QByteArray();
    QByteArray tx;
    tx.append(static_cast<char>(0x01)); // one required signature
    tx += QByteArray(64, static_cast<char>(0x00));
    tx += message;
    return tx;
}

QString SolanaUnsignedTransactionService::formatLamportsAsSol(quint64 lamports)
{
    QString digits = QString::number(lamports);
    if (digits.size() <= 9)
        digits.prepend(QString(10 - digits.size(), QLatin1Char('0')));
    QString whole = digits.left(digits.size() - 9);
    QString fraction = digits.right(9);
    while (!fraction.isEmpty() && fraction.endsWith(QLatin1Char('0')))
        fraction.chop(1);
    return (fraction.isEmpty() ? whole : whole + QLatin1Char('.') + fraction)
        + QStringLiteral(" SOL");
}

QString SolanaUnsignedTransactionService::constructionFingerprintFor(
        const QString &intentFingerprint,
        const QString &genesisHash,
        const QString &recentBlockhash,
        quint64 lastValidBlockHeight,
        quint64 feeLamports,
        const QByteArray &message)
{
    const QByteArray canonical = QByteArray("sailvault-solana-unsigned-v1\n")
        + intentFingerprint.toLatin1() + '\n'
        + genesisHash.toLatin1() + '\n'
        + recentBlockhash.toLatin1() + '\n'
        + QByteArray::number(lastValidBlockHeight) + '\n'
        + QByteArray::number(feeLamports) + '\n'
        + message.toHex();
    return QString::fromLatin1(QCryptographicHash::hash(
        canonical, QCryptographicHash::Sha256).toHex());
}

void SolanaUnsignedTransactionService::construct(const QVariantMap &intent,
                                                  const QString &solanaRpcUrl)
{
    ++m_generation;
    const quint64 generation = m_generation;
    SailVaultNetwork::cancelOutstanding(&m_network);
    clearState();

    QString intentError;
    if (!UnsignedTransactionService::validateReviewModel(intent, &intentError)) {
        m_status = QStringLiteral("Solana construction blocked · %1").arg(intentError);
        emit stateChanged();
        return;
    }
    if (intent.value(QStringLiteral("chain")).toString() != QStringLiteral("Solana")) {
        m_status = QStringLiteral("Solana construction blocked · reviewed intent is not Solana");
        emit stateChanged();
        return;
    }
    if (!amountToLamports(intent.value(QStringLiteral("normalizedAmount")).toString(),
                          &m_lamports)) {
        m_status = QStringLiteral("Solana construction blocked · invalid exact SOL amount");
        emit stateChanged();
        return;
    }
    if (!decodeBase58_32(intent.value(QStringLiteral("sourceAddress")).toString())
            || !decodeBase58_32(intent.value(QStringLiteral("destinationAddress")).toString())) {
        m_status = QStringLiteral("Solana construction blocked · source/destination must decode to 32-byte public keys");
        emit stateChanged();
        return;
    }
    if (SailVaultNetwork::offlineModeEnabled()) {
        m_status = QStringLiteral("Solana construction blocked · Offline mode is enabled");
        emit stateChanged();
        return;
    }

    const QUrl endpoint(solanaRpcUrl.trimmed());
    if (!SailVaultNetwork::isValidHttpsEndpoint(endpoint)) {
        m_status = QStringLiteral("Solana construction blocked · configured RPC endpoint is not safe HTTPS");
        emit stateChanged();
        return;
    }

    m_intent = intent;
    m_endpoint = endpoint.toString();
    m_providerHost = endpoint.host();
    m_intentFingerprint = intent.value(QStringLiteral("fingerprint")).toString();
    m_lamportsText = QString::number(m_lamports) + QStringLiteral(" lamports");
    m_loading = true;
    m_status = QStringLiteral("Verifying Solana mainnet and fetching a finalized recent blockhash…");
    emit stateChanged();

    sendRpc(GenesisHash, QStringLiteral("getGenesisHash"), QJsonArray(),
            5101, generation);

    QJsonObject config;
    config.insert(QStringLiteral("commitment"), QStringLiteral("finalized"));
    QJsonArray blockhashParams;
    blockhashParams.append(config);
    sendRpc(LatestBlockhash, QStringLiteral("getLatestBlockhash"),
            blockhashParams, 5102, generation);
}

void SolanaUnsignedTransactionService::fail(const QString &message,
                                             quint64 generation)
{
    if (generation != m_generation)
        return;
    ++m_generation;
    SailVaultNetwork::cancelOutstanding(&m_network);
    m_loading = false;
    m_constructed = false;
    m_status = message;
    emit stateChanged();
}

void SolanaUnsignedTransactionService::sendRpc(RpcKind kind,
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
        const QNetworkReply::NetworkError code = reply->error();
        const QString networkError = SailVaultNetwork::jsonErrorText(
            reply, kRequestTimeoutMs, kMaxResponseBytes);
        const QByteArray data = reply->readAll();
        reply->deleteLater();

        if (generation != m_generation)
            return;
        if (code == QNetworkReply::HostNotFoundError && attempt < kMaximumDnsRetries) {
            QTimer::singleShot(kDnsRetryDelayMs, this,
                               [this, kind, method, params, requestId, generation, attempt]() {
                sendRpc(kind, method, params, requestId, generation, attempt + 1);
            });
            return;
        }
        if (!networkError.isEmpty()) {
            fail(QStringLiteral("Solana construction failed · %1").arg(networkError), generation);
            return;
        }

        QJsonParseError parseError;
        const QJsonDocument document = QJsonDocument::fromJson(data, &parseError);
        if (parseError.error != QJsonParseError::NoError || !document.isObject()) {
            fail(QStringLiteral("Solana construction failed · invalid JSON-RPC response"), generation);
            return;
        }
        const QJsonObject object = document.object();
        QString envelopeError;
        if (!SailVaultNetwork::validateJsonRpcEnvelope(
                object, QJsonValue(requestId), &envelopeError)) {
            fail(QStringLiteral("Solana construction failed · %1").arg(envelopeError), generation);
            return;
        }
        if (object.contains(QStringLiteral("error"))) {
            fail(QStringLiteral("Solana construction failed · %1")
                     .arg(rpcErrorMessage(object, QStringLiteral("JSON-RPC error"))), generation);
            return;
        }
        handleRpcResult(kind, object, generation);
    });
}

void SolanaUnsignedTransactionService::handleRpcResult(
        RpcKind kind, const QJsonObject &object, quint64 generation)
{
    if (generation != m_generation)
        return;

    if (kind == GenesisHash) {
        const QString genesis = object.value(QStringLiteral("result")).toString();
        if (genesis != QString::fromLatin1(kSolanaMainnetGenesis)
                || !decodeBase58_32(genesis)) {
            fail(QStringLiteral("Solana construction blocked · RPC is not Solana mainnet genesis"), generation);
            return;
        }
        m_genesisHash = genesis;
        m_haveGenesis = true;
        maybeBuildMessage(generation);
        return;
    }

    if (kind == LatestBlockhash) {
        const QJsonObject result = object.value(QStringLiteral("result")).toObject();
        const QJsonObject value = result.value(QStringLiteral("value")).toObject();
        const QString blockhash = value.value(QStringLiteral("blockhash")).toString();
        const double heightDouble = value.value(QStringLiteral("lastValidBlockHeight")).toDouble(-1);
        if (!decodeBase58_32(blockhash)
                || !std::isfinite(heightDouble)
                || heightDouble < 0
                || std::floor(heightDouble) != heightDouble
                || heightDouble > 9007199254740991.0) {
            fail(QStringLiteral("Solana construction failed · invalid latest-blockhash response"), generation);
            return;
        }
        m_recentBlockhash = blockhash;
        m_lastValidBlockHeight = static_cast<quint64>(heightDouble);
        m_lastValidBlockHeightText = QString::number(m_lastValidBlockHeight);
        m_haveBlockhash = true;
        maybeBuildMessage(generation);
        return;
    }

    if (kind == FeeForMessage) {
        const QJsonObject result = object.value(QStringLiteral("result")).toObject();
        const QJsonValue feeValue = result.value(QStringLiteral("value"));
        if (!feeValue.isDouble()) {
            fail(QStringLiteral("Solana construction failed · fee quote is unavailable for the fresh message"), generation);
            return;
        }
        const double feeDouble = feeValue.toDouble(-1);
        if (!std::isfinite(feeDouble)
                || feeDouble < 0
                || std::floor(feeDouble) != feeDouble
                || feeDouble > static_cast<double>(kMaximumQuotedFeeLamports)) {
            fail(QStringLiteral("Solana construction blocked · provider fee quote is invalid or above 0.1 SOL ceiling"), generation);
            return;
        }
        finalizeConstruction(generation, static_cast<quint64>(feeDouble));
    }
}

void SolanaUnsignedTransactionService::maybeBuildMessage(quint64 generation)
{
    if (generation != m_generation || !m_haveGenesis || !m_haveBlockhash
            || m_feeRequested)
        return;

    const QString source = m_intent.value(QStringLiteral("sourceAddress")).toString();
    const QString destination = m_intent.value(QStringLiteral("destinationAddress")).toString();
    const PreSigningResult prepared = walletCorePreSigningMessage(
        source, destination, m_lamports, m_recentBlockhash);
    if (!prepared.ok) {
        fail(QStringLiteral("Solana construction blocked · %1").arg(prepared.error), generation);
        return;
    }

    m_unsignedMessage = prepared.message;
    const QByteArray templateTx = unsignedTransactionTemplate(m_unsignedMessage);
    if (templateTx.isEmpty() || templateTx.size() > 1232) {
        fail(QStringLiteral("Solana construction blocked · unsigned template exceeds legacy transaction size policy"), generation);
        return;
    }

    m_signerAddress = QString::fromUtf8(prepared.signer);
    m_instructionSummary = QStringLiteral("System Program transfer · 1 signer · 1 instruction");
    m_unsignedMessageHex = QString::fromLatin1(m_unsignedMessage.toHex());
    m_unsignedMessageBase64 = QString::fromLatin1(m_unsignedMessage.toBase64());
    m_unsignedTransactionTemplateBase64 = QString::fromLatin1(templateTx.toBase64());
    m_feeRequested = true;
    m_status = QStringLiteral("Wallet Core message constructed · requesting a public fee quote for that exact message…");
    emit stateChanged();

    QJsonObject config;
    config.insert(QStringLiteral("commitment"), QStringLiteral("finalized"));
    QJsonArray feeParams;
    feeParams.append(m_unsignedMessageBase64);
    feeParams.append(config);
    sendRpc(FeeForMessage, QStringLiteral("getFeeForMessage"),
            feeParams, 5103, generation);
}

void SolanaUnsignedTransactionService::finalizeConstruction(
        quint64 generation, quint64 feeLamports)
{
    if (generation != m_generation)
        return;

    m_networkFeeLamportsText = QString::number(feeLamports) + QStringLiteral(" lamports");
    m_networkFeeSol = formatLamportsAsSol(feeLamports);
    m_constructionFingerprint = constructionFingerprintFor(
        m_intentFingerprint, m_genesisHash, m_recentBlockhash,
        m_lastValidBlockHeight, feeLamports, m_unsignedMessage);
    m_constructedAt = QDateTime::currentDateTimeUtc().toString(Qt::ISODate);
    m_loading = false;
    m_constructed = true;
    m_status = QStringLiteral("Unsigned Solana signing message constructed · signing remains disabled");
    emit stateChanged();
}

QVariantMap SolanaUnsignedTransactionService::runSelfTest() const
{
    QVariantMap out;

    quint64 lamports = 0;
    const bool amountPassed = amountToLamports(QStringLiteral("0.000001000"), &lamports)
        && lamports == 1000ULL
        && amountToLamports(QStringLiteral("1"), &lamports)
        && lamports == 1000000000ULL
        && !amountToLamports(QStringLiteral("1e-6"), &lamports)
        && !amountToLamports(QStringLiteral("0.0000000001"), &lamports)
        && !amountToLamports(QStringLiteral("0"), &lamports);

    const QString sender = QStringLiteral("sp6VUqq1nDEuU83bU2hstmEYrJNipJYpwS7gZ7Jv7ZH");
    const QString recipient = QStringLiteral("3UVYmECPPMZSCqWKfENfuoTv51fTDTWicX9xmBD2euKe");
    const QString blockhash = QStringLiteral("TPJFTN4CjBn12HiBfAbGUhpD9zGvRSm2RcheFRA4Fyv");
    const bool base58Passed = decodeBase58_32(sender)
        && decodeBase58_32(recipient)
        && decodeBase58_32(blockhash)
        && decodeBase58_32(QString::fromLatin1(kSolanaMainnetGenesis))
        && !decodeBase58_32(QStringLiteral("not-a-solana-key"));

    const PreSigningResult prepared = walletCorePreSigningMessage(
        sender, recipient, 1000ULL, blockhash);
    const QByteArray expectedMessage = QByteArray::fromHex(
        "010001030d044a62d0a4dfe5a037a15b59fa4d4d0d3ab81103a2c10a6da08a4d058611c0"
        "24c255a8bc3e8496217a2cd2a1894b9b9dcace04fcd9c0d599acdaaea40a1b61"
        "0000000000000000000000000000000000000000000000000000000000000000"
        "06c25012cc11a599a45b3b2f7f8a7c65b0547fa0bb67170d7a0cd1eda4e2c9e5"
        "01020200010c02000000e803000000000000");
    const bool preimagePassed = prepared.ok
        && prepared.signer == sender.toUtf8()
        && prepared.message == expectedMessage;

    const QByteArray templateTx = preimagePassed
        ? unsignedTransactionTemplate(prepared.message) : QByteArray();
    const QByteArray expectedTemplateBase64(
        "AQAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAABAAEDDQRKYtCk3+WgN6FbWfpNTQ06uBEDosEKbaCKTQWGEcAkwlWovD6EliF6LNKhiUubncrOBPzZwNWZrNqupAobYQAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAABsJQEswRpZmkWzsvf4p8ZbBUf6C7ZxcNegzR7aTiyeUBAgIAAQwCAAAA6AMAAAAAAAA=");
    const bool templatePassed = !templateTx.isEmpty()
        && templateTx.size() == prepared.message.size() + 65
        && static_cast<unsigned char>(templateTx.at(0)) == 1U
        && templateTx.mid(1, 64) == QByteArray(64, static_cast<char>(0x00))
        && templateTx.mid(65) == prepared.message
        && templateTx.toBase64() == expectedTemplateBase64;

    UnsignedTransactionService intentService;
    intentService.configure(
        QStringLiteral("Solana"),
        QStringLiteral("GjJyeC1r2RgkuoCWMyPYkCWSGSGLcz266EaAkLA27AhL"),
        QStringLiteral("2 SOL"));
    intentService.setDestinationAddress(
        QStringLiteral("GjJyeC1r2RgkuoCWMyPYkCWSGSGLcz266EaAkLA27AhL"));
    intentService.setAmountText(QStringLiteral("0.000001"));
    QVariantMap reviewedIntent = intentService.reviewModel();
    QString intentError;
    const bool validReviewedIntent = UnsignedTransactionService::validateReviewModel(
        reviewedIntent, &intentError);
    reviewedIntent.insert(QStringLiteral("normalizedAmount"), QStringLiteral("0.000002"));
    const bool mutatedIntentRejected = !UnsignedTransactionService::validateReviewModel(
        reviewedIntent, &intentError);
    const bool intentBinding = validReviewedIntent && mutatedIntentRejected;

    const QString fp = preimagePassed ? constructionFingerprintFor(
        QString(64, QLatin1Char('a')),
        QString::fromLatin1(kSolanaMainnetGenesis),
        blockhash,
        123456789ULL,
        5000ULL,
        prepared.message) : QString();
    QByteArray mutatedMessage = prepared.message;
    if (!mutatedMessage.isEmpty()) {
        const int i = mutatedMessage.size() - 1;
        mutatedMessage[i] = static_cast<char>(mutatedMessage.at(i) ^ 0x01);
    }
    const QString mutatedFp = preimagePassed ? constructionFingerprintFor(
        QString(64, QLatin1Char('a')),
        QString::fromLatin1(kSolanaMainnetGenesis),
        blockhash,
        123456789ULL,
        5000ULL,
        mutatedMessage) : QString();
    const bool fingerprintPassed = fp
        == QStringLiteral("3b97097d9c22adc9d86c1a3db8822b4b009939f58c384d059aacf1af11393136")
        && mutatedFp.size() == 64 && fp != mutatedFp;

    const bool feeFormatPassed = formatLamportsAsSol(5000ULL)
        == QStringLiteral("0.000005 SOL");

    const bool passed = amountPassed && base58Passed && preimagePassed
        && templatePassed && intentBinding && fingerprintPassed && feeFormatPassed;
    out.insert(QStringLiteral("passed"), passed);
    out.insert(QStringLiteral("exactAmountPassed"), amountPassed);
    out.insert(QStringLiteral("walletCoreBase58Passed"), base58Passed);
    out.insert(QStringLiteral("walletCorePreimageVectorPassed"), preimagePassed);
    out.insert(QStringLiteral("transactionTemplatePassed"), templatePassed);
    out.insert(QStringLiteral("intentBindingPassed"), intentBinding);
    out.insert(QStringLiteral("constructionFingerprintPassed"), fingerprintPassed);
    out.insert(QStringLiteral("feeFormattingPassed"), feeFormatPassed);
    out.insert(QStringLiteral("detail"), passed
        ? QStringLiteral("Exact SOL→lamports, Wallet Core base58 validation, upstream Solana external-signing message vector, zero-signature template, reviewed-intent mutation rejection and SHA-256 construction binding passed")
        : QStringLiteral("One or more Solana unsigned-construction checks failed"));
    return out;
}
