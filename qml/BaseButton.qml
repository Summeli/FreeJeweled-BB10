/* Base button defines a bunch of properties which later used to customize different derived
buttons. These derived buttons are used later throughout the game. */
import QtQuick 1.0

Rectangle {
    id: button
    color: "white"
    //these dont seem to work well...
    width: buttonLabel.width //+ 42 //20*gameBoard.cellSize/40
    height: buttonLabel.height// + 10 //5*gameBoard.cellSize/40
    radius: button.height/2.1
    smooth: true

    property alias caption: buttonLabel.text
    property alias textColor: buttonLabel.color
    property color borderColor: "transparent"
    property int borderWidth: 0
    property int fontSize: 12
    property bool enabled: true
    signal clicked

    border { width: borderWidth; color: borderColor }

    gradient: Gradient {
        GradientStop {
            position: 0.0;
            color: {
                if (!mouseArea.pressed || !button.enabled)
                    return button.color;
                else
                    return Qt.darker(button.color);
            }
        }
        GradientStop {
            position: 0.2;
            color: {
                if (!mouseArea.pressed || !button.enabled)
                    return Qt.lighter(button.color);
                else
                    return button.color;
            }

        }
        GradientStop {
            position: 1.0;
            color: {
                if (!mouseArea.pressed || !button.enabled)
                    return button.color;
                else
                    return Qt.darker(button.color);
            }
        }
    }

    Text {
        id: buttonLabel
        anchors.centerIn: parent
        font.pointSize: fontSize
        font.family: buttonFont.name
        color: "white"
    }

    MouseArea {
        id: mouseArea
        anchors.fill: parent
        onClicked: {
            if (button.enabled)
                button.clicked()
        }
    }
}
