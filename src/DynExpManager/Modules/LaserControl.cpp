// This file is part of DynExp.

#include "stdafx.h"
#include "moc_LaserControl.cpp"
#include "ui_LaserControl.h"
#include "LaserControl.h"

namespace DynExpModule
{
	LaserControlWidget::LaserControlWidget(LaserControl& Owner, QModuleWidget* parent)
		: QModuleWidget(Owner, parent),
		ui(std::make_unique<Ui::LaserControl>())
	{
		ui->setupUi(this);

		// For shortcuts
		this->addAction(ui->action_Enable);
	}
	
	void LaserControlWidget::InitializeUI(Util::SynchronizedPointer<LaserControlData>& ModuleData)
	{
		const QSignalBlocker SBFrequencyBlocker(ui->SBFrequency);
		ui->SBFrequency->setRange(ModuleData->HardwareMinFrequency * 1e-12, ModuleData->HardwareMaxFrequency * 1e-12);
		ui->SBFrequency->setValue(ModuleData->HardwareMinFrequency * 1e-12);

		const QSignalBlocker SBWavelengthBlocker(ui->SBWavelength);
		ui->SBWavelength->setRange(Util::ConvertFrequencyWavelength(ModuleData->HardwareMaxFrequency) * 1e9, Util::ConvertFrequencyWavelength(ModuleData->HardwareMinFrequency) * 1e9);
		ui->SBWavelength->setValue(Util::ConvertFrequencyWavelength(ModuleData->HardwareMaxFrequency) * 1e9);

		const QSignalBlocker SBIntensityBlocker(ui->SBIntensity);
		ui->SBIntensity->setRange(ModuleData->HardwareMinIntensity * 1e3, ModuleData->HardwareMaxIntensity * 1e3);
		ui->SBIntensity->setValue(ModuleData->HardwareMinIntensity * 1e3);

		const QSignalBlocker SBScanRangeBlocker(ui->SBScanRange);
		ui->SBScanRange->setRange(ModuleData->HardwareMinScanRange * 1e-9, ModuleData->HardwareMaxScanRange * 1e-9);

		const QSignalBlocker SBScanRateBlocker(ui->SBScanRate);
		ui->SBScanRate->setRange(ModuleData->HardwareMinScanRate * 1e-9, ModuleData->HardwareMaxScanRate * 1e-9);

		ui->GBFrequencyWavelength->setVisible(ModuleData->HardwareMinFrequency != ModuleData->HardwareMaxFrequency);
		ui->GBIntensity->setVisible(ModuleData->HardwareMinIntensity != ModuleData->HardwareMaxIntensity);
		ui->GBScanSettings->setVisible(ModuleData->HardwareMinScanRange != ModuleData->HardwareMaxScanRange);
		ui->action_Scan->setEnabled(ModuleData->HardwareMinScanRange != ModuleData->HardwareMaxScanRange);
	}

	void LaserControlWidget::UpdateUI(Util::SynchronizedPointer<LaserControlData>& ModuleData)
	{
		if (std::isnan(ModuleData->CurrentFrequency))
		{
			ui->LActualFrequency->setText("Output unstable");
			ui->LActualWavelength->setText("Output unstable");
			ui->LActualFrequency->setStyleSheet(DynExpUI::StatusBarWarningStyleSheet);
			ui->LActualWavelength->setStyleSheet(DynExpUI::StatusBarWarningStyleSheet);
		}
		else
		{
			ui->LActualFrequency->setText(QString::number(ModuleData->CurrentFrequency * 1e-12, 'f', 6) + " THz");
			ui->LActualWavelength->setText(QString::number(Util::ConvertFrequencyWavelength(ModuleData->CurrentFrequency) * 1e9, 'f', 6) + " nm");
			ui->LActualFrequency->setStyleSheet("");
			ui->LActualWavelength->setStyleSheet("");
		}

		if (ModuleData->CurrentIntensity < ModuleData->HardwareMinIntensity)
		{
			ui->LActualIntensity->setText(QString::number(ModuleData->CurrentIntensity * 1e3, 'f', 3) + " mW (Power low)");
			ui->LActualIntensity->setStyleSheet(DynExpUI::StatusBarWarningStyleSheet);
		}
		else
		{
			ui->LActualIntensity->setText(QString::number(ModuleData->CurrentIntensity * 1e3, 'f', 3) + " mW");
			ui->LActualIntensity->setStyleSheet("");
		}

		if (!ui->SBScanRange->hasFocus())
		{
			const QSignalBlocker Blocker(ui->SBScanRange);
			ui->SBScanRange->setValue(ModuleData->CurrentScanRange * 1e-9);
		}

		if (!ui->SBScanRate->hasFocus())
		{
			const QSignalBlocker Blocker(ui->SBScanRate);
			ui->SBScanRate->setValue(ModuleData->CurrentScanRate * 1e-9);
		}

		switch (ModuleData->LaserState)
		{
		case DynExpInstr::LaserData::LaserStateType::Startup:
			ui->LState->setText(" Startup...");
			ui->LState->setStyleSheet(DynExpUI::StatusBarReadyStyleSheetBright);
			break;
		case DynExpInstr::LaserData::LaserStateType::EmissionEnabledConstant:
			ui->LState->setText(" Emitting in constant mode.");
			ui->LState->setStyleSheet(DynExpUI::StatusBarBusyStyleSheet);
			break; 
		case DynExpInstr::LaserData::LaserStateType::EmissionEnabledScanning:
			ui->LState->setText(" Emitting in scanning mode.");
			ui->LState->setStyleSheet(DynExpUI::StatusBarBusyStyleSheet);
			break;
		case DynExpInstr::LaserData::LaserStateType::Error:
			ui->LState->setText(" The Laser is in an error state.");
			ui->LState->setStyleSheet(DynExpUI::StatusBarErrorStyleSheet);
			break;
		default:
			ui->LState->setText(" Laser is ready for emission.");
			ui->LState->setStyleSheet(DynExpUI::StatusBarReadyStyleSheetBright);
		}
	}

	double LaserControlData::FrequencyInHzToLaserUnit(double Value) const
	{
		switch (FrequencyUnit)
		{
		case DynExp::Units::UnitType::Freq_Hz:
			return Value;
		case DynExp::Units::UnitType::Wavelength_nm:
			return Util::ConvertFrequencyWavelength(Value) * 1e9;
		default:
			throw Util::NotImplementedException("Cannot convert frequency in Hz to the unit required by the laser instrument.");
		}
	}

	double LaserControlData::FrequencyInLaserUnitToHz(double Value) const
	{
		switch (FrequencyUnit)
		{
		case DynExp::Units::UnitType::Freq_Hz:
			return Value;
		case DynExp::Units::UnitType::Wavelength_nm:
			return Util::ConvertFrequencyWavelength(Value * 1e-9);
		default:
			throw Util::NotImplementedException("Cannot convert frequency in unit required by the laser instrument to Hz.");
		}
	}

	double LaserControlData::IntensityInWToLaserUnit(double Value) const
	{
		switch (IntensityUnit)
		{
		case DynExp::Units::UnitType::Power_W:
			return Value;
		default:
			throw Util::NotImplementedException("Cannot convert intensity in W to the unit required by the laser instrument.");
		}
	}

	double LaserControlData::IntensityInLaserUnitToW(double Value) const
	{
		switch (IntensityUnit)
		{
		case DynExp::Units::UnitType::Power_W:
			return Value;
		default:
			throw Util::NotImplementedException("Cannot convert intensity in unit required by the laser instrument to W.");
		}
	}

	void LaserControlData::ResetImpl(dispatch_tag<QModuleDataBase>)
	{
		Init();
	}

	void LaserControlData::Init()
	{
		FrequencyUnit = DynExp::Units::UnitType::Freq_Hz;
		IntensityUnit = DynExp::Units::UnitType::Power_W;
		HardwareMinFrequency = 0.0;
		HardwareMaxFrequency = 0.0;
		HardwareMinIntensity = 0.0;
		HardwareMaxIntensity = 0.0;
		HardwareMinScanRange = 0.0;
		HardwareMaxScanRange = 0.0;
		HardwareMinScanRate = 0.0;
		HardwareMaxScanRate = 0.0;
		HardwareModeHopFreeTuningRange = 0.0;
		CurrentFrequency = 0.0;
		CurrentIntensity = 0.0;
		CurrentScanRate = 0.0;
		CurrentScanRange = 0.0;
		LaserState = DynExpInstr::LaserData::LaserStateType::Ready;

		UIInitialized = false;
	}

	Util::DynExpErrorCodes::DynExpErrorCodes LaserControl::ModuleMainLoop(DynExp::ModuleInstance& Instance)
	{
		try
		{
			auto ModuleData = DynExp::dynamic_ModuleData_cast<LaserControl>(Instance.ModuleDataGetter());
			auto InstrData = DynExp::dynamic_InstrumentData_cast<DynExpInstr::Laser>(ModuleData->GetLaser()->GetInstrumentData());

			ModuleData->CurrentFrequency = ModuleData->FrequencyInLaserUnitToHz(InstrData->GetFrequencyValue());
			ModuleData->CurrentIntensity = ModuleData->IntensityInLaserUnitToW(InstrData->GetIntensityValue());
			ModuleData->CurrentScanRange = ModuleData->FrequencyInLaserUnitToHz(InstrData->GetScanRangeValue());
			ModuleData->CurrentScanRate = ModuleData->FrequencyInLaserUnitToHz(InstrData->GetScanRateValue());
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

		Connect(Widget->GetUI()->action_Enable, &QAction::toggled, this, &LaserControl::OnEnableToggled);
		Connect(Widget->GetUI()->action_Scan, &QAction::toggled, this, &LaserControl::OnScanToggled);
		Connect(Widget->GetUI()->SBFrequency, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &LaserControl::OnFrequencyValueChanged);
		Connect(Widget->GetUI()->SBWavelength, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &LaserControl::OnWavelengthValueChanged);
		Connect(Widget->GetUI()->SBIntensity, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &LaserControl::OnIntensityValueChanged);
		Connect(Widget->GetUI()->SBScanRange, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &LaserControl::OnScanRangeValueChanged);
		Connect(Widget->GetUI()->SBScanRate, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &LaserControl::OnScanRateValueChanged);

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
		ModuleData->HardwareMinFrequency = ModuleData->FrequencyInLaserUnitToHz(ModuleData->GetLaser()->GetMinFrequency());
		ModuleData->HardwareMaxFrequency = ModuleData->FrequencyInLaserUnitToHz(ModuleData->GetLaser()->GetMaxFrequency());
		ModuleData->HardwareMinIntensity = ModuleData->IntensityInLaserUnitToW(ModuleData->GetLaser()->GetMinIntensity());
		ModuleData->HardwareMaxIntensity = ModuleData->IntensityInLaserUnitToW(ModuleData->GetLaser()->GetMaxIntensity());
		ModuleData->HardwareMinScanRange = ModuleData->FrequencyInLaserUnitToHz(ModuleData->GetLaser()->GetMinScanRange());
		ModuleData->HardwareMaxScanRange = ModuleData->FrequencyInLaserUnitToHz(ModuleData->GetLaser()->GetMaxScanRange());
		ModuleData->HardwareMinScanRate = ModuleData->FrequencyInLaserUnitToHz(ModuleData->GetLaser()->GetMinScanRate());
		ModuleData->HardwareMaxScanRate = ModuleData->FrequencyInLaserUnitToHz(ModuleData->GetLaser()->GetMaxScanRate());
		ModuleData->HardwareModeHopFreeTuningRange = ModuleData->FrequencyInLaserUnitToHz(ModuleData->GetLaser()->GetModeHopFreeTuningRange());
	}

	void LaserControl::OnExit(DynExp::ModuleInstance* Instance) const
	{
		auto ModuleData = DynExp::dynamic_ModuleData_cast<LaserControl>(Instance->ModuleDataGetter());

		Instance->UnlockObject(ModuleData->GetLaser());
	}

	void LaserControl::OnEnableToggled(DynExp::ModuleInstance* Instance, bool Checked) const
	{
		auto ModuleData = DynExp::dynamic_ModuleData_cast<LaserControl>(Instance->ModuleDataGetter());

		if (Checked)
			ModuleData->GetLaser()->Enable();
		else
			ModuleData->GetLaser()->Disable();
	}

	void LaserControl::OnScanToggled(DynExp::ModuleInstance* Instance, bool Checked) const
	{
		auto ModuleData = DynExp::dynamic_ModuleData_cast<LaserControl>(Instance->ModuleDataGetter());

		if (Checked)
			ModuleData->GetLaser()->ScanContinuously();
		else
			ModuleData->GetLaser()->DisableScan();
	}

	void LaserControl::OnWavelengthValueChanged(DynExp::ModuleInstance* Instance, double Value) const
	{
		auto ModuleData = DynExp::dynamic_ModuleData_cast<LaserControl>(Instance->ModuleDataGetter());

		const double FrequencyInHz = Util::ConvertFrequencyWavelength(Value * 1e-9);
		if (ModuleData->CurrentFrequency != FrequencyInHz)
			ModuleData->GetLaser()->SetFrequency(ModuleData->FrequencyInHzToLaserUnit(FrequencyInHz));
	}

	void LaserControl::OnFrequencyValueChanged(DynExp::ModuleInstance* Instance, double Value) const
	{
		auto ModuleData = DynExp::dynamic_ModuleData_cast<LaserControl>(Instance->ModuleDataGetter());

		const double FrequencyInHz = Value * 1e12;
		if (ModuleData->CurrentFrequency != FrequencyInHz)
			ModuleData->GetLaser()->SetFrequency(ModuleData->FrequencyInHzToLaserUnit(FrequencyInHz));
	}

	void LaserControl::OnIntensityValueChanged(DynExp::ModuleInstance* Instance, double Value) const
	{
		auto ModuleData = DynExp::dynamic_ModuleData_cast<LaserControl>(Instance->ModuleDataGetter());

		const double IntensityInW = Value * 1e-3;
		if (ModuleData->CurrentIntensity != IntensityInW)
			ModuleData->GetLaser()->SetIntensity(ModuleData->IntensityInWToLaserUnit(IntensityInW));
	}

	void LaserControl::OnScanRangeValueChanged(DynExp::ModuleInstance* Instance, double Value) const
	{
		auto ModuleData = DynExp::dynamic_ModuleData_cast<LaserControl>(Instance->ModuleDataGetter());

		const double FrequencyInHz = Value * 1e9;
		if (ModuleData->CurrentScanRange != FrequencyInHz)
			ModuleData->GetLaser()->SetScanRange(ModuleData->FrequencyInHzToLaserUnit(FrequencyInHz));
	}

	void LaserControl::OnScanRateValueChanged(DynExp::ModuleInstance* Instance, double Value) const
	{
		auto ModuleData = DynExp::dynamic_ModuleData_cast<LaserControl>(Instance->ModuleDataGetter());

		const double FrequencyInHz = Value * 1e9;
		if (ModuleData->CurrentScanRate != FrequencyInHz)
			ModuleData->GetLaser()->SetScanRate(ModuleData->FrequencyInHzToLaserUnit(FrequencyInHz));
	}
}