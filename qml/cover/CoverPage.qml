import QtQuick 2.0
import Sailfish.Silica 1.0
import org.sailfishos.sailvault 1.0

CoverBackground {
    AppTools {
        id: tools
    }

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
            text: "Milestone " + tools.milestone
            color: Theme.secondaryColor
            font.pixelSize: Theme.fontSizeSmall
        }

        Label {
            width: parent.width
            horizontalAlignment: Text.AlignHCenter
            text: tools.buildLabel
            color: Theme.highlightColor
            font.pixelSize: Theme.fontSizeSmall
            wrapMode: Text.Wrap
        }
    }
}
