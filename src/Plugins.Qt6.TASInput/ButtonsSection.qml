import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

GridLayout {
    uniformCellWidths: true
    uniformCellHeights: true

    // trigger buttons
    TASButton {
        Layout.column: 0
        Layout.row: 0
        Layout.columnSpan: 3

        text: "L"
    }
    TASButton {
        Layout.column: 3
        Layout.row: 0
        Layout.columnSpan: 3

        text: "Z"
    }
    TASButton {
        Layout.column: 6
        Layout.row: 0
        Layout.columnSpan: 3

        text: "R"
    }
    // text: "\u25B2" // up
    // text: "\u25C0" // left
    // text: "\u25B6" // right
    // text: "\u25BC" // down


    // d-pad
    TASButton {
        Layout.column: 1
        Layout.row: 1

        text: "\u25B2" // up
    }
    TASButton {
        Layout.column: 0
        Layout.row: 2

        text: "\u25C0" // left
    }
    TASButton {
        Layout.column: 2
        Layout.row: 2

        text: "\u25B6" // right
    }
    TASButton {
        Layout.column: 1
        Layout.row: 3

        text: "\u25BC" // down
    }
    TASPadLabel {
        Layout.column: 1
        Layout.row: 2

        text: "D"
    }

    // start
    TASButton {
        Layout.column: 4
        Layout.row: 2

        text: "S"
    }

    // c-buttons
    TASButton {
        Layout.column: 7
        Layout.row: 1

        text: "\u25B2" // up
    }
    TASButton {
        Layout.column: 6
        Layout.row: 2

        text: "\u25C0" // left
    }
    TASButton {
        Layout.column: 8
        Layout.row: 2

        text: "\u25B6" // right
    }
    TASButton {
        Layout.column: 7
        Layout.row: 3

        text: "\u25BC" // down
    }
    TASPadLabel {
        Layout.column: 7
        Layout.row: 2

        text: "C"
    }

    // a & b
    TASButton {
        Layout.column: 5
        Layout.row: 3

        text: "B"
    }
    TASButton {
        Layout.column: 6
        Layout.row: 4

        text: "A"
    }
}
