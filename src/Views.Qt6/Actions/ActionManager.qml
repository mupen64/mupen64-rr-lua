/*
 * Copyright (c) 2026, Mupen64 Organization (https://github.com/mupen64)
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
pragma ComponentBehavior: Bound

import QtCore
import QtQuick
import QtQml.Models
import QtQuick.Controls

// Crude class that manages actions.
// Actions are keyed by their Qt `objectName` property.
QtObject {
    id: root

    property QtObject _ActionManager_priv: QtObject {
        id: priv

        property var _settings: Settings {
            id: settings
            category: root.settingsCategory
        }
        property var _bindInst: Instantiator {
            model: root.children
            // Update settings whenever action shortcut is changed.
            // This cannot be done using traditional property bindings.
            delegate: Connections {
                required property Action modelData
                target: modelData
                function onShortcutChanged() {
                    // update settings from the shortcut
                    if (modelData instanceof EmuHeldAction) {
                        let action = modelData as EmuHeldAction
                        settings.setValue(action.key, action.heldShortcut);
                    } else if (modelData instanceof EmuAction) {
                        let action = modelData as EmuAction
                        settings.setValue(action.key, action.shortcut);
                    }
                }
                function onHeldShortcutChanged() {
                    onShortcutChanged();
                }
            }

            onObjectAdded: (_index, obj) => {
                // perform initial update from settings
                let data = obj.modelData;
                let isAction = false;
                if (data instanceof EmuHeldAction) {
                    let heldAction = data as EmuHeldAction;
                    heldAction.heldShortcut = settings.value(heldAction.key, heldAction.defaultShortcut);
                    isAction = true;
                } else if (data instanceof EmuAction) {
                    let action = data as EmuAction;
                    action.shortcut = settings.value(action.key, action.defaultShortcut);
                    isAction = true;
                }
                priv._bindSet.add(obj);
            }
            onObjectRemoved: (_index, obj) => {
                // remove from binding list
                priv._bindSet.delete(obj);
            }
        }
        property var _bindSet: new Set()
    }
    required property string settingsCategory
    default property list<QtObject> children
}
