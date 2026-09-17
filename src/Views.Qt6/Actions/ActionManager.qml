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
                    // update settings from the shortcut
                    if (modelData instanceof EmuHeldAction) {
                        let action = modelData as EmuHeldAction
                        settings.setValue(action.key, action.heldShortcut);
                    } else if (modelData instanceof EmuAction) {
                        let action = modelData as EmuAction
                        settings.setValue(action.key, action.shortcut);
                    }
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
