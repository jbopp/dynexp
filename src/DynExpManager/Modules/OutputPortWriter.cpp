// This file is part of DynExp.

#include "stdafx.h"
#include "moc_OutputPortWriter.cpp"
#include "OutputPortWriter.h"

namespace DynExpModule
{
	OutputPortWriterWidget::OutputPortWriterWidget(OutputPortWriter& Owner, QModuleWidget* parent) : QModuleWidget(Owner, parent)
	{
		ui.setupUi(this);

		ui.AnalogOutWidget->setVisible(false);
		ui.DigitalOutWidget->setVisible(false);
	}

	void OutputPortWriterData::ResetImpl(dispatch_tag<QModuleDataBase>)
	{
		Init();
	}

	void OutputPortWriterData::Init()
	{
		UIInitialized = false;
		IsDigitalPort = false;
		ValueChanged = false;
		MinAllowedValue = 0;
		MaxAllowedValue = 1;
		Value = 0;
	}

	Util::DynExpErrorCodes::DynExpErrorCodes OutputPortWriter::ModuleMainLoop(DynExp::ModuleInstance& Instance)
	{
		auto ModuleData = DynExp::dynamic_ModuleData_cast<OutputPortWriter>(Instance.ModuleDataGetter());

		if (ModuleData->ValueChanged)
		{
			{
				auto InstrData = DynExp::dynamic_InstrumentData_cast<DynExpInstr::OutputPort>(ModuleData->OutputPort->GetInstrumentData());

				InstrData->GetSampleStream()->SeekEnd(std::ios_base::out);
				InstrData->GetSampleStream()->WriteBasicSample({ ModuleData->Value });
			} // InstrData unlocked here.
			ModuleData->OutputPort->WriteData();

			ModuleData->ValueChanged = false;
		}

		return Util::DynExpErrorCodes::NoError;
	}

	void OutputPortWriter::ResetImpl(dispatch_tag<QModuleBase>)
	{
	}

	std::unique_ptr<DynExp::QModuleWidget> OutputPortWriter::MakeUIWidget()
	{
		auto Widget = std::make_unique<OutputPortWriterWidget>(*this);

		Connect(Widget->ui.ValueDial, &QAbstractSlider::valueChanged, this, &OutputPortWriter::OnValueChanged);
		Connect(Widget->ui.StateButton, &QPushButton::clicked, this, &OutputPortWriter::OnStateButtonClicked);

		return Widget;
	}

	void OutputPortWriter::UpdateUIChild(const ModuleBase::ModuleDataGetterType& ModuleDataGetter)
	{
		auto Widget = GetWidget<OutputPortWriterWidget>();
		auto ModuleData = DynExp::dynamic_ModuleData_cast<OutputPortWriter>(ModuleDataGetter());

		QString ValueUnitStr(" ");
		ValueUnitStr += DynExpInstr::DataStreamInstrumentData::UnitTypeToStr(ModuleData->OutputPort->GetValueUnit());

		if (!ModuleData->UIInitialized)
		{
			Widget->ui.AnalogOutWidget->setVisible(!ModuleData->IsDigitalPort);
			Widget->ui.DigitalOutWidget->setVisible(ModuleData->IsDigitalPort);
			Widget->adjustSize();

			Widget->ui.ValueDial->setMinimum(ModuleData->MinAllowedValue * DialControlValueDivider);
			Widget->ui.ValueDial->setMaximum(ModuleData->MaxAllowedValue * DialControlValueDivider);
			Widget->ui.ValueDial->setSingleStep(ModuleData->MaxAllowedValue - ModuleData->MinAllowedValue);
			Widget->ui.ValueDial->setPageStep((ModuleData->MaxAllowedValue - ModuleData->MinAllowedValue) * DialControlValueDivider / 10);
			Widget->ui.MinValueLabel->setText(QString::number(ModuleData->MinAllowedValue) + ValueUnitStr);
			Widget->ui.MaxValueLabel->setText(QString::number(ModuleData->MaxAllowedValue) + ValueUnitStr);

			ModuleData->UIInitialized = true;
		}

		if (!ModuleData->IsDigitalPort)
		{
			if (!Widget->ui.ValueDial->hasFocus())
				Widget->ui.ValueDial->setValue(ModuleData->Value * DialControlValueDivider);
			
			Widget->ui.ValueLabel->setText(QString::number(ModuleData->Value) + ValueUnitStr);
			
		}
		else
		{
			Widget->ui.StateButton->setText(ModuleData->Value ? "High" : "Low");
			Widget->ui.StateButton->setStyleSheet(ModuleData->Value ? "background-color: lime;" : "background-color: red;");
		}
	}

	void OutputPortWriter::OnInit(DynExp::ModuleInstance* Instance) const
	{
		auto ModuleParams = DynExp::dynamic_Params_cast<OutputPortWriter>(Instance->ParamsGetter());
		auto ModuleData = DynExp::dynamic_ModuleData_cast<OutputPortWriter>(Instance->ModuleDataGetter());

		Instance->LockObject(ModuleParams->OutputPort, ModuleData->OutputPort);

		ModuleData->IsDigitalPort = ModuleData->OutputPort->GetValueUnit() == DynExpInstr::DataStreamInstrumentData::UnitType::LogicLevel;
		ModuleData->MinAllowedValue = ModuleData->OutputPort->GetUserMinValue();
		ModuleData->MaxAllowedValue = ModuleData->OutputPort->GetUserMaxValue();
	}

	void OutputPortWriter::OnExit(DynExp::ModuleInstance* Instance) const
	{
		auto ModuleData = DynExp::dynamic_ModuleData_cast<OutputPortWriter>(Instance->ModuleDataGetter());

		Instance->UnlockObject(ModuleData->OutputPort);
	}

	void OutputPortWriter::OnValueChanged(DynExp::ModuleInstance* Instance, int Value) const
	{
		auto ModuleData = DynExp::dynamic_ModuleData_cast<OutputPortWriter>(Instance->ModuleDataGetter());
		ModuleData->Value = static_cast<double>(Value) / DialControlValueDivider;
		ModuleData->ValueChanged = true;
	}

	void OutputPortWriter::OnStateButtonClicked(DynExp::ModuleInstance* Instance, bool Checked) const
	{
		auto ModuleData = DynExp::dynamic_ModuleData_cast<OutputPortWriter>(Instance->ModuleDataGetter());
		ModuleData->Value = Checked;
		ModuleData->ValueChanged = true;
	}
}