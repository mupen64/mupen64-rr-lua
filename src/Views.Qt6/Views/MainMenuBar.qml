/*
 * Copyright (c) 2026, Mupen64 Organization (https://github.com/mupen64)
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Controls

MenuBar {
    id: root
    required property SettingsActions actions

    // True if the menu has been opened in any capacity.
    readonly property bool opened: priv.opened

    QtObject {
        id: priv
        property bool opened: false
        function findBaseItemIndex(menu, target) {
            return menu.contentChildren.findIndex(item => item == target) + 1
        }
    }

    // FIXME (MacOS): May not work as intended with native menus.
    delegate: MenuBarItem {
        id: item
        Connections {
            target: item.menu
            enabled: item.menu != null
            function onAboutToShow() {
                item.updateOpened();
            }
            function onClosed() {
                item.updateOpened();
            }
        }
        function updateOpened() {
            priv.opened = root.menus.some(child => child.visible);
        }
    }

    Menu {
        title: qsTr("File")
        MenuItem { action: root.actions.get("file/loadROM") }
        MenuItem { action: root.actions.get("file/closeROM") }
        MenuItem { action: root.actions.get("file/resetROM") }
    }
    Menu {
        title: qsTr("Emulation")
        MenuItem { action: root.actions.get("emu/pause") }
        MenuItem { action: root.actions.get("emu/speedDown") }
        MenuItem { action: root.actions.get("emu/speedUp") }
        MenuItem { action: root.actions.get("emu/speedReset") }
        MenuItem { action: root.actions.get("emu/gsButton") }
        MenuSeparator {}
        MenuItem { action: root.actions.get("emu/advance") }
        MenuItem { action: root.actions.get("emu/multiAdvance") }
        MenuItem { action: root.actions.get("emu/multiAdvanceAdd") }
        MenuItem { action: root.actions.get("emu/multiAdvanceSub") }
        MenuItem { action: root.actions.get("emu/multiAdvanceReset") }
        Menu {
            id: menuSaveState
            title: qsTr("Save State")
            MenuItem { action: root.actions.get("emu/saveCurrentSlot") }
            MenuItem { action: root.actions.get("emu/saveFile") }
            MenuSeparator {
                id: sepSaveSlots
            }
            Instantiator {
                model: 10

                delegate: MenuItem {
                    required property int index
                    action: root.actions.get(`emu/saveSlotN/${index + 1}`)
                }

                onObjectAdded: (index, object) => {
                    // ensure object is added to correct position relative to separator
                    let baseIndex = priv.findBaseItemIndex(menuSaveState, sepSaveSlots);
                    menuSaveState.insertItem(baseIndex + index, object);
                }
                onObjectRemoved: (index, object) => menuSaveState.removeItem(object)
            }
        }
        Menu {
            id: menuLoadState
            title: qsTr("Load State")
            MenuItem { action: root.actions.get("emu/loadCurrentSlot") }
            MenuItem { action: root.actions.get("emu/loadFile") }
            MenuSeparator {
                id: sepLoadSlots
            }
            Instantiator {
                model: 10

                delegate: MenuItem {
                    required property int index
                    action: root.actions.get(`emu/loadSlotN/${index + 1}`)
                }

                onObjectAdded: (index, object) => {
                    // ensure object is added to correct position relative to separator
                    let baseIndex = priv.findBaseItemIndex(menuLoadState, sepLoadSlots);
                    menuLoadState.insertItem(baseIndex + index, object);
                }
                onObjectRemoved: (index, object) => menuLoadState.removeItem(object)
            }
        }
        MenuSeparator {}
        Menu {
            id: menuCurrSlot
            title: qsTr("Current State Slot")

            Instantiator {
                model: 10

                delegate: MenuItem {
                    required property int index
                    action: root.actions.get(`emu/setCurrentSlot/${index + 1}`)
                }

                onObjectAdded: (index, object) => menuCurrSlot.insertItem(index, object)
                onObjectRemoved: (index, object) => menuCurrSlot.removeItem(object)
            }
        }
    }
    Menu {
        title: qsTr("Options")

        MenuItem { action: root.actions.get("opts/settings") }
    }
}
