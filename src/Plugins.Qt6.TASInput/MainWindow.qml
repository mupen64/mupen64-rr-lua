import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

ApplicationWindow {
    id: root
    visible: true
    //% "TASInput"
    title: qsTrId("tasinput.appName")

    // ensure window fits content
    minimumWidth: rootItem.implicitWidth + leftPadding + rightPadding
    minimumHeight: rootItem.implicitHeight + topPadding + bottomPadding

    // do not allow window to be closed normally
    onClosing: function(close) {
        close.accepted = false;
    }

    width: 300
    height: 200

    ColumnLayout {
        id: rootItem
        JoystickSection {}
    }
}
