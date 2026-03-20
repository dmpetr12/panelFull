import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import App 1.0

Rectangle {
    id: testEx
    width: stackViewList.width
    height: stackViewList.height

    property int countdown: 10
    property int selectedMinutes: 1
    property string testName: ""

    function isRunning() {
        return countdownTimer.running
    }

    function stopCurrent() {
        countdownTimer.stop()
        countdown = 0
        panel.stopCurrentTest()
        win.testStart = false
    }

    function startCurrent() {
        countdown = indexCh > -2 ? selectedMinutes * 60 : selectedMinutes * 3600
        if (indexCh === -1)
            countdown = panel.calcAllLinesTestDurationSec()
        if (countdown <= 0) {
            console.log("Длительность теста должна быть больше нуля")
            return
        }

        var ok = false

        if (indexCh >= -1) {
            ok = panel.startLineTest(indexCh, countdown)
        } else if (indexCh === -2) {
            ok = panel.startDurationTest()
        }

        if (!ok) {
            console.log("Не удалось запустить тест")
            return
        }

        countdownTimer.start()
        win.testStart = true
    }

    function loadTitle() {
        selectedMinutes = indexCh > -1 ? durationTst :
                          indexCh > -2 ?   Math.ceil(panel.calcAllLinesTestDurationSec() / 60) :
                          durationAv

        if (indexCh >= 0) {
            var ln = panel.lineAt(indexCh)
            testName = ln && ln.description ? ("линия " + ln.description) : ("линия " + (indexCh + 1))
        } else if (indexCh === -1) {
            testName = "все линии"
        } else if (indexCh === -2) {
            testName = "все линии на время"
        } else {
            testName = ""
        }
    }

    Connections {
        target: panel
        function onChanged() {
            if (!panel.testRunning && countdownTimer.running) {
                countdownTimer.stop()
                countdown = 0
                win.testStart = false
            }
        }
    }

    Component.onCompleted: {
        loadTitle()
        panel.stopCurrentTest()
    }

    Component.onDestruction: stopCurrent()


    Column {
        anchors.fill: parent
        anchors.margins: 50
        spacing: 20

        Row {
            Text {
                text: "ТЕСТ: " + testName
                font.pixelSize: 40
            }
        }

        Row {
            spacing: 80

            Column {
                spacing: 20

                Rectangle {
                    width: 250
                    height: 90
                    radius: 12
                    color: "orange"

                    Text {
                        anchors.centerIn: parent
                        text: countdownTimer.running ? "Стоп" : "Старт"
                        font.pixelSize: 40
                        color: "white"
                    }

                    MouseArea {
                        anchors.fill: parent
                        onClicked: {
                            if (countdownTimer.running)
                                stopCurrent()
                            else
                                startCurrent()
                        }
                    }
                }

                RowLayout {
                    spacing: 10

                    Text {
                        text: indexCh > -2 ? "Время теста, мин:" : "Время теста, ч:"
                        font.pixelSize: 40
                        verticalAlignment: Text.AlignVCenter
                    }
                }
            }

            Column {
                spacing: 20

                Rectangle {
                    width: 250
                    height: 90
                    border.color: "lightgray"
                    border.width: 2
                    radius: 10

                    Text {
                        id: counter
                        anchors.centerIn: parent
                        font.pixelSize: 60
                        text: countdownTimer.running
                              ? Math.floor(countdown / 60) + ":" + ("0" + (countdown % 60)).slice(-2)
                              : ""
                    }

                    Timer {
                        id: countdownTimer
                        interval: 1000
                        repeat: true
                        running: false

                        onTriggered: {
                            if (countdown > 0) {
                                countdown--
                            } else {
                                stop()
                                win.testStart = false
                            }
                        }
                    }
                }

                RowLayout {
                    spacing: 20

                    Text {
                        id: selectedMin
                        font.pixelSize: 40
                        text: selectedMinutes.toString()
                    }

                    Button {
                        visible: indexCh > -1
                        text: "ввести значение"
                        font.pixelSize: 40
                        property int p: indexCh > -2 ? maxMinute : maxHour
                        onClicked: digitalPopup.openFor(
                                       "Введите время теста",
                                       testEx,
                                       "selectedMinutes",
                                       selectedMinutes,
                                       p)
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
                    onClicked: stackViewList.replace("pageTest.qml")
                }
            }
        }
    }
}
