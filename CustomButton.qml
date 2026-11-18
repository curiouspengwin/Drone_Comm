import QtQuick 2.15

Rectangle{
    id: root
    width: 100
    height: 30
    color: "#ECEFF1"
    border.color: "#B0BEC5"

    property alias text: label.text
    signal clicked()

    Text {
        anchors.centerIn:root
        id: label
        text: qsTr("Stop Leader Drone")
    }

    MouseArea{
        anchors.fill: parent
        onClicked: root.clicked()
    }
}
