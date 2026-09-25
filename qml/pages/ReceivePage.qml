import QtQuick 2.0
import Sailfish.Silica 1.0
import org.sailfishos.sailvault 1.0

Page {
    id: page

    property string chainName
    property string chainSubtitle
    property string address
    property string pageTitle
    property bool developmentAddress: false

    AppTools {
        id: tools
    }

    Timer {
        id: copiedTimer
        interval: 1800
        onTriggered: copiedLabel.visible = false
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
                title: pageTitle.length > 0
                       ? pageTitle
                       : chainName + " address QR"
                description: chainSubtitle
            }

            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * Theme.horizontalPageMargin
                text: "Offline address QR"
                color: Theme.highlightColor
                font.pixelSize: Theme.fontSizeLarge
                horizontalAlignment: Text.AlignHCenter
            }

            Image {
                id: qrImage
                anchors.horizontalCenter: parent.horizontalCenter
                width: Math.min(
                           parent.width - 2 * Theme.horizontalPageMargin,
                           492)
                height: width
                sourceSize.width: 492
                sourceSize.height: 492
                smooth: false
                cache: true
                fillMode: Image.PreserveAspectFit
                source: address.length > 0
                        ? "image://sailvaultqr/" + address
                        : ""
            }

            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * Theme.horizontalPageMargin
                text: address
                color: Theme.primaryColor
                font.pixelSize: Theme.fontSizeSmall
                wrapMode: Text.WrapAnywhere
                horizontalAlignment: Text.AlignHCenter
            }

            Button {
                anchors.horizontalCenter: parent.horizontalCenter
                text: "Copy address"
                onClicked: {
                    tools.copyText(address)
                    copiedLabel.visible = true
                    copiedTimer.restart()
                }
            }

            Label {
                id: copiedLabel
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * Theme.horizontalPageMargin
                visible: false
                text: "Address copied to clipboard"
                color: Theme.highlightColor
                font.pixelSize: Theme.fontSizeSmall
                horizontalAlignment: Text.AlignHCenter
            }

            SectionHeader {
                text: "QR contents"
            }

            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * Theme.horizontalPageMargin
                text: "The QR code contains only the plain public "
                      + chainName + " address shown above. It contains no "
                      + "amount, token, memo, private key or recovery phrase."
                color: Theme.secondaryColor
                font.pixelSize: Theme.fontSizeSmall
                wrapMode: Text.Wrap
            }

            SectionHeader {
                text: "Offline & private"
            }

            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * Theme.horizontalPageMargin
                text: "SailVault generates this QR code locally on the device. "
                      + "Displaying or copying it makes no network request."
                color: Theme.secondaryColor
                font.pixelSize: Theme.fontSizeSmall
                wrapMode: Text.Wrap
            }

            SectionHeader {
                text: "Safety"
            }

            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * Theme.horizontalPageMargin
                text: developmentAddress
                      ? "This is the public development wallet on the Wallet Core "
                        + "4.0.27 compatibility baseline. Do not send real funds "
                        + "to this development-wallet address."
                      : "The QR code faithfully represents the public address shown "
                        + "above, but SailVault cannot determine who controls an "
                        + "arbitrary public address. Verify the destination or ownership "
                        + "independently before relying on it."
                color: Theme.secondaryColor
                font.pixelSize: Theme.fontSizeSmall
                wrapMode: Text.Wrap
            }
        }
    }
}
