import QtQuick
import QtQuick.Controls
import QtQuick.Shapes

Control {
    id: root
    implicitWidth: 128
    implicitHeight: 128

    property int posX
    property int posY

    signal posXModified()
    signal posYModified()

    contentItem: Rectangle {
        id: content
        color: root.palette.base
        border.color: root.palette.light
        border.width: 1
        radius: 4

        readonly property real centerX: width / 2
        readonly property real centerY: height / 2
        readonly property real radiusX: width / 2
        readonly property real radiusY: height / 2
        readonly property real stickX: centerX + (root.posX / 128.0) * radiusX
        readonly property real stickY: centerY - (root.posY / 128.0) * radiusY

        Shape {
            // circle and crosshair
            ShapePath {
                fillColor: "transparent"
                strokeColor: root.palette.light
                strokeWidth: 1

                PathAngleArc {
                    moveToStart: true
                    centerX: content.centerX
                    centerY: content.centerY
                    radiusX: content.radiusX
                    radiusY: content.radiusY
                    startAngle: 0
                    sweepAngle: 360
                }
                PathMove { x: content.centerX; y: 0 }
                PathLine { x: content.centerX; y: content.height }
                PathMove { x: 0; y: content.centerY }
                PathLine { x: content.width; y: content.centerY }
            }
            // rod from centre to stick position
            ShapePath {
                fillColor: "transparent"
                strokeColor: Color.darker(root.palette.accent, 1.5)
                strokeWidth: 3
                capStyle: ShapePath.RoundCap

                PathMove { x: content.centerX; y: content.centerY }
                PathLine {
                    x: content.stickX; y: content.stickY
                }
            }
            // dot at stick position
            ShapePath {
                fillColor: root.palette.accent
                strokeColor: "transparent"

                PathAngleArc {
                    moveToStart: true
                    centerX: content.stickX
                    centerY: content.stickY
                    radiusX: 4
                    radiusY: 4
                    startAngle: 0
                    sweepAngle: 360
                }
            }
        }

        PointHandler {
            id: pointHandler
        }
    }
}
