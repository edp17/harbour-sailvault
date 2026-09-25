import QtQuick 2.0
import Sailfish.Silica 1.0
import org.sailfishos.sailvault 1.0

Page {
    id: page

    property var vault

    AddressBookService {
        id: addressBook
    }

    AppTools {
        id: tools
    }

    PortfolioService {
        id: cacheReader
    }

    WatchOnlyService {
        id: watchContext
    }

    PriceService {
        id: cachedPrices
    }

    property int cacheRevision: 0

    function cachedBalanceFor(chain, address) {
        var revision = cacheRevision
        return cacheReader.cachedBalance(chain, address)
    }

    function cachedUpdatedFor(chain, address) {
        var revision = cacheRevision
        return cacheReader.cachedBalanceUpdated(chain, address)
    }

    function nativePriceFor(chain) {
        if (chain === "Ethereum")
            return cachedPrices.ethereumPriceValue
        if (chain === "Bitcoin")
            return cachedPrices.bitcoinPriceValue
        if (chain === "Solana")
            return cachedPrices.solanaPriceValue
        return 0.0
    }

    function cachedFiatFor(chain, balanceText) {
        var amount = parseFloat(balanceText)
        var price = nativePriceFor(chain)
        if (isNaN(amount) || price <= 0.0)
            return ""
        return cachedPrices.formatFiat(amount * price)
    }

    function addressesMatch(chain, left, right) {
        if (!left || !right || left.length === 0 || right.length === 0)
            return false
        return chain === "Ethereum"
               ? left.toLowerCase() === right.toLowerCase()
               : left === right
    }

    function matchesDevelopmentAddress(chain, address) {
        if (!vault || !vault.walletLoaded)
            return false
        if (chain === "Ethereum")
            return addressesMatch(chain, address, vault.ethereumAddress)
        if (chain === "Bitcoin")
            return addressesMatch(chain, address, vault.bitcoinAddress)
        if (chain === "Solana")
            return addressesMatch(chain, address, vault.solanaAddress)
        return false
    }

    function matchesWatchAddress(chain, address) {
        if (!watchContext.hasProfile)
            return false
        if (chain === "Ethereum")
            return addressesMatch(chain, address, watchContext.ethereumAddress)
        if (chain === "Bitcoin")
            return addressesMatch(chain, address, watchContext.bitcoinAddress)
        if (chain === "Solana")
            return addressesMatch(chain, address, watchContext.solanaAddress)
        return false
    }

    function activityEthereumFor(chain, address) {
        if (matchesDevelopmentAddress(chain, address))
            return vault.ethereumAddress
        if (matchesWatchAddress(chain, address))
            return watchContext.ethereumAddress
        return chain === "Ethereum" ? address : ""
    }

    function activityBitcoinFor(chain, address) {
        if (matchesDevelopmentAddress(chain, address))
            return vault.bitcoinAddress
        if (matchesWatchAddress(chain, address))
            return watchContext.bitcoinAddress
        return chain === "Bitcoin" ? address : ""
    }

    function activitySolanaFor(chain, address) {
        if (matchesDevelopmentAddress(chain, address))
            return vault.solanaAddress
        if (matchesWatchAddress(chain, address))
            return watchContext.solanaAddress
        return chain === "Solana" ? address : ""
    }

    onStatusChanged: {
        if (status === PageStatus.Active) {
            watchContext.reload()
            cacheRevision += 1
        }
    }

    RemorsePopup {
        id: remorse
    }

    function openAddDialog() {
        pageStack.push(Qt.resolvedUrl("AddressBookEditDialog.qml"), {
            addressBook: addressBook,
            editIndex: -1,
            initialLabel: "",
            initialChain: "Ethereum",
            initialAddress: ""
        })
    }

    function openEditDialog(index) {
        if (index < 0 || index >= addressBook.entries.length)
            return

        var item = addressBook.entries[index]

        pageStack.push(Qt.resolvedUrl("AddressBookEditDialog.qml"), {
            addressBook: addressBook,
            editIndex: index,
            initialLabel: item.label,
            initialChain: item.chain,
            initialAddress: item.address
        })
    }

    SilicaFlickable {
        anchors.fill: parent
        contentHeight: content.height

        PullDownMenu {
            MenuItem {
                text: "Add address"
                onClicked: openAddDialog()
            }
        }

        VerticalScrollDecorator { }

        Column {
            id: content
            width: parent.width
            spacing: Theme.paddingLarge

            PageHeader {
                title: "Address book"
                description: "Validated public addresses · cached balances shown offline"
            }

            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * Theme.horizontalPageMargin
                visible: addressBook.count === 0
                text: "No saved addresses yet.\nPull down to add a public address."
                color: Theme.secondaryColor
                horizontalAlignment: Text.AlignHCenter
                font.pixelSize: Theme.fontSizeSmall
                wrapMode: Text.Wrap
            }

            Repeater {
                model: addressBook.entries

                delegate: ListItem {
                    id: contactItem
                    width: content.width
                    contentHeight: contactColumn.height
                                   + 2 * Theme.paddingMedium

                    property string cachedBalanceText:
                        page.cachedBalanceFor(modelData.chain, modelData.address)
                    property string cachedUpdatedText:
                        page.cachedUpdatedFor(modelData.chain, modelData.address)
                    property string cachedFiatText:
                        page.cachedFiatFor(modelData.chain, cachedBalanceText)

                    menu: ContextMenu {
                        MenuItem {
                            text: "Edit"
                            onClicked: openEditDialog(index)
                        }

                        MenuItem {
                            text: "Copy address"
                            onClicked: {
                                tools.copyText(modelData.address)
                                copiedNotice.text =
                                    modelData.label + " address copied"
                                copiedNotice.visible = true
                                copiedTimer.restart()
                            }
                        }

                        MenuItem {
                            text: "Delete"
                            onClicked: remorse.execute(
                                "Deleting " + modelData.label,
                                function() {
                                    addressBook.removeContact(index)
                                })
                        }
                    }

                    onClicked: pageStack.push(
                        Qt.resolvedUrl("PublicAddressPage.qml"), {
                            chainName: modelData.chain,
                            chainSubtitle: modelData.chain + " mainnet",
                            labelText: modelData.label,
                            address: modelData.address,
                            balanceText: "",
                            fiatText: "",
                            developmentAddress: false,
                            activityEthereumAddress:
                                page.activityEthereumFor(modelData.chain, modelData.address),
                            activityBitcoinAddress:
                                page.activityBitcoinFor(modelData.chain, modelData.address),
                            activitySolanaAddress:
                                page.activitySolanaFor(modelData.chain, modelData.address)
                        })

                    Column {
                        id: contactColumn
                        x: Theme.horizontalPageMargin
                        width: parent.width - 2 * Theme.horizontalPageMargin
                        spacing: Theme.paddingSmall

                        Row {
                            width: parent.width
                            spacing: Theme.paddingMedium

                            Label {
                                width: parent.width * 0.62
                                text: modelData.label
                                color: contactItem.highlighted
                                       ? Theme.highlightColor
                                       : Theme.primaryColor
                                font.pixelSize: Theme.fontSizeMedium
                                truncationMode: TruncationMode.Fade
                            }

                            Label {
                                width: parent.width * 0.33
                                text: modelData.chain
                                color: contactItem.highlighted
                                       ? Theme.highlightColor
                                       : Theme.secondaryColor
                                font.pixelSize: Theme.fontSizeSmall
                                horizontalAlignment: Text.AlignRight
                            }
                        }

                        Label {
                            width: parent.width
                            text: modelData.address
                            color: Theme.secondaryColor
                            font.pixelSize: Theme.fontSizeExtraSmall
                            elide: Text.ElideMiddle
                        }

                        Label {
                            width: parent.width
                            visible: contactItem.cachedBalanceText.length > 0
                            text: contactItem.cachedFiatText.length > 0
                                  ? contactItem.cachedBalanceText + " · " + contactItem.cachedFiatText
                                  : contactItem.cachedBalanceText
                            color: Theme.secondaryHighlightColor
                            font.pixelSize: Theme.fontSizeSmall
                            wrapMode: Text.Wrap
                        }

                        Label {
                            width: parent.width
                            visible: contactItem.cachedUpdatedText.length > 0
                            text: "Cached " + contactItem.cachedUpdatedText
                            color: Theme.secondaryColor
                            font.pixelSize: Theme.fontSizeExtraSmall
                            wrapMode: Text.Wrap
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
            }

            Timer {
                id: copiedTimer
                interval: 1800
                onTriggered: copiedNotice.visible = false
            }

            SectionHeader {
                text: "Safety"
            }

            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * Theme.horizontalPageMargin
                text: "Wallet Core validates every saved address, but a valid address "
                      + "can still belong to the wrong person. Verify any future "
                      + "transaction destination independently."
                color: Theme.secondaryColor
                font.pixelSize: Theme.fontSizeSmall
                wrapMode: Text.Wrap
            }

            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * Theme.horizontalPageMargin
                text: "Tap an entry for its public-address details, offline QR "
                      + "and read-only explorer actions. Long-press it for Edit, "
                      + "Copy and Delete."
                color: Theme.secondaryColor
                font.pixelSize: Theme.fontSizeSmall
                wrapMode: Text.Wrap
            }

            Item {
                width: 1
                height: Theme.paddingLarge
            }

            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * Theme.horizontalPageMargin
                visible: addressBook.count > 0
                text: "Cached balances and cached fiat values are read locally. "
                      + "Open an address and pull down to refresh that public balance."
                color: Theme.secondaryColor
                font.pixelSize: Theme.fontSizeSmall
                horizontalAlignment: Text.AlignHCenter
                wrapMode: Text.Wrap
            }
        }
    }
}
