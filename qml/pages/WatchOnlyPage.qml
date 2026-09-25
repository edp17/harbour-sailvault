import QtQuick 2.0
import Sailfish.Silica 1.0
import org.sailfishos.sailvault 1.0

Page {
    id: page

    Timer {
        id: automaticRefreshTimer
        interval: 700
        repeat: false
        onTriggered: {
            if (watch.hasProfile && portfolio.autoRefreshAfterUnlock)
                refreshWatchOnly()
        }
    }

    WatchOnlyService {
        id: watch
    }

    PortfolioService {
        id: portfolio
    }

    PriceService {
        id: prices
    }

    PortfolioHistoryService {
        id: history
        fiatCurrency: prices.fiatCurrency
    }

    Connections {
        target: portfolio
        onStateChanged: page.maybeRecordHistory()
    }

    Connections {
        target: prices
        onStateChanged: page.maybeRecordHistory()
    }

    Connections {
        target: watch
        onStateChanged: page.syncHistoryContext()
    }

    function syncHistoryContext() {
        if (watch.hasProfile) {
            history.setContext(watch.ethereumAddress,
                               watch.bitcoinAddress,
                               watch.solanaAddress)
        } else {
            history.setContext("", "", "")
        }
    }

    function maybeRecordHistory() {
        if (!watch.hasProfile
                || portfolio.loading || prices.loading
                || !portfolio.lastRefreshPassed || !prices.lastRefreshPassed
                || portfolio.usingCachedData || prices.usingCachedData)
            return

        history.recordSnapshot(portfolio.ethereumBalance,
                               portfolio.bitcoinBalance,
                               portfolio.solanaBalance,
                               prices.ethereumPriceValue,
                               prices.bitcoinPriceValue,
                               prices.solanaPriceValue,
                               prices.fiatCurrency,
                               portfolio.lastUpdated,
                               prices.lastUpdated)
    }

    function numericBalance(text) {
        var value = parseFloat(text)
        return isNaN(value) ? 0.0 : value
    }

    function chainFiat(balanceText, priceValue) {
        if (priceValue <= 0.0)
            return "—"
        return prices.formatFiat(numericBalance(balanceText) * priceValue)
    }

    function freshnessText(balanceStatus, balanceUpdated, priceStatus, priceUpdated) {
        var balancePart = "Balance " + balanceStatus
        if (balanceUpdated.length > 0)
            balancePart += " · " + balanceUpdated
        var pricePart = "Price " + priceStatus
        if (priceUpdated.length > 0)
            pricePart += " · " + priceUpdated
        return balancePart + "\n" + pricePart
    }

    function refreshWatchOnly() {
        portfolio.refresh(watch.ethereumAddress,
                          watch.bitcoinAddress,
                          watch.solanaAddress)
        prices.refresh()
    }

    function openEditor() {
        var dialog = pageStack.push(
                    Qt.resolvedUrl("WatchOnlyEditDialog.qml"), {
                        watch: watch,
                        initialLabel: watch.hasProfile ? watch.label : "",
                        initialEthereumAddress: watch.ethereumAddress,
                        initialBitcoinAddress: watch.bitcoinAddress,
                        initialSolanaAddress: watch.solanaAddress,
                        editingExisting: watch.hasProfile
                    })

        dialog.accepted.connect(function() {
            if (!watch.hasProfile)
                return

            page.syncHistoryContext()
            portfolio.loadSnapshot(watch.ethereumAddress,
                                   watch.bitcoinAddress,
                                   watch.solanaAddress)

            if (portfolio.autoRefreshAfterUnlock)
                automaticRefreshTimer.restart()
        })
    }

    Component.onCompleted: {
        page.syncHistoryContext()
        if (watch.hasProfile) {
            portfolio.loadSnapshot(watch.ethereumAddress,
                                   watch.bitcoinAddress,
                                   watch.solanaAddress)

            if (portfolio.autoRefreshAfterUnlock)
                automaticRefreshTimer.start()
        }
    }

    SilicaFlickable {
        anchors.fill: parent
        contentHeight: content.height

        PullDownMenu {
            MenuItem {
                visible: !watch.hasProfile
                text: "Add watch-only profile"
                onClicked: openEditor()
            }

            MenuItem {
                visible: watch.hasProfile
                text: "Refresh portfolio"
                enabled: !portfolio.loading && !prices.loading
                onClicked: refreshWatchOnly()
            }

            MenuItem {
                visible: watch.hasProfile
                text: "Edit profile"
                onClicked: openEditor()
            }

            MenuItem {
                visible: watch.hasProfile
                text: "Clear profile"
                onClicked: remorse.execute(
                    "Clearing watch-only profile",
                    function() {
                        watch.clearProfile()
                    })
            }
        }

        RemorsePopup {
            id: remorse
        }

        VerticalScrollDecorator { }

        Column {
            id: content
            width: parent.width
            spacing: Theme.paddingLarge

            PageHeader {
                title: "Watch-only portfolio"
                description: "Public addresses only · no private keys"
            }

            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * Theme.horizontalPageMargin
                visible: portfolio.offlineMode
                text: "Offline mode · cached watch-only data remains available"
                color: Theme.highlightColor
                font.pixelSize: Theme.fontSizeSmall
                horizontalAlignment: Text.AlignHCenter
                wrapMode: Text.Wrap
            }

            Column {
                width: parent.width
                spacing: Theme.paddingLarge
                visible: !watch.hasProfile

                Label {
                    x: Theme.horizontalPageMargin
                    width: parent.width - 2 * Theme.horizontalPageMargin
                    text: "No watch-only profile is saved."
                    color: Theme.highlightColor
                    font.pixelSize: Theme.fontSizeLarge
                    horizontalAlignment: Text.AlignHCenter
                    wrapMode: Text.Wrap
                }

                Label {
                    x: Theme.horizontalPageMargin
                    width: parent.width - 2 * Theme.horizontalPageMargin
                    text: "Pull down to add one or more public Ethereum, Bitcoin "
                          + "or Solana addresses."
                    color: Theme.secondaryColor
                    horizontalAlignment: Text.AlignHCenter
                    font.pixelSize: Theme.fontSizeSmall
                    wrapMode: Text.Wrap
                }
            }

            Column {
                width: parent.width
                spacing: Theme.paddingLarge
                visible: watch.hasProfile

                SectionHeader {
                    text: watch.label
                }

                Label {
                    x: Theme.horizontalPageMargin
                    width: parent.width - 2 * Theme.horizontalPageMargin
                    text: portfolio.status
                    color: portfolio.lastRefreshPassed
                           ? Theme.highlightColor
                           : Theme.secondaryColor
                    font.pixelSize: Theme.fontSizeSmall
                    wrapMode: Text.Wrap
                }

                Label {
                    x: Theme.horizontalPageMargin
                    width: parent.width - 2 * Theme.horizontalPageMargin
                    visible: portfolio.usingCachedData || prices.usingCachedData
                    text: "Showing last-known cached data"
                    color: Theme.secondaryColor
                    font.pixelSize: Theme.fontSizeExtraSmall
                    horizontalAlignment: Text.AlignHCenter
                }

                Label {
                    x: Theme.horizontalPageMargin
                    width: parent.width - 2 * Theme.horizontalPageMargin
                    visible: portfolio.autoRefreshAfterUnlock
                    text: "Automatic portfolio refresh is enabled"
                    color: Theme.secondaryColor
                    font.pixelSize: Theme.fontSizeExtraSmall
                    horizontalAlignment: Text.AlignHCenter
                }

                BackgroundItem {
                    id: ethItem
                    width: parent.width
                    visible: watch.ethereumAddress.length > 0
                    height: ethColumn.height + 2 * Theme.paddingLarge

                    onClicked: pageStack.push(
                        Qt.resolvedUrl("PublicAddressPage.qml"), {
                            chainName: "Ethereum",
                            chainSubtitle: "Ethereum mainnet",
                            labelText: watch.label,
                            address: watch.ethereumAddress,
                            balanceText: portfolio.ethereumBalance,
                            fiatText: chainFiat(portfolio.ethereumBalance,
                                               prices.ethereumPriceValue),
                            developmentAddress: false,
                            activityEthereumAddress: watch.ethereumAddress,
                            activityBitcoinAddress: watch.bitcoinAddress,
                            activitySolanaAddress: watch.solanaAddress
                        })

                    Column {
                        id: ethColumn
                        x: Theme.horizontalPageMargin
                        width: parent.width - 2 * Theme.horizontalPageMargin
                        spacing: Theme.paddingSmall

                        Label {
                            width: parent.width
                            text: "Ethereum"
                            color: ethItem.highlighted
                                   ? Theme.highlightColor
                                   : Theme.primaryColor
                            font.pixelSize: Theme.fontSizeLarge
                        }

                        Label {
                            width: parent.width
                            text: portfolio.ethereumBalance
                            color: Theme.primaryColor
                            font.pixelSize: Theme.fontSizeMedium
                        }

                        Label {
                            width: parent.width
                            text: chainFiat(portfolio.ethereumBalance,
                                           prices.ethereumPriceValue)
                            color: Theme.secondaryHighlightColor
                            font.pixelSize: Theme.fontSizeSmall
                        }

                        Label {
                            width: parent.width
                            text: watch.ethereumAddress
                            color: Theme.secondaryColor
                            font.pixelSize: Theme.fontSizeExtraSmall
                            elide: Text.ElideMiddle
                        }

                        Label {
                            width: parent.width
                            text: freshnessText(portfolio.ethereumStatus, portfolio.ethereumUpdated,
                                                prices.ethereumStatus, prices.ethereumUpdated)
                            color: (portfolio.ethereumStatus.indexOf("FAIL") === 0
                                    || prices.ethereumStatus.indexOf("FAIL") === 0)
                                   ? Theme.highlightColor : Theme.secondaryColor
                            font.pixelSize: Theme.fontSizeExtraSmall
                            wrapMode: Text.Wrap
                        }
                    }
                }

                BackgroundItem {
                    id: btcItem
                    width: parent.width
                    visible: watch.bitcoinAddress.length > 0
                    height: btcColumn.height + 2 * Theme.paddingLarge

                    onClicked: pageStack.push(
                        Qt.resolvedUrl("PublicAddressPage.qml"), {
                            chainName: "Bitcoin",
                            chainSubtitle: "Bitcoin mainnet",
                            labelText: watch.label,
                            address: watch.bitcoinAddress,
                            balanceText: portfolio.bitcoinBalance,
                            fiatText: chainFiat(portfolio.bitcoinBalance,
                                               prices.bitcoinPriceValue),
                            developmentAddress: false,
                            activityEthereumAddress: watch.ethereumAddress,
                            activityBitcoinAddress: watch.bitcoinAddress,
                            activitySolanaAddress: watch.solanaAddress
                        })

                    Column {
                        id: btcColumn
                        x: Theme.horizontalPageMargin
                        width: parent.width - 2 * Theme.horizontalPageMargin
                        spacing: Theme.paddingSmall

                        Label {
                            width: parent.width
                            text: "Bitcoin"
                            color: btcItem.highlighted
                                   ? Theme.highlightColor
                                   : Theme.primaryColor
                            font.pixelSize: Theme.fontSizeLarge
                        }

                        Label {
                            width: parent.width
                            text: portfolio.bitcoinBalance
                            color: Theme.primaryColor
                            font.pixelSize: Theme.fontSizeMedium
                        }

                        Label {
                            width: parent.width
                            text: chainFiat(portfolio.bitcoinBalance,
                                           prices.bitcoinPriceValue)
                            color: Theme.secondaryHighlightColor
                            font.pixelSize: Theme.fontSizeSmall
                        }

                        Label {
                            width: parent.width
                            text: watch.bitcoinAddress
                            color: Theme.secondaryColor
                            font.pixelSize: Theme.fontSizeExtraSmall
                            elide: Text.ElideMiddle
                        }

                        Label {
                            width: parent.width
                            text: freshnessText(portfolio.bitcoinStatus, portfolio.bitcoinUpdated,
                                                prices.bitcoinStatus, prices.bitcoinUpdated)
                            color: (portfolio.bitcoinStatus.indexOf("FAIL") === 0
                                    || prices.bitcoinStatus.indexOf("FAIL") === 0)
                                   ? Theme.highlightColor : Theme.secondaryColor
                            font.pixelSize: Theme.fontSizeExtraSmall
                            wrapMode: Text.Wrap
                        }
                    }
                }

                BackgroundItem {
                    id: solItem
                    width: parent.width
                    visible: watch.solanaAddress.length > 0
                    height: solColumn.height + 2 * Theme.paddingLarge

                    onClicked: pageStack.push(
                        Qt.resolvedUrl("PublicAddressPage.qml"), {
                            chainName: "Solana",
                            chainSubtitle: "Solana mainnet",
                            labelText: watch.label,
                            address: watch.solanaAddress,
                            balanceText: portfolio.solanaBalance,
                            fiatText: chainFiat(portfolio.solanaBalance,
                                               prices.solanaPriceValue),
                            developmentAddress: false,
                            activityEthereumAddress: watch.ethereumAddress,
                            activityBitcoinAddress: watch.bitcoinAddress,
                            activitySolanaAddress: watch.solanaAddress
                        })

                    Column {
                        id: solColumn
                        x: Theme.horizontalPageMargin
                        width: parent.width - 2 * Theme.horizontalPageMargin
                        spacing: Theme.paddingSmall

                        Label {
                            width: parent.width
                            text: "Solana"
                            color: solItem.highlighted
                                   ? Theme.highlightColor
                                   : Theme.primaryColor
                            font.pixelSize: Theme.fontSizeLarge
                        }

                        Label {
                            width: parent.width
                            text: portfolio.solanaBalance
                            color: Theme.primaryColor
                            font.pixelSize: Theme.fontSizeMedium
                        }

                        Label {
                            width: parent.width
                            text: chainFiat(portfolio.solanaBalance,
                                           prices.solanaPriceValue)
                            color: Theme.secondaryHighlightColor
                            font.pixelSize: Theme.fontSizeSmall
                        }

                        Label {
                            width: parent.width
                            text: watch.solanaAddress
                            color: Theme.secondaryColor
                            font.pixelSize: Theme.fontSizeExtraSmall
                            elide: Text.ElideMiddle
                        }

                        Label {
                            width: parent.width
                            text: freshnessText(portfolio.solanaStatus, portfolio.solanaUpdated,
                                                prices.solanaStatus, prices.solanaUpdated)
                            color: (portfolio.solanaStatus.indexOf("FAIL") === 0
                                    || prices.solanaStatus.indexOf("FAIL") === 0)
                                   ? Theme.highlightColor : Theme.secondaryColor
                            font.pixelSize: Theme.fontSizeExtraSmall
                            wrapMode: Text.Wrap
                        }
                    }
                }

                SectionHeader {
                    text: "Explore"
                }

                Button {
                    anchors.horizontalCenter: parent.horizontalCenter
                    text: "Portfolio history"
                    onClicked: pageStack.push(
                        Qt.resolvedUrl("PortfolioHistoryPage.qml"), {
                            history: history,
                            prices: prices,
                            portfolioLabel: watch.label
                        })
                }

                Button {
                    anchors.horizontalCenter: parent.horizontalCenter
                    text: "Recent activity"
                    onClicked: pageStack.push(
                        Qt.resolvedUrl("ActivityPage.qml"), {
                            ethereumAddress: watch.ethereumAddress,
                            bitcoinAddress: watch.bitcoinAddress,
                            solanaAddress: watch.solanaAddress
                        })
                }

                Button {
                    anchors.horizontalCenter: parent.horizontalCenter
                    visible: watch.ethereumAddress.length > 0
                             || watch.solanaAddress.length > 0
                    text: "Token holdings"
                    onClicked: pageStack.push(
                        Qt.resolvedUrl("TokensPage.qml"), {
                            ethereumAddress: watch.ethereumAddress,
                            solanaAddress: watch.solanaAddress
                        })
                }
            }

            SectionHeader {
                text: "Privacy & safety"
            }

            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * Theme.horizontalPageMargin
                text: "Watch-only mode stores public addresses only. It cannot sign "
                      + "or broadcast transactions. Refreshing balances, activity or "
                      + "tokens sends configured public addresses to the selected "
                      + "RPC/API providers."
                color: Theme.secondaryColor
                font.pixelSize: Theme.fontSizeSmall
                wrapMode: Text.Wrap
            }

            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * Theme.horizontalPageMargin
                text: "Use watch-only mode to test real public blockchain data. "
                      + "Never enter a real recovery phrase or private key into this "
                      + "development build."
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
