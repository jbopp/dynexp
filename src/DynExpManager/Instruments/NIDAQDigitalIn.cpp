// This file is part of DynExp.

#include "stdafx.h"
#include "NIDAQDigitalIn.h"

namespace DynExpInstr
{
	void NIDAQDigitalInTasks::InitTask::InitFuncImpl(dispatch_tag<DigitalInTasks::InitTask>, DynExp::InstrumentInstance& Instance)
	{
		auto InstrParams = DynExp::dynamic_Params_cast<NIDAQDigitalIn>(Instance.ParamsGetter());
		auto InstrData = DynExp::dynamic_InstrumentData_cast<NIDAQDigitalIn>(Instance.InstrumentDataGetter());

		Instance.LockObject(InstrParams->HardwareAdapter, InstrData->HardwareAdapter);
		InstrData->ChannelHandle = InstrData->HardwareAdapter->InitializeDigitalInChannel(InstrParams->ChannelName.Get(), 0);

		InitFuncImpl(dispatch_tag<InitTask>(), Instance);
	}

	void NIDAQDigitalInTasks::ExitTask::ExitFuncImpl(dispatch_tag<DigitalInTasks::ExitTask>, DynExp::InstrumentInstance& Instance)
	{
		ExitFuncImpl(dispatch_tag<ExitTask>(), Instance);

		auto InstrData = DynExp::dynamic_InstrumentData_cast<NIDAQDigitalIn>(Instance.InstrumentDataGetter());

		InstrData->HardwareAdapter->DeregisterChannel(InstrData->ChannelHandle);
		Instance.UnlockObject(InstrData->HardwareAdapter);
	}

	void NIDAQDigitalInTasks::UpdateTask::UpdateFuncImpl(dispatch_tag<DigitalInTasks::UpdateTask>, DynExp::InstrumentInstance& Instance)
	{
		UpdateFuncImpl(dispatch_tag<UpdateTask>(), Instance);
	}

	DynExp::TaskResultType NIDAQDigitalInTasks::ReadTask::RunChild(DynExp::InstrumentInstance& Instance)
	{
		auto InstrData = DynExp::dynamic_InstrumentData_cast<NIDAQDigitalIn>(Instance.InstrumentDataGetter());

		auto SampleStream = InstrData->GetCastSampleStream<NIDAQDigitalInData::SampleStreamType>();
		SampleStream->WriteSamples(InstrData->HardwareAdapter->ReadDigitalValues(InstrData->ChannelHandle));

		return {};
	}

	DynExp::TaskResultType NIDAQDigitalInTasks::StartTask::RunChild(DynExp::InstrumentInstance& Instance)
	{
		auto InstrData = DynExp::dynamic_InstrumentData_cast<NIDAQDigitalIn>(Instance.InstrumentDataGetter());

		InstrData->HardwareAdapter->StartTask(InstrData->ChannelHandle);

		return {};
	}

	DynExp::TaskResultType NIDAQDigitalInTasks::StopTask::RunChild(DynExp::InstrumentInstance& Instance)
	{
		auto InstrData = DynExp::dynamic_InstrumentData_cast<NIDAQDigitalIn>(Instance.InstrumentDataGetter());

		InstrData->HardwareAdapter->StopTask(InstrData->ChannelHandle);

		return {};
	}

	DynExp::TaskResultType NIDAQDigitalInTasks::RestartTask::RunChild(DynExp::InstrumentInstance& Instance)
	{
		auto InstrData = DynExp::dynamic_InstrumentData_cast<NIDAQDigitalIn>(Instance.InstrumentDataGetter());

		InstrData->HardwareAdapter->RestartTask(InstrData->ChannelHandle);

		return {};
	}

	void NIDAQDigitalInData::ResetImpl(dispatch_tag<DigitalInData>)
	{
		ResetImpl(dispatch_tag<NIDAQDigitalInData>());
	}

	void NIDAQDigitalInParams::ConfigureParamsImpl(dispatch_tag<DigitalInParams>)
	{
		ConfigureParamsImpl(dispatch_tag<NIDAQDigitalInParams>());
	}

	NIDAQDigitalIn::NIDAQDigitalIn(const std::thread::id OwnerThreadID, DynExp::ParamsBasePtrType&& Params)
		: DigitalIn(OwnerThreadID, std::move(Params))
	{
	}

	StreamSizeParamsExtension::ValueType NIDAQDigitalIn::GetStreamSizeParams() const
	{
		auto InstrData = DynExp::dynamic_InstrumentData_cast<NIDAQDigitalIn>(GetInstrumentData());
		auto HardwareParams = dynamic_Params_cast<DynExpHardware::NIDAQHardwareAdapter>(InstrData->HardwareAdapter->GetParams());

		return HardwareParams->StreamSizeParams.Values();
	}

	NumericSampleStreamParamsExtension::ValueType NIDAQDigitalIn::GetNumericSampleStreamParams() const
	{
		auto InstrData = DynExp::dynamic_InstrumentData_cast<NIDAQDigitalIn>(GetInstrumentData());
		auto HardwareParams = dynamic_Params_cast<DynExpHardware::NIDAQHardwareAdapter>(InstrData->HardwareAdapter->GetParams());

		return HardwareParams->NumericSampleStreamParams.Values();
	}

	Util::OptionalBool NIDAQDigitalIn::HasFinished() const
	{
		auto InstrData = DynExp::dynamic_InstrumentData_cast<NIDAQDigitalIn>(GetInstrumentData());

		return InstrData->HardwareAdapter->HasFinishedTask(InstrData->ChannelHandle);
	}

	void NIDAQDigitalIn::OnErrorChild() const
	{
		auto InstrData = DynExp::dynamic_InstrumentData_cast<NIDAQDigitalIn>(GetInstrumentData());

		InstrData->HardwareAdapter->DeregisterChannel(InstrData->ChannelHandle);
	}

	void NIDAQDigitalIn::ResetImpl(dispatch_tag<DigitalIn>)
	{
		ResetImpl(dispatch_tag<NIDAQDigitalIn>());
	}
}