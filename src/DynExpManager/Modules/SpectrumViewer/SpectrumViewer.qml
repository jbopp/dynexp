// This file is part of DynExp.

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import Modules.DynExpQuick
import Modules.SpectrumViewer

Rectangle {
    SpectrumViewerBackend {
        id: backend
    }

    id: root
    property alias backend: backend

    implicitWidth: 600
    implicitHeight: 400
    anchors.fill: parent
    color: palette.window

    Component.onCompleted: {
        QQuickDefinitions.applySystemPalette(palette)
    }

    ToolBar {
        id: toolBar
        anchors.left: parent.left
        anchors.right: parent.right
        height: tbSave.height + 4
        z: 100

        Row {
            id: rToolBar
            anchors.fill: parent

            ToolButton {
                id: tbSave
                height: 32
                width: 32
                anchors.verticalCenter: parent.verticalCenter
                icon.source: "qrc:///DynExpManager/icons/Document-save.svg"
                icon.height: 32
                icon.width: 32
                onClicked: backend.saveData()
            }
            ToolSeparator {
                height: 32
                anchors.verticalCenter: parent.verticalCenter
            }
            ToolButton {
                id: tbRun
                height: 32
                width: 32
                anchors.verticalCenter: parent.verticalCenter
                icon.source: "qrc:///DynExpManager/icons/Media-playback-start.svg"
                icon.height: 32
                icon.width: 32
                onClicked: backend.runClicked()
            }
            ToolButton {
                id: tbStop
                height: 32
                width: 32
                anchors.verticalCenter: parent.verticalCenter
                icon.source: "qrc:///QModules/icons/Dialog-STOP.svg"
                icon.height: 32
                icon.width: 32
                onClicked: backend.stopClicked()
            }
            ToolSeparator {
                height: 32
                anchors.verticalCenter: parent.verticalCenter
            }
            ToolButton {
                id: tbSilent
                height: 32
                width: 32
                anchors.verticalCenter: parent.verticalCenter
                checkable: true
                icon.source: "qrc:///QModules/icons/Audio-volume-muted.svg"
                icon.height: 32
                icon.width: 32
                checked: backend.Silent
                onToggled: backend.Silent = checked
            }
        }
    }

    RowLayout {
        id: rlContent
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: toolBar.bottom
        anchors.bottom: rlStatusBar.top
        anchors.margins: 6
        spacing: 10

        ColumnLayout {
            id: clSettings
            Layout.horizontalStretchFactor: 2
            Layout.fillWidth: true
            Layout.alignment: Qt.AlignTop

            Text {
                id: tExposureTime
                text: qsTr("Exposure time")
                color: root.palette.text
            }
            SpinBox {
                id: sbExposureTime
                wheelEnabled: true
                editable: true
                onValueModified: backend.exposureTimeChanged(value)
            }
            Text {
                id: tLowerLimit
                text: qsTr("Lower limit")
                color: root.palette.text
            }
            DoubleSpinBox {
                id: dsbLowerLimit
                wheelEnabled: true
                editable: true
                decimals: 1
                onValueModified: backend.lowerLimitChanged(value)
            }
            Text {
                id: tUpperLimit
                text: qsTr("Upper limit")
                color: root.palette.text
            }
            DoubleSpinBox {
                id: dsbUpperLimit
                wheelEnabled: true
                editable: true
                decimals: 1
                onValueModified: backend.upperLimitChanged(value)
            }
        }
        QDynExpLineGraph {
            id: lgPlot
            Layout.horizontalStretchFactor: 8
            Layout.preferredHeight: rlContent.height
            Layout.fillWidth: true
            Layout.alignment: Qt.AlignTop
            
            Component.onCompleted: {
                backend.SetGraphBackend(lgPlot.backend)
            }
        }
    }

    RowLayout {
        id: rlStatusBar
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        anchors.margins: 6
        spacing: 10

        Text {
            id: tState
            Layout.horizontalStretchFactor: 6
            Layout.fillWidth: true
        }
        ProgressBar {
            id: pbProgress
            Layout.horizontalStretchFactor: 3
            Layout.fillWidth: true
        }
        Text {
            id: tProgress
            text: pbProgress.value * 100 + '%'
            color: root.palette.text
            horizontalAlignment: Text.AlignRight
            Layout.horizontalStretchFactor: 1
            Layout.fillWidth: true
        }
    }

    Shortcut {
        sequence: "R"
        onActivated: backend.runClicked()
    }
    Shortcut {
        sequence: "Esc"
        onActivated: backend.stopClicked()
    }

    Connections {
        target: backend
    }
}