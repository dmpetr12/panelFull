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
        syncUiFromEntry()
    }

    function syncUiFromEntry() {
        var now = new Date()

        periodBox.currentIndex = Math.max(0, periodBox.model.indexOf(entry.period || "один раз"))
        testTypeBox.currentIndex = Math.max(0, testTypeBox.model.indexOf(entry.testType || "Функциональный"))

        timeChenge.ready = false
        weekRow.ready = false

        timeChenge.year = now.getFullYear()
        timeChenge.month = now.getMonth() + 1
        timeChenge.day = now.getDate()
        timeChenge.hour = now.getHours()
        timeChenge.minute = now.getMinutes()

        if (entry.startDate && entry.startDate.length === 10) {
            var d = entry.startDate.split("-")
            if (d.length === 3) {
                var y = parseInt(d[0])
                var m = parseInt(d[1])
                var dd = parseInt(d[2])

                if (!isNaN(y))
                    timeChenge.year = y
                if (!isNaN(m))
                    timeChenge.month = m
                if (!isNaN(dd))
                    timeChenge.day = dd
            }
        }

        if (entry.startTime && entry.startTime.length >= 5) {
            var t = entry.startTime.split(":")
            if (t.length >= 2) {
                var hh = parseInt(t[0])
                var mm = parseInt(t[1])

                if (!isNaN(hh))
                    timeChenge.hour = hh
                if (!isNaN(mm))
                    timeChenge.minute = mm
            }
        }

        weekRow.localDays = entry.weekDays ? entry.weekDays.slice(0) : []

        timeChenge.ready = true
        weekRow.ready = true
    }

    Component.onCompleted: loadEntry()
    onVisibleChanged: if (visible) loadEntry()
    onCurrentIndexChanged: loadEntry()

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
                Layout.preferredWidth: 600
                Layout.preferredHeight: 50
                font.pixelSize: 34
                model: ["один раз", "ежедневно", "дни недели", "раз в месяц", "раз в 3 месяца", "раз в полгода", "раз в год"]

                onActivated: {
                    if (settingsPage.currentIndex < 0)
                        return

                    var value = model[currentIndex]
                    panel.updateTestProperty(settingsPage.currentIndex, "period", value)
                    panel.writeLog("period=" + value)
                    entry.period = value

                    if (value !== "дни недели") {
                        weekRow.localDays = []
                        weekRow.ready = false
                        panel.updateWeekDays(settingsPage.currentIndex, [])
                        panel.writeLog("weekDays cleared")
                        entry.weekDays = []
                        weekRow.ready = true
                    }
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

                function saveDate() {
                    if (!ready || settingsPage.currentIndex < 0)
                        return

                    month = Math.max(1, Math.min(month, 12))
                    day = Math.max(1, Math.min(day, 31))

                    var s = year + "-" + ("0" + month).slice(-2) + "-" + ("0" + day).slice(-2)
                    panel.updateTestProperty(settingsPage.currentIndex, "startDate", s)
                    panel.writeLog("startDate=" + s)
                    entry.startDate = s
                }

                function saveTime() {
                    if (!ready || settingsPage.currentIndex < 0)
                        return

                    hour = Math.max(0, Math.min(hour, 23))
                    minute = Math.max(0, Math.min(minute, 59))

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
                    Layout.fillWidth: true
                    enabled: periodBox.currentText === "дни недели"
                    opacity: periodBox.currentText === "дни недели" ? 1.0 : 0.35

                    Label {
                        text: "Дни недели:"
                        font.pixelSize: 30
                    }

                    RowLayout {
                        id: weekRow
                        spacing: 10
                        Layout.fillWidth: true

                        property bool ready: false

                        property var daysList: [
                            { short: "Пн", key: "Mon" },
                            { short: "Вт", key: "Tue" },
                            { short: "Ср", key: "Wed" },
                            { short: "Чт", key: "Thu" },
                            { short: "Пт", key: "Fri" },
                            { short: "Сб", key: "Sat" },
                            { short: "Вс", key: "Sun" }
                        ]

                        property var localDays: []

                        function saveWeekDaysInternal(days) {
                            if (settingsPage.currentIndex < 0)
                                return

                            localDays = days
                            panel.updateWeekDays(settingsPage.currentIndex, days)
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
                                    if (!weekRow.ready)
                                        return

                                    var arr = weekRow.localDays.slice(0)
                                    var pos = arr.indexOf(modelData.key)

                                    if (checked && pos === -1)
                                        arr.push(modelData.key)

                                    if (!checked && pos !== -1)
                                        arr.splice(pos, 1)

                                    weekRow.localDays = arr
                                    weekRow.saveWeekDaysInternal(arr)
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
                Layout.preferredWidth: 600
                Layout.preferredHeight: 50
                font.pixelSize: 36
                model: ["Функциональный", "На время"]

                onActivated: {
                    if (settingsPage.currentIndex < 0)
                        return

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