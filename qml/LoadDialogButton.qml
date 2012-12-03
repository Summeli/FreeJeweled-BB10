import QtQuick 1.0

BaseButton {
    borderWidth: 2
    borderColor: "white"
    height: 40*gameBoard.cellSize/40
    width: parent.width - 40*gameBoard.cellSize/40
    anchors.horizontalCenter: parent.horizontalCenter
    anchors.margins: 10*gameBoard.cellSize/40
    fontSize: 16*gameBoard.cellSize/40
}
