import QtQuick 2.0
import Sailfish.Silica 1.0
import org.sailfishos.sailvault 1.0

Page {
    id: page

    WalletVault {
        id: vault
        Component.onCompleted: refreshStatus()
    }

    WalletCoreProbe {
        id: probe
        Component.onCompleted: run()
    }

    SilicaFlickable {
        anchors.fill: parent
        contentHeight: content.height

        VerticalScrollDecorator { }

        Column {
            id: content
            width: parent.width
            spacing: Theme.paddingLarge

            PageHeader {
                title: "SailVault"
                description: "Milestone 3 · secure wallet lifecycle"
            }

            SectionHeader {
                text: "Secure demo wallet"
            }

            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * Theme.horizontalPageMargin
                text: vault.status
                color: vault.lastOperationPassed
                       ? Theme.highlightColor : Theme.primaryColor
                font.pixelSize: Theme.fontSizeLarge
                wrapMode: Text.Wrap
            }

            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * Theme.horizontalPageMargin
                text: vault.detail
                color: Theme.secondaryColor
                font.pixelSize: Theme.fontSizeSmall
                wrapMode: Text.Wrap
            }

            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * Theme.horizontalPageMargin
                text: "Secrets backend: "
                      + (vault.backendReady ? "ready" : "not ready")
                      + " · stored wallet: "
                      + (vault.storageKnown
                         ? (vault.walletStored ? "yes" : "no")
                         : "unknown / locked")
                      + " · session loaded: "
                      + (vault.walletLoaded ? "yes" : "no")
                color: Theme.secondaryHighlightColor
                font.pixelSize: Theme.fontSizeExtraSmall
                wrapMode: Text.Wrap
            }

            Button {
                anchors.horizontalCenter: parent.horizontalCenter
                text: "Create secure demo wallet"
                onClicked: vault.createDemoWallet()
            }

            Button {
                anchors.horizontalCenter: parent.horizontalCenter
                text: "Load stored wallet"
                onClicked: vault.loadStoredWallet()
            }

            Button {
                anchors.horizontalCenter: parent.horizontalCenter
                enabled: vault.walletLoaded
                text: "Clear wallet session"
                onClicked: vault.clearSession()
            }

            Button {
                anchors.horizontalCenter: parent.horizontalCenter
                text: "Delete secure demo wallet"
                onClicked: vault.deleteDemoWallet()
            }

            Button {
                anchors.horizontalCenter: parent.horizontalCenter
                text: "Refresh secure storage status"
                onClicked: vault.refreshStatus()
            }

            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * Theme.horizontalPageMargin
                text: "Wallet lifecycle operations completed: " + vault.operationCount
                color: Theme.secondaryColor
                font.pixelSize: Theme.fontSizeExtraSmall
                horizontalAlignment: Text.AlignHCenter
            }

            Column {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * Theme.horizontalPageMargin
                spacing: Theme.paddingSmall
                visible: vault.walletLoaded

                Label {
                    width: parent.width
                    text: "Ethereum"
                    color: Theme.highlightColor
                    font.pixelSize: Theme.fontSizeMedium
                }

                Label {
                    width: parent.width
                    text: vault.ethereumAddress
                    color: Theme.primaryColor
                    font.pixelSize: Theme.fontSizeSmall
                    wrapMode: Text.WrapAnywhere
                }

                Label {
                    width: parent.width
                    text: "Bitcoin · BIP84"
                    color: Theme.highlightColor
                    font.pixelSize: Theme.fontSizeMedium
                }

                Label {
                    width: parent.width
                    text: vault.bitcoinAddress
                    color: Theme.primaryColor
                    font.pixelSize: Theme.fontSizeSmall
                    wrapMode: Text.WrapAnywhere
                }
            }

            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * Theme.horizontalPageMargin
                text: "Persistence test: create the demo wallet, close SailVault, "
                      + "open it again, then tap “Load stored wallet”. "
                      + "The two deterministic addresses should reappear."
                color: Theme.secondaryColor
                font.pixelSize: Theme.fontSizeExtraSmall
                wrapMode: Text.Wrap
            }

            SectionHeader {
                text: "Wallet Core diagnostics"
            }

            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * Theme.horizontalPageMargin
                text: probe.summary.length ? probe.summary : "Running self-test…"
                color: probe.allPassed ? Theme.highlightColor : Theme.primaryColor
                font.pixelSize: Theme.fontSizeMedium
                wrapMode: Text.Wrap
            }

            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * Theme.horizontalPageMargin
                text: probe.buildInfo
                color: Theme.secondaryColor
                font.pixelSize: Theme.fontSizeExtraSmall
                wrapMode: Text.Wrap
            }

            Repeater {
                model: probe.results

                delegate: Item {
                    width: content.width
                    height: resultColumn.height + Theme.paddingSmall

                    Column {
                        id: resultColumn
                        x: Theme.horizontalPageMargin
                        width: parent.width - 2 * Theme.horizontalPageMargin
                        spacing: Theme.paddingSmall

                        Label {
                            width: parent.width
                            text: (modelData.passed ? "✓ PASS · " : "✗ FAIL · ")
                                  + modelData.name
                            color: modelData.passed
                                   ? Theme.highlightColor : Theme.primaryColor
                            font.pixelSize: Theme.fontSizeSmall
                            wrapMode: Text.Wrap
                        }

                        Label {
                            width: parent.width
                            text: modelData.detail
                            color: Theme.secondaryColor
                            font.pixelSize: Theme.fontSizeExtraSmall
                            wrapMode: Text.WrapAnywhere
                        }
                    }
                }
            }

            Button {
                anchors.horizontalCenter: parent.horizontalCenter
                text: "Run Wallet Core diagnostics again"
                onClicked: probe.run()
            }

            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * Theme.horizontalPageMargin
                text: "Wallet Core diagnostic runs: " + probe.runCount
                color: Theme.secondaryColor
                font.pixelSize: Theme.fontSizeExtraSmall
                horizontalAlignment: Text.AlignHCenter
            }

            SectionHeader {
                text: "Safety"
            }

            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * Theme.horizontalPageMargin
                text: "Milestone 3 still uses only the public BIP39 test wallet. "
                      + "The recovery material is compiled into this development build only for deterministic testing, "
                      + "stored/retrieved entirely in C++, and never exposed to QML. "
                      + "Do not enter a real recovery phrase and do not send funds to these addresses."
                color: Theme.secondaryColor
                wrapMode: Text.Wrap
            }

            Item {
                width: 1
                height: Theme.paddingLarge
            }
        }
    }
}
