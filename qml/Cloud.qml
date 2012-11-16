import QtQuick 1.0

Item {
    id: cloudItem
    width:  40*g_scaleFactor
    height:  40*g_scaleFactor
    z: 5
    property int animationDuration: 9000

    Image {
        id: cloudImageOuter
        anchors.fill: parent
        source: ":/pics/effects/cloudFront.svg"
        smooth: true
        sourceSize.width: cloudItem.width
        sourceSize.height: cloudItem.height
    }

    Image {
        id: cloudImageInner
        anchors.fill: parent
        source: ":/pics/effects/cloudFront.svg"
        scale: 0.0
        smooth: true
        sourceSize.width: cloudItem.width
        sourceSize.height: cloudItem.height

        SequentialAnimation {
            running: cloudItem.visible
            loops: Animation.Infinite
            NumberAnimation { target: cloudImageInner; property: "scale"; to: 1.0; duration: 500 }
            PropertyAction { target: cloudImageInner; property: "scale"; value: 0.0 }
            PauseAnimation { duration: 500 }
        }
    }
}
