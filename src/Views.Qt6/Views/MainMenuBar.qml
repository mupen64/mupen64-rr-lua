import QtQuick
import QtQuick.Controls
import QtQuick.Dialogs as Dialogs
import QtQuick.Layouts

import Core

MenuBar {
    id: menuBar
    property EmuContext core: null

    // binding is not implemented here as it won't update properly
    property bool opened: false

    // FIXME (MacOS): May not work as intended. More testing needed.
    delegate: MenuBarItem {
        id: item
        // update menuBar.opened as needed
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
            menuBar.opened = menuBar.menus.some(child => child.visible);
        }
    }
    Menu {
        title: qsTr("File")
        Action {
            text: qsTr("Load ROM...")
            onTriggered: diaOpenRom.open()
        }
        Action {
            text: qsTr("Close ROM")
            enabled: core.launched
            onTriggered: core.closeROM()
        }
        Action {
            text: qsTr("Reset ROM")
            enabled: core.launched
            onTriggered: core.resetROM()
        }
    }
    Menu {
        title: qsTr("Emulation")
        Action {
            id: actPause
            checkable: true
            text: qsTr("Pause")
            enabled: core.launched
        }
        Action {
            text: qsTr("Speed Down")
            enabled: core.launched
            onTriggered: core.speedModifier -= 5
        }
        Action {
            text: qsTr("Speed Up")
            enabled: core.launched
            onTriggered: core.speedModifier += 5
        }
        Action {
            text: qsTr("Reset Speed")
            enabled: core.launched
            onTriggered: core.speedModifier = 100
        }
        Action {
            id: actGSButton
            enabled: core.launched
            text: qsTr("GS Button")
            checkable: true
        }
        MenuSeparator {}
        Action {
            text: qsTr("Frame Advance")
            enabled: core.launched
            onTriggered: {
                actPause.checked = true;
                core.frameAdvance(1);
            }
        }
        // TODO: multi-frame advance
        MenuSeparator {}
        Menu {
            title: qsTr("Save State")
            Action {
                text: qsTr("Save Current Slot")
                enabled: core.launched
            }
            Action {
                text: qsTr("Save as File...")
                enabled: core.launched
            }
            MenuSeparator {}
        }
        MenuSeparator {}
        Menu {
            id: menuLoadState
            title: qsTr("Load State")
            Action {
                text: qsTr("Load Current Slot")
                enabled: core.launched
            }
            Action {
                text: qsTr("Load from File...")
                enabled: core.launched
            }
            MenuSeparator {
                id: sepSaveSlots
                property int parentIndex: menuLoadState.contentChildren.indexOf(sepSaveSlots)
            }
            Instantiator {
                model: 10

                delegate: Action {
                    required property int index
                    text: `${index + 1}`
                }

                onObjectAdded: (index, object) => menuLoadState.insertAction(sepSaveSlots.parentIndex + index + 1, object)
                onObjectRemoved: (index, object) => menuLoadState.removeItem(object)
            }
        }
        MenuSeparator {}
        Menu {
            id: menuCurrentSlot
            title: qsTr("Current State Slot")
            ActionGroup {
                id: groupCurrentSlot
                exclusive: true
            }
            Instantiator {
                model: 10

                delegate: Action {
                    required property int index
                    ActionGroup.group: groupCurrentSlot
                    text: `${index + 1}`
                }

                onObjectAdded: (index, object) => menuCurrentSlot.insertAction(index, object)
                onObjectRemoved: (index, object) => menuCurrentSlot.removeItem(object)
            }
        }
    }
    // Bindings and signals to attach for menu items
    Binding {
        when: core !== null
        core.paused: menuBar.opened || actPause.checked
        core.gsButton: actGSButton.checked
    }
    Connections {
        target: core
        function onLaunchedChanged() {
            actPause.checked = false;
            actGSButton.checked = false;
        }
    }
}