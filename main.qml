import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick3D 1.15
import QtQuick3D.Helpers 1.15
import com.app.gcsmanager 1.0

ApplicationWindow {
    visible: true
    width: 600
    height: 450
    title: "3D Autonomous Swarm GCS (MAVlink + Zenoh)"
    // color: "#1E272E"

    GcsManager {
        id : gcsManager
    }

    // 1. Full-Window 3D Viewport
    View3D {
        id: view3d
        anchors.fill: parent

        environment: SceneEnvironment {
            clearColor: "#1E272E"
            backgroundMode: SceneEnvironment.Color
            antialiasingMode: SceneEnvironment.MSAA
        }

        // Angled overview camera looking down at the flight arena
        PerspectiveCamera {
            id: camera
            position: Qt.vector3d(0, 55, 85)
            eulerRotation.x: -32
        }

        // Allows orbiting/moving the 3D camera with mouse + WASD keys!
        WasdController {
            controlledObject: camera
            speed: 0.5
        }

        DirectionalLight {
            eulerRotation.x: -45
            eulerRotation.y: 30
            brightness: 1.5
        }

        // Ground Plane (Flight Arena Floor)
        Model {
            source: "#Rectangle"
            scale: Qt.vector3d(2.0, 2.0, 1.0)
            eulerRotation.x: -90
            materials: PrincipledMaterial {
                baseColor: "#2F3640"
                roughness: 0.8
            }
        }

        // Dynamic 3D Drone Fleet
        Repeater3D {
            model: gcsManager.drones

            Node {
                // Map Aviation (X, Y, Z-up) -> 3D Graphics (X, Y-up, Z)
                position: Qt.vector3d(modelData.x, modelData.z, modelData.y)

                // Drone 3D Body
                Model {
                    source: "#Sphere"
                    scale: Qt.vector3d(0.03, 0.015, 0.03) // Sleek saucer/drone shape
                    materials: PrincipledMaterial {
                        baseColor: Qt.hsla((modelData.id * 0.22) % 1.0, 0.8, 0.55, 1.0)
                        metalness: 0.3
                        roughness: 0.4
                    }
                }

                // Vertical Altitude Drop-Line down to ground (y = 0)
                Model {
                    source: "#Cylinder"
                    position: Qt.vector3d(0, -modelData.z / 2, 0)
                    scale: Qt.vector3d(0.002, modelData.z / 100.0, 0.002)
                    materials: PrincipledMaterial {
                        baseColor: "#7F8FA6"
                        opacity: 0.5
                    }
                }
            }
        }
    }

    // 2. HUD Overlay (Telemetry & Pause/Resume Controls)
    Rectangle {
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.margins: 16
        width: 310
        height: hudColumn.height + 24
        color: "#CC1E272E"
        border.color: "#485460"
        radius: 8

        Column {
            id: hudColumn
            anchors.centerIn: parent
            spacing: 10

            Text {
                text: "Swarm 3D HUD (" + gcsManager.drones.length + " Drones)"
                color: "white"
                font.bold: true
                font.pixelSize: 16
            }

            Text {
                text: "Click viewport + WASD / Mouse to fly camera"
                color: "#808E9B"
                font.pixelSize: 11
            }

            Repeater {
                model: gcsManager.drones
                delegate: Row {
                    spacing: 10
                    Text {
                        width: 200
                        text: "D" + modelData.id +
                              " (" + modelData.x.toFixed(0) + ", " +
                              modelData.y.toFixed(0) + ", " +
                              modelData.z.toFixed(0) + "m)"
                        color: modelData.active ? "#0BE881" : "#FF5E57"
                        font.pixelSize: 13
                    }
                    CustomButton {
                        width: 75
                        height: 24
                        text: modelData.active ? "Hover" : "Resume"
                        onClicked: gcsManager.sendCommand(modelData.id)
                    }
                }
            }
        }
    }

    // Column {
    //     anchors.centerIn: parent
    //     spacing: 20

    //     Text {
    //         text: "Swarm Telemetry Overview (" + gcsManager.drones.length + " Drones Active)"
    //         font.bold: true
    //         font.pixelSize: 18
    //     }

    //     // Dynamic Telemtery List
    //     Repeater {
    //         model : gcsManager.drones
    //         delegate: Text{
    //             text: "Drone " + modelData.id + " | X: " + modelData.x.toFixed(1) + "m, Y: " + modelData.y.toFixed(1) + "m"
    //             font.pixelSize: 14
    //             color: "#2C3E50"
    //         }
    //     }

    //     // 2D Top-Down Radar View (X and Y)
    //     Rectangle {
    //         width: 450
    //         height: 180
    //         color: "#ECEFF1"
    //         border.color: "#B0BEC5"
    //         clip: true

    //         Repeater {
    //             model: gcsManager.drones
    //             delegate: Rectangle{
    //                 x: ((modelData.x % 430) + 430) % 430
    //                 y: Math.max(0, Math.min(160, 80 + modelData.y))
    //                 width: 20
    //                 height: 20
    //                 radius: 10
    //                 color: Qt.hsla((modelData.id * 0.22) % 1.0, 0.7, 0.45, 1.0) // Unique color per drone

    //                 Text {
    //                     anchors.centerIn: parent
    //                     text: modelData.id
    //                     color: "white"
    //                     font.bold: true
    //                     font.pixelSize: 11
    //                 }
    //             }
    //         }
    //     }

    //     // Dynamic Control Buttons
    //     // Created every 500ms. Do not want this behaviour
    //     Row {
    //         spacing: 10
    //         Repeater {
    //             model: gcsManager.droneIds
    //             delegate: CustomButton {
    //                 text: "Toggle D" + modelData
    //                 onClicked: gcsManager.sendCommand(modelData)
    //             }
    //         }
    //     }
    // }
}