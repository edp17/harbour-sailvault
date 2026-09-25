import QtQuick 2.0
import Sailfish.Silica 1.0

Dialog {
    id: dialog

    property var addressBook
    property int editIndex: -1
    property string initialLabel
    property string initialChain: "Ethereum"
    property string initialAddress

    function selectedChain() {
        if (chainBox.currentIndex === 1)
            return "Bitcoin"
        if (chainBox.currentIndex === 2)
            return "Solana"
        return "Ethereum"
    }

    canAccept: addressBook
               && labelField.text.trim().length > 0
               && addressBook.validateAddress(
                   selectedChain(), addressField.text)

    onOpened: {
        if (initialChain === "Bitcoin")
            chainBox.currentIndex = 1
        else if (initialChain === "Solana")
            chainBox.currentIndex = 2
        else
            chainBox.currentIndex = 0
    }

    onAccepted: {
        addressBook.saveContact(editIndex,
                                labelField.text,
                                selectedChain(),
                                addressField.text)
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
                title: editIndex >= 0
                       ? "Edit address"
                       : "Add address"
                acceptText: editIndex >= 0 ? "Save" : "Add"
            }

            TextField {
                id: labelField
                width: parent.width
                label: "Label"
                placeholderText: "Name or purpose"
                text: initialLabel
                inputMethodHints: Qt.ImhNoPredictiveText

                EnterKey.iconSource: "image://theme/icon-m-enter-next"
                EnterKey.onClicked: addressField.focus = true
            }

            ComboBox {
                id: chainBox
                width: parent.width
                label: "Chain"

                menu: ContextMenu {
                    MenuItem { text: "Ethereum" }
                    MenuItem { text: "Bitcoin" }
                    MenuItem { text: "Solana" }
                }
            }

            TextField {
                id: addressField
                width: parent.width
                label: "Public address"
                placeholderText: chainBox.currentIndex === 0
                                 ? "0x…"
                                 : chainBox.currentIndex === 1
                                   ? "bc1…"
                                   : "Base58 public address"
                text: initialAddress
                inputMethodHints: Qt.ImhNoPredictiveText

                EnterKey.iconSource: "image://theme/icon-m-enter-close"
                EnterKey.onClicked: focus = false
            }

            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * Theme.horizontalPageMargin
                visible: addressField.text.length > 0
                text: addressBook
                      && addressBook.validateAddress(
                          selectedChain(), addressField.text)
                      ? "Valid public address"
                      : "Not a valid address for this chain"
                color: addressBook
                       && addressBook.validateAddress(
                           selectedChain(), addressField.text)
                       ? Theme.highlightColor
                       : Theme.primaryColor
                font.pixelSize: Theme.fontSizeExtraSmall
                wrapMode: Text.Wrap
            }

            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * Theme.horizontalPageMargin
                visible: labelField.text.trim().length === 0
                text: "A label is required."
                color: Theme.secondaryColor
                font.pixelSize: Theme.fontSizeExtraSmall
            }

            Item {
                width: 1
                height: Theme.paddingLarge
            }
        }
    }
}
