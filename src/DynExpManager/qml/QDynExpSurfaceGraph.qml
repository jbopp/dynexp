// This file is part of DynExp.

import QtQuick
import QtQuick.Controls
import QtGraphs

Item {
    DynExpSurfaceGraphBackend {
        id: backend
    }

    id: iPlot
    property alias backend: backend

    Surface3D {
        id: gvPlot
        anchors.fill: parent
        cameraPreset: Graphs3D.CameraPreset.DirectlyAbove
        shadowQuality: Graphs3D.ShadowQuality.None
        orthoProjection: true
        selectionMode: Graphs3D.SelectionFlag.Item
        theme: GraphsTheme {
            theme: GraphsTheme.Theme.QtGreenNeon
            colorScheme: GraphsTheme.ColorScheme.Dark
            labelBorderVisible: false
        }

        Surface3DSeries {
            id: ssSurfaceSeries
            drawMode: Surface3DSeries.DrawFlag.DrawSurface
            shading: Surface3DSeries.Shading.Flat
            colorStyle: GraphsTheme.ColorStyle.RangeGradient
            selectedPoint: backend.SelectedPoint
            onSelectedPointChanged: backend.SelectedPoint = selectedPoint
        }
    }

    Connections {
        target: backend

        function onQproxyChanged(proxy) {
            if (ssSurfaceSeries.dataProxy != proxy)
                ssSurfaceSeries.dataProxy = proxy
        }

        function onQdataChanged() {
            if (gvPlot.axisX != backend.XValueAxis)
                gvPlot.axisX = backend.XValueAxis
            if (gvPlot.axisY != backend.YValueAxis)
                gvPlot.axisY = backend.YValueAxis
            if (gvPlot.axisZ != backend.ZValueAxis)
                gvPlot.axisZ = backend.ZValueAxis
        }

        function onQresetCamera() {
            gvPlot.cameraPreset = Graphs3D.CameraPreset.DirectlyAbove
        }
    }
}