import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

RowLayout {
    id: row
    required property string name
    required property string tooltip
    default required property Item control

    Layout.fillWidth: true

    ToolTipLabel {
        Layout.fillWidth: true
        text: "Core type"
        tooltip: "amogus shmamogus"
    }

    onControlChanged: {
        control.parent = row
    }
}
