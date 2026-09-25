import QtQuick 2.0
import Sailfish.Silica 1.0

Page {
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
                title: "Backup phrase"
                description: "Public development test vector"
            }

            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * Theme.horizontalPageMargin
                text: "This phrase is public and contains no funds. "
                      + "The page exists only to exercise the future backup UX."
                color: Theme.secondaryColor
                font.pixelSize: Theme.fontSizeSmall
                wrapMode: Text.Wrap
            }

            Rectangle {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * Theme.horizontalPageMargin
                height: phraseLabel.height + 2 * Theme.paddingLarge
                color: Theme.rgba(Theme.highlightBackgroundColor,
                                  Theme.highlightBackgroundOpacity)

                Label {
                    id: phraseLabel
                    anchors.centerIn: parent
                    width: parent.width - 2 * Theme.paddingLarge
                    text: developmentTestMnemonic
                    font.pixelSize: Theme.fontSizeMedium
                    wrapMode: Text.Wrap
                    horizontalAlignment: Text.AlignHCenter
                }
            }

            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * Theme.horizontalPageMargin
                text: "Production recovery material must not be treated like this public test vector. "
                      + "Real-wallet backup UX will receive additional confirmation and security hardening."
                color: Theme.highlightColor
                font.pixelSize: Theme.fontSizeSmall
                wrapMode: Text.Wrap
            }
        }
    }
}
