import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Button {
    id: button
    Layout.fillWidth: true
    Layout.fillHeight: true
    implicitWidth: 1.1 * implicitHeight
    checkable: true

    property string label
    property bool rapidFire

    text: rapidFire? `[${label}]` : label

    function doPollUpdate() {
        if (rapidFire) toggle();
    }

    TapHandler {
        acceptedButtons: Qt.LeftButton | Qt.RightButton

        onTapped: function(point, mouseBtn) {
            if (mouseBtn == Qt.RightButton) {
                button.rapidFire = true;
                button.toggle();
            } else {
                button.rapidFire = false;
            }
        }
    }
}
