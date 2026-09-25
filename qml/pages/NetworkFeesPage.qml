import QtQuick 2.0
import Sailfish.Silica 1.0
import org.sailfishos.sailvault 1.0

Page {
    id: page

    property var portfolio

    NetworkFeeService {
        id: fees
    }

    function refreshFees() {
        if (!portfolio)
            return

        fees.refresh(portfolio.ethereumRpcUrl,
                     portfolio.bitcoinApiUrl,
                     portfolio.solanaRpcUrl)
    }

    SilicaFlickable {
        anchors.fill: parent
        contentHeight: content.height + Theme.paddingLarge

        PullDownMenu {
            visible: !fees.loading && !!portfolio

            MenuItem {
                text: "Refresh fee estimates"
                onClicked: refreshFees()
            }
        }

        VerticalScrollDecorator { }

        Column {
            id: content
            width: parent.width
            spacing: Theme.paddingLarge

            PageHeader {
                title: "Network fees"
                description: "Read-only public fee estimates"
            }

            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * Theme.horizontalPageMargin
                text: "Fee estimates run only when requested. They use generic public "
                      + "network queries and do not create a transaction or send a wallet "
                      + "address, private key or recovery phrase."
                color: Theme.secondaryColor
                font.pixelSize: Theme.fontSizeSmall
                wrapMode: Text.Wrap
            }

            SectionHeader {
                text: "Status"
            }

            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * Theme.horizontalPageMargin
                text: fees.loading ? "Refreshing fee estimates…" : fees.summary
                color: fees.loading ? Theme.highlightColor : Theme.secondaryColor
                font.pixelSize: Theme.fontSizeSmall
                wrapMode: Text.Wrap
            }

            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * Theme.horizontalPageMargin
                visible: fees.lastUpdated.length > 0
                text: "Last updated: " + fees.lastUpdated
                color: Theme.secondaryColor
                font.pixelSize: Theme.fontSizeExtraSmall
                wrapMode: Text.Wrap
            }

            SectionHeader {
                text: "Current estimates"
            }

            Repeater {
                model: fees.estimates

                delegate: Column {
                    width: content.width
                    spacing: Theme.paddingSmall

                    Label {
                        x: Theme.horizontalPageMargin
                        width: parent.width - 2 * Theme.horizontalPageMargin
                        text: modelData.name + " · " + modelData.status
                        color: modelData.state === "pass"
                               ? Theme.highlightColor
                               : modelData.state === "fail"
                                 ? Theme.errorColor
                                 : Theme.primaryColor
                        font.pixelSize: Theme.fontSizeMedium
                        wrapMode: Text.Wrap
                    }

                    Label {
                        x: Theme.horizontalPageMargin
                        width: parent.width - 2 * Theme.horizontalPageMargin
                        text: modelData.headline
                        color: Theme.primaryColor
                        font.pixelSize: Theme.fontSizeSmall
                        wrapMode: Text.Wrap
                    }

                    Label {
                        x: Theme.horizontalPageMargin
                        width: parent.width - 2 * Theme.horizontalPageMargin
                        text: modelData.detail
                              + (modelData.latency.length > 0
                                 ? " · " + modelData.latency
                                 : "")
                        color: Theme.secondaryHighlightColor
                        font.pixelSize: Theme.fontSizeSmall
                        wrapMode: Text.Wrap
                    }

                    Label {
                        x: Theme.horizontalPageMargin
                        width: parent.width - 2 * Theme.horizontalPageMargin
                        text: modelData.endpoint
                        color: Theme.secondaryColor
                        font.pixelSize: Theme.fontSizeExtraSmall
                        wrapMode: Text.WrapAnywhere
                    }

                    Separator {
                        width: parent.width
                        color: Theme.secondaryColor
                    }
                }
            }

            SectionHeader {
                text: "How to read these numbers"
            }

            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * Theme.horizontalPageMargin
                text: "Ethereum shows the provider's eth_gasPrice suggestion and an "
                      + "illustrative 21,000-gas native ETH transfer. Contract calls and "
                      + "token transfers can use substantially more gas."
                color: Theme.secondaryColor
                font.pixelSize: Theme.fontSizeSmall
                wrapMode: Text.Wrap
            }

            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * Theme.horizontalPageMargin
                text: "Bitcoin shows Esplora estimates for roughly 1, 6 and 144 blocks. "
                      + "The satoshi examples assume a 140-vbyte transaction; actual size "
                      + "depends on the number and type of inputs and outputs."
                color: Theme.secondaryColor
                font.pixelSize: Theme.fontSizeSmall
                wrapMode: Text.Wrap
            }

            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * Theme.horizontalPageMargin
                text: "Solana samples recent network-wide priority prices with an empty "
                      + "account list. The base fee shown is per signature; the optional "
                      + "priority fee depends on compute-unit limit and price."
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
                text: "These are informational estimates, not a transaction quote. "
                      + "This development build still has no production signing or broadcasting UI. "
                      + "Each contacted provider can observe ordinary network metadata such "
                      + "as your IP address."
                color: Theme.highlightColor
                font.pixelSize: Theme.fontSizeSmall
                wrapMode: Text.Wrap
            }
        }
    }
}
