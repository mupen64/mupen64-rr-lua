/*
 * Copyright (c) 2026, Mupen64 Organization (https://github.com/mupen64)
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
pragma ComponentBehavior: Bound

import QtQml
import QtQml.Models
import QtQuick
import QtQuick.Controls

import Core

Item {
    id: root

    required property RomManager romManager
    required property EmuContext context

    QtObject {
        id: priv

        function flagFilename(regionCode: int): string {
            switch (regionCode) {
                case 0x00: return "beta.svg";
                case 0x37: return "beta.svg";
                case 0x41: return "flag-JP.svg"; // en64 wiki calls this "Asian"
                case 0x42: return "flag-BR.svg";
                case 0x43: return "flag-CN.svg";
                case 0x44: return "flag-DE.svg";
                case 0x45: return "flag-US.svg";
                case 0x46: return "flag-FR.svg";
                case 0x47: return "flag-US.svg"; // Gateway 64 (NTSC)
                case 0x48: return "flag-NL.svg";
                case 0x49: return "flag-IT.svg";
                case 0x4A: return "flag-JP.svg";
                case 0x4B: return "flag-KR.svg";
                case 0x4C: return "flag-EU.svg"; // Gateway 64 (PAL)
                case 0x4E: return "flag-CA.svg";
                case 0x50: return "flag-EU.svg";
                case 0x53: return "flag-ES.svg";
                case 0x55: return "flag-AU.svg";
                case 0x57: return "flag-SE.svg"; // en64 wiki calls this "Scandinavian"
                case 0x58: return "flag-EU.svg";
                case 0x59: return "flag-EU.svg"; // @Aurumaker72 says this is supposed to be Australia
                default: return "unknown.svg";
            }
        }

        function flagAssetPath(regionCode: int): string {
            return `qrc:/Assets/Regions/${flagFilename(regionCode)}`;
        }
    }

    HorizontalHeaderView {
        id: topHeader
        anchors.top: root.top
        anchors.left: root.left
        anchors.right: root.right

        syncView: table

    }
    TableView {
        id: table
        anchors.top: topHeader.bottom
        anchors.left: root.left
        anchors.right: root.right
        anchors.bottom: root.bottom

        model: root.romManager

        selectionBehavior: TableView.SelectRows
        selectionMode: TableView.SingleSelection
        selectionModel: ItemSelectionModel {
            model: table.model
        }

        delegate: DelegateChooser {
            role: "displayType"
            DelegateChoice {
                roleValue: "text"
                RomBrowserCell {
                    id: cellRoot
                    table: table
                    context: root.context

                    Label {
                        anchors.left: cellRoot.left
                        anchors.verticalCenter: cellRoot.verticalCenter
                        anchors.leftMargin: 10
                        text: cellRoot.display
                    }
                }
            }
            DelegateChoice {
                roleValue: "flagIcon"
                RomBrowserCell {
                    id: cellRoot
                    table: table
                    context: root.context

                    Item {
                        anchors.left: cellRoot.left
                        anchors.verticalCenter: cellRoot.verticalCenter
                        anchors.leftMargin: 10

                        implicitWidth: Qt.application.font.pixelSize * 1.2
                        implicitHeight: width

                        Image {
                            anchors.fill: parent
                            source: priv.flagAssetPath(cellRoot.regionCode)
                        }
                    }
                }
            }
        }
    }
}
