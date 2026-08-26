pragma Singleton

import QtCore
import QtQuick

Settings {
    category: "vcr"

    property bool backups: true
    property bool writeExtendedFormat: true
    property bool resetRecordingEnabled: false
}
