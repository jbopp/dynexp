// This file is part of DynExp.

#include "stdafx.h"
#include "moc_LaserControl.cpp"
#include "LaserControl.h"

namespace DynExpModule::LaserControl
{
	LaserControlWidget::LaserControlWidget(LaserControl& Owner, QModuleWidget* parent)
		: QModuleWidget(Owner, parent)
	{
		ui.setupUi(this);

		// For shortcuts
		this->addAction(ui.action_Enable);
		this->addAction(ui.action_Disable);
	}
	
	void LaserControlWidget::InitializeUI(Util::SynchronizedPointer<LaserControlData>& ModuleData)
	{
		const QSignalBlocker blockFreq_Hz(ui.SBFrequency);
		ui.SBFrequency->setRange(ModuleData->HardwareMinFrequency * 1e-12, ModuleData->HardwareMaxFrequency * 1e-12);
		ui.SBFrequency->setSuffix(" T" + QString(DynExpInstr::LaserData::FrequencyUnitTypeToStr(ModuleData->FrequencyUnit)));
		ui.SBFrequency->setValue(ModuleData->HardwareMinFrequency * 1e-12);

		const QSignalBlocker blockFreq_nm(ui.SBWavelength);
		ui.SBWavelength->setRange(DynExpInstr::LaserData::FrequencyWavelengthConversion(ModuleData->HardwareMaxFrequency), DynExpInstr::LaserData::FrequencyWavelengthConversion(ModuleData->HardwareMinFrequency));
		ui.SBWavelength->setSuffix(" nm");
		ui.SBWavelength->setValue(DynExpInstr::LaserData::FrequencyWavelengthConversion(ModuleData->HardwareMaxFrequency));

		const QSignalBlocker blockInt(ui.SBIntensity);
		ui.SBIntensity->setRange(ModuleData->HardwareMinIntensity * 1e3, ModuleData->HardwareMaxIntensity * 1e3);
		ui.SBIntensity->setSuffix(" m" + QString(DynExpInstr::LaserData::IntensityUnitTypeToStr(ModuleData->IntensityUnit)));
		ui.SBIntensity->setValue(ModuleData->HardwareMinIntensity * 1e3);

		const QSignalBlocker blockRange(ui.SBScanRange);
		ui.SBScanRange->setRange(ModuleData->HardwareMinBandwidth * 1e-9, ModuleData->HardwareMaxBandwidth * 1e-9);
		ui.SBScanRange->setSuffix(" G" + QString(DynExpInstr::LaserData::FrequencyUnitTypeToStr(ModuleData->FrequencyUnit)));

		const QSignalBlocker blockRate(ui.SBScanRate);
		ui.SBScanRate->setRange(0.1, ModuleData->HardwareMaxRate * 1e-9);
		ui.SBScanRate->setSuffix(" G" + QString(DynExpInstr::LaserData::FrequencyUnitTypeToStr(ModuleData->FrequencyUnit)) + "/s");
	}

	void LaserControlWidget::UpdateUI(Util::SynchronizedPointer<LaserControlData>& ModuleData)
	{
		ui.action_Enable->setEnabled(ModuleData->LaserState == DynExpInstr::LaserData::LaserStateType::Ready);
		ui.action_EnableScan->setEnabled(ModuleData->LaserState == DynExpInstr::LaserData::LaserStateType::Ready || ModuleData->LaserState == DynExpInstr::LaserData::LaserStateType::EmissionEnabledConstant || ModuleData->LaserState == DynExpInstr::LaserData::LaserStateType::EmissionEnabledScanning);
		ui.action_Disable->setEnabled(ModuleData->LaserState == DynExpInstr::LaserData::LaserStateType::EmissionEnabledConstant || ModuleData->LaserState == DynExpInstr::LaserData::LaserStateType::EmissionEnabledScanning);
		
		ui.SBFrequency->setEnabled(ModuleData->LaserState != DynExpInstr::LaserData::LaserStateType::Startup);
		ui.LActualFrequency->setText(QString::number(ModuleData->Frequency * 1e-12, 'f', 6) + " T" + QString(DynExpInstr::LaserData::FrequencyUnitTypeToStr(ModuleData->FrequencyUnit)));
		ui.SBWavelength->setEnabled(ModuleData->LaserState != DynExpInstr::LaserData::LaserStateType::Startup);
		ui.LActualWavelength->setText(QString::number(ModuleData->Wavelength, 'f', 6) + " nm");
		
		if (ModuleData->Intensity < ModuleData->HardwareMinIntensity)
		{
			ui.LActualIntensity->setText(QString::number(ModuleData->Intensity * 1e3, 'f', 3) + " m" + QString(DynExpInstr::LaserData::IntensityUnitTypeToStr(ModuleData->IntensityUnit)) + " (Power low!)");
			ui.LActualIntensity->setStyleSheet(DynExpUI::StatusBarWarningStyleSheet);
		}
		else
		{
			ui.LActualIntensity->setText(QString::number(ModuleData->Intensity * 1e3, 'f', 3) + " m" + QString(DynExpInstr::LaserData::IntensityUnitTypeToStr(ModuleData->IntensityUnit)));
			ui.LActualIntensity->setStyleSheet("");
		}

		if (!ui.SBScanRange->hasFocus())
		{
			const QSignalBlocker Blocker(ui.SBScanRange);
			ui.SBScanRange->setValue(ModuleData->ScanRange * 1e-9);
		}
		if (!ui.SBScanRate->hasFocus())
		{
			const QSignalBlocker Blocker(ui.SBScanRate);
			ui.SBScanRate->setValue(ModuleData->ScanRate * 1e-9);
		}

		switch (ModuleData->LaserState)
		{
		case DynExpInstr::LaserData::LaserStateType::Ready:
			ui.LState->setText(" Laser is ready for emission.");
			ui.LState->setStyleSheet(DynExpUI::StatusBarReadyStyleSheetBright);
			break;
		case DynExpInstr::LaserData::LaserStateType::Startup:
			ui.LState->setText(" Startup...");
			ui.LState->setStyleSheet(DynExpUI::StatusBarReadyStyleSheetBright);
			break;
		case DynExpInstr::LaserData::LaserStateType::EmissionEnabledConstant:
			ui.LState->setText(" Emitting in constant mode.");
			ui.LState->setStyleSheet(DynExpUI::StatusBarBusyStyleSheet);
			break; 
		case DynExpInstr::LaserData::LaserStateType::EmissionEnabledScanning:
			ui.LState->setText(" Emitting in scanning mode.");
			ui.LState->setStyleSheet(DynExpUI::StatusBarBusyStyleSheet);
			break;
		case DynExpInstr::LaserData::LaserStateType::Error:
			ui.LState->setText(" The Laser is in an error state. Please redial wavelength.");
			ui.LState->setStyleSheet(DynExpUI::StatusBarErrorStyleSheet);
			break;
		default:
			ui.LState->setText(" Please start by dialing a wavelength.");
			ui.LState->setStyleSheet("");
		}
	}

	void LaserControlData::ResetImpl(dispatch_tag<QModuleDataBase>)
	{
		Init();
	}

	void LaserControlData::Init()
	{
		FrequencyUnit = DynExpInstr::LaserData::FrequencyUnitType::Hz;
		IntensityUnit = DynExpInstr::LaserData::IntensityUnitType::Power_W;
		HardwareMinFrequency = 0.0;
		HardwareMaxFrequency = 0.0;
		HardwareMinIntensity = 0.0;
		HardwareMaxIntensity = 0.0;
		HardwareMinBandwidth = 0.0;
		HardwareMaxBandwidth = 0.0;
		HardwareMaxRate = 0.0;
		HardwareModeHopFreeTuningRange = 0.0;
		Frequency = 0.0;
		Wavelength = 0.0;
		Intensity = 0.0;
		ScanRate = 0.1;
		ScanRange = 1.0;
		LaserState = DynExpInstr::LaserData::LaserStateType::Ready;

		UIInitialized = false;
	}

	Util::DynExpErrorCodes::DynExpErrorCodes LaserControl::ModuleMainLoop(DynExp::ModuleInstance& Instance)
	{
		try
		{
			auto ModuleData = DynExp::dynamic_ModuleData_cast<LaserControl>(Instance.ModuleDataGetter());
			auto InstrData = DynExp::dynamic_InstrumentData_cast<DynExpInstr::Laser>(ModuleData->GetLaser()->GetInstrumentData());

			ModuleData->Frequency = InstrData->GetFrequencyValue();
			ModuleData->Wavelength = DynExpInstr::LaserData::FrequencyWavelengthConversion(InstrData->GetFrequencyValue());
			ModuleData->Intensity = InstrData->GetIntensityValue();
			ModuleData->ScanRange = InstrData->GetScanRangeValue();
			ModuleData->ScanRate = InstrData->GetScanRateValue();
			ModuleData->LaserState = InstrData->GetLaserState();

			NumFailedUpdateAttempts = 0;
		} // ModuleData and instruments' data unlocked here.
		catch (const Util::TimeoutException& e)
		{
			if (NumFailedUpdateAttempts++ >= 3)
				Instance.GetOwner().SetWarning(e);
		}

		return Util::DynExpErrorCodes::NoError;
	}

	void LaserControl::ResetImpl(dispatch_tag<QModuleBase>)
	{
		NumFailedUpdateAttempts = 0;
	}

	std::unique_ptr<DynExp::QModuleWidget> LaserControl::MakeUIWidget()
	{
		auto Widget = std::make_unique<LaserControlWidget>(*this);

		Connect(Widget->GetUI().action_Enable, &QAction::triggered, this, &LaserControl::OnEnableClicked);
		Connect(Widget->GetUI().action_Disable, &QAction::triggered, this, &LaserControl::OnDisableClicked);
		Connect(Widget->GetUI().action_EnableScan, &QAction::toggled, this, &LaserControl::OnScanToggled);

		Connect(Widget->GetUI().SBFrequency, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &LaserControl::OnFrequencyValueChanged);
		Connect(Widget->GetUI().SBWavelength, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &LaserControl::OnWavelengthValueChanged);
		Connect(Widget->GetUI().SBIntensity, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &LaserControl::OnIntensityValueChanged);
		Connect(Widget->GetUI().SBScanRange, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &LaserControl::OnScanRangeValueChanged);
		Connect(Widget->GetUI().SBScanRate, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &LaserControl::OnScanRateValueChanged);

		return Widget;
	}

	void LaserControl::UpdateUIChild(const ModuleBase::ModuleDataGetterType& ModuleDataGetter)
	{
		auto Widget = GetWidget<LaserControlWidget>();
		auto ModuleData = DynExp::dynamic_ModuleData_cast<LaserControl>(ModuleDataGetter());

		if (!ModuleData->IsUIInitialized())
		{
			Widget->InitializeUI(ModuleData);
			ModuleData->SetUIInitialized();
		}

		Widget->UpdateUI(ModuleData);
	}

	void LaserControl::OnInit(DynExp::ModuleInstance* Instance) const
	{
		auto ModuleParams = DynExp::dynamic_Params_cast<LaserControl>(Instance->ParamsGetter());
		auto ModuleData = DynExp::dynamic_ModuleData_cast<LaserControl>(Instance->ModuleDataGetter());

		Instance->LockObject(ModuleParams->Laser, ModuleData->GetLaser());
		
		ModuleData->FrequencyUnit = ModuleData->GetLaser()->GetFrequencyUnit();
		ModuleData->IntensityUnit = ModuleData->GetLaser()->GetIntensityUnit();
		ModuleData->HardwareMinFrequency = ModuleData->GetLaser()->GetMinFrequency();
		ModuleData->HardwareMaxFrequency = ModuleData->GetLaser()->GetMaxFrequency();
		ModuleData->HardwareMinIntensity = ModuleData->GetLaser()->GetMinIntensity();
		ModuleData->HardwareMaxIntensity = ModuleData->GetLaser()->GetMaxIntensity();
		ModuleData->HardwareMinBandwidth = ModuleData->GetLaser()->GetMinBandwidth();
		ModuleData->HardwareMaxBandwidth = ModuleData->GetLaser()->GetMaxBandwidth();
		ModuleData->HardwareMaxRate = ModuleData->GetLaser()->GetMaxRate();
		ModuleData->HardwareModeHopFreeTuningRange = ModuleData->GetLaser()->GetModeHopFreeTuningRange();
	}

	void LaserControl::OnExit(DynExp::ModuleInstance* Instance) const
	{
		auto ModuleData = DynExp::dynamic_ModuleData_cast<LaserControl>(Instance->ModuleDataGetter());

		Instance->UnlockObject(ModuleData->GetLaser());
	}

	void LaserControl::OnEnableClicked(DynExp::ModuleInstance* Instance) const
	{
		auto ModuleData = DynExp::dynamic_ModuleData_cast<LaserControl>(Instance->ModuleDataGetter());

		ModuleData->GetLaser()->Enable();
	}

	void LaserControl::OnDisableClicked(DynExp::ModuleInstance* Instance) const
	{
		auto ModuleData = DynExp::dynamic_ModuleData_cast<LaserControl>(Instance->ModuleDataGetter());

		ModuleData->GetLaser()->Disable();
	}

	void LaserControl::OnScanToggled(DynExp::ModuleInstance* Instance) const
	{
		auto ModuleData = DynExp::dynamic_ModuleData_cast<LaserControl>(Instance->ModuleDataGetter());

		if (ModuleData->LaserState == DynExpInstr::LaserData::LaserStateType::EmissionEnabledScanning)
			ModuleData->GetLaser()->DisableScan();
		else
			ModuleData->GetLaser()->ScanContinuously();
	}

	void LaserControl::OnWavelengthValueChanged(DynExp::ModuleInstance* Instance, double Value) const
	{
		auto ModuleData = DynExp::dynamic_ModuleData_cast<LaserControl>(Instance->ModuleDataGetter());
		auto InstrData = DynExp::dynamic_InstrumentData_cast<DynExpInstr::Laser>(ModuleData->GetLaser()->GetInstrumentData());

		// unit conversion from nm to Hz
		const double Value_in_Laser_unit{ DynExpInstr::LaserData::FrequencyWavelengthConversion(Value) };
		if (Value_in_Laser_unit != DynExpInstr::LaserData::FrequencyWavelengthConversion(InstrData->GetFrequencyValue()))
			ModuleData->GetLaser()->SetFrequency(Value_in_Laser_unit);
	}

	void LaserControl::OnFrequencyValueChanged(DynExp::ModuleInstance* Instance, double Value) const
	{
		auto ModuleData = DynExp::dynamic_ModuleData_cast<LaserControl>(Instance->ModuleDataGetter());
		auto InstrData = DynExp::dynamic_InstrumentData_cast<DynExpInstr::Laser>(ModuleData->GetLaser()->GetInstrumentData());

		// unit conversion from THz to Hz
		const double Value_in_Laser_unit{ Value * 1e12 };
		if (Value_in_Laser_unit != InstrData->GetFrequencyValue())
			ModuleData->GetLaser()->SetFrequency(Value_in_Laser_unit);
	}

	void LaserControl::OnIntensityValueChanged(DynExp::ModuleInstance* Instance, double Value) const
	{
		auto ModuleData = DynExp::dynamic_ModuleData_cast<LaserControl>(Instance->ModuleDataGetter());
		auto InstrData = DynExp::dynamic_InstrumentData_cast<DynExpInstr::Laser>(ModuleData->GetLaser()->GetInstrumentData());

		// unit conversion from mW to W
		const double Value_in_Laser_unit{ Value * 1e-3 };
		if (Value_in_Laser_unit != InstrData->GetIntensityValue())
			ModuleData->GetLaser()->SetIntensity(Value_in_Laser_unit);
	}

	void LaserControl::OnScanRangeValueChanged(DynExp::ModuleInstance* Instance, double Value) const
	{
		auto ModuleData = DynExp::dynamic_ModuleData_cast<LaserControl>(Instance->ModuleDataGetter());
		auto InstrData = DynExp::dynamic_InstrumentData_cast<DynExpInstr::Laser>(ModuleData->GetLaser()->GetInstrumentData());

		// unit conversion from GHz to Hz
		const double Value_in_Laser_unit{ Value * 1e9 };
		if (Value_in_Laser_unit != InstrData->GetScanRangeValue())
			ModuleData->GetLaser()->SetScanRange(Value_in_Laser_unit);
	}

	void LaserControl::OnScanRateValueChanged(DynExp::ModuleInstance* Instance, double Value) const
	{
		auto ModuleData = DynExp::dynamic_ModuleData_cast<LaserControl>(Instance->ModuleDataGetter());
		auto InstrData = DynExp::dynamic_InstrumentData_cast<DynExpInstr::Laser>(ModuleData->GetLaser()->GetInstrumentData());

		// unit conversion from GHz to Hz
		const double Value_in_Laser_unit{ Value * 1e9 };
		if (Value_in_Laser_unit != InstrData->GetScanRateValue())
			ModuleData->GetLaser()->SetScanRate(Value_in_Laser_unit);
	}
}
