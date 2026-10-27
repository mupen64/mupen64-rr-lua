import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

GridLayout {
    id: root
    uniformCellWidths: true
    uniformCellHeights: true

    property bool stateL
    property bool stateZ
    property bool stateR

    property bool stateDU
    property bool stateDL
    property bool stateDR
    property bool stateDD

    property bool stateS

    property bool stateCU
    property bool stateCL
    property bool stateCR
    property bool stateCD

    property bool stateB
    property bool stateA

    function doPollUpdate() {
        btnL.doPollUpdate();
        btnZ.doPollUpdate();
        btnR.doPollUpdate();

        btnDU.doPollUpdate();
        btnDL.doPollUpdate();
        btnDR.doPollUpdate();
        btnDD.doPollUpdate();

        btnS.doPollUpdate();

        btnCU.doPollUpdate();
        btnCL.doPollUpdate();
        btnCR.doPollUpdate();
        btnCD.doPollUpdate();

        btnB.doPollUpdate();
        btnA.doPollUpdate();
    }

    // trigger buttons
    TASButton {
        id: btnL
        Layout.column: 0
        Layout.row: 0
        Layout.columnSpan: 3

        label: "L"
        checked: root.stateL
        onToggled: root.stateL = checked
    }
    TASButton {
        id: btnZ
        Layout.column: 3
        Layout.row: 0
        Layout.columnSpan: 3

        label: "Z"
        checked: root.stateZ
        onToggled: root.stateZ = checked
    }
    TASButton {
        id: btnR
        Layout.column: 6
        Layout.row: 0
        Layout.columnSpan: 3

        label: "R"
        checked: root.stateR
        onToggled: root.stateR = checked
    }

    // d-pad
    TASButton {
        id: btnDU
        Layout.column: 1
        Layout.row: 1

        label: "\u25B2" // up
        checked: root.stateDU
        onToggled: root.stateDU = checked
    }
    TASButton {
        id: btnDL
        Layout.column: 0
        Layout.row: 2

        label: "\u25C0" // left
        checked: root.stateDL
        onToggled: root.stateDL = checked
    }
    TASButton {
        id: btnDR
        Layout.column: 2
        Layout.row: 2

        label: "\u25B6" // right
        checked: root.stateDR
        onToggled: root.stateDR = checked
    }
    TASButton {
        id: btnDD
        Layout.column: 1
        Layout.row: 3

        label: "\u25BC" // down
        checked: root.stateDD
        onToggled: root.stateDD = checked
    }
    TASPadLabel {
        Layout.column: 1
        Layout.row: 2

        text: "D"
    }

    // start
    TASButton {
        id: btnS
        Layout.column: 4
        Layout.row: 2

        label: "S"
        checked: root.stateS
        onToggled: root.stateS = checked
    }

    // c-buttons
    TASButton {
        id: btnCU
        Layout.column: 7
        Layout.row: 1

        label: "\u25B2" // up
        checked: root.stateCU
        onToggled: root.stateCU = checked
    }
    TASButton {
        id: btnCL
        Layout.column: 6
        Layout.row: 2

        label: "\u25C0" // left
        checked: root.stateCL
        onToggled: root.stateCL = checked
    }
    TASButton {
        id: btnCR
        Layout.column: 8
        Layout.row: 2

        label: "\u25B6" // right
        checked: root.stateCR
        onToggled: root.stateCR = checked
    }
    TASButton {
        id: btnCD
        Layout.column: 7
        Layout.row: 3

        label: "\u25BC" // down
        checked: root.stateCD
        onToggled: root.stateCD = checked
    }
    TASPadLabel {
        Layout.column: 7
        Layout.row: 2

        text: "C"
    }

    // a & b
    TASButton {
        id: btnB
        Layout.column: 5
        Layout.row: 3

        label: "B"
        checked: root.stateB
        onToggled: root.stateB = checked
    }
    TASButton {
        id: btnA
        Layout.column: 6
        Layout.row: 4

        label: "A"
        checked: root.stateA
        onToggled: root.stateA = checked
    }
}
