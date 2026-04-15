import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import App 1.0

Rectangle {
    id: testEx
    width: stackViewList.width
    height: stackViewList.height

    property int countdown: 0
    property int selectedMinutes: 1
    property string testName: ""

    property bool pendingExit: false
    property bool commandLocked: false
    property bool stopInProgress: false
    property bool initializingStop: false

    function isRunning() {
        return panel.testRunning
    }

    function syncCountdownFromSelection() {
        countdown = indexCh > -2 ? selectedMinutes * 60 : selectedMinutes * 3600
        if (indexCh === -1)
            countdown = panel.calcAllLinesTestDurationSec()
    }

    function stopCurrent() {
        if (stopInProgress)
            return

        commandLocked = true
        stopInProgress = true
        panel.stopCurrentTest()
    }

    function startCurrent() {
        if (commandLocked)
            return

        syncCountdownFromSelection()

        if (countdown <= 0) {
            panel.writeLog("Длительность теста должна быть больше нуля")
            return
        }

        commandLocked = true

        var ok = false

        if (indexCh >= -1) {
            ok = panel.startLineTest(indexCh, countdown)
        } else if (indexCh === -2) {
            ok = panel.startDurationTest()
        }

        if (!ok) {
            commandLocked = false
            panel.writeLog("Не удалось запустить тест")
            return
        }

        win.testStart = true
    }

    function requestExit() {
        if (pendingExit)
            return

        if (panel.testRunning) {
            stopInProgress = true
            panel.stopCurrentTest()
            return
        }

        stackViewList.replace("pageTest.qml")
    }

    function loadTitle() {
        selectedMinutes = indexCh > -1 ? durationTst :
                          indexCh > -2 ? panel.calcAllLinesTestDurationSec() :
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
            win.testStart = panel.testRunning

            if (panel.testRunning) {
                if (!countdownTimer.running && countdown > 0)
                    countdownTimer.start()
            } else {
                countdownTimer.stop()
                countdown = 0
            }

            if (initializingStop) {
                if (!panel.testRunning) {
                    initializingStop = false
                    stopInProgress = false
                    commandLocked = false
                }
                return
            }

            commandLocked = false

            if (!panel.testRunning) {
                stopInProgress = false
            }

            if (pendingExit && !panel.testRunning) {
                pendingExit = false
                stackViewList.replace("pageTest.qml")
            }
        }
    }

    Component.onCompleted: {
        loadTitle()
        initializingStop = true
        commandLocked = true
        stopInProgress = true
        panel.stopCurrentTest()
        win.testStart = panel.testRunning
    }

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
                    color: commandLocked || pendingExit ? "gray" : "orange"

                    Text {
                        anchors.centerIn: parent
                        text: panel.testRunning ? "Стоп" : "Старт"
                        font.pixelSize: 40
                        color: "white"
                    }

                    MouseArea {
                        anchors.fill: parent
                        enabled: !commandLocked && !pendingExit

                        onClicked: {
                            if (commandLocked || pendingExit)
                                return

                            if (panel.testRunning)
                                stopCurrent()
                            else
                                startCurrent()
                        }
                    }
                }

                RowLayout {
                    spacing: 10

                    Text {
                        text: indexCh > -1 ? "Время теста, мин:" :
                              indexCh == -1 ? "Время теста, с:" :
                                               "Время теста, ч:"
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
                        text: panel.testRunning
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
                                if (panel.testRunning)
                                    testEx.stopCurrent()
                                else
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
                        enabled: !panel.testRunning && !pendingExit && !commandLocked
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
                color: pendingExit ? "gray" : "lightgray"
                radius: 6

                Image {
                    anchors.fill: parent
                    source: "qrc:/Back.png"
                    fillMode: Image.PreserveAspectFit
                }

                MouseArea {
                    anchors.fill: parent
                    hoverEnabled: true
                    enabled: !pendingExit

                    onEntered: btnRet.color = pendingExit ? "gray" : "lightgray"
                    onExited: btnRet.color = pendingExit ? "gray" : "lightgray"
                    onPressed: if (!pendingExit) btnRet.color = "gray"
                    onReleased: if (!pendingExit) btnRet.color = "lightgray"

                    onClicked: requestExit()
                }
            }
        }
    }
}
