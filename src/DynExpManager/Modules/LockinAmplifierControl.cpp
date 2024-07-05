// This file is part of DynExp.

#include "stdafx.h"
#include "moc_LockinAmplifierControl.cpp"
#include "LockinAmplifierControl.h"

namespace DynExpModule
{
	LockinAmplifierControlWidget::LockinAmplifierControlWidget(LockinAmplifierControl& Owner, QModuleWidget* parent) : QModuleWidget(Owner, parent)
	{
		ui.setupUi(this);
	}

	void LockinAmplifierControlWidget::InitializeUI(Util::SynchronizedPointer<LockinAmplifierControlData>& ModuleData)
	{
		if (!GetUIInitialized())
		{
			ui.SBRange->setSuffix(" m" + QString::fromStdString(ModuleData->SensitivityUnitString));
			ui.LEOscillatorFreq->setFocus();

			UIInitialized = true;
		}
	}

	void LockinAmplifierControlData::ResetImpl(dispatch_tag<QModuleDataBase>)
	{
		Init();
	}

	void LockinAmplifierControlData::Init()
	{
		CurrentSensitivity = 1;
		CurrentPhase = 0;
		CurrentTimeConstant = 1e-3;
		CurrentFilterOrder = 1;
		CurrentTriggerMode = DynExpInstr::LockinAmplifierDefs::TriggerModeType::ExternSingle;
		CurrentTriggerEdge = DynExpInstr::LockinAmplifierDefs::TriggerEdgeType::Fall;
		CurrentSignal = DynExpInstr::LockinAmplifierDefs::SignalType::R;
		CurrentSamplingRate = 1000;
		CurrentEnable = false;
		CurrentOverload = false;
		CurrentNegInputLoad = 0;
		CurrentPosInputLoad = 0;
		CurrentOscillatorFrequency = 0;
		CurrentAcquisitionProgress = -1;
		SensitivityUnitString = "";
	}

	const char* LockinAmplifierControl::ProgressBarRedStylesheet = "QProgressBar::chunk { background-color: rgb(255, 0, 0); }";
	const char* LockinAmplifierControl::ProgressBarGreenStylesheet = "QProgressBar::chunk { background-color: rgb(0, 170, 0); }";

	Util::DynExpErrorCodes::DynExpErrorCodes LockinAmplifierControl::ModuleMainLoop(DynExp::ModuleInstance& Instance)
	{
		try
		{
			auto ModuleData = DynExp::dynamic_ModuleData_cast<LockinAmplifierControl>(Instance.ModuleDataGetter());
			auto InstrData = DynExp::dynamic_InstrumentData_cast<DynExpInstr::LockinAmplifier>(ModuleData->GetLockinAmplifier()->GetInstrumentData());

			ModuleData->CurrentSensitivity = InstrData->GetSensitivity();
			ModuleData->CurrentPhase = InstrData->GetPhase();
			ModuleData->CurrentTimeConstant = InstrData->GetTimeConstant();
			ModuleData->CurrentFilterOrder = InstrData->GetFilterOrder();
			ModuleData->CurrentTriggerMode = InstrData->GetTriggerMode();
			ModuleData->CurrentTriggerEdge = InstrData->GetTriggerEdge();
			ModuleData->CurrentSignal = InstrData->GetSignalType();
			ModuleData->CurrentSamplingRate = InstrData->GetSamplingRate();
			ModuleData->CurrentEnable = InstrData->IsEnabled();
			ModuleData->CurrentOverload = InstrData->IsOverloaded();
			std::tie(ModuleData->CurrentNegInputLoad, ModuleData->CurrentPosInputLoad) = InstrData->GetInputLoad();
			ModuleData->CurrentOscillatorFrequency = InstrData->GetOscillatorFrequency();
			ModuleData->CurrentAcquisitionProgress = InstrData->GetAcquisitionProgress();

			NumFailedUpdateAttempts = 0;
		} // ModuleData and instruments' data unlocked here.
		catch (const Util::TimeoutException& e)
		{
			if (NumFailedUpdateAttempts++ >= 3)
				Instance.GetOwner().SetWarning(e);
		}

		return Util::DynExpErrorCodes::NoError;
	}

	void LockinAmplifierControl::ResetImpl(dispatch_tag<QModuleBase>)
	{
		NumFailedUpdateAttempts = 0;
	}

	std::unique_ptr<DynExp::QModuleWidget> LockinAmplifierControl::MakeUIWidget()
	{
		auto Widget = std::make_unique<LockinAmplifierControlWidget>(*this);

		// Connect Qt signals to module events
		Connect(Widget->GetUI().SBRange, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &LockinAmplifierControl::OnSensitivityChanged);
		Connect(Widget->GetUI().BAutoRange, &QPushButton::clicked, this, &LockinAmplifierControl::OnAutoRangeClicked);
		Connect(Widget->GetUI().SBPhase, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &LockinAmplifierControl::OnPhaseChanged);
		Connect(Widget->GetUI().BAutoPhase, &QPushButton::clicked, this, &LockinAmplifierControl::OnAutoPhaseClicked);
		Connect(Widget->GetUI().SBTimeConstant, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &LockinAmplifierControl::OnTimeConstantChanged);
		Connect(Widget->GetUI().SBFilterOrder, QOverload<int>::of(&QSpinBox::valueChanged), this, &LockinAmplifierControl::OnFilterOrderChanged);
		Connect(Widget->GetUI().CBTriggerMode, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &LockinAmplifierControl::OnTriggerModeChanged);
		Connect(Widget->GetUI().CBTriggerEdge, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &LockinAmplifierControl::OnTriggerEdgeChanged);
		Connect(Widget->GetUI().BForceTrigger, &QPushButton::clicked, this, &LockinAmplifierControl::OnForceTriggerClicked);
		Connect(Widget->GetUI().CBQuantity, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &LockinAmplifierControl::OnSignalTypeChanged);
		Connect(Widget->GetUI().SBSamplingRate, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &LockinAmplifierControl::OnSamplingRateChanged);
		Connect(Widget->GetUI().CBEnable, &QCheckBox::stateChanged, this, &LockinAmplifierControl::OnEnableClicked);
		Connect(Widget->GetUI().BPersist, &QPushButton::clicked, this, &LockinAmplifierControl::OnPersistParamsClicked);

		return Widget;
	}

	void LockinAmplifierControl::UpdateUIChild(const ModuleBase::ModuleDataGetterType& ModuleDataGetter)
	{
		auto Widget = GetWidget<LockinAmplifierControlWidget>();
		auto ModuleData = DynExp::dynamic_ModuleData_cast<LockinAmplifierControl>(ModuleDataGetter());

		Widget->InitializeUI(ModuleData);

		if (!Widget->GetUI().SBRange->hasFocus())
		{
			const QSignalBlocker Blocker(Widget->GetUI().SBRange);
			Widget->GetUI().SBRange->setValue(ModuleData->CurrentSensitivity * 1e3);
		}

		if (ModuleData->CurrentNegInputLoad >= 0)
		{
			Widget->GetUI().PBInputNegLoad->setVisible(true);
			Widget->GetUI().PBInputNegLoad->setValue(ModuleData->CurrentNegInputLoad * 100);
			Widget->GetUI().PBInputNegLoad->setStyleSheet(ModuleData->CurrentNegInputLoad >= 0.95 ? ProgressBarRedStylesheet : ProgressBarGreenStylesheet);
				
		}
		else
			Widget->GetUI().PBInputNegLoad->setVisible(false);

		if (ModuleData->CurrentPosInputLoad >= 0)
		{
			Widget->GetUI().PBInputPosLoad->setVisible(true);
			Widget->GetUI().PBInputPosLoad->setValue(ModuleData->CurrentPosInputLoad * 100);
			Widget->GetUI().PBInputPosLoad->setStyleSheet(ModuleData->CurrentPosInputLoad >= 0.95 ? ProgressBarRedStylesheet : ProgressBarGreenStylesheet);

		}
		else
			Widget->GetUI().PBInputPosLoad->setVisible(false);

		Widget->GetUI().LInputOverload->setText(ModuleData->CurrentOverload ? "Overload" : "");

		Widget->GetUI().LOscillatorFreq->setVisible(ModuleData->CurrentOscillatorFrequency > 0);
		Widget->GetUI().LEOscillatorFreq->setVisible(ModuleData->CurrentOscillatorFrequency > 0);
		if (ModuleData->CurrentOscillatorFrequency > 0)
			Widget->GetUI().LEOscillatorFreq->setText(QString::fromStdString(Util::ToStr(ModuleData->CurrentOscillatorFrequency, 2) + " Hz"));

		if (!Widget->GetUI().SBPhase->hasFocus())
		{
			const QSignalBlocker Blocker(Widget->GetUI().SBPhase);
			Widget->GetUI().SBPhase->setValue(ModuleData->CurrentPhase / std::numbers::pi * 180.0);
		}

		if (!Widget->GetUI().SBTimeConstant->hasFocus())
		{
			const QSignalBlocker Blocker(Widget->GetUI().SBTimeConstant);
			Widget->GetUI().SBTimeConstant->setValue(ModuleData->CurrentTimeConstant * 1e6);
		}

		if (!Widget->GetUI().SBFilterOrder->hasFocus())
		{
			const QSignalBlocker Blocker(Widget->GetUI().SBFilterOrder);
			Widget->GetUI().SBFilterOrder->setValue(ModuleData->CurrentFilterOrder);
		}

		if (!Widget->GetUI().CBTriggerMode->hasFocus())
		{
			const QSignalBlocker Blocker(Widget->GetUI().CBTriggerMode);
			Widget->GetUI().CBTriggerMode->setCurrentIndex(ModuleData->CurrentTriggerMode - 1);
		}

		if (!Widget->GetUI().CBTriggerEdge->hasFocus())
		{
			const QSignalBlocker Blocker(Widget->GetUI().CBTriggerEdge);
			Widget->GetUI().CBTriggerEdge->setCurrentIndex(ModuleData->CurrentTriggerEdge - 1);
		}

		if (!Widget->GetUI().CBQuantity->hasFocus())
		{
			const QSignalBlocker Blocker(Widget->GetUI().CBQuantity);
			Widget->GetUI().CBQuantity->setCurrentIndex(ModuleData->CurrentSignal);
		}

		if (!Widget->GetUI().SBSamplingRate->hasFocus())
		{
			const QSignalBlocker Blocker(Widget->GetUI().SBSamplingRate);
			Widget->GetUI().SBSamplingRate->setValue(ModuleData->CurrentSamplingRate);
		}

		if (!Widget->GetUI().CBEnable->hasFocus())
		{
			const QSignalBlocker Blocker(Widget->GetUI().CBEnable);
			Widget->GetUI().CBEnable->setChecked(ModuleData->CurrentEnable);
		}

		Widget->GetUI().LProgress->setVisible(ModuleData->CurrentAcquisitionProgress >= 0);
		Widget->GetUI().PProgress->setVisible(ModuleData->CurrentAcquisitionProgress >= 0);
		if (ModuleData->CurrentAcquisitionProgress >= 0)
			Widget->GetUI().PProgress->setValue(ModuleData->CurrentAcquisitionProgress * 100);
	}

	void LockinAmplifierControl::OnInit(DynExp::ModuleInstance* Instance) const
	{
		auto ModuleParams = DynExp::dynamic_Params_cast<LockinAmplifierControl>(Instance->ParamsGetter());
		auto ModuleData = DynExp::dynamic_ModuleData_cast<LockinAmplifierControl>(Instance->ModuleDataGetter());

		Instance->LockObject(ModuleParams->LockinAmplifier, ModuleData->GetLockinAmplifier());

		ModuleData->SensitivityUnitString = ModuleData->GetLockinAmplifier()->GetSensitivityUnitString();
	}

	void LockinAmplifierControl::OnExit(DynExp::ModuleInstance* Instance) const
	{
		auto ModuleData = DynExp::dynamic_ModuleData_cast<LockinAmplifierControl>(Instance->ModuleDataGetter());

		Instance->UnlockObject(ModuleData->GetLockinAmplifier());
	}

	void LockinAmplifierControl::OnSensitivityChanged(DynExp::ModuleInstance* Instance, double Value) const
	{
		auto ModuleData = DynExp::dynamic_ModuleData_cast<LockinAmplifierControl>(Instance->ModuleDataGetter());
		ModuleData->GetLockinAmplifier()->SetSensitivity(Value / 1e3);
	}

	void LockinAmplifierControl::OnAutoRangeClicked(DynExp::ModuleInstance* Instance, bool) const
	{
		auto ModuleData = DynExp::dynamic_ModuleData_cast<LockinAmplifierControl>(Instance->ModuleDataGetter());
		ModuleData->GetLockinAmplifier()->AutoAdjustSensitivity();
	}

	void LockinAmplifierControl::OnPhaseChanged(DynExp::ModuleInstance* Instance, double Value) const
	{
		auto ModuleData = DynExp::dynamic_ModuleData_cast<LockinAmplifierControl>(Instance->ModuleDataGetter());
		ModuleData->GetLockinAmplifier()->SetPhase(Value / 180.0 * std::numbers::pi);
	}

	void LockinAmplifierControl::OnAutoPhaseClicked(DynExp::ModuleInstance* Instance, bool) const
	{
		auto ModuleData = DynExp::dynamic_ModuleData_cast<LockinAmplifierControl>(Instance->ModuleDataGetter());
		ModuleData->GetLockinAmplifier()->AutoAdjustPhase();
	}

	void LockinAmplifierControl::OnTimeConstantChanged(DynExp::ModuleInstance* Instance, double Value) const
	{
		auto ModuleData = DynExp::dynamic_ModuleData_cast<LockinAmplifierControl>(Instance->ModuleDataGetter());
		ModuleData->GetLockinAmplifier()->SetTimeConstant(Value / 1e6);
	}

	void LockinAmplifierControl::OnFilterOrderChanged(DynExp::ModuleInstance* Instance, int Value) const
	{
		auto ModuleData = DynExp::dynamic_ModuleData_cast<LockinAmplifierControl>(Instance->ModuleDataGetter());
		ModuleData->GetLockinAmplifier()->SetFilterOrder(Value);
	}

	void LockinAmplifierControl::OnTriggerModeChanged(DynExp::ModuleInstance* Instance, int Index) const
	{
		auto ModuleData = DynExp::dynamic_ModuleData_cast<LockinAmplifierControl>(Instance->ModuleDataGetter());
		ModuleData->GetLockinAmplifier()->SetTriggerMode(static_cast<DynExpInstr::LockinAmplifierDefs::TriggerModeType>(Index + 1));
	}

	void LockinAmplifierControl::OnTriggerEdgeChanged(DynExp::ModuleInstance* Instance, int Index) const
	{
		auto ModuleData = DynExp::dynamic_ModuleData_cast<LockinAmplifierControl>(Instance->ModuleDataGetter());
		ModuleData->GetLockinAmplifier()->SetTriggerEdge(static_cast<DynExpInstr::LockinAmplifierDefs::TriggerEdgeType>(Index + 1));
	}

	void LockinAmplifierControl::OnForceTriggerClicked(DynExp::ModuleInstance* Instance, bool) const
	{
		auto ModuleData = DynExp::dynamic_ModuleData_cast<LockinAmplifierControl>(Instance->ModuleDataGetter());
		ModuleData->GetLockinAmplifier()->ForceTrigger();
	}

	void LockinAmplifierControl::OnSignalTypeChanged(DynExp::ModuleInstance* Instance, int Index) const
	{
		auto ModuleData = DynExp::dynamic_ModuleData_cast<LockinAmplifierControl>(Instance->ModuleDataGetter());
		ModuleData->GetLockinAmplifier()->SetSignalType(static_cast<DynExpInstr::LockinAmplifierDefs::SignalType>(Index));
	}

	void LockinAmplifierControl::OnSamplingRateChanged(DynExp::ModuleInstance* Instance, double Value) const
	{
		auto ModuleData = DynExp::dynamic_ModuleData_cast<LockinAmplifierControl>(Instance->ModuleDataGetter());
		ModuleData->GetLockinAmplifier()->SetSamplingRate(Value);
	}

	void LockinAmplifierControl::OnEnableClicked(DynExp::ModuleInstance* Instance, int Value) const
	{
		auto ModuleData = DynExp::dynamic_ModuleData_cast<LockinAmplifierControl>(Instance->ModuleDataGetter());
		ModuleData->GetLockinAmplifier()->SetEnable(Value);
	}

	void LockinAmplifierControl::OnPersistParamsClicked(DynExp::ModuleInstance* Instance, bool) const
	{
		auto ModuleData = DynExp::dynamic_ModuleData_cast<LockinAmplifierControl>(Instance->ModuleDataGetter());
		ModuleData->GetLockinAmplifier()->PersistDataToParams();
	}
}