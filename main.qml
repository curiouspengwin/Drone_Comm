import QtQuick 2.15
import QtQuick.Controls 2.15
import com.app.gcsmanager 1.0

ApplicationWindow {
    visible: true
    width: 600
    height: 450
    title: "Drone Swarm GCS Monitor"

    GcsManager {
        id : gcsManager
    }

    Column {
        anchors.centerIn: parent
        spacing: 20

        Text {
            text: "Swarm Telemetry Overview (" + gcsManager.drones.length + " Drones Active)"
            font.bold: true
            font.pixelSize: 18
        }

        // Dynamic Telemtery List
        Repeater {
            model : gcsManager.drones
            delegate: Text{
                text: "Drone " + modelData.id + " | X: " + modelData.x.toFixed(1) + "m, Y: " + modelData.y.toFixed(1) + "m"
                font.pixelSize: 14
                color: "#2C3E50"
            }
        }

        // 2D Top-Down Radar View (X and Y)
        Rectangle {
            width: 450
            height: 180
            color: "#ECEFF1"
            border.color: "#B0BEC5"
            clip: true

            Repeater {
                model: gcsManager.drones
                delegate: Rectangle{
                    x: ((modelData.x % 430) + 430) % 430
                    y: Math.max(0, Math.min(160, 80 + modelData.y))
                    width: 20
                    height: 20
                    radius: 10
                    color: Qt.hsla((modelData.id * 0.22) % 1.0, 0.7, 0.45, 1.0) // Unique color per drone

                    Text {
                        anchors.centerIn: parent
                        text: modelData.id
                        color: "white"
                        font.bold: true
                        font.pixelSize: 11
                    }
                }
            }
        }

        // Dynamic Control Buttons
        // Created every 500ms. Do not want this behaviour
        Row {
            spacing: 10
            Repeater {
                model: gcsManager.droneIds
                delegate: CustomButton {
                    text: "Toggle D" + modelData
                    onClicked: gcsManager.sendCommand(modelData)
                }
            }
        }
    }
}