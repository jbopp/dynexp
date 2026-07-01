// This file is part of DynExp.

/**
 * @file SignalPlotter.h
 * @brief Implementation of a module to plot the samples stored in data stream instruments.
*/

#pragma once

#include "stdafx.h"
#include "DynExpCore.h"
#include "../GraphUtil.h"
#include "../../MetaInstruments/DataStreamInstrument.h"

namespace DynExpQuick
{
	struct DynExpLineGraphPlotInfo;
}

namespace DynExpModule
{
	class SignalPlotter;

	class SignalPlotterData : public DynExp::QMLModuleDataBase
	{
	public:
		struct SampleDataType
		{
			/**
			 * @brief Data points of a single data series to plot.
			*/
			QList<QPointF> Samples;

			/**
			 * @brief Determines whether this series should be processed and plotted.
			*/
			bool Visible = true;
		};

		SignalPlotterData();
		virtual ~SignalPlotterData() = default;

		void LockInstruments(DynExp::ModuleInstance* Instance, const DynExp::ParamsBase::ListParam<DynExp::ObjectLink<DynExpInstr::DataStreamInstrument>>& InstrParam) { Instance->LockObject(InstrParam, DataStreamInstr); }
		void UnlockInstruments(DynExp::ModuleInstance* Instance) { Instance->UnlockObject(DataStreamInstr); }

		auto& GetDataStreamInstr(size_t Index) { return DataStreamInstr[Index]; }
		auto GetDataStreamInstrCount() const noexcept { return DataStreamInstr.GetList().size(); }
		const auto& GetDataStreamInstrLabels() const noexcept { return DataStreamInstr.GetLabels(); }
		std::string_view GetDataStreamInstrIconPath() const { return DataStreamInstr.GetIconPath(); }

		bool IsUIInitialized() const noexcept { return UIInitialized; }
		void SetUIInitialized() noexcept { UIInitialized = true; }

		bool Running;
		bool RollingView;
		bool Autoscale;

		Graph::LineGraphPlotInfo PlotInfo;
		std::vector<SampleDataType> SampleDataList;

	private:
		void ResetImpl(dispatch_tag<QMLModuleDataBase>) override final;
		virtual void ResetImpl(dispatch_tag<SignalPlotterData>) {};

		void Init();

		bool UIInitialized;

		DynExp::LinkedObjectWrapperContainerList<DynExpInstr::DataStreamInstrument> DataStreamInstr;
	};

	class SignalPlotterParams : public DynExp::QMLModuleParamsBase
	{
	public:
		SignalPlotterParams(DynExp::ItemIDType ID, const DynExp::DynExpCore& Core) : QMLModuleParamsBase(ID, Core) {}
		virtual ~SignalPlotterParams() = default;

		virtual const char* GetParamClassTag() const noexcept override { return "SignalPlotterParams"; }

		ListParam<DynExp::ObjectLink<DynExpInstr::DataStreamInstrument>> DataStreamInstr = { *this, GetCore().GetInstrumentManager(),
			"DataStreamInstr", "Data stream instrument(s)", "Underlying data stream instrument(s) to be used as a data source(s)", DynExpUI::Icons::Instrument };

		Param<bool> RollingView = { *this, "RollingView", false };
		Param<bool> Autoscale = { *this, "Autoscale", true };

	private:
		void ConfigureParamsImpl(dispatch_tag<QMLModuleParamsBase>) override final {}
	};

	class SignalPlotterConfigurator : public DynExp::QMLModuleConfiguratorBase
	{
	public:
		using ObjectType = SignalPlotter;
		using ParamsType = SignalPlotterParams;

		SignalPlotterConfigurator() = default;
		virtual ~SignalPlotterConfigurator() = default;

	private:
		virtual DynExp::ParamsBasePtrType MakeParams(DynExp::ItemIDType ID, const DynExp::DynExpCore& Core) const override final { return DynExp::MakeParams<SignalPlotterConfigurator>(ID, Core); }
	};

	class SignalPlotter : public DynExp::QMLModuleBase
	{
	public:
		using ParamsType = SignalPlotterParams;
		using ConfigType = SignalPlotterConfigurator;
		using ModuleDataType = SignalPlotterData;

		constexpr static auto Name() noexcept { return "Signal Plotter"; }
		constexpr static auto Category() noexcept { return "I/O"; }

		SignalPlotter(const std::thread::id OwnerThreadID, DynExp::ParamsBasePtrType&& Params)
			: QMLModuleBase(OwnerThreadID, std::move(Params)) {}
		virtual ~SignalPlotter() = default;

		virtual std::string GetName() const override { return Name(); }
		virtual std::string GetCategory() const override { return Category(); }

		std::chrono::milliseconds GetMainLoopDelay() const override final { return std::chrono::milliseconds(50); }

	private:
		Util::DynExpErrorCodes::DynExpErrorCodes ModuleMainLoop(DynExp::ModuleInstance& Instance) override final;

		void ResetImpl(dispatch_tag<QMLModuleBase>) override final;

		void MakeConnections(QObject* Backend) override final;
		QAnyStringView GetModuleSourceUri() const noexcept override final { return "Modules.SignalPlotter"; }
		QAnyStringView GetModuleSourceTypeName() const noexcept override final { return "SignalPlotter"; }
		void UpdateUIChild(const ModuleBase::ModuleDataGetterType& ModuleDataGetter) override final;

		// Events, run in module thread
		void OnInit(DynExp::ModuleInstance* Instance) const override final;
		void OnExit(DynExp::ModuleInstance* Instance) const override final;
		void OnRollingViewChanged(DynExp::ModuleInstance* Instance, bool State) const;
		void OnAutoscaleChanged(DynExp::ModuleInstance* Instance, bool State) const;
		void OnClearStream(DynExp::ModuleInstance* Instance) const;

		// Events, run in UI thread
		void OnSaveData() const;

		void FinishedSavingData() const noexcept { IsSavingData = false; }
		using FinishedSavingDataGuardType = Util::OnDestruction<const SignalPlotter, decltype(&SignalPlotter::FinishedSavingData)>;

		/**
		 * @brief If data is currently being saved to file, do not update internal data.
		*/
		mutable std::atomic_bool IsSavingData = false;

		size_t NumFailedUpdateAttempts = 0;

		/**
		 * @brief Does nothing but ensures that this module's Qt connections are disconnected when
		 * @p SignalContext is destroyed. Moreover, @p SignalContext is created in the UI thread
		 * (that creates this module instance). Hence, it ensures that Qt connections using this
		 * context are executed in the UI thread.
		*/
		QObject SignalContext;
	};
}