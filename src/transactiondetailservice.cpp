#include "transactiondetailservice.h"
#include "settingsstore.h"
#include "networkrequestutils.h"

#include <QDateTime>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QJsonValue>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QUrl>

#include <boost/multiprecision/cpp_int.hpp>

namespace {

const char kEthereumExplorerKey[] = "network/ethereumExplorerUrl";
const char kBitcoinApiKey[] = "network/bitcoinApiUrl";
const char kSolanaRpcKey[] = "network/solanaRpcUrl";

const char kDefaultEthereumExplorer[] = "https://eth.blockscout.com/api/v2";
const char kDefaultBitcoinApi[] = "https://blockstream.info/api";
const char kDefaultSolanaRpc[] = "https://solana-rpc.publicnode.com";

QString trimDecimalFraction(QString value)
{
    while (value.contains(QLatin1Char('.'))
            && value.endsWith(QLatin1Char('0'))) {
        value.chop(1);
    }

    if (value.endsWith(QLatin1Char('.')))
        value.chop(1);

    return value;
}

} // namespace

TransactionDetailService::TransactionDetailService(QObject *parent)
    : QObject(parent)
    , m_status(QStringLiteral("Transaction details not loaded"))
{
}

bool TransactionDetailService::loading() const
{
    return m_loading;
}

bool TransactionDetailService::loaded() const
{
    return m_loaded;
}

QString TransactionDetailService::status() const
{
    return m_status;
}

QString TransactionDetailService::chain() const
{
    return m_chain;
}

QString TransactionDetailService::identifier() const
{
    return m_identifier;
}

QString TransactionDetailService::direction() const
{
    return m_direction;
}

QString TransactionDetailService::amount() const
{
    return m_amount;
}

QString TransactionDetailService::fee() const
{
    return m_fee;
}

QString TransactionDetailService::transactionState() const
{
    return m_transactionState;
}

QString TransactionDetailService::blockText() const
{
    return m_blockText;
}

QString TransactionDetailService::timeText() const
{
    return m_timeText;
}

QString TransactionDetailService::fromAddress() const
{
    return m_fromAddress;
}

QString TransactionDetailService::toAddress() const
{
    return m_toAddress;
}

QString TransactionDetailService::method() const
{
    return m_method;
}

QString TransactionDetailService::extraText() const
{
    return m_extraText;
}

QString TransactionDetailService::setting(const char *key,
                                          const char *fallback)
{
    return SailVaultSettings::value(
        QString::fromLatin1(key),
        QString::fromLatin1(fallback)).toString();
}

QString TransactionDetailService::normalizeBaseUrl(const QString &url)
{
    QString value = url.trimmed();

    while (value.endsWith(QLatin1Char('/')))
        value.chop(1);

    return value;
}

QString TransactionDetailService::networkErrorText(QNetworkReply *reply)
{
    if (SailVaultNetwork::timedOut(reply))
        return QStringLiteral("Request timed out after 15 seconds");

    const int httpStatus =
        reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();

    if (reply->error() != QNetworkReply::NoError) {
        return httpStatus > 0
            ? QStringLiteral("HTTP %1 · %2")
                  .arg(httpStatus)
                  .arg(reply->errorString())
            : reply->errorString();
    }

    if (httpStatus < 200 || httpStatus >= 300)
        return QStringLiteral("HTTP %1").arg(httpStatus);

    return QString();
}

QString TransactionDetailService::addressHash(const QJsonValue &value)
{
    if (value.isString())
        return value.toString();

    if (value.isObject()) {
        const QJsonObject object = value.toObject();

        QString address =
            object.value(QStringLiteral("hash")).toString();

        if (address.isEmpty())
            address = object.value(QStringLiteral("address")).toString();

        return address;
    }

    return QString();
}

QString TransactionDetailService::jsonScalarString(const QJsonValue &value)
{
    if (value.isString())
        return value.toString();

    if (value.isDouble())
        return QString::number(value.toDouble(), 'f', 0);

    if (value.isBool())
        return value.toBool() ? QStringLiteral("true")
                              : QStringLiteral("false");

    return QString();
}

QString TransactionDetailService::formatBitcoin(qint64 satoshis)
{
    const bool negative = satoshis < 0;

    quint64 value = negative
        ? static_cast<quint64>(-(satoshis + 1)) + 1
        : static_cast<quint64>(satoshis);

    const quint64 whole = value / 100000000ULL;
    const quint64 fraction = value % 100000000ULL;

    QString result = QStringLiteral("%1.%2")
        .arg(whole)
        .arg(fraction, 8, 10, QLatin1Char('0'));

    result = trimDecimalFraction(result);

    if (!result.contains(QLatin1Char('.')))
        result += QStringLiteral(".0");

    if (negative)
        result.prepend(QLatin1Char('-'));

    return result + QStringLiteral(" BTC");
}

QString TransactionDetailService::formatSolana(qint64 lamports)
{
    const bool negative = lamports < 0;

    quint64 value = negative
        ? static_cast<quint64>(-(lamports + 1)) + 1
        : static_cast<quint64>(lamports);

    const quint64 whole = value / 1000000000ULL;
    const quint64 fraction = value % 1000000000ULL;

    QString result = QStringLiteral("%1.%2")
        .arg(whole)
        .arg(fraction, 9, 10, QLatin1Char('0'));

    result = trimDecimalFraction(result);

    if (!result.contains(QLatin1Char('.')))
        result += QStringLiteral(".0");

    if (negative)
        result.prepend(QLatin1Char('-'));

    return result + QStringLiteral(" SOL");
}

QString TransactionDetailService::formatEthereumWei(const QString &weiText)
{
    QString digits = weiText.trimmed();

    if (digits.isEmpty())
        return QStringLiteral("0.0 ETH");

    bool negative = false;

    if (digits.startsWith(QLatin1Char('-'))) {
        negative = true;
        digits.remove(0, 1);
    }

    if (digits.isEmpty())
        return QStringLiteral("0.0 ETH");

    boost::multiprecision::cpp_int wei = 0;

    for (const QChar ch : digits) {
        if (ch < QLatin1Char('0') || ch > QLatin1Char('9'))
            return QString();

        wei *= 10;
        wei += ch.unicode() - QLatin1Char('0').unicode();
    }

    const boost::multiprecision::cpp_int divisor(
        "1000000000000000000");

    const boost::multiprecision::cpp_int whole = wei / divisor;
    const boost::multiprecision::cpp_int fraction = wei % divisor;

    QString result =
        QString::fromStdString(whole.convert_to<std::string>())
        + QLatin1Char('.')
        + QString::fromStdString(fraction.convert_to<std::string>())
              .rightJustified(18, QLatin1Char('0'))
              .left(10);

    result = trimDecimalFraction(result);

    if (!result.contains(QLatin1Char('.')))
        result += QStringLiteral(".0");

    if (negative)
        result.prepend(QLatin1Char('-'));

    return result + QStringLiteral(" ETH");
}

QString TransactionDetailService::isoTimeToLocal(const QString &timestamp)
{
    if (timestamp.trimmed().isEmpty())
        return QString();

    const QDateTime dt =
        QDateTime::fromString(timestamp, Qt::ISODate);

    if (!dt.isValid())
        return timestamp;

    return dt.toLocalTime()
        .toString(QStringLiteral("yyyy-MM-dd hh:mm:ss"));
}

QString TransactionDetailService::unixTimeToLocal(qint64 timestamp)
{
    if (timestamp <= 0)
        return QString();

    return QDateTime::fromMSecsSinceEpoch(timestamp * 1000)
        .toLocalTime()
        .toString(QStringLiteral("yyyy-MM-dd hh:mm:ss"));
}

void TransactionDetailService::clearDetails()
{
    m_direction.clear();
    m_amount.clear();
    m_fee.clear();
    m_transactionState.clear();
    m_blockText.clear();
    m_timeText.clear();
    m_fromAddress.clear();
    m_toAddress.clear();
    m_method.clear();
    m_extraText.clear();
}

void TransactionDetailService::load(const QString &chain,
                                    const QString &identifier,
                                    const QString &walletAddress)
{
    ++m_generation;
    const quint64 generation = m_generation;
    SailVaultNetwork::cancelOutstanding(&m_network);

    clearDetails();
    m_chain = chain.trimmed();
    m_identifier = identifier.trimmed();
    m_walletAddress = walletAddress.trimmed();

    if (SailVaultNetwork::offlineModeEnabled()) {
        m_loading = false;
        m_loaded = false;
        m_status = QStringLiteral(
            "FAIL · Offline mode · transaction details require a provider request");
        emit stateChanged();
        return;
    }

    m_loading = true;
    m_loaded = false;
    m_status = QStringLiteral("Loading transaction details…");

    emit stateChanged();

    if (m_identifier.isEmpty()) {
        fail(QStringLiteral("Transaction identifier is empty"), generation);
        return;
    }

    if (m_chain == QStringLiteral("Ethereum")) {
        loadEthereum(m_identifier, m_walletAddress, generation);
    } else if (m_chain == QStringLiteral("Bitcoin")) {
        loadBitcoin(m_identifier, m_walletAddress, generation);
    } else if (m_chain == QStringLiteral("Solana")) {
        loadSolana(m_identifier, m_walletAddress, generation);
    } else {
        fail(QStringLiteral("Unsupported transaction chain"), generation);
    }
}

void TransactionDetailService::fail(const QString &message,
                                    quint64 generation)
{
    if (generation != m_generation)
        return;

    m_loading = false;
    m_loaded = false;
    m_status = QStringLiteral("FAIL · %1").arg(message);

    emit stateChanged();
}

void TransactionDetailService::succeed(quint64 generation)
{
    if (generation != m_generation)
        return;

    m_loading = false;
    m_loaded = true;
    m_status = QStringLiteral("PASS · Transaction details loaded");

    emit stateChanged();
}

void TransactionDetailService::loadEthereum(const QString &hash,
                                            const QString &walletAddress,
                                            quint64 generation)
{
    const QString endpoint =
        normalizeBaseUrl(
            setting(kEthereumExplorerKey, kDefaultEthereumExplorer))
        + QStringLiteral("/transactions/")
        + QString::fromUtf8(QUrl::toPercentEncoding(hash));

    QNetworkRequest request{QUrl(endpoint)};
    request.setRawHeader("Accept", "application/json");

    QNetworkReply *reply = m_network.get(request);
    SailVaultNetwork::armTimeout(reply);

    connect(reply, &QNetworkReply::finished, this,
            [this, reply, walletAddress, generation]() {
        const QByteArray payload = reply->readAll();
        const QString networkError = networkErrorText(reply);
        reply->deleteLater();

        if (generation != m_generation)
            return;

        if (!networkError.isEmpty()) {
            fail(networkError, generation);
            return;
        }

        QJsonParseError parseError;
        const QJsonDocument document =
            QJsonDocument::fromJson(payload, &parseError);

        if (parseError.error != QJsonParseError::NoError
                || !document.isObject()) {
            fail(QStringLiteral("invalid Ethereum transaction response"),
                 generation);
            return;
        }

        const QJsonObject tx = document.object();

        m_fromAddress =
            addressHash(tx.value(QStringLiteral("from")));
        m_toAddress =
            addressHash(tx.value(QStringLiteral("to")));

        const QString rawValue =
            jsonScalarString(tx.value(QStringLiteral("value")));

        const QString formattedValue =
            formatEthereumWei(rawValue);

        if (!formattedValue.isEmpty())
            m_amount = formattedValue;

        QJsonValue feeValue =
            tx.value(QStringLiteral("fee"));

        QString rawFee;

        if (feeValue.isObject()) {
            rawFee =
                jsonScalarString(
                    feeValue.toObject().value(QStringLiteral("value")));
        } else {
            rawFee = jsonScalarString(feeValue);
        }

        if (!rawFee.isEmpty()) {
            const QString formattedFee =
                formatEthereumWei(rawFee);

            if (!formattedFee.isEmpty())
                m_fee = formattedFee;
        }

        if (m_fee.isEmpty()) {
            const QString gasUsed =
                jsonScalarString(
                    tx.value(QStringLiteral("gas_used")));
            const QString gasPrice =
                jsonScalarString(
                    tx.value(QStringLiteral("gas_price")));

            bool gasUsedOk = false;
            bool gasPriceOk = false;

            const qulonglong used =
                gasUsed.toULongLong(&gasUsedOk);
            const qulonglong price =
                gasPrice.toULongLong(&gasPriceOk);

            if (gasUsedOk && gasPriceOk) {
                boost::multiprecision::cpp_int fee =
                    boost::multiprecision::cpp_int(used)
                    * boost::multiprecision::cpp_int(price);

                m_fee = formatEthereumWei(
                    QString::fromStdString(
                        fee.convert_to<std::string>()));
            }
        }

        const QString statusText =
            tx.value(QStringLiteral("status")).toString();

        if (statusText.compare(
                QStringLiteral("ok"),
                Qt::CaseInsensitive) == 0
                || statusText.compare(
                    QStringLiteral("success"),
                    Qt::CaseInsensitive) == 0) {
            m_transactionState = QStringLiteral("Confirmed");
        } else if (statusText.compare(
                       QStringLiteral("error"),
                       Qt::CaseInsensitive) == 0
                   || statusText.compare(
                       QStringLiteral("failed"),
                       Qt::CaseInsensitive) == 0) {
            m_transactionState = QStringLiteral("Failed");
        } else {
            m_transactionState = statusText;
        }

        QString block =
            jsonScalarString(
                tx.value(QStringLiteral("block")));

        if (block.isEmpty()) {
            block =
                jsonScalarString(
                    tx.value(QStringLiteral("block_number")));
        }

        if (!block.isEmpty())
            m_blockText = QStringLiteral("Block %1").arg(block);

        const QJsonValue timestampValue =
            tx.value(QStringLiteral("timestamp"));

        if (timestampValue.isString()) {
            m_timeText =
                isoTimeToLocal(timestampValue.toString());
        } else if (timestampValue.isDouble()) {
            m_timeText =
                unixTimeToLocal(
                    static_cast<qint64>(
                        timestampValue.toDouble()));
        }

        m_method =
            tx.value(QStringLiteral("method")).toString();

        const bool outgoing =
            !walletAddress.isEmpty()
            && m_fromAddress.compare(
                   walletAddress,
                   Qt::CaseInsensitive) == 0;

        const bool incoming =
            !walletAddress.isEmpty()
            && m_toAddress.compare(
                   walletAddress,
                   Qt::CaseInsensitive) == 0;

        if (outgoing && !incoming)
            m_direction = QStringLiteral("Sent");
        else if (incoming && !outgoing)
            m_direction = QStringLiteral("Received");
        else
            m_direction = QStringLiteral("Ethereum transaction");

        QStringList extras;

        const QString gasUsed =
            jsonScalarString(
                tx.value(QStringLiteral("gas_used")));

        if (!gasUsed.isEmpty())
            extras << QStringLiteral("Gas used: %1").arg(gasUsed);

        const QString gasLimit =
            jsonScalarString(
                tx.value(QStringLiteral("gas_limit")));

        if (!gasLimit.isEmpty())
            extras << QStringLiteral("Gas limit: %1").arg(gasLimit);

        m_extraText =
            extras.join(QStringLiteral(" · "));

        succeed(generation);
    });
}

void TransactionDetailService::loadBitcoin(const QString &txid,
                                           const QString &walletAddress,
                                           quint64 generation)
{
    const QString endpoint =
        normalizeBaseUrl(
            setting(kBitcoinApiKey, kDefaultBitcoinApi))
        + QStringLiteral("/tx/")
        + QString::fromUtf8(QUrl::toPercentEncoding(txid));

    QNetworkRequest request{QUrl(endpoint)};
    request.setRawHeader("Accept", "application/json");

    QNetworkReply *reply = m_network.get(request);
    SailVaultNetwork::armTimeout(reply);

    connect(reply, &QNetworkReply::finished, this,
            [this, reply, walletAddress, generation]() {
        const QByteArray payload = reply->readAll();
        const QString networkError = networkErrorText(reply);
        reply->deleteLater();

        if (generation != m_generation)
            return;

        if (!networkError.isEmpty()) {
            fail(networkError, generation);
            return;
        }

        QJsonParseError parseError;
        const QJsonDocument document =
            QJsonDocument::fromJson(payload, &parseError);

        if (parseError.error != QJsonParseError::NoError
                || !document.isObject()) {
            fail(QStringLiteral("invalid Bitcoin transaction response"),
                 generation);
            return;
        }

        const QJsonObject tx = document.object();
        const QJsonArray inputs =
            tx.value(QStringLiteral("vin")).toArray();
        const QJsonArray outputs =
            tx.value(QStringLiteral("vout")).toArray();

        qint64 spent = 0;
        qint64 received = 0;

        for (const QJsonValue &value : inputs) {
            const QJsonObject input = value.toObject();
            const QJsonObject prevout =
                input.value(QStringLiteral("prevout")).toObject();

            if (prevout.value(
                    QStringLiteral("scriptpubkey_address")).toString()
                    == walletAddress) {
                spent += static_cast<qint64>(
                    prevout.value(QStringLiteral("value")).toDouble());
            }
        }

        for (const QJsonValue &value : outputs) {
            const QJsonObject output = value.toObject();

            if (output.value(
                    QStringLiteral("scriptpubkey_address")).toString()
                    == walletAddress) {
                received += static_cast<qint64>(
                    output.value(QStringLiteral("value")).toDouble());
            }
        }

        const qint64 net = received - spent;

        if (net > 0) {
            m_direction = QStringLiteral("Received");
            m_amount = formatBitcoin(net);
        } else if (net < 0) {
            m_direction = QStringLiteral("Sent");
            m_amount = formatBitcoin(-net);
        } else {
            m_direction = QStringLiteral("Bitcoin transaction");
            m_amount = QStringLiteral("0.0 BTC net");
        }

        const qint64 fee =
            static_cast<qint64>(
                tx.value(QStringLiteral("fee")).toDouble());

        if (fee >= 0)
            m_fee = formatBitcoin(fee);

        const QJsonObject status =
            tx.value(QStringLiteral("status")).toObject();

        const bool confirmed =
            status.value(QStringLiteral("confirmed")).toBool(false);

        m_transactionState =
            confirmed
                ? QStringLiteral("Confirmed")
                : QStringLiteral("Pending");

        if (confirmed) {
            const QString height =
                jsonScalarString(
                    status.value(QStringLiteral("block_height")));

            if (!height.isEmpty())
                m_blockText =
                    QStringLiteral("Block %1").arg(height);

            m_timeText =
                unixTimeToLocal(
                    static_cast<qint64>(
                        status.value(
                            QStringLiteral("block_time")).toDouble()));
        }

        const QString size =
            jsonScalarString(
                tx.value(QStringLiteral("size")));

        const QString weight =
            jsonScalarString(
                tx.value(QStringLiteral("weight")));

        QStringList extras;
        extras << QStringLiteral("Inputs: %1").arg(inputs.size())
               << QStringLiteral("Outputs: %1").arg(outputs.size());

        if (!size.isEmpty())
            extras << QStringLiteral("Size: %1 bytes").arg(size);

        if (!weight.isEmpty())
            extras << QStringLiteral("Weight: %1").arg(weight);

        m_extraText =
            extras.join(QStringLiteral(" · "));

        succeed(generation);
    });
}

void TransactionDetailService::loadSolana(const QString &signature,
                                          const QString &walletAddress,
                                          quint64 generation)
{
    QNetworkRequest request{
        QUrl(setting(kSolanaRpcKey, kDefaultSolanaRpc))
    };

    request.setHeader(
        QNetworkRequest::ContentTypeHeader,
        QStringLiteral("application/json"));
    request.setRawHeader("Accept", "application/json");

    QJsonObject config;
    config.insert(QStringLiteral("commitment"),
                  QStringLiteral("confirmed"));
    config.insert(QStringLiteral("encoding"),
                  QStringLiteral("jsonParsed"));
    config.insert(QStringLiteral("maxSupportedTransactionVersion"), 0);

    QJsonArray params;
    params.append(signature);
    params.append(config);

    QJsonObject body;
    body.insert(QStringLiteral("jsonrpc"),
                QStringLiteral("2.0"));
    body.insert(QStringLiteral("id"), 1);
    body.insert(QStringLiteral("method"),
                QStringLiteral("getTransaction"));
    body.insert(QStringLiteral("params"), params);

    QNetworkReply *reply =
        m_network.post(
            request,
            QJsonDocument(body).toJson(
                QJsonDocument::Compact));
    SailVaultNetwork::armTimeout(reply);

    connect(reply, &QNetworkReply::finished, this,
            [this, reply, walletAddress, generation]() {
        const QByteArray payload = reply->readAll();
        const QString networkError = networkErrorText(reply);
        reply->deleteLater();

        if (generation != m_generation)
            return;

        if (!networkError.isEmpty()) {
            fail(networkError, generation);
            return;
        }

        QJsonParseError parseError;
        const QJsonDocument document =
            QJsonDocument::fromJson(payload, &parseError);

        if (parseError.error != QJsonParseError::NoError
                || !document.isObject()) {
            fail(QStringLiteral("invalid Solana transaction response"),
                 generation);
            return;
        }

        const QJsonObject root = document.object();

        if (root.contains(QStringLiteral("error"))) {
            const QJsonObject error =
                root.value(QStringLiteral("error")).toObject();

            fail(error.value(QStringLiteral("message"))
                     .toString(QStringLiteral("Solana RPC error")),
                 generation);
            return;
        }

        const QJsonValue resultValue =
            root.value(QStringLiteral("result"));

        if (!resultValue.isObject()) {
            fail(QStringLiteral(
                     "transaction is not available from the Solana RPC"),
                 generation);
            return;
        }

        const QJsonObject result = resultValue.toObject();

        m_blockText =
            QStringLiteral("Slot %1")
                .arg(jsonScalarString(
                    result.value(QStringLiteral("slot"))));

        m_timeText =
            unixTimeToLocal(
                static_cast<qint64>(
                    result.value(
                        QStringLiteral("blockTime")).toDouble()));

        const QJsonObject meta =
            result.value(QStringLiteral("meta")).toObject();

        const QJsonValue errorValue =
            meta.value(QStringLiteral("err"));

        m_transactionState =
            (errorValue.isNull() || errorValue.isUndefined())
                ? QStringLiteral("Confirmed")
                : QStringLiteral("Failed");

        const qint64 fee =
            static_cast<qint64>(
                meta.value(QStringLiteral("fee")).toDouble());

        if (fee >= 0)
            m_fee = formatSolana(fee);

        const QJsonObject transaction =
            result.value(QStringLiteral("transaction")).toObject();

        const QJsonObject message =
            transaction.value(QStringLiteral("message")).toObject();

        const QJsonArray accountKeys =
            message.value(QStringLiteral("accountKeys")).toArray();

        int walletIndex = -1;

        for (int i = 0; i < accountKeys.size(); ++i) {
            const QJsonValue keyValue = accountKeys.at(i);

            QString pubkey;

            if (keyValue.isString()) {
                pubkey = keyValue.toString();
            } else if (keyValue.isObject()) {
                pubkey =
                    keyValue.toObject()
                        .value(QStringLiteral("pubkey"))
                        .toString();
            }

            if (pubkey == walletAddress) {
                walletIndex = i;
                break;
            }
        }

        const QJsonArray preBalances =
            meta.value(QStringLiteral("preBalances")).toArray();
        const QJsonArray postBalances =
            meta.value(QStringLiteral("postBalances")).toArray();

        if (walletIndex >= 0
                && walletIndex < preBalances.size()
                && walletIndex < postBalances.size()) {
            const qint64 before =
                static_cast<qint64>(
                    preBalances.at(walletIndex).toDouble());

            const qint64 after =
                static_cast<qint64>(
                    postBalances.at(walletIndex).toDouble());

            const qint64 delta = after - before;

            if (delta > 0) {
                m_direction = QStringLiteral("Wallet balance increased");
                m_amount = formatSolana(delta);
            } else if (delta < 0) {
                m_direction = QStringLiteral("Wallet balance decreased");
                m_amount = formatSolana(-delta);
            } else {
                m_direction = QStringLiteral("Solana transaction");
                m_amount = QStringLiteral("0.0 SOL balance change");
            }
        } else {
            m_direction = QStringLiteral("Solana transaction");
        }

        QStringList extras;

        const QString computeUnits =
            jsonScalarString(
                meta.value(
                    QStringLiteral("computeUnitsConsumed")));

        if (!computeUnits.isEmpty())
            extras << QStringLiteral("Compute units: %1")
                          .arg(computeUnits);

        extras << QStringLiteral("Accounts: %1")
                      .arg(accountKeys.size());

        const QJsonArray preTokenBalances =
            meta.value(
                QStringLiteral("preTokenBalances")).toArray();

        const QJsonArray postTokenBalances =
            meta.value(
                QStringLiteral("postTokenBalances")).toArray();

        if (!preTokenBalances.isEmpty()
                || !postTokenBalances.isEmpty()) {
            extras << QStringLiteral("Token balance metadata present");
        }

        m_extraText =
            extras.join(QStringLiteral(" · "));

        succeed(generation);
    });
}
