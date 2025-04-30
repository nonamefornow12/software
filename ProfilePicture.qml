import QtQuick 2.15
import com.rhynec.app 1.0

Item {
    id: profilePictureRoot

    // Default profile image URL
    property url imageSource: "qrc:/assets/icons/default-avatar.png"

    // Connect to C++ backend signal
    Connections {
        target: AppUI
        function onProfileImageChanged(imageUrl) {
            profilePictureRoot.imageSource = imageUrl
        }
    }

    // Circle mask for the image
    Rectangle {
        id: mask
        anchors.fill: parent
        radius: width / 2
        visible: false
    }

    // Profile image
    Image {
        id: image
        anchors.fill: parent
        source: profilePictureRoot.imageSource
        fillMode: Image.PreserveAspectCrop
        layer.enabled: true
        layer.effect: OpacityMask {
            maskSource: mask
        }

        // Border for the image
        Rectangle {
            id: border
            anchors.fill: parent
            radius: width / 2
            color: "transparent"
            border.width: 1
            border.color: "#e0e0e0"
        }
    }
}
