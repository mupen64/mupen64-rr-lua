import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

ApplicationWindow {
    id: root
    visible: true
    //% "TASInput"
    title: qsTrId("tasinput.appName")

    required property int port
    required property var backend

    // ensure window fits content
    minimumWidth: rootItem.implicitWidth + leftPadding + rightPadding
    minimumHeight: rootItem.implicitHeight + topPadding + bottomPadding

    topPadding: 5
    leftPadding: 5
    rightPadding: 5
    bottomPadding: 5

    // do not allow window to be closed normally
    onClosing: function (close) {
        close.accepted = false;
    }

    width: 300
    height: 200

    ColumnLayout {
        id: rootItem
        JoystickSection {
            id: joystick
        }
        ButtonsSection {
            id: buttons
        }
    }

    Connections {
        target: root.backend
        function onRequestInput(port: int) {
            if (port != root.port)
                return;

            // Yes, this is comically inefficient.
            // It'll work though.
            root.backend.receiveInput(port, {
                x: joystick.x,
                y: joystick.y,
                btnL: buttons.stateL,
                btnZ: buttons.stateZ,
                btnR: buttons.stateR,
                btnDU: buttons.stateDU,
                btnDL: buttons.stateDL,
                btnDR: buttons.stateDR,
                btnDD: buttons.stateDD,
                btnS: buttons.stateS,
                btnCU: buttons.stateCU,
                btnCL: buttons.stateCL,
                btnCR: buttons.stateCR,
                btnCD: buttons.stateCD,
                btnB: buttons.stateB,
                btnA: buttons.stateA
            });
        }
    }
}
