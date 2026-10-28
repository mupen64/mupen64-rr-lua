/*
 * Copyright (c) 2026, Mupen64 Organization (https://github.com/mupen64)
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
pragma ComponentBehavior: Bound

import QtQuick
import Lua

Canvas {
    id: root

    property LuaCanvas canvasObject: null

    Connections {
        target: root.canvasObject
        function onNeedsPaint() {
            root.requestPaint()
        }
        function onCleared() {
            root.requestPaint()
        }
    }

    onPaint: {
        if (!root.canvasObject)
            return

        let ctx = root.getContext("2d")
        root.canvasObject.renderToContext(ctx)
    }
}
