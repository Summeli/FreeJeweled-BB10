import QtQuick 1.0

BaseButton {
    borderWidth: 2
    borderColor: "white"
    height: 85//40*gameBoard.cellSize/40
    width: parent.width - 85// 40*gameBoard.cellSize/40
    anchors.horizontalCenter: parent.horizontalCenter
    anchors.margins: 21//10*gameBoard.cellSize/40
    fontSize: 34//16*gameBoard.cellSize/40
}
