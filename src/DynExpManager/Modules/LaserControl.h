// This file is part of DynExp.

/**
 * @file LaserControl.h
 * @brief Implementation of a module to control a laser source.
*/

#pragma once

#include "stdafx.h"
#include "DynExpCore.h"
#include "../MetaInstruments/Laser.h"

#include <QWidget>

namespace Ui
{
	class LaserControl;
}

namespace DynExpModule
{
	class LaserControl;
	class LaserControlData;

	class LaserControlWidget : public DynExp::QModuleWidget
	{
		Q_OBJECT

	public:
		LaserControlWidget(LaserControl& Owner, QModuleWidget* parent = nullptr);
		~LaserControlWidget() = default;

		bool AllowResize() const noexcept override final { return false; }

		const auto GetUI() const noexcept { return ui.get(); }

		void InitializeUI(Util::SynchronizedPointer<LaserControlData>& ModuleData);
		void UpdateUI(Util::SynchronizedPointer<LaserControlData>& ModuleData);

	private:
		std::unique_ptr<Ui::LaserControl> ui;
	};

	class LaserControlData : public DynExp::QModuleDataBase
	{
	public:
		LaserControlData() { Init(); }
		virtual ~LaserControlData() = default;

		bool IsUIInitialized() const noexcept { return UIInitialized; }
		void SetUIInitialized() noexcept { UIInitialized = true; }
		auto& GetLaser() { return Laser; }

		double FrequencyInHzToLaserUnit(double Value) const;
		double FrequencyInLaserUnitToHz(double Value) const;
		double IntensityInWToLaserUnit(double Value) const;
		double IntensityInLaserUnitToW(double Value) const;

		DynExp::Units::UnitType FrequencyUnit;
		DynExp::Units::UnitType IntensityUnit;
		double HardwareMinFrequency;
		double HardwareMaxFrequency;
		double HardwareMinIntensity;
		double HardwareMaxIntensity;
		double HardwareMinScanRange;
		double HardwareMaxScanRange;
		double HardwareMinScanRate;
		double HardwareMaxScanRate;
		double HardwareModeHopFreeTuningRange;
		double CurrentFrequency;
		double CurrentIntensity;
		double CurrentScanRange;
		double CurrentScanRate;
		DynExpInstr::LaserData::LaserStateType LaserState;

	private:
		void ResetImpl(dispatch_tag<QModuleDataBase>) override final;
		virtual void ResetImpl(dispatch_tag<LaserControlData>) {};

		void Init();

		bool UIInitialized;

		DynExp::LinkedObjectWrapperContainer<DynExpInstr::Laser> Laser;
	};

	class LaserControlParams : public DynExp::QModuleParamsBase
	{
	public:
		LaserControlParams(DynExp::ItemIDType ID, const DynExp::DynExpCore& Core) : QModuleParamsBase(ID, Core) {}
		virtual ~LaserControlParams() = default;

		virtual const char* GetParamClassTag() const noexcept override { return "LaserControlParams"; }

		Param<DynExp::ObjectLink<DynExpInstr::Laser>> Laser = { *this, GetCore().GetInstrumentManager(),
			"Laser", "Laser", "Underlying laser instrument to be controlled by this module", DynExpUI::Icons::Instrument };

	private:
		void ConfigureParamsImpl(dispatch_tag<QModuleParamsBase>) override final {}
	};

	class LaserControlConfigurator : public DynExp::QModuleConfiguratorBase
	{
	public:
		using ObjectType = LaserControl;
		using ParamsType = LaserControlParams;

		LaserControlConfigurator() = default;
		virtual ~LaserControlConfigurator() = default;

	private:
		virtual DynExp::ParamsBasePtrType MakeParams(DynExp::ItemIDType ID, const DynExp::DynExpCore& Core) const override final { return DynExp::MakeParams<LaserControlConfigurator>(ID, Core); }
	};

	class LaserControl : public DynExp::QModuleBase
	{
	public:
		using ParamsType = LaserControlParams;
		using ConfigType = LaserControlConfigurator;
		using ModuleDataType = LaserControlData;

		constexpr static auto Name() noexcept { return "Laser Control"; }
		constexpr static auto Category() noexcept { return "I/O"; }

		LaserControl(const std::thread::id OwnerThreadID, DynExp::ParamsBasePtrType&& Params)
			: QModuleBase(OwnerThreadID, std::move(Params)) {}
		virtual ~LaserControl() = default;

		virtual std::string GetName() const override { return Name(); }
		virtual std::string GetCategory() const override { return Category(); }

		std::chrono::milliseconds GetMainLoopDelay() const override final { return std::chrono::milliseconds(50); }

	private:
		Util::DynExpErrorCodes::DynExpErrorCodes ModuleMainLoop(DynExp::ModuleInstance& Instance) override final;

		void ResetImpl(dispatch_tag<QModuleBase>) override final;

		std::unique_ptr<DynExp::QModuleWidget> MakeUIWidget() override final;
		void UpdateUIChild(const ModuleBase::ModuleDataGetterType& ModuleDataGetter) override final;

		// Events, run in module thread
		void OnInit(DynExp::ModuleInstance* Instance) const override final;
		void OnExit(DynExp::ModuleInstance* Instance) const override final;

		void OnEnableToggled(DynExp::ModuleInstance* Instance, bool) const;
		void OnScanToggled(DynExp::ModuleInstance* Instance, bool) const;
		void OnFrequencyValueChanged(DynExp::ModuleInstance* Instance, double Frequency) const;
		void OnWavelengthValueChanged(DynExp::ModuleInstance* Instance, double Wavelength) const;
		void OnIntensityValueChanged(DynExp::ModuleInstance* Instance, double Intensity) const;
		void OnScanRangeValueChanged(DynExp::ModuleInstance* Instance, double ScanRange) const;
		void OnScanRateValueChanged(DynExp::ModuleInstance* Instance, double ScanRate) const;

		size_t NumFailedUpdateAttempts = 0;
	};
}