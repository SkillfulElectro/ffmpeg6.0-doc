import QtQuick

Window {
    width: 640
    height: 480
    visible: true
    title: qsTr("Hello World")
    Image{
        id : show
        objectName: "show"
        anchors.fill: parent
    }
    MouseArea{
        anchors.fill: parent
        onClicked: show.source = "start"
    }
}
