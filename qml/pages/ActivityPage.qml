import QtQuick 2.0
import Sailfish.Silica 1.0
import org.sailfishos.sailvault 1.0

Page {
    id: page

    property string ethereumAddress
    property string bitcoinAddress
    property string solanaAddress
    property string initialChain: "All chains"

    function chainIndex(chain) {
        if (chain === "Ethereum")
            return 1
        if (chain === "Bitcoin")
            return 2
        if (chain === "Solana")
            return 3
        return 0
    }

    function selectedChain() {
        if (chainFilter.currentIndex === 1)
            return "Ethereum"
        if (chainFilter.currentIndex === 2)
            return "Bitcoin"
        if (chainFilter.currentIndex === 3)
            return "Solana"
        return "All chains"
    }

    function chainVisible(chain) {
        var selected = selectedChain()
        return selected === "All chains" || chain === selected
    }

    function selectedHasMore() {
        if (chainFilter.currentIndex === 1)
            return activity.ethereumHasMore
        if (chainFilter.currentIndex === 2)
            return activity.bitcoinHasMore
        if (chainFilter.currentIndex === 3)
            return activity.solanaHasMore
        return activity.canLoadOlder
    }

    property int filteredEntryCount: {
        var count = 0
        var list = activity.entries
        for (var i = 0; i < list.length; ++i) {
            if (chainVisible(list[i].chain))
                ++count
        }
        return count
    }

    ActivityService {
        id: activity
    }

    Component.onCompleted: {
        chainFilter.currentIndex = chainIndex(initialChain)
        activity.refresh(ethereumAddress, bitcoinAddress, solanaAddress)
    }

    SilicaFlickable {
        anchors.fill: parent
        contentHeight: content.height + Theme.paddingLarge

        PullDownMenu {
            visible: !activity.loading && !activity.loadingOlder

            MenuItem {
                text: "Refresh activity"
                onClicked: activity.refresh(ethereumAddress,
                                            bitcoinAddress,
                                            solanaAddress)
            }
        }

        VerticalScrollDecorator { }

        Column {
            id: content
            width: parent.width
            spacing: Theme.paddingLarge

            PageHeader {
                title: "Recent activity"
                description: "Paginated read-only public-chain history"
            }

            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * Theme.horizontalPageMargin
                text: activity.status
                color: activity.lastRefreshPassed
                       ? Theme.highlightColor : Theme.primaryColor
                font.pixelSize: Theme.fontSizeSmall
                wrapMode: Text.Wrap
            }

            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * Theme.horizontalPageMargin
                text: "Ethereum: " + activity.ethereumStatus
                      + "\nBitcoin: " + activity.bitcoinStatus
                      + "\nSolana: " + activity.solanaStatus
                color: Theme.secondaryColor
                font.pixelSize: Theme.fontSizeExtraSmall
                wrapMode: Text.Wrap
            }

            ComboBox {
                id: chainFilter
                width: parent.width
                label: "Show activity"

                menu: ContextMenu {
                    MenuItem { text: "All chains" }
                    MenuItem { text: "Ethereum" }
                    MenuItem { text: "Bitcoin" }
                    MenuItem { text: "Solana" }
                }
            }

            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * Theme.horizontalPageMargin
                visible: filteredEntryCount > 0
                text: filteredEntryCount + " transaction"
                      + (filteredEntryCount === 1 ? "" : "s")
                      + " shown · tap an entry for details"
                color: Theme.secondaryColor
                font.pixelSize: Theme.fontSizeExtraSmall
                horizontalAlignment: Text.AlignHCenter
                wrapMode: Text.Wrap
            }

            BusyIndicator {
                anchors.horizontalCenter: parent.horizontalCenter
                running: activity.loading || activity.loadingOlder
                size: BusyIndicatorSize.Medium
            }

            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * Theme.horizontalPageMargin
                visible: !activity.loading && filteredEntryCount === 0
                text: chainFilter.currentIndex === 0
                      ? "No recent activity was returned for these public addresses."
                      : "No recent " + selectedChain() + " activity was returned."
                color: Theme.secondaryColor
                horizontalAlignment: Text.AlignHCenter
                font.pixelSize: Theme.fontSizeSmall
                wrapMode: Text.Wrap
            }

            Repeater {
                model: activity.entries

                delegate: BackgroundItem {
                    width: content.width
                    visible: page.chainVisible(modelData.chain)
                    height: visible ? entryColumn.height + 2 * Theme.paddingMedium : 0

                    onClicked: {
                        var walletAddress = ""

                        if (modelData.chain === "Ethereum")
                            walletAddress = ethereumAddress
                        else if (modelData.chain === "Bitcoin")
                            walletAddress = bitcoinAddress
                        else if (modelData.chain === "Solana")
                            walletAddress = solanaAddress

                        pageStack.push(Qt.resolvedUrl("TransactionDetailPage.qml"), {
                            chainName: modelData.chain,
                            identifier: modelData.identifier,
                            walletAddress: walletAddress,
                            fallbackSummary: modelData.summary,
                            fallbackState: modelData.state,
                            fallbackTime: modelData.timeText
                        })
                    }

                    Column {
                        id: entryColumn
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
                                text: modelData.state
                                color: Theme.secondaryColor
                                font.pixelSize: Theme.fontSizeExtraSmall
                                horizontalAlignment: Text.AlignRight
                            }
                        }

                        Label {
                            width: parent.width
                            text: modelData.summary
                            color: Theme.primaryColor
                            font.pixelSize: Theme.fontSizeMedium
                            wrapMode: Text.Wrap
                        }

                        Label {
                            width: parent.width
                            text: modelData.timeText
                            color: Theme.secondaryColor
                            font.pixelSize: Theme.fontSizeExtraSmall
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

            Button {
                anchors.horizontalCenter: parent.horizontalCenter
                visible: activity.canLoadOlder || activity.loadingOlder
                enabled: selectedHasMore()
                         && !activity.loading
                         && !activity.loadingOlder
                text: activity.loadingOlder
                      ? "Loading older…"
                      : selectedHasMore()
                        ? "Load older activity"
                        : "No older " + selectedChain() + " activity"
                onClicked: activity.loadOlder(selectedChain())
            }

            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * Theme.horizontalPageMargin
                visible: !activity.loading
                         && !activity.loadingOlder
                         && filteredEntryCount > 0
                         && !selectedHasMore()
                text: chainFilter.currentIndex === 0
                      ? "No older page is currently available from the configured providers."
                      : "No older " + selectedChain() + " page is currently available."
                color: Theme.secondaryColor
                font.pixelSize: Theme.fontSizeExtraSmall
                horizontalAlignment: Text.AlignHCenter
                wrapMode: Text.Wrap
            }

            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * Theme.horizontalPageMargin
                visible: activity.lastUpdated.length > 0
                text: "Last activity update: " + activity.lastUpdated
                color: Theme.secondaryColor
                font.pixelSize: Theme.fontSizeExtraSmall
                horizontalAlignment: Text.AlignHCenter
            }

            SectionHeader {
                text: "Privacy"
            }

            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * Theme.horizontalPageMargin
                text: "Activity lookup sends public wallet addresses to the configured "
                      + "Ethereum explorer, Bitcoin Esplora and Solana RPC providers. "
                      + "Loading older pages uses only the selected public address(es) "
                      + "plus provider pagination cursors. No recovery phrase or private "
                      + "key enters this service."
                color: Theme.secondaryColor
                font.pixelSize: Theme.fontSizeSmall
                wrapMode: Text.Wrap
            }
        }
    }
}
