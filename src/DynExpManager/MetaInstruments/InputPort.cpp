// This file is part of DynExp.

#include "stdafx.h"
#include "InputPort.h"

namespace DynExpInstr
{
	void InputPortTasks::InitTask::InitFuncImpl(dispatch_tag<DataStreamInstrumentTasks::InitTask>, DynExp::InstrumentInstance& Instance)
	{
		InitFuncImpl(dispatch_tag<InitTask>(), Instance);

		// ...after the entire instrument has been initialized.
		auto Instr = DynExp::dynamic_Object_cast<InputPort>(&Instance.GetOwner());

		if (ApplyDataStreamSizeFromParams())
		{
			const auto StreamSize = Instr->GetStreamSizeParams().StreamSize;
			auto InstrData = DynExp::dynamic_InstrumentData_cast<InputPort>(Instance.InstrumentDataGetter());

			InstrData->GetSampleStream()->SetStreamSize(StreamSize);
		} // InstrData unlocked here.

		auto InstrData = DynExp::dynamic_InstrumentData_cast<InputPort>(Instance.InstrumentDataGetter());
		InstrData->SetHardwareMinValue(Instr->GetHardwareMinValue());
		InstrData->SetHardwareMaxValue(Instr->GetHardwareMaxValue());
		InstrData->SetValueUnit(Instr->GetValueUnit());
	}

	DynExp::TaskResultType DynExpInstr::InputPortTasks::ResetBufferSizeTask::RunChild(DynExp::InstrumentInstance& Instance)
	{
		auto Instr = DynExp::dynamic_Object_cast<InputPort>(&Instance.GetOwner());
		const auto StreamSize = Instr->GetStreamSizeParams().StreamSize;
		auto InstrData = DynExp::dynamic_InstrumentData_cast<InputPort>(Instance.InstrumentDataGetter());

		InstrData->GetSampleStream()->SetStreamSize(StreamSize);

		return {};
	}

	void InputPortData::ResetImpl(dispatch_tag<DataStreamInstrumentData>)
	{
		ResetImpl(dispatch_tag<InputPortData>());
	}

	InputPortParams::~InputPortParams()
	{
	}

	void InputPortParams::DisableUserEditable()
	{
	}

	InputPortConfigurator::~InputPortConfigurator()
	{
	}

	InputPort::~InputPort()
	{
	}

	void InputPort::ResetImpl(dispatch_tag<DataStreamInstrument>)
	{
		ResetImpl(dispatch_tag<InputPort>());
	}
}