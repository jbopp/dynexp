// This file is part of DynExp.

#pragma once

#include "stdafx.h"
#include "DynExpCore.h"
#include "../MetaInstruments/Laser.h"

#include <QWidget>
#include "ui_LaserControl.h"

namespace DynExpModule::LaserControl
{
	class LaserControl;
	class LaserControlData;

	class LaserControlWidget : public DynExp::QModuleWidget
	{
		Q_OBJECT

	public:
		LaserControlWidget(LaserControl& Owner, QModuleWidget* parent = nullptr);
		~LaserControlWidget() = default;

		bool AllowResize() const noexcept override final { return true; }

		const auto& GetUI() const noexcept { return ui; }

		void InitializeUI(Util::SynchronizedPointer<LaserControlData>& ModuleData);
		void UpdateUI(Util::SynchronizedPointer<LaserControlData>& ModuleData);
	
	private:
		Ui::LaserControl ui;
	};

	class LaserControlData : public DynExp::QModuleDataBase
	{
	public:
		LaserControlData() { Init(); }
		virtual ~LaserControlData() = default;

		bool IsUIInitialized() const noexcept { return UIInitialized; }
		void SetUIInitialized() noexcept { UIInitialized = true; }
		auto& GetLaser() { return Laser; }

		DynExpInstr::LaserData::FrequencyUnitType FrequencyUnit;
		DynExpInstr::LaserData::IntensityUnitType IntensityUnit;
		double HardwareMinFrequency;
		double HardwareMaxFrequency;
		double HardwareMinIntensity;
		double HardwareMaxIntensity;
		double HardwareMinBandwidth;
		double HardwareMaxBandwidth;
		double HardwareMaxRate;
		double HardwareModeHopFreeTuningRange;
		double Frequency;
		double Wavelength;
		double Intensity;
		double ScanRange;
		double ScanRate;
		DynExpInstr::LaserData::LaserStateType LaserState;

		constexpr auto ConvertToNm() const noexcept { return 299792458 / Frequency * 1e9; }

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

		void OnFrequencyValueChanged(DynExp::ModuleInstance* Instance, const double Frequency) const;
		void OnWavelengthValueChanged(DynExp::ModuleInstance* Instance, const double Wavelength) const;
		void OnIntensityValueChanged(DynExp::ModuleInstance* Instance, const double Intensity) const;
		void OnScanRangeValueChanged(DynExp::ModuleInstance* Instance, const double ScanRange) const;
		void OnScanRateValueChanged(DynExp::ModuleInstance* Instance, const double ScanRate) const;

		void OnEnableClicked(DynExp::ModuleInstance* Instance) const;
		void OnDisableClicked(DynExp::ModuleInstance* Instance) const;
		void OnScanToggled(DynExp::ModuleInstance* Instance) const;

		size_t NumFailedUpdateAttempts = 0;
	};
}
