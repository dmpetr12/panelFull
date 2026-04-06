import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

Page {
    id: logPage
    title: "Лог системы"

    property int pageSize: 200
    property int loadedCount: 0
    property bool allLoaded: false
    property bool loading: false
    property double lastClickMs: 0
    property int minIntervalMs: 2000

    ListModel { id: logModel }
    ListModel { id: summaryModel }

    function loadMore() {
        if (loading || allLoaded)
            return

        loading = true

        var chunk = panel.readLogs(loadedCount, pageSize)

        if (!chunk || chunk.length === 0) {
            allLoaded = true
        } else {
            for (var i = 0; i < chunk.length; ++i) {
                logModel.append({ line: chunk[i] })
            }
            loadedCount += chunk.length
        }

        loading = false
    }

    Component.onCompleted: {
        loadMore()

        var s = panel.testSummary()
        summaryLongTestText.text = "Тест длительности: "
                + "        "
                + (s.longTestResult || "—")
                + "           "
                + (s.longTestDate || "—")
                + "  "

        summaryModel.clear()

        var arr = s.lines || []
        for (var i = 0; i < arr.length; ++i)
            summaryModel.append(arr[i])

        summaryPopup.open()
    }

    Popup {
        id: infoPopup
        x: (parent.width - width) / 2
        y: (parent.height - height) / 2
        modal: false
        focus: false
        padding: 12

        property string infoPopupText: ""

        background: Rectangle {
            radius: 6
            color: "#333333"
            opacity: 0.9
        }

        contentItem: Text {
            text: infoPopup.infoPopupText
            color: "white"
            wrapMode: Text.Wrap
        }

        Timer {
            id: popupTimer
            interval: 8000
            running: false
            repeat: false
            onTriggered: infoPopup.close()
        }

        function show(message) {
            infoPopupText = message
            open()
            popupTimer.restart()
        }
    }

    Popup {
        id: summaryPopup
        modal: true
        focus: true
        closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside
        width: Math.min(parent.width * 0.92, 900)
        height: Math.min(parent.height * 0.88, 700)
        x: (parent.width - width) / 2
        y: (parent.height - height) / 2

        background: Rectangle {
            radius: 14
            color: "white"
        }

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 16
            spacing: 12

            Text {
                text: "Сводка по тестам"
                font.pixelSize: 34
                font.bold: true
                Layout.alignment: Qt.AlignHCenter
            }

            Text {
                id: summaryLongTestText
                text: "Тест длительности: —"
                font.pixelSize: 22
            }

            Rectangle {
                height: 1
                color: "#ddd"
                Layout.fillWidth: true
            }

            ListView {
                Layout.fillWidth: true
                Layout.fillHeight: true
                model: summaryModel
                clip: true

                delegate: RowLayout {
                    width: ListView.view.width
                    height: 44
                    spacing: 10

                    Text {
                        text: index + ")"
                        font.pixelSize: 20
                        width: 40
                    }

                    Text {
                        text: description
                        font.pixelSize: 20
                        Layout.fillWidth: true
                        elide: Text.ElideRight
                    }

                    Text {
                        text: "   " + status + "   "
                        font.pixelSize: 20
                        width: 100
                    }

                    Text {
                        text: lastTestDate
                        font.pixelSize: 20
                        width: 220
                        horizontalAlignment: Text.AlignRight
                    }
                }

                ScrollBar.vertical: ScrollBar {}
            }

            Button {
                text: "OK"
                font.pixelSize: 22
                Layout.alignment: Qt.AlignRight
                onClicked: summaryPopup.close()
            }
        }
    }

    header: ToolBar {
        implicitHeight: 80

        RowLayout {
            anchors.fill: parent
            anchors.margins: 8
            spacing: 8

            Rectangle {
                width: 60
                height: 60
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

            Label {
                text: "Журнал системы"
                font.pixelSize: 40
                Layout.fillWidth: true
                horizontalAlignment: Text.AlignHCenter
            }

            Button {
                id: statusLabel
                text: "Сбросить логи на флешку"

                onClicked: {
                    const now = Date.now()
                    if (now - lastClickMs < minIntervalMs)
                        return

                    lastClickMs = now
                    const res = panel.exportLogsToUsb()
                    //console.log(res)
                    infoPopup.show(res)
                }
            }

            ToolButton {
                text: "  ↑  "
                font.pixelSize: 50
                onClicked: list.positionViewAtBeginning()
            }
        }
    }

    ListView {
        id: list
        anchors.fill: parent
        clip: true
        model: logModel

        delegate: Rectangle {
            width: ListView.view.width
            height: textItem.implicitHeight + 8
            color: index % 2 === 0 ? "#f7f7f7" : "#ffffff"

            Text {
                id: textItem
                anchors.fill: parent
                anchors.margins: 4
                text: line
                font.family: "Consolas"
                font.pixelSize: 18
                color: "#202020"
                wrapMode: Text.NoWrap
                horizontalAlignment: Text.AlignLeft
                verticalAlignment: Text.AlignVCenter
                elide: Text.ElideRight
            }
        }

        ScrollBar.vertical: ScrollBar {
            policy: ScrollBar.AlwaysOn
            width: 50
        }

        onAtYEndChanged: {
            if (atYEnd)
                logPage.loadMore()
        }
    }
}
