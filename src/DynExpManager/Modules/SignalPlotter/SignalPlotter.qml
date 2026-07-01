// This file is part of DynExp.

import QtQuick
import QtQuick.Controls

import Modules.DynExpQuick
import Modules.SignalPlotter

Rectangle {
    SignalPlotterBackend {
        id: backend
    }

    id: root
    property alias backend: backend

    implicitWidth: 600
    implicitHeight: 400
    anchors.fill: parent

    Component.onCompleted: {
        QQuickDefinitions.applySystemPalette(palette)
    }

    ToolBar {
        id: toolBar
        anchors.left: parent.left
        anchors.right: parent.right
        height: tbSave.height + 4
        z: 100

        Switch {
            id: sRun
            text: qsTr("Run")
            anchors.verticalCenter: parent.verticalCenter
            anchors.left: parent.left
            anchors.leftMargin: 6
            checked: backend.Running
            onToggled: backend.Running = checked
        }
        ToolButton {
            id: tbSave
            text: qsTr("Save")
            anchors.verticalCenter: parent.verticalCenter
            anchors.left: sRun.right
            icon.source: "qrc:///DynExpManager/icons/Document-save.svg"
            onClicked: backend.saveData()
        }
        ToolButton {
            id: tbSources
            text: qsTr("Sources...")
            anchors.verticalCenter: parent.verticalCenter
            anchors.left: tbSave.right
            anchors.right: parent.right
            anchors.leftMargin: 20
            anchors.rightMargin: 10
            onClicked: pSources.open()
            icon.source: "qrc:///DynExpManager/icons/Utilities-system-monitor.svg"
        }
    }

    Popup {
        id: pSources
        x: tbSources.x + tbSources.leftPadding
        y: tbSources.y + tbSources.implicitHeight
        margins: 0
        padding: 0
        implicitWidth: tbSources.width
        implicitHeight: lvSources.contentHeight

        contentItem: ListView {
            id: lvSources
            model: lgPlot.backend.PlotModel
            boundsBehavior: Flickable.StopAtBounds

            delegate: CheckDelegate {
                text: model.name + " [" + model.numsamples + " sample" + (model.numsamples == 1 ? "" : "s") + "]"
                checked: model.visible
                width: ListView.view.width
                background: Rectangle {
                    color: "black"
                }
                contentItem: Text {
                    text: parent.text
                    color: model.color
                    anchors.left: parent.left
                    anchors.right: parent.indicator.left
                    verticalAlignment: Text.AlignVCenter
                    leftPadding: parent.spacing
                    rightPadding: parent.spacing
                    elide: Text.ElideRight
                }
                Component.onCompleted: {
                    QQuickDefinitions.applySystemPalette(palette)
                }
                onToggled: {
                    model.visible = checked
                }
            }
        }
    }

    property var miPlotRollingView
    property var miPlotAutoscale

    QDynExpLineGraph {
        id: lgPlot
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: toolBar.bottom
        anchors.bottom: parent.bottom

        Component.onCompleted: {
            let menuItem = lgPlot.cMenuAction.createObject(lgPlot.contextMenu.contentItem, {
                text: qsTr("&Clear stream")
            })
            menuItem.triggered.connect(backend.clearStream)
            lgPlot.contextMenu.addItem(menuItem)

            menuItem = lgPlot.cMenuSeparator.createObject(lgPlot.contextMenu.contentItem, {})
            lgPlot.contextMenu.addItem(menuItem)

            miPlotRollingView = lgPlot.cMenuAction.createObject(lgPlot.contextMenu.contentItem, {
                text: qsTr("&Rolling view"),
                checkable: true,
                checked: false
            })
            miPlotRollingView.triggered.connect(function() { backend.rollingViewChanged(miPlotRollingView.checked) })
            lgPlot.contextMenu.addItem(miPlotRollingView)

            miPlotAutoscale = lgPlot.cMenuAction.createObject(lgPlot.contextMenu.contentItem, {
                text: qsTr("&Autoscale y axis"),
                checkable: true,
                checked: true
            })
            miPlotAutoscale.triggered.connect(function() { backend.autoscaleChanged(miPlotAutoscale.checked) })
            lgPlot.contextMenu.addItem(miPlotAutoscale)

            lgPlot.contextMenu.applyStyle()
            backend.SetGraphBackend(lgPlot.backend)
        }
    }

    Connections {
        target: backend

        function onQrollingViewChanged(state) {
            miPlotRollingView.checked = state
        }

        function onQautoscaleChanged(state) {
            miPlotAutoscale.checked = state
        }
    }
}