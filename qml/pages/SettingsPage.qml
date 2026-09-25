import QtQuick 2.0
import Sailfish.Silica 1.0

Page {
    id: page

    property var portfolio
    property var prices
    property var security
    property var vault

    RemorsePopup {
        id: remorse
    }

    SilicaFlickable {
        anchors.fill: parent
        contentHeight: content.height + Theme.paddingLarge

        PullDownMenu {
            MenuItem {
                text: "Network & portfolio"
                onClicked: pageStack.push(Qt.resolvedUrl("NetworkSettingsPage.qml"), {
                    portfolio: portfolio,
                    prices: prices
                })
            }

            MenuItem {
                text: "Security"
                onClicked: pageStack.push(Qt.resolvedUrl("SecuritySettingsPage.qml"), {
                    security: security
                })
            }

            MenuItem {
                text: "Privacy & local data"
                onClicked: pageStack.push(Qt.resolvedUrl("PrivacyDataPage.qml"), {
                    portfolio: portfolio
                })
            }

            MenuItem {
                text: "Release readiness"
                onClicked: pageStack.push(Qt.resolvedUrl("ReleaseReadinessPage.qml"), {
                    portfolio: portfolio,
                    vault: vault
                })
            }

            MenuItem {
                text: "Developer diagnostics"
                onClicked: pageStack.push(Qt.resolvedUrl("DiagnosticsPage.qml"), {
                    portfolio: portfolio
                })
            }
        }

        VerticalScrollDecorator { }

        Column {
            id: content
            width: parent.width
            spacing: Theme.paddingLarge

            PageHeader {
                title: "Settings"
                description: "Pull down for Network, Security, Privacy, Readiness and Diagnostics"
            }

            SectionHeader {
                text: "Current preferences"
            }

            DetailItem {
                label: "Fiat currency"
                value: prices ? prices.fiatCurrency : "—"
            }

            DetailItem {
                label: "Portfolio refresh"
                value: portfolio && portfolio.autoRefreshAfterUnlock
                       ? "Automatic"
                       : "Manual"
            }

            DetailItem {
                label: "Connectivity"
                value: portfolio && portfolio.offlineMode
                       ? "Offline mode"
                       : "Online"
            }

            DetailItem {
                label: "Background lock"
                value: security && security.enabled
                       ? security.backgroundDelayText
                       : "Disabled"
            }

            DetailItem {
                label: "Inactivity lock"
                value: security && security.enabled
                       ? security.inactivityText
                       : "Disabled"
            }

            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * Theme.horizontalPageMargin
                text: "These rows are status summaries. Use the pulley menu to change "
                      + "network, portfolio, privacy and security preferences."
                color: Theme.secondaryColor
                font.pixelSize: Theme.fontSizeSmall
                wrapMode: Text.Wrap
            }

            SectionHeader {
                visible: vault && vault.walletStored
                text: "Development wallet"
            }

            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * Theme.horizontalPageMargin
                visible: vault && vault.walletStored
                text: "The stored development wallet uses the public BIP39 test vector."
                color: Theme.secondaryColor
                font.pixelSize: Theme.fontSizeSmall
                wrapMode: Text.Wrap
            }

            Button {
                anchors.horizontalCenter: parent.horizontalCenter
                visible: vault && vault.walletStored
                text: "Backup development phrase"
                onClicked: pageStack.push(Qt.resolvedUrl("BackupPage.qml"), {
                    developmentTestMnemonic: vault.developmentTestMnemonic
                })
            }

            SectionHeader {
                text: "Storage"
            }

            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * Theme.horizontalPageMargin
                text: "Network endpoints, fiat currency, watch-only profiles, address-book "
                      + "entries and display preferences are non-sensitive data stored in "
                      + "SailVault's persistent INI configuration."
                color: Theme.secondaryColor
                font.pixelSize: Theme.fontSizeSmall
                wrapMode: Text.Wrap
            }

            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * Theme.horizontalPageMargin
                text: "The development recovery phrase remains in Sailfish Secrets. "
                      + "Last-known public balances and market prices may be cached locally "
                      + "for offline display."
                color: Theme.secondaryColor
                font.pixelSize: Theme.fontSizeSmall
                wrapMode: Text.Wrap
            }

            SectionHeader {
                visible: vault && vault.walletStored
                text: "Danger zone"
            }

            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * Theme.horizontalPageMargin
                visible: vault && vault.walletStored
                text: "Deleting the development wallet removes its encrypted test recovery "
                      + "phrase from Sailfish Secrets. Watch-only profiles and Address book "
                      + "entries are not deleted."
                color: Theme.secondaryColor
                font.pixelSize: Theme.fontSizeSmall
                wrapMode: Text.Wrap
            }

            Button {
                anchors.horizontalCenter: parent.horizontalCenter
                visible: vault && vault.walletStored
                text: "Delete development wallet"
                onClicked: remorse.execute(
                    "Deleting development wallet",
                    function() {
                        vault.deleteDemoWallet()
                    })
            }

            Item {
                width: 1
                height: Theme.paddingMedium
            }
        }
    }
}
