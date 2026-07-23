// This file is part of DynExp.

import QtQuick
import QtQuick.Controls
import QtGraphs

Item {
    DynExpSurfaceGraphBackend {
        id: backend
    }

    id: iPlot
    focus: true
    property alias backend: backend
    property vector3d selectedPosition

    Surface3D {
        id: sSurface
        focus: false
        anchors.fill: parent
        cameraPreset: Graphs3D.CameraPreset.DirectlyAbove
        shadowQuality: Graphs3D.ShadowQuality.None
        orthoProjection: true
        selectionMode: Graphs3D.SelectionFlag.None
        theme: GraphsTheme {
            theme: GraphsTheme.Theme.QtGreenNeon
            colorScheme: GraphsTheme.ColorScheme.Dark
            labelBorderVisible: false
        }
        customItemList: [
            Custom3DLabel {
                id: clSurfaceLabel
                visible: selectedPosition.length() > 0
                position: selectedPosition.plus(Qt.vector3d(0, 1, 0))
                text: "(" + selectedPosition.x.toFixed(3) + ", " + selectedPosition.y.toFixed(3) + ", " + selectedPosition.z.toFixed(3) + ")"
                facingCamera: true
                positionAbsolute: false
            }
        ]

        Surface3DSeries {
            id: ssSurfaceSeries
            drawMode: Surface3DSeries.DrawSurface
            shading: Surface3DSeries.Shading.Flat
            colorStyle: GraphsTheme.ColorStyle.RangeGradient
            baseGradient: QQuickDefinitions.highlightGradient
            Component.onCompleted: dataProxy = backend.GetProxy()
        }
    }

    Shortcut {
        sequence: "Left"
        onActivated: {
            backend.SelectedPoint.x = Math.max(0, backend.SelectedPoint.x)
            backend.SelectedPoint.y = Math.max(0, backend.SelectedPoint.y - 1)
            selectedPosition = backend.GetDataItemPosition()
        }
    }
    Shortcut {
        sequence: "Right"
        onActivated: {
            backend.SelectedPoint.x = Math.max(0, backend.SelectedPoint.x)
            backend.SelectedPoint.y = Math.min(ssSurfaceSeries.dataProxy.columnCount - 1, backend.SelectedPoint.y + 1)
            selectedPosition = backend.GetDataItemPosition()
        }
    }
    Shortcut {
        sequence: "Down"
        onActivated: {
            backend.SelectedPoint.x = Math.max(0, backend.SelectedPoint.x - 1)
            backend.SelectedPoint.y = Math.max(0, backend.SelectedPoint.y)
            selectedPosition = backend.GetDataItemPosition()
        }
    }
    Shortcut {
        sequence: "Up"
        onActivated: {
            backend.SelectedPoint.x = Math.min(ssSurfaceSeries.dataProxy.rowCount - 1, backend.SelectedPoint.x + 1)
            backend.SelectedPoint.y = Math.max(0, backend.SelectedPoint.y)
            selectedPosition = backend.GetDataItemPosition()
        }
    }

    Connections {
        target: backend

        function onQdataChanged() {
            if (sSurface.axisX != backend.XValueAxis)
                sSurface.axisX = backend.XValueAxis
            if (sSurface.axisY != backend.YValueAxis)
                sSurface.axisY = backend.YValueAxis
            if (sSurface.axisZ != backend.ZValueAxis)
                sSurface.axisZ = backend.ZValueAxis

            ssSurfaceSeries.dataProxyChanged(ssSurfaceSeries.dataProxy)
        }

        function onQresetCamera() {
            sSurface.cameraPreset = Graphs3D.CameraPreset.DirectlyAbove
        }
    }
}