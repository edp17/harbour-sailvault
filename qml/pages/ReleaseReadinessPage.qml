import QtQuick 2.0
import Sailfish.Silica 1.0
import org.sailfishos.sailvault 1.0

Page {
    id: page

    property var portfolio
    property var vault
    property var storageInfo: ({})
    property var networkInfo: ({})
    property var transactionInfo: ({})
    property var ethereumConstructionInfo: ({})
    property var bitcoinConstructionInfo: ({})
    property var solanaConstructionInfo: ({})
    property var signingBoundaryInfo: ({})

    function refreshStorageInfo() {
        storageInfo = tools.settingsDiagnostics()
        networkInfo = tools.networkSecurityDiagnostics()
        transactionInfo = unsignedIntent.runSelfTest()
        ethereumConstructionInfo = ethereumBuilder.runSelfTest()
        bitcoinConstructionInfo = bitcoinBuilder.runSelfTest()
        solanaConstructionInfo = solanaBuilder.runSelfTest()
        signingBoundaryInfo = developmentSigner.runSelfTest()
    }

    function checkMark(passed) {
        return passed ? "✓" : "✗"
    }

    function checkColor(passed) {
        return passed ? Theme.highlightColor : Theme.primaryColor
    }

    function secretsBackendCheckPassed() {
        return vault && vault.backendReady
    }

    function automaticChecksPassed() {
        return storageInfo.storageReleaseGatePassed
                && networkInfo.networkBoundaryPassed
                && transactionInfo.passed
                && ethereumConstructionInfo.passed
                && bitcoinConstructionInfo.passed
                && solanaConstructionInfo.passed
                && signingBoundaryInfo.passed
                && probe.allPassed
                && secretsBackendCheckPassed()
    }

    function attentionDetail() {
        var failed = []
        if (!storageInfo.storageReleaseGatePassed)
            failed.push("storage lifecycle")
        if (!networkInfo.networkBoundaryPassed)
            failed.push("network boundary")
        if (!transactionInfo.passed)
            failed.push("unsigned transaction intent")
        if (!ethereumConstructionInfo.passed)
            failed.push("Ethereum unsigned construction")
        if (!bitcoinConstructionInfo.passed)
            failed.push("Bitcoin unsigned construction")
        if (!solanaConstructionInfo.passed)
            failed.push("Solana unsigned construction")
        if (!signingBoundaryInfo.passed)
            failed.push("development signing boundary")
        if (!probe.allPassed)
            failed.push("Wallet Core self-test")
        if (!secretsBackendCheckPassed())
            failed.push("Sailfish Secrets backend")
        return failed.length > 0 ? "Needs attention: " + failed.join(", ") : ""
    }

    Component.onCompleted: {
        refreshStorageInfo()
        probe.run()
    }

    AppTools {
        id: tools
    }

    WalletCoreProbe {
        id: probe
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
        contentHeight: content.height + Theme.paddingLarge

        PullDownMenu {
            MenuItem {
                text: "Run automatic checks again"
                onClicked: {
                    storageInfo = tools.rerunSettingsPersistenceProbe()
                    networkInfo = tools.networkSecurityDiagnostics()
                    transactionInfo = unsignedIntent.runSelfTest()
                    ethereumConstructionInfo = ethereumBuilder.runSelfTest()
                    bitcoinConstructionInfo = bitcoinBuilder.runSelfTest()
                    solanaConstructionInfo = solanaBuilder.runSelfTest()
                    signingBoundaryInfo = developmentSigner.runSelfTest()
                    probe.run()
                }
            }
        }

        VerticalScrollDecorator { }

        Column {
            id: content
            width: parent.width
            spacing: Theme.paddingLarge

            PageHeader {
                title: "Release readiness"
                description: tools.buildLabel + " · Milestone " + tools.milestone
            }

            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * Theme.horizontalPageMargin
                text: automaticChecksPassed()
                      ? "✓ Automatic device checks passed"
                      : "Automatic device checks need attention"
                color: automaticChecksPassed()
                       ? Theme.highlightColor
                       : Theme.primaryColor
                font.pixelSize: Theme.fontSizeMedium
                horizontalAlignment: Text.AlignHCenter
                wrapMode: Text.Wrap
            }

            SectionHeader {
                text: "Build identity"
            }

            DetailItem {
                label: "Application"
                value: "SailVault " + tools.packageVersion
            }

            DetailItem {
                label: "Milestone"
                value: tools.milestone + " · " + tools.buildLabel
            }

            DetailItem {
                label: "Wallet Core"
                value: tools.walletCoreVersion + " · Sailfish baseline"
            }

            SectionHeader {
                text: "Automatic device checks"
            }

            DetailItem {
                label: "Storage release gate"
                value: checkMark(storageInfo.storageReleaseGatePassed)
                       + " "
                       + (storageInfo.storageReleaseGatePassed
                          ? "Fresh / upgrade / persistence validated"
                          : "Needs attention")
            }

            DetailItem {
                label: "Persistent INI round-trip"
                value: checkMark(storageInfo.persistenceProbePassed)
                       + " "
                       + (storageInfo.persistenceProbePassed ? "Passed" : "Failed")
            }

            DetailItem {
                label: "Storage lifecycle self-test"
                value: checkMark(storageInfo.storageLifecycleProbePassed)
                       + " "
                       + (storageInfo.storageLifecycleProbePassed
                          ? "Passed" : "Failed")
            }

            DetailItem {
                label: "Provider request policy"
                value: checkMark(networkInfo.networkBoundaryPassed)
                       + " "
                       + (networkInfo.networkBoundaryPassed
                          ? "HTTPS / stateless / bounded" : "Needs attention")
            }

            DetailItem {
                label: "Provider response validation"
                value: checkMark(networkInfo.providerResponsePolicyPassed)
                       + " "
                       + (networkInfo.providerResponsePolicyPassed
                          ? "Media type / JSON-RPC validated" : "Needs attention")
            }

            DetailItem {
                label: "Unsigned intent model"
                value: checkMark(transactionInfo.passed)
                       + " "
                       + (transactionInfo.passed
                          ? "Address / amount / fingerprint validated"
                          : "Needs attention")
            }

            DetailItem {
                label: "Ethereum construction model"
                value: checkMark(ethereumConstructionInfo.passed)
                       + " "
                       + (ethereumConstructionInfo.passed
                          ? "EIP-1559 / signing snapshot binding validated"
                          : "Needs attention")
            }

            DetailItem {
                label: "Bitcoin construction model"
                value: checkMark(bitcoinConstructionInfo.passed)
                       + " "
                       + (bitcoinConstructionInfo.passed
                          ? "UTXO / fee / serialization / PSBT validated"
                          : "Needs attention")
            }

            DetailItem {
                label: "Solana construction model"
                value: checkMark(solanaConstructionInfo.passed)
                       + " "
                       + (solanaConstructionInfo.passed
                          ? "Wallet Core pre-signing / blockhash / fee binding validated"
                          : "Needs attention")
            }

            DetailItem {
                label: "Development signing boundary"
                value: checkMark(signingBoundaryInfo.passed)
                       + " "
                       + (signingBoundaryInfo.passed
                          ? "Published test wallet / non-exporting sign-verify validated"
                          : "Needs attention")
            }

            DetailItem {
                label: "Wallet Core self-test"
                value: checkMark(probe.allPassed)
                       + " "
                       + (probe.allPassed ? "Passed" : "Failed")
            }

            DetailItem {
                label: "Sailfish Secrets backend"
                value: checkMark(vault && vault.backendReady)
                       + " "
                       + (vault && vault.backendReady ? "Ready" : "Not ready")
            }

            DetailItem {
                label: "Secure wallet protection"
                value: checkMark(secretsBackendCheckPassed())
                       + " "
                       + (!vault || !vault.backendReady
                          ? "Backend not ready"
                          : (vault.storageProtected
                             ? "Device-lock protected"
                             : (vault.storageKnown
                                ? (vault.walletStored ? "Stored · status known" : "No wallet stored")
                                : "Not interrogated by readiness check")))
            }

            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * Theme.horizontalPageMargin
                text: automaticChecksPassed() ? "" : attentionDetail()
                visible: text.length > 0
                color: Theme.highlightColor
                font.pixelSize: Theme.fontSizeSmall
                wrapMode: Text.Wrap
            }

            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * Theme.horizontalPageMargin
                text: "Release readiness verifies that the Sailfish Secrets backend is available, "
                      + "but deliberately does not probe, unlock or authenticate the DeviceLockRelock "
                      + "wallet collection. Its current protection state is shown only when already known."
                color: Theme.secondaryColor
                font.pixelSize: Theme.fontSizeSmall
                wrapMode: Text.Wrap
            }

            DetailItem {
                label: "Profile lineage"
                value: (storageInfo.profileOrigin === "fresh"
                        ? "Fresh profile"
                        : (storageInfo.profileOrigin === "legacy-import"
                           ? "Legacy import"
                           : (storageInfo.profileOrigin === "existing"
                              ? "Existing profile" : "Unknown")))
                       + (storageInfo.firstRecordedAppVersion
                          ? " · tracked from " + storageInfo.firstRecordedAppVersion : "")
            }

            DetailItem {
                label: "Last upgrade"
                value: storageInfo.lastUpgradeFromVersion
                       ? storageInfo.lastUpgradeFromVersion + " → " + tools.applicationVersion
                       : "None recorded"
            }

            DetailItem {
                label: "Install identity"
                value: storageInfo.installIdentityPresent ? "Present" : "Missing"
            }

            DetailItem {
                label: "Startup records"
                value: storageInfo.launchCount !== undefined
                       ? String(storageInfo.launchCount) + " launches"
                       : "—"
            }

            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * Theme.horizontalPageMargin
                text: "First initialized: " + (storageInfo.firstStartedAt || "—")
                      + "\nLast startup: " + (storageInfo.lastStartedAt || "—")
                color: Theme.secondaryColor
                font.pixelSize: Theme.fontSizeExtraSmall
                wrapMode: Text.Wrap
            }

            SectionHeader {
                text: "Transaction safety gates"
            }

            DetailItem {
                label: "Transaction intent / review"
                value: "Available · unsigned / local-only"
            }

            DetailItem {
                label: "Ethereum construction"
                value: ethereumConstructionInfo.passed
                       ? "Unsigned EIP-1559 · local self-test passed"
                       : "Needs attention"
            }

            DetailItem {
                label: "Bitcoin construction"
                value: bitcoinConstructionInfo.passed
                       ? "Unsigned transaction + PSBT v0 · local self-test passed"
                       : "Needs attention"
            }

            DetailItem {
                label: "Solana construction"
                value: solanaConstructionInfo.passed
                       ? "Unsigned Wallet Core signing message · local self-test passed"
                       : "Needs attention"
            }

            DetailItem {
                label: "Production signing"
                value: "Unavailable"
            }

            DetailItem {
                label: "M52 development sign / verify"
                value: signingBoundaryInfo.passed
                       ? "Ethereum test-wallet proof available · raw signature discarded"
                       : "Needs attention"
            }

            DetailItem {
                label: "Broadcasting"
                value: "Unavailable"
            }

            DetailItem {
                label: "Offline mode"
                value: portfolio && portfolio.offlineMode ? "Enabled" : "Available"
            }

            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * Theme.horizontalPageMargin
                text: "M52 preserves the proven M49/M50/M51r2 unsigned constructors and introduces the first deliberately narrow Secrets → Wallet Core signing boundary for Ethereum. "
                      + "Only the published public test wallet is accepted, the unsigned construction must be fresh and C++-revalidated, and the generated signature is verified then discarded before QML can see it. "
                      + "No signed transaction is assembled; production signing and broadcasting remain unavailable."
                color: Theme.secondaryColor
                font.pixelSize: Theme.fontSizeSmall
                wrapMode: Text.Wrap
            }

            SectionHeader {
                text: "Real-funds gate"
            }

            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * Theme.horizontalPageMargin
                text: "Real-funds use remains blocked. Wallet Core " + tools.walletCoreVersion
                      + (probe.allPassed
                         ? " passes SailVault's current local compatibility/security diagnostics on this run. "
                         : " is still awaiting the local compatibility/security diagnostics on this run. ")
                      + "M41 expanded local cryptographic and ABI diagnostics; M42 hardened the provider "
                      + "network boundary; M43 isolates provider request state for additional privacy; "
                      + "M44 validates provider media types and JSON-RPC envelopes before data is trusted; "
                      + "M45 moves the compatibility baseline to the pinned Wallet Core 4.8.4 release; "
                      + "M46 completes the systematic Sailfish UI/typography consistency pass; "
                      + "M47 gates storage on real persistence plus isolated fresh/upgrade/import lifecycle tests; "
                      + "M48 introduces reviewed unsigned transaction intent; M49 adds reviewed Ethereum EIP-1559 construction; M50 adds deterministic Bitcoin UTXO/fee/change planning, unsigned transaction serialization and PSBT v0 construction; "
                      + "M51 completes unsigned native-chain construction with a Wallet Core-generated Solana transfer signing message, mainnet/blockhash checks and fee quoting; "
                      + "M52 adds a development-only Ethereum sign/verify proof that is hard-bound to the published test wallet, a fresh reviewed M49 construction and non-exporting signature handling. "
                      + "Dedicated secure Bitcoin change derivation, production signing policy/user confirmation, signed-transaction assembly and broadcasting remain separate later milestones."
                color: Theme.highlightColor
                font.pixelSize: Theme.fontSizeSmall
                wrapMode: Text.Wrap
            }

            Item {
                width: 1
                height: Theme.paddingLarge
            }
        }
    }
}
