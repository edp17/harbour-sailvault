import QtQuick 2.0
import Sailfish.Silica 1.0
import org.sailfishos.sailvault 1.0

Page {
    id: page

    property var storageInfo: ({})
    property var portfolio

    function refreshStorageInfo() {
        storageInfo = tools.settingsDiagnostics()
    }

    Component.onCompleted: refreshStorageInfo()

    AppTools {
        id: tools
    }

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
                title: "Developer diagnostics"
                description: probe.buildInfo
            }

            SectionHeader {
                text: "Configuration storage"
            }

            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * Theme.horizontalPageMargin
                text: storageInfo.persistenceProbePassed
                      ? "✓ Persistent INI round-trip passed"
                      : "✗ Persistent INI round-trip failed"
                color: storageInfo.persistenceProbePassed
                       ? Theme.highlightColor
                       : Theme.primaryColor
                font.pixelSize: Theme.fontSizeMedium
                wrapMode: Text.Wrap
            }

            DetailItem {
                label: "Schema"
                value: storageInfo.schemaVersion !== undefined
                       ? "v" + storageInfo.schemaVersion
                       : "—"
            }

            DetailItem {
                label: "Settings file"
                value: storageInfo.fileExists ? "Present" : "Missing"
            }

            DetailItem {
                label: "Writable"
                value: storageInfo.fileWritable ? "Yes" : "No"
            }

            DetailItem {
                label: "Stored keys"
                value: storageInfo.keyCount !== undefined
                       ? storageInfo.keyCount.toString()
                       : "—"
            }

            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * Theme.horizontalPageMargin
                text: storageInfo.persistenceProbeDetail || "Storage probe not available"
                color: Theme.secondaryColor
                font.pixelSize: Theme.fontSizeSmall
                wrapMode: Text.Wrap
            }

            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * Theme.horizontalPageMargin
                text: storageInfo.path || ""
                color: Theme.secondaryColor
                font.pixelSize: Theme.fontSizeExtraSmall
                wrapMode: Text.WrapAnywhere
            }

            DetailItem {
                label: "Launch count"
                value: storageInfo.launchCount !== undefined
                       ? String(storageInfo.launchCount)
                       : "—"
            }

            DetailItem {
                label: "First initialized"
                value: storageInfo.firstStartedAt || "—"
            }

            DetailItem {
                label: "Last app version"
                value: storageInfo.lastAppVersion || "—"
            }

            Button {
                anchors.horizontalCenter: parent.horizontalCenter
                text: "Run storage probe again"
                onClicked: {
                    storageInfo = tools.rerunSettingsPersistenceProbe()
                }
            }

            SectionHeader {
                text: "Network resilience"
            }

            DetailItem {
                label: "Offline mode"
                value: portfolio && portfolio.offlineMode ? "Enabled" : "Disabled"
            }

            DetailItem {
                label: "Ordinary provider timeout"
                value: "15 seconds"
            }

            DetailItem {
                label: "Health / fee timeout"
                value: "12 seconds"
            }

            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * Theme.horizontalPageMargin
                text: "Superseded requests are cancelled and stale replies are ignored before "
                      + "newer refresh state can be replaced."
                color: Theme.secondaryColor
                font.pixelSize: Theme.fontSizeSmall
                wrapMode: Text.Wrap
            }

            SectionHeader {
                text: "Wallet Core self-test"
            }

            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * Theme.horizontalPageMargin
                text: probe.summary.length ? probe.summary : "Running self-test…"
                color: probe.allPassed ? Theme.highlightColor : Theme.primaryColor
                font.pixelSize: Theme.fontSizeLarge
                wrapMode: Text.Wrap
            }

            Repeater {
                model: probe.results

                delegate: Column {
                    x: Theme.horizontalPageMargin
                    width: content.width - 2 * Theme.horizontalPageMargin
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

                    Item {
                        width: 1
                        height: Theme.paddingMedium
                    }
                }
            }

            Button {
                anchors.horizontalCenter: parent.horizontalCenter
                text: "Run diagnostics again"
                onClicked: probe.run()
            }

            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * Theme.horizontalPageMargin
                text: "Diagnostic runs: " + probe.runCount
                color: Theme.secondaryColor
                font.pixelSize: Theme.fontSizeExtraSmall
                horizontalAlignment: Text.AlignHCenter
            }
        }
    }
}
