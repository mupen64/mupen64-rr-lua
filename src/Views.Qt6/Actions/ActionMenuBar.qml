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

    readonly property bool opened: priv.opened

    QtObject {
        id: priv

        property bool opened

        readonly property Component blankMenu: Menu {
            width: {
                let widest = 160.0;
                for (let i = 0; i < count; i++) {
                    let nextItem = itemAt(i);
                    let nextWidth = nextItem.contentItem.implicitWidth + nextItem.leftPadding + nextItem.rightPadding;
                    if (nextItem.indicator?.visible ?? false)
                        nextWidth += nextItem.indicator.implicitWidth + nextItem.leftPadding;
                    widest = Math.max(widest, nextWidth);
                }
                return widest + leftPadding + rightPadding;
            }
            delegate: MenuItem {
                id: menuItem
                contentItem: Item {
                    id: cntRoot
                    implicitWidth: nameLabel.implicitWidth + shortcutLabel.implicitWidth + 20
                    // implicitHeight: Math.max(nameLabel.height, shortcutLabel.height)

                    anchors.left: (menuItem.indicator.visible) ? menuItem.indicator.right : menuItem.left
                    anchors.leftMargin: menuItem.leftPadding
                    anchors.right: menuItem.right
                    anchors.rightMargin: menuItem.rightPadding

                    Label {
                        id: nameLabel
                        text: menuItem.action?.text ?? menuItem.subMenu.title

                        anchors.left: cntRoot.left
                        anchors.verticalCenter: cntRoot.verticalCenter
                    }
                    Label {
                        id: shortcutLabel
                        text: menuItem.action?.heldShortcut ?? menuItem.action?.shortcut ?? ""

                        anchors.right: cntRoot.right
                        anchors.verticalCenter: cntRoot.verticalCenter
                    }
                }
            }
        }
        readonly property Component separator: MenuSeparator {}

        function newMenu(menu: EmuMenu): Menu {
            return blankMenu.createObject(null, {
                title: Qt.binding(() => menu.text)
            }) as Menu;
        }
        function newSeparator(action: EmuAction): MenuSeparator {
            return separator.createObject(null, {}) as MenuSeparator;
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
        // clear out all the old menus
        while (root.count > 0) {
            root.takeMenu(root.count - 1);
        }
        // construct the new menu
        let menuCache = {};
        let actionCache = {};
        for (const item of actions.children) {
            if (item instanceof EmuMenu) {
                if (item.key in menuCache)
                    throw Error(`menu with name ${item.key} already declared`);
                if (item.key in actionCache)
                    throw Error(`action with name ${item.key} already declared`);

                let viewMenu = priv.newMenu(item);
                menuCache[item.key] = viewMenu;

                let parentKey = priv.parentKey(item.key);
                if (parentKey == "menu") {
                    // menu is a root menu, add to self
                    root.addMenu(viewMenu);
                    viewMenu.visibleChanged.connect(() => {
                        priv.opened = root.menus.some(menu => menu.visible);
                    });
                } else {
                    // menu is a submenu
                    if (!(parentKey in menuCache))
                        throw Error(`parent menu ${parentKey} does not exist`);
                    menuCache[parentKey].addMenu(viewMenu);

                    if (item.addSeparator)
                        menuCache[parentKey].addItem(priv.newSeparator());
                }
            } else if (item instanceof EmuAction) {
                if (item.key in menuCache)
                    throw Error(`menu with name ${item.key} already declared`);
                if (item.key in actionCache)
                    throw Error(`action with name ${item.key} already declared`);

                actionCache[item.key] = item;

                let parentKey = priv.parentKey(item.key);
                if (!(parentKey in menuCache))
                    throw Error(`parent menu ${parentKey} does not exist`);

                menuCache[parentKey].addAction(item);
                if (item.addSeparator)
                    menuCache[parentKey].addItem(priv.newSeparator());
            }
        }
    }
}
