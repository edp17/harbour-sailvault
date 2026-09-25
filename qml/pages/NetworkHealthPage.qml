import QtQuick 2.0
import Sailfish.Silica 1.0
import org.sailfishos.sailvault 1.0

Page {
    id: page

    property var portfolio
    property var prices

    NetworkHealthService {
        id: health
    }

    function runChecks() {
        if (!portfolio)
            return

        health.checkAll(portfolio.ethereumRpcUrl,
                        portfolio.ethereumExplorerUrl,
                        portfolio.bitcoinApiUrl,
                        portfolio.solanaRpcUrl,
                        portfolio.solanaTokenRpcUrl,
                        prices ? prices.fiatCurrency : "GBP")
    }

    SilicaFlickable {
        anchors.fill: parent
        contentHeight: content.height + Theme.paddingLarge

        PullDownMenu {
            visible: !health.loading && !!portfolio

            MenuItem {
                text: "Check providers"
                onClicked: runChecks()
            }
        }

        VerticalScrollDecorator { }

        Column {
            id: content
            width: parent.width
            spacing: Theme.paddingLarge

            PageHeader {
                title: "Provider health"
                description: "Address-free network verification"
            }

            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * Theme.horizontalPageMargin
                text: "Checks run only when requested. They verify reachability and, "
                      + "where possible, mainnet identity without sending a wallet address, "
                      + "balance query, recovery phrase or private key."
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
                text: health.loading ? "Checking configured providers…" : health.summary
                color: health.loading ? Theme.highlightColor : Theme.secondaryColor
                font.pixelSize: Theme.fontSizeSmall
                wrapMode: Text.Wrap
            }

            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * Theme.horizontalPageMargin
                visible: health.lastChecked.length > 0
                text: "Last checked: " + health.lastChecked
                color: Theme.secondaryColor
                font.pixelSize: Theme.fontSizeExtraSmall
                wrapMode: Text.Wrap
            }

            SectionHeader {
                text: "Providers"
            }

            Repeater {
                model: health.providers

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
                        text: modelData.purpose
                        color: Theme.secondaryColor
                        font.pixelSize: Theme.fontSizeExtraSmall
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
                text: "What is sent"
            }

            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * Theme.horizontalPageMargin
                text: "Ethereum: eth_chainId. Bitcoin: block-height/0. "
                      + "Solana: getGenesisHash. Explorer: /stats. "
                      + "Kraken: one public XBT fiat ticker request."
                color: Theme.secondaryColor
                font.pixelSize: Theme.fontSizeSmall
                wrapMode: Text.Wrap
            }

            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * Theme.horizontalPageMargin
                text: "These checks avoid wallet identifiers, but each contacted provider "
                      + "can still observe ordinary network metadata such as your IP address."
                color: Theme.highlightColor
                font.pixelSize: Theme.fontSizeSmall
                wrapMode: Text.Wrap
            }

            SectionHeader {
                text: "Interpretation"
            }

            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * Theme.horizontalPageMargin
                text: "Verified means the endpoint answered the expected public probe. "
                      + "Attention means it answered but did not match the expected mainnet "
                      + "identity or returned incomplete diagnostic data. Failed means the "
                      + "endpoint could not be verified. This is a connectivity diagnostic, "
                      + "not a security endorsement of a provider."
                color: Theme.secondaryColor
                font.pixelSize: Theme.fontSizeSmall
                wrapMode: Text.Wrap
            }
        }
    }
}
