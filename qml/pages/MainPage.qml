import QtQuick 2.0
import Sailfish.Silica 1.0
import org.sailfishos.sailvault 1.0

Page {
    id: page

    WalletCoreProbe {
        id: probe
        Component.onCompleted: run()
    }

    SecretsProbe {
        id: secretsProbe
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
                description: "Milestone 2 · secure wallet storage plumbing"
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
                text: "Run Wallet Core self-test again"
                onClicked: probe.run()
            }

            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * Theme.horizontalPageMargin
                text: "Wallet Core reruns completed: " + probe.runCount
                color: Theme.secondaryColor
                font.pixelSize: Theme.fontSizeExtraSmall
                horizontalAlignment: Text.AlignHCenter
            }

            SectionHeader {
                text: "Sailfish Secrets · test-only"
            }

            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * Theme.horizontalPageMargin
                text: secretsProbe.status
                color: secretsProbe.lastOperationPassed
                       ? Theme.highlightColor : Theme.primaryColor
                font.pixelSize: Theme.fontSizeMedium
                wrapMode: Text.Wrap
            }

            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * Theme.horizontalPageMargin
                text: secretsProbe.detail
                color: Theme.secondaryColor
                font.pixelSize: Theme.fontSizeSmall
                wrapMode: Text.Wrap
            }

            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * Theme.horizontalPageMargin
                text: "Secrets operations completed: " + secretsProbe.operationCount
                      + " · test secret present: "
                      + (secretsProbe.testSecretPresent ? "yes" : "no")
                color: Theme.secondaryHighlightColor
                font.pixelSize: Theme.fontSizeExtraSmall
                wrapMode: Text.Wrap
            }

            Button {
                anchors.horizontalCenter: parent.horizontalCenter
                text: "Store public test secret"
                onClicked: secretsProbe.storeTestSecret()
            }

            Button {
                anchors.horizontalCenter: parent.horizontalCenter
                text: "Verify stored test secret"
                onClicked: secretsProbe.verifyTestSecret()
            }

            Button {
                anchors.horizontalCenter: parent.horizontalCenter
                text: "Delete public test secret"
                onClicked: secretsProbe.deleteTestSecret()
            }

            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * Theme.horizontalPageMargin
                text: "Persistence test: store the test secret, close SailVault, "
                      + "open it again, then tap “Verify stored test secret”."
                color: Theme.secondaryColor
                font.pixelSize: Theme.fontSizeExtraSmall
                wrapMode: Text.Wrap
            }

            SectionHeader {
                text: "Safety"
            }

            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * Theme.horizontalPageMargin
                text: "Milestone 2 is still a development probe, not a usable wallet. "
                      + "The Secrets test stores only the public “abandon … about” test mnemonic. "
                      + "The recovery text remains in C++ and is never exposed to QML. "
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
