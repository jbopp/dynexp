// This file is part of DynExp.

import QtQuick
import QtQuick.Controls

Menu {
    id: cmPlot
    implicitWidth: 200

    delegate: MenuItem {
        id: miPlotMenu

        contentItem: Label {
            text: miPlotMenu.text.replace(/&([^&])/g, "<u>$1</u>")
            textFormat: Text.StyledText
            font: miPlotMenu.font
            elide: Text.ElideRight

            leftPadding: miPlotMenu.indicator.width + 6
            rightPadding: miPlotMenu.arrow.width + 6
            horizontalAlignment: Text.AlignLeft
            verticalAlignment: Text.AlignVCenter

            opacity: enabled ? 1.0 : 0.3
            color: miPlotMenu.highlighted ? QQuickDefinitions.sPaletteActive.highlightedText : QQuickDefinitions.sPaletteActive.text
        }

        background: Rectangle {
            opacity: enabled ? 1 : 0.3
            color: miPlotMenu.highlighted ? QQuickDefinitions.sPaletteActive.highlight : QQuickDefinitions.sPaletteActive.window
        }
    }

    background: Rectangle {
        color: QQuickDefinitions.sPaletteActive.window
    }

    function applyStyle() {
        QQuickDefinitions.applySystemPalette(lgPlot.contextMenu.palette)
    }
}