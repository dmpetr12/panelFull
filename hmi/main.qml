import QtQuick 2.15
import QtQuick.Window 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

import App 1.0

Window {
    id: win

    property bool unlocked: false
    property bool testStart: false
    property bool btnStopTestVsb: false
    property int durationAv: 1
    property int countLn: 25
    property int durationTst: 2
    property double showPower: 0
    property int indexCh: 0
    property int coutPush: 0
    property int coutPushCh: 10
    property int mintpower: 0
    property Line currentLine
    property string testName: ""
    property int maxPower: 2000
    property int maxHour: 3
    property int maxCount: 25
    property int maxTolerance: 50
    property int maxMinute: 55
    property int idleTimeoutSec: 600

    Component.onCompleted: {
        durationAv = 1
        countLn = panel.lineCount
        testStart = panel.testRunning
    }

    Connections {
        target: panel
        function onChanged() {
            countLn = panel.lineCount
            testStart = panel.testRunning
        }
    }

    visible: true
    visibility: Window.FullScreen   // <--- главное !!!
    flags: Qt.FramelessWindowHint | Qt.WindowStaysOnTopHint // !!! без рамки и заголовка И  всегд сверху
    //width: 1024
    //height: 768
    color: "#FFFFFF"
    //maximumWidth: width
    //maximumHeight: height
    //minimumWidth: width
    //minimumHeight: height
    //x:0
    //y:0

    Timer {
        id: idleTimer
        interval: idleTimeoutSec * 1000
        repeat: false
        running: true

        onTriggered: {
            if (!win.testStart) {
                win.unlocked = false
                stackViewList.clear()
                stackViewList.push("pageStart.qml")
            } else {
                idleTimer.restart()
            }
        }
    }

    function resetIdleTimer() {
        idleTimer.restart()
    }

    Popup {
        id: digitalPopup
        modal: true
        focus: true
        closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside
        width: 360
        height: 450
        margins: 20

        property var targetObject: null
        property string targetProperty: ""
        property real multiplier: 1.0
        property string nameP: ""
        property int maxV: 1000

        function openFor(nameText, target, propName, initialValue, maxValue) {
            nameP = nameText
            targetObject = target
            targetProperty = propName
            powerField.text = "" // initialValue.toString()
            maxV = maxValue
            open()
        }

        Rectangle {
            anchors.fill: parent
            color: "#333333"
            radius: 10

            Column {
                anchors.fill: parent
                anchors.margins: 16
                spacing: 12

                Text {
                    text: digitalPopup.nameP
                    color: "white"
                    font.pixelSize: 30
                }

                TextField {
                    id: powerField
                    text: ""
                    readOnly: true
                    font.pixelSize: 40
                    inputMethodHints: Qt.ImhPreferNumbers
                }

                GridLayout {
                    columns: 3
                    rowSpacing: 4
                    columnSpacing: 4

                    Repeater {
                        model: ["1", "2", "3", "4", "5", "6", "7", "8", "9", "0", ",", "←"]
                        delegate: Button {
                            text: modelData
                            font.pixelSize: 34

                            onClicked: {
                                if (text === "←") {
                                    powerField.text = powerField.text.slice(0, -1)
                                } else {
                                    powerField.text += text
                                }
                            }
                        }
                    }
                }

                Row {
                    spacing: 12

                    Button {
                        text: "Отмена"
                        font.pixelSize: 40
                        onClicked: digitalPopup.close()
                    }

                    Button {
                        text: " OK "
                        font.pixelSize: 40

                        onClicked: {
                            var cont = true
                            var v = parseFloat(powerField.text.replace(",", "."))

                            if (!isNaN(v) && v < 1 &&
                                    digitalPopup.targetProperty !== "minute" &&
                                    digitalPopup.targetProperty !== "hour" &&
                                    digitalPopup.targetProperty !== "mpower")
                                cont = false

                            if (!isNaN(v) && v < 2025 &&
                                    digitalPopup.targetProperty === "year")
                                cont = false

                            if (cont && !isNaN(v) &&
                                    digitalPopup.targetObject &&
                                    digitalPopup.targetProperty !== "") {
                                digitalPopup.targetObject[digitalPopup.targetProperty] =
                                        digitalPopup.maxV > v ? v : digitalPopup.maxV
                            }

                            digitalPopup.close()
                        }
                    }
                }
            }
        }
    }

    Popup {
        id: maintenancePopup
        modal: true
        focus: true
        width: 460
        height: 280
        x: (parent.width - width) / 2
        y: (parent.height - height) / 2
        closePolicy: Popup.CloseOnPressOutside | Popup.CloseOnEscape

        property int overdueLines: 0
        property bool longTestOverdue: false

        background: Rectangle {
            radius: 12
            color: "white"
            border.color: "#FFA500"
            border.width: 2
        }

        Column {
            anchors.centerIn: parent
            spacing: 12

            Text {
                text: "ОБСЛУЖИВАНИЕ"
                font.pixelSize: 26
                font.bold: true
            }

            Text {
                text: "⚠ Проверка линий не выполнена"
                font.pixelSize: 20
                color: "#FF8C00"
            }

            Text {
                text: "Просрочено линий: " + maintenancePopup.overdueLines
                visible: maintenancePopup.overdueLines > 0
                font.pixelSize: 18
            }

            Text {
                text: "Длинный тест: ПРОСРОЧЕН"
                visible: maintenancePopup.longTestOverdue
                font.pixelSize: 18
                color: "#FF4C4C"
            }

            Text {
                text: "Рекомендуется выполнить тест"
                font.pixelSize: 16
                color: "gray"
            }

            Button {
                text: "OK"
                onClicked: maintenancePopup.close()
            }
        }
    }

    Rectangle {
        id: header
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        height: 120
        color: "transparent"
        border.width: 0

        RowLayout {
            anchors.fill: parent
            anchors.margins: 12

            Image {
                source: "qrc:/logo.png"
                scale: 1

                MouseArea {
                    anchors.fill: parent
                    onClicked: {
                        if (stackViewList.currentItem && stackViewList.currentItem.objectName === "pageStart") {
                            coutPush = 0
                            stackViewList.push("pageInfo.qml")
                        } else if (stackViewList.currentItem && stackViewList.currentItem.objectName === "pageInfo") {
                            coutPush++
                            if (coutPush > 15) {
                                coutPush = 0
                                stackViewList.push("pagePassCh.qml")
                            }
                        }
                    }
                }
            }

            Item {
                Layout.fillWidth: true
            }

            Text {
                text:  panel.temperatureAvailable ? panel.temperatureValue.toFixed(0) + " °C" : "—"
                font.pixelSize: 40
                Layout.margins: 55
                Layout.alignment: Qt.AlignVCenter | Qt.AlignTop
            }

            Column {
                spacing: 4
                Layout.alignment: Qt.AlignRight | Qt.AlignTop
                Layout.rightMargin: 20

                Text {
                    id: dateText
                    text: Qt.formatDate(new Date(), "dd.MM.yyyy")
                    font.pixelSize: 40
                    color: "#000000"
                }

                Text {
                    id: timeText
                    text: Qt.formatTime(new Date(), "hh:mm")
                    font.pixelSize: 40
                    color: "#000000"
                }

                Timer {
                    interval: 1000
                    running: true
                    repeat: true
                    onTriggered: {
                        timeText.text = Qt.formatTime(new Date(), "hh:mm")
                        dateText.text = Qt.formatDate(new Date(), "dd.MM.yyyy")
                    }
                }
            }

            Item {
                width: 80
            }
        }
    }

    StackView {
        id: stackViewList
        anchors.top: header.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        height: 650

        pushEnter: null
        pushExit: null
        popEnter: null
        popExit: null
        replaceEnter: null
        replaceExit: null

        initialItem: "pageStart.qml"
    }

    MouseArea {
        id: globalTouchCatcher
        anchors.fill: parent
        hoverEnabled: true
        propagateComposedEvents: true

        onPressed: function(mouse) {
            win.resetIdleTimer()
            mouse.accepted = false
        }

        onReleased: function(mouse) {
            win.resetIdleTimer()
            mouse.accepted = false
        }

        onClicked: function(mouse) {
            win.resetIdleTimer()
            mouse.accepted = false
        }
    }
}
