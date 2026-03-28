import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import App 1.0

Rectangle {
    id: root
    objectName: "pageStart"
    width: stackViewList.width
    height: stackViewList.height

    function recomputeMode() {
        btnStopTestVsb = false
        btnResetForcedFireVsb = false

        if (panel.fireActive) {
            appMode.state = Mode.Fire
            if (panel.forcedFireActive && unlocked)
                btnResetForcedFireVsb = true
            return
        }
        if (panel.forcedFireActive) {
            appMode.state = Mode.Fire
            if (unlocked)
                btnResetForcedFireVsb = true
            return
        }
        if (panel.dispatcherActive) {
            appMode.state = Mode.Force
            return
        }
        if (panel.testRunning) {
            appMode.state = Mode.Test
            if (unlocked)
                btnStopTestVsb = true
            return
        }

        appMode.state = Mode.Work
    }

    function recomputeSystemState() {
        sys.status = (panel.systemState === 2) ? System.Test
                  : (panel.systemState === 1) ? System.Failure
                  : System.Ok
    }
    property bool lastBackendConnected: true
    property bool btnStopTestVsb: false
    property bool btnResetForcedFireVsb: false

    Connections {
        target: panel
        function onChanged() {
            if (lastBackendConnected && !panel.connected) {
                console.log("Backend не подключён")
            }

            lastBackendConnected = panel.connected

            recomputeMode()
            recomputeSystemState()
            net.status = panel.busConnected ? Network.Ok : Network.Failure
        }
    }

    Component.onCompleted: {
        recomputeMode()
        recomputeSystemState()
        net.status = panel.busConnected ? Network.Ok : Network.Failure
    }

    Rectangle {
        id: body
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        height: 400
        anchors.margins: 12
        color: "transparent"

        RowLayout {
            anchors.fill: parent
            spacing: 20

            Rectangle {
                id: bodyleft
                height: parent.height
                width: 380
                anchors.margins: 12
                color: "transparent"

                Column {
                    anchors.fill: parent
                    spacing: 14

                    Network { id: net }
                    System { id: sys }
                    Mode { id: appMode }

                    Text {
                        text: "СИСТЕМА"
                        font.pixelSize: 40
                        color: "black"
                        font.bold: true
                        anchors.horizontalCenter: parent.horizontalCenter
                    }

                    RowLayout {
                        Layout.preferredHeight: 330
                        width: parent.width
                        anchors.margins: 10
                        spacing: 10

                        Rectangle {
                            width: 50
                            height: 50
                            color: appMode.stateColor
                            border.color: "black"
                            radius: 25
                        }

                        Text {
                            text: "Режим: "
                            color: "black"
                            font.pixelSize: 40
                            Layout.alignment: Qt.AlignVCenter
                        }

                        Text {
                            color: appMode.stateColor
                            text: appMode.stateText
                            font.pixelSize: 40
                            Layout.alignment: Qt.AlignVCenter
                            Layout.fillWidth: true
                            horizontalAlignment: Text.AlignRight
                        }
                    }

                    RowLayout {
                        Layout.preferredHeight: 330
                        width: parent.width
                        anchors.margins: 10
                        spacing: 10

                        Rectangle {
                            width: 50
                            height: 50
                            color: sys.statusColor
                            border.color: "black"
                            radius: 25
                        }

                        Text {
                            text: "Система: "
                            color: "black"
                            font.pixelSize: 40
                            Layout.alignment: Qt.AlignVCenter
                        }

                        Text {
                            id: sysText
                            text: sys.statusText
                            color: sys.statusColor
                            font.pixelSize: 40
                            Layout.alignment: Qt.AlignVCenter
                            Layout.fillWidth: true
                            horizontalAlignment: Text.AlignRight
                        }
                    }

                    GridLayout {
                        columns: 1
                        rowSpacing: 10
                        columnSpacing: 10

                        Text {
                            text: "Мощность  " + (panel.inletPAvailable ? panel.inletPValue.toFixed(1) + " Вт" : "—")
                            font.pixelSize: 30
                        }
                        Text {
                            text: "Напряжение  " + (panel.inletUAvailable ? panel.inletUValue.toFixed(1) + " В" : "—")
                            font.pixelSize: 30
                        }
                        Text {
                            text: "Ток  " + (panel.inletIAvailable ? panel.inletIValue.toFixed(1) + " A" : "—")
                            font.pixelSize: 30
                        }
                        Text {
                            text: "Частота  " + (panel.inletFAvailable ? panel.inletFValue.toFixed(1) + " Гц" : "—")
                            font.pixelSize: 30
                        }
                    }
                }
            }

            Rectangle {
                width: 2
                height: parent.height
                color: "black"
            }

            Rectangle {
                id: bodyright
                Layout.fillWidth: true
                Layout.fillHeight: true
                color: "transparent"

                Column {
                    anchors.fill: parent
                    spacing: 0

                    Text {
                        text: "ЛИНИИ"
                        font.pixelSize: 40
                        color: "black"
                        font.bold: true
                        anchors.horizontalCenter: parent.horizontalCenter
                    }

                    Text {
                        text: "___"
                        font.pixelSize: 25
                        color: "white"
                        anchors.horizontalCenter: parent.horizontalCenter
                    }

                    ListView {
                        id: listView
                        width: parent.width
                        height: 280
                        model: panel.lines
                        reuseItems: true

                        delegate: Item {
                            width: ListView.view ? ListView.view.width : 0
                            height: (modelData.mode !== 2 && index < panel.lineCount) ? 50 : 0
                            visible: height > 0

                            RowLayout {
                                anchors.fill: parent
                                anchors.margins: 0
                                spacing: 10

                                Rectangle {
                                    width: 24
                                    height: 24
                                    radius: 12
                                    border.color: "black"
                                    color: modelData.status === 0 ? "#5EC85E"
                                          : modelData.status === 1 ? "#FF4C4C"
                                          : modelData.status === 2 ? "#FFC700"
                                          : "blue"
                                }

                                Rectangle {
                                    width: 250
                                    height: 38
                                    radius: 3
                                    color: "#E7E7E7"
                                    clip: true

                                    Text {
                                        text: (index + 1) + ": " + modelData.description
                                        color: "black"
                                        font.pixelSize: 30
                                        anchors.verticalCenter: parent.verticalCenter
                                        anchors.left: parent.left
                                        anchors.leftMargin: 6
                                        elide: Text.ElideRight
                                    }
                                }

                                Rectangle {
                                    width: 80
                                    height: 38
                                    radius: 3
                                    color: (modelData.lineState === 1) ? "#FFC700" : "lightgrey"

                                    Text {
                                        text: (modelData.lineState === 1) ? "вкл  " : "выкл"
                                        color: "black"
                                        font.pixelSize: 30
                                        anchors.centerIn: parent
                                    }
                                }

                                Rectangle {
                                    width: 90
                                    height: 38
                                    color: "transparent"

                                    Text {
                                        text: modelData.mode ? "непост." : "пост."
                                        color: "black"
                                        font.pixelSize: 26
                                        anchors.centerIn: parent
                                    }
                                }

                                Text {
                                    text: modelData.status === 0 ? "OK"
                                         : modelData.status === 1 ? "АВАРИЯ"
                                         : modelData.status === 2 ? "ТЕСТ"
                                         : " ? "
                                    color: modelData.status === 0 ? "#5EC85E"
                                          : modelData.status === 1 ? "#FF4C4C"
                                          : modelData.status === 2 ? "#FFC700"
                                          : "blue"
                                    font.pixelSize: 20
                                    Layout.alignment: Qt.AlignVCenter
                                    Layout.fillWidth: true
                                    horizontalAlignment: Text.AlignLeft
                                }
                            }

                            Rectangle {
                                anchors.left: parent.left
                                anchors.right: parent.right
                                anchors.bottom: parent.bottom
                                height: 1
                                color: "#DDDDDD"
                            }
                        }

                        ScrollBar.vertical: ScrollBar {
                            policy: ScrollBar.AlwaysOn
                            width: 22
                        }
                    }
                }
            }
        }
    }

    Rectangle {
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        height: 250

        ColumnLayout {
            anchors.left: parent.left
            anchors.top: parent.top
            anchors.leftMargin: 30
            anchors.topMargin: 20
            spacing: 10

            Row {
                spacing: 30

                Rectangle {
                    id: btnPass
                    width: 300
                    height: 100
                    radius: 20
                    color: "grey"

                    Text {
                        id: txtPass
                        anchors.centerIn: parent
                        text: !unlocked ? "ВХОД" : "ВЫХОД"
                        color: "white"
                        font.bold: true
                        font.pixelSize: 38
                    }

                    MouseArea {
                        anchors.fill: parent
                        hoverEnabled: true
                        onEntered: btnPass.color = "grey"
                        onExited: btnPass.color = "grey"
                        onPressed: btnPass.color = "black"
                        onReleased: btnPass.color = "grey"
                        onClicked: {
                            if (!unlocked) {
                                stackViewList.replace("pagePass.qml")
                            } else {
                                unlocked = false
                            }
                        }
                    }
                }

                Rectangle {
                    width: 300
                    height: 100
                    color: "transparent"
                    Rectangle {
                        id: btnResetForcedFire
                        visible: btnResetForcedFireVsb
                        width: 300
                        height: 100
                        radius: 20
                        color: "#FF4C4C"

                        Text {
                            anchors.centerIn: parent
                            text: "СНЯТЬ ПОЖАР"
                            color: "white"
                            font.bold: true
                            font.pixelSize: 34
                        }

                        MouseArea {
                            anchors.fill: parent
                            hoverEnabled: true
                            onEntered: btnResetForcedFire.color = "#D63B3B"
                            onExited: btnResetForcedFire.color = "#FF4C4C"
                            onPressed: {
                                if (btnResetForcedFireVsb)
                                    btnResetForcedFire.color = "black"
                            }
                            onReleased: btnResetForcedFire.color = "#FF4C4C"
                            onClicked: {
                                if (btnResetForcedFireVsb) {
                                    panel.setForcedFire(false)
                                }
                            }
                        }
                    }

                    Rectangle {
                        id: btnStopTest
                        visible: (btnStopTestVsb )
                        width: 300
                        height: 100
                        radius: 20
                        color: "#FFC700"

                        Text {
                            anchors.centerIn: parent
                            text: "Стоп Тест"
                            color: "white"
                            font.bold: true
                            font.pixelSize: 40
                        }

                        MouseArea {
                            anchors.fill: parent
                            hoverEnabled: true
                            onEntered: btnStopTest.color = "#FFC700"
                            onExited: btnStopTest.color = "#FFC700"
                            onPressed: {
                                if (btnStopTestVsb ) {
                                    btnStopTest.color = "black"
                                }
                            }
                            onReleased: btnStopTest.color = "#FFC700"
                            onClicked: {
                                if (btnStopTestVsb && unlocked)
                                    panel.stopCurrentTest()
                            }
                        }
                    }
                }

                Rectangle {
                    id: btnTestDur
                    visible: unlocked
                    width: 300
                    height: 100
                    radius: 20
                    color: "grey"

                    Text {
                        anchors.centerIn: parent
                        text: "ЖУРНАЛ"
                        color: !unlocked ? "grey" : "white"
                        font.bold: true
                        font.pixelSize: 40
                    }

                    MouseArea {
                        anchors.fill: parent
                        hoverEnabled: true
                        onEntered: btnTestDur.color = "grey"
                        onExited: btnTestDur.color = "grey"
                        onPressed: btnTestDur.color = unlocked ? "black" : "grey"
                        onReleased: btnTestDur.color = "grey"
                        onClicked: {
                            if (unlocked)
                                stackViewList.replace("LogPage.qml")
                        }
                    }
                }
            }

            Row {
                Layout.alignment: Qt.AlignRight
                spacing: 30

                Rectangle {
                    id: btnTest
                    visible: unlocked
                    width: 300
                    height: 100
                    radius: 20
                    color: "grey"

                    Text {
                        anchors.centerIn: parent
                        text: "РУЧНОЙ ТЕСТ"
                        color: !unlocked ? "grey" : "white"
                        font.bold: true
                        font.pixelSize: 36
                    }

                    MouseArea {
                        anchors.fill: parent
                        hoverEnabled: true
                        onEntered: btnTest.color = "grey"
                        onExited: btnTest.color = "grey"
                        onPressed: btnTest.color = unlocked ? "black" : "grey"
                        onReleased: btnTest.color = "grey"
                        onClicked: {
                            if (unlocked)
                                stackViewList.replace("pageTest.qml")
                        }
                    }
                }

                Rectangle {
                    id: btnAjst
                    visible: unlocked
                    width: 300
                    height: 100
                    radius: 20
                    color: "grey"

                    Text {
                        anchors.centerIn: parent
                        text: "НАСТРОЙКА"
                        color: !unlocked ? "grey" : "white"
                        font.bold: true
                        font.pixelSize: 36
                    }

                    MouseArea {
                        anchors.fill: parent
                        hoverEnabled: true
                        onEntered: btnAjst.color = "grey"
                        onExited: btnAjst.color = "grey"
                        onPressed: btnAjst.color = unlocked ? "black" : "grey"
                        onReleased: btnAjst.color = "grey"
                        onClicked: {
                            if (unlocked)
                                stackViewList.replace("pageAjaste.qml")
                        }
                    }
                }

                Rectangle {
                    id: btnRasp
                    visible: unlocked
                    width: 300
                    height: 100
                    radius: 20
                    color: "grey"

                    Text {
                        anchors.centerIn: parent
                        text: "РАСПИСАНИЕ"
                        color: !unlocked ? "grey" : "white"
                        font.bold: true
                        font.pixelSize: 36
                    }

                    MouseArea {
                        anchors.fill: parent
                        hoverEnabled: true
                        onEntered: btnRasp.color = "grey"
                        onExited: btnRasp.color = "grey"
                        onPressed: btnRasp.color = unlocked ? "black" : "grey"
                        onReleased: btnRasp.color = "grey"
                        onClicked: {
                            if (unlocked)
                                stackViewList.replace("SchedulePage.qml")
                        }
                    }
                }
            }
        }
    }
}
