// This file is part of DynExp.

pragma Singleton

import QtQuick

Item {
    readonly property SystemPalette sPaletteActive: SystemPalette {
        colorGroup: SystemPalette.Active
    }
    readonly property SystemPalette sPaletteInactive: SystemPalette {
        colorGroup: SystemPalette.Inactive
    }
    readonly property SystemPalette sPaletteDisabled: SystemPalette {
        colorGroup: SystemPalette.Disabled
    }

    function applySystemPaletteColorGroup(p, sp) {
        p.window = Qt.binding(function() { return sp.window })
        p.windowText = Qt.binding(function() { return sp.windowText })

        p.base = Qt.binding(function() { return sp.base })
        p.alternateBase = Qt.binding(function() { return sp.alternateBase })

        p.toolTipBase = Qt.binding(function() { return sp.toolTipBase })
        p.toolTipText = Qt.binding(function() { return sp.toolTipText })

        p.text = Qt.binding(function() { return sp.text })
        p.brightText = Qt.binding(function() { return sp.brightText })

        p.button = Qt.binding(function() { return sp.button })
        p.buttonText = Qt.binding(function() { return sp.buttonText })

        p.link = Qt.binding(function() { return sp.link })
        p.linkVisited = Qt.binding(function() { return sp.linkVisited })

        p.highlight = Qt.binding(function() { return sp.highlight })
        p.highlightedText = Qt.binding(function() { return sp.highlightedText })

        p.light = Qt.binding(function() { return sp.light })
        p.midlight = Qt.binding(function() { return sp.midlight })
        p.mid = Qt.binding(function() { return sp.mid })
        p.dark = Qt.binding(function() { return sp.dark })

        p.accent = Qt.binding(function() { return sp.accent })
        p.shadow = Qt.binding(function() { return sp.shadow })
    }

    function applySystemPalette(p) {
        applySystemPaletteColorGroup(p.active, sPaletteActive)
        applySystemPaletteColorGroup(p.inactive, sPaletteInactive)
        applySystemPaletteColorGroup(p.disabled, sPaletteDisabled)
    }

    readonly property Gradient defaultGradient: Gradient {
        GradientStop { position: 0.00; color: "#231557" }
        GradientStop { position: 0.29; color: "#44107A" }
        GradientStop { position: 0.67; color: "#FF1361" }
        GradientStop { position: 1.00; color: "#FFF800" }
    }

    readonly property Gradient highlightGradient: Gradient {
        GradientStop { position: 0.00; color: "#231557" }
        GradientStop { position: 0.29; color: "#44107A" }
        GradientStop { position: 0.67; color: "#FF1361" }
        GradientStop { position: 0.97; color: "#FFF800" }
        GradientStop { position: 0.971; color: "#0FFF50" }
        GradientStop { position: 1.00; color: "#0FFF50" }
    }
}