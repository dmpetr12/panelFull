import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

Page {
    id: lineEditPage

    property int lineMode: 0
    property string lineDescription: ""
    property double lineMpower: 0
    property double lineTolerance: 0
    property bool loadingLine: false

    // новые флаги
    property bool pendingExit: false
    property bool stopInProgress: false
    property bool testButtonLocked: false

    function loadLine() {
        loadingLine = true

        var ln = panel.lineAt(indexCh)
        if (!ln) {
            loadingLine = false
            return
        }

        lineDescription = ln.description || ""
        lineMpower = Number(ln.mpower || 0)
        lineTolerance = Number(ln.tolerance || 0)
        lineMode = Number(ln.mode || 0)

        loadingLine = false
    }

    function saveLine() {
        panel.updateLine(indexCh, {
            "description": lineNameField.text,
            "mpower": lineMpower,
            "tolerance": lineTolerance,
            "mode": lineMode
        })
    }

    function goBackNow() {
        saveLine()
        panel.applyLineModes()
        panel.saveLines()
        stackViewList.replace("pageAjaste.qml")
    }

    function requestStopAndExit() {
        if (stopInProgress || pendingExit)
            return

        pendingExit = true
        stopInProgress = true
        testButtonLocked = true

        panel.stopLineTest(indexCh)
    }

    Connections {
        target: panel

        function onChanged() {
            win.testStart = panel.testRunning

            // пришло новое состояние от backend — кнопку можно снова жать
            testButtonLocked = false

            // если ждали остановку для выхода назад — уходим только когда тест реально остановлен
            if (pendingExit && !panel.testRunning) {
                pendingExit = false
                stopInProgress = false
                goBackNow()
                return
            }

            // если stop завершился — сбрасываем флаг
            if (!panel.testRunning)
                stopInProgress = false
        }
    }

    Component.onCompleted: {
        loadLine()
        win.testStart = panel.testRunning
    }

    // аварийная страховка:
    // если страницу убрали не через кнопку "назад", пробуем остановить тест
    Component.onDestruction: {
        if (panel.testRunning)
            panel.stopLineTest(indexCh)
    }

    Column {
        anchors.fill: parent
        anchors.leftMargin: 20

        Text {
            text: "Настройка линии: " + lineDescription
            font.pixelSize: 30
        }

        GridLayout {
            columns: 2
            rowSpacing: 10
            columnSpacing: 10

            Text {
                text: "Описание:"
                font.pixelSize: 30
                Layout.alignment: Qt.AlignRight
            }

            Rectangle {
                width: 580
                height: 40

                TextField {
                    id: lineNameField
                    width: 580
                    text: lineDescription
                    font.pixelSize: 30
                    readOnly: true

                    TapHandler {
                        onTapped: keyboard.openFor(lineNameField)
                    }
                }
            }

            Rectangle {
                width: 250
                height: 90
                radius: 12
                color: testButtonLocked ? "gray" : "orange"

                Text {
                    anchors.centerIn: parent
                    text: panel.testRunning ? "Стоп" : "Тест"
                    font.pixelSize: 30
                    color: "white"
                }

                MouseArea {
                    anchors.fill: parent
                    enabled: !testButtonLocked && !pendingExit

                    onClicked: {
                        if (testButtonLocked || pendingExit)
                            return

                        testButtonLocked = true

                        if (panel.testRunning) {
                            stopInProgress = true
                            panel.stopLineTest(indexCh)
                        } else {
                            var ok = panel.startLineTest(indexCh, durationTst * 60 * 5)
                            if (!ok)
                                testButtonLocked = false
                        }
                    }
                }
            }

            Rectangle {
                width: 250
                height: 90
                radius: 12
                color: "orange"
                visible: panel.testRunning

                Text {
                    anchors.centerIn: parent
                    text: "ввести \nизмеренное"
                    font.pixelSize: 30
                    color: "white"
                }

                MouseArea {
                    anchors.fill: parent
                    enabled: !pendingExit
                    onClicked: lineMpower = panel.testPValue
                }
            }

            Text {
                Layout.row: 4
                text: "Установочная мощность, Вт:"
                font.pixelSize: 30
                Layout.alignment: Qt.AlignRight
            }

            RowLayout {
                spacing: 20

                Text {
                    font.pixelSize: 30
                    text: lineMpower.toFixed(1)
                }

                Button {
                    text: "ввести значение"
                    font.pixelSize: 30
                    enabled: !pendingExit
                    onClicked: digitalPopup.openFor(
                                   "Введите мощность",
                                   lineEditPage,
                                   "lineMpower",
                                   lineMpower,
                                   maxPower)
                }
            }

            Text {
                Layout.row: 5
                text: "Мощность, Вт:"
                font.pixelSize: 30
                Layout.alignment: Qt.AlignRight
            }

            Label {
                visible: panel.testRunning
                text: panel.testPAvailable ? Number(panel.testPValue).toFixed(1) : "—"
                font.pixelSize: 30
            }

            Text {
                Layout.row: 6
                text: "Напряжение, В:"
                font.pixelSize: 30
                Layout.alignment: Qt.AlignRight
            }

            Label {
                visible: panel.testRunning
                text: panel.testUAvailable ? Number(panel.testUValue).toFixed(1) : "—"
                font.pixelSize: 30
            }

            Text {
                Layout.row: 7
                text: "Ток, А:"
                font.pixelSize: 30
                Layout.alignment: Qt.AlignRight
            }

            Label {
                visible: panel.testRunning
                text: panel.testIAvailable ? Number(panel.testIValue).toFixed(3) : "—"
                font.pixelSize: 30
            }

            Text {
                Layout.row: 8
                text: "Допуск, %"
                font.pixelSize: 30
                Layout.alignment: Qt.AlignRight
            }

            RowLayout {
                spacing: 20

                Text {
                    font.pixelSize: 30
                    text: lineTolerance.toFixed(0)
                }

                Button {
                    text: "ввести значение"
                    font.pixelSize: 30
                    enabled: !pendingExit
                    onClicked: digitalPopup.openFor(
                                   "Введите допуск, %",
                                   lineEditPage,
                                   "lineTolerance",
                                   lineTolerance,
                                   maxTolerance)
                }
            }

            Text {
                text: "Режим работы:"
                font.pixelSize: 35
                Layout.alignment: Qt.AlignRight
            }

            Rectangle {
                width: 400
                height: 40

                ComboBox {
                    id: control
                    anchors.fill: parent
                    font.pixelSize: 35
                    enabled: !pendingExit

                    delegate: ItemDelegate {
                        width: control.width
                        text: modelData
                        font.weight: control.currentIndex === index ? Font.DemiBold : Font.Normal
                        highlighted: control.highlightedIndex === index
                        hoverEnabled: control.hoverEnabled
                    }

                    model: ["постоянный", "непостоянный", "линия отключена"]
                    currentIndex: lineMode

                    onActivated: function(index) {
                        lineMode = index
                        saveLine()
                        panel.applyLineModes()
                    }
                }
            }
        }

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

                onClicked: {
                    if (pendingExit)
                        return

                    if (panel.testRunning) {
                        requestStopAndExit()
                        return
                    }

                    goBackNow()
                }
            }
        }
    }

    OperatorKeyboard {
        id: keyboard
        parent: lineEditPage
    }
}