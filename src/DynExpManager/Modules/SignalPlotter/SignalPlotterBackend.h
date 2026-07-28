// This file is part of DynExp.

/**
 * @file SignalPlotterBackend.h
 * @brief C++ backend of the SignalPlotter QML module.
*/

#pragma once

#include "stdafx.h"

#include <qqml.h>
#include <QtGraphs/QBarCategoryAxis>
#include <QtGraphs/QBarSet>
#include <QtGraphs/QBarSeries>
#include <QtGraphs/QLineSeries>
#include <QtGraphs/QValueAxis>

namespace DynExpModule
{
	struct SignalPlotterPlotSeries
	{
		SignalPlotterPlotSeries(QString Name);

		QString Name;
		bool Visible = true;

		std::shared_ptr<QBarSeries> BarSeries;
		std::shared_ptr<QLineSeries> LineSeries;

		QBarSet* BarSet;
	};

	class SignalPlotterPlotModel : public QAbstractListModel
	{
		Q_OBJECT

	public:
		enum Roles { NameRole = Qt::UserRole + 1, VisibleRole, NumSamplesRole, ColorRole };

		SignalPlotterPlotModel(QObject* Parent = nullptr) : QAbstractListModel(Parent) {}
		~SignalPlotterPlotModel() = default;

		size_t InsertSeries(QString Name);
		void RemoveSeries(size_t Index);
		size_t GetNumSeries() const noexcept { return PlotSeries.size(); }

		size_t GetIndex(QString Name) const;
		const SignalPlotterPlotSeries& GetSeries(size_t Index) const;
		SignalPlotterPlotSeries& GetSeries(size_t Index);

		void SeriesChanged(size_t Index);

	private:
		QHash<int, QByteArray> roleNames() const override;
		Qt::ItemFlags flags(const QModelIndex&) const override;
		int rowCount(const QModelIndex&) const override;

		QVariant data(const QModelIndex& index, int role) const override;
		bool setData(const QModelIndex& index, const QVariant& value, int role) override;

		std::vector<SignalPlotterPlotSeries> PlotSeries;
	};

	class SignalPlotterBackend : public QObject
	{
		Q_OBJECT
		QML_ELEMENT

		Q_PROPERTY(SignalPlotterPlotModel* PlotModel READ GetPlotModel CONSTANT)
		Q_PROPERTY(bool Running READ IsRunning WRITE SetRunning NOTIFY qrunningChanged)
		Q_PROPERTY(QPointF CursorPosition READ GetCursorPosition WRITE SetCursorPosition NOTIFY qcursorPositionChanged)
		Q_PROPERTY(QPointF HoveredPoint READ GetHoveredPoint NOTIFY qhoveredPointChanged)
		Q_PROPERTY(QPointF HoveredSample READ GetHoveredSample NOTIFY qhoveredSampleChanged)
		Q_PROPERTY(QBarCategoryAxis* XCategoryAxis READ GetXCategoryAxis CONSTANT)
		Q_PROPERTY(QValueAxis* XValueAxis READ GetXValueAxis CONSTANT)
		Q_PROPERTY(QValueAxis* YValueAxis READ GetYValueAxis CONSTANT)

	public:
		SignalPlotterBackend(QObject* parent = nullptr);
		~SignalPlotterBackend() = default;

		auto GetPlotModel() const noexcept { return &PlotModel; }
		auto GetPlotModel() noexcept { return &PlotModel; }
		bool IsRunning() const noexcept { return Running; }
		void SetRunning(bool Running) noexcept;
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

		void SetRollingView(bool State) { emit qrollingViewChanged(State); }
		void SetAutoscale(bool State) { emit qautoscaleChanged(State); }

		void InsertSeries(QString Name);
		void RemoveSeries(QString Name);
		size_t GetNumSeries() const noexcept { return static_cast<size_t>(PlotModel.GetNumSeries()); }

		const auto& GetSeries(size_t Index) const { return PlotModel.GetSeries(Index); }
		auto& GetSeries(size_t Index) { return PlotModel.GetSeries(Index); }

		void SeriesChanged(size_t Index) { PlotModel.SeriesChanged(Index); }
		void UpdateAxes(bool AnyLineSeriesVisible) { emit qdataChanged(AnyLineSeriesVisible); }

	signals:
		// to QML
		void qinsertSeries(QBarSeries*, QLineSeries*);
		void qremoveSeries(QBarSeries*, QLineSeries*);
		void qdataChanged(bool);
		void qrunningChanged(bool);
		void qcursorPositionChanged(QPointF);
		void qhoveredPointChanged(QPointF);
		void qhoveredSampleChanged(QPointF);
		void qrollingViewChanged(bool);
		void qautoscaleChanged(bool);

		// from QML
		void saveData();
		void rollingViewChanged(bool);
		void autoscaleChanged(bool);
		void clearStream();

	private:
		SignalPlotterPlotModel PlotModel;
		bool Running = true;
		QPointF CursorPosition;
		QPointF HoveredPoint;
		QPointF HoveredSample;

		const std::unique_ptr<QBarCategoryAxis> XCategoryAxis;
		const std::unique_ptr<QValueAxis> XValueAxis;
		const std::unique_ptr<QValueAxis> YValueAxis;
	};
}