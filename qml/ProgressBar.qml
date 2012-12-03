import QtQuick 1.0

Item {
    id: progressbar

    property int minimum: 0
    property int maximum: 100
    property int value: 0
    property alias color: gradient1.color
    property alias secondColor: gradient2.color

    width: parent.width; height: 20*gameBoard.cellSize/40
    clip: true

    Rectangle {
        id: highlight

        property int widthDest: ((progressbar.width * (value - minimum)) / (maximum - minimum)
            + highlight.height*3/2)

        width: highlight.widthDest
        Behavior on width { SmoothedAnimation { velocity: 1200 } }

        anchors {
            left: parent.left;
            top: parent.top;
            bottom: parent.bottom;
            leftMargin: -height; topMargin: 1; bottomMargin: 1
        }
        radius: height/2
        smooth: true
        gradient: Gradient {
            GradientStop { id: gradient1; position: 0.0 }
            GradientStop { id: gradient2; position: 1.0 }
        }
    }
}
