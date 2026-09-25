import QtQuick 2.0
import Sailfish.Silica 1.0
import org.sailfishos.sailvault 1.0

Page {
    id: page

    property string chainName
    property string chainSubtitle
    property string address
    property string balance
    property var vault
    property var portfolio

    AppTools {
        id: tools
    }

    Timer {
        id: copiedTimer
        interval: 1800
        onTriggered: copiedLabel.visible = false
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
                title: chainName
                description: chainSubtitle
            }

            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * Theme.horizontalPageMargin
                text: balance
                color: Theme.highlightColor
                font.pixelSize: Theme.fontSizeHuge
                horizontalAlignment: Text.AlignHCenter
                wrapMode: Text.Wrap
            }

            SectionHeader {
                text: "Receive address"
            }

            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * Theme.horizontalPageMargin
                text: address
                color: Theme.primaryColor
                font.pixelSize: Theme.fontSizeSmall
                wrapMode: Text.WrapAnywhere
                horizontalAlignment: Text.AlignHCenter
            }

            Button {
                anchors.horizontalCenter: parent.horizontalCenter
                text: "Show address QR"
                onClicked: pageStack.push(
                    Qt.resolvedUrl("ReceivePage.qml"), {
                        chainName: chainName,
                        chainSubtitle: chainSubtitle,
                        address: address,
                        pageTitle: "Receive " + chainName,
                        developmentAddress: true
                    })
            }

            Button {
                anchors.horizontalCenter: parent.horizontalCenter
                text: "Copy address"
                onClicked: {
                    tools.copyText(address)
                    copiedLabel.visible = true
                    copiedTimer.restart()
                }
            }

            Label {
                id: copiedLabel
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * Theme.horizontalPageMargin
                visible: false
                text: "Address copied to clipboard"
                color: Theme.highlightColor
                font.pixelSize: Theme.fontSizeSmall
                horizontalAlignment: Text.AlignHCenter
            }

            Button {
                anchors.horizontalCenter: parent.horizontalCenter
                enabled: vault && vault.walletLoaded && portfolio && !portfolio.loading
                text: portfolio && portfolio.loading ? "Refreshing…" : "Refresh balances"
                onClicked: portfolio.refresh(vault.ethereumAddress,
                                             vault.bitcoinAddress,
                                             vault.solanaAddress)
            }

            Button {
                anchors.horizontalCenter: parent.horizontalCenter
                enabled: vault && vault.walletLoaded
                text: "Recent activity"
                onClicked: pageStack.push(Qt.resolvedUrl("ActivityPage.qml"), {
                    ethereumAddress: vault.ethereumAddress,
                    bitcoinAddress: vault.bitcoinAddress,
                    solanaAddress: vault.solanaAddress,
                    initialChain: chainName
                })
            }

            SectionHeader {
                text: "Privacy"
            }

            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * Theme.horizontalPageMargin
                text: "Refreshing balances sends public addresses to the configured RPC/API providers. "
                      + "Recovery phrases and private keys never enter the network layer."
                color: Theme.secondaryColor
                font.pixelSize: Theme.fontSizeSmall
                wrapMode: Text.Wrap
            }

            SectionHeader {
                text: "Safety"
            }

            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * Theme.horizontalPageMargin
                text: "This is the public development wallet. The address is safe to copy for testing, "
                      + "but do not send real funds to it."
                color: Theme.secondaryColor
                font.pixelSize: Theme.fontSizeSmall
                wrapMode: Text.Wrap
            }
        }
    }
}
