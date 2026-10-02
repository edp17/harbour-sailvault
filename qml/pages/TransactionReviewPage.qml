import QtQuick 2.0
import Sailfish.Silica 1.0
import org.sailfishos.sailvault 1.0

Page {
    id: page

    property var transaction: ({})
    property var vault
    property string ethereumRpcUrl
    property string bitcoinApiUrl
    property string solanaRpcUrl

    AppTools {
        id: tools
    }

    EthereumUnsignedTransactionService {
        id: ethBuilder
    }

    BitcoinUnsignedTransactionService {
        id: btcBuilder
    }

    SolanaUnsignedTransactionService {
        id: solBuilder
    }

    DevelopmentSigningService {
        id: devSigner
    }

    Connections {
        target: vault
        onStateChanged: {
            if (vault && !vault.walletLoaded)
                devSigner.reset()
        }
    }

    Timer {
        id: copiedTimer
        interval: 1800
        onTriggered: copiedLabel.visible = false
    }

    function constructionDetail() {
        if (transaction.chain === "Ethereum")
            return "M52 keeps M49 construction intact and adds a published-test-wallet-only sign/verify proof. The signature is verified inside C++ and discarded; production signing remains disabled."
        if (transaction.chain === "Bitcoin")
            return "M52 preserves M50 Bitcoin construction unchanged. Bitcoin signing remains disconnected; dedicated secure change derivation is still required before any real-funds signing work."
        if (transaction.chain === "Solana")
            return "M52 preserves the proven M51r2 Solana constructor unchanged. Solana signing remains disconnected and the zero-signature transaction template is inspection-only."
        return "Chain-specific transaction construction is intentionally unavailable."
    }

    SilicaFlickable {
        anchors.fill: parent
        contentHeight: content.height + Theme.paddingLarge

        VerticalScrollDecorator { }

        Column {
            id: content
            width: parent.width
            spacing: Theme.paddingLarge

            PageHeader {
                title: "Review " + (transaction.chain || "transaction") + " intent"
                description: (transaction.chain === "Ethereum"
                              || transaction.chain === "Bitcoin"
                              || transaction.chain === "Solana")
                             ? "Reviewed intent · optional unsigned construction"
                             : "Unsigned snapshot · signing disabled"
            }

            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * Theme.horizontalPageMargin
                text: transaction.amountDisplay || "—"
                color: Theme.highlightColor
                font.pixelSize: Theme.fontSizeHuge
                horizontalAlignment: Text.AlignHCenter
                wrapMode: Text.Wrap
            }

            DetailItem {
                label: "Network"
                value: transaction.network || "—"
            }

            DetailItem {
                label: "Model"
                value: transaction.modelVersion !== undefined
                       ? "SailVault intent v" + transaction.modelVersion
                       : "—"
            }

            DetailItem {
                label: "Displayed balance"
                value: transaction.availableBalance && transaction.availableBalance.length > 0
                       ? transaction.availableBalance
                       : "—"
            }

            DetailItem {
                label: "Funding check"
                value: !transaction.balanceKnown
                       ? "Balance unknown"
                       : (transaction.amountWithinBalance
                          ? "Amount within displayed balance"
                          : "Amount exceeds displayed balance")
            }

            SectionHeader {
                text: "Addresses"
            }

            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * Theme.horizontalPageMargin
                text: "From\n" + (transaction.sourceAddress || "—")
                color: Theme.secondaryColor
                font.pixelSize: Theme.fontSizeSmall
                wrapMode: Text.WrapAnywhere
            }

            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * Theme.horizontalPageMargin
                text: "To\n" + (transaction.destinationAddress || "—")
                color: Theme.primaryColor
                font.pixelSize: Theme.fontSizeSmall
                wrapMode: Text.WrapAnywhere
            }

            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * Theme.horizontalPageMargin
                visible: transaction.balanceWarning && transaction.balanceWarning.length > 0
                text: transaction.balanceWarning || ""
                color: Theme.highlightColor
                font.pixelSize: Theme.fontSizeSmall
                wrapMode: Text.Wrap
            }

            SectionHeader {
                text: "Intent fingerprint"
            }

            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * Theme.horizontalPageMargin
                text: "SHA-256 binds chain, network, source, destination and normalized amount. "
                      + "M52 revalidates this fingerprint before construction, and again before the Ethereum development sign/verify boundary can run."
                color: Theme.secondaryColor
                font.pixelSize: Theme.fontSizeSmall
                wrapMode: Text.Wrap
            }

            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * Theme.horizontalPageMargin
                text: transaction.fingerprint || "—"
                color: Theme.primaryColor
                font.pixelSize: Theme.fontSizeExtraSmall
                wrapMode: Text.WrapAnywhere
                horizontalAlignment: Text.AlignHCenter
            }

            Button {
                anchors.horizontalCenter: parent.horizontalCenter
                enabled: transaction.fingerprint && transaction.fingerprint.length > 0
                text: "Copy intent fingerprint"
                onClicked: {
                    tools.copyText(transaction.fingerprint)
                    copiedLabel.text = "Intent fingerprint copied"
                    copiedLabel.visible = true
                    copiedTimer.restart()
                }
            }

            SectionHeader {
                visible: transaction.chain === "Ethereum"
                text: "Ethereum unsigned construction"
            }

            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * Theme.horizontalPageMargin
                visible: transaction.chain === "Ethereum"
                text: "Construction is explicit because it contacts the configured Ethereum RPC. "
                      + "The provider receives the public source address for the pending nonce and the public destination address for an eth_getCode EOA check. "
                      + "The transfer amount stays local. No secret material is requested."
                color: Theme.secondaryColor
                font.pixelSize: Theme.fontSizeSmall
                wrapMode: Text.Wrap
            }

            Button {
                anchors.horizontalCenter: parent.horizontalCenter
                visible: transaction.chain === "Ethereum"
                enabled: !ethBuilder.loading && ethereumRpcUrl.length > 0
                text: ethBuilder.loading
                      ? "Fetching network inputs…"
                      : (ethBuilder.constructed
                         ? "Reconstruct with fresh inputs"
                         : "Construct unsigned Ethereum payload")
                onClicked: {
                    devSigner.reset()
                    ethBuilder.construct(transaction, ethereumRpcUrl)
                }
            }

            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * Theme.horizontalPageMargin
                visible: transaction.chain === "Ethereum"
                text: ethBuilder.status
                color: ethBuilder.constructed ? Theme.highlightColor : Theme.secondaryColor
                font.pixelSize: Theme.fontSizeSmall
                wrapMode: Text.Wrap
            }

            DetailItem {
                visible: transaction.chain === "Ethereum" && ethBuilder.constructed
                label: "RPC provider"
                value: ethBuilder.providerHost || "—"
            }

            DetailItem {
                visible: transaction.chain === "Ethereum" && ethBuilder.constructed
                label: "Chain ID"
                value: ethBuilder.chainId || "—"
            }

            DetailItem {
                visible: transaction.chain === "Ethereum" && ethBuilder.constructed
                label: "Pending nonce"
                value: ethBuilder.nonce || "—"
            }

            DetailItem {
                visible: transaction.chain === "Ethereum" && ethBuilder.constructed
                label: "Gas limit"
                value: ethBuilder.gasLimit || "—"
            }

            DetailItem {
                visible: transaction.chain === "Ethereum" && ethBuilder.constructed
                label: "Latest base fee"
                value: ethBuilder.baseFeeGwei || "—"
            }

            DetailItem {
                visible: transaction.chain === "Ethereum" && ethBuilder.constructed
                label: "Priority fee"
                value: ethBuilder.maxPriorityFeeGwei || "—"
            }

            DetailItem {
                visible: transaction.chain === "Ethereum" && ethBuilder.constructed
                label: "Max fee per gas"
                value: ethBuilder.maxFeeGwei || "—"
            }

            DetailItem {
                visible: transaction.chain === "Ethereum" && ethBuilder.constructed
                label: "Maximum network fee"
                value: ethBuilder.maximumNetworkFeeEth || "—"
            }

            DetailItem {
                visible: transaction.chain === "Ethereum" && ethBuilder.constructed
                label: "Fee source"
                value: ethBuilder.feeSource || "—"
            }

            DetailItem {
                visible: transaction.chain === "Ethereum" && ethBuilder.constructed
                label: "Constructed"
                value: ethBuilder.constructedAt || "—"
            }

            SectionHeader {
                visible: transaction.chain === "Ethereum" && ethBuilder.constructed
                text: "EIP-1559 signing payload"
            }

            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * Theme.horizontalPageMargin
                visible: transaction.chain === "Ethereum" && ethBuilder.constructed
                text: "Unsigned type-2 payload\n" + ethBuilder.unsignedPayloadHex
                color: Theme.secondaryColor
                font.pixelSize: Theme.fontSizeExtraSmall
                wrapMode: Text.WrapAnywhere
            }

            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * Theme.horizontalPageMargin
                visible: transaction.chain === "Ethereum" && ethBuilder.constructed
                text: "Wallet Core Keccak-256 signing hash\n" + ethBuilder.signingHashHex
                color: Theme.secondaryColor
                font.pixelSize: Theme.fontSizeExtraSmall
                wrapMode: Text.WrapAnywhere
            }

            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * Theme.horizontalPageMargin
                visible: transaction.chain === "Ethereum" && ethBuilder.constructed
                text: "Construction fingerprint (SHA-256)\n" + ethBuilder.constructionFingerprint
                color: Theme.primaryColor
                font.pixelSize: Theme.fontSizeExtraSmall
                wrapMode: Text.WrapAnywhere
            }

            Button {
                anchors.horizontalCenter: parent.horizontalCenter
                visible: transaction.chain === "Ethereum" && ethBuilder.constructed
                enabled: ethBuilder.constructionFingerprint.length > 0
                text: "Copy construction fingerprint"
                onClicked: {
                    tools.copyText(ethBuilder.constructionFingerprint)
                    copiedLabel.text = "Construction fingerprint copied"
                    copiedLabel.visible = true
                    copiedTimer.restart()
                }
            }

            SectionHeader {
                visible: transaction.chain === "Ethereum" && ethBuilder.constructed
                text: "M52 development signing proof"
            }

            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * Theme.horizontalPageMargin
                visible: transaction.chain === "Ethereum" && ethBuilder.constructed
                text: "This is deliberately limited to the published BIP39 development wallet. "
                      + "WalletVault reopens Sailfish Secrets only after the explicit action below, re-derives ETH/BTC/SOL and refuses any wallet that differs from the public test vectors. "
                      + "Wallet Core then signs and verifies the already-reviewed Ethereum hash inside C++. The raw signature is wiped and never exposed to QML, so M52 still does not assemble a signed transaction."
                color: Theme.highlightColor
                font.pixelSize: Theme.fontSizeSmall
                wrapMode: Text.Wrap
            }

            DetailItem {
                visible: transaction.chain === "Ethereum" && ethBuilder.constructed
                label: "Wallet restriction"
                value: "Published test mnemonic / addresses only"
            }

            DetailItem {
                visible: transaction.chain === "Ethereum" && ethBuilder.constructed
                label: "Construction freshness"
                value: "Must be ≤ 2 minutes old"
            }

            Button {
                anchors.horizontalCenter: parent.horizontalCenter
                visible: transaction.chain === "Ethereum" && ethBuilder.constructed
                enabled: vault && vault.walletLoaded && !devSigner.signing
                text: devSigner.signing
                      ? "Signing / verifying…"
                      : (devSigner.signatureVerified
                         ? "Repeat development sign / verify"
                         : "Development sign / verify")
                onClicked: devSigner.signEthereum(vault, ethBuilder, transaction)
            }

            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * Theme.horizontalPageMargin
                visible: transaction.chain === "Ethereum" && ethBuilder.constructed
                text: !vault || !vault.walletLoaded
                      ? "Load/unlock the public development wallet before running the M52 sign/verify proof."
                      : devSigner.status
                color: devSigner.signatureVerified ? Theme.highlightColor : Theme.secondaryColor
                font.pixelSize: Theme.fontSizeSmall
                wrapMode: Text.Wrap
            }

            DetailItem {
                visible: transaction.chain === "Ethereum" && devSigner.signatureVerified
                label: "Verified signer"
                value: devSigner.signerAddress || "—"
            }

            DetailItem {
                visible: transaction.chain === "Ethereum" && devSigner.signatureVerified
                label: "Signature handling"
                value: String(devSigner.signatureSize) + " bytes · verified · discarded"
            }

            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * Theme.horizontalPageMargin
                visible: transaction.chain === "Ethereum" && devSigner.signatureVerified
                text: "Signed Wallet Core hash\n" + (devSigner.signingHashHex || "—")
                color: Theme.secondaryColor
                font.pixelSize: Theme.fontSizeExtraSmall
                wrapMode: Text.WrapAnywhere
            }

            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * Theme.horizontalPageMargin
                visible: transaction.chain === "Ethereum" && devSigner.signatureVerified
                text: "Bound construction fingerprint\n" + (devSigner.constructionFingerprint || "—")
                color: Theme.secondaryColor
                font.pixelSize: Theme.fontSizeExtraSmall
                wrapMode: Text.WrapAnywhere
            }

            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * Theme.horizontalPageMargin
                visible: transaction.chain === "Ethereum" && devSigner.signatureVerified
                text: "Discarded-signature proof fingerprint (SHA-256)\n"
                      + (devSigner.signatureProofFingerprint || "—")
                color: Theme.primaryColor
                font.pixelSize: Theme.fontSizeExtraSmall
                wrapMode: Text.WrapAnywhere
            }

            DetailItem {
                visible: transaction.chain === "Ethereum" && devSigner.signatureVerified
                label: "Verified at"
                value: devSigner.signedAt || "—"
            }

            SectionHeader {
                visible: transaction.chain === "Bitcoin"
                text: "Bitcoin unsigned construction"
            }

            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * Theme.horizontalPageMargin
                visible: transaction.chain === "Bitcoin"
                text: "Construction is explicit because it contacts the configured Bitcoin API. "
                      + "The provider receives the public source address to enumerate UTXOs; the destination and transfer amount stay local. "
                      + "Only confirmed UTXOs are considered. No secret material is requested."
                color: Theme.secondaryColor
                font.pixelSize: Theme.fontSizeSmall
                wrapMode: Text.Wrap
            }

            Button {
                anchors.horizontalCenter: parent.horizontalCenter
                visible: transaction.chain === "Bitcoin"
                enabled: !btcBuilder.loading && bitcoinApiUrl.length > 0
                text: btcBuilder.loading
                      ? "Fetching UTXOs / fees…"
                      : (btcBuilder.constructed
                         ? "Reconstruct with fresh inputs"
                         : "Construct unsigned Bitcoin transaction")
                onClicked: btcBuilder.construct(transaction, bitcoinApiUrl)
            }

            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * Theme.horizontalPageMargin
                visible: transaction.chain === "Bitcoin"
                text: btcBuilder.status
                color: btcBuilder.constructed ? Theme.highlightColor : Theme.secondaryColor
                font.pixelSize: Theme.fontSizeSmall
                wrapMode: Text.Wrap
            }

            DetailItem {
                visible: transaction.chain === "Bitcoin" && btcBuilder.constructed
                label: "API provider"
                value: btcBuilder.providerHost || "—"
            }

            DetailItem {
                visible: transaction.chain === "Bitcoin" && btcBuilder.constructed
                label: "Fee target"
                value: btcBuilder.feeTarget || "—"
            }

            DetailItem {
                visible: transaction.chain === "Bitcoin" && btcBuilder.constructed
                label: "Provider estimate"
                value: btcBuilder.providerFeeEstimate || "—"
            }

            DetailItem {
                visible: transaction.chain === "Bitcoin" && btcBuilder.constructed
                label: "Selected fee rate"
                value: btcBuilder.feeRateSatVb || "—"
            }

            DetailItem {
                visible: transaction.chain === "Bitcoin" && btcBuilder.constructed
                label: "Confirmed UTXOs"
                value: String(btcBuilder.confirmedUtxoCount)
            }

            DetailItem {
                visible: transaction.chain === "Bitcoin" && btcBuilder.constructed
                label: "Selected inputs"
                value: String(btcBuilder.selectedInputCount)
            }

            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * Theme.horizontalPageMargin
                visible: transaction.chain === "Bitcoin" && btcBuilder.constructed
                text: btcBuilder.selectedInputsSummary
                color: Theme.secondaryColor
                font.pixelSize: Theme.fontSizeExtraSmall
                wrapMode: Text.WrapAnywhere
            }

            DetailItem {
                visible: transaction.chain === "Bitcoin" && btcBuilder.constructed
                label: "Total selected"
                value: btcBuilder.totalInputBtc || "—"
            }

            DetailItem {
                visible: transaction.chain === "Bitcoin" && btcBuilder.constructed
                label: "Transfer amount"
                value: btcBuilder.amountBtc || "—"
            }

            DetailItem {
                visible: transaction.chain === "Bitcoin" && btcBuilder.constructed
                label: "Network fee"
                value: btcBuilder.networkFeeBtc || "—"
            }

            DetailItem {
                visible: transaction.chain === "Bitcoin" && btcBuilder.constructed
                label: "Change"
                value: btcBuilder.changeBtc || "—"
            }

            DetailItem {
                visible: transaction.chain === "Bitcoin" && btcBuilder.constructed
                label: "Estimated size"
                value: btcBuilder.estimatedVbytes || "—"
            }

            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * Theme.horizontalPageMargin
                visible: transaction.chain === "Bitcoin" && btcBuilder.constructed
                text: btcBuilder.changePolicy
                color: Theme.highlightColor
                font.pixelSize: Theme.fontSizeSmall
                wrapMode: Text.Wrap
            }

            DetailItem {
                visible: transaction.chain === "Bitcoin" && btcBuilder.constructed
                label: "Constructed"
                value: btcBuilder.constructedAt || "—"
            }

            SectionHeader {
                visible: transaction.chain === "Bitcoin" && btcBuilder.constructed
                text: "Unsigned Bitcoin artifacts"
            }

            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * Theme.horizontalPageMargin
                visible: transaction.chain === "Bitcoin" && btcBuilder.constructed
                text: "Unsigned transaction
" + btcBuilder.unsignedTransactionHex
                color: Theme.secondaryColor
                font.pixelSize: Theme.fontSizeExtraSmall
                wrapMode: Text.WrapAnywhere
            }

            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * Theme.horizontalPageMargin
                visible: transaction.chain === "Bitcoin" && btcBuilder.constructed
                text: "Unsigned transaction ID (SHA256d)
" + btcBuilder.unsignedTxid
                color: Theme.secondaryColor
                font.pixelSize: Theme.fontSizeExtraSmall
                wrapMode: Text.WrapAnywhere
            }

            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * Theme.horizontalPageMargin
                visible: transaction.chain === "Bitcoin" && btcBuilder.constructed
                text: "PSBT v0 (Base64)
" + btcBuilder.psbtBase64
                color: Theme.secondaryColor
                font.pixelSize: Theme.fontSizeExtraSmall
                wrapMode: Text.WrapAnywhere
            }

            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * Theme.horizontalPageMargin
                visible: transaction.chain === "Bitcoin" && btcBuilder.constructed
                text: "Construction fingerprint (SHA-256)
" + btcBuilder.constructionFingerprint
                color: Theme.primaryColor
                font.pixelSize: Theme.fontSizeExtraSmall
                wrapMode: Text.WrapAnywhere
            }

            Button {
                anchors.horizontalCenter: parent.horizontalCenter
                visible: transaction.chain === "Bitcoin" && btcBuilder.constructed
                enabled: btcBuilder.constructionFingerprint.length > 0
                text: "Copy construction fingerprint"
                onClicked: {
                    tools.copyText(btcBuilder.constructionFingerprint)
                    copiedLabel.text = "Bitcoin construction fingerprint copied"
                    copiedLabel.visible = true
                    copiedTimer.restart()
                }
            }

            SectionHeader {
                visible: transaction.chain === "Solana"
                text: "Solana unsigned construction"
            }

            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * Theme.horizontalPageMargin
                visible: transaction.chain === "Solana"
                text: "Construction is explicit because it contacts the configured Solana RPC. "
                      + "SailVault first verifies the mainnet genesis hash and fetches a finalized recent blockhash. "
                      + "Wallet Core constructs the exact public signing message without a private key. "
                      + "getFeeForMessage receives that public message, so the RPC can observe source, destination and amount."
                color: Theme.secondaryColor
                font.pixelSize: Theme.fontSizeSmall
                wrapMode: Text.Wrap
            }

            Button {
                anchors.horizontalCenter: parent.horizontalCenter
                visible: transaction.chain === "Solana"
                enabled: !solBuilder.loading && solanaRpcUrl.length > 0
                text: solBuilder.loading
                      ? "Fetching blockhash / fee…"
                      : (solBuilder.constructed
                         ? "Reconstruct with fresh blockhash"
                         : "Construct unsigned Solana message")
                onClicked: solBuilder.construct(transaction, solanaRpcUrl)
            }

            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * Theme.horizontalPageMargin
                visible: transaction.chain === "Solana"
                text: solBuilder.status
                color: solBuilder.constructed ? Theme.highlightColor : Theme.secondaryColor
                font.pixelSize: Theme.fontSizeSmall
                wrapMode: Text.Wrap
            }

            DetailItem {
                visible: transaction.chain === "Solana" && solBuilder.constructed
                label: "RPC provider"
                value: solBuilder.providerHost || "—"
            }

            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * Theme.horizontalPageMargin
                visible: transaction.chain === "Solana" && solBuilder.constructed
                text: "Mainnet genesis\n" + (solBuilder.genesisHash || "—")
                color: Theme.secondaryColor
                font.pixelSize: Theme.fontSizeExtraSmall
                wrapMode: Text.WrapAnywhere
            }

            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * Theme.horizontalPageMargin
                visible: transaction.chain === "Solana" && solBuilder.constructed
                text: "Recent blockhash\n" + (solBuilder.recentBlockhash || "—")
                color: Theme.secondaryColor
                font.pixelSize: Theme.fontSizeExtraSmall
                wrapMode: Text.WrapAnywhere
            }

            DetailItem {
                visible: transaction.chain === "Solana" && solBuilder.constructed
                label: "Last valid block height"
                value: solBuilder.lastValidBlockHeight || "—"
            }

            DetailItem {
                visible: transaction.chain === "Solana" && solBuilder.constructed
                label: "Transfer amount"
                value: solBuilder.lamports || "—"
            }

            DetailItem {
                visible: transaction.chain === "Solana" && solBuilder.constructed
                label: "Quoted network fee"
                value: (solBuilder.networkFeeSol || "—")
                       + (solBuilder.networkFeeLamports
                          ? " · " + solBuilder.networkFeeLamports : "")
            }

            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * Theme.horizontalPageMargin
                visible: transaction.chain === "Solana" && solBuilder.constructed
                text: "Signer / fee payer\n" + (solBuilder.signerAddress || "—")
                color: Theme.secondaryColor
                font.pixelSize: Theme.fontSizeExtraSmall
                wrapMode: Text.WrapAnywhere
            }

            DetailItem {
                visible: transaction.chain === "Solana" && solBuilder.constructed
                label: "Instruction"
                value: solBuilder.instructionSummary || "—"
            }

            DetailItem {
                visible: transaction.chain === "Solana" && solBuilder.constructed
                label: "Constructed"
                value: solBuilder.constructedAt || "—"
            }

            SectionHeader {
                visible: transaction.chain === "Solana" && solBuilder.constructed
                text: "Unsigned Solana artifacts"
            }

            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * Theme.horizontalPageMargin
                visible: transaction.chain === "Solana" && solBuilder.constructed
                text: "Wallet Core signing message (Base64)\n" + solBuilder.unsignedMessageBase64
                color: Theme.secondaryColor
                font.pixelSize: Theme.fontSizeExtraSmall
                wrapMode: Text.WrapAnywhere
            }

            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * Theme.horizontalPageMargin
                visible: transaction.chain === "Solana" && solBuilder.constructed
                text: "Wallet Core signing message (hex)\n" + solBuilder.unsignedMessageHex
                color: Theme.secondaryColor
                font.pixelSize: Theme.fontSizeExtraSmall
                wrapMode: Text.WrapAnywhere
            }

            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * Theme.horizontalPageMargin
                visible: transaction.chain === "Solana" && solBuilder.constructed
                text: "Zero-signature transaction template (Base64 · not broadcastable)\n"
                      + solBuilder.unsignedTransactionTemplateBase64
                color: Theme.secondaryColor
                font.pixelSize: Theme.fontSizeExtraSmall
                wrapMode: Text.WrapAnywhere
            }

            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * Theme.horizontalPageMargin
                visible: transaction.chain === "Solana" && solBuilder.constructed
                text: "Construction fingerprint (SHA-256)\n" + solBuilder.constructionFingerprint
                color: Theme.primaryColor
                font.pixelSize: Theme.fontSizeExtraSmall
                wrapMode: Text.WrapAnywhere
            }

            Button {
                anchors.horizontalCenter: parent.horizontalCenter
                visible: transaction.chain === "Solana" && solBuilder.constructed
                enabled: solBuilder.constructionFingerprint.length > 0
                text: "Copy construction fingerprint"
                onClicked: {
                    tools.copyText(solBuilder.constructionFingerprint)
                    copiedLabel.text = "Solana construction fingerprint copied"
                    copiedLabel.visible = true
                    copiedTimer.restart()
                }
            }

            Label {
                id: copiedLabel
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * Theme.horizontalPageMargin
                visible: false
                text: ""
                color: Theme.highlightColor
                font.pixelSize: Theme.fontSizeSmall
                horizontalAlignment: Text.AlignHCenter
            }

            SectionHeader {
                text: "Construction boundary"
            }

            DetailItem {
                label: "Chain transaction"
                value: transaction.chain === "Ethereum" && ethBuilder.constructed
                       ? "Unsigned EIP-1559 signing payload constructed"
                       : (transaction.chain === "Bitcoin" && btcBuilder.constructed
                          ? "Unsigned Bitcoin transaction + PSBT v0 constructed"
                          : (transaction.chain === "Solana" && solBuilder.constructed
                             ? "Unsigned Solana signing message constructed"
                             : (transaction.constructionState || "Not constructed")))
            }

            DetailItem {
                label: "Production signing"
                value: "Disabled"
            }

            DetailItem {
                label: "M52 development sign / verify"
                value: transaction.chain === "Ethereum"
                       ? (devSigner.signatureVerified
                          ? "Published test wallet · verified / signature discarded"
                          : "Published test wallet only")
                       : "Not enabled for this chain"
            }

            DetailItem {
                label: "Broadcasting"
                value: "Disabled"
            }

            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * Theme.horizontalPageMargin
                text: constructionDetail()
                color: Theme.secondaryColor
                font.pixelSize: Theme.fontSizeSmall
                wrapMode: Text.Wrap
            }

            Button {
                anchors.horizontalCenter: parent.horizontalCenter
                enabled: false
                text: "Production signing unavailable in M52"
            }

            SectionHeader {
                text: "Security"
            }

            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * Theme.horizontalPageMargin
                text: transaction.chain === "Ethereum"
                      ? "M52 preserves the proven M49 Ethereum constructor and adds a separate development-only C++ signing proof. WalletVault revalidates the published test wallet through Sailfish Secrets, the construction snapshot is reconstructed and freshness-gated, and raw signature/private-key/recovery material never enters QML. Production signing and broadcasting remain disabled."
                      : (transaction.chain === "Bitcoin"
                         ? "M52 preserves the proven M50 Bitcoin boundary unchanged. Bitcoin signing is not connected to Sailfish Secrets yet, and the temporary source-address change policy remains blocked from real-funds signing."
                         : (transaction.chain === "Solana"
                            ? "M52 preserves the proven M51r2 Solana constructor unchanged. Solana signing is not connected to Sailfish Secrets yet; its RPC fee quote still sees only the public signing message, and no broadcast API is available."
                            : "The review snapshot contains public addresses and an amount only. No recovery phrase/private key is requested and signing/broadcasting remain unavailable."))
                color: Theme.secondaryColor
                font.pixelSize: Theme.fontSizeSmall
                wrapMode: Text.Wrap
            }
        }
    }
}
