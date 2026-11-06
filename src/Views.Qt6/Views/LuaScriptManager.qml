/*
 * Copyright (c) 2026, Mupen64 Organization (https://github.com/mupen64)
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
pragma ComponentBehavior: Bound

import QtCore
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs as Dialogs

import Mupen64.Lua

ApplicationWindow {
    id: window
    //% "Lua Script Manager"
    title: qsTrId("lua.manager.title")
    minimumWidth: 640
    minimumHeight: 440
    width: 720
    height: 480

    required property LuaInstanceManager instanceManager

    property LuaInstance selectedInstance: (instanceManager && instanceManager.count > 0 && scriptList.currentIndex >= 0 && scriptList.currentIndex < instanceManager.count)
        ? instanceManager.get(scriptList.currentIndex)
        : null

    Dialogs.FileDialog {
        id: fileDialog
        //% "Choose Script"
        title: qsTrId("lua.manager.chooseScript")
        nameFilters: ["Lua scripts (*.lua)", "All files (*)"]
        onAccepted: {
            // note: toLocalFile() on the dialog's url fails with the platform file picker; decode the url manually
            let urlStr = fileDialog.selectedFile.toString()
            let localPath = ""
            if (urlStr.startsWith("file://")) {
                localPath = decodeURIComponent(urlStr.substring(7))
                // drive letter on Windows: file:///C:/... -> C:/...
                if (localPath.length >= 3 && localPath[0] === '/' && localPath[2] === ':') {
                    localPath = localPath.substring(1)
                }
            }
            window.instanceManager.addInstance(localPath, false)
        }
    }

    SplitView {
        anchors.fill: parent
        orientation: Qt.Horizontal

        Item {
            SplitView.preferredWidth: 420
            SplitView.minimumWidth: 320
            SplitView.fillHeight: true

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 8
                spacing: 6

                // flow so buttons wrap instead of going under the splitter handle
                Flow {
                    Layout.fillWidth: true
                    spacing: 4

                    Button {
                        //% "Add..."
                        text: qsTrId("lua.manager.add")
                        onClicked: fileDialog.open()
                    }

                    Button {
                        //% "Remove"
                        text: qsTrId("lua.manager.remove")
                        enabled: window.selectedInstance !== null
                        onClicked: {
                            if (scriptList.currentIndex >= 0) {
                                window.instanceManager.removeInstance(scriptList.currentIndex)
                            }
                        }
                    }

                    Button {
                        //% "Start All"
                        text: qsTrId("lua.manager.startAll")
                        onClicked: window.instanceManager.startAll()
                    }

                    Button {
                        //% "Stop All"
                        text: qsTrId("lua.manager.stopAll")
                        onClicked: window.instanceManager.stopAll()
                    }
                }

                ScrollView {
                    Layout.fillWidth: true
                    Layout.fillHeight: true

                    ListView {
                        id: scriptList
                        model: window.instanceManager
                        clip: true
                        currentIndex: 0

                        delegate: ItemDelegate {
                            id: delegateItem
                            required property string name
                            required property string path
                            required property bool running
                            required property bool trusted
                            required property string lastError
                            required property LuaInstance instance
                            required property int index

                            width: ListView.view.width
                            highlighted: scriptList.currentIndex === delegateItem.index

                            onClicked: {
                                scriptList.currentIndex = delegateItem.index
                            }

                            // surface failures in the console pane without requiring a click
                            Connections {
                                target: delegateItem.instance
                                function onErrorOccurred(error) {
                                    scriptList.currentIndex = delegateItem.index
                                }
                            }

                            contentItem: RowLayout {
                                spacing: 8

                                Rectangle {
                                    width: 10
                                    height: 10
                                    radius: 5
                                    color: delegateItem.running
                                        ? "#4CAF50"
                                        : (delegateItem.lastError !== "" ? "#F44336" : "#9E9E9E")
                                    Layout.alignment: Qt.AlignVCenter
                                }

                                ColumnLayout {
                                    Layout.fillWidth: true
                                    spacing: 2

                                    Label {
                                        text: delegateItem.name
                                        font.bold: true
                                        elide: Text.ElideRight
                                        Layout.fillWidth: true
                                    }

                                    Label {
                                        text: delegateItem.path
                                        font.pointSize: 8
                                        opacity: 0.7
                                        elide: Text.ElideMiddle
                                        Layout.fillWidth: true
                                    }

                                    Label {
                                        visible: delegateItem.lastError !== ""
                                        text: delegateItem.lastError
                                        font.pointSize: 8
                                        color: "#F44336"
                                        elide: Text.ElideMiddle
                                        Layout.fillWidth: true
                                    }
                                }

                                Button {
                                    //% "Stop"
                                    property string stopLabel: qsTrId("lua.manager.stop")
                                    //% "Start"
                                    property string startLabel: qsTrId("lua.manager.start")
                                    text: delegateItem.running ? stopLabel : startLabel
                                    onClicked: {
                                        if (delegateItem.running) {
                                            delegateItem.instance.stop()
                                        } else {
                                            delegateItem.instance.start()
                                        }
                                    }
                                }
                            }
                        }
                    }
                }

                CheckBox {
                    id: trustedBox
                    //% "Trusted (disable sandbox)"
                    text: qsTrId("lua.manager.trusted")
                    enabled: window.selectedInstance !== null && !window.selectedInstance.running
                    checked: window.selectedInstance ? window.selectedInstance.trusted : false
                    onToggled: {
                        if (window.selectedInstance) {
                            window.selectedInstance.trusted = checked
                        }
                    }
                }
            }
        }

        Item {
            SplitView.fillWidth: true
            SplitView.fillHeight: true

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 8
                spacing: 6

                RowLayout {
                    Layout.fillWidth: true

                    Label {
                        //% "Console for"
                        property string consoleForLabel: qsTrId("lua.manager.consoleFor")
                        //% "Console"
                        property string consoleLabel: qsTrId("lua.manager.console")
                        text: window.selectedInstance
                            ? consoleForLabel + " " + window.selectedInstance.name
                            : consoleLabel
                        font.bold: true
                    }

                    Item { Layout.fillWidth: true }

                    Button {
                        //% "Restart"
                        text: qsTrId("lua.manager.restart")
                        enabled: window.selectedInstance !== null
                        onClicked: {
                            if (window.selectedInstance) {
                                window.selectedInstance.restart()
                            }
                        }
                    }

                    Button {
                        //% "Clear Output"
                        text: qsTrId("lua.manager.clearOutput")
                        enabled: window.selectedInstance !== null
                        onClicked: {
                            if (window.selectedInstance) {
                                window.selectedInstance.clearLogs()
                            }
                        }
                    }
                }

                ScrollView {
                    Layout.fillWidth: true
                    Layout.fillHeight: true

                    TextArea {
                        id: logArea
                        readOnly: true
                        wrapMode: TextEdit.Wrap
                        font.family: "Monospace"
                        font.pointSize: 9
                        text: window.selectedInstance ? window.selectedInstance.logs : ""
                        background: Rectangle {
                            color: "#1E1E1E"
                            border.color: "#333333"
                            radius: 3
                        }
                        color: "#E0E0E0"
                    }
                }
            }
        }
    }
}
