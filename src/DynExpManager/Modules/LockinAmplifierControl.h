// This file is part of DynExp.

/**
 * @file LockinAmplifierControl.h
 * @brief Implementation of a module to control a lock-in amplifier.
*/

#pragma once

#include "stdafx.h"
#include "DynExpCore.h"
#include "../MetaInstruments/LockinAmplifier.h"

#include <QWidget>
#include "ui_LockinAmplifierControl.h"

namespace DynExpModule
{
	class LockinAmplifierControl;
	class LockinAmplifierControlData;

	class LockinAmplifierControlWidget : public DynExp::QModuleWidget
	{
		Q_OBJECT

	public:
		LockinAmplifierControlWidget(LockinAmplifierControl& Owner, QModuleWidget* parent = nullptr);
		~LockinAmplifierControlWidget() = default;

		bool AllowResize() const noexcept override final { return false; }

		const auto& GetUI() const noexcept { return ui; }
		bool GetUIInitialized() const noexcept { return UIInitialized; }

		void InitializeUI(Util::SynchronizedPointer<LockinAmplifierControlData>& ModuleData);

	private:
		Ui::LockinAmplifierControl ui;

		bool UIInitialized = false;
	};

	class LockinAmplifierControlData : public DynExp::QModuleDataBase
	{
	public:
		LockinAmplifierControlData() { Init(); }
		virtual ~LockinAmplifierControlData() = default;

		auto& GetLockinAmplifier() noexcept { return LockinAmplifier; }
		auto& GetLockinAmplifier() const noexcept { return LockinAmplifier; }

		double CurrentSensitivity{};
		double CurrentPhase{};			// in rad
		double CurrentTimeConstant{};	// in s
		uint8_t CurrentFilterOrder{};
		DynExpInstr::LockinAmplifierDefs::TriggerModeType CurrentTriggerMode{};
		DynExpInstr::LockinAmplifierDefs::TriggerEdgeType CurrentTriggerEdge{};
		DynExpInstr::LockinAmplifierDefs::SignalType CurrentSignal{};
		double CurrentSamplingRate{};	// in samples/s
		bool CurrentEnable{};
		bool CurrentOverload{};
		double CurrentNegInputLoad{};
		double CurrentPosInputLoad{};
		double CurrentOscillatorFrequency{};
		double CurrentAcquisitionProgress{};
		std::string SensitivityUnitString;

	private:
		void ResetImpl(dispatch_tag<QModuleDataBase>) override final;
		virtual void ResetImpl(dispatch_tag<LockinAmplifierControlData>) {};

		void Init();

		DynExp::LinkedObjectWrapperContainer<DynExpInstr::LockinAmplifier> LockinAmplifier;
	};

	class LockinAmplifierControlParams : public DynExp::QModuleParamsBase
	{
	public:
		LockinAmplifierControlParams(DynExp::ItemIDType ID, const DynExp::DynExpCore& Core) : QModuleParamsBase(ID, Core) {}
		virtual ~LockinAmplifierControlParams() = default;

		virtual const char* GetParamClassTag() const noexcept override { return "LockinAmplifierControlParams"; }

		Param<DynExp::ObjectLink<DynExpInstr::LockinAmplifier>> LockinAmplifier = { *this, GetCore().GetInstrumentManager(),
			"LockinAmplifier", "Lock-in amplifier", "Underlying lock-in amplifier to be controlled", DynExpUI::Icons::Instrument };

	private:
		void ConfigureParamsImpl(dispatch_tag<QModuleParamsBase>) override final {}
	};

	class LockinAmplifierControlConfigurator : public DynExp::QModuleConfiguratorBase
	{
	public:
		using ObjectType = LockinAmplifierControl;
		using ParamsType = LockinAmplifierControlParams;

		LockinAmplifierControlConfigurator() = default;
		virtual ~LockinAmplifierControlConfigurator() = default;

	private:
		virtual DynExp::ParamsBasePtrType MakeParams(DynExp::ItemIDType ID, const DynExp::DynExpCore& Core) const override final { return DynExp::MakeParams<LockinAmplifierControlConfigurator>(ID, Core); }
	};

	class LockinAmplifierControl : public DynExp::QModuleBase
	{
	public:
		using ParamsType = LockinAmplifierControlParams;
		using ConfigType = LockinAmplifierControlConfigurator;
		using ModuleDataType = LockinAmplifierControlData;

		constexpr static auto Name() noexcept { return "Lock-in Amplifier Control"; }
		constexpr static auto Category() noexcept { return "I/O"; }

		LockinAmplifierControl(const std::thread::id OwnerThreadID, DynExp::ParamsBasePtrType&& Params)
			: QModuleBase(OwnerThreadID, std::move(Params)) {}
		virtual ~LockinAmplifierControl() = default;

		virtual std::string GetName() const override { return Name(); }
		virtual std::string GetCategory() const override { return Category(); }

		std::chrono::milliseconds GetMainLoopDelay() const override final { return std::chrono::milliseconds(100); }

	private:
		Util::DynExpErrorCodes::DynExpErrorCodes ModuleMainLoop(DynExp::ModuleInstance& Instance) override final;

		void ResetImpl(dispatch_tag<QModuleBase>) override final;

		std::unique_ptr<DynExp::QModuleWidget> MakeUIWidget() override final;
		void UpdateUIChild(const ModuleBase::ModuleDataGetterType& ModuleDataGetter) override final;

		// Events, run in module thread
		void OnInit(DynExp::ModuleInstance* Instance) const override final;
		void OnExit(DynExp::ModuleInstance* Instance) const override final;

		void OnSensitivityChanged(DynExp::ModuleInstance* Instance, double Value) const;
		void OnAutoRangeClicked(DynExp::ModuleInstance* Instance, bool) const;
		void OnPhaseChanged(DynExp::ModuleInstance* Instance, double Value) const;
		void OnAutoPhaseClicked(DynExp::ModuleInstance* Instance, bool) const;
		void OnTimeConstantChanged(DynExp::ModuleInstance* Instance, double Value) const;
		void OnFilterOrderChanged(DynExp::ModuleInstance* Instance, int Value) const;
		void OnTriggerModeChanged(DynExp::ModuleInstance* Instance, int Index) const;
		void OnTriggerEdgeChanged(DynExp::ModuleInstance* Instance, int Index) const;
		void OnForceTriggerClicked(DynExp::ModuleInstance* Instance, bool) const;
		void OnSignalTypeChanged(DynExp::ModuleInstance* Instance, int Index) const;
		void OnSamplingRateChanged(DynExp::ModuleInstance* Instance, double Value) const;
		void OnEnableClicked(DynExp::ModuleInstance* Instance, int Value) const;
		void OnPersistParamsClicked(DynExp::ModuleInstance* Instance, bool) const;

		static const char* ProgressBarRedStylesheet;
		static const char* ProgressBarGreenStylesheet;

		size_t NumFailedUpdateAttempts = 0;
	};
}