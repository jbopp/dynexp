// This file is part of DynExp.

#include "stdafx.h"
#include "NIDAQAnalogIn.h"

namespace DynExpInstr
{
	void NIDAQAnalogInTasks::InitTask::InitFuncImpl(dispatch_tag<AnalogInTasks::InitTask>, DynExp::InstrumentInstance& Instance)
	{
		auto InstrParams = DynExp::dynamic_Params_cast<NIDAQAnalogIn>(Instance.ParamsGetter());
		auto InstrData = DynExp::dynamic_InstrumentData_cast<NIDAQAnalogIn>(Instance.InstrumentDataGetter());

		Instance.LockObject(InstrParams->HardwareAdapter, InstrData->HardwareAdapter);
		InstrData->ChannelHandle = InstrData->HardwareAdapter->InitializeAnalogInChannel(InstrParams->ChannelName.Get(),
			NIDAQAnalogIn::HardwareMinValue(), NIDAQAnalogIn::HardwareMaxValue(), 0, InstrParams->TerminalConfig);

		InitFuncImpl(dispatch_tag<InitTask>(), Instance);
	}

	void NIDAQAnalogInTasks::ExitTask::ExitFuncImpl(dispatch_tag<AnalogInTasks::ExitTask>, DynExp::InstrumentInstance& Instance)
	{
		ExitFuncImpl(dispatch_tag<ExitTask>(), Instance);

		auto InstrData = DynExp::dynamic_InstrumentData_cast<NIDAQAnalogIn>(Instance.InstrumentDataGetter());

		InstrData->HardwareAdapter->DeregisterChannel(InstrData->ChannelHandle);
		Instance.UnlockObject(InstrData->HardwareAdapter);
	}

	void NIDAQAnalogInTasks::UpdateTask::UpdateFuncImpl(dispatch_tag<AnalogInTasks::UpdateTask>, DynExp::InstrumentInstance& Instance)
	{
		UpdateFuncImpl(dispatch_tag<UpdateTask>(), Instance);
	}

	DynExp::TaskResultType NIDAQAnalogInTasks::ReadTask::RunChild(DynExp::InstrumentInstance& Instance)
	{
		auto InstrData = DynExp::dynamic_InstrumentData_cast<NIDAQAnalogIn>(Instance.InstrumentDataGetter());

		auto SampleStream = InstrData->GetCastSampleStream<NIDAQAnalogInData::SampleStreamType>();
		SampleStream->WriteSamples(InstrData->HardwareAdapter->ReadAnalogValues(InstrData->ChannelHandle));

		return {};
	}

	DynExp::TaskResultType NIDAQAnalogInTasks::StartTask::RunChild(DynExp::InstrumentInstance& Instance)
	{
		auto InstrData = DynExp::dynamic_InstrumentData_cast<NIDAQAnalogIn>(Instance.InstrumentDataGetter());

		InstrData->HardwareAdapter->StartTask(InstrData->ChannelHandle);

		return {};
	}

	DynExp::TaskResultType NIDAQAnalogInTasks::StopTask::RunChild(DynExp::InstrumentInstance& Instance)
	{
		auto InstrData = DynExp::dynamic_InstrumentData_cast<NIDAQAnalogIn>(Instance.InstrumentDataGetter());

		InstrData->HardwareAdapter->StopTask(InstrData->ChannelHandle);

		return {};
	}

	DynExp::TaskResultType NIDAQAnalogInTasks::RestartTask::RunChild(DynExp::InstrumentInstance& Instance)
	{
		auto InstrData = DynExp::dynamic_InstrumentData_cast<NIDAQAnalogIn>(Instance.InstrumentDataGetter());

		InstrData->HardwareAdapter->RestartTask(InstrData->ChannelHandle);

		return {};
	}

	void NIDAQAnalogInData::ResetImpl(dispatch_tag<AnalogInData>)
	{
		ResetImpl(dispatch_tag<NIDAQAnalogInData>());
	}

	Util::TextValueListType<NIDAQAnalogInParams::TerminalConfigType> NIDAQAnalogInParams::TerminalConfigTypeStrList()
	{
		Util::TextValueListType<TerminalConfigType> List = {
			{ "Default terminal configuration chosen by NI DAQ at run time", TerminalConfigType::Default },
			{ "Referenced single-ended terminal configuration", TerminalConfigType::RSE },
			{ "Non-referenced single-ended terminal configuration", TerminalConfigType::NRSE },
			{ "Differential terminal configuration", TerminalConfigType::Diff },
			{ "Pseudodifferential terminal configuration", TerminalConfigType::PseudoDiff }
		};

		return List;
	}

	void NIDAQAnalogInParams::ConfigureParamsImpl(dispatch_tag<AnalogInParams>)
	{
		ConfigureParamsImpl(dispatch_tag<NIDAQAnalogInParams>());
	}

	NIDAQAnalogIn::NIDAQAnalogIn(const std::thread::id OwnerThreadID, DynExp::ParamsBasePtrType&& Params)
		: AnalogIn(OwnerThreadID, std::move(Params))
	{
	}

	StreamSizeParamsExtension::ValueType NIDAQAnalogIn::GetStreamSizeParams() const
	{
		auto InstrData = DynExp::dynamic_InstrumentData_cast<NIDAQAnalogIn>(GetInstrumentData());
		auto HardwareParams = dynamic_Params_cast<DynExpHardware::NIDAQHardwareAdapter>(InstrData->HardwareAdapter->GetParams());

		return HardwareParams->StreamSizeParams.Values();
	}

	NumericSampleStreamParamsExtension::ValueType NIDAQAnalogIn::GetNumericSampleStreamParams() const
	{
		auto InstrData = DynExp::dynamic_InstrumentData_cast<NIDAQAnalogIn>(GetInstrumentData());
		auto HardwareParams = dynamic_Params_cast<DynExpHardware::NIDAQHardwareAdapter>(InstrData->HardwareAdapter->GetParams());

		return HardwareParams->NumericSampleStreamParams.Values();
	}

	Util::OptionalBool NIDAQAnalogIn::HasFinished() const
	{
		auto InstrData = DynExp::dynamic_InstrumentData_cast<NIDAQAnalogIn>(GetInstrumentData());

		return InstrData->HardwareAdapter->HasFinishedTask(InstrData->ChannelHandle);
	}

	void NIDAQAnalogIn::OnErrorChild() const
	{
		auto InstrData = DynExp::dynamic_InstrumentData_cast<NIDAQAnalogIn>(GetInstrumentData());

		InstrData->HardwareAdapter->DeregisterChannel(InstrData->ChannelHandle);
	}

	void NIDAQAnalogIn::ResetImpl(dispatch_tag<AnalogIn>)
	{
		ResetImpl(dispatch_tag<NIDAQAnalogIn>());
	}
}