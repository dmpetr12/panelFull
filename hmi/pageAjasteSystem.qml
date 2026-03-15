import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import App 1.0  // <- новый модуль


//rect parent.width x 650
Rectangle {
    width: stackViewList.width
    height: stackViewList.height
    TimeAjst { id: timeAjst }   // <- новый объект
    Rectangle {
        //anchors.fill: parent
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        height : 500
        anchors.margins: 12
        color: "transparent"
        Column {
            topPadding: 100
            spacing: 20
            //Длительность аварийного режима
            Row {
                spacing: 30
                Text {
                     text: "Длительность аварийного режима, ч"
                     width: 700; height: 70
                     font.pixelSize: 40
                }

                RowLayout{
                    spacing: 25
                   Text {
                       font.pixelSize: 40
                       text: (durationAv).toString()
                   }

                   Button {
                       text: "ввести"
                       font.pixelSize: 40
                       onClicked: digitalPopup.openFor("Введите время теста на длит.",win,"durationAv", "", maxHour)
                   }
                }
            }
            //Количество линий
            Row {
                spacing: 30
                Text {
                     text: "Количество линий"
                     width: 700; height: 70
                     font.pixelSize: 40
                }
                RowLayout{
                    spacing: 25
                   Text {
                       font.pixelSize: 40
                       text: (countLn).toString()
                   }

                   Button {
                       text: "ввести"
                       font.pixelSize: 40
                       onClicked: digitalPopup.openFor("Введите количество линий",win, "countLn" , "", maxCount)
                   }
                }
            }
        }

    }
    Rectangle {
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        height: 150
        anchors.margins: 12
        color: "transparent"
        Row {
            Rectangle {
                id: btnRet
                width: 150
                height: 150
                color: "lightgray"
                radius: 6

                // Text {
                //     color: "black"
                //     anchors.centerIn: parent
                //     text: "ВЕРНУТЬСЯ"
                //     font.pixelSize: 30
                // }
                Image {
                    anchors.fill: parent
                    source: "qrc:/Back.png"
                    //scale: 1
                    // width: 1; height: 1
                     fillMode: Image.PreserveAspectFit
                   // fillMode: Image.Stretch
                }

                MouseArea {
                    anchors.fill: parent
                    hoverEnabled: true
                    onEntered: btnRet.color = "lightgray"
                    onExited: btnRet.color = "lightgray"
                    onPressed: btnRet.color = "gray"
                    onReleased: btnRet.color = "lightgray"
                    onClicked: stackViewList.replace("pageAjaste.qml")
                }
            }
        }
    }
}





