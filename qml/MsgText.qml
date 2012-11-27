import QtQuick 1.0

Text {
    id: msgText
    color: "gold"
    font.family: gameFont.name
    font.pointSize: 80//38*gameBoard.cellSize/40
    font.bold: true
    anchors.centerIn: gameBoard
    style: Text.Outline
    smooth: true
    styleColor: "red"
    z: 5
    opacity: 0

    function show() {
        msgTextAnimation.start();
    }

    SequentialAnimation {
        id: msgTextAnimation

        ParallelAnimation {
            NumberAnimation { target: msgText; properties: "opacity"; from: 0; to: 1; duration: 400 }
            NumberAnimation { target: msgText; properties: "scale"; from: 0.1; to: 1; duration: 400 }
        }

//            PauseAnimation { duration: 500 }
        NumberAnimation { target: msgText; properties: "scale"; from: 1; to: 0.7; duration: 400 }
        NumberAnimation { target: msgText; properties: "scale"; from: 0.7; to: 1; duration: 400 }

        PauseAnimation { duration: 700 }
        ParallelAnimation {
            NumberAnimation { target: msgText; properties: "opacity"; from: 1; to: 0; duration: 1000 }
            NumberAnimation { target: msgText; properties: "scale"; from: 1; to: 0.1; duration: 1000 }
        }
    }
}
