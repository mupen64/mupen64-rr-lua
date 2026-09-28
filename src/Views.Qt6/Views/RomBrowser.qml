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

Item {
    id: root

    required property RomManager romManager

    HorizontalHeaderView {
        id: topHeader
        anchors.top: root.top
        anchors.left: root.left
        anchors.right: root.right

        syncView: table

    }
    TableView {
        id: table
        anchors.top: topHeader.bottom
        anchors.left: root.left
        anchors.right: root.right
        anchors.bottom: root.bottom

        model: root.romManager

        selectionBehavior: TableView.SelectRows
        selectionMode: TableView.SingleSelection
        selectionModel: ItemSelectionModel {
            model: table.model
        }

        delegate: Rectangle {
            id: cellRoot
            required property string display
            required property int row
            required property int column
            required property bool selected
            required property bool current

            implicitWidth: cellLabel.implicitWidth + 10
            implicitHeight: cellLabel.implicitHeight + 10

            color: {
                const activePalette = Window.window.palette.active;
                if (selected)
                    return activePalette.highlight;
                return (row % 2 == 0)? activePalette.base : activePalette.alternateBase;
            }
            onSelectedChanged: {
                console.log(`index ${row}, ${column}: selected -> ${selected}`);
            }

            Label {
                id: cellLabel
                anchors.centerIn: parent
                text: cellRoot.display
            }

            MouseArea {
                onClicked: {
                    console.log(`clicked ${cellRoot.row}, ${cellRoot.column}`);
                    const index = table.index(cellRoot.row, cellRoot.column);
                    table.selectionModel.select(index, ItemSelectionModel.ClearAndSelect | ItemSelectionModel.Rows);
                }
            }
        }
    }
}
