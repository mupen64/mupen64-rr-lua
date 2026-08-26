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
    id: dialog
    modal: true
    popupType: Popup.Window

    property int coreType
    property string content

    contentItem: RowLayout {
        id: root
        onVisibleChanged: {
            // Lock the created window's width/height when it is displayed.
            if (visible && Window.window !== null) {
                let window = Window.window;
                window.minimumWidth = Qt.binding(() => {
                    return Math.max(root.implicitWidth, 256);
                });
                window.minimumHeight = Qt.binding(() => {
                    let implicitTotalHeight = root.implicitHeight + dialog.header.implicitHeight + dialog.footer.implicitHeight;
                    return Math.max(implicitTotalHeight, 128);
                });
                window.maximumWidth = Qt.binding(() => window.minimumWidth);
                window.maximumHeight = Qt.binding(() => window.minimumHeight);
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
                    // Theme icons come from QtIconImageProvider (in Utils), deferring directly
                    // to QIcon::fromTheme. Qt uses the XDG specification for icon names:
                    // https://specifications.freedesktop.org/icon-naming/latest/
                    switch (dialog.coreType) {
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

            text: dialog.content
        }
    }
}
