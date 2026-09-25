import QtQuick 2.0
import Sailfish.Silica 1.0

Dialog {
    id: dialog

    property var watch
    property string initialLabel
    property string initialEthereumAddress
    property string initialBitcoinAddress
    property string initialSolanaAddress
    property bool editingExisting: false

    function fieldsValid() {
        if (!watch)
            return false

        var eth = ethereumField.text.trim()
        var btc = bitcoinField.text.trim()
        var sol = solanaField.text.trim()

        if (eth.length === 0 && btc.length === 0 && sol.length === 0)
            return false

        if (eth.length > 0 && !watch.validateEthereum(eth))
            return false

        if (btc.length > 0 && !watch.validateBitcoin(btc))
            return false

        if (sol.length > 0 && !watch.validateSolana(sol))
            return false

        return true
    }

    canAccept: fieldsValid()

    onAccepted: {
        watch.saveProfile(labelField.text,
                          ethereumField.text,
                          bitcoinField.text,
                          solanaField.text)
    }

    SilicaFlickable {
        anchors.fill: parent
        contentHeight: content.height

        VerticalScrollDecorator { }

        Column {
            id: content
            width: parent.width
            spacing: Theme.paddingLarge

            DialogHeader {
                title: editingExisting
                       ? "Edit watch-only profile"
                       : "Add watch-only profile"
                acceptText: editingExisting ? "Save" : "Add"
            }

            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * Theme.horizontalPageMargin
                text: "Enter any combination of Ethereum, Bitcoin and Solana "
                      + "public addresses. Every non-empty address is validated "
                      + "locally by Wallet Core."
                color: Theme.highlightColor
                font.pixelSize: Theme.fontSizeSmall
                wrapMode: Text.Wrap
            }

            TextField {
                id: labelField
                width: parent.width
                label: "Label"
                placeholderText: "Watch-only"
                text: initialLabel

                EnterKey.iconSource: "image://theme/icon-m-enter-next"
                EnterKey.onClicked: ethereumField.focus = true
            }

            TextField {
                id: ethereumField
                width: parent.width
                label: "Ethereum address"
                placeholderText: "0x…"
                text: initialEthereumAddress
                inputMethodHints: Qt.ImhNoPredictiveText

                EnterKey.iconSource: "image://theme/icon-m-enter-next"
                EnterKey.onClicked: bitcoinField.focus = true
            }

            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * Theme.horizontalPageMargin
                visible: ethereumField.text.length > 0
                text: watch && watch.validateEthereum(ethereumField.text)
                      ? "Valid Ethereum address"
                      : "Not a valid Ethereum address"
                color: watch && watch.validateEthereum(ethereumField.text)
                       ? Theme.highlightColor
                       : Theme.primaryColor
                font.pixelSize: Theme.fontSizeExtraSmall
                wrapMode: Text.Wrap
            }

            TextField {
                id: bitcoinField
                width: parent.width
                label: "Bitcoin address"
                placeholderText: "bc1…"
                text: initialBitcoinAddress
                inputMethodHints: Qt.ImhNoPredictiveText

                EnterKey.iconSource: "image://theme/icon-m-enter-next"
                EnterKey.onClicked: solanaField.focus = true
            }

            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * Theme.horizontalPageMargin
                visible: bitcoinField.text.length > 0
                text: watch && watch.validateBitcoin(bitcoinField.text)
                      ? "Valid Bitcoin address"
                      : "Not a valid Bitcoin address"
                color: watch && watch.validateBitcoin(bitcoinField.text)
                       ? Theme.highlightColor
                       : Theme.primaryColor
                font.pixelSize: Theme.fontSizeExtraSmall
                wrapMode: Text.Wrap
            }

            TextField {
                id: solanaField
                width: parent.width
                label: "Solana address"
                placeholderText: "Base58 public address"
                text: initialSolanaAddress
                inputMethodHints: Qt.ImhNoPredictiveText

                EnterKey.iconSource: "image://theme/icon-m-enter-close"
                EnterKey.onClicked: focus = false
            }

            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * Theme.horizontalPageMargin
                visible: solanaField.text.length > 0
                text: watch && watch.validateSolana(solanaField.text)
                      ? "Valid Solana address"
                      : "Not a valid Solana address"
                color: watch && watch.validateSolana(solanaField.text)
                       ? Theme.highlightColor
                       : Theme.primaryColor
                font.pixelSize: Theme.fontSizeExtraSmall
                wrapMode: Text.Wrap
            }

            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * Theme.horizontalPageMargin
                visible: !fieldsValid()
                text: "At least one valid public address is required."
                color: Theme.secondaryColor
                font.pixelSize: Theme.fontSizeExtraSmall
                wrapMode: Text.Wrap
            }

            Item {
                width: 1
                height: Theme.paddingLarge
            }
        }
    }
}
