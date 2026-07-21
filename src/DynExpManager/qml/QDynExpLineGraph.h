// This file is part of DynExp.

/**
 * @file QDynExpLineGraph.h
 * @brief C++ backend of the QDynExpLineGraph QML component.
*/

#pragma once

#include <qqml.h>
#include <QtGraphs/QBarCategoryAxis>
#include <QtGraphs/QBarSet>
#include <QtGraphs/QBarSeries>
#include <QtGraphs/QLineSeries>
#include <QtGraphs/QValueAxis>

#include "../Modules/GraphUtil.h"

namespace DynExpQuick
{
	struct DynExpLineGraphPlotSeries
	{
		DynExpLineGraphPlotSeries(QString Name);

		QString Name;
		bool Visible = true;

		std::shared_ptr<QBarSeries> BarSeries;
		std::shared_ptr<QLineSeries> LineSeries;

		QBarSet* BarSet;
	};

	class DynExpLineGraphPlotModel : public QAbstractListModel
	{
		Q_OBJECT

	public:
		enum Roles { NameRole = Qt::UserRole + 1, VisibleRole, NumSamplesRole, ColorRole };

		DynExpLineGraphPlotModel(QObject* Parent = nullptr) : QAbstractListModel(Parent) {}
		~DynExpLineGraphPlotModel() = default;

		size_t InsertSeries(QString Name, QColor Color);
		void RemoveSeries(size_t Index);
		size_t GetNumSeries() const noexcept { return PlotSeries.size(); }
		QStringList GetSeriesNames() const;

		size_t GetIndex(QString Name) const;
		const DynExpLineGraphPlotSeries& GetSeries(size_t Index) const;
		DynExpLineGraphPlotSeries& GetSeries(size_t Index);

		bool IsAnyLineSeriesVisible() const;

		void SeriesChanged(size_t Index);

	private:
		QHash<int, QByteArray> roleNames() const override;
		Qt::ItemFlags flags(const QModelIndex&) const override;
		int rowCount(const QModelIndex&) const override;

		QVariant data(const QModelIndex& index, int role) const override;
		bool setData(const QModelIndex& index, const QVariant& value, int role) override;

		std::vector<DynExpLineGraphPlotSeries> PlotSeries;
	};

	class DynExpLineGraphBackend : public QObject
	{
		Q_OBJECT
		QML_ELEMENT

		Q_PROPERTY(DynExpLineGraphPlotModel* PlotModel READ GetPlotModel CONSTANT)
		Q_PROPERTY(QPointF CursorPosition READ GetCursorPosition WRITE SetCursorPosition NOTIFY qcursorPositionChanged)
		Q_PROPERTY(QPointF HoveredPoint READ GetHoveredPoint NOTIFY qhoveredPointChanged)
		Q_PROPERTY(QPointF HoveredSample READ GetHoveredSample NOTIFY qhoveredSampleChanged)
		Q_PROPERTY(QBarCategoryAxis* XCategoryAxis READ GetXCategoryAxis CONSTANT)
		Q_PROPERTY(QValueAxis* XValueAxis READ GetXValueAxis CONSTANT)
		Q_PROPERTY(QValueAxis* YValueAxis READ GetYValueAxis CONSTANT)

	public:
		DynExpLineGraphBackend(QObject* parent = nullptr);
		~DynExpLineGraphBackend() = default;
		
		auto GetPlotModel() const noexcept { return &PlotModel; }
		auto GetPlotModel() noexcept { return &PlotModel; }
		QPointF GetCursorPosition() const noexcept { return CursorPosition; }
		void SetCursorPosition(QPointF CursorPosition) noexcept;
		QPointF GetHoveredPoint() const noexcept { return HoveredPoint; }
		void SetHoveredPoint(QPointF HoveredPoint) noexcept;
		QPointF GetHoveredSample() const noexcept { return HoveredSample; }
		void SetHoveredSample(QPointF HoveredSample) noexcept;

		auto GetXCategoryAxis() const noexcept { return XCategoryAxis.get(); }
		auto GetXCategoryAxis() noexcept { return XCategoryAxis.get(); }
		auto GetXValueAxis() const noexcept { return XValueAxis.get(); }
		auto GetXValueAxis() noexcept { return XValueAxis.get(); }
		auto GetYValueAxis() const noexcept { return YValueAxis.get(); }
		auto GetYValueAxis() noexcept { return YValueAxis.get(); }

		void InsertSeries(QString Name, QColor Color = {});
		void RemoveSeries(QString Name);
		const DynExpLineGraphPlotSeries& UpdateSeries(size_t Index, const QList<QPointF>& Samples,
			const DynExpModule::Graph::LineGraphPlotInfo& PlotInfo, bool UpdateSamples = true);
		size_t GetNumSeries() const noexcept { return static_cast<size_t>(PlotModel.GetNumSeries()); }
		const auto& GetSeries(size_t Index) const { return PlotModel.GetSeries(Index); }
		auto& GetSeries(size_t Index) { return PlotModel.GetSeries(Index); }

		void SeriesChanged(size_t Index) { PlotModel.SeriesChanged(Index); }

		void UpdateData(DynExpModule::Graph::LineGraphPlotInfo& PlotInfo,
			bool Autoscale = true, bool UpdateAxes = true);

	signals:
		// to QML
		void qinsertSeries(QBarSeries*, QLineSeries*);
		void qremoveSeries(QBarSeries*, QLineSeries*);
		void qdataChanged(bool);
		void qcursorPositionChanged(QPointF);
		void qhoveredPointChanged(QPointF);
		void qhoveredSampleChanged(QPointF);

	private:
		DynExpLineGraphPlotModel PlotModel;
		QPointF CursorPosition;
		QPointF HoveredPoint;
		QPointF HoveredSample;

		const std::unique_ptr<QBarCategoryAxis> XCategoryAxis;
		const std::unique_ptr<QValueAxis> XValueAxis;
		const std::unique_ptr<QValueAxis> YValueAxis;
	};
}