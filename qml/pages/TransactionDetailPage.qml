import QtQuick 2.0
import Sailfish.Silica 1.0
import org.sailfishos.sailvault 1.0

Page {
    id: page

    property string chainName
    property string identifier
    property string walletAddress
    property string fallbackSummary
    property string fallbackState
    property string fallbackTime

    TransactionDetailService {
        id: detail
    }

    AppTools {
        id: tools
    }

    Component.onCompleted: {
        detail.load(chainName, identifier, walletAddress)
    }

    SilicaFlickable {
        anchors.fill: parent
        contentHeight: content.height

        PullDownMenu {
            MenuItem {
                text: "Reload details"
                enabled: !detail.loading
                onClicked: detail.load(chainName, identifier, walletAddress)
            }
        }

        VerticalScrollDecorator { }

        Column {
            id: content
            width: parent.width
            spacing: Theme.paddingLarge

            PageHeader {
                title: chainName + " transaction"
                description: "Read-only transaction details"
            }

            BusyIndicator {
                anchors.horizontalCenter: parent.horizontalCenter
                running: detail.loading
                size: BusyIndicatorSize.Medium
            }

            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * Theme.horizontalPageMargin
                text: detail.status
                color: detail.loaded
                       ? Theme.highlightColor
                       : Theme.primaryColor
                font.pixelSize: Theme.fontSizeSmall
                wrapMode: Text.Wrap
            }

            SectionHeader {
                text: "Summary"
            }

            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * Theme.horizontalPageMargin
                text: detail.direction.length > 0
                      ? detail.direction
                      : fallbackSummary
                color: Theme.primaryColor
                font.pixelSize: Theme.fontSizeLarge
                wrapMode: Text.Wrap
            }

            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * Theme.horizontalPageMargin
                visible: detail.amount.length > 0
                text: detail.amount
                color: Theme.highlightColor
                font.pixelSize: Theme.fontSizeHuge
                wrapMode: Text.WrapAnywhere
            }

            DetailItem {
                visible: detail.transactionState.length > 0
                         || fallbackState.length > 0
                label: "Status"
                value: detail.transactionState.length > 0
                       ? detail.transactionState
                       : fallbackState
            }

            DetailItem {
                visible: detail.timeText.length > 0
                         || fallbackTime.length > 0
                label: "Time"
                value: detail.timeText.length > 0
                       ? detail.timeText
                       : fallbackTime
            }

            DetailItem {
                visible: detail.blockText.length > 0
                label: chainName === "Solana" ? "Slot" : "Block"
                value: detail.blockText
            }

            DetailItem {
                visible: detail.fee.length > 0
                label: "Network fee"
                value: detail.fee
            }

            DetailItem {
                visible: detail.method.length > 0
                label: "Method"
                value: detail.method
            }

            SectionHeader {
                visible: detail.fromAddress.length > 0
                         || detail.toAddress.length > 0
                text: "Addresses"
            }

            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * Theme.horizontalPageMargin
                visible: detail.fromAddress.length > 0
                text: "From\n" + detail.fromAddress
                color: Theme.secondaryColor
                font.pixelSize: Theme.fontSizeSmall
                wrapMode: Text.WrapAnywhere
            }

            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * Theme.horizontalPageMargin
                visible: detail.toAddress.length > 0
                text: "To\n" + detail.toAddress
                color: Theme.secondaryColor
                font.pixelSize: Theme.fontSizeSmall
                wrapMode: Text.WrapAnywhere
            }

            SectionHeader {
                text: "Transaction identifier"
            }

            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * Theme.horizontalPageMargin
                text: identifier
                color: Theme.secondaryColor
                font.pixelSize: Theme.fontSizeExtraSmall
                wrapMode: Text.WrapAnywhere
                horizontalAlignment: Text.AlignHCenter
            }

            Button {
                anchors.horizontalCenter: parent.horizontalCenter
                text: "Copy transaction identifier"
                onClicked: {
                    tools.copyText(identifier)
                    copiedLabel.visible = true
                    copiedTimer.restart()
                }
            }

            Label {
                id: copiedLabel
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * Theme.horizontalPageMargin
                visible: false
                text: "Transaction identifier copied"
                color: Theme.highlightColor
                horizontalAlignment: Text.AlignHCenter
            }

            Timer {
                id: copiedTimer
                interval: 1800
                onTriggered: copiedLabel.visible = false
            }

            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * Theme.horizontalPageMargin
                visible: detail.extraText.length > 0
                text: detail.extraText
                color: Theme.secondaryColor
                font.pixelSize: Theme.fontSizeExtraSmall
                wrapMode: Text.Wrap
            }

            SectionHeader {
                text: "Privacy"
            }

            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * Theme.horizontalPageMargin
                text: "The detail lookup sends only this public transaction identifier "
                      + "and, where needed, uses the public wallet address already known "
                      + "to SailVault. No recovery phrase or private key is involved."
                color: Theme.secondaryColor
                font.pixelSize: Theme.fontSizeSmall
                wrapMode: Text.Wrap
            }

            Item {
                width: 1
                height: Theme.paddingLarge
            }
        }
    }
}
