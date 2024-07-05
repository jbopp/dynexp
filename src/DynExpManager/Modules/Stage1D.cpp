// This file is part of DynExp.

#include "stdafx.h"
#include "moc_Stage1D.cpp"
#include "Stage1D.h"

namespace DynExpModule
{
	Stage1DWidget::Stage1DWidget(Stage1D& Owner, QModuleWidget* parent) : QModuleWidget(Owner, parent)
	{
		ui.setupUi(this);
		ui.SBPosition->blockSignals(true);

		// For shortcuts
		this->addAction(ui.action_Stop_current_action);
	}

	void Stage1DData::ResetImpl(dispatch_tag<QModuleDataBase>)
	{
		Init();
	}

	void Stage1DData::Init()
	{
		Velocity = 0;
		Position = 0;
		IsUsingSIUnits = true;
		IsMoving = false;
		HasFailed = false;
		LabelsUpdated = false;
	}

	Util::DynExpErrorCodes::DynExpErrorCodes Stage1D::ModuleMainLoop(DynExp::ModuleInstance& Instance)
	{
		try
		{
			auto ModuleData = DynExp::dynamic_ModuleData_cast<Stage1D>(Instance.ModuleDataGetter());
			auto StageData = DynExp::dynamic_InstrumentData_cast<DynExpInstr::PositionerStage>(ModuleData->PositionerStage->GetInstrumentData());

			ModuleData->Velocity = StageData->GetVelocity() * ModuleData->PositionerStage->GetStepNanoMeterRatio();
			ModuleData->Position = StageData->GetCurrentPosition() * ModuleData->PositionerStage->GetStepNanoMeterRatio();
			ModuleData->IsUsingSIUnits = ModuleData->PositionerStage->IsUsingSIUnits();
			ModuleData->IsMoving = StageData->IsMoving();
			ModuleData->HasFailed = StageData->HasFailed();

			NumFailedUpdateAttempts = 0;
		} // ModuleData and StageData unlocked here.
		catch (const Util::TimeoutException& e)
		{
			if (NumFailedUpdateAttempts++ >= 3)
				Instance.GetOwner().SetWarning(e);
		}

		return Util::DynExpErrorCodes::NoError;
	}

	void Stage1D::ResetImpl(dispatch_tag<QModuleBase>)
	{
		NumFailedUpdateAttempts = 0;
	}

	std::unique_ptr<DynExp::QModuleWidget> Stage1D::MakeUIWidget()
	{
		auto Widget = std::make_unique<Stage1DWidget>(*this);

		Connect(Widget->ui.action_Stop_current_action, &QAction::triggered, this, &Stage1D::OnStopClicked);
		Connect(Widget->ui.ButtonReference, &QPushButton::clicked, this, &Stage1D::OnFindReferenceClicked);
		Connect(Widget->ui.ButtonSetHome, &QPushButton::clicked, this, &Stage1D::OnSetHomeClicked);
		Connect(Widget->ui.ButtonCalibrate, &QPushButton::clicked, this, &Stage1D::OnCalibrateClicked);
		Connect(Widget->ui.ButtonFirst, &QPushButton::clicked, this, &Stage1D::OnMoveFirstClicked);
		Connect(Widget->ui.ButtonLast, &QPushButton::clicked, this, &Stage1D::OnMoveLastClicked);
		Connect(Widget->ui.ButtonLeft, &QPushButton::clicked, this, &Stage1D::OnMoveLeftClicked);
		Connect(Widget->ui.ButtonRight, &QPushButton::clicked, this, &Stage1D::OnMoveRightClicked);
		Connect(Widget->ui.ButtonHome, &QPushButton::clicked, this, &Stage1D::OnMoveHomeClicked);
		Connect(Widget->ui.ButtonStop, &QPushButton::clicked, this, &Stage1D::OnStopClicked);
		Connect(Widget->ui.SBVelocity, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &Stage1D::OnVelocityValueChanged);
		Connect(Widget->ui.SBPosition, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &Stage1D::OnPositionValueChanged);

		return Widget;
	}

	void Stage1D::UpdateUIChild(const ModuleBase::ModuleDataGetterType& ModuleDataGetter)
	{
		auto Widget = GetWidget<Stage1DWidget>();
		auto ModuleData = DynExp::dynamic_ModuleData_cast<Stage1D>(ModuleDataGetter());

		if (!Widget->ui.SBVelocity->hasFocus())
		{
			const QSignalBlocker Blocker(Widget->ui.SBVelocity);
			Widget->ui.SBVelocity->setValue(ModuleData->Velocity);
		}
		if (!Widget->ui.SBPosition->hasFocus())
		{
			const QSignalBlocker Blocker(Widget->ui.SBPosition);
			Widget->ui.SBPosition->setValue(ModuleData->Position);
		}
		
		Widget->ui.CBMoving->setChecked(ModuleData->IsMoving);
		Widget->ui.CBErrorState->setChecked(ModuleData->HasFailed);

		if (!ModuleData->LabelsUpdated)
		{
			ModuleData->LabelsUpdated = true;
			Widget->ui.LVelocity->setText("Velocity (" + QString(ModuleData->IsUsingSIUnits ? "nm/s" : "steps/s") + ")");
			Widget->ui.SBVelocity->setMaximum(ModuleData->PositionerStage->GetMaxVelocity());
			Widget->ui.SBVelocity->setMinimum(ModuleData->PositionerStage->GetMinVelocity());
			Widget->ui.SBVelocity->setDecimals(0);
			Widget->ui.SBVelocity->setValue(ModuleData->PositionerStage->GetDefaultVelocity());
			Widget->ui.LPosition->setText("Position (" + QString(ModuleData->IsUsingSIUnits ? "nm" : "steps") + ")");
			Widget->ui.SBPosition->setMaximum(ModuleData->PositionerStage->GetMaxPosition());
			Widget->ui.SBPosition->setMinimum(ModuleData->PositionerStage->GetMinPosition());
			Widget->ui.SBPosition->setDecimals(0);

			// Now unblock signals (if they weren't blocked initially, stage would move to 0)
			Widget->ui.SBPosition->blockSignals(false);
		}
	}

	void Stage1D::OnInit(DynExp::ModuleInstance* Instance) const
	{
		auto ModuleParams = DynExp::dynamic_Params_cast<Stage1D>(Instance->ParamsGetter());
		auto ModuleData = DynExp::dynamic_ModuleData_cast<Stage1D>(Instance->ModuleDataGetter());

		Instance->LockObject(ModuleParams->PositionerStage, ModuleData->PositionerStage);
	}

	void Stage1D::OnExit(DynExp::ModuleInstance* Instance) const
	{
		auto ModuleData = DynExp::dynamic_ModuleData_cast<Stage1D>(Instance->ModuleDataGetter());

		Instance->UnlockObject(ModuleData->PositionerStage);
	}

	void Stage1D::OnFindReferenceClicked(DynExp::ModuleInstance* Instance, bool) const
	{
		auto ModuleData = DynExp::dynamic_ModuleData_cast<Stage1D>(Instance->ModuleDataGetter());
		ModuleData->PositionerStage->Reference();
	}

	void Stage1D::OnSetHomeClicked(DynExp::ModuleInstance* Instance, bool) const
	{
		auto ModuleData = DynExp::dynamic_ModuleData_cast<Stage1D>(Instance->ModuleDataGetter());
		ModuleData->PositionerStage->SetHome();
	}

	void Stage1D::OnCalibrateClicked(DynExp::ModuleInstance* Instance, bool) const
	{
		auto ModuleData = DynExp::dynamic_ModuleData_cast<Stage1D>(Instance->ModuleDataGetter());
		ModuleData->PositionerStage->Calibrate();
	}

	void Stage1D::OnMoveFirstClicked(DynExp::ModuleInstance* Instance, bool) const
	{
		auto ModuleData = DynExp::dynamic_ModuleData_cast<Stage1D>(Instance->ModuleDataGetter());
		ModuleData->PositionerStage->MoveAbsolute(ModuleData->PositionerStage->GetMinPosition());
	}

	void Stage1D::OnMoveLastClicked(DynExp::ModuleInstance* Instance, bool) const
	{
		auto ModuleData = DynExp::dynamic_ModuleData_cast<Stage1D>(Instance->ModuleDataGetter());
		ModuleData->PositionerStage->MoveAbsolute(ModuleData->PositionerStage->GetMaxPosition());
	}

	void Stage1D::OnMoveLeftClicked(DynExp::ModuleInstance* Instance, bool) const
	{
		auto ModuleData = DynExp::dynamic_ModuleData_cast<Stage1D>(Instance->ModuleDataGetter());
		ModuleData->PositionerStage->MoveRelative(-ModuleData->Velocity / ModuleData->PositionerStage->GetStepNanoMeterRatio() * .1);
	}

	void Stage1D::OnMoveRightClicked(DynExp::ModuleInstance* Instance, bool) const
	{
		auto ModuleData = DynExp::dynamic_ModuleData_cast<Stage1D>(Instance->ModuleDataGetter());
		ModuleData->PositionerStage->MoveRelative(ModuleData->Velocity / ModuleData->PositionerStage->GetStepNanoMeterRatio() * .1);
	}

	void Stage1D::OnMoveHomeClicked(DynExp::ModuleInstance* Instance, bool) const
	{
		auto ModuleData = DynExp::dynamic_ModuleData_cast<Stage1D>(Instance->ModuleDataGetter());
		ModuleData->PositionerStage->MoveToHome();
	}

	void Stage1D::OnStopClicked(DynExp::ModuleInstance* Instance, bool) const
	{
		auto ModuleData = DynExp::dynamic_ModuleData_cast<Stage1D>(Instance->ModuleDataGetter());
		ModuleData->PositionerStage->StopMotion();
	}

	void Stage1D::OnVelocityValueChanged(DynExp::ModuleInstance* Instance, const double Value) const
	{
		auto ModuleData = DynExp::dynamic_ModuleData_cast<Stage1D>(Instance->ModuleDataGetter());
		ModuleData->PositionerStage->SetVelocity(Value / ModuleData->PositionerStage->GetStepNanoMeterRatio());
	}

	void Stage1D::OnPositionValueChanged(DynExp::ModuleInstance* Instance, const double Value) const
	{
		auto ModuleData = DynExp::dynamic_ModuleData_cast<Stage1D>(Instance->ModuleDataGetter());
		ModuleData->PositionerStage->MoveAbsolute(Value / ModuleData->PositionerStage->GetStepNanoMeterRatio());
	}
}