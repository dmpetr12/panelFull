import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

Rectangle {
    width: stackViewList.width
    height: stackViewList.height
    color: "white"

    property string passwordBuffer: ""

    Column {
        anchors.centerIn: parent
        spacing: 20

        Text {
            id: passwordDisplay
            text: "*".repeat(passwordBuffer.length)
            font.pixelSize: 28
            horizontalAlignment: Text.AlignHCenter
            width: 200
        }

        GridLayout {
            id: keypad
            columns: 3
            rowSpacing: 10
            columnSpacing: 10

            Repeater {
                model: 9
                delegate: Button {
                    text: index + 1
                    Layout.preferredWidth: 80
                    Layout.preferredHeight: 80
                    onClicked: passwordBuffer += text
                }
            }

            Button {
                text: "Clear"
                Layout.preferredWidth: 80
                Layout.preferredHeight: 80
                onClicked: passwordBuffer = ""
            }

            Button {
                text: "0"
                Layout.preferredWidth: 80
                Layout.preferredHeight: 80
                onClicked: passwordBuffer += "0"
            }

            Button {
                text: "⌫"
                Layout.preferredWidth: 80
                Layout.preferredHeight: 80
                onClicked: {
                    if (passwordBuffer.length > 0)
                        passwordBuffer = passwordBuffer.slice(0, -1)
                }
            }
        }

        Button {
            text: "OK"
            width: 260
            height: 60

            onClicked: {
                if (panel.checkPassword(passwordBuffer)) {
                    unlocked = true
                    //console.log("Доступ разрешён")
                } else {
                    unlocked = false
                    //console.log("Введён неверный пароль")
                }

                passwordBuffer = ""
                stackViewList.replace("pageStart.qml")
            }
        }
    }
}
