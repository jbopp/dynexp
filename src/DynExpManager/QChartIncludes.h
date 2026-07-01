// This file is part of DynExp.

/**
 * @file QChartIncludes.h
 * @brief Includes related to the deprecated QtCharts module.
 * To be removed after full porting to QtGraphs.
*/

#pragma once

#include <QChartView>

#include <QtCharts/QBarSeries>
#include <QtCharts/QBarSet>
#include <QtCharts/QLineSeries>
#include <QtCharts/QLogValueAxis>
#include <QtCharts/QScatterSeries>
#include <QtCharts/QValueAxis>
#include <QtCharts/QXYSeries>

#include <QtDataVisualization/Q3DSurface>

namespace DynExpUI
{
	constexpr auto DefaultQChartTheme = QChart::ChartThemeDark;
	constexpr auto DefaultQ3DTheme = Q3DTheme::ThemeStoneMoss;
}