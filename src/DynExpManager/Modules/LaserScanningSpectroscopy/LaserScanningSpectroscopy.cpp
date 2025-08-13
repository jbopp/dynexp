// This file is part of DynExp.

#include "stdafx.h"
#include "moc_LaserScanningSpectroscopy.cpp"
#include "LaserScanningSpectroscopy.h"

namespace DynExpModule::LaserScanningSpectroscopy
{
	LaserScanningSpectroscopyWidget::LaserScanningSpectroscopyWidget(LaserScanningSpectroscopy& Owner, QModuleWidget* parent)
		: QModuleWidget(Owner, parent)
	{
		ui.setupUi(this);

		// For shortcuts
		this->addAction(ui.action_Start);
		this->addAction(ui.action_Stop);
	}

	void LaserScanningSpectroscopyWidget::InitializeUI(Util::SynchronizedPointer<LaserScanningSpectroscopyData>& ModuleData)
		{
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
		ui.SBRepetitions->setRange(1, 10000);
		ui.SBRepetitions->setValue(1);
		ui.SBStepsize->setRange(1, 4000);
		ui.SBStepsize->setSuffix(" M" + QString(DynExpInstr::LaserData::FrequencyUnitTypeToStr(ModuleData->GetLaser()->GetFrequencyUnit())));
		ui.SBStepsize->setValue(1000);
		ui.SBNumberOfSteps->setRange(1, 10000);
		ui.SBNumberOfSteps->setValue(ui.SBFrequencyRange->value() / ui.SBStepsize->value());
	}

	void LaserScanningSpectroscopyData::ResetImpl(dispatch_tag<QModuleDataBase>)
	{
		Init();
	}

	void LaserScanningSpectroscopyData::Init()
	{
		UIInitialized = false;
		LaserScanningSpectroscopyState = StateType::Ready;
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
		LaserScanningSpectroscopyProgress = 0.0;
	}

	LaserScanningSpectroscopy::LaserScanningSpectroscopy(const std::thread::id OwnerThreadID, DynExp::ParamsBasePtrType&& Params)
		: QModuleBase(OwnerThreadID, std::move(Params)),
		StateMachine(ReadyState, WaitForSettingFrequencyState, FrequencyStepState, WaitForCapturingState)
		//PauseUpdatingUI(std::make_shared<std::atomic<bool>>(false))
	{
	}

	LaserScanningSpectroscopy::~LaserScanningSpectroscopy()
	{
	}

	Util::DynExpErrorCodes::DynExpErrorCodes LaserScanningSpectroscopy::ModuleMainLoop(DynExp::ModuleInstance& Instance)
	{
		auto Widget = GetWidget<LaserScanningSpectroscopyWidget>();
		auto ModuleData = DynExp::dynamic_ModuleData_cast<LaserScanningSpectroscopy>(Instance->ModuleDataGetter());

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

	void LaserScanningSpectroscopy::ResetImpl(dispatch_tag<QModuleBase>)
	{
		StateMachine.SetCurrentState(StateType::Ready);

		NumFailedUpdateAttempts = 0;
	}

	std::unique_ptr<DynExp::QModuleWidget> LaserScanningSpectroscopy::MakeUIWidget()
	{
		auto Widget = std::make_unique<LaserScanningSpectroscopyWidget>(*this);

		Connect(Widget->GetUI().action_Start, &QAction::triggered, this, &LaserScanningSpectroscopy::OnStartClicked);
		Connect(Widget->GetUI().action_Stop, &QAction::triggered, this, &LaserScanningSpectroscopy::OnStopClicked);

		Connect(Widget->GetUI().SBLowerFrequencyLimit, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &LaserScanningSpectroscopy::OnLowerFrequencyLimitChanged);
		Connect(Widget->GetUI().SBUpperFrequencyLimit, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &LaserScanningSpectroscopy::OnUpperFrequencyLimitChanged);
		Connect(Widget->GetUI().SBFrequencyRange, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &LaserScanningSpectroscopy::OnFrequencyRangeChanged);
		Connect(Widget->GetUI().SBCenterFrequency, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &LaserScanningSpectroscopy::OnFrequencyCenterChanged);
		Connect(Widget->GetUI().SBStepsize, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &LaserScanningSpectroscopy::OnStepsizeChanged);
		Connect(Widget->GetUI().SBNumberOfSteps, QOverload<int>::of(&QSpinBox::valueChanged), this, &LaserScanningSpectroscopy::OnNumberOfStepsChanged);
		Connect(Widget->GetUI().SBRepetitions, QOverload<int>::of(&QSpinBox::valueChanged), this, &LaserScanningSpectroscopy::OnRepetitionsChanged);
		Connect(Widget->GetUI().RBStartAtMinimum, &QRadioButton::toggled, this, &LaserScanningSpectroscopy::OnStartAtMinimumToggled);
		Connect(Widget->GetUI().RBStartAtMaximum, &QRadioButton::toggled, this, &LaserScanningSpectroscopy::OnStartAtMaximumToggled);
		Connect(Widget->GetUI().CBScanBackAndForth, &QCheckBox::toggled, this, &LaserScanningSpectroscopy::OnScanBackAndForthToggled);
		Connect(Widget->GetUI().LEPath, &QLineEdit::textChanged, this, &LaserScanningSpectroscopy::OnPathChanged);

		return Widget;
	}

	void LaserScanningSpectroscopy::UpdateUIChild(const ModuleBase::ModuleDataGetterType& ModuleDataGetter)
	{ 
		auto Widget = GetWidget<LaserScanningSpectroscopyWidget>();
		auto ModuleData = DynExp::dynamic_ModuleData_cast<LaserScanningSpectroscopy>(ModuleDataGetter());

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
			Widget->ui.LLaserScanningSpectroscopyState->setText(" LaserScanningSpectroscopy state: Ready");
		else if (SettingFrequency)
			Widget->ui.LLaserScanningSpectroscopyState->setText(" LaserScanningSpectroscopy state: Stabilizing at target frequency");
		else if (Capturing)
			Widget->ui.LLaserScanningSpectroscopyState->setText(" LaserScanningSpectroscopy state: Capturing");
		else
			Widget->ui.LLaserScanningSpectroscopyState->setText(" LaserScanningSpectroscopy state: ");

		Widget->ui.PBLaserScanningSpectroscopyProgress->setVisible(ModuleData->LaserScanningSpectroscopyState != StateType::Ready
			&& ModuleData->LaserScanningSpectroscopyProgress > 0);
		Widget->ui.PBLaserScanningSpectroscopyProgress->setValue(ModuleData->LaserScanningSpectroscopyProgress > 0 ? Util::NumToT<int>(ModuleData->LaserScanningSpectroscopyProgress) : 0);
	}

	bool LaserScanningSpectroscopy::IsReadyState() const
	{
		const auto CurrentState = StateMachine.GetCurrentState()->GetState();

		return CurrentState == StateType::Ready;
	}

	bool LaserScanningSpectroscopy::IsSettingFrequencyState() const
	{
		const auto CurrentState = StateMachine.GetCurrentState()->GetState();

		return CurrentState == StateType::FrequencyStep ||
			CurrentState == StateType::WaitForSettingFrequency;
	}

	bool LaserScanningSpectroscopy::IsCapturingState() const
	{
		const auto CurrentState = StateMachine.GetCurrentState()->GetState();

		return CurrentState == StateType::WaitForCapturing;
	}
 
	void LaserScanningSpectroscopy::StartCapturing(Util::SynchronizedPointer<ModuleDataType>& ModuleData, const StartCapturingEvent& Event) const
	{
		// wir wollen irgendwie den file path mit schicken, damit capturing module weiß in welches csv file es schreiben soll
		// ModuleData->Filename;

		if (ModuleData->Communicator.valid())
			ModuleData->Communicator->PostEvent(*this, Event);
	}

	void LaserScanningSpectroscopy::OnInit(DynExp::ModuleInstance* Instance) const
	{
		auto ModuleParams = DynExp::dynamic_Params_cast<LaserScanningSpectroscopy>(Instance->ParamsGetter());
		auto ModuleData = DynExp::dynamic_ModuleData_cast<LaserScanningSpectroscopy>(Instance->ModuleDataGetter());

		Instance->LockObject(ModuleParams->Laser, ModuleData->GetLaser());

		auto LaserInstrData = DynExp::dynamic_InstrumentData_cast<DynExpInstr::Laser>(ModuleData->Laser->GetInstrumentData());
	}

	void LaserScanningSpectroscopy::OnExit(DynExp::ModuleInstance* Instance) const
	{
		auto ModuleData = DynExp::dynamic_ModuleData_cast<LaserScanningSpectroscopy>(Instance->ModuleDataGetter());

		Instance->UnlockObject(ModuleData->Laser);
		Instance->UnlockObject(ModuleData->Communicator);
	}

	void LaserScanningSpectroscopy::OnStartClicked(DynExp::ModuleInstance* Instance, bool) const
	{
		auto ModuleData = DynExp::dynamic_ModuleData_cast<LaserScanningSpectroscopy>(Instance->ModuleDataGetter());

		ModuleData->StepCount = 0;
		ModuleData->RepCount = 0;
		ModuleData->LaserScanningSpectroscopyProgress = 0;

		StateMachine.SetCurrentState(StateType::FrequencyStep);
	}

	void LaserScanningSpectroscopy::OnStopClicked(DynExp::ModuleInstance* Instance, bool) const
	{
		StateMachine.SetCurrentState(StateType::Ready);
	}

	void LaserScanningSpectroscopy::OnLowerFrequencyLimitChanged(DynExp::ModuleInstance* Instance, double LowerFrequencyLimit) const
	{
		auto ModuleData = DynExp::dynamic_ModuleData_cast<LaserScanningSpectroscopy>(Instance->ModuleDataGetter());
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
	
	void LaserScanningSpectroscopy::OnUpperFrequencyLimitChanged(DynExp::ModuleInstance* Instance, double UpperFrequencyLimit) const
	{
		auto ModuleData = DynExp::dynamic_ModuleData_cast<LaserScanningSpectroscopy>(Instance->ModuleDataGetter());
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
	
	void LaserScanningSpectroscopy::OnFrequencyRangeChanged(DynExp::ModuleInstance* Instance, double FrequencyRange) const
	{
		auto ModuleData = DynExp::dynamic_ModuleData_cast<LaserScanningSpectroscopy>(Instance->ModuleDataGetter());
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

	void LaserScanningSpectroscopy::OnFrequencyCenterChanged(DynExp::ModuleInstance* Instance, double FrequencyCenter) const
	{
		auto ModuleData = DynExp::dynamic_ModuleData_cast<LaserScanningSpectroscopy>(Instance->ModuleDataGetter());

		// modify LowerFrequencyLimit and UpperFrequencyLimit to match new FrequencyCenter
		ModuleData->UpperFrequencyLimit = FrequencyCenter + ModuleData->FrequencyRange/2;
		ModuleData->LowerFrequencyLimit = FrequencyCenter - ModuleData->FrequencyRange/2;
	}

	void LaserScanningSpectroscopy::OnStepsizeChanged(DynExp::ModuleInstance* Instance, double Stepsize) const
	{
		auto ModuleData = DynExp::dynamic_ModuleData_cast<LaserScanningSpectroscopy>(Instance->ModuleDataGetter());

		// modify NumberOfSteps to match new Stepsize
		ModuleData->NumberOfSteps = ModuleData->FrequencyRange / Stepsize;
	}

	void LaserScanningSpectroscopy::OnNumberOfStepsChanged(DynExp::ModuleInstance* Instance, int NumberOfSteps) const
	{
		auto ModuleData = DynExp::dynamic_ModuleData_cast<LaserScanningSpectroscopy>(Instance->ModuleDataGetter());

		// modify Stepsize to match new NumberOfSteps
		ModuleData->Stepsize = ModuleData->FrequencyRange / NumberOfSteps;
	}

	void LaserScanningSpectroscopy::OnRepetitionsChanged(DynExp::ModuleInstance* Instance, int Repetitions) const
	{
	}

	void LaserScanningSpectroscopy::OnStartAtMinimumToggled(DynExp::ModuleInstance* Instance) const
	{
		auto Widget = GetWidget<LaserScanningSpectroscopyWidget>();
		auto ModuleData = DynExp::dynamic_ModuleData_cast<LaserScanningSpectroscopy>(Instance->ModuleDataGetter());

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

	void LaserScanningSpectroscopy::OnStartAtMaximumToggled(DynExp::ModuleInstance* Instance) const
	{
		auto Widget = GetWidget<LaserScanningSpectroscopyWidget>();
		auto ModuleData = DynExp::dynamic_ModuleData_cast<LaserScanningSpectroscopy>(Instance->ModuleDataGetter());

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

	void LaserScanningSpectroscopy::OnScanBackAndForthToggled(DynExp::ModuleInstance* Instance) const
	{
		auto Widget = GetWidget<LaserScanningSpectroscopyWidget>();
		auto ModuleData = DynExp::dynamic_ModuleData_cast<LaserScanningSpectroscopy>(Instance->ModuleDataGetter());

		if (Widget->ui.CBScanBackAndForth->isChecked())
			ModuleData->ScanBackAndForth = true;
		else
			ModuleData->ScanBackAndForth = false;
	}
	
	void LaserScanningSpectroscopy::OnPathChanged(DynExp::ModuleInstance* Instance, QString Path) const
	{
		auto ModuleData = DynExp::dynamic_ModuleData_cast<LaserScanningSpectroscopy>(Instance->ModuleDataGetter());
		ModuleData->Filepath = Path.toStdString();
	}

	StateType LaserScanningSpectroscopy::ReadyStateFunc(DynExp::ModuleInstance& Instance)
	{
		auto ModuleData = DynExp::dynamic_ModuleData_cast<LaserScanningSpectroscopy>(Instance.ModuleDataGetter());
		auto LaserInstrData = DynExp::dynamic_InstrumentData_cast<DynExpInstr::Laser>(ModuleData->GetLaser()->GetInstrumentData());

		ModuleData->LaserState = LaserInstrData->GetLaserState();

		return StateType::Ready;
	}

	StateType LaserScanningSpectroscopy::WaitForSettingFrequencyStateFunc(DynExp::ModuleInstance& Instance)
	{
		auto ModuleData = DynExp::dynamic_ModuleData_cast<LaserScanningSpectroscopy>(Instance.ModuleDataGetter());
		auto LaserInstrData = DynExp::dynamic_InstrumentData_cast<DynExpInstr::Laser>(ModuleData->GetLaser()->GetInstrumentData());

		if (LaserInstrData->GetLaserState() == DynExpInstr::LaserData::LaserStateType::Ready ||
			LaserInstrData->GetLaserState() == DynExpInstr::LaserData::LaserStateType::EmissionEnabledConstant)
			//&& (ModuleData->LowerFrequencyLimit - ModuleData->FrequencyRange/2 < LaserInstrData->GetFrequencyValue() < ModuleData->UpperFrequencyLimit + ModuleData->FrequencyRange/2))
			{
			if (ModuleData->StepCount == 0)
				return StateType::FrequencyStep;
				else
					{
					//StartCapturing(ModuleData, StartCapturingEvent)
					return StateType::WaitForCapturing;
					}
			}
		else
			return StateType::WaitForSettingFrequency;
	}

	StateType LaserScanningSpectroscopy::FrequencyStepStateFunc(DynExp::ModuleInstance& Instance)
	{
		auto ModuleData = DynExp::dynamic_ModuleData_cast<LaserScanningSpectroscopy>(Instance.ModuleDataGetter());
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
		ModuleData->LaserScanningSpectroscopyProgress++;

		return StateType::WaitForSettingFrequency;
	}

	StateType LaserScanningSpectroscopy::WaitForCapturingStateFunc(DynExp::ModuleInstance& Instance)
	{
		auto ModuleData = DynExp::dynamic_ModuleData_cast<LaserScanningSpectroscopy>(Instance.ModuleDataGetter());

		if (ModuleData->StepCount < ModuleData->NumberOfSteps)
			return StateType::FrequencyStep;
		else
		{
			ModuleData->RepCount++;
			if (ModuleData->RepCount = ModuleData->Repetitions)
				return StateType::Ready;
			else
				return StateType::FrequencyStep;
		}
	}
}
