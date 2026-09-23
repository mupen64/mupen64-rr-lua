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
    required property ActionManager actions

    QtObject {
        id: priv

        readonly property Component blankMenu: Menu {}
        readonly property Component blankItem: MenuItem {}

        function newMenu(menu: EmuMenu): Menu {
            return blankMenu.createObject(null, {
                title: Qt.binding(() => menu.text)
            }) as Menu;
        }
        function newItem(action: EmuAction): MenuItem {
            return blankItem.createObject(null, {
                action: action
            }) as MenuItem;
        }

        function parentKey(key: string): string {
            let lastDot = key.lastIndexOf(".");
            if (lastDot == -1)
                return "";
            else
                return key.substring(0, lastDot);
        }
    }

    // Rebuilds the menu based on the currently-present actions.
    // This only needs to occur when items or menus are added/removed.
    function rebuildMenu() {
        let menuCache = {};
        let itemCache = {};
        for (const item of actions.children) {
            if (item instanceof EmuMenu) {
                if (item.key in menuCache)
                    throw Error(`menu with name ${item.key} already declared`);
                if (item.key in itemCache)
                    throw Error(`action with name ${item.key} already declared`);

                let viewMenu = priv.newMenu(item);
                menuCache[item.key] = viewMenu;

                let parentKey = priv.parentKey(item.key);
                if (parentKey == "menu") {
                    root.addMenu(viewMenu);
                } else {
                    if (!(parentKey in menuCache))
                        throw Error(`parent menu ${parentKey} does not exist`);
                    menuCache[parentKey].addMenu(viewMenu);
                }
            }
            else if (item instanceof EmuAction) {
                if (item.key in menuCache)
                    throw Error(`menu with name ${item.key} already declared`);
                if (item.key in itemCache)
                    throw Error(`action with name ${item.key} already declared`);

                let viewItem = priv.newItem(item);
                itemCache[item.key] = viewItem;

                let parentKey = priv.parentKey(item.key);
                if (!(parentKey in menuCache))
                    throw Error(`parent menu ${parentKey} does not exist`);

                menuCache[parentKey].addItem(viewItem);
            }
        }
    }
}
