// This file is part of DynExp.

#include "QDynExpLineGraph.h"

namespace DynExpQuick
{
	DynExpLineGraphPlotSeries::DynExpLineGraphPlotSeries(QString Name)
		: Name(std::move(Name)),
		BarSeries(std::make_unique<QBarSeries>()), LineSeries(std::make_unique<QLineSeries>()),
		BarSet(new QBarSet(this->Name))
	{
		BarSet->append(.0);
		BarSeries->append(BarSet);
	}

	size_t DynExpLineGraphPlotModel::InsertSeries(QString Name)
	{
		int iIndex = Util::NumToT<int>(PlotSeries.size());

		beginInsertRows({}, iIndex, iIndex);
		PlotSeries.push_back({ Name });
		PlotSeries.back().LineSeries->setColor(DynExpUI::PlotColors::ColorFromIndex(PlotSeries.size() - 1));
		PlotSeries.back().BarSet->setColor(DynExpUI::PlotColors::ColorFromIndex(PlotSeries.size() - 1));
		endInsertRows();

		return PlotSeries.size() - 1;
	}

	void DynExpLineGraphPlotModel::RemoveSeries(size_t Index)
	{
		if (Index < 0 || Index >= PlotSeries.size())
			throw Util::OutOfRangeException("Index exceeds the bounds of PlotSeries.");

		int iIndex = Util::NumToT<int>(Index);

		beginRemoveRows({}, iIndex, iIndex);
		PlotSeries.erase(PlotSeries.begin() + Index);
		endRemoveRows();
	}

	QStringList DynExpLineGraphPlotModel::GetSeriesNames() const
	{
		QStringList SeriesNames;

		for (const auto& Series : PlotSeries)
			SeriesNames.push_back(QString::fromStdString(DynExp::Object::RemoveCategoryAndName(Series.Name.toStdString())));

		return SeriesNames;
	}

	size_t DynExpLineGraphPlotModel::GetIndex(QString Name) const
	{
		size_t Index = -1;

		for (size_t i = 0; i < PlotSeries.size(); ++i)
		{
			if (PlotSeries.at(i).Name == Name)
			{
				Index = i;

				break;
			}
		}

		return Index;
	}

	const DynExpLineGraphPlotSeries& DynExpLineGraphPlotModel::GetSeries(size_t Index) const
	{
		if (Index < 0 || Index >= PlotSeries.size())
			throw Util::OutOfRangeException("Index exceeds the bounds of PlotSeries.");

		return PlotSeries.at(Index);
	}

	DynExpLineGraphPlotSeries& DynExpLineGraphPlotModel::GetSeries(size_t Index)
	{
		return const_cast<DynExpLineGraphPlotSeries&>(std::as_const(*this).GetSeries(Index));
	}

	bool DynExpLineGraphPlotModel::IsAnyLineSeriesVisible() const
	{
		return std::any_of(PlotSeries.cbegin(), PlotSeries.cend(), [](auto& Series) { return Series.LineSeries->isVisible(); });
	}

	void DynExpLineGraphPlotModel::SeriesChanged(size_t Index)
	{
		if (Index < 0 || Index >= PlotSeries.size())
			throw Util::OutOfRangeException("Index exceeds the bounds of PlotSeries.");

		int iIndex = Util::NumToT<int>(Index);

		emit dataChanged(QAbstractItemModel::createIndex(iIndex, 0), QAbstractItemModel::createIndex(iIndex, 0));
	}

	QHash<int, QByteArray> DynExpLineGraphPlotModel::roleNames() const
	{
		return {
			{ NameRole, "name" },
			{ VisibleRole, "visible" },
			{ NumSamplesRole, "numsamples" },
			{ ColorRole, "color" }
		};
	}

	Qt::ItemFlags DynExpLineGraphPlotModel::flags(const QModelIndex&) const
	{
		return Qt::ItemIsEnabled | Qt::ItemIsUserCheckable | Qt::ItemIsEditable;
	}

	int DynExpLineGraphPlotModel::rowCount(const QModelIndex&) const
	{
		return Util::NumToT<int>(PlotSeries.size());
	}

	QVariant DynExpLineGraphPlotModel::data(const QModelIndex& index, int role) const
	{
		if (!index.isValid())
			return {};

		const auto& Series = PlotSeries.at(index.row());

		switch (role)
		{
		case NameRole: return Series.Name;
		case VisibleRole: return Series.Visible;
		case NumSamplesRole: return Series.LineSeries->points().size();
		case ColorRole: return Series.LineSeries->color();
		}

		return {};
	}

	bool DynExpLineGraphPlotModel::setData(const QModelIndex& index, const QVariant& value, int role)
	{
		if (!index.isValid())
			return false;

		auto& Series = PlotSeries[index.row()];

		switch (role)
		{
		case VisibleRole: Series.Visible = value.toBool(); break;
		default: return false;
		}

		emit dataChanged(index, index, { role });

		return true;
	}

	DynExpLineGraphBackend::DynExpLineGraphBackend(QObject* parent) : QObject(parent),
		XCategoryAxis(std::make_unique<QBarCategoryAxis>()), XValueAxis(std::make_unique<QValueAxis>()),
		YValueAxis(std::make_unique<QValueAxis>())
	{
		XValueAxis->setSubGridVisible(false);
		YValueAxis->setSubGridVisible(false);
	}

	void DynExpLineGraphBackend::SetCursorPosition(QPointF CursorPosition) noexcept
	{
		if (this->CursorPosition == CursorPosition)
			return;

		this->CursorPosition = CursorPosition;

		emit qcursorPositionChanged(CursorPosition);
	}

	void DynExpLineGraphBackend::SetHoveredPoint(QPointF HoveredPoint) noexcept
	{
		if (this->HoveredPoint == HoveredPoint)
			return;

		this->HoveredPoint = HoveredPoint;

		emit qhoveredPointChanged(HoveredPoint);
	}

	void DynExpLineGraphBackend::SetHoveredSample(QPointF HoveredSample) noexcept
	{
		if (this->HoveredSample == HoveredSample)
			return;

		this->HoveredSample = HoveredSample;

		emit qhoveredSampleChanged(HoveredSample);
	}

	void DynExpLineGraphBackend::InsertSeries(QString Name)
	{
		const auto Index = PlotModel.InsertSeries(Name);
		auto& Series = PlotModel.GetSeries(Index);

		emit qinsertSeries(Series.BarSeries.get(), Series.LineSeries.get());
	}

	void DynExpLineGraphBackend::RemoveSeries(QString Name)
	{
		const auto Index = PlotModel.GetIndex(Name);
		if (Index < 0)
			return;

		auto& Series = PlotModel.GetSeries(Index);

		emit qremoveSeries(Series.BarSeries.get(), Series.LineSeries.get());
		PlotModel.RemoveSeries(Index);
	}

	const DynExpLineGraphPlotSeries& DynExpLineGraphBackend::UpdateSeries(size_t Index, const QList<QPointF>& Samples,
		const DynExpModule::Graph::LineGraphPlotInfo& PlotInfo, bool UpdateSamples)
	{
		auto& Series = GetSeries(Index);

		if (Series.Visible)
		{
			if (UpdateSamples)
			{
				if (!Samples.empty())
					Series.BarSet->replace(0, std::abs(Samples.front().y()));
				Series.LineSeries->replace(Samples);
			}

			Series.LineSeries->setWidth(!PlotInfo.HoveredPoint.isNull() && PlotInfo.HoveredSeries == Index ? 4 : 2);
		}

		Series.BarSeries->setVisible(Series.Visible && Samples.size() == 1);
		Series.LineSeries->setVisible(Series.Visible && Samples.size() > 1);

		SeriesChanged(Index);

		return Series;
	}

	void DynExpLineGraphBackend::UpdateData(DynExpModule::Graph::LineGraphPlotInfo& PlotInfo,
		bool Autoscale, bool UpdateAxes)
	{
		const bool AnyLineSeriesVisible = PlotModel.IsAnyLineSeriesVisible();

		if (UpdateAxes)
		{
			XCategoryAxis->setVisible(!AnyLineSeriesVisible);
			XValueAxis->setVisible(AnyLineSeriesVisible);
			if (AnyLineSeriesVisible)
			{
				XValueAxis->setTitleText(DynExp::UnitCategoryToStr(PlotInfo.XUnit) + QString(" in ") +
					(PlotInfo.XUnit == DynExp::UnitType::Time_s ? PlotInfo.GetMultiplierLabel() + "s" : DynExp::UnitTypeToStr(PlotInfo.XUnit)));
				XValueAxis->setLabelFormat(DynExp::IsIntegerUnit(PlotInfo.XUnit) ? "%.0f" : "%.3f");
				XValueAxis->setRange(PlotInfo.MinValues.x(), PlotInfo.MaxValues.x());
			}
			else
				XCategoryAxis->setCategories(PlotModel.GetSeriesNames());

			YValueAxis->setTitleText(DynExp::UnitCategoryToStr(PlotInfo.YUnit) + QString(" in ") + DynExp::UnitTypeToStr(PlotInfo.YUnit));
			YValueAxis->setLabelFormat(DynExp::IsIntegerUnit(PlotInfo.YUnit) ? "%.0f" : "%.3f");
			if (PlotInfo.YUnit == DynExp::UnitType::LogicLevel)
			{
				if (Autoscale)
					YValueAxis->setRange(0, 1);
				YValueAxis->setTickInterval(1);
			}
			else
			{
				if (Autoscale)
					YValueAxis->setRange(PlotInfo.MinValues.y(), PlotInfo.MaxValues.y());
				YValueAxis->setTickInterval(0);
			}

			emit qdataChanged(AnyLineSeriesVisible);
		}

		if (AnyLineSeriesVisible)
		{
			SetHoveredPoint(PlotInfo.HoveredPoint);
			SetHoveredSample(PlotInfo.HoveredSample);
		}

		PlotInfo.CursorPosition = GetCursorPosition();
	}
}