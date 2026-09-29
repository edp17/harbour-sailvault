import QtQuick 2.0
import Sailfish.Silica 1.0
import org.sailfishos.sailvault 1.0

Page {
    id: page

    property var portfolio
    property var vault
    property var storageInfo: ({})
    property var networkInfo: ({})

    function refreshStorageInfo() {
        storageInfo = tools.settingsDiagnostics()
        networkInfo = tools.networkSecurityDiagnostics()
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
        return storageInfo.persistenceProbePassed
                && networkInfo.networkBoundaryPassed
                && probe.allPassed
                && secretsBackendCheckPassed()
    }

    function attentionDetail() {
        var failed = []
        if (!storageInfo.persistenceProbePassed)
            failed.push("INI persistence")
        if (!networkInfo.networkBoundaryPassed)
            failed.push("network boundary")
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

    SilicaFlickable {
        anchors.fill: parent
        contentHeight: content.height + Theme.paddingLarge

        PullDownMenu {
            MenuItem {
                text: "Run automatic checks again"
                onClicked: {
                    storageInfo = tools.rerunSettingsPersistenceProbe()
                    networkInfo = tools.networkSecurityDiagnostics()
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
                label: "Persistent INI round-trip"
                value: checkMark(storageInfo.persistenceProbePassed)
                       + " "
                       + (storageInfo.persistenceProbePassed ? "Passed" : "Failed")
            }

            DetailItem {
                label: "Provider request policy"
                value: checkMark(networkInfo.networkBoundaryPassed)
                       + " "
                       + (networkInfo.networkBoundaryPassed
                          ? "HTTPS / stateless / bounded" : "Needs attention")
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
                text: "Read-only beta guards"
            }

            DetailItem {
                label: "Transaction construction"
                value: "Unavailable"
            }

            DetailItem {
                label: "Signing"
                value: "Unavailable"
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
                text: "Fresh-install launcher persistence, explicit Offline mode/provider "
                      + "timeouts, HTTPS/stateless request policy, redirect/response limits, privacy/local-data "
                      + "cleanup and the relocked Secrets readiness path are covered by the read-only "
                      + "beta and security-hardening milestones."
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
                      + " has passed SailVault's compatibility gate and is the current Sailfish baseline. "
                      + "M41 expanded local cryptographic and ABI diagnostics; M42 hardened the provider "
                      + "network boundary; M43 isolates provider request state for additional privacy. "
                      + "Transaction construction, user signing and broadcasting remain "
                      + "separate later milestones."
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
