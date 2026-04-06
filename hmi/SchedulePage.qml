import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

Page {
    id: page
    title: "Расписание тестов"

    property int currentRow: -1

    function refresh() {
        list.model = panel.getAllTests()
        if (currentRow >= list.count)
            currentRow = -1
    }
    function weekDaysToText(weekDays) {
        if (!weekDays)
            return "-"

        var names = {
            "Mon": "Пн",
            "Tue": "Вт",
            "Wed": "Ср",
            "Thu": "Чт",
            "Fri": "Пт",
            "Sat": "Сб",
            "Sun": "Вс"
        }

        var arr = []
        for (var i = 0; i < weekDays.length; ++i)
            arr.push(names[weekDays[i]] || weekDays[i])

        return arr.length > 0 ? arr.join(", ") : "-"
    }

    Component.onCompleted: refresh()
    onVisibleChanged: if (visible) refresh()

    header: ToolBar {
        implicitHeight: 110

        RowLayout {
            anchors.fill: parent
            anchors.margins: 10
            spacing: 12

            Rectangle {
                id: btnRet
                width: 80
                height: 80
                radius: 6
                color: "lightgray"
                Layout.alignment: Qt.AlignVCenter

                Image {
                    anchors.fill: parent
                    source: "qrc:/Back.png"
                    fillMode: Image.PreserveAspectFit
                }

                MouseArea {
                    anchors.fill: parent
                    onClicked: stackViewList.replace("pageStart.qml")
                }
            }

            Item { Layout.fillWidth: true }

            Label {
                text: "Расписание тестов"
                font.pixelSize: 34
                Layout.alignment: Qt.AlignHCenter | Qt.AlignVCenter
            }

            Item { Layout.fillWidth: true }

            Rectangle {
                width: 200
                height: 80
                radius: 10
                color: "orange"
                Layout.alignment: Qt.AlignVCenter

                Text {
                    anchors.centerIn: parent
                    text: "Добавить"
                    font.pixelSize: 26
                    color: "white"
                }

                MouseArea {
                    anchors.fill: parent
                    onClicked: {
                        panel.addTest({
                            "enabled": true,
                            "period": "один раз",
                            "startDate": "2026-01-15",
                            "startTime": "00:00",
                            "testType": "Функциональный",
                            "weekDays": []
                        })
                        panel.writeLog("Добавлена запись расписания")
                        refresh()
                    }
                }
            }

            Rectangle {
                width: 200
                height: 80
                radius: 10
                color: currentRow >= 0 ? "orange" : "grey"
                Layout.alignment: Qt.AlignVCenter

                Text {
                    anchors.centerIn: parent
                    text: "Удалить"
                    font.pixelSize: 26
                    color: "white"
                }

                MouseArea {
                    anchors.fill: parent
                    onClicked: {
                        if (currentRow >= 0) {
                            panel.removeTest(currentRow)
                            panel.writeLog("Удалена запись №" + currentRow)
                            currentRow = -1
                            refresh()
                        }
                    }
                }
            }
        }
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 12
        spacing: 8

        Rectangle {
            Layout.fillWidth: true
            height: 50
            color: "#f2f2f2"
            border.color: "#d0d0d0"
            radius: 6

            RowLayout {
                anchors.fill: parent
                anchors.margins: 6
                spacing: 6

                Label { text: "Период"; Layout.preferredWidth: 170; font.pixelSize: 24 }
                Label { text: "Дата"; Layout.preferredWidth: 140; font.pixelSize: 24 }
                Label { text: "Время"; Layout.preferredWidth: 90; font.pixelSize: 24 }
                Label { text: "Дни"; Layout.preferredWidth: 280; font.pixelSize: 24 }
                Label { text: "Описание теста"; Layout.fillWidth: true; font.pixelSize: 24 }
                Label { text: "  "; Layout.preferredWidth: 90; horizontalAlignment: Text.AlignHCenter; font.pixelSize: 24 }
            }
        }

        Frame {
            Layout.fillWidth: true
            Layout.fillHeight: true
            padding: 0

            ListView {
                id: list
                anchors.fill: parent
                clip: true
                spacing: 4

                delegate: Rectangle {
                    width: list.width
                    height: 52
                    radius: 6
                    color: ListView.isCurrentItem ? "#eaf4ff" : "#ffffff"
                    border.color: ListView.isCurrentItem ? "#80b0ff" : "#cccccc"

                    MouseArea {
                        anchors.fill: parent
                        onClicked: {
                            list.currentIndex = index
                            page.currentRow = index
                        }
                    }

                    RowLayout {
                        anchors.fill: parent
                        spacing: 6

                        Label {
                            text: " " + modelData.period
                            Layout.preferredWidth: 170
                            font.pixelSize: 24
                        }

                        Label {
                            text: modelData.startDate
                            Layout.preferredWidth: 140
                            font.pixelSize: 24
                        }

                        Label {
                            text: modelData.startTime
                            Layout.preferredWidth: 90
                            font.pixelSize: 24
                        }
                        Label {
                            text: modelData.period === "дни недели"
                                  ? page.weekDaysToText(modelData.weekDays)
                                  : "-"
                            Layout.preferredWidth: 280
                            font.pixelSize: 22
                            elide: Text.ElideRight
                        }

                        Label {
                            text: modelData.testType
                            Layout.fillWidth: true
                            font.pixelSize: 24
                            elide: Text.ElideRight
                        }

                        Rectangle {
                            width: 90
                            height: 52
                            radius: 6
                            color: "orange"

                            Text {
                                anchors.bottom: parent.bottom
                                bottomPadding: 10
                                leftPadding: 10
                                text: "настр."
                                font.pixelSize: 24
                            }

                            MouseArea {
                                anchors.fill: parent
                                onClicked: {
                                    stackViewList.replace("TestSettingsPage.qml", {
                                        currentIndex: index
                                    })
                                }
                            }
                        }
                    }
                }

                ScrollBar.vertical: ScrollBar {
                    policy: ScrollBar.AlwaysOn
                }
            }
        }
    }
}
