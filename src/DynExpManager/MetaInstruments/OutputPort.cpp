// This file is part of DynExp.

#include "stdafx.h"
#include "OutputPort.h"

namespace DynExpInstr
{
	void OutputPortTasks::InitTask::InitFuncImpl(dispatch_tag<FunctionGeneratorTasks::InitTask>, DynExp::InstrumentInstance& Instance)
	{
		InitFuncImpl(dispatch_tag<InitTask>(), Instance);

		// ...after the entire instrument has been initialized.
		auto Instr = DynExp::dynamic_Object_cast<OutputPort>(&Instance.GetOwner());

		if (ApplyDataStreamSizeFromParams())
		{
			const auto StreamSize = Instr->GetStreamSizeParams().StreamSize;
			auto InstrData = DynExp::dynamic_InstrumentData_cast<OutputPort>(Instance.InstrumentDataGetter());

			InstrData->GetSampleStream()->SetStreamSize(StreamSize);
		} // InstrData unlocked here.
		
		auto InstrData = DynExp::dynamic_InstrumentData_cast<OutputPort>(Instance.InstrumentDataGetter());
		InstrData->SetHardwareMinValue(Instr->GetHardwareMinValue());
		InstrData->SetHardwareMaxValue(Instr->GetHardwareMaxValue());
		InstrData->SetValueUnit(Instr->GetValueUnit());
	}

	DynExp::TaskResultType DynExpInstr::OutputPortTasks::ResetBufferSizeTask::RunChild(DynExp::InstrumentInstance& Instance)
	{
		auto Instr = DynExp::dynamic_Object_cast<OutputPort>(&Instance.GetOwner());
		const auto StreamSize = Instr->GetStreamSizeParams().StreamSize;
		auto InstrData = DynExp::dynamic_InstrumentData_cast<OutputPort>(Instance.InstrumentDataGetter());

		InstrData->GetSampleStream()->SetStreamSize(StreamSize);

		return {};
	}

	void OutputPortData::ResetImpl(dispatch_tag<FunctionGeneratorData>)
	{
		ResetImpl(dispatch_tag<OutputPortData>());
	}

	OutputPortParams::~OutputPortParams()
	{
	}

	void OutputPortParams::DisableUserEditable()
	{
	}

	OutputPortConfigurator::~OutputPortConfigurator()
	{
	}

	OutputPort::~OutputPort()
	{
	}

	void OutputPort::ResetImpl(dispatch_tag<FunctionGenerator>)
	{
		ResetImpl(dispatch_tag<OutputPort>());
	}
}