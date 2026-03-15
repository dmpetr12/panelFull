import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

Page {
    id: lineEditPage

    property bool tested: false
    property int lineMode: 0
    property string lineDescription: ""
    property double lineMpower: 0
    property double lineTolerance: 0

    function loadLine() {
        var ln = panel.lineAt(indexCh)
        if (!ln)
            return

        lineDescription = ln.description || ""
        lineMpower = Number(ln.mpower || 0)
        lineTolerance = Number(ln.tolerance || 0)
        lineMode = Number(ln.mode || 0)
    }

    function saveLine() {
        panel.updateLine(indexCh, {
            "description": lineDescription,
            "mpower": lineMpower,
            "tolerance": lineTolerance,
            "mode": lineMode
        })
    }

    Component.onCompleted: {
        loadLine()
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
                    onTextChanged: lineDescription = text

                    TapHandler {
                        onTapped: keyboard.openFor(lineNameField)
                    }
                }
            }

            Rectangle {
                width: 250
                height: 90
                radius: 12
                color: "orange"

                Text {
                    anchors.centerIn: parent
                    text: tested ? "Стоп" : "Тест"
                    font.pixelSize: 30
                    color: "white"
                }

                MouseArea {
                    anchors.fill: parent
                    onClicked: {
                        if (tested) {
                            panel.stopLineTest(indexCh)
                            tested = false
                            win.testStart = false
                        } else {
                            panel.startLineTest(indexCh, durationTst * 300)
                            tested = true
                            win.testStart = true
                        }
                    }
                }
            }

            Rectangle {
                width: 250
                height: 90
                radius: 12
                color: "orange"
                visible: tested

                Text {
                    anchors.centerIn: parent
                    text: "ввести \nизмеренное"
                    font.pixelSize: 30
                    color: "white"
                }

                MouseArea {
                    anchors.fill: parent
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
                visible: tested
                text: Number(panel.testPValue).toFixed(1)
                font.pixelSize: 30
            }

            Text {
                Layout.row: 6
                text: "Напряжение, В:"
                font.pixelSize: 30
                Layout.alignment: Qt.AlignRight
            }

            Label {
                visible: tested
                text: Number(panel.testUValue).toFixed(1)
                font.pixelSize: 30
            }

            Text {
                Layout.row: 7
                text: "Ток, А:"
                font.pixelSize: 30
                Layout.alignment: Qt.AlignRight
            }

            Label {
                visible: tested
                text: Number(panel.testIValue).toFixed(3)
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

                    delegate: ItemDelegate {
                        width: control.width
                        text: modelData
                        font.weight: control.currentIndex === index ? Font.DemiBold : Font.Normal
                        highlighted: control.highlightedIndex === index
                        hoverEnabled: control.hoverEnabled
                    }

                    model: ["постоянный", "непостоянный", "линия отключена"]
                    currentIndex: lineMode

                    onCurrentIndexChanged: {
                        lineMode = currentIndex
                    }
                }
            }
        }

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
                onClicked: {
                    if (tested)
                        panel.stopLineTest(indexCh)

                    tested = false
                    win.testStart = false

                    saveLine()
                    panel.applyLineModes()
                    panel.saveLines()

                    stackViewList.replace("pageAjaste.qml")
                }
            }
        }
    }

    OperatorKeyboard {
        id: keyboard
        parent: lineEditPage
    }
}
