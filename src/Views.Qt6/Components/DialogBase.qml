import QtQuick
import QtQuick.Controls

Dialog {
    id: dialog
    popupType: Popup.Window
    modal: true

    property bool windowResizable

    Connections {
        enabled: dialog.contentItem != null
        target: dialog.contentItem

        function onVisibleChanged() {
            let window = dialog.contentItem.Window.window;
            if (!dialog.contentItem.visible || window == null) return;

            window.minimumWidth = dialog.width;
            window.minimumHeight = dialog.height;

            if (!windowResizable) {
                window.maximumWidth = Qt.binding(() => window.minimumWidth);
                window.maximumHeight = Qt.binding(() => window.minimumHeight);
            }
        }
    }
}
