import QtQuick 1.0
Item {
    id: dlgSettings
    anchors.fill: parent

    property bool canIncreaseSize: false
    property bool canDecreaseSize: false

    signal sizeIncrease
    signal sizeDecrease
    signal backPressed
    signal aboutPressed

    Rectangle {
        anchors.fill: dlgSettings
        color: "black"
        opacity: 0.3
    }

    Item {
        anchors.centerIn: dlgSettings
        width: btnIncSize.width + btnDecSize.width + btnIncSize.anchors.leftMargin

        BaseButton {
            id: btnIncSize
            color: {
                if (btnIncSize.enabled)
                    return "blue";
                else
                    return "gray";
            }
            borderColor: "white"
            borderWidth: 2
            caption: "SIZE +"
            enabled: canIncreaseSize
            fontSize: {
                /* This is workaround for unset g_scaleFactor property */
                if (g_scaleFactor != 0)
                    return 20*g_scaleFactor;
                else
                    return 20;
            }
            anchors { left: btnDecSize.right; leftMargin: 10*g_scaleFactor }
            onClicked: dlgSettings.sizeIncrease()
        }

        BaseButton {
            id: btnDecSize
            color: {
                if (btnDecSize.enabled)
                    return "red";
                else
                    return "gray";
            }
            borderColor: "white"
            borderWidth: 2
            caption: "SIZE -"
            enabled: canDecreaseSize
            fontSize: {
                /* This is workaround for unset g_scaleFactor property */
                if (g_scaleFactor != 0)
                    return 20*g_scaleFactor;
                else
                    return 20;
            }
            onClicked: dlgSettings.sizeDecrease()
        }
    }

    BaseButton {
        id: btnBack
        color: "blue"
        borderColor: "white"
        borderWidth: 2
        caption: "BACK"
        anchors { left: dlgSettings.left; bottom: dlgSettings.bottom; margins: 20*g_scaleFactor }
        fontSize: {
            /* This is workaround for unset g_scaleFactor property */
            if (g_scaleFactor != 0)
                return 20*g_scaleFactor;
            else
                return 20;
        }
        onClicked: dlgSettings.backPressed()
    }

    BaseButton {
        id: btnAbout
        color: "green"
        borderColor: "white"
        borderWidth: 2
        caption: "ABOUT"
        anchors { right: dlgSettings.right; bottom: dlgSettings.bottom; margins: 20*g_scaleFactor }
        fontSize: {
            /* This is workaround for unset g_scaleFactor property */
            if (g_scaleFactor != 0)
                return 20*g_scaleFactor;
            else
                return 20;
        }
        onClicked: dlgSettings.aboutPressed()
    }
}
