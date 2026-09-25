import QtQuick 2.0
import Sailfish.Silica 1.0
import org.sailfishos.sailvault 1.0

Page {
    id: page

    property var tokenService
    property string chainName
    property string tokenName
    property string tokenSymbol
    property string amount
    property string identifier
    property string standard
    property string extra
    property bool hiddenState: false

    AppTools {
        id: tools
    }

    Component.onCompleted: {
        if (tokenService)
            hiddenState = tokenService.tokenHidden(chainName, identifier)
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
                title: tokenName.length > 0 ? tokenName : "Token"
                description: chainName + " · " + standard
            }

            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * Theme.horizontalPageMargin
                text: amount
                      + (tokenSymbol.length > 0
                         ? " " + tokenSymbol
                         : "")
                color: Theme.highlightColor
                font.pixelSize: Theme.fontSizeHuge
                horizontalAlignment: Text.AlignHCenter
                wrapMode: Text.WrapAnywhere
            }

            DetailItem {
                visible: tokenSymbol.length > 0
                label: "Symbol"
                value: tokenSymbol
            }

            DetailItem {
                label: "Standard"
                value: standard
            }

            DetailItem {
                label: "Visibility"
                value: hiddenState ? "Hidden" : "Shown"
            }

            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * Theme.horizontalPageMargin
                visible: extra.length > 0
                text: extra
                color: Theme.secondaryColor
                font.pixelSize: Theme.fontSizeSmall
                horizontalAlignment: Text.AlignHCenter
                wrapMode: Text.Wrap
            }

            SectionHeader {
                text: chainName === "Ethereum" ? "Contract" : "Mint"
            }

            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * Theme.horizontalPageMargin
                text: identifier
                color: Theme.primaryColor
                font.pixelSize: Theme.fontSizeSmall
                wrapMode: Text.WrapAnywhere
                horizontalAlignment: Text.AlignHCenter
            }

            Button {
                anchors.horizontalCenter: parent.horizontalCenter
                text: chainName === "Ethereum"
                      ? "Copy contract address"
                      : "Copy mint address"

                onClicked: {
                    tools.copyText(identifier)
                    copiedLabel.visible = true
                    copiedTimer.restart()
                }
            }

            Label {
                id: copiedLabel
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * Theme.horizontalPageMargin
                visible: false
                text: chainName === "Ethereum"
                      ? "Contract address copied"
                      : "Mint address copied"
                color: Theme.highlightColor
                horizontalAlignment: Text.AlignHCenter
            }

            Timer {
                id: copiedTimer
                interval: 1800
                onTriggered: copiedLabel.visible = false
            }

            Button {
                anchors.horizontalCenter: parent.horizontalCenter
                text: hiddenState ? "Show token" : "Hide token"

                onClicked: {
                    if (!tokenService)
                        return

                    tokenService.setTokenHidden(
                        chainName, identifier, !hiddenState)
                    hiddenState =
                        tokenService.tokenHidden(chainName, identifier)
                }
            }

            SectionHeader {
                text: "Token safety"
            }

            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * Theme.horizontalPageMargin
                text: "A token appearing in an address does not establish that the "
                      + "token, its name, symbol, website, price or any message attached "
                      + "to it is trustworthy. Unsolicited tokens can be spam or deceptive. "
                      + "Hiding a token is a local display preference only; it does not "
                      + "transfer, burn or otherwise interact with the token on-chain."
                color: Theme.secondaryColor
                font.pixelSize: Theme.fontSizeSmall
                wrapMode: Text.Wrap
            }

            SectionHeader {
                text: "Privacy"
            }

            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * Theme.horizontalPageMargin
                text: "This page performs no additional network lookup. It displays "
                      + "the public token information already returned by Token holdings."
                color: Theme.secondaryColor
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
