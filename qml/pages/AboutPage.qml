import QtQuick 2.0
import Sailfish.Silica 1.0
import org.sailfishos.sailvault 1.0

Page {
    id: page

    AppTools {
        id: tools
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
                title: "About SailVault"
                description: "Native multi-chain wallet for Sailfish OS"
            }

            SectionHeader {
                text: "Build"
            }

            DetailItem {
                label: "Version"
                value: tools.packageVersion
            }

            DetailItem {
                label: "Milestone"
                value: tools.milestone + " · " + tools.buildLabel
            }

            DetailItem {
                label: "Wallet Core"
                value: tools.walletCoreVersion + " compatibility baseline"
            }

            SectionHeader {
                text: "Architecture"
            }

            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * Theme.horizontalPageMargin
                text: "SailVault is a native Sailfish application: Silica/QML for the interface, "
                      + "a C++ application layer, Trust Wallet Core for public wallet primitives, "
                      + "QNetworkAccessManager for read-only providers and Sailfish Secrets for "
                      + "development recovery material."
                color: Theme.secondaryColor
                font.pixelSize: Theme.fontSizeSmall
                wrapMode: Text.Wrap
            }

            SectionHeader {
                text: "Security status"
            }

            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * Theme.horizontalPageMargin
                text: "This development build remains read-only. Transaction construction, signing "
                      + "and broadcasting are intentionally unavailable. Never use a personal recovery "
                      + "phrase or real funds with the current Wallet Core " + tools.walletCoreVersion + " compatibility baseline."
                color: Theme.highlightColor
                font.pixelSize: Theme.fontSizeSmall
                wrapMode: Text.Wrap
            }

            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * Theme.horizontalPageMargin
                text: "The development wallet accepts only the published BIP39 test vector. Public "
                      + "addresses, cached balances, prices and read-only history are non-secret data."
                color: Theme.secondaryColor
                font.pixelSize: Theme.fontSizeSmall
                wrapMode: Text.Wrap
            }

            SectionHeader {
                text: "Release state"
            }

            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * Theme.horizontalPageMargin
                text: "This build is " + tools.buildLabel + ". "
                      + "Automatic device checks are available from Settings → Release readiness."
                color: Theme.secondaryColor
                font.pixelSize: Theme.fontSizeSmall
                wrapMode: Text.Wrap
            }

            SectionHeader {
                text: "Project"
            }

            DetailItem {
                label: "Package"
                value: "harbour-sailvault"
            }

            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * Theme.horizontalPageMargin
                text: "github.com/edp17/harbour-sailvault"
                color: Theme.secondaryColor
                font.pixelSize: Theme.fontSizeExtraSmall
                wrapMode: Text.WrapAnywhere
                horizontalAlignment: Text.AlignHCenter
            }

            DetailItem {
                label: "License"
                value: "BSD 3-Clause"
            }

            Item {
                width: 1
                height: Theme.paddingLarge
            }
        }
    }
}
