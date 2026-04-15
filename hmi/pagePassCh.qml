import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

Rectangle {
    width: stackViewList.width
    height: stackViewList.height
    color: "white"
    objectName: "pagePassCh"

    property string newPasswordBuffer: ""
    property string confirmPasswordBuffer: ""
    property string stage: "new"
    property string errorText: ""
    property int maxPasswordLength: 8

    Column {
        anchors.centerIn: parent
        spacing: 20

        Text {
            text: stage === "new" ? "Введите новый пароль"
                                  : "Подтвердите новый пароль"
            font.pixelSize: 24
            horizontalAlignment: Text.AlignHCenter
            width: 320
        }

        Text {
            text: (stage === "new" ? newPasswordBuffer : confirmPasswordBuffer).length > 0
                  ? "*".repeat((stage === "new" ? newPasswordBuffer : confirmPasswordBuffer).length)
                  : ""
            font.pixelSize: 28
            horizontalAlignment: Text.AlignHCenter
            width: 200
        }

        Text {
            text: errorText
            color: "red"
            font.pixelSize: 20
            horizontalAlignment: Text.AlignHCenter
            width: 320
            visible: errorText.length > 0
        }

        GridLayout {
            columns: 3
            rowSpacing: 10
            columnSpacing: 10

            Repeater {
                model: 9
                delegate: Button {
                    text: index + 1
                    Layout.preferredWidth: 80
                    Layout.preferredHeight: 80

                    onClicked: {
                        if (stage === "new") {
                            if (newPasswordBuffer.length < maxPasswordLength)
                                newPasswordBuffer += text
                        } else {
                            if (confirmPasswordBuffer.length < maxPasswordLength)
                                confirmPasswordBuffer += text
                        }
                    }
                }
            }

            Button {
                text: "Clear"
                Layout.preferredWidth: 80
                Layout.preferredHeight: 80
                onClicked: {
                    errorText = ""
                    if (stage === "new")
                        newPasswordBuffer = ""
                    else
                        confirmPasswordBuffer = ""
                }
            }

            Button {
                text: "0"
                Layout.preferredWidth: 80
                Layout.preferredHeight: 80
                onClicked: {
                    if (stage === "new") {
                        if (newPasswordBuffer.length < maxPasswordLength)
                            newPasswordBuffer += "0"
                    } else {
                        if (confirmPasswordBuffer.length < maxPasswordLength)
                            confirmPasswordBuffer += "0"
                    }
                }
            }

            Button {
                text: "⌫"
                Layout.preferredWidth: 80
                Layout.preferredHeight: 80
                onClicked: {
                    errorText = ""
                    if (stage === "new" && newPasswordBuffer.length > 0)
                        newPasswordBuffer = newPasswordBuffer.slice(0, -1)
                    else if (stage === "confirm" && confirmPasswordBuffer.length > 0)
                        confirmPasswordBuffer = confirmPasswordBuffer.slice(0, -1)
                }
            }
        }

        Row {
            spacing: 20

            Button {
                text: "Cancel"
                width: 120
                onClicked: stackViewList.replace("pageAjaste.qml")
            }

            Button {
                text: "OK"
                width: 120

                onClicked: {
                    errorText = ""

                    if (!panel.connected) {
                        errorText = "Backend не подключён"
                        return
                    }

                    if (stage === "new") {
                        if (newPasswordBuffer.length < 4) {
                            errorText = "Пароль слишком короткий"
                        } else {
                            stage = "confirm"
                        }
                    } else if (stage === "confirm") {
                        if (confirmPasswordBuffer !== newPasswordBuffer) {
                            errorText = "Пароли не совпадают"
                            return
                        }

                        if (panel.changePassword(newPasswordBuffer)) {
                            panel.writeLog("Пароль изменён")
                            stackViewList.replace("pageAjaste.qml")
                        } else {
                            errorText = "Не удалось изменить пароль"
                            panel.writeLog("Не удалось изменить пароль")
                        }
                    }
                }
            }
        }
    }
}
