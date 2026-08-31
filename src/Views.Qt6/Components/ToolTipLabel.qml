import QtQuick
import QtQuick.Controls

Label {
    property string tooltip

    HoverHandler { id: hoverHandler }

    ToolTip.visible: hoverHandler.hovered
    ToolTip.text: tooltip
}
