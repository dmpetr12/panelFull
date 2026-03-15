import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

Rectangle {
    id: pageInfo
    objectName: "pageInfo"
    width: stackViewList.width
    height: stackViewList.height
    color: "transparent"

    Rectangle {
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        height: 500
        color: "transparent"
        anchors.margins: 12

        ScrollView {
            anchors.fill: parent
            anchors.margins: 12
            clip: true

            Text {
                id: infoText
                width: parent.width - 10
                wrapMode: Text.WordWrap
                textFormat: Text.PlainText
                color: "black"
                font.pixelSize: 24

                text:
                    "Группа компаний «Световые Технологии» — ведущий российский производитель\n"
                    + "современного светотехнического оборудования. Компания является признанным\n"
                    + "экспертом в области разработки и производства эффективных световых решений.\n"
                    + "Контактная информация: 127273, г. Москва, улица Отрадная, дом 2Б, строение 7.\n"
                    + "Связаться с нами можно по бесплатному телефону 8(800)333-23-77 или\n"
                    + "по московскому номеру +7(495)995-55-95.\n\n"
                    + "Панель написана с использованием Qt 6 под открытой лицензией GNU LGPL v3.\n"
                    + "This product uses Qt 6 under the GNU LGPL v3 license.\n"
                    + "Qt is a registered trademark of The Qt Company Ltd."
            }
        }
    }

    Rectangle {
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        height: 150
        color: "transparent"
        anchors.margins: 12

        RowLayout {
            anchors.fill: parent

            Rectangle {
                id: btnRet
                width: 150
                height: 150
                radius: 6
                color: "lightgray"

                Image {
                    anchors.fill: parent
                    source: "qrc:/Back.png"
                    fillMode: Image.PreserveAspectFit
                }

                MouseArea {
                    anchors.fill: parent
                    hoverEnabled: true
                    onPressed: btnRet.color = "gray"
                    onReleased: btnRet.color = "lightgray"
                    onClicked: stackViewList.replace("pageStart.qml")
                }
            }

            Item {
                Layout.fillWidth: true
            }

            Text {
                text: "@design P.Dmitriev  dmpetr@ya.ru"
                font.pixelSize: 20
                wrapMode: Text.WordWrap
                Layout.alignment: Qt.AlignRight | Qt.AlignBottom
            }
        }
    }
}
