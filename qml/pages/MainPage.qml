import QtQuick 2.0
import Sailfish.Silica 1.0
import org.sailfishos.sailvault 1.0

Page {
    id: page

    WalletCoreProbe {
        id: probe
        Component.onCompleted: run()
    }

    SilicaFlickable {
        anchors.fill: parent
        contentHeight: content.height

        VerticalScrollDecorator { }

        Column {
            id: content
            width: parent.width
            spacing: Theme.paddingLarge

            PageHeader {
                title: "SailVault"
                description: "Milestone 1 · Wallet Core port"
            }

            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * Theme.horizontalPageMargin
                text: probe.summary.length ? probe.summary : "Running self-test…"
                color: probe.allPassed ? Theme.highlightColor : Theme.primaryColor
                font.pixelSize: Theme.fontSizeLarge
                wrapMode: Text.Wrap
            }

            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * Theme.horizontalPageMargin
                text: probe.buildInfo
                color: Theme.secondaryColor
                font.pixelSize: Theme.fontSizeSmall
                wrapMode: Text.Wrap
            }

            SectionHeader {
                text: "Deterministic offline checks"
            }

            Repeater {
                model: probe.results

                delegate: Item {
                    width: content.width
                    height: resultColumn.height + Theme.paddingMedium

                    Column {
                        id: resultColumn
                        x: Theme.horizontalPageMargin
                        width: parent.width - 2 * Theme.horizontalPageMargin
                        spacing: Theme.paddingSmall

                        Label {
                            width: parent.width
                            text: (modelData.passed ? "✓ PASS · " : "✗ FAIL · ") + modelData.name
                            color: modelData.passed ? Theme.highlightColor : Theme.primaryColor
                            font.pixelSize: Theme.fontSizeMedium
                            wrapMode: Text.Wrap
                        }

                        Label {
                            width: parent.width
                            text: modelData.detail
                            color: Theme.secondaryColor
                            font.pixelSize: Theme.fontSizeSmall
                            wrapMode: Text.WrapAnywhere
                        }

                        Label {
                            width: parent.width
                            visible: modelData.expected && modelData.expected.length > 0
                            text: "Expected: " + modelData.expected
                            color: Theme.secondaryHighlightColor
                            font.pixelSize: Theme.fontSizeExtraSmall
                            wrapMode: Text.WrapAnywhere
                        }
                    }
                }
            }

            Button {
                anchors.horizontalCenter: parent.horizontalCenter
                text: "Run self-test again"
                onClicked: probe.run()
            }

            SectionHeader {
                text: "Safety"
            }

            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * Theme.horizontalPageMargin
                text: "Milestone 1 is a porting probe, not a usable wallet. "
                      + "It uses only the public “abandon … about” test mnemonic. "
                      + "Do not import a real recovery phrase and do not send funds to addresses shown by this build."
                color: Theme.secondaryColor
                wrapMode: Text.Wrap
            }

            Item {
                width: 1
                height: Theme.paddingLarge
            }
        }
    }
}
