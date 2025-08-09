// This file is part of DynExp.

#include "stdafx.h"
#include "moc_PLE.cpp"
#include "PLE.h"

namespace DynExpModule::PLE
{
	PLEWidget::PLEWidget(PLE& Owner, QModuleWidget* parent)
		: QModuleWidget(Owner, parent)
	{
		ui.setupUi(this);

		// For shortcuts
		this->addAction(ui.action_Start);
		this->addAction(ui.action_Stop);
	}

	void PLEWidget::InitializeUI(Util::SynchronizedPointer<PLEData>& ModuleData)
		{
		//will ich Frequenz in THz oder GHz angeben?
		ui.SBLowerFrequencyLimit->setRange(ModuleData->GetLaser()->GetMinFrequency() * 1e-9, ModuleData->GetLaser()->GetMaxFrequency() * 1e-9);
		ui.SBLowerFrequencyLimit->setSuffix(" G" + QString(DynExpInstr::LaserData::FrequencyUnitTypeToStr(ModuleData->GetLaser()->GetFrequencyUnit())));
		ui.SBLowerFrequencyLimit->setValue(ModuleData->GetLaser()->GetMinFrequency() * 1e-9);
		ui.SBUpperFrequencyLimit->setRange(ModuleData->GetLaser()->GetMinFrequency() * 1e-9, ModuleData->GetLaser()->GetMaxFrequency() * 1e-9);
		ui.SBUpperFrequencyLimit->setSuffix(" G" + QString(DynExpInstr::LaserData::FrequencyUnitTypeToStr(ModuleData->GetLaser()->GetFrequencyUnit())));
		ui.SBUpperFrequencyLimit->setValue(ModuleData->GetLaser()->GetMinFrequency() * 1e-9 + ModuleData->GetLaser()->GetModeHopFreeTuningRange());
		ui.SBFrequencyRange->setRange(0.0, ModuleData->GetLaser()->GetModeHopFreeTuningRange());
		ui.SBFrequencyRange->setSuffix(" G" + QString(DynExpInstr::LaserData::FrequencyUnitTypeToStr(ModuleData->GetLaser()->GetFrequencyUnit())));
		ui.SBFrequencyRange->setValue(ModuleData->GetLaser()->GetModeHopFreeTuningRange());
		ui.SBCenterFrequency->setRange(ModuleData->GetLaser()->GetMinFrequency() * 1e-9 + 0.5 * ModuleData->GetLaser()->GetModeHopFreeTuningRange(), ModuleData->GetLaser()->GetMinFrequency() * 1e-9 - 0.5 * ModuleData->GetLaser()->GetModeHopFreeTuningRange());
		ui.SBCenterFrequency->setSuffix(" G" + QString(DynExpInstr::LaserData::FrequencyUnitTypeToStr(ModuleData->GetLaser()->GetFrequencyUnit())));
		ui.SBCenterFrequency->setValue(ModuleData->GetLaser()->GetMinFrequency() * 1e-9 + 0.5 * ModuleData->GetLaser()->GetModeHopFreeTuningRange());
		ui.SBRepetitions->setRange(1, 100);
		ui.SBRepetitions->setValue(1);
		ui.SBStepsize->setRange(0.4, 1.6);
		ui.SBStepsize->setSuffix(" G" + QString(DynExpInstr::LaserData::FrequencyUnitTypeToStr(ModuleData->GetLaser()->GetFrequencyUnit())));
		ui.SBStepsize->setValue(0.4);
		ui.SBNumberOfSteps->setRange(1, 100);
		ui.SBNumberOfSteps->setValue(ui.SBFrequencyRange->value() / ui.SBStepsize->value());
	}
	/*
	std::filesystem::path PLEData::GetAutoMeasureSavePath() const
	{
		return CurrentCellID.Valid ?
			(AutoMeasureSavePath.parent_path() / CurrentCellID.IDString / AutoMeasureSavePath.filename()) : AutoMeasureSavePath;
	}*/

	void PLEData::ResetImpl(dispatch_tag<QModuleDataBase>)
	{
		Init();
	}

	void PLEData::Init()
	{
		UIInitialized = false;
		PLEState = StateType::Ready;
		LowerFrequencyLimit = 0.0;
		UpperFrequencyLimit = 0.0;
		FrequencyRange = 0.0;
		CenterFrequency = 0.0;
		ModeHopFreeTuningRange = 0.0;
		Stepsize = 0.0;
		NumberOfSteps = 0.0;
		Repetitions = 0.0;
		StartingPoint = 0.0;
		EndingPoint = 0.0;
		ScanBackAndForth = false;
		StepCount = 0;
		RepCount = 0;
		PLEProgress = 0.0;
		//AutoMeasureSavePath.clear();
	}

	/*PLE::PLE(const std::thread::id OwnerThreadID, DynExp::ParamsBasePtrType&& Params)
		: QModuleBase(OwnerThreadID, std::move(Params)),
		StateMachine(ReadyState, WaitForSettingFrequencyState, PLEStepState, WaitForCapturingState),
		PauseUpdatingUI(std::make_shared<std::atomic<bool>>(false))
	{
	}*/

	PLE::~PLE()
	{
	}

	Util::DynExpErrorCodes::DynExpErrorCodes PLE::ModuleMainLoop(DynExp::ModuleInstance& Instance)
	{
		try
		{
			StateMachine.Invoke(*this, Instance);

			NumFailedUpdateAttempts = 0;
		} // ModuleData and instruments' data unlocked here.

		catch (const Util::TimeoutException& e)
		{
			if (NumFailedUpdateAttempts++ >= 3)
				Instance.GetOwner().SetWarning(e);
		}

		return Util::DynExpErrorCodes::NoError;
	}

	void PLE::ResetImpl(dispatch_tag<QModuleBase>)
	{
		StateMachine.SetCurrentState(StateType::Ready);

		NumFailedUpdateAttempts = 0;
	}

	std::unique_ptr<DynExp::QModuleWidget> PLE::MakeUIWidget()
	{
		auto Widget = std::make_unique<PLEWidget>(*this);

		Connect(Widget->GetUI().action_Start, &QAction::triggered, this, &PLE::OnStartClicked);
		Connect(Widget->GetUI().action_Stop, &QAction::triggered, this, &PLE::OnStopClicked);

		Connect(Widget->GetUI().SBLowerFrequencyLimit, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &PLE::OnLowerFrequencyLimitChanged);
		Connect(Widget->GetUI().SBUpperFrequencyLimit, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &PLE::OnUpperFrequencyLimitChanged);
		Connect(Widget->GetUI().SBFrequencyRange, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &PLE::OnFrequencyRangeChanged);
		Connect(Widget->GetUI().SBCenterFrequency, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &PLE::OnFrequencyCenterChanged);
		Connect(Widget->GetUI().SBStepsize, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &PLE::OnStepsizeChanged);
		Connect(Widget->GetUI().SBNumberOfSteps, QOverload<int>::of(&QSpinBox::valueChanged), this, &PLE::OnNumberOfStepsChanged);
		Connect(Widget->GetUI().SBRepetitions, QOverload<int>::of(&QSpinBox::valueChanged), this, &PLE::OnRepetitionsChanged);
		Connect(Widget->GetUI().RBStartAtMinimum, &QRadioButton::toggled, this, &PLE::OnStartAtMinimumToggled);
		Connect(Widget->GetUI().RBStartAtMaximum, &QRadioButton::toggled, this, &PLE::OnStartAtMaximumToggled);
		Connect(Widget->GetUI().CBScanBackAndForth, &QCheckBox::toggled, this, &PLE::OnScanBackAndForthToggled);
		//Connect(Widget->GetUI().LEAutoMeasureSavePath, &QLineEdit::textChanged, this, &PLE::OnAutoMeasureSavePathChanged);

		return Widget;
	}

	void PLE::UpdateUIChild(const ModuleBase::ModuleDataGetterType& ModuleDataGetter)
	{ 
		auto Widget = GetWidget<PLEWidget>();
		auto ModuleData = DynExp::dynamic_ModuleData_cast<PLE>(ModuleDataGetter());

		if (!ModuleData->IsUIInitialized())
		{
			Widget->InitializeUI(ModuleData);
			ModuleData->SetUIInitialized();
		}

		const bool Ready = IsReadyState();
		const bool SettingFrequency = IsSettingFrequencyState();
		const bool Capturing = IsCapturingState();

		Widget->ui.action_Start->setEnabled(Ready && ModuleData->LaserState != DynExpInstr::LaserData::LaserStateType::Startup); //|| ModuleData->LaserState = DynExpInstr::LaserData::LaserStateType::EmissionEnabledScanning));
		Widget->ui.action_Stop->setEnabled(!Ready);

		Widget->ui.SBLowerFrequencyLimit->setEnabled(Ready);
		Widget->ui.SBUpperFrequencyLimit->setEnabled(Ready);
		Widget->ui.SBFrequencyRange->setEnabled(Ready);
		Widget->ui.SBCenterFrequency->setEnabled(Ready);
		Widget->ui.SBStepsize->setEnabled(Ready);
		Widget->ui.SBNumberOfSteps->setEnabled(Ready);
		Widget->ui.RBStartAtMinimum->setEnabled(Ready);
		Widget->ui.RBStartAtMaximum->setEnabled(Ready);
		Widget->ui.CBScanBackAndForth->setEnabled(Ready);
		Widget->ui.SBRepetitions->setEnabled(Ready);

		if (Ready)
			Widget->ui.LPLEState->setText(" PLE state: Ready");
		else if (SettingFrequency)
			Widget->ui.LPLEState->setText(" PLE state: Stabilizing at target frequency");
		else if (Capturing)
			Widget->ui.LPLEState->setText(" PLE state: Capturing");
		else
			Widget->ui.LPLEState->setText(" PLE state: ");

		Widget->ui.PBProgress->setVisible(ModuleData->PLEState != StateType::Ready
			&& ModuleData->PLEProgress > 0);
		Widget->ui.PBProgress->setValue(ModuleData->PLEProgress > 0 ? Util::NumToT<int>(ModuleData->PLEProgress) : 0);
	}

	bool PLE::IsReadyState() const
	{
		const auto CurrentState = StateMachine.GetCurrentState()->GetState();

		return CurrentState == StateType::Ready;
	}

	bool PLE::IsSettingFrequencyState() const
	{
		const auto CurrentState = StateMachine.GetCurrentState()->GetState();

		return CurrentState == StateType::PLEStep ||
			CurrentState == StateType::LaserInit ||
			CurrentState == StateType::WaitForSettingFrequency;
	}

	bool PLE::IsCapturingState() const
	{
		const auto CurrentState = StateMachine.GetCurrentState()->GetState();

		return CurrentState == StateType::WaitForCapturing;
	}

	/* 
	void PLE::StartCapturing(Util::SynchronizedPointer<ModuleDataType>& ModuleData, const StartCapturingEvent& Event) const
	{
		// wir wollen irgendwie den file path mit schicken, damit capturing module weiß in welches csv file es schreiben soll
		// std::filesystem::path Filename;

		if (ModuleData->Communicator.valid())
			ModuleData->Communicator->PostEvent(*this, Event);
	}
	*/

	void PLE::OnInit(DynExp::ModuleInstance* Instance) const
	{
		auto ModuleParams = DynExp::dynamic_Params_cast<PLE>(Instance->ParamsGetter());
		auto ModuleData = DynExp::dynamic_ModuleData_cast<PLE>(Instance->ModuleDataGetter());

		Instance->LockObject(ModuleParams->Laser, ModuleData->GetLaser());

		auto LaserInstrData = DynExp::dynamic_InstrumentData_cast<DynExpInstr::Laser>(ModuleData->Laser->GetInstrumentData());
	}

	void PLE::OnExit(DynExp::ModuleInstance* Instance) const
	{
		auto ModuleData = DynExp::dynamic_ModuleData_cast<PLE>(Instance->ModuleDataGetter());

		Instance->UnlockObject(ModuleData->Laser);
		Instance->UnlockObject(ModuleData->Communicator);
	}

	void PLE::OnStartClicked(DynExp::ModuleInstance* Instance, bool) const
	{
		auto Widget = GetWidget<PLEWidget>();
		auto ModuleData = DynExp::dynamic_ModuleData_cast<PLE>(Instance->ModuleDataGetter());

		ModuleData->LowerFrequencyLimit = Widget->ui.SBLowerFrequencyLimit->value() * 1e9;
		ModuleData->UpperFrequencyLimit = Widget->ui.SBUpperFrequencyLimit->value() * 1e9;
		ModuleData->FrequencyRange = Widget->ui.SBFrequencyRange->value() * 1e9;
		ModuleData->CenterFrequency = Widget->ui.SBCenterFrequency->value() * 1e9;
		ModuleData->Stepsize = Widget->ui.SBStepsize->value() * 1e9;
		ModuleData->NumberOfSteps = Widget->ui.SBNumberOfSteps->value();
		ModuleData->Repetitions = Widget->ui.SBRepetitions->value();
		ModuleData->StartingPoint = Widget->ui.RBStartAtMinimum->isChecked() ? ModuleData->LowerFrequencyLimit : ModuleData->UpperFrequencyLimit * 1e9;
		ModuleData->EndingPoint = Widget->ui.RBStartAtMinimum->isChecked() ? ModuleData->UpperFrequencyLimit : ModuleData->LowerFrequencyLimit * 1e9;
		ModuleData->ScanBackAndForth = Widget->ui.CBScanBackAndForth->isChecked();
		ModuleData->StepCount = 0;
		ModuleData->RepCount = 0;
		ModuleData->PLEProgress = 0;

		StateMachine.SetCurrentState(StateType::LaserInit);
	}

	void PLE::OnStopClicked(DynExp::ModuleInstance* Instance, bool) const
	{
		StateMachine.SetCurrentState(StateType::Ready);
	}

	void PLE::OnLowerFrequencyLimitChanged(DynExp::ModuleInstance* Instance, double LowerFrequencyLimit) const
	{
		auto ModuleData = DynExp::dynamic_ModuleData_cast<PLE>(Instance->ModuleDataGetter());
		double NewFrequencyRange{};

		// modify FrequencyRange, FrequencyCenter and UpperFrequencyLimit to match new LowerFrequencyLimit
		if (ModuleData->UpperFrequencyLimit - LowerFrequencyLimit <= ModuleData->ModeHopFreeTuningRange)
			NewFrequencyRange = ModuleData->UpperFrequencyLimit - LowerFrequencyLimit;
		else
		{
			NewFrequencyRange = ModuleData->ModeHopFreeTuningRange;
			ModuleData->UpperFrequencyLimit = LowerFrequencyLimit + NewFrequencyRange;
		}
		ModuleData->CenterFrequency = LowerFrequencyLimit + NewFrequencyRange/2;
		ModuleData->FrequencyRange = NewFrequencyRange;
	}	
	
	void PLE::OnUpperFrequencyLimitChanged(DynExp::ModuleInstance* Instance, double UpperFrequencyLimit) const
	{
		auto ModuleData = DynExp::dynamic_ModuleData_cast<PLE>(Instance->ModuleDataGetter());
		double NewFrequencyRange;

		// modify FrequencyRange, FrequencyCenter and LowerFrequencyLimit to match new UpperFrequencyLimit
		if (UpperFrequencyLimit - ModuleData->LowerFrequencyLimit <= ModuleData->ModeHopFreeTuningRange)
			NewFrequencyRange = UpperFrequencyLimit - ModuleData->LowerFrequencyLimit;
		else
		{
			NewFrequencyRange = ModuleData->ModeHopFreeTuningRange;
			ModuleData->LowerFrequencyLimit = UpperFrequencyLimit - NewFrequencyRange;
		}

		ModuleData->CenterFrequency = UpperFrequencyLimit - NewFrequencyRange/2;
		ModuleData->FrequencyRange = NewFrequencyRange;
	}
	
	void PLE::OnFrequencyRangeChanged(DynExp::ModuleInstance* Instance, double FrequencyRange) const
	{
		auto ModuleData = DynExp::dynamic_ModuleData_cast<PLE>(Instance->ModuleDataGetter());
		double NewFrequencyRange;

		// modify FrequencyCenter and UpperFrequencyLimit to match new FrequencyRange
		if (FrequencyRange <= ModuleData->ModeHopFreeTuningRange)
			NewFrequencyRange = FrequencyRange;
		else
		{
			NewFrequencyRange = ModuleData->ModeHopFreeTuningRange;
		}

		ModuleData->UpperFrequencyLimit = ModuleData->LowerFrequencyLimit + NewFrequencyRange;
		ModuleData->CenterFrequency = ModuleData->LowerFrequencyLimit + NewFrequencyRange/2;
		ModuleData->FrequencyRange = NewFrequencyRange;
	}

	void PLE::OnFrequencyCenterChanged(DynExp::ModuleInstance* Instance, double FrequencyCenter) const
	{
		auto ModuleData = DynExp::dynamic_ModuleData_cast<PLE>(Instance->ModuleDataGetter());

		// modify LowerFrequencyLimit and UpperFrequencyLimit to match new FrequencyCenter
		ModuleData->UpperFrequencyLimit = FrequencyCenter + ModuleData->FrequencyRange/2;
		ModuleData->LowerFrequencyLimit = FrequencyCenter - ModuleData->FrequencyRange/2;
	}

	void PLE::OnStepsizeChanged(DynExp::ModuleInstance* Instance, double Stepsize) const
	{
		auto ModuleData = DynExp::dynamic_ModuleData_cast<PLE>(Instance->ModuleDataGetter());

		// modify NumberOfSteps to match new Stepsize
		ModuleData->NumberOfSteps = ModuleData->FrequencyRange / Stepsize;
	}

	void PLE::OnNumberOfStepsChanged(DynExp::ModuleInstance* Instance, int NumberOfSteps) const
	{
		auto ModuleData = DynExp::dynamic_ModuleData_cast<PLE>(Instance->ModuleDataGetter());

		// modify Stepsize to match new NumberOfSteps
		ModuleData->Stepsize = ModuleData->FrequencyRange / NumberOfSteps;
	}

	void PLE::OnRepetitionsChanged(DynExp::ModuleInstance* Instance, int Repetitions) const
	{
	}

	void PLE::OnStartAtMinimumToggled(DynExp::ModuleInstance* Instance) const
	{
		auto Widget = GetWidget<PLEWidget>();
		auto ModuleData = DynExp::dynamic_ModuleData_cast<PLE>(Instance->ModuleDataGetter());

		if (Widget->ui.RBStartAtMinimum->isChecked())
		{
			Widget->ui.RBStartAtMaximum->setChecked(false);
			ModuleData->StartingPoint = ModuleData->LowerFrequencyLimit;
			ModuleData->EndingPoint = ModuleData->UpperFrequencyLimit;
		}
		else
		{
			Widget->ui.RBStartAtMaximum->setChecked(true);
			ModuleData->StartingPoint = ModuleData->UpperFrequencyLimit;
			ModuleData->EndingPoint = ModuleData->LowerFrequencyLimit;
		}
	}

	void PLE::OnStartAtMaximumToggled(DynExp::ModuleInstance* Instance) const
	{
		auto Widget = GetWidget<PLEWidget>();
		auto ModuleData = DynExp::dynamic_ModuleData_cast<PLE>(Instance->ModuleDataGetter());

		if (Widget->ui.RBStartAtMaximum->isChecked())
		{
			Widget->ui.RBStartAtMinimum->setChecked(false);
			ModuleData->StartingPoint = ModuleData->UpperFrequencyLimit;
			ModuleData->EndingPoint = ModuleData->LowerFrequencyLimit;
		}
		else
		{
			Widget->ui.RBStartAtMinimum->setChecked(true);
			ModuleData->StartingPoint = ModuleData->LowerFrequencyLimit;
			ModuleData->EndingPoint = ModuleData->UpperFrequencyLimit;
		}
	}

	void PLE::OnScanBackAndForthToggled(DynExp::ModuleInstance* Instance) const
	{
		auto Widget = GetWidget<PLEWidget>();
		auto ModuleData = DynExp::dynamic_ModuleData_cast<PLE>(Instance->ModuleDataGetter());

		if (Widget->ui.CBScanBackAndForth->isChecked())
			ModuleData->ScanBackAndForth = true;
		else
			ModuleData->ScanBackAndForth = false;
	}
	/*
	void PLE::OnAutoMeasureSavePathChanged(DynExp::ModuleInstance* Instance, QString Path) const
	{
		auto ModuleData = DynExp::dynamic_ModuleData_cast<PLE>(Instance->ModuleDataGetter());
		ModuleData->SetAutoMeasureSavePath(Path.toStdString());
	}*/

	StateType PLE::ReadyStateFunc(DynExp::ModuleInstance& Instance)
	{
		auto ModuleData = DynExp::dynamic_ModuleData_cast<PLE>(Instance.ModuleDataGetter());
		auto LaserInstrData = DynExp::dynamic_InstrumentData_cast<DynExpInstr::Laser>(ModuleData->GetLaser()->GetInstrumentData());

		ModuleData->LaserState = LaserInstrData->GetLaserState();

		return StateType::Ready;
	}

	StateType PLE::LaserInitStateFunc(DynExp::ModuleInstance& Instance)
	{
		auto ModuleData = DynExp::dynamic_ModuleData_cast<PLE>(Instance.ModuleDataGetter());
		ModuleData->GetLaser()->SetFrequency(ModuleData->StartingPoint);

		return StateType::WaitForSettingFrequency;
	}

	StateType PLE::WaitForSettingFrequencyStateFunc(DynExp::ModuleInstance& Instance)
	{
		auto ModuleData = DynExp::dynamic_ModuleData_cast<PLE>(Instance.ModuleDataGetter());
		auto LaserInstrData = DynExp::dynamic_InstrumentData_cast<DynExpInstr::Laser>(ModuleData->GetLaser()->GetInstrumentData());

		if (LaserInstrData->GetLaserState() == DynExpInstr::LaserData::LaserStateType::Ready ||
			LaserInstrData->GetLaserState() == DynExpInstr::LaserData::LaserStateType::EmissionEnabledConstant)
			//&& (ModuleData->LowerFrequencyLimit - ModuleData->FrequencyRange/2 < LaserInstrData->GetFrequencyValue() < ModuleData->UpperFrequencyLimit + ModuleData->FrequencyRange/2))
			{
			if (ModuleData->StepCount == 0)
				return StateType::PLEStep;
				else
					{
					//StartCapturing(ModuleData, StartCapturingEvent)
					return StateType::WaitForCapturing;
					}
			}
		else
			return StateType::WaitForSettingFrequency;
	}

	StateType PLE::PLEStepStateFunc(DynExp::ModuleInstance& Instance)
	{
		auto ModuleData = DynExp::dynamic_ModuleData_cast<PLE>(Instance.ModuleDataGetter());
		double Frequency = 0.0;

		if (ModuleData->StepCount == 0)
			Frequency = ModuleData->StartingPoint;
		else if (ModuleData->StepCount != 0 && ModuleData->StepCount < ModuleData->NumberOfSteps)
			{
			if ((ModuleData->StartingPoint == ModuleData->LowerFrequencyLimit && ModuleData->RepCount % 2 == 0) ||
    			(ModuleData->StartingPoint == ModuleData->UpperFrequencyLimit && ModuleData->RepCount % 2 == 1))
				Frequency = ModuleData->LowerFrequencyLimit + ModuleData->StepCount * ModuleData->Stepsize;
			else if ((ModuleData->StartingPoint == ModuleData->LowerFrequencyLimit && ModuleData->RepCount % 2 == 1) ||
    				(ModuleData->StartingPoint == ModuleData->UpperFrequencyLimit && ModuleData->RepCount % 2 == 0))
					Frequency = ModuleData->UpperFrequencyLimit - ModuleData->StepCount * ModuleData->Stepsize;
			}
		else if (ModuleData->StepCount > ModuleData->NumberOfSteps)
			{
			ModuleData->StepCount = 0;

			if (!ModuleData->ScanBackAndForth)
				Frequency = ModuleData->StartingPoint;
			else
				{
				if (ModuleData->RepCount % 2 == 0)
					Frequency = ModuleData->StartingPoint;
				else
					Frequency = ModuleData->EndingPoint;
				}
			}

		ModuleData->GetLaser()->SetFrequency(Frequency);
		ModuleData->StepCount++;
		ModuleData->PLEProgress++;

		return StateType::WaitForSettingFrequency;
	}

	StateType PLE::WaitForCapturingStateFunc(DynExp::ModuleInstance& Instance)
	{
		// abfragen ob "Finished Capturing" event angekommen ist. Wenn angekommen dann:
	
		auto ModuleData = DynExp::dynamic_ModuleData_cast<PLE>(Instance.ModuleDataGetter());

		if (ModuleData->StepCount < ModuleData->NumberOfSteps)
			return StateType::PLEStep;
		else
		{
			ModuleData->RepCount++;
			if (ModuleData->RepCount = ModuleData->Repetitions)
				return StateType::Ready;
			else
				return StateType::PLEStep;
		}
	}

	/*std::filesystem::path PLE::BuildFilename(Util::SynchronizedPointer<ModuleDataType>& ModuleData, std::string_view FilenameSuffix) const
    {
        auto SavePath = ModuleData->GetAutoMeasureSavePath();
        SavePath.replace_filename(SavePath.filename().stem().concat(FilenameSuffix));
        std::filesystem::create_directories(SavePath.parent_path());
 
        return SavePath;
    }*/
}
