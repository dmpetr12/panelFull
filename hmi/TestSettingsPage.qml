import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

Page {
    id: settingsPage
    title: "Настройки теста"

    property int currentIndex: -1
    property var entry: ({})

    function loadEntry() {
        var all = panel.getAllTests()
        entry = (currentIndex >= 0 && currentIndex < all.length) ? all[currentIndex] : ({})
    }

    function saveWeekDays(days) {
        panel.updateTestProperty(currentIndex, "weekDays", days)
        panel.writeLog("Изменены weekDays: " + days.join(","))
        entry.weekDays = days
    }

    Component.onCompleted: loadEntry()
    onVisibleChanged: if (visible) loadEntry()

    header: ToolBar {
        height: 110

        RowLayout {
            anchors.fill: parent
            spacing: 12
            anchors.margins: 10

            Rectangle {
                id: btnRet
                width: 80
                height: 80
                radius: 6
                color: "lightgray"

                Image {
                    anchors.fill: parent
                    source: "qrc:/Back.png"
                    fillMode: Image.PreserveAspectFit
                }

                MouseArea {
                    anchors.fill: parent
                    onClicked: stackViewList.replace("SchedulePage.qml")
                }
            }

            Label {
                text: "Настройка теста"
                font.pixelSize: 40
                Layout.fillWidth: true
                horizontalAlignment: Text.AlignHCenter
            }
        }
    }

    Rectangle {
        anchors.fill: parent
        color: "white"

        GridLayout {
            anchors.fill: parent
            anchors.margins: 20
            columns: 2
            columnSpacing: 28
            rowSpacing: 22

            Label {
                text: "Периодичность:"
                font.pixelSize: 36
                Layout.alignment: Qt.AlignRight | Qt.AlignVCenter
            }

            ComboBox {
                id: periodBox
                width: 600
                implicitWidth: width
                height: 50
                implicitHeight: height
                font.pixelSize: 34
                model: ["один раз", "ежедневно", "дни недели", "раз в месяц", "раз в 3 месяца", "раз в полгода", "раз в год"]

                Component.onCompleted: {
                    periodBox.currentIndex = model.indexOf(entry.period || "один раз")
                }

                onActivated: {
                    var value = model[currentIndex]
                    panel.updateTestProperty(settingsPage.currentIndex, "period", value)
                    panel.writeLog("period=" + value)
                    entry.period = value
                }

                popup: Popup {
                    y: periodBox.height
                    width: periodBox.width
                    implicitHeight: contentItem.implicitHeight + 8
                    modal: true
                    clip: false

                    background: Rectangle {
                        color: "white"
                        border.color: "#aaa"
                        radius: 8
                    }

                    contentItem: ListView {
                        implicitHeight: contentHeight
                        model: periodBox.delegateModel
                        currentIndex: periodBox.highlightedIndex
                        clip: true
                        interactive: false
                    }
                }

                delegate: ItemDelegate {
                    width: periodBox.width
                    text: modelData
                    font.pixelSize: 32
                    highlighted: periodBox.highlightedIndex === index
                }
            }

            Label {
                text: "Дата и время\nпервого старта:"
                font.pixelSize: 36
                Layout.alignment: Qt.AlignRight | Qt.AlignTop
            }

            ColumnLayout {
                id: timeChenge
                spacing: 20
                Layout.fillWidth: true

                property int year: new Date().getFullYear()
                property int month: new Date().getMonth() + 1
                property int day: new Date().getDate()
                property int hour: new Date().getHours()
                property int minute: new Date().getMinutes()
                property bool ready: false

                Component.onCompleted: {
                    if (entry.startDate && entry.startDate.length === 10) {
                        var d = entry.startDate.split("-")
                        if (d.length === 3) {
                            year = parseInt(d[0])
                            month = parseInt(d[1])
                            day = parseInt(d[2])
                        }
                    }

                    if (entry.startTime && entry.startTime.length >= 5) {
                        var t = entry.startTime.split(":")
                        if (t.length >= 2) {
                            hour = parseInt(t[0])
                            minute = parseInt(t[1])
                        }
                    }

                    ready = true
                }

                function saveDate() {
                    if (!ready)
                        return
                    var s = year + "-" + ("0" + month).slice(-2) + "-" + ("0" + day).slice(-2)
                    panel.updateTestProperty(settingsPage.currentIndex, "startDate", s)
                    panel.writeLog("startDate=" + s)
                    entry.startDate = s
                }

                function saveTime() {
                    if (!ready)
                        return
                    var s = ("0" + hour).slice(-2) + ":" + ("0" + minute).slice(-2)
                    panel.updateTestProperty(settingsPage.currentIndex, "startTime", s)
                    panel.writeLog("startTime=" + s)
                    entry.startTime = s
                }

                onYearChanged: saveDate()
                onMonthChanged: saveDate()
                onDayChanged: saveDate()

                onHourChanged: saveTime()
                onMinuteChanged: saveTime()

                RowLayout {
                    spacing: 10

                    Label {
                        text: "  Дата:"
                        font.pixelSize: 30
                        verticalAlignment: Text.AlignVCenter
                    }

                    Rectangle {
                        width: 80
                        height: 70
                        radius: 10
                        color: "lightgrey"

                        Text {
                            anchors.centerIn: parent
                            text: ("0" + timeChenge.day).slice(-2)
                            font.pixelSize: 40
                        }

                        MouseArea {
                            anchors.fill: parent
                            hoverEnabled: true
                            onPressed: digitalPopup.openFor("Введите день", timeChenge, "day", timeChenge.day, 31)
                        }
                    }

                    Rectangle {
                        width: 80
                        height: 70
                        radius: 10
                        color: "lightgrey"

                        Text {
                            anchors.centerIn: parent
                            text: ("0" + timeChenge.month).slice(-2)
                            font.pixelSize: 40
                        }

                        MouseArea {
                            anchors.fill: parent
                            hoverEnabled: true
                            onPressed: digitalPopup.openFor("Введите месяц", timeChenge, "month", timeChenge.month, 12)
                        }
                    }

                    Rectangle {
                        width: 120
                        height: 70
                        radius: 10
                        color: "lightgrey"

                        Text {
                            anchors.centerIn: parent
                            text: timeChenge.year
                            font.pixelSize: 40
                        }

                        MouseArea {
                            anchors.fill: parent
                            hoverEnabled: true
                            onPressed: digitalPopup.openFor("Введите год", timeChenge, "year", timeChenge.year, 2100)
                        }
                    }
                }

                RowLayout {
                    spacing: 10

                    Label {
                        text: "Время:"
                        font.pixelSize: 30
                        verticalAlignment: Text.AlignVCenter
                    }

                    Rectangle {
                        width: 80
                        height: 70
                        radius: 10
                        color: "lightgrey"

                        Text {
                            anchors.centerIn: parent
                            text: ("0" + timeChenge.hour).slice(-2)
                            font.pixelSize: 40
                        }

                        MouseArea {
                            anchors.fill: parent
                            hoverEnabled: true
                            onPressed: digitalPopup.openFor("Введите час", timeChenge, "hour", timeChenge.hour, 23)
                        }
                    }

                    Rectangle {
                        width: 80
                        height: 70
                        radius: 10
                        color: "lightgrey"

                        Text {
                            anchors.centerIn: parent
                            text: ("0" + timeChenge.minute).slice(-2)
                            font.pixelSize: 40
                        }

                        MouseArea {
                            anchors.fill: parent
                            hoverEnabled: true
                            onPressed: digitalPopup.openFor("Введите минуты", timeChenge, "minute", timeChenge.minute, 59)
                        }
                    }
                }

                ColumnLayout {
                    spacing: 10

                    Label {
                        text: "Дни недели:"
                        font.pixelSize: 30
                    }

                    RowLayout {
                        id: weekRow
                        spacing: 10
                        Layout.fillWidth: true

                        property var daysList: [
                            { short: "Пн", key: "Mon" },
                            { short: "Вт", key: "Tue" },
                            { short: "Ср", key: "Wed" },
                            { short: "Чт", key: "Thu" },
                            { short: "Пт", key: "Fri" },
                            { short: "Сб", key: "Sat" },
                            { short: "Вс", key: "Sun" }
                        ]

                        property var localDays: (entry.weekDays ? entry.weekDays.slice(0) : [])

                        function saveWeekDaysInternal(days) {
                            localDays = days
                            panel.updateTestProperty(settingsPage.currentIndex, "weekDays", days)
                            panel.writeLog("weekDays=" + days.join(","))
                            entry.weekDays = days
                        }

                        Repeater {
                            model: weekRow.daysList

                            CheckBox {
                                id: dayCheck
                                text: modelData.short
                                font.pixelSize: 36
                                checked: weekRow.localDays.indexOf(modelData.key) !== -1

                                indicator: Rectangle {
                                    implicitWidth: 30
                                    implicitHeight: 30
                                    radius: 6
                                    border.width: 2
                                    border.color: dayCheck.checked ? "orange" : "#808080"
                                    color: dayCheck.checked ? "orange" : "white"
                                }

                                onToggled: {
                                    var arr = weekRow.localDays.slice(0)
                                    var pos = arr.indexOf(modelData.key)
                                    if (checked && pos === -1)
                                        arr.push(modelData.key)
                                    if (!checked && pos !== -1)
                                        arr.splice(pos, 1)
                                    weekRow.localDays = arr
                                    saveWeekDaysInternal(arr)
                                }
                            }
                        }
                    }
                }
            }

            Label {
                text: "Тип теста:"
                font.pixelSize: 36
                Layout.alignment: Qt.AlignRight | Qt.AlignVCenter
            }

            ComboBox {
                id: testTypeBox
                implicitWidth: 600
                implicitHeight: 50
                font.pixelSize: 36
                model: ["Функциональный", "На время"]

                Component.onCompleted: {
                    testTypeBox.currentIndex = model.indexOf(entry.testType || "Функциональный")
                }

                onActivated: {
                    var value = model[currentIndex]
                    panel.updateTestProperty(settingsPage.currentIndex, "testType", value)
                    panel.writeLog("testType=" + value)
                    entry.testType = value
                }

                popup: Popup {
                    y: testTypeBox.height
                    width: 600
                    implicitHeight: contentItem.implicitHeight
                    modal: true
                    clip: true

                    background: Rectangle {
                        color: "white"
                        border.color: "#aaa"
                        radius: 8
                    }

                    contentItem: ListView {
                        implicitHeight: contentHeight
                        model: testTypeBox.delegateModel
                        currentIndex: testTypeBox.highlightedIndex
                        clip: true
                    }
                }

                delegate: ItemDelegate {
                    width: 600
                    height: 90
                    text: modelData
                    font.pixelSize: 36
                    highlighted: testTypeBox.highlightedIndex === index
                }
            }
        }
    }
}
