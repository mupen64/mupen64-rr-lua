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
            model: root.actions
            // Update settings whenever action shortcut is changed.
            // This cannot be done using traditional property bindings.
            delegate: Connections {
                required property Action modelData
                target: modelData
                function onShortcutChanged() {
                    // update settings from the shortcut
                    if (modelData instanceof HeldAction) {
                        // heldShortcut takes priority
                        settings.setValue(modelData.objectName, (modelData as HeldAction).heldShortcut);
                    } else {
                        // otherwise store normal shortcut
                        settings.setValue(modelData.objectName, modelData.shortcut);
                    }
                }
                function onHeldShortcutChanged() {
                    onShortcutChanged();
                }
            }

            onObjectAdded: (index, obj) => {
                // perform initial update from settings
                obj.shortcut = settings.value(obj.objectName, null);
                priv._bindList.splice(index, 0, obj);
            }
            onObjectRemoved: (index, obj) => priv._bindList.splice(index, 1)
        }
        property list<QtObject> _bindList
    }
    required property string settingsCategory
    default property list<Action> actions
}
