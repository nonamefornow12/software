import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import "components"
import com.rhynec.app 1.0

Rectangle {
    id: sidebarRoot
    color: "white"

    // Custom properties
    property string currentMenuItem: "Dashboard"

    // Shadow effect for the sidebar
    Rectangle {
        anchors {
            top: parent.top
            bottom: parent.bottom
            right: parent.right
        }
        width: 1
        color: "#e0e0e0"
    }

    // Main sidebar content layout
    ColumnLayout {
        anchors {
            fill: parent
            margins: 20
        }
        spacing: 20

        // Top section with logo and app name
        RowLayout {
            Layout.fillWidth: true
            spacing: 12

            // Logo image
            Image {
                id: logoImage
                source: "https://rhynec.com/logo.png"
                fillMode: Image.PreserveAspectFit
                Layout.preferredWidth: 32
                Layout.preferredHeight: 32

                // Fallback if the external image fails to load
                onStatusChanged: {
                    if (status === Image.Error) {
                        source = "qrc:/assets/icons/logo-placeholder.png"
                    }
                }
            }

            // App name text
            Text {
                text: "Rhynec Total Security"
                font {
                    family: "Montserrat"
                    bold: true
                    pixelSize: 16
                }
                color: "black"
                Layout.fillWidth: true
            }
        }

        // First divider
        Rectangle {
            Layout.fillWidth: true
            height: 1
            color: "#e0e0e0"
        }

        // First menu section
        ColumnLayout {
            Layout.fillWidth: true
            spacing: 12

            // Dashboard menu item
            MenuButton {
                icon: "qrc:/assets/icons/dashboard.svg"
                label: "Dashboard"
                isActive: currentMenuItem === "Dashboard"
                onClicked: currentMenuItem = "Dashboard"
            }

            // VPN menu item
            MenuButton {
                icon: "qrc:/assets/icons/globe.svg"
                label: "VPN"
                isActive: currentMenuItem === "VPN"
                onClicked: currentMenuItem = "VPN"
            }

            // Protection menu item
            MenuButton {
                icon: "qrc:/assets/icons/shield.svg"
                label: "Protection"
                isActive: currentMenuItem === "Protection"
                onClicked: currentMenuItem = "Protection"
            }

            // Network menu item
            MenuButton {
                icon: "qrc:/assets/icons/network.svg"
                label: "Network"
                isActive: currentMenuItem === "Network"
                onClicked: currentMenuItem = "Network"
            }
        }

        // Second divider
        Rectangle {
            Layout.fillWidth: true
            height: 1
            color: "#e0e0e0"
        }

        // Second menu section
        ColumnLayout {
            Layout.fillWidth: true
            spacing: 12

            // Settings menu item
            MenuButton {
                icon: "qrc:/assets/icons/settings.svg"
                label: "Settings"
                isActive: currentMenuItem === "Settings"
                onClicked: currentMenuItem = "Settings"
            }

            // Subscription menu item
            MenuButton {
                icon: "qrc:/assets/icons/card.svg"
                label: "Subscription"
                isActive: currentMenuItem === "Subscription"
                onClicked: currentMenuItem = "Subscription"
            }

            // Profile menu item
            MenuButton {
                icon: "qrc:/assets/icons/user.svg"
                label: "Profile"
                isActive: currentMenuItem === "Profile"
                onClicked: currentMenuItem = "Profile"
            }
        }

        // Spacer
        Item {
            Layout.fillHeight: true
        }

        // Subscription panel
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 50
            color: "#f5f5f5"
            radius: 8

            Text {
                anchors.centerIn: parent
                text: "FREE Subscription"
                font {
                    family: "Montserrat"
                    bold: true
                    pixelSize: 14
                }
                color: "#9e9e9e"
            }
        }

        // Third divider
        Rectangle {
            Layout.fillWidth: true
            height: 1
            color: "#e0e0e0"
        }

        // Profile section
        RowLayout {
            Layout.fillWidth: true
            spacing: 12

            // Profile picture component
            ProfilePicture {
                id: profilePic
                Layout.preferredWidth: 40
                Layout.preferredHeight: 40

                // Open file dialog when clicked
                MouseArea {
                    anchors.fill: parent
                    cursorShape: Qt.PointingHandCursor
                    onClicked: AppUI.openFileDialog()
                }
            }

            // Username
            Text {
                text: "Username"
                font {
                    family: "Montserrat"
                    bold: true
                    pixelSize: 14
                }
                color: "black"
                Layout.fillWidth: true
            }

            // Three-dot settings button
            Button {
                id: settingsButton
                Layout.preferredWidth: 24
                Layout.preferredHeight: 24
                flat: true

                contentItem: Text {
                    text: "⋮"
                    font.pixelSize: 18
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                    color: settingsButton.hovered ? "#555555" : "#888888"
                }

                background: Rectangle {
                    color: "transparent"
                }
            }
        }
    }

    // Define MenuItem component inline
    component MenuButton: Rectangle {
        id: menuItem
        Layout.fillWidth: true
        height: 40
        color: "transparent"
        radius: 4

        // Properties
        property string icon: ""
        property string label: ""
        property bool isActive: false

        // Signals
        signal clicked()

        // Background - shows when active or hovered
        Rectangle {
            id: itemBackground
            anchors.fill: parent
            color: isActive ? "#f0f0f0" : (mouseArea.containsMouse ? "#f8f8f8" : "transparent")
            radius: 4

            // Animation for smooth hover effect
            Behavior on color {
                ColorAnimation {
                    duration: 150
                }
            }
        }

        // Content layout
        RowLayout {
            anchors {
                fill: parent
                leftMargin: 8
                rightMargin: 8
            }
            spacing: 12

            // Menu icon
            Image {
                source: icon
                fillMode: Image.PreserveAspectFit
                Layout.preferredWidth: 20
                Layout.preferredHeight: 20
                opacity: isActive ? 1.0 : 0.8
            }

            // Menu text
            Text {
                text: label
                font {
                    family: "Montserrat"
                    bold: true
                    pixelSize: 14
                }
                color: isActive ? "#333333" : "#666666"
                Layout.fillWidth: true
            }
        }

        // Mouse area for interaction
        MouseArea {
            id: mouseArea
            anchors.fill: parent
            hoverEnabled: true
            cursorShape: Qt.PointingHandCursor
            onClicked: parent.clicked()
        }
    }
}
