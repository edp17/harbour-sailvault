import QtQuick 2.0
import Sailfish.Silica 1.0
import org.sailfishos.sailvault 1.0

Page {
    id: page

    property string chainName
    property string chainSubtitle
    property string sourceAddress
    property string availableBalance
    property var vault
    property string ethereumRpcUrl
    property string bitcoinApiUrl
    property string solanaRpcUrl

    UnsignedTransactionService {
        id: draft
    }

    AddressBookService {
        id: addressBook
    }

    Component.onCompleted: {
        draft.configure(chainName, sourceAddress, availableBalance)
    }

    SilicaFlickable {
        anchors.fill: parent
        contentHeight: content.height + Theme.paddingLarge

        VerticalScrollDecorator { }

        Column {
            id: content
            width: parent.width
            spacing: Theme.paddingLarge

            PageHeader {
                title: "Prepare " + chainName + " transfer"
                description: "Unsigned intent · local-only review"
            }

            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * Theme.horizontalPageMargin
                text: chainName === "Ethereum"
                      ? "M52 keeps this draft local, then permits explicit M49 EIP-1559 construction and a separate published-test-wallet-only sign/verify proof. The raw signature is discarded; production signing and broadcasting remain disabled."
                      : (chainName === "Bitcoin"
                         ? "M52 preserves explicit confirmed-UTXO / fee / PSBT construction after local review. Bitcoin signing and broadcasting remain disabled."
                         : "M52 preserves the M51r2 Solana mainnet/blockhash/Wallet Core message construction flow. Solana signing and broadcasting remain disabled.")
                color: Theme.highlightColor
                font.pixelSize: Theme.fontSizeSmall
                wrapMode: Text.Wrap
            }

            SectionHeader {
                text: "From"
            }

            DetailItem {
                label: "Network"
                value: draft.network
            }

            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * Theme.horizontalPageMargin
                text: sourceAddress
                color: Theme.secondaryColor
                font.pixelSize: Theme.fontSizeSmall
                wrapMode: Text.WrapAnywhere
            }

            DetailItem {
                label: "Displayed balance"
                value: availableBalance.length > 0 ? availableBalance : "—"
            }

            SectionHeader {
                text: "Destination"
            }

            TextField {
                id: destinationField
                width: parent.width
                label: "Destination " + draft.symbol + " address"
                placeholderText: "Enter " + chainName + " address"
                inputMethodHints: Qt.ImhNoPredictiveText
                onTextChanged: draft.destinationAddress = text
            }

            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * Theme.horizontalPageMargin
                visible: destinationField.text.length > 0
                text: draft.destinationValid
                      ? "✓ Wallet Core accepts this " + chainName + " address"
                      : "✗ Not a valid " + chainName + " address"
                color: draft.destinationValid ? Theme.highlightColor : Theme.primaryColor
                font.pixelSize: Theme.fontSizeSmall
                wrapMode: Text.Wrap
            }

            SectionHeader {
                visible: addressBook.count > 0
                text: "Address book"
            }

            Repeater {
                model: addressBook.entries

                delegate: BackgroundItem {
                    width: content.width
                    height: contactColumn.height + 2 * Theme.paddingSmall
                    visible: modelData.chain === chainName
                    enabled: visible
                    onClicked: destinationField.text = modelData.address

                    Column {
                        id: contactColumn
                        x: Theme.horizontalPageMargin
                        width: parent.width - 2 * Theme.horizontalPageMargin
                        spacing: Theme.paddingSmall

                        Label {
                            width: parent.width
                            text: modelData.label
                            color: Theme.highlightColor
                            font.pixelSize: Theme.fontSizeMedium
                            truncationMode: TruncationMode.Fade
                        }

                        Label {
                            width: parent.width
                            text: modelData.address
                            color: Theme.secondaryColor
                            font.pixelSize: Theme.fontSizeExtraSmall
                            truncationMode: TruncationMode.Fade
                        }
                    }
                }
            }

            SectionHeader {
                text: "Amount"
            }

            TextField {
                id: amountField
                width: parent.width
                label: "Amount (" + draft.symbol + ")"
                placeholderText: "0.0"
                inputMethodHints: Qt.ImhFormattedNumbersOnly
                onTextChanged: draft.amountText = text
            }

            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * Theme.horizontalPageMargin
                visible: amountField.text.length > 0
                text: draft.amountValid
                      ? "✓ " + draft.normalizedAmount + " " + draft.symbol
                      : "Use a positive decimal amount with at most " + draft.decimals + " decimal places"
                color: draft.amountValid ? Theme.highlightColor : Theme.primaryColor
                font.pixelSize: Theme.fontSizeSmall
                wrapMode: Text.Wrap
            }

            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * Theme.horizontalPageMargin
                visible: draft.balanceWarning.length > 0
                text: draft.balanceWarning
                color: Theme.secondaryColor
                font.pixelSize: Theme.fontSizeSmall
                wrapMode: Text.Wrap
            }

            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * Theme.horizontalPageMargin
                text: draft.status
                color: draft.readyForReview ? Theme.highlightColor : Theme.secondaryColor
                font.pixelSize: Theme.fontSizeSmall
                wrapMode: Text.Wrap
            }

            Button {
                anchors.horizontalCenter: parent.horizontalCenter
                enabled: draft.readyForReview
                text: "Review unsigned intent"
                onClicked: pageStack.push(Qt.resolvedUrl("TransactionReviewPage.qml"), {
                    transaction: draft.reviewModel(),
                    vault: vault,
                    ethereumRpcUrl: ethereumRpcUrl,
                    bitcoinApiUrl: bitcoinApiUrl,
                    solanaRpcUrl: solanaRpcUrl
                })
            }

            SectionHeader {
                text: "Safety boundary"
            }

            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * Theme.horizontalPageMargin
                text: "This draft stays in memory and contains public transaction intent only. "
                      + "It does not access the recovery phrase/private key or contact a provider. "
                      + "Ethereum/Bitcoin/Solana network access, if requested, occurs only from the separate review page. M52's Ethereum development sign/verify also requires an explicit review-page action and the loaded published test wallet; production signing and broadcasting stay disabled."
                color: Theme.secondaryColor
                font.pixelSize: Theme.fontSizeSmall
                wrapMode: Text.Wrap
            }
        }
    }
}
