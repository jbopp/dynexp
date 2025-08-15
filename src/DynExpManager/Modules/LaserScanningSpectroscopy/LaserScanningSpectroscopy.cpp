// This file is part of DynExp.

#include "stdafx.h"
#include "moc_LaserScanningSpectroscopy.cpp"
#include "LaserScanningSpectroscopy.h"

#include <QDir>

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
		const QSignalBlocker b1(ui.SBLowerFrequencyLimit);
		const QSignalBlocker b2(ui.SBUpperFrequencyLimit);
		const QSignalBlocker b3(ui.SBFrequencyRange);
		const QSignalBlocker b4(ui.SBCenterFrequency);
		const QSignalBlocker b5(ui.SBRepetitions);
		const QSignalBlocker b6(ui.SBStepsize);
		const QSignalBlocker b7(ui.SBNumberOfSteps);

		ui.SBLowerFrequencyLimit->setRange(ModuleData->GetLaser()->GetMinFrequency() * 1e-9, ModuleData->GetLaser()->GetMaxFrequency() * 1e-9);
		ui.SBLowerFrequencyLimit->setSuffix(" G" + QString(DynExpInstr::LaserData::FrequencyUnitTypeToStr(ModuleData->GetLaser()->GetFrequencyUnit())));
		ui.SBLowerFrequencyLimit->setValue(ModuleData->GetLaser()->GetMinFrequency() * 1e-9);
		ui.SBUpperFrequencyLimit->setRange(ModuleData->GetLaser()->GetMinFrequency() * 1e-9, ModuleData->GetLaser()->GetMaxFrequency() * 1e-9);
		ui.SBUpperFrequencyLimit->setSuffix(" G" + QString(DynExpInstr::LaserData::FrequencyUnitTypeToStr(ModuleData->GetLaser()->GetFrequencyUnit())));
		ui.SBUpperFrequencyLimit->setValue(ModuleData->GetLaser()->GetMinFrequency() * 1e-9 + ModuleData->GetLaser()->GetModeHopFreeTuningRange() * 1e-9);
		ui.SBFrequencyRange->setRange(0.0, ModuleData->GetLaser()->GetModeHopFreeTuningRange() * 1e-9);
		ui.SBFrequencyRange->setSuffix(" G" + QString(DynExpInstr::LaserData::FrequencyUnitTypeToStr(ModuleData->GetLaser()->GetFrequencyUnit())));
		ui.SBFrequencyRange->setValue(ModuleData->GetLaser()->GetModeHopFreeTuningRange() * 1e-9);
		ui.SBCenterFrequency->setRange(ModuleData->GetLaser()->GetMinFrequency() * 1e-9 + 0.5 * ModuleData->GetLaser()->GetModeHopFreeTuningRange() * 1e-9, ModuleData->GetLaser()->GetMinFrequency() * 1e-9 - 0.5 * ModuleData->GetLaser()->GetModeHopFreeTuningRange() * 1e-9);
		ui.SBCenterFrequency->setSuffix(" G" + QString(DynExpInstr::LaserData::FrequencyUnitTypeToStr(ModuleData->GetLaser()->GetFrequencyUnit())));
		ui.SBCenterFrequency->setValue(ModuleData->GetLaser()->GetMinFrequency() * 1e-9 + 0.5 * ModuleData->GetLaser()->GetModeHopFreeTuningRange() * 1e-9);
		ui.SBRepetitions->setRange(1, 10000);
		ui.SBRepetitions->setValue(1);
		ui.SBStepsize->setRange(1, 4000);
		ui.SBStepsize->setSuffix(" M" + QString(DynExpInstr::LaserData::FrequencyUnitTypeToStr(ModuleData->GetLaser()->GetFrequencyUnit())));
		ui.SBStepsize->setValue(1000);
		ui.SBNumberOfSteps->setRange(1, 10000);
		ui.SBNumberOfSteps->setValue(ui.SBFrequencyRange->value() / ui.SBStepsize->value());

		ModuleData->LowerFrequencyLimit = ui.SBLowerFrequencyLimit->value() * 1e9; 
		ModuleData->UpperFrequencyLimit = ui.SBUpperFrequencyLimit->value() * 1e9;
		ModuleData->FrequencyRange = ui.SBFrequencyRange->value() * 1e9; 
		ModuleData->CenterFrequency = ui.SBCenterFrequency->value() * 1e9; 
		ModuleData->Stepsize = ui.SBStepsize->value() * 1e9; 
		ModuleData->NumberOfSteps = ui.SBNumberOfSteps->value(); 
		ModuleData->Repetitions = ui.SBRepetitions->value(); 
		ModuleData->StartingPoint = ui.RBStartAtMinimum->isChecked() ? 
		ModuleData->LowerFrequencyLimit : ModuleData->UpperFrequencyLimit * 1e9; 
		ModuleData->EndingPoint = ui.RBStartAtMinimum->isChecked() ? 
		ModuleData->UpperFrequencyLimit : ModuleData->LowerFrequencyLimit * 1e9; 
		ModuleData->ScanBackAndForth = ui.CBScanBackAndForth->isChecked();
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
		StateMachine(ReadyState, WaitForSettingFrequencyState, WaitForCapturingState)
		//PauseUpdatingUI(std::make_shared<std::atomic<bool>>(false))
	{
	}

	LaserScanningSpectroscopy::~LaserScanningSpectroscopy()
	{
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
		Connect(Widget->GetUI().LEPath, &QLineEdit::textChanged, this, &LaserScanningSpectroscopy::OnPath);
		Connect(Widget->GetUI().BPathBrowse, &QPushButton::clicked, this, &LaserScanningSpectroscopy::OnPathBrowseClicked);

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
	
		if (Widget->ui.RBStartAtMinimum->isChecked())
		{
			ModuleData->StartingPoint = ModuleData->LowerFrequencyLimit;
			ModuleData->EndingPoint = ModuleData->UpperFrequencyLimit;
		}
		else if (Widget->ui.RBStartAtMaximum->isChecked())
		{
			ModuleData->EndingPoint = ModuleData->LowerFrequencyLimit;
			ModuleData->StartingPoint = ModuleData->UpperFrequencyLimit;
		}

		if (Widget->ui.CBScanBackAndForth->isChecked())
			ModuleData->ScanBackAndForth = true;
		else if (!Widget->ui.CBScanBackAndForth->isChecked())
			ModuleData->ScanBackAndForth = false;
	}

	Util::DynExpErrorCodes::DynExpErrorCodes LaserScanningSpectroscopy::ModuleMainLoop(DynExp::ModuleInstance& Instance)
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

	bool LaserScanningSpectroscopy::IsReadyState() const
	{
		const auto CurrentState = StateMachine.GetCurrentState()->GetState();

		return CurrentState == StateType::Ready;
	}

	bool LaserScanningSpectroscopy::IsSettingFrequencyState() const
	{
		const auto CurrentState = StateMachine.GetCurrentState()->GetState();

		return CurrentState == StateType::WaitForSettingFrequency;
	}

	bool LaserScanningSpectroscopy::IsCapturingState() const
	{
		const auto CurrentState = StateMachine.GetCurrentState()->GetState();

		return CurrentState == StateType::WaitForCapturing;
	}

	void LaserScanningSpectroscopy::OnInit(DynExp::ModuleInstance* Instance) const
	{

		auto ModuleParams = DynExp::dynamic_Params_cast<LaserScanningSpectroscopy>(Instance->ParamsGetter());
		auto ModuleData = DynExp::dynamic_ModuleData_cast<LaserScanningSpectroscopy>(Instance->ModuleDataGetter());

		if (ModuleParams->PLECommunicator.ContainsID())
		{
			Instance->LockObject(ModuleParams->PLECommunicator, ModuleData->PLECommunicator);
			FinishedEvent::Register(*this, &LaserScanningSpectroscopy::OnFinishedCapturing, ModuleData->GetPLECommunicator()->GetID());
		}
		if (ModuleParams->WFCommunicator.ContainsID())
		{
			Instance->LockObject(ModuleParams->WFCommunicator, ModuleData->WFCommunicator);
			StartEvent::Register(*this, &LaserScanningSpectroscopy::OnStart, ModuleData->GetWFCommunicator()->GetID());
			StopEvent::Register(*this, &LaserScanningSpectroscopy::OnStop, ModuleData->GetWFCommunicator()->GetID());
			SetFilenameEvent::Register(*this, &LaserScanningSpectroscopy::OnPathChanged, ModuleData->GetWFCommunicator()->GetID());
		}

		Instance->LockObject(ModuleParams->Laser, ModuleData->GetLaser());
		ModuleData->ModeHopFreeTuningRange = ModuleData->GetLaser()->GetModeHopFreeTuningRange();
	}

	void LaserScanningSpectroscopy::OnExit(DynExp::ModuleInstance* Instance) const
	{
		auto ModuleData = DynExp::dynamic_ModuleData_cast<LaserScanningSpectroscopy>(Instance->ModuleDataGetter());

		Instance->UnlockObject(ModuleData->Laser);
		Instance->UnlockObject(ModuleData->PLECommunicator);
		Instance->UnlockObject(ModuleData->WFCommunicator);

		FinishedEvent::Deregister(*this);
		StartEvent::Deregister(*this);
		StopEvent::Deregister(*this);
		SetFilenameEvent::Deregister(*this);
	}

	void LaserScanningSpectroscopy::OnStart(DynExp::ModuleInstance* Instance) const
	{
		auto ModuleData = DynExp::dynamic_ModuleData_cast<LaserScanningSpectroscopy>(Instance->ModuleDataGetter());

		ModuleData->PLECommunicator->PostEvent(*this, StartEvent{});

		ModuleData->StepCount = 0;
		ModuleData->RepCount = 0;
		ModuleData->LaserScanningSpectroscopyProgress = 0;

		FrequencyStep(Instance);
		StateMachine.SetCurrentState(StateType::WaitForSettingFrequency);
	}

	void LaserScanningSpectroscopy::OnStartClicked(DynExp::ModuleInstance* Instance, bool) const
	{
		OnStart(Instance);
	}

	void LaserScanningSpectroscopy::OnStop(DynExp::ModuleInstance* Instance) const
	{
		auto ModuleData = DynExp::dynamic_ModuleData_cast<LaserScanningSpectroscopy>(Instance->ModuleDataGetter());

		ModuleData->PLECommunicator->PostEvent(*this, StopEvent{});

		StateMachine.SetCurrentState(StateType::Ready);
	}

	void LaserScanningSpectroscopy::OnStopClicked(DynExp::ModuleInstance* Instance, bool) const
	{
		OnStop(Instance);
	}

	void LaserScanningSpectroscopy::OnPath(DynExp::ModuleInstance* Instance, const QString SaveFilename) const
	{
		auto ModuleData = DynExp::dynamic_ModuleData_cast<LaserScanningSpectroscopy>(Instance->ModuleDataGetter());
		ModuleData->Filepath = std::filesystem::path(SaveFilename.toStdWString());
	}

	void LaserScanningSpectroscopy::OnPathChanged(DynExp::ModuleInstance* Instance, const std::string& SaveFilename) const
	{
		OnPath(Instance, QString::fromStdString(SaveFilename));
	}

	void LaserScanningSpectroscopy::OnPathBrowseClicked(DynExp::ModuleInstance* Instance, bool) const
	{
	}

	void LaserScanningSpectroscopy::OnLowerFrequencyLimitChanged(DynExp::ModuleInstance* Instance, double LowerFrequencyLimit) const
	{
		auto ModuleData = DynExp::dynamic_ModuleData_cast<LaserScanningSpectroscopy>(Instance->ModuleDataGetter());
		auto Widget = GetWidget<LaserScanningSpectroscopyWidget>();
		double NewFrequencyRange{};
		const double diff = ModuleData->UpperFrequencyLimit - LowerFrequencyLimit * 1e9;
		ModuleData->LowerFrequencyLimit = LowerFrequencyLimit * 1e9;

		// modify FrequencyRange, FrequencyCenter and UpperFrequencyLimit to match new LowerFrequencyLimit
		if (diff < 0)
		{
			NewFrequencyRange = ModuleData->FrequencyRange;
			ModuleData->UpperFrequencyLimit = LowerFrequencyLimit * 1e9 + NewFrequencyRange;
		}
		else if (0 < diff && diff <= ModuleData->ModeHopFreeTuningRange)
			NewFrequencyRange = ModuleData->UpperFrequencyLimit - LowerFrequencyLimit * 1e9;
		else
		{
			NewFrequencyRange = ModuleData->ModeHopFreeTuningRange;
			ModuleData->UpperFrequencyLimit = LowerFrequencyLimit * 1e9 + NewFrequencyRange;
		}

		ModuleData->CenterFrequency = LowerFrequencyLimit * 1e9 + NewFrequencyRange/2;
		ModuleData->FrequencyRange = NewFrequencyRange;
		ModuleData->NumberOfSteps = NewFrequencyRange / (ModuleData->Stepsize * 1e6);
		
		const QSignalBlocker b2(Widget->ui.SBUpperFrequencyLimit);
		const QSignalBlocker b3(Widget->ui.SBFrequencyRange);
		const QSignalBlocker b4(Widget->ui.SBCenterFrequency);
		const QSignalBlocker b7(Widget->ui.SBNumberOfSteps);
		Widget->ui.SBNumberOfSteps->setValue(ModuleData->NumberOfSteps);
		Widget->ui.SBFrequencyRange->setValue(ModuleData->FrequencyRange / 1e9);
		Widget->ui.SBCenterFrequency->setValue(ModuleData->CenterFrequency / 1e9);
		Widget->ui.SBUpperFrequencyLimit->setValue(ModuleData->UpperFrequencyLimit / 1e9);
	}
	
	void LaserScanningSpectroscopy::OnUpperFrequencyLimitChanged(DynExp::ModuleInstance* Instance, double UpperFrequencyLimit) const
	{
		auto ModuleData = DynExp::dynamic_ModuleData_cast<LaserScanningSpectroscopy>(Instance->ModuleDataGetter());
		auto Widget = GetWidget<LaserScanningSpectroscopyWidget>();
		double NewFrequencyRange;
		const double diff = UpperFrequencyLimit * 1e9 - ModuleData->LowerFrequencyLimit;
		ModuleData->UpperFrequencyLimit = UpperFrequencyLimit * 1e9;

		// modify FrequencyRange, FrequencyCenter and LowerFrequencyLimit to match new UpperFrequencyLimit
		if (diff < 0)
		{
			NewFrequencyRange = ModuleData->FrequencyRange;
			ModuleData->LowerFrequencyLimit = UpperFrequencyLimit * 1e9 - NewFrequencyRange;
		}
		else if (0 < diff && diff <= ModuleData->ModeHopFreeTuningRange)
			NewFrequencyRange = UpperFrequencyLimit * 1e9 - ModuleData->LowerFrequencyLimit;
		else
		{
			NewFrequencyRange = ModuleData->ModeHopFreeTuningRange;
			ModuleData->LowerFrequencyLimit = UpperFrequencyLimit * 1e9 - NewFrequencyRange;
		}

		ModuleData->CenterFrequency = UpperFrequencyLimit * 1e9 - NewFrequencyRange/2;
		ModuleData->FrequencyRange = NewFrequencyRange;
		ModuleData->NumberOfSteps = NewFrequencyRange / (ModuleData->Stepsize * 1e6);
		
		const QSignalBlocker b1(Widget->ui.SBLowerFrequencyLimit);
		const QSignalBlocker b3(Widget->ui.SBFrequencyRange);
		const QSignalBlocker b4(Widget->ui.SBCenterFrequency);
		const QSignalBlocker b7(Widget->ui.SBNumberOfSteps);
		Widget->ui.SBNumberOfSteps->setValue(ModuleData->NumberOfSteps);
		Widget->ui.SBFrequencyRange->setValue(NewFrequencyRange / 1e9);
		Widget->ui.SBCenterFrequency->setValue(ModuleData->CenterFrequency / 1e9);
		Widget->ui.SBLowerFrequencyLimit->setValue(ModuleData->LowerFrequencyLimit / 1e9);
	}
	
	void LaserScanningSpectroscopy::OnFrequencyRangeChanged(DynExp::ModuleInstance* Instance, double FrequencyRange) const
	{
		auto ModuleData = DynExp::dynamic_ModuleData_cast<LaserScanningSpectroscopy>(Instance->ModuleDataGetter());
		auto Widget = GetWidget<LaserScanningSpectroscopyWidget>();
		double NewFrequencyRange;
		ModuleData->FrequencyRange = FrequencyRange * 1e9;

		// modify FrequencyCenter and UpperFrequencyLimit to match new FrequencyRange
		if (FrequencyRange * 1e9 <= ModuleData->ModeHopFreeTuningRange)
			NewFrequencyRange = FrequencyRange * 1e9;
		else
		{
			NewFrequencyRange = ModuleData->ModeHopFreeTuningRange;
		}

		ModuleData->UpperFrequencyLimit = ModuleData->LowerFrequencyLimit + NewFrequencyRange;
		ModuleData->CenterFrequency = ModuleData->LowerFrequencyLimit + NewFrequencyRange/2;
		ModuleData->FrequencyRange = NewFrequencyRange;
		ModuleData->NumberOfSteps = NewFrequencyRange / (ModuleData->Stepsize * 1e6);
		
		const QSignalBlocker b2(Widget->ui.SBUpperFrequencyLimit);
		const QSignalBlocker b3(Widget->ui.SBFrequencyRange);
		const QSignalBlocker b4(Widget->ui.SBCenterFrequency);
		const QSignalBlocker b7(Widget->ui.SBNumberOfSteps);
		Widget->ui.SBNumberOfSteps->setValue(ModuleData->NumberOfSteps);
		Widget->ui.SBFrequencyRange->setValue(ModuleData->FrequencyRange / 1e9);
		Widget->ui.SBCenterFrequency->setValue(ModuleData->CenterFrequency / 1e9);
		Widget->ui.SBUpperFrequencyLimit->setValue(ModuleData->UpperFrequencyLimit / 1e9);
	}

	void LaserScanningSpectroscopy::OnFrequencyCenterChanged(DynExp::ModuleInstance* Instance, double FrequencyCenter) const
	{
		auto Widget = GetWidget<LaserScanningSpectroscopyWidget>();
		auto ModuleData = DynExp::dynamic_ModuleData_cast<LaserScanningSpectroscopy>(Instance->ModuleDataGetter());
		ModuleData->CenterFrequency = FrequencyCenter * 1e9;

		// modify LowerFrequencyLimit and UpperFrequencyLimit to match new FrequencyCenter
		ModuleData->UpperFrequencyLimit = FrequencyCenter * 1e9 + ModuleData->FrequencyRange/2;
		ModuleData->LowerFrequencyLimit = FrequencyCenter * 1e9 - ModuleData->FrequencyRange/2;
		
		const QSignalBlocker b1(Widget->ui.SBLowerFrequencyLimit);
		const QSignalBlocker b2(Widget->ui.SBUpperFrequencyLimit);
		Widget->ui.SBLowerFrequencyLimit->setValue(ModuleData->LowerFrequencyLimit / 1e9);
		Widget->ui.SBUpperFrequencyLimit->setValue(ModuleData->UpperFrequencyLimit / 1e9);
	}

	void LaserScanningSpectroscopy::OnStepsizeChanged(DynExp::ModuleInstance* Instance, double Stepsize) const
	{
		auto Widget = GetWidget<LaserScanningSpectroscopyWidget>();
		auto ModuleData = DynExp::dynamic_ModuleData_cast<LaserScanningSpectroscopy>(Instance->ModuleDataGetter());
		ModuleData->Stepsize = Stepsize * 1e6;

		// modify NumberOfSteps to match new Stepsize
		ModuleData->NumberOfSteps = ModuleData->FrequencyRange / (Stepsize * 1e6);

		const QSignalBlocker b7(Widget->ui.SBNumberOfSteps);
		Widget->ui.SBNumberOfSteps->setValue(ModuleData->NumberOfSteps);
	}

	void LaserScanningSpectroscopy::OnNumberOfStepsChanged(DynExp::ModuleInstance* Instance, int NumberOfSteps) const
	{
		auto Widget = GetWidget<LaserScanningSpectroscopyWidget>();
		auto ModuleData = DynExp::dynamic_ModuleData_cast<LaserScanningSpectroscopy>(Instance->ModuleDataGetter());
		ModuleData->NumberOfSteps = NumberOfSteps * 1e9;

		// modify Stepsize to match new NumberOfSteps
		ModuleData->Stepsize = ModuleData->FrequencyRange / NumberOfSteps;

		const QSignalBlocker b6(Widget->ui.SBStepsize);
		Widget->ui.SBStepsize->setValue(ModuleData->Stepsize / 1e6);
	}

	void LaserScanningSpectroscopy::OnRepetitionsChanged(DynExp::ModuleInstance* Instance, int Repetitions) const
	{
		auto ModuleData = DynExp::dynamic_ModuleData_cast<LaserScanningSpectroscopy>(Instance->ModuleDataGetter());
		
		ModuleData->Repetitions = Repetitions;
	}

	void LaserScanningSpectroscopy::OnStartAtMinimumToggled(DynExp::ModuleInstance* Instance, bool Checked) const
	{
		auto Widget = GetWidget<LaserScanningSpectroscopyWidget>();
		auto ModuleData = DynExp::dynamic_ModuleData_cast<LaserScanningSpectroscopy>(Instance->ModuleDataGetter());

		//if (Widget->ui.RBStartAtMinimum->isChecked())
		if (Checked)
		{
			const QSignalBlocker b8(Widget->ui.RBStartAtMaximum);
			Widget->ui.RBStartAtMaximum->setChecked(false);
			ModuleData->StartingPoint = ModuleData->LowerFrequencyLimit;
			ModuleData->EndingPoint = ModuleData->UpperFrequencyLimit;
		}
		else
		{
			const QSignalBlocker b8(Widget->ui.RBStartAtMaximum);
			Widget->ui.RBStartAtMaximum->setChecked(true);
			ModuleData->StartingPoint = ModuleData->UpperFrequencyLimit;
			ModuleData->EndingPoint = ModuleData->LowerFrequencyLimit;
		}
	}

	void LaserScanningSpectroscopy::OnStartAtMaximumToggled(DynExp::ModuleInstance* Instance, bool Checked) const
	{
		auto Widget = GetWidget<LaserScanningSpectroscopyWidget>();
		auto ModuleData = DynExp::dynamic_ModuleData_cast<LaserScanningSpectroscopy>(Instance->ModuleDataGetter());

		//if (Widget->ui.RBStartAtMaximum->isChecked())
		if (Checked)
		{
			const QSignalBlocker b9(Widget->ui.RBStartAtMinimum);
			Widget->ui.RBStartAtMinimum->setChecked(false);
			ModuleData->StartingPoint = ModuleData->UpperFrequencyLimit;
			ModuleData->EndingPoint = ModuleData->LowerFrequencyLimit;
		}
		else
		{
			const QSignalBlocker b9(Widget->ui.RBStartAtMinimum);
			Widget->ui.RBStartAtMinimum->setChecked(true);
			ModuleData->StartingPoint = ModuleData->LowerFrequencyLimit;
			ModuleData->EndingPoint = ModuleData->UpperFrequencyLimit;
		}
	}

	void LaserScanningSpectroscopy::OnScanBackAndForthToggled(DynExp::ModuleInstance* Instance, bool Checked) const
	{
		auto Widget = GetWidget<LaserScanningSpectroscopyWidget>();
		auto ModuleData = DynExp::dynamic_ModuleData_cast<LaserScanningSpectroscopy>(Instance->ModuleDataGetter());

		//if (Widget->ui.CBScanBackAndForth->isChecked())
		if (Checked)
			ModuleData->ScanBackAndForth = true;
		else
			ModuleData->ScanBackAndForth = false;
	}
	
	void LaserScanningSpectroscopy::OnFinishedCapturing(DynExp::ModuleInstance* Instance) const
	{
		auto ModuleData = DynExp::dynamic_ModuleData_cast<LaserScanningSpectroscopy>(Instance->ModuleDataGetter());

		if (ModuleData->StepCount < ModuleData->NumberOfSteps)
		{
			FrequencyStep(Instance);
			StateMachine.SetCurrentState(StateType::WaitForSettingFrequency);
		}
		else
		{
			ModuleData->RepCount++;
			if (ModuleData->RepCount == ModuleData->Repetitions)
			{
				ModuleData->WFCommunicator->PostEvent(*this, FinishedEvent{});
				StateMachine.SetCurrentState(StateType::Ready);
			}
			else
			{
				FrequencyStep(Instance);
				StateMachine.SetCurrentState(StateType::WaitForSettingFrequency);
			}
		}
	}

	std::filesystem::path LaserScanningSpectroscopy::BuildFilename(Util::SynchronizedPointer<ModuleDataType>& ModuleData, std::string_view FilenameSuffix) const
	{
		auto SavePath = ModuleData->Filepath;
		SavePath.replace_filename(SavePath.filename().stem().concat(FilenameSuffix));
		std::filesystem::create_directories(SavePath.parent_path());

		return SavePath;
	}

	void LaserScanningSpectroscopy::FrequencyStep(DynExp::ModuleInstance* Instance) const
	{
		auto ModuleData = DynExp::dynamic_ModuleData_cast<LaserScanningSpectroscopy>(Instance->ModuleDataGetter());

		//ModuleData->PLECommunicator->PostEvent(*this, SetFilenameEvent{
		//	BuildFilename(ModuleData, Util::ToStr(ModuleData->Filepath) + "_PLE_rep_" + Util::ToStr(ModuleData->RepCount) + "_step_" + Util::ToStr(ModuleData->StepCount)).string() });

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

		//StateMachine.SetCurrentState(StateType::WaitForSettingFrequency);
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
			ModuleData->PLECommunicator->PostEvent(*this, TriggerEvent{});
			return StateType::WaitForCapturing;
			}
		else
			return StateType::WaitForSettingFrequency;
	}

	StateType LaserScanningSpectroscopy::WaitForCapturingStateFunc(DynExp::ModuleInstance& Instance)
	{
		return StateType::WaitForCapturing;
	}
}
