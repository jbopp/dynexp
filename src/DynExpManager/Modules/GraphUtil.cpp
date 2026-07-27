// This file is part of DynExp.

#include "GraphUtil.h"

namespace DynExpModule::Graph
{
	QString LineGraphPlotInfo::GetMultiplierLabel() const
	{
		switch (Multiplier)
		{
		case 0: return "";
		case 3: return "m";
		case 6: return "u";
		case 9: return "n";
		case 12: return "p";
		default: return "?";
		}
	}

	void LineGraphPlotInfo::Reset()
	{
		Multiplier = 0;
		MaxSampleCountPerSeries = 0;
		LastMinValues = MinValues;
		LastMaxValues = MaxValues;
		MinValues = {
			std::numeric_limits<QPointFValueType>::max(),
			std::numeric_limits<QPointFValueType>::max()
		};
		MaxValues = {
			std::numeric_limits<QPointFValueType>::lowest(),
			std::numeric_limits<QPointFValueType>::lowest()
		};

		ResetHoveredSample();
	}

	void LineGraphPlotInfo::GenerateSampleTimingInfo(std::vector<DynExpInstr::DataStreamBase::BasicSampleListType>& BasicSamplesSeries)
	{
		bool TimingInfoFound = false;
		Multiplier = std::numeric_limits<decltype(Multiplier)>::max();

		if (DynExp::Units::IsTimeUnitStrict(XUnit))
		{
			for (auto& Samples : BasicSamplesSeries)
			{
				if (Samples.empty())
					continue;

				// Use stable_sort() to not affect the order of samples with equal time, in case the data
				// stream supports sample timing but the user of the stream ignores it.
				std::stable_sort(Samples.begin(), Samples.end(), [](const auto& a, const auto& b) {
					return a.Time < b.Time;
				});

				TimingInfoFound = true;

				// Switch back to use sample indices as x values if all Time values are equal.
				if (Samples.front().Time == Samples.back().Time && Samples.size() > 1)
				{
					XUnit = DynExp::Units::UnitType::Index;
					Multiplier = 0;

					break;
				}

				if (XUnit == DynExp::Units::UnitType::Time_s)
				{
					// Determine best order of magnitude to display the time with if time is given in seconds.
					if (std::abs(Samples.front().Time) < 1e-9 && std::abs(Samples.back().Time) < 1e-9)
						Multiplier = std::min(Multiplier, 12u);
					else if (std::abs(Samples.front().Time) < 1e-6 && std::abs(Samples.back().Time) < 1e-6)
						Multiplier = std::min(Multiplier, 9u);
					else if (std::abs(Samples.front().Time) < 1e-3 && std::abs(Samples.back().Time) < 1e-3)
						Multiplier = std::min(Multiplier, 6u);
					else if (std::abs(Samples.front().Time) < 1.0 && std::abs(Samples.back().Time) < 1.0)
						Multiplier = std::min(Multiplier, 3u);
					else
						Multiplier = 0;
				}
				else
					Multiplier = 0;
			}
		}
		
		if (!TimingInfoFound)
			Multiplier = 0;
	}

	bool LineGraphPlotInfo::ProcessSamples(QList<QPointF> RawSamples, QList<QPointF>& Samples, const size_t SeriesIndex)
	{
		if (RawSamples.empty())
			return false;

		auto YMin{ std::numeric_limits<QPointFValueType>::max() };
		auto YMax{ std::numeric_limits<QPointFValueType>::lowest() };

		for (qsizetype i = 0; i < RawSamples.size(); ++i)
		{
			const auto X = ApplyXLog(!DynExp::Units::IsIndexUnit(XUnit) ? RawSamples.at(i).x() * std::pow(10.0, Multiplier) : i);
			const auto Y = ApplyYLog(RawSamples.at(i).y());
			Samples.append({ X, Y });

			if (std::isfinite(Y))
			{
				YMin = std::min(YMin, Y);
				YMax = std::max(YMax, Y);
			}

			// To avoid a second loop, do the calculation with axes limits from the previous run.
			// Find hovered point for series with more than a single sample.
			if (i)
				CheckSampleHovered(X, Y, SeriesIndex);
		}

		MaxSampleCountPerSeries = std::max(MaxSampleCountPerSeries, Util::NumToT<size_t>(RawSamples.size()));
		MinValues = { std::min(MinValues.x(), Samples.first().x()), std::min(MinValues.y(), YMin) };
		MaxValues = { std::max(MaxValues.x(), Samples.last().x()), std::max(MaxValues.y(), YMax) };

		return true;
	}

	bool LineGraphPlotInfo::ProcessBasicSamples(DynExpInstr::DataStreamBase::BasicSampleListType BasicSamples,
		QList<QPointF>& Samples, const size_t SeriesIndex)
	{
		if (BasicSamples.empty())
			return false;

		auto YMin{ std::numeric_limits<QPointFValueType>::max() };
		auto YMax{ std::numeric_limits<QPointFValueType>::lowest() };

		for (size_t i = 0; i < BasicSamples.size(); ++i)
		{
			const auto X = ApplyXLog(!DynExp::Units::IsIndexUnit(XUnit) ? BasicSamples[i].Time * std::pow(10.0, Multiplier) : i);
			const auto Y = ApplyYLog(BasicSamples[i].Value);
			Samples.append({ X, Y });

			if (std::isfinite(Y))
			{
				YMin = std::min(YMin, Y);
				YMax = std::max(YMax, Y);
			}

			// To avoid a second loop, do the calculation with axes limits from the previous run.
			// Find hovered point for series with more than a single sample.
			if (i)
				CheckSampleHovered(X, Y, SeriesIndex);
		}

		MaxSampleCountPerSeries = std::max(MaxSampleCountPerSeries, BasicSamples.size());
		MinValues = { std::min(MinValues.x(), Samples.first().x()), std::min(MinValues.y(), YMin) };
		MaxValues = { std::max(MaxValues.x(), Samples.last().x()), std::max(MaxValues.y(), YMax) };

		return true;
	}

	bool LineGraphPlotInfo::ProcessSpectrum(DynExpInstr::SpectrometerData::SpectrumType Spectrum,
		QList<QPointF>& Samples, const size_t SeriesIndex)
	{
		if (!Spectrum.HasSpectrum())
			return false;

		auto YMin{ std::numeric_limits<QPointFValueType>::max() };
		auto YMax{ std::numeric_limits<QPointFValueType>::lowest() };

		size_t i = 0;
		for (const auto& Sample : Spectrum.GetSpectrum())
		{
			const auto X = ApplyXLog(!DynExp::Units::IsIndexUnit(XUnit) ? Sample.first : i);
			const auto Y = ApplyYLog(Sample.second);
			Samples.append({ X, Y });

			if (std::isfinite(Y))
			{
				YMin = std::min(YMin, Y);
				YMax = std::max(YMax, Y);
			}

			// To avoid a second loop, do the calculation with axes limits from the previous run.
			// Find hovered point for series with more than a single sample.
			if (i)
				CheckSampleHovered(X, Y, SeriesIndex);

			++i;
		}

		MaxSampleCountPerSeries = std::max(MaxSampleCountPerSeries, Spectrum.GetSpectrum().size());
		MinValues = { std::min(MinValues.x(), Samples.first().x()), std::min(MinValues.y(), YMin) };
		MaxValues = { std::max(MaxValues.x(), Samples.last().x()), std::max(MaxValues.y(), YMax) };

		return true;
	}

	void LineGraphPlotInfo::AdjustAxesLimits()
	{
		if (!MaxSampleCountPerSeries)
		{
			MinValues = { 0., 0. };
			MaxValues = { 1., 1. };
		}
		else
		{
			if (MaxSampleCountPerSeries == 1)
			{
				MaxValues.setY(std::max(std::abs(MinValues.y()), std::abs(MaxValues.y())));
				MinValues.setY(0.);
			}
			else if (MinValues.y() == MaxValues.y())
			{
				auto YRangeDelta = std::abs(MinValues.y()) * .01;
				YRangeDelta = YRangeDelta == 0. ? 2. : YRangeDelta;
				MinValues.setY(MinValues.y() - YRangeDelta);
				MaxValues.setY(MaxValues.y() + YRangeDelta);
			}
		}
	}

	void LineGraphPlotInfo::ReprocessSamples(const QList<QPointF>& Samples, const size_t SeriesIndex)
	{
		LastMinValues = MinValues;
		LastMaxValues = MaxValues;

		for (size_t i = 0; i < Util::NumToT<size_t>(Samples.size()); ++i)
		{
			// Find hovered point for series with more than a single sample.
			if (i)
				CheckSampleHovered(Samples.at(i).x(), Samples.at(i).y(), SeriesIndex);
		}
	}

	void LineGraphPlotInfo::CheckSampleHovered(const QPointFValueType X, const QPointFValueType Y, const size_t SeriesIndex)
	{
		if (!(LastMinValues.isNull() && LastMaxValues.isNull()) && !CursorPosition.isNull())
		{
			const auto XNormalized = (X - LastMinValues.x()) / (LastMaxValues.x() - LastMinValues.x());
			const auto YNormalized = (Y - LastMinValues.y()) / (LastMaxValues.y() - LastMinValues.y());
			const auto XDist = XNormalized - CursorPosition.x();
			const auto YDist = YNormalized - CursorPosition.y();
			const auto Dist = XDist * XDist + YDist * YDist;

			if (Dist < 0.001 && Dist < HoveredDistance)
			{
				HoveredDistance = Dist;
				HoveredPoint = { XNormalized, YNormalized };
				HoveredSample = { X, Y };
				HoveredSeries = SeriesIndex;
			}
		}
	}

	void LineGraphPlotInfo::ResetHoveredSample()
	{
		HoveredPoint = {};
		HoveredSample = {};
		HoveredDistance = std::numeric_limits<QPointFValueType>::max();
	}
}