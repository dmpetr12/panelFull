import QtQuick 2.15
import QtQuick.Controls 2.15

Rectangle {
    id: root
    width: parent ? parent.width : 400
    height: 56
    color: "transparent"

    property string lineName: "Линия"
    property string statusText: "OK" // OK / ERR / TEST
    property string statusColor: "#5EC85E" // зелёный по умолчанию

    Row {
        anchors.fill: parent
        anchors.margins: 8
        spacing: 12

        // индикатор
        Rectangle {
            width: 28; height: 28
            radius: 14
            color: root.statusColor
            border.width: 2
            border.color: "#21323a"
            anchors.verticalCenter: parent.verticalCenter
        }

        // имя линии
        Text {
            text: root.lineName
            font.pixelSize: 20
            color: "#21323a"
            verticalAlignment: Text.AlignVCenter
            elide: Text.ElideRight
            width: parent.width * 0.6
        }

        // статус справа
        Text {
            text: root.statusText
            font.pixelSize: 20
            color: "#21323a"
            verticalAlignment: Text.AlignVCenter
            anchors.right: parent.right
        }
    }

    Rectangle {
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        height: 1
        color: "#cfd8dc"
        opacity: 0.6
    }
}
