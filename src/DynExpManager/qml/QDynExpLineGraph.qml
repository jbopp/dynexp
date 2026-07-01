// This file is part of DynExp.

import QtQuick
import QtQuick.Controls
import QtQuick.Shapes
import QtGraphs

Item {
    DynExpLineGraphBackend {
        id: backend
    }

    id: iPlot
    property alias backend: backend

    GraphsView {
        id: gvPlot
        anchors.fill: parent

        theme: GraphsTheme {
            theme: GraphsTheme.Theme.QtGreenNeon
            colorScheme: GraphsTheme.ColorScheme.Dark
        }
    }

    Rectangle {
        id: rPlotInfoBox
        anchors {
            right: parent.right
            top: parent.top
            rightMargin: 16
            topMargin: 12
        }
        implicitWidth: Math.max(lPlotInfoBoxXValue.implicitWidth, lPlotInfoBoxYValue.implicitWidth) + 16
        implicitHeight: lPlotInfoBoxXValue.implicitHeight + lPlotInfoBoxYValue.implicitHeight + 12
        visible: false
        radius: 6
        color: "#C0606060"

        Column {
            anchors.centerIn: parent
            spacing: 2

            Label {
                id: lPlotInfoBoxXValue
                color: "white"
            }

            Label {
                id: lPlotInfoBoxYValue
                color: "white"
            }
        }
    }

    Shape {
        id: sPlotHLine
        visible: false

        ShapePath {
            strokeColor: "lightgray"
            strokeWidth: 2
            strokeStyle: ShapePath.DashLine
            dashPattern: [4, 4]
            startX: gvPlot.plotArea.x

            PathLine {
                x: gvPlot.plotArea.x + gvPlot.plotArea.width
            }
        }
    }

    Shape {
        id: sPlotVLine
        visible: false

        ShapePath {
            strokeColor: "lightgray"
            strokeWidth: 2
            strokeStyle: ShapePath.DashLine
            dashPattern: [4, 4]
            startY: gvPlot.plotArea.y

            PathLine {
                y: gvPlot.plotArea.y + gvPlot.plotArea.height
            }
        }
    }

    Rectangle {
        id: rPlotMarker
        visible: false
        width: 16
        height: 16
        color: "white"
        radius: width * 0.5
        border.width: 4
        border.color: "black"
    }

    MouseArea {
        id: maPlot
        anchors.fill: parent
        hoverEnabled: true
        scrollGestureEnabled: false
        cursorShape: Qt.CrossCursor
        acceptedButtons: Qt.RightButton
        onClicked: {
            cmPlot.popup()
        }
        onPositionChanged: function(mouse) {
            if (mouse.x >= gvPlot.plotArea.x && mouse.x <= gvPlot.plotArea.x + gvPlot.plotArea.width &&
                mouse.y >= gvPlot.plotArea.y && mouse.y <= gvPlot.plotArea.y + gvPlot.plotArea.height) {
                backend.CursorPosition = Qt.point((mouse.x - gvPlot.plotArea.x) / gvPlot.plotArea.width,
                    1.0 - (mouse.y - gvPlot.plotArea.y) / gvPlot.plotArea.height)
                if (rPlotMarker.x > 0 && rPlotMarker.y > 0) {
                    sPlotHLine.data[0].startY = rPlotMarker.y + rPlotMarker.height / 2
                    sPlotHLine.data[0].pathElements[0].y = rPlotMarker.y + rPlotMarker.height / 2
                    sPlotVLine.data[0].startX = rPlotMarker.x + rPlotMarker.width / 2
                    sPlotVLine.data[0].pathElements[0].x = rPlotMarker.x + rPlotMarker.width / 2
                } else {
                    sPlotHLine.data[0].startY = mouse.y
                    sPlotHLine.data[0].pathElements[0].y = mouse.y
                    sPlotVLine.data[0].startX = mouse.x
                    sPlotVLine.data[0].pathElements[0].x = mouse.x
                }
                sPlotHLine.visible = true
                sPlotVLine.visible = true
            } else {
                backend.CursorPosition = Qt.point(0, 0)
                sPlotHLine.visible = false
                sPlotVLine.visible = false
            }
        }
    }

    QDynExpMenu {
        id: cmPlot
    }

    property alias contextMenu: cmPlot
    readonly property Component cMenuAction: Component { MenuItem {} }
    readonly property Component cMenuSeparator: Component { MenuSeparator {} }

    Connections {
        target: backend

        function onQinsertSeries(barSeries, lineSeries) {
            gvPlot.addSeries(barSeries)
            gvPlot.addSeries(lineSeries)
        }

        function onQremoveSeries(barSeries, lineSeries) {
            gvPlot.removeSeries(barSeries)
            gvPlot.removeSeries(lineSeries)
        }

        function onQdataChanged(anyLineSeriesVisible) {
            if (!anyLineSeriesVisible && gvPlot.axisX != backend.XCategoryAxis)
                gvPlot.axisX = backend.XCategoryAxis
            if (anyLineSeriesVisible && gvPlot.axisX != backend.XValueAxis)
                gvPlot.axisX = backend.XValueAxis
            if (gvPlot.axisY != backend.YValueAxis)
                gvPlot.axisY = backend.YValueAxis
        }

        function onQhoveredPointChanged(hoveredPoint) {
            if (hoveredPoint.x > 0 && hoveredPoint.y > 0) {
                rPlotMarker.x = hoveredPoint.x * gvPlot.plotArea.width + gvPlot.plotArea.x - rPlotMarker.width / 2
                rPlotMarker.y = gvPlot.plotArea.height - hoveredPoint.y * gvPlot.plotArea.height + gvPlot.plotArea.y - rPlotMarker.height / 2
                rPlotMarker.visible = true
            } else {
                rPlotMarker.x = 0
                rPlotMarker.y = 0
                rPlotMarker.visible = false
            }
        }

        function onQhoveredSampleChanged(hoveredSample) {
            if (hoveredSample.x != 0 && hoveredSample.y != 0) {
                lPlotInfoBoxXValue.text = "x: " + hoveredSample.x.toFixed(3)
                lPlotInfoBoxYValue.text = "y: " + hoveredSample.y.toFixed(3)
                rPlotInfoBox.visible = true
            } else {
                rPlotInfoBox.visible = false
            }
        }
    }
}