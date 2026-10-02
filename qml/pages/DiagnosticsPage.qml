import QtQuick 2.0
import Sailfish.Silica 1.0
import org.sailfishos.sailvault 1.0

Page {
    id: page

    property var storageInfo: ({})
    property var networkInfo: ({})
    property var transactionInfo: ({})
    property var ethereumConstructionInfo: ({})
    property var bitcoinConstructionInfo: ({})
    property var solanaConstructionInfo: ({})
    property var signingBoundaryInfo: ({})
    property var portfolio

    function refreshStorageInfo() {
        storageInfo = tools.settingsDiagnostics()
        networkInfo = tools.networkSecurityDiagnostics()
        transactionInfo = unsignedIntent.runSelfTest()
        ethereumConstructionInfo = ethereumBuilder.runSelfTest()
        bitcoinConstructionInfo = bitcoinBuilder.runSelfTest()
        solanaConstructionInfo = solanaBuilder.runSelfTest()
        signingBoundaryInfo = developmentSigner.runSelfTest()
    }

    function runAllDiagnostics() {
        storageInfo = tools.rerunSettingsPersistenceProbe()
        networkInfo = tools.networkSecurityDiagnostics()
        transactionInfo = unsignedIntent.runSelfTest()
        ethereumConstructionInfo = ethereumBuilder.runSelfTest()
        bitcoinConstructionInfo = bitcoinBuilder.runSelfTest()
        solanaConstructionInfo = solanaBuilder.runSelfTest()
        signingBoundaryInfo = developmentSigner.runSelfTest()
        probe.run()
    }

    Component.onCompleted: refreshStorageInfo()

    AppTools {
        id: tools
    }

    WalletCoreProbe {
        id: probe
        Component.onCompleted: run()
    }

    UnsignedTransactionService {
        id: unsignedIntent
    }

    EthereumUnsignedTransactionService {
        id: ethereumBuilder
    }

    BitcoinUnsignedTransactionService {
        id: bitcoinBuilder
    }

    SolanaUnsignedTransactionService {
        id: solanaBuilder
    }

    DevelopmentSigningService {
        id: developmentSigner
    }

    SilicaFlickable {
        anchors.fill: parent
        contentHeight: content.height

        PullDownMenu {
            MenuItem {
                text: "Run all diagnostics again"
                onClicked: page.runAllDiagnostics()
            }
        }

        VerticalScrollDecorator { }

        Column {
            id: content
            width: parent.width
            spacing: Theme.paddingLarge

            PageHeader {
                title: "Developer diagnostics"
                description: probe.buildInfo
            }

            SectionHeader {
                text: "Configuration storage"
            }

            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * Theme.horizontalPageMargin
                text: storageInfo.storageReleaseGatePassed
                      ? "✓ Storage release checks passed"
                      : "✗ Storage release checks failed"
                color: storageInfo.storageReleaseGatePassed
                       ? Theme.highlightColor
                       : Theme.primaryColor
                font.pixelSize: Theme.fontSizeMedium
                wrapMode: Text.Wrap
            }

            DetailItem {
                label: "INI round-trip"
                value: storageInfo.persistenceProbePassed ? "Passed" : "Failed"
            }

            DetailItem {
                label: "Lifecycle self-test"
                value: storageInfo.storageLifecycleProbePassed ? "Passed" : "Failed"
            }

            DetailItem {
                label: "Profile metadata"
                value: storageInfo.metadataCoherent ? "Coherent" : "Needs attention"
            }

            DetailItem {
                label: "Schema"
                value: storageInfo.schemaVersion !== undefined
                       ? "v" + storageInfo.schemaVersion
                       : "—"
            }

            DetailItem {
                label: "Settings file"
                value: storageInfo.fileExists ? "Present" : "Missing"
            }

            DetailItem {
                label: "Writable"
                value: storageInfo.fileWritable ? "Yes" : "No"
            }

            DetailItem {
                label: "Stored keys"
                value: storageInfo.keyCount !== undefined
                       ? storageInfo.keyCount.toString()
                       : "—"
            }

            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * Theme.horizontalPageMargin
                text: storageInfo.persistenceProbeDetail || "Storage probe not available"
                color: Theme.secondaryColor
                font.pixelSize: Theme.fontSizeSmall
                wrapMode: Text.Wrap
            }

            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * Theme.horizontalPageMargin
                text: storageInfo.path || ""
                color: Theme.secondaryColor
                font.pixelSize: Theme.fontSizeExtraSmall
                wrapMode: Text.WrapAnywhere
            }

            DetailItem {
                label: "Launch count"
                value: storageInfo.launchCount !== undefined
                       ? String(storageInfo.launchCount)
                       : "—"
            }

            DetailItem {
                label: "First initialized"
                value: storageInfo.firstStartedAt || "—"
            }

            DetailItem {
                label: "Last app version"
                value: storageInfo.lastAppVersion || "—"
            }

            DetailItem {
                label: "Install identity"
                value: storageInfo.installIdentityPresent ? "Present" : "Missing"
            }

            DetailItem {
                label: "Profile origin"
                value: storageInfo.profileOrigin === "fresh"
                       ? "Fresh profile"
                       : (storageInfo.profileOrigin === "legacy-import"
                          ? "Legacy import"
                          : (storageInfo.profileOrigin === "existing"
                             ? "Existing profile" : "—"))
            }

            DetailItem {
                label: "Tracked from"
                value: storageInfo.firstRecordedAppVersion || "—"
            }

            DetailItem {
                label: "Last upgrade from"
                value: storageInfo.lastUpgradeFromVersion
                       ? storageInfo.lastUpgradeFromVersion
                         + (storageInfo.lastUpgradeFromMilestone
                            ? " · M" + storageInfo.lastUpgradeFromMilestone : "")
                       : "None recorded"
            }

            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * Theme.horizontalPageMargin
                text: storageInfo.storageLifecycleProbeDetail || "Storage lifecycle probe not available"
                color: Theme.secondaryColor
                font.pixelSize: Theme.fontSizeSmall
                wrapMode: Text.Wrap
            }

            SectionHeader {
                text: "Network boundary"
            }

            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * Theme.horizontalPageMargin
                text: networkInfo.networkBoundaryPassed
                      ? "✓ Local network-boundary checks passed"
                      : "✗ Local network-boundary checks failed"
                color: networkInfo.networkBoundaryPassed
                       ? Theme.highlightColor
                       : Theme.primaryColor
                font.pixelSize: Theme.fontSizeMedium
                wrapMode: Text.Wrap
            }

            DetailItem {
                label: "Endpoint policy"
                value: networkInfo.detail || "—"
            }

            DetailItem {
                label: "Policy self-test"
                value: networkInfo.policySelfTestPassed ? "Passed" : "Failed"
            }

            DetailItem {
                label: "Provider redirects"
                value: networkInfo.redirectsBlocked ? "Blocked" : "Allowed"
            }

            DetailItem {
                label: "Cookies"
                value: networkInfo.cookieIsolationPassed ? "Not sent / stored" : "Needs attention"
            }

            DetailItem {
                label: "HTTP auth reuse"
                value: networkInfo.authIsolationPassed ? "Disabled" : "Needs attention"
            }

            DetailItem {
                label: "HTTP cache"
                value: networkInfo.cacheIsolationPassed ? "Bypassed / not stored" : "Needs attention"
            }

            DetailItem {
                label: "Referer / Origin"
                value: networkInfo.contextHeaderIsolationPassed ? "Cleared" : "Needs attention"
            }

            DetailItem {
                label: "Response media types"
                value: networkInfo.jsonMediaTypePolicyPassed ? "Validated" : "Needs attention"
            }

            DetailItem {
                label: "JSON-RPC envelopes"
                value: networkInfo.jsonRpcEnvelopePolicyPassed ? "Validated" : "Needs attention"
            }

            DetailItem {
                label: "Response ceiling"
                value: networkInfo.responseLimitText || "—"
            }

            DetailItem {
                label: "Offline mode"
                value: portfolio && portfolio.offlineMode ? "Enabled" : "Disabled"
            }

            DetailItem {
                label: "Ordinary provider timeout"
                value: "15 seconds"
            }

            DetailItem {
                label: "Health / fee timeout"
                value: "12 seconds"
            }

            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * Theme.horizontalPageMargin
                text: "Provider requests are HTTPS-only, redirect-free and stateless: cookies, "
                      + "cached HTTP authentication, local HTTP caching and referrer/origin context "
                      + "are disabled. JSON/text media types are checked before parsing, JSON-RPC "
                      + "version/request IDs/result-error envelopes are validated, responses remain "
                      + "bounded, and stale replies cannot replace newer refresh state."
                color: Theme.secondaryColor
                font.pixelSize: Theme.fontSizeSmall
                wrapMode: Text.Wrap
            }

            SectionHeader {
                text: "Unsigned transaction intent"
            }

            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * Theme.horizontalPageMargin
                text: transactionInfo.passed
                      ? "✓ Local transaction-intent checks passed"
                      : "✗ Transaction-intent checks failed"
                color: transactionInfo.passed ? Theme.highlightColor : Theme.primaryColor
                font.pixelSize: Theme.fontSizeMedium
                wrapMode: Text.Wrap
            }

            DetailItem {
                label: "Chain metadata"
                value: transactionInfo.chainMetadataPassed ? "Passed" : "Failed"
            }

            DetailItem {
                label: "Address validation"
                value: transactionInfo.addressValidationPassed ? "Wallet Core validated" : "Failed"
            }

            DetailItem {
                label: "Decimal amounts"
                value: transactionInfo.decimalValidationPassed ? "Exact parser passed" : "Failed"
            }

            DetailItem {
                label: "Intent fingerprint"
                value: transactionInfo.fingerprintValidationPassed ? "SHA-256 binding passed" : "Failed"
            }

            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * Theme.horizontalPageMargin
                text: transactionInfo.detail || "Transaction-intent self-test not available"
                color: Theme.secondaryColor
                font.pixelSize: Theme.fontSizeSmall
                wrapMode: Text.Wrap
            }

            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * Theme.horizontalPageMargin
                text: "Intent diagnostics are local-only: they do not read Sailfish Secrets, contact a provider or sign/broadcast anything."
                color: Theme.secondaryColor
                font.pixelSize: Theme.fontSizeSmall
                wrapMode: Text.Wrap
            }

            SectionHeader {
                text: "Ethereum unsigned construction"
            }

            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * Theme.horizontalPageMargin
                text: ethereumConstructionInfo.passed
                      ? "✓ Local EIP-1559 construction checks passed"
                      : "✗ Ethereum construction checks failed"
                color: ethereumConstructionInfo.passed ? Theme.highlightColor : Theme.primaryColor
                font.pixelSize: Theme.fontSizeMedium
                wrapMode: Text.Wrap
            }

            DetailItem {
                label: "RPC quantities"
                value: ethereumConstructionInfo.quantityValidationPassed ? "Strict parser passed" : "Failed"
            }

            DetailItem {
                label: "Exact ETH → wei"
                value: ethereumConstructionInfo.amountToWeiPassed ? "Passed" : "Failed"
            }

            DetailItem {
                label: "Reviewed intent gate"
                value: ethereumConstructionInfo.intentBindingPassed ? "Mutation rejected" : "Failed"
            }

            DetailItem {
                label: "EIP-1559 RLP vector"
                value: ethereumConstructionInfo.eip1559RlpVectorPassed ? "Passed" : "Failed"
            }

            DetailItem {
                label: "Signing hash"
                value: ethereumConstructionInfo.keccakVectorPassed ? "Wallet Core Keccak-256 passed" : "Failed"
            }

            DetailItem {
                label: "Construction binding"
                value: ethereumConstructionInfo.constructionFingerprintPassed ? "SHA-256 mutation check passed" : "Failed"
            }

            DetailItem {
                label: "Signing snapshot gate"
                value: ethereumConstructionInfo.signingSnapshotBindingPassed
                       ? "C++ reconstruction / mutation rejection passed" : "Failed"
            }

            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * Theme.horizontalPageMargin
                text: ethereumConstructionInfo.detail || "Ethereum construction self-test not available"
                color: Theme.secondaryColor
                font.pixelSize: Theme.fontSizeSmall
                wrapMode: Text.Wrap
            }

            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * Theme.horizontalPageMargin
                text: "The M52 Ethereum construction self-test is network-free and secret-free. It exercises deterministic RLP/Keccak/binding code only; live nonce/fee/code queries occur only after the user explicitly requests construction from an Ethereum review page."
                color: Theme.secondaryColor
                font.pixelSize: Theme.fontSizeSmall
                wrapMode: Text.Wrap
            }

            SectionHeader {
                text: "Bitcoin unsigned construction"
            }

            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * Theme.horizontalPageMargin
                text: bitcoinConstructionInfo.passed
                      ? "✓ Local Bitcoin construction checks passed"
                      : "✗ Bitcoin construction checks failed"
                color: bitcoinConstructionInfo.passed ? Theme.highlightColor : Theme.primaryColor
                font.pixelSize: Theme.fontSizeMedium
                wrapMode: Text.Wrap
            }

            DetailItem {
                label: "Exact BTC → sats"
                value: bitcoinConstructionInfo.exactAmountPassed ? "Passed" : "Failed"
            }

            DetailItem {
                label: "Wallet Core scripts"
                value: bitcoinConstructionInfo.walletCoreScriptPassed ? "BIP84 P2WPKH passed" : "Failed"
            }

            DetailItem {
                label: "Coin selection / fee"
                value: bitcoinConstructionInfo.coinSelectionPassed ? "Deterministic vector passed" : "Failed"
            }

            DetailItem {
                label: "Unsigned transaction"
                value: bitcoinConstructionInfo.serializationVectorPassed ? "Serialization / SHA256d passed" : "Failed"
            }

            DetailItem {
                label: "PSBT v0"
                value: bitcoinConstructionInfo.psbtPassed ? "Witness UTXO / SIGHASH_ALL passed" : "Failed"
            }

            DetailItem {
                label: "Construction binding"
                value: bitcoinConstructionInfo.constructionFingerprintPassed ? "SHA-256 mutation check passed" : "Failed"
            }

            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * Theme.horizontalPageMargin
                text: bitcoinConstructionInfo.detail || "Bitcoin construction self-test not available"
                color: Theme.secondaryColor
                font.pixelSize: Theme.fontSizeSmall
                wrapMode: Text.Wrap
            }

            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * Theme.horizontalPageMargin
                text: "The M52 Bitcoin self-test is network-free and secret-free. It uses synthetic public UTXOs and published BIP84 addresses. Live construction queries only confirmed UTXOs and fee estimates after an explicit request; the public development address is expected to have no spendable UTXOs."
                color: Theme.secondaryColor
                font.pixelSize: Theme.fontSizeSmall
                wrapMode: Text.Wrap
            }

            SectionHeader {
                text: "Solana unsigned construction"
            }

            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * Theme.horizontalPageMargin
                text: solanaConstructionInfo.passed
                      ? "✓ Local Solana construction checks passed"
                      : "✗ Solana construction checks failed"
                color: solanaConstructionInfo.passed ? Theme.highlightColor : Theme.primaryColor
                font.pixelSize: Theme.fontSizeMedium
                wrapMode: Text.Wrap
            }

            DetailItem {
                label: "Exact SOL → lamports"
                value: solanaConstructionInfo.exactAmountPassed ? "Passed" : "Failed"
            }

            DetailItem {
                label: "Base58 / key width"
                value: solanaConstructionInfo.walletCoreBase58Passed
                       ? "Wallet Core 32-byte validation passed" : "Failed"
            }

            DetailItem {
                label: "Wallet Core pre-signing vector"
                value: solanaConstructionInfo.walletCorePreimageVectorPassed
                       ? "Upstream transfer vector matched" : "Failed"
            }

            DetailItem {
                label: "Zero-signature template"
                value: solanaConstructionInfo.transactionTemplatePassed
                       ? "One signature slot / message binding passed" : "Failed"
            }

            DetailItem {
                label: "Reviewed intent gate"
                value: solanaConstructionInfo.intentBindingPassed
                       ? "Mutation rejected" : "Failed"
            }

            DetailItem {
                label: "Construction binding"
                value: solanaConstructionInfo.constructionFingerprintPassed
                       ? "SHA-256 mutation check passed" : "Failed"
            }

            DetailItem {
                label: "Fee formatting"
                value: solanaConstructionInfo.feeFormattingPassed
                       ? "Exact lamports → SOL passed" : "Failed"
            }

            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * Theme.horizontalPageMargin
                text: solanaConstructionInfo.detail || "Solana construction self-test not available"
                color: Theme.secondaryColor
                font.pixelSize: Theme.fontSizeSmall
                wrapMode: Text.Wrap
            }

            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * Theme.horizontalPageMargin
                text: "The M52 Solana self-test is network-free and secret-free. It uses Wallet Core's external-signing compiler and its upstream SOL-transfer vector. Live construction contacts the configured RPC only after explicit review-page action."
                color: Theme.secondaryColor
                font.pixelSize: Theme.fontSizeSmall
                wrapMode: Text.Wrap
            }

            SectionHeader {
                text: "M52 development signing boundary"
            }

            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * Theme.horizontalPageMargin
                text: signingBoundaryInfo.passed
                      ? "✓ Local non-exporting signing checks passed"
                      : "✗ Development signing-boundary checks failed"
                color: signingBoundaryInfo.passed
                       ? Theme.highlightColor : Theme.primaryColor
                font.pixelSize: Theme.fontSizeMedium
                wrapMode: Text.Wrap
            }

            DetailItem {
                label: "Published wallet identity"
                value: signingBoundaryInfo.publishedIdentityPassed
                       ? "ETH / BTC / SOL test vectors matched" : "Failed"
            }

            DetailItem {
                label: "Wallet Core sign / verify"
                value: signingBoundaryInfo.signVerifyPassed
                       ? "65-byte secp256k1 signature verified" : "Failed"
            }

            DetailItem {
                label: "Mutated digest"
                value: signingBoundaryInfo.mutatedDigestRejected
                       ? "Rejected" : "Failed"
            }

            DetailItem {
                label: "Signature proof binding"
                value: signingBoundaryInfo.signatureProofBindingPassed
                       ? "SHA-256 mutation check passed" : "Failed"
            }

            DetailItem {
                label: "Raw signature to QML"
                value: signingBoundaryInfo.rawSignatureExported === false
                       ? "Not exported" : "Needs attention"
            }

            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * Theme.horizontalPageMargin
                text: signingBoundaryInfo.detail || "Development signing self-test not available"
                color: Theme.secondaryColor
                font.pixelSize: Theme.fontSizeSmall
                wrapMode: Text.Wrap
            }

            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * Theme.horizontalPageMargin
                text: "This automatic diagnostic is network-free and Secrets-free: it uses only the published BIP39 test mnemonic. "
                      + "The live M52 path is stricter: WalletVault must already have an unlocked public-test session, reopens Sailfish Secrets inside C++, revalidates all three addresses, signs the fresh M49 payload hash, verifies it, hashes the signature into a proof fingerprint and wipes the raw signature before QML can see it."
                color: Theme.secondaryColor
                font.pixelSize: Theme.fontSizeSmall
                wrapMode: Text.Wrap
            }

            SectionHeader {
                text: "Wallet Core self-test"
            }

            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * Theme.horizontalPageMargin
                text: probe.summary.length ? probe.summary : "Running self-test…"
                color: probe.allPassed ? Theme.highlightColor : Theme.primaryColor
                font.pixelSize: Theme.fontSizeMedium
                wrapMode: Text.Wrap
            }

            Repeater {
                model: probe.results

                delegate: Column {
                    x: Theme.horizontalPageMargin
                    width: content.width - 2 * Theme.horizontalPageMargin
                    spacing: Theme.paddingSmall

                    Label {
                        width: parent.width
                        text: (modelData.passed ? "✓ PASS · " : "✗ FAIL · ") + modelData.name
                        color: modelData.passed ? Theme.highlightColor : Theme.primaryColor
                        font.pixelSize: Theme.fontSizeMedium
                        wrapMode: Text.Wrap
                    }

                    Label {
                        width: parent.width
                        text: modelData.detail
                        color: Theme.secondaryColor
                        font.pixelSize: Theme.fontSizeSmall
                        wrapMode: Text.WrapAnywhere
                    }

                    Label {
                        width: parent.width
                        visible: modelData.expected && modelData.expected.length > 0
                        text: "Expected: " + modelData.expected
                        color: Theme.secondaryHighlightColor
                        font.pixelSize: Theme.fontSizeExtraSmall
                        wrapMode: Text.WrapAnywhere
                    }

                    Item {
                        width: 1
                        height: Theme.paddingMedium
                    }
                }
            }

            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * Theme.horizontalPageMargin
                text: "Diagnostic runs: " + probe.runCount
                color: Theme.secondaryColor
                font.pixelSize: Theme.fontSizeExtraSmall
                horizontalAlignment: Text.AlignHCenter
            }
        }
    }
}
