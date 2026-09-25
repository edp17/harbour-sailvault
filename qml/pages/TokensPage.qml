import QtQuick 2.0
import Sailfish.Silica 1.0
import org.sailfishos.sailvault 1.0

Page {
    id: page

    property string ethereumAddress
    property string solanaAddress

    TokenService {
        id: tokens
    }

    AppTools {
        id: tools
    }

    Component.onCompleted: {
        tokens.refresh(ethereumAddress, solanaAddress)
    }

    SilicaFlickable {
        anchors.fill: parent
        contentHeight: content.height

        PullDownMenu {
            MenuItem {
                visible: tokens.hiddenTokenCount > 0
                text: tokens.showHidden
                      ? "Hide hidden tokens"
                      : "Show hidden tokens (" + tokens.hiddenTokenCount + ")"
                onClicked: tokens.showHidden = !tokens.showHidden
            }

            MenuItem {
                text: "Refresh token holdings"
                enabled: !tokens.loading
                onClicked: tokens.refresh(ethereumAddress, solanaAddress)
            }
        }

        VerticalScrollDecorator { }

        Column {
            id: content
            width: parent.width
            spacing: Theme.paddingLarge

            PageHeader {
                title: "Token holdings"
                description: "Ethereum ERC-20 · Solana SPL / Token-2022"
            }

            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * Theme.horizontalPageMargin
                text: tokens.status
                color: tokens.lastRefreshPassed
                       ? Theme.highlightColor : Theme.primaryColor
                font.pixelSize: Theme.fontSizeSmall
                wrapMode: Text.Wrap
            }

            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * Theme.horizontalPageMargin
                text: "Ethereum: " + tokens.ethereumStatus
                      + "\nSolana: " + tokens.solanaStatus
                color: Theme.secondaryColor
                font.pixelSize: Theme.fontSizeExtraSmall
                wrapMode: Text.Wrap
            }

            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * Theme.horizontalPageMargin
                visible: !tokens.loading
                text: "Visible holdings: " + tokens.tokenCount
                      + (tokens.hiddenTokenCount > 0
                         ? " · hidden: " + tokens.hiddenTokenCount
                         : "")
                color: Theme.secondaryColor
                font.pixelSize: Theme.fontSizeExtraSmall
                horizontalAlignment: Text.AlignHCenter
            }

            BusyIndicator {
                anchors.horizontalCenter: parent.horizontalCenter
                running: tokens.loading
                size: BusyIndicatorSize.Medium
            }

            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * Theme.horizontalPageMargin
                visible: !tokens.loading && tokens.tokenCount === 0
                text: tokens.hiddenTokenCount > 0 && !tokens.showHidden
                      ? "All returned token holdings are hidden. Use the pulley menu "
                        + "to show hidden tokens."
                      : "No non-zero ERC-20, SPL Token or Token-2022 holdings "
                        + "were returned for the development wallet."
                color: Theme.secondaryColor
                horizontalAlignment: Text.AlignHCenter
                font.pixelSize: Theme.fontSizeSmall
                wrapMode: Text.Wrap
            }

            Repeater {
                model: tokens.entries

                delegate: ListItem {
                    width: content.width
                    contentHeight: tokenColumn.height + 2 * Theme.paddingMedium
                    opacity: modelData.hidden ? 0.55 : 1.0

                    menu: ContextMenu {
                        MenuItem {
                            text: modelData.hidden ? "Show token" : "Hide token"
                            onClicked: tokens.setTokenHidden(
                                modelData.chain,
                                modelData.identifier,
                                !modelData.hidden)
                        }

                        MenuItem {
                            text: "Copy token identifier"
                            onClicked: {
                                tools.copyText(modelData.identifier)
                                copiedNotice.text =
                                    modelData.chain + " token identifier copied"
                                copiedNotice.visible = true
                                copiedTimer.restart()
                            }
                        }
                    }

                    onClicked: {
                        pageStack.push(Qt.resolvedUrl("TokenDetailPage.qml"), {
                            tokenService: tokens,
                            chainName: modelData.chain,
                            tokenName: modelData.name,
                            tokenSymbol: modelData.symbol,
                            amount: modelData.amount,
                            identifier: modelData.identifier,
                            standard: modelData.standard,
                            extra: modelData.extra
                        })
                    }

                    Column {
                        id: tokenColumn
                        x: Theme.horizontalPageMargin
                        width: parent.width - 2 * Theme.horizontalPageMargin
                        spacing: Theme.paddingSmall

                        Row {
                            width: parent.width

                            Label {
                                width: parent.width * 0.55
                                text: modelData.chain
                                color: Theme.highlightColor
                                font.pixelSize: Theme.fontSizeSmall
                            }

                            Label {
                                width: parent.width * 0.45
                                text: modelData.standard
                                      + (modelData.hidden ? " · hidden" : "")
                                color: Theme.secondaryColor
                                font.pixelSize: Theme.fontSizeExtraSmall
                                horizontalAlignment: Text.AlignRight
                            }
                        }

                        Label {
                            width: parent.width
                            text: modelData.name
                                  + (modelData.symbol
                                     ? " · " + modelData.symbol
                                     : "")
                            color: Theme.primaryColor
                            font.pixelSize: Theme.fontSizeMedium
                            wrapMode: Text.Wrap
                        }

                        Label {
                            width: parent.width
                            text: modelData.amount
                                  + (modelData.symbol
                                     ? " " + modelData.symbol
                                     : "")
                            color: Theme.highlightColor
                            font.pixelSize: Theme.fontSizeLarge
                            wrapMode: Text.WrapAnywhere
                        }

                        Label {
                            width: parent.width
                            visible: modelData.extra.length > 0
                            text: modelData.extra
                            color: Theme.secondaryColor
                            font.pixelSize: Theme.fontSizeExtraSmall
                            wrapMode: Text.Wrap
                        }

                        Label {
                            width: parent.width
                            text: modelData.identifier
                            color: Theme.secondaryColor
                            font.pixelSize: Theme.fontSizeExtraSmall
                            elide: Text.ElideMiddle
                        }
                    }
                }
            }

            Label {
                id: copiedNotice
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * Theme.horizontalPageMargin
                visible: false
                color: Theme.highlightColor
                horizontalAlignment: Text.AlignHCenter
                font.pixelSize: Theme.fontSizeSmall
                wrapMode: Text.Wrap
            }

            Timer {
                id: copiedTimer
                interval: 1800
                onTriggered: copiedNotice.visible = false
            }

            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * Theme.horizontalPageMargin
                visible: tokens.lastUpdated.length > 0
                text: "Last token update: " + tokens.lastUpdated
                color: Theme.secondaryColor
                font.pixelSize: Theme.fontSizeExtraSmall
                horizontalAlignment: Text.AlignHCenter
            }

            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * Theme.horizontalPageMargin
                text: "Tap a holding for details. Long-press a holding to hide/show "
                      + "it or copy its public token identifier."
                color: Theme.secondaryColor
                font.pixelSize: Theme.fontSizeSmall
                horizontalAlignment: Text.AlignHCenter
                wrapMode: Text.Wrap
            }

            SectionHeader {
                text: "Token safety"
            }

            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * Theme.horizontalPageMargin
                text: "Unsolicited tokens can appear in any public address. Their "
                      + "presence does not establish legitimacy or value. Hidden-token "
                      + "choices are local display preferences stored by SailVault."
                color: Theme.secondaryColor
                font.pixelSize: Theme.fontSizeSmall
                wrapMode: Text.Wrap
            }

            SectionHeader {
                text: "Bitcoin"
            }

            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * Theme.horizontalPageMargin
                text: "Bitcoin remains native BTC-only in this milestone. "
                      + "Ordinals, Runes and BRC-20 are deliberately not mixed "
                      + "into the Ethereum/Solana fungible-token model yet."
                color: Theme.secondaryColor
                font.pixelSize: Theme.fontSizeSmall
                wrapMode: Text.Wrap
            }

            SectionHeader {
                text: "Solana metadata"
            }

            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * Theme.horizontalPageMargin
                text: "Solana token-account RPC data provides the mint and exact "
                      + "amount, but not a trusted symbol/name. SailVault therefore "
                      + "shows the mint identifier instead of guessing metadata."
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
                text: "Token lookup sends only the public Ethereum and Solana "
                      + "addresses to the configured providers. Recovery phrases "
                      + "and private keys never enter TokenService."
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
