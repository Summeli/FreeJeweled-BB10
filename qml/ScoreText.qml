import QtQuick 1.0

Item {
    id: scoreItem
    opacity: 0
    width: 40*g_scaleFactor
    height: 40*g_scaleFactor
    /* Should be in front of gems */
    z: 2

    property int type: 1
    property int shiftValue: 80*g_scaleFactor
    property int scoreValue: 0
    property bool animationStarted: false

    Text {
        id: scoreLabel

        anchors.centerIn: scoreItem
        text: scoreItem.scoreValue
        font.bold: true
        font.pointSize: 16*g_scaleFactor

        color: {
            if (scoreItem.type == 1)
                return "red";
            else if (scoreItem.type == 2)
                return "steelblue";
            else if (scoreItem.type == 3)
                return "green";
            else if (scoreItem.type == 4)
                return "purple";
            else if (scoreItem.type == 5)
                return "white";
            else if (scoreItem.type == 6)
                return "orange";
            else if (scoreItem.type == 0)
                return "yellow";
        }

        ParallelAnimation {
            id: scoreLabelAnimation
            running: scoreItem.animationStarted
            PropertyAction { target: scoreItem; property: "opacity"; value: 1 }
            NumberAnimation { target: scoreItem; property: "y"; to: scoreItem.y - scoreItem.shiftValue; duration: 1500 }
            NumberAnimation { target: scoreItem; property: "opacity"; to: 0; duration: 1500 }
        }
    }
}


