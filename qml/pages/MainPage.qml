import QtQuick 2.0
import Sailfish.Silica 1.0
import org.sailfishos.sailvault 1.0

Page {
    id: page

    property bool previousWalletLoaded: false

    AppTools {
        id: appTools
    }

    Component.onCompleted: page.syncHistoryContext()

    Timer {
        id: unlockRefreshTimer
        interval: 900
        repeat: false

        onTriggered: {
            if (!vault.walletLoaded || !portfolio.autoRefreshAfterUnlock)
                return

            portfolio.refresh(vault.ethereumAddress,
                              vault.bitcoinAddress,
                              vault.solanaAddress)
            prices.refresh()
        }
    }

    WalletVault {
        id: vault
        Component.onCompleted: refreshStatus()
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

    SessionSecurity {
        id: sessionSecurity
        sessionActive: vault.walletLoaded

        onLockRequested: {
            vault.lockSession(reason)
        }
    }

    Connections {
        target: vault

        onStateChanged: {
            page.syncHistoryContext()
            if (vault.walletLoaded && !page.previousWalletLoaded) {
                page.previousWalletLoaded = true

                portfolio.loadSnapshot(vault.ethereumAddress,
                                       vault.bitcoinAddress,
                                       vault.solanaAddress)

                if (portfolio.autoRefreshAfterUnlock)
                    unlockRefreshTimer.restart()
            } else if (!vault.walletLoaded) {
                page.previousWalletLoaded = false
                unlockRefreshTimer.stop()
            }
        }
    }

    Connections {
        target: portfolio
        onStateChanged: page.maybeRecordHistory()
    }

    Connections {
        target: prices
        onStateChanged: page.maybeRecordHistory()
    }

    function syncHistoryContext() {
        if (vault.walletLoaded) {
            history.setContext(vault.ethereumAddress,
                               vault.bitcoinAddress,
                               vault.solanaAddress)
        } else {
            history.setContext("", "", "")
        }
    }

    function maybeRecordHistory() {
        if (!vault.walletLoaded
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

    function nativePortfolioValue() {
        return numericBalance(portfolio.bitcoinBalance) * prices.bitcoinPriceValue
             + numericBalance(portfolio.ethereumBalance) * prices.ethereumPriceValue
             + numericBalance(portfolio.solanaBalance) * prices.solanaPriceValue
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

    SilicaFlickable {
        anchors.fill: parent
        contentHeight: content.height

        PullDownMenu {
            MenuItem {
                text: "Settings"
                onClicked: pageStack.push(Qt.resolvedUrl("SettingsPage.qml"), {
                    portfolio: portfolio,
                    prices: prices,
                    security: sessionSecurity,
                    vault: vault
                })
            }

            MenuItem {
                text: "Watch-only portfolio"
                onClicked: pageStack.push(Qt.resolvedUrl("WatchOnlyPage.qml"))
            }

            MenuItem {
                text: "Address book"
                onClicked: pageStack.push(Qt.resolvedUrl("AddressBookPage.qml"), {
                    vault: vault
                })
            }

            MenuItem {
                text: "About SailVault"
                onClicked: pageStack.push(Qt.resolvedUrl("AboutPage.qml"))
            }

        }

        VerticalScrollDecorator { }

        Column {
            id: content
            width: parent.width
            spacing: Theme.paddingLarge

            PageHeader {
                title: "SailVault"
                description: "Milestone " + appTools.milestone + " · " + appTools.buildLabel
            }

            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * Theme.horizontalPageMargin
                visible: portfolio.offlineMode
                text: "Offline mode · provider requests are blocked; cached data remains available"
                color: Theme.highlightColor
                font.pixelSize: Theme.fontSizeSmall
                horizontalAlignment: Text.AlignHCenter
                wrapMode: Text.Wrap
            }

            Column {
                width: parent.width
                spacing: Theme.paddingLarge
                visible: !vault.backendReady

                SectionHeader {
                    text: "Secure storage"
                }

                Label {
                    x: Theme.horizontalPageMargin
                    width: parent.width - 2 * Theme.horizontalPageMargin
                    text: "Waiting for Sailfish Secrets…"
                    color: Theme.secondaryColor
                    horizontalAlignment: Text.AlignHCenter
                }
            }

            Column {
                width: parent.width
                spacing: Theme.paddingLarge
                visible: vault.backendReady
                         && vault.storageKnown
                         && !vault.walletStored
                         && !vault.walletLoaded

                SectionHeader {
                    text: "Development wallet"
                }

                Label {
                    x: Theme.horizontalPageMargin
                    width: parent.width - 2 * Theme.horizontalPageMargin
                    text: "No wallet is stored in Sailfish Secrets. Create or restore "
                          + "the public development wallet."
                    color: Theme.secondaryColor
                    font.pixelSize: Theme.fontSizeSmall
                    wrapMode: Text.Wrap
                }

                Button {
                    anchors.horizontalCenter: parent.horizontalCenter
                    text: "Create development wallet"
                    onClicked: vault.createDemoWallet()
                }

                Button {
                    anchors.horizontalCenter: parent.horizontalCenter
                    text: "Restore development wallet"
                    onClicked: pageStack.push(Qt.resolvedUrl("RestoreWalletPage.qml"), {
                        vault: vault,
                        developmentTestMnemonic: vault.developmentTestMnemonic
                    })
                }
            }

            Column {
                width: parent.width
                spacing: Theme.paddingLarge
                visible: vault.backendReady
                         && !vault.walletLoaded
                         && (!vault.storageKnown || vault.walletStored)

                SectionHeader {
                    text: vault.storageKnown ? "Wallet locked" : "Secure wallet locked"
                }

                Label {
                    x: Theme.horizontalPageMargin
                    width: parent.width - 2 * Theme.horizontalPageMargin
                    text: vault.storageKnown
                          ? "The wallet is stored in Sailfish Secrets. Unlock it to "
                            + "derive the public chain addresses for this session."
                          : "Sailfish Secrets is locked, so SailVault cannot inspect "
                            + "the encrypted wallet without interaction. Unlock secure "
                            + "storage to load the stored wallet."
                    color: Theme.secondaryColor
                    font.pixelSize: Theme.fontSizeSmall
                    wrapMode: Text.Wrap
                }

                Label {
                    x: Theme.horizontalPageMargin
                    width: parent.width - 2 * Theme.horizontalPageMargin
                    visible: vault.status.indexOf("locked automatically") >= 0
                    text: vault.detail
                    color: Theme.highlightColor
                    font.pixelSize: Theme.fontSizeSmall
                    wrapMode: Text.Wrap
                }

                Button {
                    anchors.horizontalCenter: parent.horizontalCenter
                    text: "Unlock stored wallet"
                    onClicked: vault.loadStoredWallet()
                }

                Button {
                    anchors.horizontalCenter: parent.horizontalCenter
                    visible: vault.storageKnown
                    text: "Refresh secure storage status"
                    onClicked: vault.refreshStatus()
                }
            }

            Column {
                width: parent.width
                spacing: Theme.paddingLarge
                visible: vault.walletLoaded

                SectionHeader {
                    text: "Portfolio"
                }

                Label {
                    x: Theme.horizontalPageMargin
                    width: parent.width - 2 * Theme.horizontalPageMargin
                    visible: sessionSecurity.enabled
                    text: "Auto-lock: background "
                          + sessionSecurity.backgroundDelayText
                          + " · inactivity "
                          + sessionSecurity.inactivityText
                    color: Theme.secondaryColor
                    font.pixelSize: Theme.fontSizeExtraSmall
                    horizontalAlignment: Text.AlignHCenter
                    wrapMode: Text.Wrap
                }

                Label {
                    x: Theme.horizontalPageMargin
                    width: parent.width - 2 * Theme.horizontalPageMargin
                    visible: portfolio.autoRefreshAfterUnlock
                    text: "Portfolio refreshes automatically after unlock"
                    color: Theme.secondaryColor
                    font.pixelSize: Theme.fontSizeExtraSmall
                    horizontalAlignment: Text.AlignHCenter
                }

                Label {
                    x: Theme.horizontalPageMargin
                    width: parent.width - 2 * Theme.horizontalPageMargin
                    text: (prices.bitcoinPriceValue > 0.0
                           && prices.ethereumPriceValue > 0.0
                           && prices.solanaPriceValue > 0.0)
                          ? prices.formatFiat(nativePortfolioValue())
                          : "—"
                    color: Theme.highlightColor
                    font.pixelSize: Theme.fontSizeHuge
                    horizontalAlignment: Text.AlignHCenter
                    wrapMode: Text.Wrap
                }

                Label {
                    x: Theme.horizontalPageMargin
                    width: parent.width - 2 * Theme.horizontalPageMargin
                    text: "Estimated native-asset value · " + prices.fiatCurrency
                    color: Theme.secondaryColor
                    font.pixelSize: Theme.fontSizeExtraSmall
                    horizontalAlignment: Text.AlignHCenter
                }

                Label {
                    x: Theme.horizontalPageMargin
                    width: parent.width - 2 * Theme.horizontalPageMargin
                    text: prices.status
                    color: prices.lastRefreshPassed
                           ? Theme.secondaryHighlightColor : Theme.secondaryColor
                    font.pixelSize: Theme.fontSizeExtraSmall
                    horizontalAlignment: Text.AlignHCenter
                    wrapMode: Text.Wrap
                }

                Label {
                    x: Theme.horizontalPageMargin
                    width: parent.width - 2 * Theme.horizontalPageMargin
                    text: portfolio.status
                    color: portfolio.lastRefreshPassed
                           ? Theme.highlightColor : Theme.secondaryColor
                    font.pixelSize: Theme.fontSizeSmall
                    wrapMode: Text.Wrap
                }

                Label {
                    x: Theme.horizontalPageMargin
                    width: parent.width - 2 * Theme.horizontalPageMargin
                    visible: portfolio.usingCachedData || prices.usingCachedData
                    text: "Showing last-known cached data while current values are unavailable or refreshing"
                    color: Theme.secondaryColor
                    font.pixelSize: Theme.fontSizeExtraSmall
                    horizontalAlignment: Text.AlignHCenter
                    wrapMode: Text.Wrap
                }

                BackgroundItem {
                    width: parent.width
                    height: ethColumn.height + 2 * Theme.paddingLarge
                    onClicked: pageStack.push(Qt.resolvedUrl("ChainPage.qml"), {
                        chainName: "Ethereum",
                        chainSubtitle: "Ethereum mainnet",
                        address: vault.ethereumAddress,
                        balance: portfolio.ethereumBalance,
                        vault: vault,
                        portfolio: portfolio
                    })

                    Column {
                        id: ethColumn
                        x: Theme.horizontalPageMargin
                        width: parent.width - 2 * Theme.horizontalPageMargin
                        spacing: Theme.paddingSmall

                        Label {
                            width: parent.width
                            text: "Ethereum"
                            color: Theme.highlightColor
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
                            text: chainFiat(portfolio.ethereumBalance, prices.ethereumPriceValue)
                            color: Theme.secondaryHighlightColor
                            font.pixelSize: Theme.fontSizeSmall
                        }

                        Label {
                            width: parent.width
                            text: vault.ethereumAddress
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
                    width: parent.width
                    height: btcColumn.height + 2 * Theme.paddingLarge
                    onClicked: pageStack.push(Qt.resolvedUrl("ChainPage.qml"), {
                        chainName: "Bitcoin",
                        chainSubtitle: "Bitcoin mainnet · BIP84",
                        address: vault.bitcoinAddress,
                        balance: portfolio.bitcoinBalance,
                        vault: vault,
                        portfolio: portfolio
                    })

                    Column {
                        id: btcColumn
                        x: Theme.horizontalPageMargin
                        width: parent.width - 2 * Theme.horizontalPageMargin
                        spacing: Theme.paddingSmall

                        Label {
                            width: parent.width
                            text: "Bitcoin"
                            color: Theme.highlightColor
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
                            text: chainFiat(portfolio.bitcoinBalance, prices.bitcoinPriceValue)
                            color: Theme.secondaryHighlightColor
                            font.pixelSize: Theme.fontSizeSmall
                        }

                        Label {
                            width: parent.width
                            text: vault.bitcoinAddress
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
                    width: parent.width
                    height: solColumn.height + 2 * Theme.paddingLarge
                    onClicked: pageStack.push(Qt.resolvedUrl("ChainPage.qml"), {
                        chainName: "Solana",
                        chainSubtitle: "Solana mainnet · Wallet Core 4.0.27 default",
                        address: vault.solanaAddress,
                        balance: portfolio.solanaBalance,
                        vault: vault,
                        portfolio: portfolio
                    })

                    Column {
                        id: solColumn
                        x: Theme.horizontalPageMargin
                        width: parent.width - 2 * Theme.horizontalPageMargin
                        spacing: Theme.paddingSmall

                        Label {
                            width: parent.width
                            text: "Solana"
                            color: Theme.highlightColor
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
                            text: chainFiat(portfolio.solanaBalance, prices.solanaPriceValue)
                            color: Theme.secondaryHighlightColor
                            font.pixelSize: Theme.fontSizeSmall
                        }

                        Label {
                            width: parent.width
                            text: vault.solanaAddress
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

                Button {
                    anchors.horizontalCenter: parent.horizontalCenter
                    enabled: !portfolio.loading && !prices.loading
                    text: (portfolio.loading || prices.loading)
                          ? "Refreshing…"
                          : "Refresh portfolio"
                    onClicked: {
                        portfolio.refresh(vault.ethereumAddress,
                                          vault.bitcoinAddress,
                                          vault.solanaAddress)
                        prices.refresh()
                    }
                }

                Label {
                    x: Theme.horizontalPageMargin
                    width: parent.width - 2 * Theme.horizontalPageMargin
                    visible: prices.bitcoinPriceValue > 0.0
                             || prices.ethereumPriceValue > 0.0
                             || prices.solanaPriceValue > 0.0
                    text: "Market: BTC " + prices.bitcoinPrice
                          + " · ETH " + prices.ethereumPrice
                          + " · SOL " + prices.solanaPrice
                    color: Theme.secondaryColor
                    font.pixelSize: Theme.fontSizeExtraSmall
                    wrapMode: Text.Wrap
                    horizontalAlignment: Text.AlignHCenter
                }

                Button {
                    anchors.horizontalCenter: parent.horizontalCenter
                    text: "Portfolio history"
                    onClicked: pageStack.push(Qt.resolvedUrl("PortfolioHistoryPage.qml"), {
                        history: history,
                        prices: prices,
                        portfolioLabel: "Development wallet"
                    })
                }

                Button {
                    anchors.horizontalCenter: parent.horizontalCenter
                    text: "Recent activity"
                    onClicked: pageStack.push(Qt.resolvedUrl("ActivityPage.qml"), {
                        ethereumAddress: vault.ethereumAddress,
                        bitcoinAddress: vault.bitcoinAddress,
                        solanaAddress: vault.solanaAddress
                    })
                }

                Button {
                    anchors.horizontalCenter: parent.horizontalCenter
                    text: "Token holdings"
                    onClicked: pageStack.push(Qt.resolvedUrl("TokensPage.qml"), {
                        ethereumAddress: vault.ethereumAddress,
                        solanaAddress: vault.solanaAddress
                    })
                }

                Label {
                    x: Theme.horizontalPageMargin
                    width: parent.width - 2 * Theme.horizontalPageMargin
                    text: "Portfolio total currently includes native BTC, ETH and SOL only. "
                          + "Token holdings are intentionally excluded until token price "
                          + "metadata is independently validated."
                    color: Theme.secondaryColor
                    font.pixelSize: Theme.fontSizeSmall
                    wrapMode: Text.Wrap
                }

                Button {
                    anchors.horizontalCenter: parent.horizontalCenter
                    text: "Lock wallet"
                    onClicked: vault.clearSession()
                }

                Label {
                    x: Theme.horizontalPageMargin
                    width: parent.width - 2 * Theme.horizontalPageMargin
                    visible: portfolio.lastUpdated.length > 0
                    text: "Latest balance verification: " + portfolio.lastUpdated
                    color: Theme.secondaryColor
                    font.pixelSize: Theme.fontSizeExtraSmall
                    horizontalAlignment: Text.AlignHCenter
                }
            }

            SectionHeader {
                text: "Development safety"
            }

            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * Theme.horizontalPageMargin
                text: "Read-only development build. Only the public BIP39 test wallet is accepted. "
                      + "No transaction construction or broadcasting is available. "
                      + "Do not send real funds to these addresses."
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
