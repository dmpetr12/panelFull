import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import App 1.0

Rectangle {
    id: timeChange

    property int year: new Date().getFullYear()
    property int month: new Date().getMonth() + 1
    property int day: new Date().getDate()
    property int hour: new Date().getHours()
    property int minute: new Date().getMinutes()

    width: stackViewList.width
    height: stackViewList.height

    Rectangle {
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        height: 500
        anchors.margins: 12
        color: "transparent"

        Column {
            topPadding: 150
            leftPadding: 20
            spacing: 20

            Text {
                text: "Установка системного времени"
                font.pixelSize: 40
                horizontalAlignment: Text.AlignHCenter
            }

            Row {
                spacing: 30

                Column {
                    spacing: 20

                    Text {
                        font.pixelSize: 50
                        text: day
                    }

                    Button {
                        text: "день"
                        font.pixelSize: 50
                        onClicked: digitalPopup.openFor("Введите день", timeChange, "day", day, 31)
                    }
                }

                Column {
                    spacing: 20

                    Text {
                        font.pixelSize: 50
                        text: month
                    }

                    Button {
                        text: "месяц"
                        font.pixelSize: 50
                        onClicked: digitalPopup.openFor("Введите месяц", timeChange, "month", month, 12)
                    }
                }

                Column {
                    spacing: 20

                    Text {
                        font.pixelSize: 50
                        text: year
                    }

                    Button {
                        text: "год"
                        font.pixelSize: 50
                        onClicked: digitalPopup.openFor("Введите полный год", timeChange, "year", year, 2100)
                    }
                }

                Column {
                    spacing: 20

                    Text {
                        font.pixelSize: 50
                        text: "   "
                    }
                }

                Column {
                    spacing: 20

                    Text {
                        font.pixelSize: 50
                        text: hour + "   :"
                    }

                    Button {
                        text: "часы"
                        font.pixelSize: 50
                        onClicked: digitalPopup.openFor("Введите часы", timeChange, "hour", hour, 23)
                    }
                }

                Column {
                    spacing: 20

                    Text {
                        font.pixelSize: 50
                        text: minute
                    }

                    Button {
                        text: "минуты"
                        font.pixelSize: 50
                        onClicked: digitalPopup.openFor("Введите минуты", timeChange, "minute", minute, 59)
                    }
                }
            }

            Button {
                text: "Установить системное время"
                width: 400
                height: 70
                font.pixelSize: 22

                onClicked: {
                    let d = new Date(year, month - 1, day, hour, minute)
                    panel.setSystemTime(d.getTime())


                    let updated = new Date()
                    year = updated.getFullYear()
                    month = updated.getMonth() + 1
                    day = updated.getDate()
                    hour = updated.getHours()
                    minute = updated.getMinutes()

                    dateText.text = Qt.formatDate(updated, "dd.MM.yyyy")
                    timeText.text = Qt.formatTime(updated, "hh:mm")
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

                Image {
                    anchors.fill: parent
                    source: "qrc:/Back.png"
                    fillMode: Image.PreserveAspectFit
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
