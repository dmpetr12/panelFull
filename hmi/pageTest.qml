import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import App 1.0

Rectangle {
    width: stackViewList.width
    height: stackViewList.height

    Rectangle {
        width: parent.width
        height: 550

        ColumnLayout {
            anchors.fill: parent
            spacing: 0

            Text {
                text: "          ТЕСТ ЛИНИЙ"
                font.pixelSize: 40
                color: "black"
                Layout.fillWidth: true
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
                height: 40
            }

            RowLayout {
                Layout.fillWidth: true
                Layout.fillHeight: true
                spacing: 10

                ColumnLayout {
                    Layout.preferredWidth: 300
                    spacing: 20
                    Layout.topMargin: 60

                    Rectangle {
                        width: 250
                        height: 90
                        radius: 12
                        color: "orange"
                        Layout.alignment: Qt.AlignTop | Qt.AlignHCenter

                        Text {
                            anchors.centerIn: parent
                            text: "Тест всех\n линий"
                            font.pixelSize: 30
                            color: "white"
                            horizontalAlignment: Text.AlignHCenter
                        }

                        MouseArea {
                            anchors.fill: parent
                            onClicked: {
                                indexCh = -1
                                stackViewList.replace("pageTestEx.qml")
                            }
                        }
                    }

                    Rectangle {
                        width: 250
                        height: 90
                        radius: 12
                        color: "orange"
                        Layout.alignment: Qt.AlignTop | Qt.AlignHCenter

                        Text {
                            anchors.centerIn: parent
                            text: "Тест на время"
                            font.pixelSize: 30
                            color: "white"
                        }

                        MouseArea {
                            anchors.fill: parent
                            onClicked: {
                                indexCh = -2
                                stackViewList.replace("pageTestEx.qml")
                            }
                        }
                    }

                    Item {
                        Layout.fillHeight: true
                    }
                }

                ColumnLayout {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    spacing: 10

                    Text {
                        text: "тест линии"
                        font.pixelSize: 30
                        color: "white"
                        Layout.fillWidth: true
                        horizontalAlignment: Text.AlignHCenter
                    }

                    ListView {
                        id: listView
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        model: panel.lines
                        spacing: 0
                        reuseItems: true

                        delegate: Item {
                            width: ListView.view.width
                            visible: modelData.mode !== 2 && index < panel.lineCount
                            height: visible ? 80 : 0

                            RowLayout {
                                anchors.fill: parent
                                anchors.margins: 10
                                spacing: 10

                                Rectangle {
                                    width: 500
                                    height: 70
                                    radius: 3
                                    color: "#E7E7E7"

                                    Text {
                                        text: (index + 1) + ": " + modelData.description
                                        font.pixelSize: 30
                                        anchors.fill: parent
                                        color: "black"
                                        verticalAlignment: Text.AlignVCenter
                                        elide: Text.ElideRight
                                    }
                                }

                                Rectangle {
                                    id: idT
                                    width: 180
                                    height: 70
                                    radius: 3
                                    color: "orange"

                                    Text {
                                        text: "  тест"
                                        anchors.fill: parent
                                        verticalAlignment: Text.AlignVCenter
                                        font.pixelSize: 30
                                        color: "black"
                                    }

                                    MouseArea {
                                        anchors.fill: parent
                                        hoverEnabled: true
                                        onClicked: {
                                            indexCh = index
                                            stackViewList.replace("pageTestEx.qml")
                                        }
                                    }
                                }

                                Item {
                                    Layout.fillWidth: true
                                }
                            }
                        }

                        ScrollBar.vertical: ScrollBar {
                            policy: ScrollBar.AlwaysOn
                            width: 30
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
                    onClicked: stackViewList.replace("pageStart.qml")
                }
            }
        }
    }
}
