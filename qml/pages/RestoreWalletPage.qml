import QtQuick 2.0
import Sailfish.Silica 1.0

Page {
    id: page
    property var vault
    property string developmentTestMnemonic

    SilicaFlickable {
        anchors.fill: parent
        contentHeight: content.height

        VerticalScrollDecorator { }

        Column {
            id: content
            width: parent.width
            spacing: Theme.paddingLarge

            PageHeader {
                title: "Restore wallet"
                description: "Development-safe restore test"
            }

            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * Theme.horizontalPageMargin
                text: "This development build accepts only the published BIP39 test phrase. "
                      + "Any other phrase is rejected before Sailfish Secrets is written."
                color: Theme.secondaryColor
                font.pixelSize: Theme.fontSizeSmall
                wrapMode: Text.Wrap
            }

            TextArea {
                id: phraseField
                width: parent.width
                label: "Recovery phrase"
                placeholderText: "Enter the public development test phrase"
                wrapMode: TextEdit.Wrap
            }

            Button {
                anchors.horizontalCenter: parent.horizontalCenter
                text: "Insert public test phrase"
                onClicked: phraseField.text = developmentTestMnemonic
            }

            Button {
                anchors.horizontalCenter: parent.horizontalCenter
                text: "Restore development wallet"
                enabled: phraseField.text.length > 0
                onClicked: {
                    vault.restoreDevelopmentWallet(phraseField.text)
                    phraseField.text = ""
                    if (vault.walletLoaded)
                        pageStack.pop()
                }
            }

            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * Theme.horizontalPageMargin
                text: vault ? vault.status : ""
                color: vault && vault.lastOperationPassed
                       ? Theme.highlightColor : Theme.primaryColor
                font.pixelSize: Theme.fontSizeSmall
                wrapMode: Text.Wrap
            }

            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * Theme.horizontalPageMargin
                text: "Never type a personal recovery phrase into this build."
                color: Theme.highlightColor
                font.pixelSize: Theme.fontSizeSmall
                wrapMode: Text.Wrap
            }
        }
    }
}
