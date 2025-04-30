import QtQuick 2.15
import QtQuick.Window 2.15
import QtQuick.Controls 2.15

Window {
    id: mainWindow
    visible: true
    width: 1024
    height: 768
    title: "Rhynec Total Security"

    // Set application-wide font
    font.family: "Montserrat"

    // Main layout with sidebar
    Row {
        anchors.fill: parent

        // Sidebar component
        Sidebar {
            id: sidebar
            width: 280
            height: parent.height
        }

        // Main content area
        Rectangle {
            width: parent.width - sidebar.width
            height: parent.height
            color: "#f5f5f5"

            // Placeholder for main content
            Text {
                anchors.centerIn: parent
                text: "Main Content Area"
                font.pixelSize: 24
                color: "#333333"
            }
        }
    }
}
