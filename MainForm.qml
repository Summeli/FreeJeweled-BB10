import QtQuick 1.0
import com.mycompany.gemcell 1.0

Rectangle {
    id: screen
    color: "black"
    width: 8*gameBoard.cellSize
    height: 12*gameBoard.cellSize
    property double g_scaleFactor: gameBoard.cellSize/40
    state: "stateMainMenu"
    z: -10

    function setBackgroundSource() {
        var source = generateBackgroundFileName();
        while (source == background.source) {
            source = generateBackgroundFileName();
        }
        background.source = source;
    }

    function generateBackgroundFileName() {
        var bgrStr = Math.floor(Math.random()*20 + 1).toString();
        if (bgrStr.length == 1) {
            bgrStr = "0" + bgrStr;
        }
        return ":/pics/backgrounds/bgr" + bgrStr + ".jpg";
    }

    SystemPalette { id: activePalette }

    FontLoader { id: gameFont; source: ":/fonts/mailrays.ttf" }
    FontLoader { id: buttonFont; source: ":/fonts/pirulen.ttf" }

    Image {
        id: background
        anchors.fill: parent
        fillMode: Image.PreserveAspectCrop
    }

    Image {
        id: bgrMainMenu
        anchors.fill: parent
        source: ":/pics/backgrounds/bgr00.jpg"
    }

    AboutDialog {
        id: dlgAbout
        visible: opacity > 0
        opacity: 0.0
        MouseArea {
            anchors.fill: parent
            onClicked: screen.state = "stateMainMenu"
        }
        Behavior on opacity {
            NumberAnimation { duration: 500 }
        }
    }

    SettingsDialog {
        id: dlgSettings
        visible: opacity > 0
        opacity: 0.0
        canIncreaseSize: gameBoard.cellSize < 80
        canDecreaseSize: gameBoard.cellSize > 40
        Behavior on opacity {
            NumberAnimation { duration: 500 }
        }
        onSizeDecrease: gameBoard.cellSize -= 10;
        onSizeIncrease: gameBoard.cellSize += 10;
        onBackPressed: screen.state = "stateMainMenu"
        onAboutPressed: screen.state = "stateAbout"
    }

    Rectangle {
        id: topGameBoardBorder
        visible: screen.state == "stateGame"
        color: "black"
        opacity: 0.5
        anchors.top: scoreBox.bottom
        height: 5*g_scaleFactor
        width: parent.width
    }

    Rectangle {
        id: bottomGameBoardBorder
        visible: screen.state == "stateGame"
        color: "black"
        opacity: 0.5
        anchors.top: gameBoard.bottom
        height: 5*g_scaleFactor
        width: parent.width
    }

    GameBoard {
        id: gameBoard
        width: 8*gameBoard.cellSize
        height: 8*gameBoard.cellSize
        anchors.top: topGameBoardBorder.bottom
        visible: opacity > 0
        opacity: 0
        property int hintX: 0
        property int hintY: 0
        property bool hintVisible: false

        MouseArea {
            anchors.fill: parent
            onClicked: gameBoard.handleClick(mouse.x, mouse.y)
        }

        Image {
            id: gbBackground
            source: ":/pics/field.svg"
            anchors.fill: parent
            sourceSize.width: gbBackground.width
            sourceSize.height: gbBackground.height
        }

        Item {
            id: selectionRect
            width: gameBoard.cellSize
            height: gameBoard.cellSize
            visible: gameBoard.gemSelected
            x: gameBoard.selGemColumn*width
            y: gameBoard.selGemRow*width

            Image {
                anchors.fill: parent
                source: ":/pics/selectionBorder.png"
                opacity: 0.8
                fillMode: Image.PreserveAspectCrop
            }
        }

        Image {
            id: hintImage
            source: ":/pics/hintArrow.svg"
            x: gameBoard.hintX
            y: gameBoard.hintY - height/4
            width: gameBoard.cellSize
            height: gameBoard.cellSize/2
            sourceSize.width: width
            sourceSize.height: height


            visible: gameBoard.hintVisible
            z: 5
            ParallelAnimation {
                running: hintImage.visible
                SequentialAnimation {
                    loops: Animation.Infinite
                    PropertyAnimation {
                        target: hintImage;
                        property: "y";
                        to: gameBoard.hintY - 3*hintImage.height/4;
                        duration: 300;
                        easing.type: Easing.InOutQuad
                    }
                    PropertyAnimation {
                        target: hintImage;
                        property: "y";
                        to: gameBoard.hintY - hintImage.height/4;
                        duration: 300;
                        easing.type: Easing.InOutQuad
                    }
                }
                SequentialAnimation {
                    PauseAnimation { duration: 3000 }
                    ScriptAction { script: gameBoard.hintVisible = false }
                }
            }
        }

        onLevelUp: levelUpAnimation.start()

        onNoMoreMoves: {
            msgText.text = "NO MORE MOVES";
            msgText.font.pointSize = 30*g_scaleFactor;
            msgText.show();
            dlgEndGame.show();
        }
    }

    SequentialAnimation {
        id: levelUpAnimation
        ScriptAction {
            script: {
                msgText.text = "LEVEL UP!";
                msgText.font.pointSize = 38*g_scaleFactor;
                msgText.show();
                gameBoard.dropGemsDown();
            }
        }
        PauseAnimation { duration: 3000 }
        ScriptAction {
            script: {
                msgText.text = "LEVEL " + gameBoard.level;
                msgText.font.pointSize = 38*g_scaleFactor;
                msgText.show();
                pbLevelProgress.minimum = gameBoard.levelCap(gameBoard.level - 1);
                pbLevelProgress.maximum = gameBoard.levelCap(gameBoard.level);
                setBackgroundSource();
                gameBoard.resetBoard();
            }
        }
    }

    EndOfGameDialog {
        id: dlgEndGame
        anchors.centerIn: gameBoard
        z: 10
        onClosed: screen.state = "stateMainMenu"
    }

    LoadSavedGameDialog {
        id: dlgLoadSave
        anchors.centerIn: screen
        z: 10
        onCancel: screen.state = "stateMainMenu"
        onLoadSaved: {
            gameBoard.loadBoardStateFromFile();
            pbLevelProgress.minimum = gameBoard.levelCap(gameBoard.level - 1);
            pbLevelProgress.maximum = gameBoard.levelCap(gameBoard.level);
        }
        onNewGame: gameBoard.newGame();
    }

    ProgressBar {
        id: pbLevelProgress
        visible: screen.state == "stateGame"
        anchors.horizontalCenter: screen.horizontalCenter
        anchors.top: bottomGameBoardBorder.bottom
        color: "black"
        secondColor: "green"
        height: 20*g_scaleFactor
        maximum: gameBoard.levelCap(gameBoard.level)
        value: gameBoard.score
    }

    Item {
        id: scoreBox

        width: parent.width
        height: 60*g_scaleFactor /* cellSize*1.5, actually */
        state: "stateHidden"

        anchors.top: parent.top

        Text {
            id: txtScore
            color: "white"
            font.family: gameFont.name
            font.pointSize: 16*g_scaleFactor
            font.bold: true
            text: gameBoard.score
            anchors.bottom: parent.bottom
            anchors.leftMargin: 10*g_scaleFactor
            anchors.bottomMargin: 5*g_scaleFactor
        }

        Text {
            id: txtLevel
            color: "white"
            font.family: gameFont.name
            font.pointSize: 16*g_scaleFactor
            font.bold: true
            text: "Level " + gameBoard.level + " "
            anchors.bottom: parent.bottom
            anchors.rightMargin: 10*g_scaleFactor
            anchors.bottomMargin: 5*g_scaleFactor
        }

        states: [
            State {
                name: "stateNormal"
                AnchorChanges { target: txtScore; anchors.left: scoreBox.left }
                AnchorChanges { target: txtLevel; anchors.right: scoreBox.right }
            },
            State {
                name: "stateHidden"
                AnchorChanges { target: txtScore; anchors.right: scoreBox.left }
                AnchorChanges { target: txtLevel; anchors.left: scoreBox.right }
            }
        ]

        transitions: [
            Transition {
                from: "stateHidden"
                to: "stateNormal"
                AnchorAnimation { duration: 200; easing.type: Easing.Linear }
            },
            Transition {
                from: "stateNormal"
                to: "stateHidden"
                AnchorAnimation { duration: 200; easing.type: Easing.Linear }
            }
        ]

    }

    MsgText {
        id: msgText
    }

    Item {
        id: toolBar
        width: parent.width
        height: parent.height - scoreBox.height - topGameBoardBorder.height - gameBoard.height
            - bottomGameBoardBorder.height - pbLevelProgress.height
        anchors.top: pbLevelProgress.bottom
        visible: opacity > 0

        InGameButton {
            id: btnReset
            anchors.top: parent.top
            anchors.left: parent.left
            color: "red"
            caption: "Reset"

            onClicked: gameBoard.resetBoard()
        }
/*
        InGameButton {
            id: btnRemoveAll
            anchors.top: parent.top
            anchors.left: btnReset.right
            caption: "Run"
            color: "steelblue"

            onClicked: gameBoard.removeAll()
        }*/

        InGameButton {
            id: btnLevelUp
            anchors.top: parent.top
            anchors.left: btnReset.right
            caption: "LevelUp"
            color: "blue"

            onClicked: {
                gameBoard.score = gameBoard.levelCap(gameBoard.level);
                gameBoard.removeAll();
            }
        }

        /* Second row of buttons */
/*        InGameButton {
            id: btnLoadTest
            anchors.bottom: parent.bottom
            anchors.left: parent.left
            caption: "Test"
            color: "royalblue"

            onClicked: gameBoard.loadTestBoard()
        }*/

        InGameButton {
            id: btnShowHint
            anchors.bottom: parent.bottom
            anchors.left: btnLoadTest.right
            caption: "Hint"
            color: "green"

            onClicked: gameBoard.showHint()
        }

        InGameButton {
            id: btnMenu
            anchors.bottom: parent.bottom
            anchors.left: btnShowHint.right
            caption: "Menu"
            color: "red"

            onClicked: screen.state = "stateMainMenu"
        }
    }

    Text {
        id: txtAppVersion
        text: g_appVersion
        font.pointSize: 14*g_scaleFactor
        font.family: buttonFont.name
        color: "lightgray"
        visible: opacity > 0
        anchors { bottom: screen.bottom; right: screen.right; margins: 3*g_scaleFactor }
        Behavior on opacity {
            NumberAnimation { duration: 500 }
        }
    }

    Image {
        id: gameTitle
        anchors.topMargin: 30*g_scaleFactor
        anchors.horizontalCenter: parent.horizontalCenter
        width: parent.width*0.9
        height: 126./346.*width
        source: ":/pics/titleText.svg"
        sourceSize { width: gameTitle.width; height: gameTitle.height }

        Image {
            anchors.centerIn: parent
            width: 50*g_scaleFactor
            height: 50*g_scaleFactor
            sourceSize.width: width
            sourceSize.height: height
            visible: gameTitle.y > 0
            source: ":/pics/gems/orangeGem.svg"
            Shine { anchors { leftMargin: 10*g_scaleFactor; topMargin: 10*g_scaleFactor } }
        }
    }

    MainMenuButton {
        id: btnClassic
        caption: "CLASSIC"
        anchors.top: screen.top
        anchors.margins: gameTitle.height + gameTitle.anchors.topMargin + 40*g_scaleFactor
        color: "steelblue"
        onClicked: screen.state = "stateGame"
    }
/*
    MainMenuButton {
        id: btnEndless
        enabled: false
        caption: "ENDLESS"
        anchors.top: btnClassic.bottom
        anchors.margins: 10*g_scaleFactor
        color: "gray"
    }

    MainMenuButton {
        id: btnAction
        enabled: false
        caption: "Action"
        anchors.top: btnEndless.bottom
        anchors.margins: 10*g_scaleFactor
        color: "gray"
    }*/
/*
    MainMenuButton {
        id: btnAbout
        caption: "settings"
        anchors.top: btnAction.bottom
        anchors.margins: 10*g_scaleFactor
        color: "steelblue"
        onClicked: screen.state = "stateSettings"
    }*/

    MainMenuButton {
        id: btnAbout
        caption: "About"
        anchors.top: btnClassic.bottom
        anchors.margins: 40*g_scaleFactor
        color: "steelblue"
        onClicked: screen.state = "stateAbout"
    }
    
    states: [
        State {
            name: "stateMainMenu"
            /* Main menu elements anchors */
            AnchorChanges { target: gameTitle; anchors.top: screen.top }
            AnchorChanges { target: btnClassic; anchors.horizontalCenter: screen.horizontalCenter }
            AnchorChanges { target: btnEndless; anchors.horizontalCenter: screen.horizontalCenter }
            AnchorChanges { target: btnAction; anchors.horizontalCenter: screen.horizontalCenter }
            AnchorChanges { target: btnAbout; anchors.horizontalCenter: screen.horizontalCenter }

            /* Game elements anchors */
            AnchorChanges { target: toolBar; anchors.top: screen.bottom }
            AnchorChanges { target: gameBoard; anchors.left: screen.right }
        },
        State {
            name: "stateGame"
            /* Main menu elements anchors */
            AnchorChanges { target: gameTitle; anchors.bottom: screen.top }
            AnchorChanges { target: btnClassic; anchors.right: screen.left }
            AnchorChanges { target: btnAbout; anchors.left: screen.right }

            /* Game elements anchors */
            AnchorChanges { target: toolBar; anchors.top: pbLevelProgress.bottom }
            AnchorChanges { target: gameBoard; anchors.left: screen.left }
        },
        State {
            name: "stateSettings"
            /* Main menu elements anchors */
            AnchorChanges { target: gameTitle; anchors.bottom: screen.top }
            AnchorChanges { target: btnClassic; anchors.right: screen.left }
            AnchorChanges { target: btnEndless; anchors.left: screen.right }
            AnchorChanges { target: btnAction; anchors.right: screen.left }
            AnchorChanges { target: btnAbout; anchors.left: screen.right }

            /* Game elements anchors */
            AnchorChanges { target: toolBar; anchors.top: screen.bottom }

            /* Showing Settings and hiding About dialog */
            PropertyChanges { target: dlgSettings; opacity: 1.0 }
            PropertyChanges { target: dlgAbout; opacity: 0.0 }
        },
        State {
            name: "stateAbout"
            /* Showing About and hiding Settings dialogs */
            PropertyChanges { target: dlgSettings; opacity: 0.0 }
            PropertyChanges { target: dlgAbout; opacity: 1.0 }

            /* Showing info about app version */
            PropertyChanges { target: txtAppVersion; opacity: 1.0 }

            /* Game elements anchors */
            AnchorChanges { target: toolBar; anchors.top: screen.bottom }
        }
    ]

    transitions: [
        Transition {
            from: "stateMainMenu"
            to: "stateGame"
            SequentialAnimation {
                AnchorAnimation {
                    targets: [
                        gameTitle,
                        btnClassic,
                        btnAbout
                    ]
                    duration: 500;
                    easing.type: Easing.InOutQuad
                }

                ScriptAction {
                    script: {
                        setBackgroundSource();
                        if (gameBoard.hasSave()) {
                            dlgLoadSave.show();
                        } else {
                            gameBoard.newGame();
                        }
                    }
                }

                ScriptAction { script: txtAppVersion.opacity = 0.0 }
                PropertyAction { target: bgrMainMenu; property: "visible"; value: false }
                PropertyAction { target: gameBoard; property: "opacity"; value: 1.0 }
                AnchorAnimation {
                    duration: 500;
                    targets: gameBoard;
                }

                ParallelAnimation {
                    PropertyAction { target: scoreBox; property: "state"; value: "stateNormal" }
                    AnchorAnimation { targets: toolBar; duration: 200 }
                }
            }
        },
        Transition {
            from: "stateMainMenu"
            to: "stateSettings"
            AnchorAnimation { duration: 500; easing.type: Easing.InOutQuad }
            PropertyAction { target: scoreBox; property: "state"; value: "stateHidden" }
            ScriptAction { script: txtAppVersion.opacity = 0.0 }
        },
        Transition {
            from: "stateSettings"
            to: "stateMainMenu"
            SequentialAnimation {
                ScriptAction { script: dlgSettings.opacity = 0.0 }
                ScriptAction { script: txtAppVersion.opacity = 1.0 }
                AnchorAnimation { duration: 500; easing.type: Easing.InOutQuad }
            }
        },
        Transition {
            from: "stateGame"
            to: "stateMainMenu"
            SequentialAnimation {
                ScriptAction {
                    script: {
                        if (!gameBoard.gameLost)
                            gameBoard.saveBoardStateToFile();
                    }
                }
                ScriptAction { script: txtAppVersion.opacity = 1.0 }
                PropertyAction { target: gameBoard; property: "opacity"; value: 0.0 }
                PropertyAction { target: scoreBox; property: "state"; value: "stateHidden" }
                AnchorAnimation { targets: toolBar; duration: 400; }
                PropertyAction { target: bgrMainMenu; property: "visible"; value: true }
                AnchorAnimation {
                    targets: [
                        gameTitle,
                        btnClassic,
                        btnAbout
                    ]
                    duration: 500;
                    easing.type: Easing.InOutQuad
                }
//                ScriptAction {
//                    script: {
//                        gameBoard.clearBoard();
//                        pbLevelProgress.minimum = 0;
//                        pbLevelProgress.maximum = gameBoard.levelCap(1);
//                    }
//                }
            }
        }
    ]

}
