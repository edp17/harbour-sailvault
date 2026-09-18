import QtQuick 2.0
import Sailfish.Silica 1.0

CoverBackground {
    Column {
        anchors.centerIn: parent
        width: parent.width - 2 * Theme.paddingLarge
        spacing: Theme.paddingMedium

        Label {
            anchors.horizontalCenter: parent.horizontalCenter
            text: "SailVault"
            font.pixelSize: Theme.fontSizeLarge
        }

        Label {
            anchors.horizontalCenter: parent.horizontalCenter
            text: "Milestone 1"
            color: Theme.secondaryColor
            font.pixelSize: Theme.fontSizeSmall
        }

        Label {
            width: parent.width
            horizontalAlignment: Text.AlignHCenter
            text: "Wallet Core\nport probe"
            color: Theme.highlightColor
            wrapMode: Text.Wrap
        }
    }
}
