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
                enabled: backend.State != SpectrumViewerBackend.Capturing
                opacity: enabled ? 1.0 : 0.4
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
                enabled: backend.State != SpectrumViewerBackend.Capturing
                opacity: enabled ? 1.0 : 0.4
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
                enabled: backend.State == SpectrumViewerBackend.Capturing
                opacity: enabled ? 1.0 : 0.4
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
                opacity: enabled ? 1.0 : 0.4
                height: 32
                width: 32
                anchors.verticalCenter: parent.verticalCenter
                checkable: true
                icon.source: "qrc:///QModules/icons/Audio-volume-muted.svg"
                icon.height: 32
                icon.width: 32
                checked: backend.Silent
                onToggled: backend.Silent = checked
                onActiveFocusChanged: backend.SilentFocused = activeFocus
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
                enabled: backend.State != SpectrumViewerBackend.Capturing
                wheelEnabled: true
                editable: true
                from: backend.ExposureTimeRange.x
                to: backend.ExposureTimeRange.y
                value: backend.ExposureTime
                onValueModified: {
                    forceActiveFocus()
                    backend.ExposureTime = value
                }
                onActiveFocusChanged: backend.ExposureTimeFocused = activeFocus
                textFromValue: function(value, locale) { return Number(value).toLocaleString(locale, 'f', 0) + " " + backend.ExposureTimeUnit; }
                valueFromText: function(text, locale) { return Number.fromLocaleString(locale, text.replace(backend.ExposureTimeUnit, "").trim()); }
            }
            Text {
                id: tLowerLimit
                text: qsTr("Lower limit")
                color: root.palette.text
            }
            DoubleSpinBox {
                id: dsbLowerLimit
                enabled: backend.State != SpectrumViewerBackend.Capturing
                wheelEnabled: true
                editable: true
                decimals: 1
                from: backend.LimitRange.x
                to: backend.LimitRange.y
                value: backend.LowerLimit
                onValueModified: {
                    forceActiveFocus()
                    backend.LowerLimit = value
                }
                onActiveFocusChanged: backend.LowerLimitFocused = activeFocus
                textFromValue: function(value, locale) { return Number(value).toLocaleString(locale, 'f', 1) + " " + backend.LimitUnit; }
                valueFromText: function(text, locale) { return Number.fromLocaleString(locale, text.replace(backend.LimitUnit, "").trim()); }
            }
            Text {
                id: tUpperLimit
                text: qsTr("Upper limit")
                color: root.palette.text
            }
            DoubleSpinBox {
                id: dsbUpperLimit
                enabled: backend.State != SpectrumViewerBackend.Capturing
                wheelEnabled: true
                editable: true
                decimals: 1
                from: backend.LimitRange.x
                to: backend.LimitRange.y
                value: backend.UpperLimit
                onValueModified: {
                    forceActiveFocus()
                    backend.UpperLimit = value
                }
                onActiveFocusChanged: backend.UpperLimitFocused = activeFocus
                textFromValue: function(value, locale) { return Number(value).toLocaleString(locale, 'f', 1) + " " + backend.LimitUnit; }
                valueFromText: function(text, locale) { return Number.fromLocaleString(locale, text.replace(backend.LimitUnit, "").trim()); }
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
        height: 24
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        anchors.margins: 6
        spacing: 10

        Rectangle {
            id: rState
            Layout.horizontalStretchFactor: 6
            Layout.fillWidth: true
            Layout.fillHeight: true
            radius: 3
            color: {
                switch (backend.State) {
                case SpectrumViewerBackend.Capturing: return "dodgerblue"
                case SpectrumViewerBackend.Warning: return "orange"
                case SpectrumViewerBackend.Error: return "red"
                default: return "transparent"
                }
            }

            Text {
                id: tState
                anchors.fill: parent
                anchors.leftMargin: 6
                anchors.rightMargin: 6
                verticalAlignment: Text.AlignVCenter
                horizontalAlignment: Text.AlignLeft
                font.bold: backend.State != SpectrumViewerBackend.Ready
                color: {
                    switch (backend.State) {
                    case SpectrumViewerBackend.Capturing: return "white"
                    case SpectrumViewerBackend.Warning: return "black"
                    case SpectrumViewerBackend.Error: return "white"
                    default: return palette.text
                    }
                }
                text: {
                    switch (backend.State) {
                    case SpectrumViewerBackend.Capturing:
                        return "Acquiring spectrum..."
                    case SpectrumViewerBackend.Warning:
                        return "The spectrometer is in a warning state."
                    case SpectrumViewerBackend.Error:
                        return "The spectrometer is in an error state."
                    default:
                        return "Ready"
                    }
                }
            }
        }
        ProgressBar {
            id: pbProgress
            value: backend.Progress
            visible: backend.State == SpectrumViewerBackend.Capturing && value > 0
            Layout.horizontalStretchFactor: 3
            Layout.fillWidth: true
        }
        Text {
            id: tProgress
            text: pbProgress.value * 100 + '%'
            visible: pbProgress.visible
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