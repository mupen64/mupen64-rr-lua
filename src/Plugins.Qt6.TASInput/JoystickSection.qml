/*
 * Copyright (c) 2026, Mupen64 Organization (https://github.com/mupen64)
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

GridLayout {
    id: root
    property int posX
    property int posY

    QtObject {
        id: priv
        readonly property IntValidator stickValidator: IntValidator {
            bottom: -128
            top: 127
        }

        function computeIncrement(base: int, modifiers): int {
            if ((modifiers & Qt.ControlModifier) != 0)
                base *= 2;
            if ((modifiers & Qt.AltModifier) != 0)
                base *= 4;
            return base;
        }
    }

    Joystick {
        id: joystick
        Layout.column: 0
        Layout.row: 0
        Layout.rowSpan: 2
        Layout.fillHeight: true

        Layout.minimumWidth: height

        posX: root.posX
        posY: root.posY

        onPosModified: {
            root.posX = posX;
            root.posY = posY;
        }
    }
    GridLayout {
        Layout.column: 1
        Layout.row: 0

        Label {
            Layout.column: 0
            Layout.row: 0
            Layout.columnSpan: 2

            //% "Sensitivity"
            text: qsTrId("tasinput.sensitivity")
        }
        Label {
            Layout.column: 0
            Layout.row: 1

            text: "X"
        }
        Slider {
            Layout.column: 1
            Layout.row: 1
            Layout.fillWidth: true
        }
        Label {
            Layout.column: 0
            Layout.row: 2

            text: "Y"
        }
        Slider {
            Layout.column: 1
            Layout.row: 2
            Layout.fillWidth: true
        }
    }
    RowLayout {
        Layout.column: 1
        Layout.row: 1

        GridLayout {
            Label {
                Layout.column: 0
                Layout.row: 0
                Layout.columnSpan: 2

                //% "Position"
                text: qsTrId("tasinput.position")
            }
            Label {
                Layout.column: 0
                Layout.row: 1

                text: "X"
            }
            TextField {
                Layout.column: 1
                Layout.row: 1
                Layout.fillWidth: true

                text: `${root.posX}`
                validator: priv.stickValidator
                onEditingFinished: root.posX = parseInt(text)
            }
            Label {
                Layout.column: 0
                Layout.row: 2

                text: "Y"
            }
            TextField {
                Layout.column: 1
                Layout.row: 2
                Layout.fillWidth: true

                text: `${root.posY}`
                validator: priv.stickValidator
                onEditingFinished: root.posY = parseInt(text)
            }
        }

        GridLayout {
            Button {
                Layout.row: 0
                Layout.column: 1
                implicitWidth: implicitHeight
                text: "\u25B2" // up

                TapHandler {
                    onTapped: {
                        let nextPosY = root.posY + priv.computeIncrement(1, point.modifiers);
                        root.posY = Math.min(Math.max(nextPosY, -128), 127);
                    }
                }
            }
            Button {
                Layout.row: 1
                Layout.column: 0
                implicitWidth: implicitHeight
                text: "\u25C0" // left

                TapHandler {
                    onTapped: {
                        let nextPosX = root.posX + priv.computeIncrement(-1, point.modifiers);
                        root.posX = Math.min(Math.max(nextPosX, -128), 127);
                    }
                }
            }
            Button {
                Layout.row: 1
                Layout.column: 1
                implicitWidth: implicitHeight
                text: "\u25CF" // reset

                TapHandler {
                    onTapped: {
                        root.posX = 0;
                        root.posY = 0;
                    }
                }
            }
            Button {
                Layout.row: 1
                Layout.column: 2
                implicitWidth: implicitHeight
                text: "\u25B6" // right

                TapHandler {
                    onTapped: {
                        let nextPosX = root.posX + priv.computeIncrement(1, point.modifiers);
                        root.posX = Math.min(Math.max(nextPosX, -128), 127);
                    }
                }
            }
            Button {
                Layout.row: 2
                Layout.column: 1
                implicitWidth: implicitHeight
                text: "\u25BC" // down

                TapHandler {
                    onTapped: {
                        let nextPosY = root.posY + priv.computeIncrement(-1, point.modifiers);
                        root.posY = Math.min(Math.max(nextPosY, -128), 127);
                    }
                }
            }
        }
    }
}
