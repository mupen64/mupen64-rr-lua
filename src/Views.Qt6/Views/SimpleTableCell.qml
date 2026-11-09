/*
 * Copyright (c) 2026, Mupen64 Organization (https://github.com/mupen64)
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
pragma ComponentBehavior: Bound

import QtQml
import QtQuick
import QtQuick.Controls

import Mupen64.Core

Rectangle {
    id: root

    // from model
    required property string display

    // from TableView
    required property int row
    required property int column

    implicitWidth: text.implicitWidth + 20
    implicitHeight: text.implicitHeight + 10

    color: {
        if (Window.window == null) return "black";
        const activePalette = Window.window.palette.active;
        return (row % 2 == 0) ? activePalette.base : activePalette.alternateBase;
    }

    Label {
        id: text
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.verticalCenter: parent.verticalCenter
        anchors.leftMargin: 10
        anchors.rightMargin: 10

        text: root.display

        horizontalAlignment: Text.AlignLeft
        verticalAlignment: Text.AlignVCenter
        elide: Text.ElideRight
    }
}
