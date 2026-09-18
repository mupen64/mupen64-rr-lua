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

// Crude class that manages actions and syncs shortcuts to the settings window.
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
            delegate: QtObject {
                id: bindHandler
                required property EmuAction modelData

                readonly property Component _actionConn: Connections {
                    target: bindHandler.modelData
                    function onShortcutChanged() {
                        bindHandler.update();
                    }
                }

                readonly property Component _heldActionConn: Connections {
                    target: bindHandler.modelData
                    function onHeldShortcutChanged() {
                        bindHandler.update();
                    }
                }

                readonly property list<QtObject> _conns: {
                    // instantiate bindings as needed by the modelData
                    let result = [_actionConn.createObject(null, {})];
                    if (modelData instanceof EmuHeldAction)
                        result.push(_heldActionConn.createObject(null, {}));
                    return result;
                }

                function update() {
                    if (!(modelData instanceof EmuAction))
                        return;

                    // update settings from the shortcut
                    let action = modelData as EmuAction;
                    let shortcut = (action instanceof EmuHeldAction) ?
                        (action as EmuHeldAction).heldShortcut :
                        action.shortcut;

                    settings.setValue(action.key, JSON.stringify(shortcut));
                }
            }

            onObjectAdded: (_index, obj) => {
                // perform initial update from settings
                if (!(obj.modelData instanceof EmuAction))
                    return;

                let action = obj.modelData as EmuAction;
                let isAction = false;

                let setShortcut = settings.value(action.key, null);
                let shortcut = (setShortcut == null) ? action.defaultShortcut : JSON.parse(setShortcut);

                if (action instanceof EmuHeldAction)
                    (action as EmuHeldAction).heldShortcut = shortcut;
                else
                    action.shortcut = shortcut;

                priv.bindSet.add(obj);
            }
            onObjectRemoved: (_index, obj) => {
                // remove from binding list
                priv.bindSet.delete(obj);
            }
        }
        property var bindSet: new Set()

        property var registerInst: Instantiator {
            model: root.children

            delegate: QtObject {
                required property QtObject modelData
            }

            onObjectAdded: (_index, obj) => {
                if (!(obj.modelData instanceof EmuAction))
                    return;
                let action = obj.modelData as EmuAction;

                if (priv.registerMap.has(action.key))
                    throw new Error(`Key ${action.key} is already present. Keys should be unique.`);
                priv.registerMap.set(action.key, action);
            }
            onObjectRemoved: (_index, obj) => {
                if (!(obj.modelData instanceof EmuAction))
                    return;
                let action = obj.modelData as EmuAction;

                priv.registerMap.delete(action.key);
            }
        }
        property var registerMap: new Map()
    }
    required property string settingsCategory
    default property list<QtObject> children

    function get(key) {
        return priv.registerMap.get(key);
    }
}
