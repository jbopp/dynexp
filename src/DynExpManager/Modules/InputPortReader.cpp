// This file is part of DynExp.

#include "stdafx.h"
#include "moc_InputPortReader.cpp"
#include "InputPortReader.h"

namespace DynExpModule
{
	InputPortReaderWidget::InputPortReaderWidget(InputPortReader& Owner, QModuleWidget* parent) : QModuleWidget(Owner, parent)
	{
		ui.setupUi(this);

		ui.AnalogInWidget->setVisible(false);
		ui.DigitalInWidget->setVisible(false);
	}

	void InputPortReaderData::ResetImpl(dispatch_tag<QModuleDataBase>)
	{
		Init();
	}

	void InputPortReaderData::Init()
	{
		UIInitialized = false;
		IsDigitalPort = false;
		Value = 0;
	}

	Util::DynExpErrorCodes::DynExpErrorCodes InputPortReader::ModuleMainLoop(DynExp::ModuleInstance& Instance)
	{
		try
		{
			auto ModuleData = DynExp::dynamic_ModuleData_cast<InputPortReader>(Instance.ModuleDataGetter());

			{
				auto InstrData = DynExp::dynamic_InstrumentData_cast<DynExpInstr::InputPort>(ModuleData->InputPort->GetInstrumentData());

				if (InstrData->GetSampleStream()->CanRead())
					ModuleData->Value = InstrData->GetSampleStream()->ReadBasicSample().Value;
			} // InstrData unlocked here.
			ModuleData->InputPort->ReadData();

			NumFailedUpdateAttempts = 0;
		} // ModuleData unlocked here.
		catch (const Util::TimeoutException& e)
		{
			if (NumFailedUpdateAttempts++ >= 3)
				Instance.GetOwner().SetWarning(e);
		}

		return Util::DynExpErrorCodes::NoError;
	}

	void InputPortReader::ResetImpl(dispatch_tag<QModuleBase>)
	{
		NumFailedUpdateAttempts = 0;
	}

	std::unique_ptr<DynExp::QModuleWidget> InputPortReader::MakeUIWidget()
	{
		auto Widget = std::make_unique<InputPortReaderWidget>(*this);

		return Widget;
	}

	void InputPortReader::UpdateUIChild(const ModuleBase::ModuleDataGetterType& ModuleDataGetter)
	{
		auto Widget = GetWidget<InputPortReaderWidget>();
		auto ModuleData = DynExp::dynamic_ModuleData_cast<InputPortReader>(ModuleDataGetter());

		if (!ModuleData->UIInitialized)
		{
			Widget->ui.AnalogInWidget->setVisible(!ModuleData->IsDigitalPort);
			Widget->ui.DigitalInWidget->setVisible(ModuleData->IsDigitalPort);
			Widget->adjustSize();

			ModuleData->UIInitialized = true;
		}

		if (!ModuleData->IsDigitalPort)
		{
			Widget->ui.ValueLabel->setText(QString::number(ModuleData->Value) + " " + DynExpInstr::DataStreamInstrumentData::UnitTypeToStr(ModuleData->InputPort->GetValueUnit()));
			Widget->ui.ValueProgressBar->setMinimum(ModuleData->InputPort->GetHardwareMinValue());
			Widget->ui.ValueProgressBar->setMaximum(ModuleData->InputPort->GetHardwareMaxValue());
			Widget->ui.ValueProgressBar->setValue(ModuleData->Value);
		}
		else
		{
			Widget->ui.StateLabel->setText(ModuleData->Value ? "High" : "Low");
			Widget->ui.StateFrame->setStyleSheet(ModuleData->Value ? "background-color: lime;" : "background-color: red;");
		}
	}

	void InputPortReader::OnInit(DynExp::ModuleInstance* Instance) const
	{
		auto ModuleParams = DynExp::dynamic_Params_cast<InputPortReader>(Instance->ParamsGetter());
		auto ModuleData = DynExp::dynamic_ModuleData_cast<InputPortReader>(Instance->ModuleDataGetter());

		Instance->LockObject(ModuleParams->InputPort, ModuleData->InputPort);
		ModuleData->InputPort->ReadData();

		ModuleData->IsDigitalPort = ModuleData->InputPort->GetValueUnit() == DynExpInstr::DataStreamInstrumentData::UnitType::LogicLevel;
	}

	void InputPortReader::OnExit(DynExp::ModuleInstance* Instance) const
	{
		auto ModuleData = DynExp::dynamic_ModuleData_cast<InputPortReader>(Instance->ModuleDataGetter());

		Instance->UnlockObject(ModuleData->InputPort);
	}
}