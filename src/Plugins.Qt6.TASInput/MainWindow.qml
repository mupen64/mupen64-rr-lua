import QtQuick
import QtQuick.Controls

Window {
    id: mainWindow
    visible: true
    //% "TASInput"
    title: qsTrId("tasinput.appName")

    // do not allow window to be closed normally
    onClosing: function(close) {
        close.accepted = false;
    }

    width: 300
    height: 200
}
