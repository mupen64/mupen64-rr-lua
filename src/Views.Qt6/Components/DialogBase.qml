import QtQuick
import QtQuick.Controls

Dialog {
    id: dialog
    popupType: Popup.Window
    modal: true

    width: {
        function updateWidth(curr, item) {
            if (item != null && item.implicitWidth > curr)
                return item.implicitWidth;
            else
                return curr;
        }
        return [dialog.header, dialog.contentItem, dialog.footer]
            .reduce(updateWidth, minimumWidth);
    }
    height: {
        function updateHeight(curr, item) {
            if (item != null)
                return curr + item.implicitHeight;
            else
                return curr;
        }
        return [dialog.header, dialog.contentItem, dialog.footer]
            .reduce(updateHeight, minimumHeight);
    }

    property int minimumWidth: 0
    property int minimumHeight: 0

    property bool windowResizable: true

    Connections {
        enabled: contentItem != null
        target: contentItem

        function onVisibleChanged() {
            let window = contentItem.Window.window;
            if (!contentItem.visible || window == null) return;

            console.log(`fire in the hole ${window}`);

            window.minimumWidth = dialog.width;
            window.minimumHeight = dialog.height;

            if (!windowResizable) {
                window.maximumWidth = Qt.binding(() => window.minimumWidth);
                window.maximumHeight = Qt.binding(() => window.minimumHeight);
            }
        }
    }
}
