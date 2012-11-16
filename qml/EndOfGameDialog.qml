import QtQuick 1.0

Rectangle {
    id: page

    signal closed
    signal opened

    function forceClose() {
        if(page.opacity == 0)
            return; //already closed
        page.closed();
        page.state = "stateHidden";
        page.opacity = 0;
    }

    function show() {
        page.opened();
        page.state = "stateShown";
    }

    anchors.fill: parent
    anchors.margins: 10
    state: "stateHidden"
    color: "black"
    border.width: 1
    opacity: 0
    visible: opacity > 0
    radius: 5
    Behavior on opacity {
        NumberAnimation { duration: 100 }
    }

    Text {
        id: titleText;
        anchors.top: parent.top;
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.topMargin: 5;
        font.bold: true;
        font.pointSize: 20*gameBoard.cellSize/40
        font.family: buttonFont.name
        color: "white"
        text: "Your result"
    }

    Text {
        id: lblLevel
        color: "white"
        text: "Level"
        anchors.top: titleText.bottom
        anchors.margins: 20*gameBoard.cellSize/40
        font.pointSize: 16*gameBoard.cellSize/40
        font.family: buttonFont.name
    }

    Text {
        id: lblScore
        color: "white"
        text: "Score"
        anchors.top: lblLevel.bottom
        anchors.margins: 20*gameBoard.cellSize/40
        font.pointSize: 16*gameBoard.cellSize/40
        font.family: buttonFont.name
    }

    Text {
        id: lblHighScore
        color: "white"
        text: "High score"
        anchors.top: lblScore.bottom
        anchors.margins: 20*gameBoard.cellSize/40
        font.pointSize: 16*gameBoard.cellSize/40
        font.family: buttonFont.name
    }

    Text {
        id: valueLevel
        color: "white"
        text: gameBoard.level
        anchors { top: titleText.bottom; margins: 20*gameBoard.cellSize/40 }
        font { pointSize: 16*gameBoard.cellSize/40; family: buttonFont.name }
    }

    Text {
        id: valueHighScore
        color: "white"
        text: gameBoard.settings("classic/highScore")
        anchors { top: valueScore.bottom; margins: 20*gameBoard.cellSize/40 }
        font { pointSize: 16*gameBoard.cellSize/40; family: buttonFont.name }
    }

    Text {
        id: valueScore
        color: "white"
        text: gameBoard.score
        anchors { top: valueLevel.bottom; margins: 20*gameBoard.cellSize/40 }
        font { pointSize: 16*gameBoard.cellSize/40; family: buttonFont.name }
    }

    states: [
        State {
            name: "stateShown"
            when: visible == true
            AnchorChanges { target: titleText; anchors.top: page.top }
            AnchorChanges { target: lblLevel; anchors.left: page.left }
            AnchorChanges { target: lblScore; anchors.left: page.left }
            AnchorChanges { target: lblHighScore; anchors.left: page.left }
            AnchorChanges { target: valueLevel; anchors.right: page.right }
            AnchorChanges { target: valueScore; anchors.right: page.right }
            AnchorChanges { target: valueHighScore; anchors.right: page.right }
            StateChangeScript {
                script: valueHighScore.text = gameBoard.settings("classic/highScore")
            }
        },
        State {
            name: "stateHidden"
            when: visible == false
            AnchorChanges { target: titleText; anchors.top: page.bottom }
            AnchorChanges { target: lblLevel; anchors.right: page.left }
            AnchorChanges { target: lblScore; anchors.right: page.left }
            AnchorChanges { target: lblHighScore; anchors.right: page.left }
            AnchorChanges { target: valueLevel; anchors.left: page.right }
            AnchorChanges { target: valueScore; anchors.left: page.right }
            AnchorChanges { target: valueHighScore; anchors.left: page.right }
            AnchorChanges { target: page; anchors.bottom: parent.top }
        }
    ]

    transitions: [
        Transition {
            from: "stateHidden"
            to: "stateShown"
            SequentialAnimation {
                /* Turning labeles and scores invisible */
                PropertyAction {
                    targets: [lblLevel, lblScore, valueLevel, valueScore, titleText, lblHighScore
                        , valueHighScore];
                    properties: "visible";
                    value: false
                }

                /* This pause is used to wait until text message animation ends */
                PauseAnimation { duration: 2800 }

                /* Showing main page and title text */
                PropertyAction { target: page; property: "opacity"; value: 0.7 }
                PropertyAction { target: titleText; property: "visible"; value: true }
                AnchorAnimation { targets: titleText; duration: 500; easing.type: Easing.OutBounce }

                /* Turning labeles and scores visible and show 'em */
                PauseAnimation { duration: 500 }
                PropertyAction {
                    targets: [lblLevel, lblScore, valueLevel, valueScore, lblHighScore
                        , valueHighScore];
                    properties: "visible";
                    value: true
                }
                AnchorAnimation { duration: 500; easing.type: Easing.OutBounce }
            }
        },
        Transition {
            from: "stateShown"
            to: "stateHidden"
            AnchorAnimation { duration: 1000 }
        }
    ]


    MouseArea { anchors.fill: parent; onClicked: page.forceClose(); }
}
