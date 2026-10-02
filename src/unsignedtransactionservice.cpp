#include "unsignedtransactionservice.h"

#include <QCryptographicHash>
#include <QRegularExpression>

#include <TrustWalletCore/TWAnyAddress.h>
#include <TrustWalletCore/TWCoinType.h>
#include <TrustWalletCore/TWString.h>

namespace {

class TWStringGuard
{
public:
    explicit TWStringGuard(TWString *value = nullptr) : m_value(value) {}
    ~TWStringGuard() { if (m_value) TWStringDelete(m_value); }
    TWString *get() const { return m_value; }
private:
    TWString *m_value;
};

QString stripLeadingZeros(const QString &value)
{
    int first = 0;
    while (first + 1 < value.size() && value.at(first) == QLatin1Char('0'))
        ++first;
    return value.mid(first);
}

} // namespace

UnsignedTransactionService::UnsignedTransactionService(QObject *parent)
    : QObject(parent)
{
    updateState();
}

QString UnsignedTransactionService::chain() const { return m_info.chain; }
QString UnsignedTransactionService::network() const { return m_info.network; }
QString UnsignedTransactionService::symbol() const { return m_info.symbol; }
int UnsignedTransactionService::decimals() const { return m_info.decimals; }
QString UnsignedTransactionService::sourceAddress() const { return m_sourceAddress; }
QString UnsignedTransactionService::destinationAddress() const { return m_destinationAddress; }
QString UnsignedTransactionService::amountText() const { return m_amountText; }
QString UnsignedTransactionService::normalizedAmount() const { return m_normalizedAmount; }
QString UnsignedTransactionService::availableBalance() const { return m_availableBalance; }
bool UnsignedTransactionService::destinationValid() const { return m_destinationValid; }
bool UnsignedTransactionService::amountValid() const { return m_amountValid; }
bool UnsignedTransactionService::balanceKnown() const { return m_balanceKnown; }
bool UnsignedTransactionService::amountWithinBalance() const { return m_amountWithinBalance; }
bool UnsignedTransactionService::readyForReview() const { return m_readyForReview; }
bool UnsignedTransactionService::selfTransfer() const { return m_selfTransfer; }
QString UnsignedTransactionService::status() const { return m_status; }
QString UnsignedTransactionService::balanceWarning() const { return m_balanceWarning; }

void UnsignedTransactionService::setDestinationAddress(const QString &address)
{
    if (m_destinationAddress == address)
        return;
    m_destinationAddress = address;
    updateState();
    emit stateChanged();
}

void UnsignedTransactionService::setAmountText(const QString &amount)
{
    if (m_amountText == amount)
        return;
    m_amountText = amount;
    updateState();
    emit stateChanged();
}

void UnsignedTransactionService::configure(const QString &chain,
                                           const QString &sourceAddress,
                                           const QString &availableBalance)
{
    m_info = chainInfo(chain);
    m_sourceAddress = sourceAddress.trimmed();
    m_availableBalance = availableBalance.trimmed();
    m_destinationAddress.clear();
    m_amountText.clear();
    updateState();
    emit stateChanged();
}

UnsignedTransactionService::ChainInfo
UnsignedTransactionService::chainInfo(const QString &chain)
{
    const QString value = chain.trimmed();
    ChainInfo info;

    if (value.compare(QStringLiteral("Ethereum"), Qt::CaseInsensitive) == 0) {
        info.valid = true;
        info.chain = QStringLiteral("Ethereum");
        info.network = QStringLiteral("Ethereum mainnet · chain ID 1");
        info.symbol = QStringLiteral("ETH");
        info.decimals = 18;
        info.coinType = TWCoinTypeEthereum;
    } else if (value.compare(QStringLiteral("Bitcoin"), Qt::CaseInsensitive) == 0) {
        info.valid = true;
        info.chain = QStringLiteral("Bitcoin");
        info.network = QStringLiteral("Bitcoin mainnet");
        info.symbol = QStringLiteral("BTC");
        info.decimals = 8;
        info.coinType = TWCoinTypeBitcoin;
    } else if (value.compare(QStringLiteral("Solana"), Qt::CaseInsensitive) == 0) {
        info.valid = true;
        info.chain = QStringLiteral("Solana");
        info.network = QStringLiteral("Solana mainnet-beta");
        info.symbol = QStringLiteral("SOL");
        info.decimals = 9;
        info.coinType = TWCoinTypeSolana;
    }

    return info;
}

bool UnsignedTransactionService::validateAddress(const QString &address,
                                                 int coinType)
{
    const QString value = address.trimmed();
    if (value.isEmpty() || coinType < 0)
        return false;

    const QByteArray utf8 = value.toUtf8();
    TWStringGuard string(TWStringCreateWithUTF8Bytes(utf8.constData()));
    if (!string.get())
        return false;

    return TWAnyAddressIsValid(string.get(), static_cast<TWCoinType>(coinType));
}

UnsignedTransactionService::DecimalValue
UnsignedTransactionService::parseDecimal(const QString &text,
                                         int maximumDecimals)
{
    DecimalValue result;
    const QString value = text.trimmed();
    if (value.isEmpty() || maximumDecimals < 0)
        return result;

    static const QRegularExpression pattern(QStringLiteral("^[0-9]+(?:\\.[0-9]+)?$"));
    if (!pattern.match(value).hasMatch())
        return result;

    const int point = value.indexOf(QLatin1Char('.'));
    QString whole = point >= 0 ? value.left(point) : value;
    QString fraction = point >= 0 ? value.mid(point + 1) : QString();

    if (fraction.size() > maximumDecimals)
        return result;

    whole = stripLeadingZeros(whole);
    while (!fraction.isEmpty() && fraction.endsWith(QLatin1Char('0')))
        fraction.chop(1);

    result.normalized = whole;
    if (!fraction.isEmpty())
        result.normalized += QLatin1Char('.') + fraction;

    result.decimals = fraction.size();
    result.digits = stripLeadingZeros(whole + fraction);
    result.zero = true;
    for (int i = 0; i < result.digits.size(); ++i) {
        if (result.digits.at(i) != QLatin1Char('0')) {
            result.zero = false;
            break;
        }
    }
    result.valid = true;
    return result;
}

UnsignedTransactionService::DecimalValue
UnsignedTransactionService::parseBalance(const QString &text,
                                         const QString &symbol,
                                         int maximumDecimals)
{
    QString value = text.trimmed();
    if (value.isEmpty() || value == QStringLiteral("—"))
        return DecimalValue();

    if (!symbol.isEmpty() && value.endsWith(symbol, Qt::CaseInsensitive)) {
        value.chop(symbol.size());
        value = value.trimmed();
    }

    return parseDecimal(value, maximumDecimals);
}

int UnsignedTransactionService::compareDecimal(const DecimalValue &left,
                                               const DecimalValue &right,
                                               int scale)
{
    QString a = left.digits;
    QString b = right.digits;
    if (left.decimals < scale)
        a += QString(scale - left.decimals, QLatin1Char('0'));
    if (right.decimals < scale)
        b += QString(scale - right.decimals, QLatin1Char('0'));
    a = stripLeadingZeros(a);
    b = stripLeadingZeros(b);
    if (a.size() < b.size())
        return -1;
    if (a.size() > b.size())
        return 1;
    return QString::compare(a, b, Qt::CaseSensitive);
}

QString UnsignedTransactionService::canonicalAddress(const QString &chain,
                                                     const QString &address)
{
    const QString value = address.trimmed();
    if (chain == QStringLiteral("Ethereum"))
        return value.toLower();
    return value;
}

QString UnsignedTransactionService::fingerprintFor(const QVariantMap &model)
{
    const QString canonical = QStringLiteral("sailvault-intent-v1\n")
        + model.value(QStringLiteral("chain")).toString() + QLatin1Char('\n')
        + model.value(QStringLiteral("network")).toString() + QLatin1Char('\n')
        + canonicalAddress(model.value(QStringLiteral("chain")).toString(),
                           model.value(QStringLiteral("sourceAddress")).toString()) + QLatin1Char('\n')
        + canonicalAddress(model.value(QStringLiteral("chain")).toString(),
                           model.value(QStringLiteral("destinationAddress")).toString()) + QLatin1Char('\n')
        + model.value(QStringLiteral("normalizedAmount")).toString() + QLatin1Char('\n')
        + model.value(QStringLiteral("symbol")).toString();

    return QString::fromLatin1(QCryptographicHash::hash(
        canonical.toUtf8(), QCryptographicHash::Sha256).toHex());
}

void UnsignedTransactionService::updateState()
{
    m_destinationValid = m_info.valid
        && validateAddress(m_destinationAddress, m_info.coinType);

    const DecimalValue amount = parseDecimal(m_amountText, m_info.decimals);
    m_amountValid = amount.valid && !amount.zero;
    m_normalizedAmount = m_amountValid ? amount.normalized : QString();

    const DecimalValue balance = parseBalance(
        m_availableBalance, m_info.symbol, m_info.decimals);
    m_balanceKnown = balance.valid;
    m_amountWithinBalance = m_balanceKnown && m_amountValid
        && compareDecimal(amount, balance, m_info.decimals) <= 0;

    m_selfTransfer = m_destinationValid
        && !m_sourceAddress.isEmpty()
        && canonicalAddress(m_info.chain, m_sourceAddress)
            == canonicalAddress(m_info.chain, m_destinationAddress);

    m_readyForReview = m_info.valid
        && validateAddress(m_sourceAddress, m_info.coinType)
        && m_destinationValid
        && m_amountValid;

    if (!m_info.valid) {
        m_status = QStringLiteral("Unsupported transaction chain");
    } else if (!validateAddress(m_sourceAddress, m_info.coinType)) {
        m_status = QStringLiteral("Source address is not valid for %1").arg(m_info.chain);
    } else if (m_destinationAddress.trimmed().isEmpty()) {
        m_status = QStringLiteral("Enter a destination address");
    } else if (!m_destinationValid) {
        m_status = QStringLiteral("Destination is not a valid %1 address").arg(m_info.chain);
    } else if (m_amountText.trimmed().isEmpty()) {
        m_status = QStringLiteral("Enter an amount");
    } else if (!m_amountValid) {
        m_status = QStringLiteral("Enter a positive amount with at most %1 decimal places")
            .arg(m_info.decimals);
    } else {
        m_status = QStringLiteral("Ready for local unsigned-intent review");
    }

    m_balanceWarning.clear();
    if (m_amountValid) {
        if (!m_balanceKnown) {
            m_balanceWarning = QStringLiteral(
                "Available balance is unknown; funding is not verified.");
        } else if (!m_amountWithinBalance) {
            m_balanceWarning = QStringLiteral(
                "Amount exceeds the currently displayed balance. Review and unsigned construction are allowed for development testing, but a future signing gate must refuse insufficient funds.");
        } else {
            m_balanceWarning = QStringLiteral(
                "Amount fits the displayed balance, but no network fee has been reserved yet.");
        }
    }

    if (m_selfTransfer) {
        if (!m_balanceWarning.isEmpty())
            m_balanceWarning += QLatin1Char(' ');
        m_balanceWarning += QStringLiteral("Destination is the same as the source address.");
    }
}

QVariantMap UnsignedTransactionService::reviewModel() const
{
    QVariantMap model;
    if (!m_readyForReview)
        return model;

    model.insert(QStringLiteral("modelVersion"), 1);
    model.insert(QStringLiteral("chain"), m_info.chain);
    model.insert(QStringLiteral("network"), m_info.network);
    model.insert(QStringLiteral("symbol"), m_info.symbol);
    model.insert(QStringLiteral("decimals"), m_info.decimals);
    model.insert(QStringLiteral("sourceAddress"), m_sourceAddress);
    model.insert(QStringLiteral("destinationAddress"), m_destinationAddress.trimmed());
    model.insert(QStringLiteral("normalizedAmount"), m_normalizedAmount);
    model.insert(QStringLiteral("amountDisplay"),
                 m_normalizedAmount + QLatin1Char(' ') + m_info.symbol);
    model.insert(QStringLiteral("availableBalance"), m_availableBalance);
    model.insert(QStringLiteral("balanceKnown"), m_balanceKnown);
    model.insert(QStringLiteral("amountWithinBalance"), m_amountWithinBalance);
    model.insert(QStringLiteral("selfTransfer"), m_selfTransfer);
    model.insert(QStringLiteral("balanceWarning"), m_balanceWarning);
    model.insert(QStringLiteral("constructionState"),
                 QStringLiteral("Intent only · chain transaction not constructed"));
    model.insert(QStringLiteral("signingState"), QStringLiteral("Disabled"));
    model.insert(QStringLiteral("broadcastState"), QStringLiteral("Disabled"));
    model.insert(QStringLiteral("fingerprintAlgorithm"), QStringLiteral("SHA-256"));
    model.insert(QStringLiteral("fingerprint"), fingerprintFor(model));
    return model;
}

bool UnsignedTransactionService::validateReviewModel(
        const QVariantMap &model, QString *error)
{
    const auto fail = [error](const QString &message) {
        if (error)
            *error = message;
        return false;
    };

    if (model.value(QStringLiteral("modelVersion")).toInt() != 1)
        return fail(QStringLiteral("unsupported intent model version"));

    const ChainInfo info = chainInfo(model.value(QStringLiteral("chain")).toString());
    if (!info.valid)
        return fail(QStringLiteral("unsupported transaction chain"));

    if (model.value(QStringLiteral("network")).toString() != info.network
            || model.value(QStringLiteral("symbol")).toString() != info.symbol
            || model.value(QStringLiteral("decimals")).toInt() != info.decimals) {
        return fail(QStringLiteral("intent chain metadata does not match SailVault policy"));
    }

    const QString source = model.value(QStringLiteral("sourceAddress")).toString();
    const QString destination = model.value(QStringLiteral("destinationAddress")).toString();
    if (!validateAddress(source, info.coinType)
            || !validateAddress(destination, info.coinType)) {
        return fail(QStringLiteral("intent contains an invalid chain address"));
    }

    const QString normalizedAmount =
        model.value(QStringLiteral("normalizedAmount")).toString();
    const DecimalValue amount = parseDecimal(normalizedAmount, info.decimals);
    if (!amount.valid || amount.zero || amount.normalized != normalizedAmount) {
        return fail(QStringLiteral("intent amount is not canonical"));
    }

    const QString fingerprint = model.value(QStringLiteral("fingerprint")).toString();
    static const QRegularExpression fingerprintPattern(
        QStringLiteral("^[0-9a-f]{64}$"));
    if (!fingerprintPattern.match(fingerprint).hasMatch()
            || fingerprint != fingerprintFor(model)) {
        return fail(QStringLiteral("intent fingerprint does not match reviewed fields"));
    }

    if (error)
        error->clear();
    return true;
}

QVariantMap UnsignedTransactionService::runSelfTest() const
{
    QVariantMap result;

    const ChainInfo eth = chainInfo(QStringLiteral("Ethereum"));
    const ChainInfo btc = chainInfo(QStringLiteral("Bitcoin"));
    const ChainInfo sol = chainInfo(QStringLiteral("Solana"));

    const bool chainMetadata = eth.valid && btc.valid && sol.valid
        && eth.decimals == 18 && btc.decimals == 8 && sol.decimals == 9
        && eth.coinType == TWCoinTypeEthereum
        && btc.coinType == TWCoinTypeBitcoin
        && sol.coinType == TWCoinTypeSolana;

    const bool addressValidation =
        validateAddress(QStringLiteral("0x9858EfFD232B4033E47d90003D41EC34EcaEda94"), eth.coinType)
        && validateAddress(QStringLiteral("bc1qcr8te4kr609gcawutmrza0j4xv80jy8z306fyu"), btc.coinType)
        && validateAddress(QStringLiteral("GjJyeC1r2RgkuoCWMyPYkCWSGSGLcz266EaAkLA27AhL"), sol.coinType)
        && !validateAddress(QStringLiteral("not-an-address"), eth.coinType)
        && !validateAddress(QStringLiteral("not-an-address"), btc.coinType)
        && !validateAddress(QStringLiteral("not-an-address"), sol.coinType);

    const DecimalValue goodEth = parseDecimal(QStringLiteral("001.2300"), 18);
    const DecimalValue goodBtc = parseDecimal(QStringLiteral("0.00000001"), 8);
    const DecimalValue goodSol = parseDecimal(QStringLiteral("10.123456789"), 9);
    const DecimalValue tooPreciseBtc = parseDecimal(QStringLiteral("0.000000001"), 8);
    const DecimalValue exponent = parseDecimal(QStringLiteral("1e-3"), 18);
    const DecimalValue zero = parseDecimal(QStringLiteral("0.000"), 18);
    const bool decimalValidation = goodEth.valid && goodEth.normalized == QStringLiteral("1.23")
        && goodBtc.valid && goodSol.valid
        && !tooPreciseBtc.valid && !exponent.valid && zero.valid && zero.zero;

    QVariantMap snapshot;
    snapshot.insert(QStringLiteral("chain"), QStringLiteral("Ethereum"));
    snapshot.insert(QStringLiteral("network"), QStringLiteral("Ethereum mainnet · chain ID 1"));
    snapshot.insert(QStringLiteral("sourceAddress"), QStringLiteral("0x9858EfFD232B4033E47d90003D41EC34EcaEda94"));
    snapshot.insert(QStringLiteral("destinationAddress"), QStringLiteral("0x9858EfFD232B4033E47d90003D41EC34EcaEda94"));
    snapshot.insert(QStringLiteral("normalizedAmount"), QStringLiteral("1.23"));
    snapshot.insert(QStringLiteral("symbol"), QStringLiteral("ETH"));
    const QString firstFingerprint = fingerprintFor(snapshot);
    snapshot.insert(QStringLiteral("normalizedAmount"), QStringLiteral("1.24"));
    const QString secondFingerprint = fingerprintFor(snapshot);
    const bool fingerprintValidation = firstFingerprint.size() == 64
        && secondFingerprint.size() == 64
        && firstFingerprint != secondFingerprint;

    const bool passed = chainMetadata && addressValidation
        && decimalValidation && fingerprintValidation;

    result.insert(QStringLiteral("passed"), passed);
    result.insert(QStringLiteral("chainMetadataPassed"), chainMetadata);
    result.insert(QStringLiteral("addressValidationPassed"), addressValidation);
    result.insert(QStringLiteral("decimalValidationPassed"), decimalValidation);
    result.insert(QStringLiteral("fingerprintValidationPassed"), fingerprintValidation);
    result.insert(QStringLiteral("detail"), passed
        ? QStringLiteral("ETH/BTC/SOL intent metadata, Wallet Core address checks, exact decimal parsing and fingerprint binding passed")
        : QStringLiteral("Unsigned transaction intent self-test failed"));
    return result;
}
