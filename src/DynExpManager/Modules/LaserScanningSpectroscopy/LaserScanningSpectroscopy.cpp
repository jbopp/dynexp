// This file is part of DynExp.

#include "stdafx.h"
#include "moc_LaserScanningSpectroscopy.cpp"
#include "ui_LaserScanningSpectroscopy.h"
#include "LaserScanningSpectroscopy.h"

namespace DynExpModule::LaserScanningSpectroscopy
{
	LaserScanningSpectroscopyWidget::LaserScanningSpectroscopyWidget(LaserScanningSpectroscopy& Owner, QModuleWidget* parent)
		: QModuleWidget(Owner, parent),
		ui(std::make_unique<Ui::LaserScanningSpectroscopy>())
	{
		ui->setupUi(this);

		// For shortcuts
		this->addAction(ui->action_Stop);
	}

	void LaserScanningSpectroscopyWidget::InitializeUI(Util::SynchronizedPointer<LaserScanningSpectroscopyData>& ModuleData)
	{
		if (!GetUIInitialized())
		{
			const QSignalBlocker SBLowerFrequencyLimitBlocker(ui->SBLowerFrequencyLimit);
			ui->SBLowerFrequencyLimit->setRange(ModuleData->GetLaser()->GetMinFrequency() * 1e-9, ModuleData->GetLaser()->GetMaxFrequency() * 1e-9);
			ui->SBLowerFrequencyLimit->setValue(ModuleData->GetLaser()->GetMinFrequency() * 1e-9);

			const QSignalBlocker SBUpperFrequencyLimitBlocker(ui->SBUpperFrequencyLimit);
			ui->SBUpperFrequencyLimit->setRange(ModuleData->GetLaser()->GetMinFrequency() * 1e-9, ModuleData->GetLaser()->GetMaxFrequency() * 1e-9);
			ui->SBUpperFrequencyLimit->setValue((ModuleData->GetLaser()->GetMinFrequency() + ModuleData->GetLaser()->GetModeHopFreeTuningRange()) * 1e-9);

			const QSignalBlocker SBFrequencyRangeBlocker(ui->SBFrequencyRange);
			ui->SBFrequencyRange->setRange(0.0, ModuleData->GetLaser()->GetModeHopFreeTuningRange() * 1e-9);
			ui->SBFrequencyRange->setValue(ModuleData->GetLaser()->GetModeHopFreeTuningRange() * 1e-9);

			const QSignalBlocker SBCenterFrequencyBlocker(ui->SBCenterFrequency);
			ui->SBCenterFrequency->setRange(ModuleData->GetLaser()->GetMinFrequency() * 1e-9 + 0.5 * ModuleData->GetLaser()->GetModeHopFreeTuningRange() * 1e-9, ModuleData->GetLaser()->GetMaxFrequency() * 1e-9 - 0.5 * ModuleData->GetLaser()->GetModeHopFreeTuningRange() * 1e-9);
			ui->SBCenterFrequency->setValue(ModuleData->GetLaser()->GetMinFrequency() * 1e-9 + 0.5 * ModuleData->GetLaser()->GetModeHopFreeTuningRange() * 1e-9);
			
			const QSignalBlocker SBStepSizeBlocker(ui->SBStepSize);
			ui->SBStepSize->setRange(1, 10000);
			ui->SBStepSize->setValue(100);

			const QSignalBlocker SBNumberOfStepsBlocker(ui->SBNumberOfSteps);
			ui->SBNumberOfSteps->setRange(1, 10000);
			ui->SBNumberOfSteps->setValue(ui->SBFrequencyRange->value() * 1e3 / ui->SBStepSize->value());

			const QSignalBlocker SBNumberOfRepetitionsBlocker(ui->SBNumberOfRepetitions);
			ui->SBNumberOfRepetitions->setRange(1, 10000);
			ui->SBNumberOfRepetitions->setValue(1);

			ModuleData->LowerFrequencyLimit = ui->SBLowerFrequencyLimit->value() * 1e9;
			ModuleData->UpperFrequencyLimit = ui->SBUpperFrequencyLimit->value() * 1e9;
			ModuleData->FrequencyRange = ui->SBFrequencyRange->value() * 1e9;
			ModuleData->CenterFrequency = ui->SBCenterFrequency->value() * 1e9;
			ModuleData->StepSize = ui->SBStepSize->value() * 1e6;
			ModuleData->NumberOfSteps = ui->SBNumberOfSteps->value();
			ModuleData->NumberOfRepetitions = ui->SBNumberOfRepetitions->value();
			ModuleData->ScanBackAndForth = ui->CBScanBackAndForth->isChecked();

			ModuleData->ScanStartFrequency = ui->RBStartAtMinimum->isChecked() ? ModuleData->LowerFrequencyLimit : ModuleData->UpperFrequencyLimit;
			ModuleData->ScanEndFrequency = ui->RBStartAtMinimum->isChecked() ? ModuleData->UpperFrequencyLimit : ModuleData->LowerFrequencyLimit;

			UIInitialized = true;
		}
	}

	void LaserScanningSpectroscopyWidget::OnPathBrowseClicked()
	{
		auto Filename = Util::PromptSaveFilePathModule(this, "Select directory and filename prefix for saving data", ".csv", "Comma-separated values file (*.csv)");
		if (Filename.isEmpty())
			return;

		// Emits signal to update module data accordingly.
		ui->LEPath->setText(Filename);
	}

	void LaserScanningSpectroscopyData::ResetImpl(dispatch_tag<QModuleDataBase>)
	{
		Init();
	}

	void LaserScanningSpectroscopyData::Init()
	{
		IsStepwiseScan = false;
		LowerFrequencyLimit = 0.0;
		UpperFrequencyLimit = 0.0;
		FrequencyRange = 0.0;
		ModeHopFreeTuningRange = 0.0;
		CenterFrequency = 0.0;
		StepSize = 0.0;
		NumberOfSteps = 0;
		NumberOfRepetitions = 0;
		ScanStartFrequency = 0.0;
		ScanEndFrequency = 0.0;
		ScanBackAndForth = false;
		CurrentStepCount = 0;
		CurrentRepCount = 0;
		
		FileSavePath.clear();
		FileSavePathChanged = false;
		LaserState = DynExpInstr::LaserData::LaserStateType::Ready;

		LaserScanningSpectroscopyState = StateType::Ready;
		LaserScanningSpectroscopyProgress = 0;
	}

	LaserScanningSpectroscopy::LaserScanningSpectroscopy(const std::thread::id OwnerThreadID, DynExp::ParamsBasePtrType&& Params)
		: QModuleBase(OwnerThreadID, std::move(Params)),
		StateMachine(ReadyState, WaitForSettingFrequencyState, WaitForCapturingState)
	{
	}

	Util::DynExpErrorCodes::DynExpErrorCodes LaserScanningSpectroscopy::ModuleMainLoop(DynExp::ModuleInstance& Instance)
	{
		try
		{
			auto ModuleData = DynExp::dynamic_ModuleData_cast<LaserScanningSpectroscopy>(Instance.ModuleDataGetter());

			ModuleData->LaserState = DynExp::dynamic_InstrumentData_cast<DynExpInstr::Laser>(ModuleData->GetLaser()->GetInstrumentData())->GetLaserState();

			StateMachine.Invoke(*this, Instance);

			NumFailedUpdateAttempts = 0;
		}

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

		Connect(Widget->GetUI()->action_Start, &QAction::triggered, this, &LaserScanningSpectroscopy::OnStartClicked);
		Connect(Widget->GetUI()->action_Stop, &QAction::triggered, this, &LaserScanningSpectroscopy::OnStopClicked);
		Connect(Widget->GetUI()->action_StepwiseScan, &QAction::toggled, this, &LaserScanningSpectroscopy::OnStartAtToggled);

		Connect(Widget->GetUI()->SBLowerFrequencyLimit, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &LaserScanningSpectroscopy::OnLowerFrequencyLimitChanged);
		Connect(Widget->GetUI()->SBUpperFrequencyLimit, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &LaserScanningSpectroscopy::OnUpperFrequencyLimitChanged);
		Connect(Widget->GetUI()->SBFrequencyRange, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &LaserScanningSpectroscopy::OnFrequencyRangeChanged);
		Connect(Widget->GetUI()->SBCenterFrequency, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &LaserScanningSpectroscopy::OnCenterFrequencyChanged);
		Connect(Widget->GetUI()->SBStepSize, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &LaserScanningSpectroscopy::OnStepSizeChanged);
		Connect(Widget->GetUI()->SBNumberOfSteps, QOverload<int>::of(&QSpinBox::valueChanged), this, &LaserScanningSpectroscopy::OnNumberOfStepsChanged);
		Connect(Widget->GetUI()->SBNumberOfRepetitions, QOverload<int>::of(&QSpinBox::valueChanged), this, &LaserScanningSpectroscopy::OnNumberOfRepetitionsChanged);
		Connect(Widget->GetUI()->CBScanBackAndForth, &QCheckBox::toggled, this, &LaserScanningSpectroscopy::OnScanBackAndForthToggled);
		Connect(Widget->GetUI()->RBStartAtMinimum, &QRadioButton::toggled, this, &LaserScanningSpectroscopy::OnStartAtToggled);
		Connect(Widget->GetUI()->RBStartAtMaximum, &QRadioButton::toggled, this, &LaserScanningSpectroscopy::OnStartAtToggled);
		Connect(Widget->GetUI()->LEPath, &QLineEdit::textChanged, this, static_cast<void(LaserScanningSpectroscopy::*)(DynExp::ModuleInstance*, const QString) const>(&LaserScanningSpectroscopy::OnPathChanged));

		return Widget;
	}

	void LaserScanningSpectroscopy::UpdateUIChild(const ModuleBase::ModuleDataGetterType& ModuleDataGetter)
	{
		const bool Ready = IsReadyState();
		const bool SettingFrequency = IsSettingFrequencyState();
		const bool Capturing = IsCapturingState();

		auto Widget = GetWidget<LaserScanningSpectroscopyWidget>();
		auto ModuleData = DynExp::dynamic_ModuleData_cast<LaserScanningSpectroscopy>(ModuleDataGetter());

		Widget->InitializeUI(ModuleData);

		Widget->GetUI()->action_Start->setEnabled(Ready && ModuleData->LaserState != DynExpInstr::LaserData::LaserStateType::Startup);
		Widget->GetUI()->action_Stop->setEnabled(!Ready);
		Widget->GetUI()->action_StepwiseScan->setEnabled(Ready);

		Widget->GetUI()->SBLowerFrequencyLimit->setEnabled(Ready);
		Widget->GetUI()->SBUpperFrequencyLimit->setEnabled(Ready);
		Widget->GetUI()->SBFrequencyRange->setEnabled(Ready);
		Widget->GetUI()->SBCenterFrequency->setEnabled(Ready);
		Widget->GetUI()->SBStepSize->setEnabled(Ready);
		Widget->GetUI()->SBNumberOfSteps->setEnabled(Ready);
		Widget->GetUI()->SBNumberOfRepetitions->setEnabled(Ready);
		Widget->GetUI()->CBScanBackAndForth->setEnabled(Ready && ModuleData->IsStepwiseScan);
		Widget->GetUI()->RBStartAtMinimum->setEnabled(Ready && ModuleData->IsStepwiseScan);
		Widget->GetUI()->RBStartAtMaximum->setEnabled(Ready && ModuleData->IsStepwiseScan);
		
		Widget->GetUI()->PBLaserScanningSpectroscopyProgress->setVisible(!Ready);
		Widget->GetUI()->PBLaserScanningSpectroscopyProgress->setValue(static_cast<int>(100.0 *
			ModuleData->LaserScanningSpectroscopyProgress / ModuleData->NumberOfRepetitions / (ModuleData->NumberOfSteps + 1)));
		
		if (SettingFrequency)
			Widget->GetUI()->LLaserScanningSpectroscopyState->setText(" Stabilizing");
		else if (Capturing)
			Widget->GetUI()->LLaserScanningSpectroscopyState->setText(" Capturing");
		else
			Widget->GetUI()->LLaserScanningSpectroscopyState->setText(" Ready");

		if (ModuleData->FileSavePathChanged)
		{
			const QSignalBlocker LEPathBlocker(Widget->GetUI()->LEPath);
			Widget->GetUI()->LEPath->setText(QString::fromStdString(ModuleData->FileSavePath.string()));

			ModuleData->FileSavePathChanged = false;
		}
	}

	bool LaserScanningSpectroscopy::IsReadyState() const noexcept
	{
		return StateMachine.GetCurrentState()->GetState() == StateType::Ready;
	}

	bool LaserScanningSpectroscopy::IsSettingFrequencyState() const noexcept
	{
		return StateMachine.GetCurrentState()->GetState() == StateType::WaitForSettingFrequency;
	}

	bool LaserScanningSpectroscopy::IsCapturingState() const noexcept
	{
		return StateMachine.GetCurrentState()->GetState() == StateType::WaitForCapturing;
	}

	void LaserScanningSpectroscopy::FrequencyStep(DynExp::ModuleInstance* Instance) const
	{
		auto ModuleData = DynExp::dynamic_ModuleData_cast<LaserScanningSpectroscopy>(Instance->ModuleDataGetter());

		double Frequency = 0.0;

		if (!ModuleData->CurrentStepCount)
			Frequency = ModuleData->ScanStartFrequency;
		else if (ModuleData->CurrentStepCount <= ModuleData->NumberOfSteps)
		{
			if ((ModuleData->ScanStartFrequency == ModuleData->LowerFrequencyLimit && ModuleData->ScanBackAndForth && ModuleData->CurrentRepCount % 2 == 0) ||
				(ModuleData->ScanStartFrequency == ModuleData->UpperFrequencyLimit && ModuleData->ScanBackAndForth && ModuleData->CurrentRepCount % 2 == 1) ||
				(ModuleData->ScanStartFrequency == ModuleData->LowerFrequencyLimit && !ModuleData->ScanBackAndForth))
				Frequency = ModuleData->LowerFrequencyLimit + ModuleData->CurrentStepCount * ModuleData->StepSize;
			else
				Frequency = ModuleData->UpperFrequencyLimit - ModuleData->CurrentStepCount * ModuleData->StepSize;
		}
		else
		{
			ModuleData->CurrentStepCount = 0;

			if (!ModuleData->ScanBackAndForth)
				Frequency = ModuleData->ScanStartFrequency;
			else
				Frequency = (ModuleData->CurrentRepCount % 2 == 0) ? ModuleData->ScanStartFrequency : ModuleData->ScanEndFrequency;
		}

		ModuleData->GetLaser()->SetFrequency(Frequency);
	}

	std::filesystem::path LaserScanningSpectroscopy::BuildFilename(Util::SynchronizedPointer<ModuleDataType>& ModuleData, std::string_view FilenameSuffix) const
	{
		auto SavePath = ModuleData->FileSavePath;
		SavePath.replace_filename(SavePath.filename().stem().concat(FilenameSuffix));
		std::filesystem::create_directories(SavePath.parent_path());

		return SavePath;
	}

	void LaserScanningSpectroscopy::OnInit(DynExp::ModuleInstance* Instance) const
	{
		auto ModuleParams = DynExp::dynamic_Params_cast<LaserScanningSpectroscopy>(Instance->ParamsGetter());
		auto ModuleData = DynExp::dynamic_ModuleData_cast<LaserScanningSpectroscopy>(Instance->ModuleDataGetter());

		if (ModuleParams->PLECommunicator.ContainsID())
		{
			Instance->LockObject(ModuleParams->PLECommunicator, ModuleData->GetPLECommunicator());
			FinishedEvent::Register(*this, &LaserScanningSpectroscopy::OnFinishedCapturing, ModuleData->GetPLECommunicator()->GetID());
		}
		if (ModuleParams->WFCommunicator.ContainsID())
		{
			Instance->LockObject(ModuleParams->WFCommunicator, ModuleData->GetWFCommunicator());
			StartEvent::Register(*this, &LaserScanningSpectroscopy::OnStart, ModuleData->GetWFCommunicator()->GetID());
			StopEvent::Register(*this, &LaserScanningSpectroscopy::OnStop, ModuleData->GetWFCommunicator()->GetID());
			SetFilenameEvent::Register(*this, static_cast<void(LaserScanningSpectroscopy::*)(DynExp::ModuleInstance*, const std::string&) const>(&LaserScanningSpectroscopy::OnPathChanged),
				ModuleData->GetWFCommunicator()->GetID());
		}

		Instance->LockObject(ModuleParams->Laser, ModuleData->GetLaser());
		ModuleData->ModeHopFreeTuningRange = ModuleData->GetLaser()->GetModeHopFreeTuningRange();
	}

	void LaserScanningSpectroscopy::OnExit(DynExp::ModuleInstance* Instance) const
	{
		auto ModuleData = DynExp::dynamic_ModuleData_cast<LaserScanningSpectroscopy>(Instance->ModuleDataGetter());

		Instance->UnlockObject(ModuleData->GetPLECommunicator());		
		Instance->UnlockObject(ModuleData->GetWFCommunicator());
		Instance->UnlockObject(ModuleData->GetLaser());
		
		FinishedEvent::Deregister(*this);
		StartEvent::Deregister(*this);
		StopEvent::Deregister(*this);
		SetFilenameEvent::Deregister(*this);
	}

	void LaserScanningSpectroscopy::OnStart(DynExp::ModuleInstance* Instance) const
	{
		auto ModuleData = DynExp::dynamic_ModuleData_cast<LaserScanningSpectroscopy>(Instance->ModuleDataGetter());

		ModuleData->GetPLECommunicator()->PostEvent(*this, StartEvent{});

		ModuleData->CurrentStepCount = 0;
		ModuleData->CurrentRepCount = 0;
		ModuleData->LaserScanningSpectroscopyProgress = 0;

		FrequencyStep(Instance);
		
		auto ModuleParams = DynExp::dynamic_Params_cast<LaserScanningSpectroscopy>(Instance->ParamsGetter());
		WaitingEndTimePoint = std::chrono::system_clock::now() + std::chrono::milliseconds(ModuleParams->CapturingTimeDifference);
		
		StateMachine.SetCurrentState(StateType::WaitForSettingFrequency);
	}

	void LaserScanningSpectroscopy::OnStartClicked(DynExp::ModuleInstance* Instance, bool) const
	{
		OnStart(Instance);
	}

	void LaserScanningSpectroscopy::OnStop(DynExp::ModuleInstance* Instance) const
	{
		auto ModuleData = DynExp::dynamic_ModuleData_cast<LaserScanningSpectroscopy>(Instance->ModuleDataGetter());

		ModuleData->GetPLECommunicator()->PostEvent(*this, StopEvent{});
		ModuleData->GetLaser()->DisableScan();

		StateMachine.SetCurrentState(StateType::Ready);
	}

	void LaserScanningSpectroscopy::OnStopClicked(DynExp::ModuleInstance* Instance, bool) const
	{
		OnStop(Instance);
	}

	void LaserScanningSpectroscopy::OnLowerFrequencyLimitChanged(DynExp::ModuleInstance* Instance, double LowerFrequencyLimit) const
	{
		auto ModuleData = DynExp::dynamic_ModuleData_cast<LaserScanningSpectroscopy>(Instance->ModuleDataGetter());
		auto Widget = GetWidget<LaserScanningSpectroscopyWidget>();

		double NewFrequencyRange = ModuleData->UpperFrequencyLimit - LowerFrequencyLimit * 1e9;
		ModuleData->LowerFrequencyLimit = LowerFrequencyLimit * 1e9;

		if (NewFrequencyRange < 0)
		{
			NewFrequencyRange = ModuleData->FrequencyRange;
			ModuleData->UpperFrequencyLimit = LowerFrequencyLimit * 1e9 + NewFrequencyRange;
		}
		else if (NewFrequencyRange > ModuleData->ModeHopFreeTuningRange)
		{
			NewFrequencyRange = ModuleData->ModeHopFreeTuningRange;
			ModuleData->UpperFrequencyLimit = LowerFrequencyLimit * 1e9 + NewFrequencyRange;
		}

		// Modify FrequencyRange, CenterFrequency and UpperFrequencyLimit to match new LowerFrequencyLimit.
		ModuleData->CenterFrequency = LowerFrequencyLimit * 1e9 + NewFrequencyRange / 2;
		ModuleData->FrequencyRange = NewFrequencyRange;
		ModuleData->NumberOfSteps = NewFrequencyRange / ModuleData->StepSize;
		ModuleData->GetLaser()->SetScanRange(NewFrequencyRange);
		
		const QSignalBlocker SBUpperFrequencyLimitBlocker(Widget->GetUI()->SBUpperFrequencyLimit);
		const QSignalBlocker SBFrequencyRangeBlocker(Widget->GetUI()->SBFrequencyRange);
		const QSignalBlocker SBCenterFrequencyBlocker(Widget->GetUI()->SBCenterFrequency);
		const QSignalBlocker SBNumberOfStepsBlocker(Widget->GetUI()->SBNumberOfSteps);
		Widget->GetUI()->SBUpperFrequencyLimit->setValue(ModuleData->UpperFrequencyLimit / 1e9);
		Widget->GetUI()->SBFrequencyRange->setValue(ModuleData->FrequencyRange / 1e9);
		Widget->GetUI()->SBCenterFrequency->setValue(ModuleData->CenterFrequency / 1e9);
		Widget->GetUI()->SBNumberOfSteps->setValue(ModuleData->NumberOfSteps);
	}
	
	void LaserScanningSpectroscopy::OnUpperFrequencyLimitChanged(DynExp::ModuleInstance* Instance, double UpperFrequencyLimit) const
	{
		auto ModuleData = DynExp::dynamic_ModuleData_cast<LaserScanningSpectroscopy>(Instance->ModuleDataGetter());
		auto Widget = GetWidget<LaserScanningSpectroscopyWidget>();

		double NewFrequencyRange = UpperFrequencyLimit * 1e9 - ModuleData->LowerFrequencyLimit;
		ModuleData->UpperFrequencyLimit = UpperFrequencyLimit * 1e9;

		if (NewFrequencyRange < 0)
		{
			NewFrequencyRange = ModuleData->FrequencyRange;
			ModuleData->LowerFrequencyLimit = UpperFrequencyLimit * 1e9 - NewFrequencyRange;
		}
		else if (NewFrequencyRange > ModuleData->ModeHopFreeTuningRange)
		{
			NewFrequencyRange = ModuleData->ModeHopFreeTuningRange;
			ModuleData->LowerFrequencyLimit = UpperFrequencyLimit * 1e9 - NewFrequencyRange;
		}

		// Modify FrequencyRange, CenterFrequency and LowerFrequencyLimit to match new UpperFrequencyLimit.
		ModuleData->CenterFrequency = UpperFrequencyLimit * 1e9 - NewFrequencyRange / 2;
		ModuleData->FrequencyRange = NewFrequencyRange;
		ModuleData->NumberOfSteps = NewFrequencyRange / ModuleData->StepSize;
		ModuleData->GetLaser()->SetScanRange(NewFrequencyRange);
		
		const QSignalBlocker SBLowerFrequencyLimitBlocker(Widget->GetUI()->SBLowerFrequencyLimit);
		const QSignalBlocker SBFrequencyRangeBlocker(Widget->GetUI()->SBFrequencyRange);
		const QSignalBlocker SBCenterFrequencyBlocker(Widget->GetUI()->SBCenterFrequency);
		const QSignalBlocker SBNumberOfStepsBlocker(Widget->GetUI()->SBNumberOfSteps);
		Widget->GetUI()->SBLowerFrequencyLimit->setValue(ModuleData->LowerFrequencyLimit / 1e9);
		Widget->GetUI()->SBFrequencyRange->setValue(NewFrequencyRange / 1e9);
		Widget->GetUI()->SBCenterFrequency->setValue(ModuleData->CenterFrequency / 1e9);
		Widget->GetUI()->SBNumberOfSteps->setValue(ModuleData->NumberOfSteps);
	}
	
	void LaserScanningSpectroscopy::OnFrequencyRangeChanged(DynExp::ModuleInstance* Instance, double FrequencyRange) const
	{
		auto ModuleData = DynExp::dynamic_ModuleData_cast<LaserScanningSpectroscopy>(Instance->ModuleDataGetter());
		auto Widget = GetWidget<LaserScanningSpectroscopyWidget>();

		ModuleData->FrequencyRange = FrequencyRange * 1e9;
		const double NewFrequencyRange = FrequencyRange * 1e9 <= ModuleData->ModeHopFreeTuningRange ? FrequencyRange * 1e9 : ModuleData->ModeHopFreeTuningRange;

		// Modify CenterFrequency and UpperFrequencyLimit to match new FrequencyRange.
		ModuleData->UpperFrequencyLimit = ModuleData->LowerFrequencyLimit + NewFrequencyRange;
		ModuleData->CenterFrequency = ModuleData->LowerFrequencyLimit + NewFrequencyRange / 2;
		ModuleData->FrequencyRange = NewFrequencyRange;
		ModuleData->NumberOfSteps = NewFrequencyRange / ModuleData->StepSize;
		ModuleData->GetLaser()->SetScanRange(NewFrequencyRange);

		const QSignalBlocker SBUpperFrequencyLimitBlocker(Widget->GetUI()->SBUpperFrequencyLimit);
		const QSignalBlocker SBFrequencyRangeBlocker(Widget->GetUI()->SBFrequencyRange);
		const QSignalBlocker SBCenterFrequencyBlocker(Widget->GetUI()->SBCenterFrequency);
		const QSignalBlocker SBNumberOfStepsBlocker(Widget->GetUI()->SBNumberOfSteps);
		Widget->GetUI()->SBUpperFrequencyLimit->setValue(ModuleData->UpperFrequencyLimit / 1e9);
		Widget->GetUI()->SBFrequencyRange->setValue(ModuleData->FrequencyRange / 1e9);
		Widget->GetUI()->SBCenterFrequency->setValue(ModuleData->CenterFrequency / 1e9);
		Widget->GetUI()->SBNumberOfSteps->setValue(ModuleData->NumberOfSteps);
	}

	void LaserScanningSpectroscopy::OnCenterFrequencyChanged(DynExp::ModuleInstance* Instance, double CenterFrequency) const
	{
		auto Widget = GetWidget<LaserScanningSpectroscopyWidget>();
		auto ModuleData = DynExp::dynamic_ModuleData_cast<LaserScanningSpectroscopy>(Instance->ModuleDataGetter());

		ModuleData->CenterFrequency = CenterFrequency * 1e9;

		// Modify LowerFrequencyLimit and UpperFrequencyLimit to match new CenterFrequency.
		ModuleData->UpperFrequencyLimit = CenterFrequency * 1e9 + ModuleData->FrequencyRange / 2;
		ModuleData->LowerFrequencyLimit = CenterFrequency * 1e9 - ModuleData->FrequencyRange / 2;
		
		const QSignalBlocker SBLowerFrequencyLimitBlocker(Widget->GetUI()->SBLowerFrequencyLimit);
		const QSignalBlocker SBUpperFrequencyLimitBlocker(Widget->GetUI()->SBUpperFrequencyLimit);
		Widget->GetUI()->SBLowerFrequencyLimit->setValue(ModuleData->LowerFrequencyLimit / 1e9);
		Widget->GetUI()->SBUpperFrequencyLimit->setValue(ModuleData->UpperFrequencyLimit / 1e9);
	}

	void LaserScanningSpectroscopy::OnStepSizeChanged(DynExp::ModuleInstance* Instance, double StepSize) const
	{
		auto Widget = GetWidget<LaserScanningSpectroscopyWidget>();
		auto ModuleData = DynExp::dynamic_ModuleData_cast<LaserScanningSpectroscopy>(Instance->ModuleDataGetter());

		ModuleData->StepSize = StepSize * 1e6;

		// Modify NumberOfSteps to match new StepSize.
		ModuleData->NumberOfSteps = ModuleData->FrequencyRange / ModuleData->StepSize;

		const QSignalBlocker SBNumberOfStepsBlocker(Widget->GetUI()->SBNumberOfSteps);
		Widget->GetUI()->SBNumberOfSteps->setValue(ModuleData->NumberOfSteps);
	}

	void LaserScanningSpectroscopy::OnNumberOfStepsChanged(DynExp::ModuleInstance* Instance, int NumberOfSteps) const
	{
		auto Widget = GetWidget<LaserScanningSpectroscopyWidget>();
		auto ModuleData = DynExp::dynamic_ModuleData_cast<LaserScanningSpectroscopy>(Instance->ModuleDataGetter());

		ModuleData->NumberOfSteps = NumberOfSteps;

		// Modify StepSize to match new NumberOfSteps.
		ModuleData->StepSize = ModuleData->FrequencyRange / ModuleData->NumberOfSteps;

		const QSignalBlocker SBStepSizeBlocker(Widget->GetUI()->SBStepSize);
		Widget->GetUI()->SBStepSize->setValue(ModuleData->StepSize / 1e6);
	}

	void LaserScanningSpectroscopy::OnNumberOfRepetitionsChanged(DynExp::ModuleInstance* Instance, int NumberOfRepetitions) const
	{
		auto ModuleData = DynExp::dynamic_ModuleData_cast<LaserScanningSpectroscopy>(Instance->ModuleDataGetter());
		ModuleData->NumberOfRepetitions = NumberOfRepetitions;
	}

	void LaserScanningSpectroscopy::OnScanBackAndForthToggled(DynExp::ModuleInstance* Instance, bool Checked) const
	{
		auto ModuleData = DynExp::dynamic_ModuleData_cast<LaserScanningSpectroscopy>(Instance->ModuleDataGetter());
		ModuleData->ScanBackAndForth = Checked;
	}

	void LaserScanningSpectroscopy::OnStartAtToggled(DynExp::ModuleInstance* Instance, bool) const
	{
		auto Widget = GetWidget<LaserScanningSpectroscopyWidget>();
		auto ModuleData = DynExp::dynamic_ModuleData_cast<LaserScanningSpectroscopy>(Instance->ModuleDataGetter());

		ModuleData->IsStepwiseScan = Widget->GetUI()->action_StepwiseScan->isChecked();
		if (ModuleData->IsStepwiseScan)
		{
			ModuleData->ScanStartFrequency = Widget->GetUI()->RBStartAtMinimum->isChecked() ? ModuleData->LowerFrequencyLimit : ModuleData->UpperFrequencyLimit;
			ModuleData->ScanEndFrequency = Widget->GetUI()->RBStartAtMinimum->isChecked() ? ModuleData->UpperFrequencyLimit : ModuleData->LowerFrequencyLimit;
		}
		else
		{
			ModuleData->ScanStartFrequency = ModuleData->CenterFrequency;
			ModuleData->ScanEndFrequency = 0.0;
		}
	}

	void LaserScanningSpectroscopy::OnPathChanged(DynExp::ModuleInstance* Instance, const QString SaveFilename) const
	{
		OnPathChanged(Instance, SaveFilename.toStdString());
	}

	void LaserScanningSpectroscopy::OnPathChanged(DynExp::ModuleInstance* Instance, const std::string& SaveFilename) const
	{
		auto ModuleData = DynExp::dynamic_ModuleData_cast<LaserScanningSpectroscopy>(Instance->ModuleDataGetter());
		ModuleData->FileSavePath = std::filesystem::path(SaveFilename);
		ModuleData->FileSavePathChanged = true;
	}
	
	void LaserScanningSpectroscopy::OnFinishedCapturing(DynExp::ModuleInstance* Instance) const
	{
		auto ModuleData = DynExp::dynamic_ModuleData_cast<LaserScanningSpectroscopy>(Instance->ModuleDataGetter());

		if (ModuleData->CurrentStepCount > ModuleData->NumberOfSteps)
		{
			ModuleData->CurrentStepCount = 0;
			ModuleData->CurrentRepCount++;

			if (ModuleData->CurrentRepCount >= ModuleData->NumberOfRepetitions)
			{
				// Turn off continuous scan mode.
				ModuleData->GetLaser()->DisableScan();

				ModuleData->GetPLECommunicator()->PostEvent(*this, FinishedEvent{});
				ModuleData->GetWFCommunicator()->PostEvent(*this, FinishedEvent{});

				StateMachine.SetCurrentState(StateType::Ready);

				return;
			}
		}

		if (ModuleData->IsStepwiseScan)
			FrequencyStep(Instance);

		auto ModuleParams = DynExp::dynamic_Params_cast<LaserScanningSpectroscopy>(Instance->ParamsGetter());
		WaitingEndTimePoint = std::chrono::system_clock::now() + std::chrono::milliseconds(ModuleParams->CapturingTimeDifference);

		StateMachine.SetCurrentState(StateType::WaitForSettingFrequency);
	}

	StateType LaserScanningSpectroscopy::ReadyStateFunc(DynExp::ModuleInstance& Instance)
	{
		return StateType::Ready;
	}

	StateType LaserScanningSpectroscopy::WaitForSettingFrequencyStateFunc(DynExp::ModuleInstance& Instance)
	{
		auto ModuleData = DynExp::dynamic_ModuleData_cast<LaserScanningSpectroscopy>(Instance.ModuleDataGetter());
		auto LaserInstrData = DynExp::dynamic_InstrumentData_cast<DynExpInstr::Laser>(ModuleData->GetLaser()->GetInstrumentData());

		auto Suffix = std::string("_CenterHz_") + Util::ToStr(ModuleData->CenterFrequency, 0)
			+ "_RangeHz_" + Util::ToStr(ModuleData->FrequencyRange, 0)
			+ "_Rep_" + Util::ToStr(ModuleData->CurrentRepCount)
			+ "_Step_" + Util::ToStr(ModuleData->CurrentStepCount);
		auto Filename = BuildFilename(ModuleData, Suffix);

		if (ModuleData->IsStepwiseScan || (!ModuleData->CurrentStepCount && !ModuleData->CurrentRepCount))
		{ 
			if (LaserInstrData->GetLaserState() == DynExpInstr::LaserData::LaserStateType::Ready ||
				LaserInstrData->GetLaserState() == DynExpInstr::LaserData::LaserStateType::EmissionEnabledConstant)
			{
				ModuleData->GetPLECommunicator()->PostEvent(*this, SetFilenameEvent{ Filename.string() });
				ModuleData->GetPLECommunicator()->PostEvent(*this, TriggerEvent{});

				ModuleData->CurrentStepCount++;
				ModuleData->LaserScanningSpectroscopyProgress++;

				return StateType::WaitForCapturing;
			}
			else
				return StateType::WaitForSettingFrequency;
		}
		else
		{
			if (!ModuleData->CurrentRepCount && ModuleData->CurrentStepCount == 1)
			{
				auto ModuleParams = DynExp::dynamic_Params_cast<LaserScanningSpectroscopy>(Instance.ParamsGetter());

				ModuleData->GetLaser()->SetScanRange(ModuleData->FrequencyRange);
				ModuleData->GetLaser()->SetScanRate(ModuleData->FrequencyRange / (2 * ModuleData->NumberOfSteps * ModuleParams->CapturingTimeDifference));
				
				// Turn on continuous scan mode.
				ModuleData->GetLaser()->ScanContinuously();
			}
			
			if (std::chrono::system_clock::now() < WaitingEndTimePoint)
				return StateType::WaitForSettingFrequency;

			ModuleData->LaserScanningSpectroscopyProgress++;
			ModuleData->CurrentStepCount++;
		
			ModuleData->GetPLECommunicator()->PostEvent(*this, SetFilenameEvent{ Filename.string() }); 
			ModuleData->GetPLECommunicator()->PostEvent(*this, TriggerEvent{});
		
			return StateType::WaitForCapturing;
		}
	}

	StateType LaserScanningSpectroscopy::WaitForCapturingStateFunc(DynExp::ModuleInstance& Instance)
	{
		return StateType::WaitForCapturing;
	}
}