/*
 * Copyright (c) 2026, Mupen64 Organization (https://github.com/mupen64)
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls
import QtQuick.Dialogs as Dialogs

import Utils
import Components
import Config as Config

DelegateChooser {
    id: root
    enum Type {
        Bool = 0,
        Choices,
        Int,
        Double,
        FolderPath
    }

    property QtObject _DelegateChooser_priv: QtObject {
        id: priv

        function keyName(key: string): string {
            let lastDot = key.lastIndexOf(".");
            if (lastDot === -1 || lastDot === key.length - 1)
                return "";
            else
                return key.substring(lastDot + 1);
        }
    }

    required property QtObject dataSource
    property double itemWidth

    role: "type"

    DelegateChoice {
        id: boolChoice
        roleValue: ListPageItem.Bool
        Config.Row {
            id: boolRow
            required property string key
            readonly property string keyName: priv.keyName(key)

            name: qsTrId(key)
            width: root.itemWidth

            Switch {
                checked: root.dataSource[boolRow.keyName]
                onClicked: root.dataSource[boolRow.keyName] = checked
            }
        }
    }
    DelegateChoice {
        roleValue: ListPageItem.Choices
        Config.Row {
            id: choicesRow
            required property string key
            required property var choices
            readonly property string keyName: priv.keyName(key)

            name: qsTrId(key)
            width: root.itemWidth

            ComboBox {
                model: choicesRow.choices
                textRole: "text"
                valueRole: "value"

                currentValue: root.dataSource[choicesRow.keyName]
                onActivated: root.dataSource[choicesRow.keyName] = currentValue
            }
        }
    }
    DelegateChoice {
        roleValue: ListPageItem.Int
        Config.Row {
            id: intRow
            required property string key
            required property int from
            required property int to
            property int stepSize: 1
            readonly property string keyName: priv.keyName(key)

            name: qsTrId(key)
            width: root.itemWidth

            SpinBox {
                from: intRow.from
                to: intRow.to
                stepSize: intRow.stepSize

                value: root.dataSource[intRow.keyName]
                onValueModified: root.dataSource[intRow.keyName] = value
            }
        }
    }
    DelegateChoice {
        roleValue: ListPageItem.Double
        Config.Row {
            id: doubleRow
            required property string key
            required property double from
            required property double to
            property double stepSize: 1.0
            property int decimals: 2
            readonly property string keyName: priv.keyName(key)

            name: qsTrId(key)
            width: root.itemWidth

            FixedPointSpinBox {
                dFrom: doubleRow.from
                dTo: doubleRow.to
                dStepSize: doubleRow.stepSize
                decimals: doubleRow.decimals

                dValue: root.dataSource[doubleRow.keyName]
                onValueModified: root.dataSource[doubleRow.keyName] = dValue
            }
        }
    }
    DelegateChoice {
        roleValue: ListPageItem.FolderPath
        Config.PathBox {
            id: folderBox
            required property string key
            required property string dialogTitle
            property string acceptLabel
            property string rejectLabel
            readonly property string keyName: priv.keyName(key)

            width: root.itemWidth
            title: qsTrId(key)
            path: root.dataSource[keyName]
            onOpenDialog: {
                folderDialog.open()
            }

            Dialogs.FolderDialog {
                id: folderDialog
                title: folderBox.dialogTitle
                acceptLabel: folderBox.acceptLabel
                rejectLabel: folderBox.rejectLabel
                onAccepted: {
                    let path = Paths.toLocalFile(folderDialog.selectedFolder)
                    folderBox.setPath(path);
                }
            }
        }
    }
}
