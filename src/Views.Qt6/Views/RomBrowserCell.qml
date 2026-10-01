/*
 * Copyright (c) 2026, Mupen64 Organization (https://github.com/mupen64)
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
pragma ComponentBehavior: Bound

import QtQml
import QtQuick
import QtQuick.Controls

import Core

Rectangle {
    id: root
    // external
    required property TableView table
    required property EmuContext context

    // from model
    required property string display
    required property string romPath
    required property var regionCode

    // from TableView
    required property int row
    required property int column
    required property bool selected

    implicitWidth: children[1].implicitWidth + 20
    implicitHeight: children[1].implicitHeight + 10

    color: {
        const activePalette = Window.window.palette.active;
        if (selected)
            return activePalette.highlight;
        return (row % 2 == 0)? activePalette.base : activePalette.alternateBase;
    }

    MouseArea {
        anchors.fill: parent
        z: 1000
        onClicked: {
            const index = root.table.index(root.row, root.column);
            table.selectionModel.select(index, ItemSelectionModel.ClearAndSelect | ItemSelectionModel.Rows);
        }
        onDoubleClicked: {
            context.startROM(root.romPath);
        }
    }
}
