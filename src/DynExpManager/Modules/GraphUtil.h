// This file is part of DynExp.

/**
 * @file GraphUtil.h
 * @brief Defines helper classes for QML graphs.
*/

#pragma once

#include "stdafx.h"
#include "../MetaInstruments/DataStreamInstrument.h"

namespace DynExpModule::Graph
{
	struct LineGraphPlotInfo
	{
		/**
		 * @brief Data type of sample values for plotting.
		*/
		using QPointFValueType = decltype(std::declval<QPointF>().x());

		/**
		 * @brief Returns the multiplier prefix associated with @p Multiplier.
		 * @return Prefix of the axis multiplier, like 'n' for nano.
		*/
		QString GetMultiplierLabel() const;

		/**
		 * @brief Resets the instance to generate information from new sample series.
		*/
		void Reset();

		/**
		 * @brief Extracts sample timing information for x axis scaling from @p BasicSamples and stores the
		 * results in this @p DynExpLineGraphPlotInfo instance.
		 * @param BasicSamplesSeries Vector of vectors of BasicSamples to investigate. The outer vector represents a
		 * list of data series. The funcion may sort the entries of @p BasicSamples according to their x values in
		 * ascending order.
		*/
		void GenerateSampleTimingInfo(std::vector<DynExpInstr::DataStreamBase::BasicSampleListType>& BasicSamplesSeries);

		/**
		 * @brief Converts @p BasicSamples to displayable format and stores their minimal and maximal values in
		 * this @p DynExpLineGraphPlotInfo instance.
		 * @param BasicSamples Vector of BasicSamples to convert. Move to this parameter to avoid copying.
		 * @param Samples Destiny to store the processed samples in.
		 * @param SeriesIndex Index of the data series that is to be processed.
		 * @return Returns true if at least one sample has been processed, false otherwise.
		*/
		bool ProcessBasicSamples(DynExpInstr::DataStreamBase::BasicSampleListType BasicSamples,
			QList<QPointF>& Samples, const size_t SeriesIndex);

		/**
		 * @brief Adjusts @p MinValues and @p MaxValues to best display all data series.
		*/
		void AdjustAxesLimits();

		/**
		 * @brief If data plotting is not enabled (running), the available and already plotted samples have to be
		 * reprocessed, e.g. for data sample hovering detection.
		 * @param Samples Processed samples to reprocess.
		 * @param SeriesIndex Index of the data series that is to be processed.
		*/
		void ReprocessSamples(const QList<QPointF>& Samples, const size_t SeriesIndex);

		/**
		 * @brief Checks whether the given sample is close to the mouse cursor (hovered).
		 * @param X x value of the sample.
		 * @param Y y value of the sample.
		 * @param SeriesIndex Index of the data series the sample belongs to.
		*/
		void CheckSampleHovered(const QPointFValueType X, const QPointFValueType Y, const size_t SeriesIndex);

		/**
		 * @brief Removes the stored hovered sample.
		*/
		void ResetHoveredSample();

		/**
		 * @brief Joint unit of the plot's value axis.
		*/
		DynExpInstr::DataStreamInstr::UnitType ValueUnit = DynExpInstr::DataStreamInstr::UnitType::Arbitrary;

		/**
		 * @brief Use sample indices or time data for plot's joint x axis?
		*/
		bool IsBasicSampleTimeUsed = true;

		/**
		 * @brief Best order of magnitude to scale the plot's joint time axis with.
		*/
		unsigned int Multiplier = 0;

		/**
		 * @brief Indicates the number of samples contained in the longest data series to plot.
		 * A value of 0 indicates that no samples are available for plotting.
		*/
		size_t MaxSampleCountPerSeries = 0;

		/**
		 * @brief Current lower axes limits of x and y axes.
		*/
		QPointF MinValues;

		/**
		 * @brief Current upper axes limits of x and y axes.
		*/
		QPointF MaxValues;

		/**
		 * @brief Lower axes limits of x and y axes from previous graph update.
		*/
		QPointF LastMinValues;

		/**
		 * @brief Upper axes limits of x and y axes from previous graph update.
		*/
		QPointF LastMaxValues;

		/**
		 * @brief Relative position of the mouse cursor inside the coordinate system (between 0. and 1.).
		*/
		QPointF CursorPosition;

		/**
		 * @brief Hovered point belonging to any series in normalized coordinate system coordinates (between 0. and 1.).
		*/
		QPointF HoveredPoint;

		/**
		 * @brief Hovered sample belonging to any series in the graphs's real coordinate system.
		*/
		QPointF HoveredSample;

		/**
		 * @brief Index of the data series the hovered point belongs to.
		*/
		size_t HoveredSeries = 0;

		/**
		 * @brief Distance between the mouse cursor and the hovered point.
		*/
		QPointFValueType HoveredDistance = std::numeric_limits<QPointFValueType>::max();
	};
}