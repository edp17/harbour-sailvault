import QtQuick 2.0
import Sailfish.Silica 1.0

Page {
    id: page

    property var security

    SilicaFlickable {
        anchors.fill: parent
        contentHeight: content.height

        VerticalScrollDecorator { }

        Column {
            id: content
            width: parent.width
            spacing: Theme.paddingLarge

            PageHeader {
                title: "Security settings"
                description: "Wallet session auto-lock"
            }

            TextSwitch {
                width: parent.width
                text: "Automatic session lock"
                description: "Clear the unlocked public wallet session automatically."
                checked: security ? security.enabled : true

                onClicked: {
                    if (security)
                        security.enabled = checked
                }
            }

            ComboBox {
                id: backgroundBox
                property bool settingsLoaded: false
                width: parent.width
                enabled: security && security.enabled
                label: "Lock after leaving foreground"

                menu: ContextMenu {
                    MenuItem {
                        text: "Immediately"
                        onClicked: if (security) security.backgroundDelaySeconds = 0
                    }
                    MenuItem {
                        text: "30 seconds"
                        onClicked: if (security) security.backgroundDelaySeconds = 30
                    }
                    MenuItem {
                        text: "1 minute"
                        onClicked: if (security) security.backgroundDelaySeconds = 60
                    }
                    MenuItem {
                        text: "5 minutes"
                        onClicked: if (security) security.backgroundDelaySeconds = 300
                    }
                }

                Component.onCompleted: {
                    if (!security)
                        return

                    if (security.backgroundDelaySeconds === 30)
                        currentIndex = 1
                    else if (security.backgroundDelaySeconds === 60)
                        currentIndex = 2
                    else if (security.backgroundDelaySeconds === 300)
                        currentIndex = 3
                    else
                        currentIndex = 0

                    settingsLoaded = true
                }

                onCurrentIndexChanged: {
                    if (!security || !settingsLoaded)
                        return
                    var seconds = 0
                    if (currentIndex === 1) seconds = 30
                    else if (currentIndex === 2) seconds = 60
                    else if (currentIndex === 3) seconds = 300
                    if (security.backgroundDelaySeconds !== seconds)
                        security.backgroundDelaySeconds = seconds
                }

            }

            ComboBox {
                id: inactivityBox
                property bool settingsLoaded: false
                width: parent.width
                enabled: security && security.enabled
                label: "Lock after no interaction"

                menu: ContextMenu {
                    MenuItem {
                        text: "Never"
                        onClicked: if (security) security.inactivityMinutes = 0
                    }
                    MenuItem {
                        text: "1 minute"
                        onClicked: if (security) security.inactivityMinutes = 1
                    }
                    MenuItem {
                        text: "5 minutes"
                        onClicked: if (security) security.inactivityMinutes = 5
                    }
                    MenuItem {
                        text: "15 minutes"
                        onClicked: if (security) security.inactivityMinutes = 15
                    }
                }

                Component.onCompleted: {
                    if (!security)
                        return

                    if (security.inactivityMinutes === 0)
                        currentIndex = 0
                    else if (security.inactivityMinutes === 1)
                        currentIndex = 1
                    else if (security.inactivityMinutes === 15)
                        currentIndex = 3
                    else
                        currentIndex = 2

                    settingsLoaded = true
                }

                onCurrentIndexChanged: {
                    if (!security || !settingsLoaded)
                        return
                    var minutes = 5
                    if (currentIndex === 0) minutes = 0
                    else if (currentIndex === 1) minutes = 1
                    else if (currentIndex === 3) minutes = 15
                    if (security.inactivityMinutes !== minutes)
                        security.inactivityMinutes = minutes
                }

            }

            Button {
                anchors.horizontalCenter: parent.horizontalCenter
                text: "Save security settings"
                onClicked: {
                    if (!security) return
                    var seconds = 0
                    if (backgroundBox.currentIndex === 1) seconds = 30
                    else if (backgroundBox.currentIndex === 2) seconds = 60
                    else if (backgroundBox.currentIndex === 3) seconds = 300
                    var minutes = 5
                    if (inactivityBox.currentIndex === 0) minutes = 0
                    else if (inactivityBox.currentIndex === 1) minutes = 1
                    else if (inactivityBox.currentIndex === 3) minutes = 15
                    security.backgroundDelaySeconds = seconds
                    security.inactivityMinutes = minutes
                }
            }

            SectionHeader {
                text: "What gets locked?"
            }

            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * Theme.horizontalPageMargin
                text: "Auto-lock clears the derived public addresses from the active "
                      + "SailVault session. The encrypted development wallet remains "
                      + "stored in Sailfish Secrets and must be loaded again to reopen "
                      + "the session."
                color: Theme.secondaryColor
                font.pixelSize: Theme.fontSizeSmall
                wrapMode: Text.Wrap
            }

            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * Theme.horizontalPageMargin
                text: "SailVault does not keep the recovery phrase or derived private "
                      + "keys in this session object. Wallet Core key objects are created "
                      + "inside individual operations and released afterwards."
                color: Theme.secondaryColor
                font.pixelSize: Theme.fontSizeSmall
                wrapMode: Text.Wrap
            }

            SectionHeader {
                text: "Saved policy"
            }

            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * Theme.horizontalPageMargin
                text: security
                      ? "Background: " + security.backgroundDelayText
                        + " · inactivity: " + security.inactivityText
                      : ""
                color: Theme.highlightColor
                font.pixelSize: Theme.fontSizeSmall
                wrapMode: Text.Wrap
            }

            SectionHeader {
                text: "Defaults"
            }

            Label {
                x: Theme.horizontalPageMargin
                width: parent.width - 2 * Theme.horizontalPageMargin
                text: "Automatic lock is enabled by default. Leaving SailVault locks "
                      + "immediately; foreground inactivity locks after 5 minutes. "
                      + "Changes are persisted immediately and restored on the next start."
                color: Theme.secondaryColor
                font.pixelSize: Theme.fontSizeSmall
                wrapMode: Text.Wrap
            }
        }
    }
}
