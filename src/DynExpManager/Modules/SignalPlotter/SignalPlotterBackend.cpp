// This file is part of DynExp.

#include "stdafx.h"
#include "SignalPlotterBackend.h"

namespace DynExpModule
{
	SignalPlotterPlotSeries::SignalPlotterPlotSeries(QString Name)
		: Name(std::move(Name)),
		BarSeries(std::make_unique<QBarSeries>()), LineSeries(std::make_unique<QLineSeries>()),
		BarSet(new QBarSet(this->Name))
	{
		BarSet->append(.0);
		BarSeries->append(BarSet);
	}

	size_t SignalPlotterPlotModel::InsertSeries(QString Name)
	{
		int iIndex = Util::NumToT<int>(PlotSeries.size());

		beginInsertRows({}, iIndex, iIndex);
		PlotSeries.push_back({ Name });
		PlotSeries.back().LineSeries->setColor(DynExpUI::PlotColors::ColorFromIndex(PlotSeries.size() - 1));
		PlotSeries.back().BarSet->setColor(DynExpUI::PlotColors::ColorFromIndex(PlotSeries.size() - 1));
		endInsertRows();

		return PlotSeries.size() - 1;
	}

	void SignalPlotterPlotModel::RemoveSeries(size_t Index)
	{
		if (Index < 0 || Index >= PlotSeries.size())
			throw Util::OutOfRangeException("Index exceeds the bounds of PlotSeries.");

		int iIndex = Util::NumToT<int>(Index);

		beginRemoveRows({}, iIndex, iIndex);
		PlotSeries.erase(PlotSeries.begin() + Index);
		endRemoveRows();
	}

	size_t SignalPlotterPlotModel::GetIndex(QString Name) const
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

	const SignalPlotterPlotSeries& SignalPlotterPlotModel::GetSeries(size_t Index) const
	{
		if (Index < 0 || Index >= PlotSeries.size())
			throw Util::OutOfRangeException("Index exceeds the bounds of PlotSeries.");

		return PlotSeries.at(Index);
	}

	SignalPlotterPlotSeries& SignalPlotterPlotModel::GetSeries(size_t Index)
	{
		return const_cast<SignalPlotterPlotSeries&>(std::as_const(*this).GetSeries(Index));
	}

	void SignalPlotterPlotModel::SeriesChanged(size_t Index)
	{
		if (Index < 0 || Index >= PlotSeries.size())
			throw Util::OutOfRangeException("Index exceeds the bounds of PlotSeries.");

		int iIndex = Util::NumToT<int>(Index);

		emit dataChanged(QAbstractItemModel::createIndex(iIndex, 0), QAbstractItemModel::createIndex(iIndex, 0));
	}

	QHash<int, QByteArray> SignalPlotterPlotModel::roleNames() const
	{
		return {
			{ NameRole, "name" },
			{ VisibleRole, "visible" },
			{ NumSamplesRole, "numsamples" },
			{ ColorRole, "color" }
		};
	}

	Qt::ItemFlags SignalPlotterPlotModel::flags(const QModelIndex&) const
	{
		return Qt::ItemIsEnabled | Qt::ItemIsUserCheckable | Qt::ItemIsEditable;
	}

	int SignalPlotterPlotModel::rowCount(const QModelIndex&) const
	{
		return Util::NumToT<int>(PlotSeries.size());
	}

	QVariant SignalPlotterPlotModel::data(const QModelIndex& index, int role) const
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

	bool SignalPlotterPlotModel::setData(const QModelIndex& index, const QVariant& value, int role)
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

	SignalPlotterBackend::SignalPlotterBackend(QObject* parent) : QObject(parent),
		XCategoryAxis(std::make_unique<QBarCategoryAxis>()), XValueAxis(std::make_unique<QValueAxis>()),
		YValueAxis(std::make_unique<QValueAxis>())
	{
		XValueAxis->setSubGridVisible(false);
		YValueAxis->setSubGridVisible(false);
	}

	void SignalPlotterBackend::SetRunning(bool Running) noexcept
	{
		if (this->Running == Running)
			return;

		this->Running = Running;
		
		emit qrunningChanged(Running);
	}

	void SignalPlotterBackend::SetCursorPosition(QPointF CursorPosition) noexcept
	{
		if (this->CursorPosition == CursorPosition)
			return;

		this->CursorPosition = CursorPosition;

		emit qcursorPositionChanged(CursorPosition);
	}

	void SignalPlotterBackend::SetHoveredPoint(QPointF HoveredPoint) noexcept
	{
		if (this->HoveredPoint == HoveredPoint)
			return;

		this->HoveredPoint = HoveredPoint;

		emit qhoveredPointChanged(HoveredPoint);
	}

	void SignalPlotterBackend::SetHoveredSample(QPointF HoveredSample) noexcept
	{
		if (this->HoveredSample == HoveredSample)
			return;

		this->HoveredSample = HoveredSample;

		emit qhoveredSampleChanged(HoveredSample);
	}

	void SignalPlotterBackend::InsertSeries(QString Name)
	{
		const auto Index = PlotModel.InsertSeries(Name);
		auto& Series = PlotModel.GetSeries(Index);

		emit qinsertSeries(Series.BarSeries.get(), Series.LineSeries.get());
	}

	void SignalPlotterBackend::RemoveSeries(QString Name)
	{
		const auto Index = PlotModel.GetIndex(Name);
		if (Index < 0)
			return;

		auto& Series = PlotModel.GetSeries(Index);

		emit qremoveSeries(Series.BarSeries.get(), Series.LineSeries.get());
		PlotModel.RemoveSeries(Index);
	}
}