import QtQuick 2.0
import Sailfish.Silica 1.0
import org.sailfishos.sailvault 1.0

Page {
    id: page

    property string chainName
    property string chainSubtitle
    property string labelText
    property string address
    property string balanceText
    property string fiatText
    property bool developmentAddress: false
    property bool balanceRefreshAttempted: false
    property string activityEthereumAddress: ""
    property string activityBitcoinAddress: ""
    property string activitySolanaAddress: ""

    function numericBalance(text) {
        var value = parseFloat(text)
        return isNaN(value) ? 0.0 : value
    }

    function localBalance() {
        if (chainName === "Ethereum")
            return publicPortfolio.ethereumBalance
        if (chainName === "Bitcoin")
            return publicPortfolio.bitcoinBalance
        if (chainName === "Solana")
            return publicPortfolio.solanaBalance
        return "—"
    }

    function localPrice() {
        if (chainName === "Ethereum")
            return publicPrices.ethereumPriceValue
        if (chainName === "Bitcoin")
            return publicPrices.bitcoinPriceValue
        if (chainName === "Solana")
            return publicPrices.solanaPriceValue
        return 0.0
    }

    property string effectiveBalance: {
        var local = localBalance()
        if (publicPortfolio.lastUpdated.length > 0
                && local.length > 0 && local !== "—")
            return local
        return balanceText
    }

    property string effectiveFiat: {
        var local = localBalance()
        var price = localPrice()
        if (publicPortfolio.lastUpdated.length > 0
                && local.length > 0 && local !== "—" && price > 0.0)
            return publicPrices.formatFiat(numericBalance(local) * price)
        return fiatText
    }

    AppTools {
        id: tools
    }

    PortfolioService {
        id: publicPortfolio
    }

    PriceService {
        id: publicPrices
    }

    Timer {
        id: copiedTimer
        interval: 1800
        onTriggered: copiedLabel.visible = false
    }

    function ethereumPublicAddress() {
        return chainName === "Ethereum" ? address : ""
    }

    function bitcoinPublicAddress() {
        return chainName === "Bitcoin" ? address : ""
    }

    function solanaPublicAddress() {
        return chainName === "Solana" ? address : ""
    }

    function openActivity() {
        pageStack.push(Qt.resolvedUrl("ActivityPage.qml"), {
            ethereumAddress: activityEthereumAddress.length > 0
                             ? activityEthereumAddress : ethereumPublicAddress(),
            bitcoinAddress: activityBitcoinAddress.length > 0
                            ? activityBitcoinAddress : bitcoinPublicAddress(),
            solanaAddress: activitySolanaAddress.length > 0
                           ? activitySolanaAddress : solanaPublicAddress(),
            initialChain: chainName
        })
    }

    function openTokens() {
        pageStack.push(Qt.resolvedUrl("TokensPage.qml"), {
            ethereumAddress: ethereumPublicAddress(),
            solanaAddress: solanaPublicAddress()
        })
    }

    Component.onCompleted: {
        // If the caller already supplied a balance (portfolio/watch-only), keep
        // that value. Otherwise read a last-known single-address snapshot. This
        // is local-only and never sends the public address to a provider.
        if (balanceText.length === 0) {
            publicPortfolio.loadSnapshot(ethereumPublicAddress(),
                                         bitcoinPublicAddress(),
                                         solanaPublicAddress())
        }
    }

    SilicaFlickable {
        anchors.fill: parent
        contentHeight: content.height + Theme.paddingLarge

        PullDownMenu {
            visible: !publicPortfolio.loading

            MenuItem {
                text: "Refresh public balance"
                onClicked: {
                    balanceRefreshAttempted = true
                    publicPortfolio.refresh(ethereumPublicAddress(),
                                            bitcoinPublicAddress(),
                                            solanaPublicAddress())
                }
            }
        }

        VerticalScrollDecorator { }

        Column {
            id: content
            width: parent.width
            spacing: Theme.paddingLarge

            PageHeader {
                title: labelText.length > 0
                       ? labelText
                       : chainName + " address"
                description: chainSubtitle
            }

            SectionHeader {
                text: "Balance"
            }

            BusyIndicator {
                anchors.horizontalCenter: parent.horizontalCenter
                running: publicPortfolio.loading
                size: BusyIndicatorSize.Medium
            }

            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * Theme.horizontalPageMargin
                visible: balanceRefreshAttempted && !publicPortfolio.loading
                text: publicPortfolio.status
                color: publicPortfolio.lastRefreshPassed
                       ? Theme.highlightColor
                       : Theme.primaryColor
                horizontalAlignment: Text.AlignHCenter
                font.pixelSize: Theme.fontSizeSmall
                wrapMode: Text.Wrap
            }

            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * Theme.horizontalPageMargin
                visible: effectiveBalance.length > 0
                text: effectiveBalance
                color: Theme.highlightColor
                font.pixelSize: Theme.fontSizeHuge
                horizontalAlignment: Text.AlignHCenter
                wrapMode: Text.Wrap
            }

            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * Theme.horizontalPageMargin
                visible: effectiveFiat.length > 0
                text: effectiveFiat
                color: Theme.secondaryHighlightColor
                font.pixelSize: Theme.fontSizeSmall
                horizontalAlignment: Text.AlignHCenter
                wrapMode: Text.Wrap
            }

            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * Theme.horizontalPageMargin
                visible: effectiveBalance.length === 0 && !publicPortfolio.loading
                text: "Balance not loaded. Pull down to refresh this public address."
                color: Theme.secondaryColor
                horizontalAlignment: Text.AlignHCenter
                font.pixelSize: Theme.fontSizeSmall
                wrapMode: Text.Wrap
            }

            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * Theme.horizontalPageMargin
                visible: publicPortfolio.lastUpdated.length > 0
                text: (publicPortfolio.usingCachedData ? "Cached balance: " : "Balance updated: ")
                      + publicPortfolio.lastUpdated
                color: Theme.secondaryColor
                font.pixelSize: Theme.fontSizeExtraSmall
                horizontalAlignment: Text.AlignHCenter
                wrapMode: Text.Wrap
            }

            SectionHeader {
                text: "Public address"
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
                        pageTitle: chainName + " address QR",
                        developmentAddress: developmentAddress
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

            SectionHeader {
                text: "Explore"
            }

            Button {
                anchors.horizontalCenter: parent.horizontalCenter
                text: "Recent activity"
                onClicked: openActivity()
            }

            Button {
                anchors.horizontalCenter: parent.horizontalCenter
                visible: chainName === "Ethereum" || chainName === "Solana"
                text: "Token holdings"
                onClicked: openTokens()
            }

            SectionHeader {
                text: "Privacy"
            }

            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * Theme.horizontalPageMargin
                text: "Opening this page and displaying/copying its address QR are offline. "
                      + "A cached balance, when available, is also loaded locally. Pulling "
                      + "down to refresh the balance sends this public address to the "
                      + "configured chain provider. Recent activity and Token holdings send "
                      + "the same public address only when those functions are opened."
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
                text: developmentAddress
                      ? "This is the public SailVault development-wallet address. "
                        + "Do not send real funds to it."
                      : "A syntactically valid public address does not prove who "
                        + "controls it. Verify addresses independently before any "
                        + "future transfer."
                color: Theme.secondaryColor
                font.pixelSize: Theme.fontSizeSmall
                wrapMode: Text.Wrap
            }
        }
    }
}
