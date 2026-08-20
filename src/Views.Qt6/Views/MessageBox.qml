/*
 * Copyright (c) 2026, Mupen64 Organization (https://github.com/mupen64)
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import Core

Dialog {
    id: diaMessage
    modal: true
    popupType: Popup.Window

    property int coreType
    property string content

    contentItem: RowLayout {
        Window.onWindowChanged: {
            // Lock the created window's width/height when it is displayed.
            if (Window.window !== null && Window.window !== mainWindow) {
                Window.window.width = width
                Window.window.height = height

                Window.window.minimumWidth = Window.window.width
                Window.window.maximumWidth = Window.window.width
                Window.window.minimumHeight = Window.window.height
                Window.window.maximumHeight = Window.window.height
            }
        }

        spacing: 16
        Item {
            Layout.minimumWidth: 32
            Layout.fillHeight: true

            Image {
                anchors.centerIn: parent
                width: 32
                height: 32

                source: (() => {
                    switch (diaMessage.coreType) {
                        case CoreDialogType.Error:
                            return "image://icons/dialog-error";
                        case CoreDialogType.Warning:
                            return "image://icons/dialog-warning";
                        case CoreDialogType.Information:
                        default:
                            return "image://icons/dialog-information";
                    }
                })()
            }
        }
        Label {
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.minimumWidth: 256
            
            verticalAlignment: Text.AlignVCenter

            text: diaMessage.content
        }
    }
}