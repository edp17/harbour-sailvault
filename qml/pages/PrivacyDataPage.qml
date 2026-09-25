import QtQuick 2.0
import Sailfish.Silica 1.0
import org.sailfishos.sailvault 1.0

Page {
    id: page

    property var portfolio
    property var info: ({})
    property string operationStatus: ""
    property bool operationPassed: true

    function refreshInfo() {
        info = tools.privacyDiagnostics()
    }

    Component.onCompleted: refreshInfo()

    AppTools {
        id: tools
    }

    RemorsePopup {
        id: remorse
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
                title: "Privacy & local data"
                description: "Read-only beta security boundary"
            }

            SectionHeader {
                text: "Network disclosure"
            }

            DetailItem {
                label: "Offline mode"
                value: portfolio && portfolio.offlineMode ? "Enabled" : "Disabled"
            }

            DetailItem {
                label: "Balance / activity / tokens"
                value: "Public address + network metadata"
            }

            DetailItem {
                label: "Market prices"
                value: "No wallet address"
            }

            DetailItem {
                label: "Provider health / fees"
                value: "Address-free"
            }

            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * Theme.horizontalPageMargin
                text: "When a wallet provider is queried, that provider can observe the "
                      + "public address and ordinary network metadata such as the connecting "
                      + "IP address. SailVault sends no recovery phrase or private key to "
                      + "network services. Offline mode blocks SailVault provider requests."
                color: Theme.secondaryColor
                font.pixelSize: Theme.fontSizeSmall
                wrapMode: Text.Wrap
            }

            SectionHeader {
                text: "Local storage"
            }

            DetailItem {
                label: "Development recovery material"
                value: "Sailfish Secrets only"
            }

            DetailItem {
                label: "Watch-only profile"
                value: info.watchOnlyStored ? "Stored in INI" : "Not configured"
            }

            DetailItem {
                label: "Address book"
                value: info.addressBookStored ? "Stored in INI" : "Empty"
            }

            DetailItem {
                label: "Cached public-data values"
                value: info.cachedPublicKeys !== undefined
                       ? String(info.cachedPublicKeys)
                       : "—"
            }

            DetailItem {
                label: "Portfolio-history contexts"
                value: info.historyContexts !== undefined
                       ? String(info.historyContexts)
                       : "—"
            }

            DetailItem {
                label: "Quarantined corrupt records"
                value: info.quarantinedRecords !== undefined
                       ? String(info.quarantinedRecords)
                       : "—"
            }

            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * Theme.horizontalPageMargin
                text: "The INI backend contains public addresses, preferences and disposable "
                      + "read-only caches. Recovery material is not stored in this file. "
                      + "Quarantine records are local copies of malformed non-sensitive INI "
                      + "values retained only for diagnostics."
                color: Theme.secondaryColor
                font.pixelSize: Theme.fontSizeSmall
                wrapMode: Text.Wrap
            }

            Button {
                anchors.horizontalCenter: parent.horizontalCenter
                text: "Clear cached public data"
                enabled: info.cachedPublicKeys > 0
                onClicked: remorse.execute(
                    "Clearing cached balances, prices and history",
                    function() {
                        var result = tools.clearCachedPublicData()
                        info = result
                        operationPassed = result.operationPassed
                        operationStatus = result.operationDetail
                    })
            }

            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * Theme.horizontalPageMargin
                text: "This removes persisted balance snapshots, market-price caches and "
                      + "portfolio history. Watch-only addresses, Address book entries, hidden-token "
                      + "preferences, endpoints and Sailfish Secrets are preserved. Values already "
                      + "loaded in memory may remain visible until their page is reopened."
                color: Theme.secondaryColor
                font.pixelSize: Theme.fontSizeSmall
                wrapMode: Text.Wrap
            }

            Button {
                anchors.horizontalCenter: parent.horizontalCenter
                visible: info.quarantinedRecords > 0
                text: "Clear quarantined INI data"
                onClicked: remorse.execute(
                    "Clearing quarantined non-sensitive data",
                    function() {
                        var result = tools.clearQuarantinedSettingsData()
                        info = result
                        operationPassed = result.operationPassed
                        operationStatus = result.operationDetail
                    })
            }

            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * Theme.horizontalPageMargin
                visible: operationStatus.length > 0
                text: operationStatus
                color: operationPassed ? Theme.highlightColor : Theme.primaryColor
                font.pixelSize: Theme.fontSizeSmall
                horizontalAlignment: Text.AlignHCenter
                wrapMode: Text.Wrap
            }

            SectionHeader {
                text: "Read-only beta gate"
            }

            DetailItem {
                label: "Transaction construction"
                value: "Unavailable"
            }

            DetailItem {
                label: "Signing / broadcasting"
                value: "Unavailable"
            }

            DetailItem {
                label: "Wallet Core"
                value: "4.0.27 compatibility baseline"
            }

            DetailItem {
                label: "Sailjail permissions"
                value: "Secrets + Internet"
            }

            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * Theme.horizontalPageMargin
                text: "Wallet Core 4.0.27 is retained only as the proven Sailfish build "
                      + "baseline. Do not use a personal recovery phrase or real funds. "
                      + "Real-funds transaction functionality remains blocked until the "
                      + "Wallet Core security baseline has been upgraded or reviewed."
                color: Theme.highlightColor
                font.pixelSize: Theme.fontSizeSmall
                wrapMode: Text.Wrap
            }
        }
    }
}
