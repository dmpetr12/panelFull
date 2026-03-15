import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

Popup {
    id: root
    modal: true
    focus: true
    closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside

    onClosed: target = null

    // Popup на всю страницу/окно
    x: 0
    y: 0
    width: parent ? parent.width : implicitWidth
    height: parent ? parent.height : implicitHeight

    // фон на всё окно
    background: Rectangle {
        anchors.fill: parent
        color: "#40808080"
    }

    // сюда кладём саму клавиатуру
    contentItem: Item {
        id: contentRoot
        anchors.fill: parent

        Rectangle {
            id: keyboardPanel
            color: "#303030"
            radius: 12
            border.color: "#505050"

            anchors.left: parent.left
            anchors.right: parent.right
            anchors.bottom: parent.bottom
            anchors.margins: 16

            // ограничиваем высоту: не больше 45% окна и не больше 400px
            property int maxHeight: 600
            height: Math.min(parent.height * 0.75, maxHeight)

            ColumnLayout {
                id: column
                anchors.fill: parent
                anchors.margins: 12
                spacing: 6

                Label {
                    text: qsTr("Ввод названия линии")
                    color: "white"
                    font.pixelSize: 24
                    horizontalAlignment: Text.AlignHCenter
                    Layout.alignment: Qt.AlignHCenter
                }

                property var rows: [
                    "1234567890",
                    "йцукенгшщзхъ",
                    "фывапролджэ",
                    "ячсмитьбю"
                ]

                Repeater {
                    model: column.rows.length
                    delegate: RowLayout {
                        spacing: 4
                        Layout.alignment: Qt.AlignHCenter

                        property string row: column.rows[index]

                        Repeater {
                            model: row.length
                            delegate: Button {
                                required property int index
                                property string key: row.charAt(index)

                                text: key
                                Layout.preferredWidth: 60
                                Layout.preferredHeight: 60
                                font.pixelSize: 24

                                onClicked: root.appendText(key)
                            }
                        }
                    }
                }

                RowLayout {
                    spacing: 4
                    Layout.fillWidth: true

                    Button {
                        text: qsTr("Пробел")
                        Layout.fillWidth: true
                        Layout.preferredHeight: 60
                        font.pixelSize: 20
                        onClicked: root.appendText(" ")
                    }

                    Button {
                        text: "⌫"
                        Layout.preferredWidth: 70
                        Layout.preferredHeight: 60
                        font.pixelSize: 24
                        onClicked: root.backspace()
                    }

                    Button {
                        text: qsTr("Очистить")
                        Layout.preferredWidth: 110
                        Layout.preferredHeight: 60
                        font.pixelSize: 20
                        onClicked: root.clearText()
                    }

                    Button {
                        text: qsTr("ОК")
                        Layout.preferredWidth: 70
                        Layout.preferredHeight: 60
                        font.pixelSize: 24
                        onClicked: root.close()
                    }
                }
            }
        }
    }

    // --- куда печатаем ---

    property var target: null

    function openFor(t) {
        target = t
        root.open()
    }

    function appendText(s) {
        if (!target)
            return
        var t = target.text
        var p = target.cursorPosition
        target.text = t.slice(0, p) + s + t.slice(p)
        target.cursorPosition = p + s.length
    }

    function backspace() {
        if (!target)
            return
        var t = target.text
        var p = target.cursorPosition
        if (p <= 0)
            return
        target.text = t.slice(0, p - 1) + t.slice(p)
        target.cursorPosition = p - 1
    }

    function clearText() {
        if (!target)
            return
        target.text = ""
        target.cursorPosition = 0
    }
}
