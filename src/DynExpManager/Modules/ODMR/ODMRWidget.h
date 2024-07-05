// This file is part of DynExp.

/**
 * @file ODMRWidget.h
 * @brief User interface belonging to the DynExpModule::ODMR::ODMR module.
*/

#pragma once

#include "stdafx.h"
#include "DynExpCore.h"

#include <QWidget>
#include "ui_ODMR.h"

namespace DynExpModule::ODMR
{
	class ODMR;
	class ODMRData;

	enum class StateType {
		Initializing,
		Ready,
		MeasurementSeriesInit,
		MeasurementSeriesStep,
		ODMRTraceInit,
		ODMRTraceWait,
		ODMRTraceFinish,
		SensitivityInit,
		SensitivityWait,
		SensitivityFinish
	};

	using StateMachineStateType = Util::StateMachineState<StateType(ODMR::*)(DynExp::ModuleInstance&)>;

	struct ODMRPlotType
	{
		// x is frequency in GHz, y is ODMR signal in units of SignalDetector.
		QList<QPointF> DataPoints;
		QList<QPointF> FitPoints;
		std::tuple<double, double> FitParams;	// tuple<c0, c1> with fit model ODMRSignal = c0 + c1 * frequency
		QPointF DataPointsMinValues;
		QPointF DataPointsMaxValues;
		QPointF SelectedPoint;
		bool HasChanged = false;
	};

	struct SensitivityPlotType
	{
		// x is frequency in Hz, y is sensitivity in T/sqrt(Hz).
		QList<QPointF> DataPoints;
		QPointF DataPointsMinValues;
		QPointF DataPointsMaxValues;
		bool HasChanged = false;
	};

	class ODMRWidget : public DynExp::QModuleWidget
	{
		Q_OBJECT

	private:
		struct StatusBarType
		{
			StatusBarType(ODMRWidget* Owner);

			void Update();

			const StateMachineStateType* CurrentState;

			QLabel* StateLabel;
			QLabel* SweepStateLabel;
			QLabel* AcquisitionTimeLabel;
		};

	public:
		ODMRWidget(ODMR& Owner, QModuleWidget* parent = nullptr);
		~ODMRWidget() = default;

		bool AllowResize() const noexcept override final { return true; }

		void InitializeUI(Util::SynchronizedPointer<ODMRData>& ModuleData);
		void SetUIState(const StateMachineStateType* State, Util::SynchronizedPointer<ODMRData>& ModuleData);
		void UpdateUIData(Util::SynchronizedPointer<ODMRData>& ModuleData);
		void UpdateODMRPlot(const ODMRPlotType& ODMRPlot);
		void UpdateSensitivityPlot(const SensitivityPlotType& SensitivityPlot);

		const auto& GetUI() const noexcept { return ui; }
		bool GetUIInitialized() const noexcept { return UIInitialized; }
		auto GetODMRDataSeries() const noexcept { return ODMRDataSeries; }

	private:
		Ui::ODMR ui;
		StatusBarType StatusBar;

		QLineSeries* ODMRDataSeries;
		QLineSeries* ODMRFitSeries;
		QChart* ODMRDataChart;
		QValueAxis* ODMRXAxis;
		QValueAxis* ODMRYAxis;

		QXYSeries* SensitivityDataSeries;
		QChart* SensitivityDataChart;
		QLogValueAxis* SensitivityXAxis;
		QLogValueAxis* SensitivityYAxis;

		DynExpInstr::DataStreamInstrumentData::UnitType AuxAnalogOutValueUnit = DynExpInstr::DataStreamInstrumentData::UnitType::Arbitrary;
		DynExpInstr::DataStreamInstrumentData::ValueType AuxAnalogOutMinValue = 0.0;
		DynExpInstr::DataStreamInstrumentData::ValueType AuxAnalogOutMaxValue = 1.0;

		bool UIInitialized = false;

	private slots:
		void OnBrowseSavePathClicked();
		void OnSweepSeriesParamChanged(int Index);
	};
}