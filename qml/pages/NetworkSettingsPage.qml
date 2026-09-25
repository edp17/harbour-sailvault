import QtQuick 2.0
import Sailfish.Silica 1.0

Page {
    id: page
    property var portfolio
    property var prices

    SilicaFlickable {
        anchors.fill: parent
        contentHeight: content.height

        PullDownMenu {
            MenuItem {
                text: "Network fees"
                onClicked: pageStack.push(Qt.resolvedUrl("NetworkFeesPage.qml"), {
                    portfolio: portfolio
                })
            }

            MenuItem {
                text: "Provider health"
                onClicked: pageStack.push(Qt.resolvedUrl("NetworkHealthPage.qml"), {
                    portfolio: portfolio,
                    prices: prices
                })
            }
        }

        VerticalScrollDecorator { }

        Column {
            id: content
            width: parent.width
            spacing: Theme.paddingLarge

            PageHeader {
                title: "Network settings"
                description: "Read-only balance, activity & token providers"
            }

            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * Theme.horizontalPageMargin
                text: "Endpoints are non-sensitive preferences. Saved values are "
                      + "written immediately to SailVault's persistent INI configuration "
                      + "and restored after restart. Only HTTPS endpoints are accepted."
                color: Theme.secondaryColor
                font.pixelSize: Theme.fontSizeSmall
                wrapMode: Text.Wrap
            }

            TextField {
                id: ethereumField
                width: parent.width
                label: "Ethereum JSON-RPC"
                text: portfolio ? portfolio.ethereumRpcUrl : ""
                inputMethodHints: Qt.ImhUrlCharactersOnly
            }

            TextField {
                id: ethereumExplorerField
                width: parent.width
                label: "Ethereum explorer API (activity + tokens)"
                text: portfolio ? portfolio.ethereumExplorerUrl : ""
                inputMethodHints: Qt.ImhUrlCharactersOnly
            }

            TextField {
                id: bitcoinField
                width: parent.width
                label: "Bitcoin Esplora API base"
                text: portfolio ? portfolio.bitcoinApiUrl : ""
                inputMethodHints: Qt.ImhUrlCharactersOnly
            }

            TextField {
                id: solanaField
                width: parent.width
                label: "Solana JSON-RPC"
                text: portfolio ? portfolio.solanaRpcUrl : ""
                inputMethodHints: Qt.ImhUrlCharactersOnly
            }

            TextField {
                id: solanaTokenField
                width: parent.width
                label: "Solana token-account RPC"
                text: portfolio ? portfolio.solanaTokenRpcUrl : ""
                inputMethodHints: Qt.ImhUrlCharactersOnly
            }

            Button {
                anchors.horizontalCenter: parent.horizontalCenter
                text: "Save endpoints"
                onClicked: {
                    portfolio.ethereumRpcUrl = ethereumField.text
                    portfolio.ethereumExplorerUrl = ethereumExplorerField.text
                    portfolio.bitcoinApiUrl = bitcoinField.text
                    portfolio.solanaRpcUrl = solanaField.text
                    portfolio.solanaTokenRpcUrl = solanaTokenField.text
                    ethereumField.text = portfolio.ethereumRpcUrl
                    ethereumExplorerField.text = portfolio.ethereumExplorerUrl
                    bitcoinField.text = portfolio.bitcoinApiUrl
                    solanaField.text = portfolio.solanaRpcUrl
                    solanaTokenField.text = portfolio.solanaTokenRpcUrl
                }
            }

            Button {
                anchors.horizontalCenter: parent.horizontalCenter
                text: "Restore default endpoints"
                onClicked: {
                    portfolio.resetEndpoints()
                    ethereumField.text = portfolio.ethereumRpcUrl
                    ethereumExplorerField.text = portfolio.ethereumExplorerUrl
                    bitcoinField.text = portfolio.bitcoinApiUrl
                    solanaField.text = portfolio.solanaRpcUrl
                    solanaTokenField.text = portfolio.solanaTokenRpcUrl
                }
            }

            SectionHeader {
                text: "Connectivity"
            }

            TextSwitch {
                width: parent.width
                text: "Offline mode"
                description: "Block SailVault provider requests and use only local or cached public data."
                checked: portfolio ? portfolio.offlineMode : false

                onClicked: {
                    if (portfolio)
                        portfolio.offlineMode = checked
                }
            }

            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * Theme.horizontalPageMargin
                text: "When Offline mode is enabled, SailVault will not request balances, "
                      + "prices, activity, tokens, transaction details, fee estimates or "
                      + "provider-health data. Offline QR codes, Address book, cached balances "
                      + "and local portfolio history remain available."
                color: portfolio && portfolio.offlineMode
                       ? Theme.highlightColor : Theme.secondaryColor
                font.pixelSize: Theme.fontSizeSmall
                wrapMode: Text.Wrap
            }

            SectionHeader {
                text: "Refresh policy"
            }

            TextSwitch {
                width: parent.width
                text: "Automatically refresh portfolios"
                description: "Refresh the stored wallet after unlock and a saved watch-only portfolio when opened."
                checked: portfolio ? portfolio.autoRefreshAfterUnlock : true

                onClicked: {
                    if (portfolio)
                        portfolio.autoRefreshAfterUnlock = checked
                }
            }

            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * Theme.horizontalPageMargin
                text: "When enabled, opening a usable portfolio automatically sends its "
                      + "configured public addresses to the selected balance providers and "
                      + "queries public market prices. Disable this for fully manual refresh."
                color: Theme.secondaryColor
                font.pixelSize: Theme.fontSizeSmall
                wrapMode: Text.Wrap
            }

            SectionHeader {
                text: "Fiat valuation"
            }

            ComboBox {
                id: currencyBox
                property bool settingsLoaded: false
                width: parent.width
                label: "Display currency"

                menu: ContextMenu {
                    MenuItem {
                        text: "GBP"
                        onClicked: if (prices) prices.fiatCurrency = "GBP"
                    }
                    MenuItem {
                        text: "USD"
                        onClicked: if (prices) prices.fiatCurrency = "USD"
                    }
                    MenuItem {
                        text: "EUR"
                        onClicked: if (prices) prices.fiatCurrency = "EUR"
                    }
                }

                Component.onCompleted: {
                    if (!prices)
                        return
                    if (prices.fiatCurrency === "USD")
                        currentIndex = 1
                    else if (prices.fiatCurrency === "EUR")
                        currentIndex = 2
                    else
                        currentIndex = 0

                    settingsLoaded = true
                }

                onCurrentIndexChanged: {
                    if (!prices || !settingsLoaded)
                        return
                    var currency = "GBP"
                    if (currentIndex === 1) currency = "USD"
                    else if (currentIndex === 2) currency = "EUR"
                    if (prices.fiatCurrency !== currency)
                        prices.fiatCurrency = currency
                }

            }

            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * Theme.horizontalPageMargin
                text: "BTC, ETH and SOL market prices use Kraken's public ticker API. "
                      + "No API key or trading account is used."
                color: Theme.secondaryColor
                font.pixelSize: Theme.fontSizeSmall
                wrapMode: Text.Wrap
            }

            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * Theme.horizontalPageMargin
                text: "Solana token-account discovery uses a separate endpoint because "
                      + "some general-purpose public RPC providers reject "
                      + "getTokenAccountsByOwner. The development default is Solana's "
                      + "rate-limited public mainnet RPC."
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
                text: "A provider that receives a balance, activity or token request can "
                      + "observe the public wallet address being queried and your network metadata. "
                      + "For stronger privacy, use endpoints you control."
                color: Theme.highlightColor
                font.pixelSize: Theme.fontSizeSmall
                wrapMode: Text.Wrap
            }
        }
    }
}
