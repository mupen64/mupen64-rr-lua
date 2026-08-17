//import related modules
import QtQuick 2.0
import QtQuick.Controls

//window containing the application
ApplicationWindow {
    width: 640
    height: 480
    visible: true
    //title of the application
    title: qsTr("Mupen64RR")

    minimumWidth: width
    maximumWidth: width

    minimumHeight: height
    maximumHeight: height

    //menu containing two menu items
    header: MenuBar {
        Menu {
            title: qsTr("&File")
            Action {
                text: qsTr("&Open...")
                onTriggered: console.log("Open action triggered")
            }
            MenuSeparator { }
            Action {
                text: qsTr("&Exit")
                onTriggered: Qt.quit()
            }
        }
    }

    //Content Area

    //a button in the middle of the content area
    Button {
        text: qsTr("Hello World")
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.verticalCenter: parent.verticalCenter
    }
}