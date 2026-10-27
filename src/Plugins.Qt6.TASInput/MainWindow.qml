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

    property var backend
    property int port: 0

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
        enabled: root.backend != null
        target: root.backend
        function onRequestInput(port: int) {
            if (port != root.port)
                return;

            // buttons need to be updated
            buttons.doPollUpdate();

            // Yes, this is comically inefficient, but it's the easiest way to send all
            // this data over the FFI barrier.
            root.backend.receiveInput(port, {
                x: joystick.posX,
                y: joystick.posY,
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
