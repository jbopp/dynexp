// This file is part of DynExp.

/**
 * @file SignalPlotter.h
 * @brief Implementation of a module to plot the samples stored in data stream instruments.
*/

#pragma once

#include "stdafx.h"
#include "DynExpCore.h"
#include "../MetaInstruments/DataStreamInstrument.h"

#include <QWidget>
#include "ui_SignalPlotter.h"

namespace DynExpModule
{
	class SignalPlotter;

	class SignalPlotterWidget : public DynExp::QModuleWidget
	{
		Q_OBJECT

	public:
		struct SampleDataType
		{
			SampleDataType() { Reset(); }

			void Reset();
			QString GetMultiplierLabel() const;

			QList<QPointF> Points;
			QPointF MinValues;
			QPointF MaxValues;
			unsigned int Multiplier = 0;
		};

		SignalPlotterWidget(SignalPlotter& Owner, QModuleWidget* parent = nullptr);
		~SignalPlotterWidget() = default;

		bool AllowResize() const noexcept override final { return true; }
		
		auto* GetAutoscalePlotAction() noexcept { return PlotAutoscaleAction; }
		auto* GetRollingViewPlotAction() noexcept { return PlotRollingViewAction; }
		auto* GetClearPlotAction() const noexcept { return PlotClearAction; }

		void UpdateUI(bool IsRunning);
		void SetAxes(QValueAxis* XAxis, QValueAxis* YAxis);
		void SetData(const SampleDataType& SampleData);

		auto GetXAxis() noexcept { return XAxis; }
		auto GetYAxis() noexcept { return YAxis; }
		auto GetMultiplier() noexcept { return Multiplier; }

		Ui::SignalPlotter ui;

	private:
		void FinishedSavingData() noexcept { IsSavingData = false; }
		using FinishedSavingDataGuardType = Util::OnDestruction<SignalPlotterWidget, decltype(&SignalPlotterWidget::FinishedSavingData)>;

		QMenu* PlotContextMenu;
		QAction* PlotAutoscaleAction;
		QAction* PlotRollingViewAction;
		QAction* PlotClearAction;

		QXYSeries* DataSeries;
		QChart* DataChart;
		QValueAxis* XAxis;
		QValueAxis* YAxis;

		/**
		 * @brief Describes the factor 10^Multiplier the displayed data has been multiplied with for better x-label readability.
		*/
		decltype(SampleDataType::Multiplier) Multiplier = 0;

		// If data is currently being saved to file, do not update internal data.
		std::atomic_bool IsSavingData = false;

	private slots:
		void OnPlotContextMenuRequested(const QPoint& Position);
		void OnSaveCSVClicked();
	};

	class SignalPlotterData : public DynExp::QModuleDataBase
	{
	public:
		SignalPlotterData() { Init(); }
		virtual ~SignalPlotterData() = default;

		void LockInstruments(DynExp::ModuleInstance* Instance, const DynExp::ParamsBase::ListParam<DynExp::ObjectLink<DynExpInstr::DataStreamInstrument>>& InstrParam) { Instance->LockObject(InstrParam, DataStreamInstr); }
		void UnlockInstruments(DynExp::ModuleInstance* Instance) { Instance->UnlockObject(DataStreamInstr); }

		void SetCurrentSourceIndex(int Index);
		auto& GetDataStreamInstr() { return DataStreamInstr[CurrentSourceIndex]; }
		const auto& GetDataStreamInstrLabels() const noexcept { return DataStreamInstr.GetLabels(); }
		std::string_view GetDataStreamInstrIconPath() const { return DataStreamInstr.GetIconPath(); }

		bool IsUIInitialized() const noexcept { return UIInitialized; }
		void SetUIInitialized() noexcept { UIInitialized = true; }

		bool IsRunning;
		bool IsBasicSampleTimeUsed;
		DynExpInstr::DataStreamInstrumentData::UnitType ValueUnit;
		bool PlotAxesChanged;
		bool Autoscale;
		bool RollingView;

		SignalPlotterWidget::SampleDataType SampleData;

	private:
		void ResetImpl(dispatch_tag<QModuleDataBase>) override final;
		virtual void ResetImpl(dispatch_tag<SignalPlotterData>) {};

		void Init();

		bool UIInitialized;
		int CurrentSourceIndex;

		DynExp::LinkedObjectWrapperContainerList<DynExpInstr::DataStreamInstrument> DataStreamInstr;
	};

	class SignalPlotterParams : public DynExp::QModuleParamsBase
	{
	public:
		SignalPlotterParams(DynExp::ItemIDType ID, const DynExp::DynExpCore& Core) : QModuleParamsBase(ID, Core) {}
		virtual ~SignalPlotterParams() = default;

		virtual const char* GetParamClassTag() const noexcept override { return "SignalPlotterParams"; }

		ListParam<DynExp::ObjectLink<DynExpInstr::DataStreamInstrument>> DataStreamInstr = { *this, GetCore().GetInstrumentManager(),
			"DataStreamInstr", "Data stream instrument(s)", "Underlying data stream instrument(s) to be used as a data source(s)", DynExpUI::Icons::Instrument };

		Param<bool> Autoscale = { *this, "Autoscale", true };
		Param<bool> RollingView = { *this, "RollingView", false };

	private:
		void ConfigureParamsImpl(dispatch_tag<QModuleParamsBase>) override final {}
	};

	class SignalPlotterConfigurator : public DynExp::QModuleConfiguratorBase
	{
	public:
		using ObjectType = SignalPlotter;
		using ParamsType = SignalPlotterParams;

		SignalPlotterConfigurator() = default;
		virtual ~SignalPlotterConfigurator() = default;

	private:
		virtual DynExp::ParamsBasePtrType MakeParams(DynExp::ItemIDType ID, const DynExp::DynExpCore& Core) const override final { return DynExp::MakeParams<SignalPlotterConfigurator>(ID, Core); }
	};

	class SignalPlotter : public DynExp::QModuleBase
	{
	public:
		using ParamsType = SignalPlotterParams;
		using ConfigType = SignalPlotterConfigurator;
		using ModuleDataType = SignalPlotterData;

		constexpr static auto Name() noexcept { return "Signal Plotter"; }
		constexpr static auto Category() noexcept { return "I/O"; }

		SignalPlotter(const std::thread::id OwnerThreadID, DynExp::ParamsBasePtrType&& Params)
			: QModuleBase(OwnerThreadID, std::move(Params)) {}
		virtual ~SignalPlotter() = default;

		virtual std::string GetName() const override { return Name(); }
		virtual std::string GetCategory() const override { return Category(); }

		std::chrono::milliseconds GetMainLoopDelay() const override final { return std::chrono::milliseconds(50); }

	private:
		Util::DynExpErrorCodes::DynExpErrorCodes ModuleMainLoop(DynExp::ModuleInstance& Instance) override final;

		void ResetImpl(dispatch_tag<QModuleBase>) override final;

		std::unique_ptr<DynExp::QModuleWidget> MakeUIWidget() override final;
		void UpdateUIChild(const ModuleBase::ModuleDataGetterType& ModuleDataGetter) override final;

		/**
		 * @brief Converts BasicSamples to displayable format.
		 * @param BasicSamples Vector of BasicSamples to convert. Samples are move from this vector.
		 * @param IsBasicSampleTimeUsed Use sample indices or time data for x coordinate? Takes IsBasicSampleTimeUsed as a parameter
		 * and not from SignalPlotterData in order not to lock module data while performing potentially heavy calculation.
		 * @return Returns true when the function decided to fall back to IsBasicSampleTimeUsed = false due to BasicSamples having
		 * all equal x values. 
		*/
		bool ProcessBasicSamples(std::vector<DynExpInstr::BasicSample>&& BasicSamples, bool IsBasicSampleTimeUsed);

		// Events, run in module thread
		void OnInit(DynExp::ModuleInstance* Instance) const override final;
		void OnExit(DynExp::ModuleInstance* Instance) const override final;
		void OnRunClicked(DynExp::ModuleInstance* Instance, bool Checked) const;
		void OnSourceChanged(DynExp::ModuleInstance* Instance, int Index) const;
		void OnPlotAutoscaleClicked(DynExp::ModuleInstance* Instance, bool Checked) const;
		void OnPlotRollingViewClicked(DynExp::ModuleInstance* Instance, bool Checked) const;
		void OnClearStream(DynExp::ModuleInstance* Instance, bool) const;

		size_t NumFailedUpdateAttempts = 0;
	};
}