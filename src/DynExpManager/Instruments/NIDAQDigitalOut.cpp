// This file is part of DynExp.

#include "stdafx.h"
#include "NIDAQDigitalOut.h"

namespace DynExpInstr
{
	void NIDAQDigitalOutTasks::InitTask::InitFuncImpl(dispatch_tag<DigitalOutTasks::InitTask>, DynExp::InstrumentInstance& Instance)
	{
		auto InstrParams = DynExp::dynamic_Params_cast<NIDAQDigitalOut>(Instance.ParamsGetter());
		auto InstrData = DynExp::dynamic_InstrumentData_cast<NIDAQDigitalOut>(Instance.InstrumentDataGetter());

		Instance.LockObject(InstrParams->HardwareAdapter, InstrData->HardwareAdapter);
		InstrData->ChannelHandle = InstrData->HardwareAdapter->InitializeDigitalOutChannel(InstrParams->ChannelName.Get(),
			InstrParams->NIDAQOutputPortParams.UseOnlyOnBrdMem);

		InitFuncImpl(dispatch_tag<InitTask>(), Instance);
	}

	void NIDAQDigitalOutTasks::ExitTask::ExitFuncImpl(dispatch_tag<DigitalOutTasks::ExitTask>, DynExp::InstrumentInstance& Instance)
	{
		ExitFuncImpl(dispatch_tag<ExitTask>(), Instance);

		auto InstrData = DynExp::dynamic_InstrumentData_cast<NIDAQDigitalOut>(Instance.InstrumentDataGetter());

		InstrData->HardwareAdapter->DeregisterChannel(InstrData->ChannelHandle);
		Instance.UnlockObject(InstrData->HardwareAdapter);
	}

	void NIDAQDigitalOutTasks::UpdateTask::UpdateFuncImpl(dispatch_tag<DigitalOutTasks::UpdateTask>, DynExp::InstrumentInstance& Instance)
	{
		UpdateFuncImpl(dispatch_tag<UpdateTask>(), Instance);
	}

	DynExp::TaskResultType NIDAQDigitalOutTasks::WriteTask::RunChild(DynExp::InstrumentInstance& Instance)
	{
		auto InstrData = DynExp::dynamic_InstrumentData_cast<NIDAQDigitalOut>(Instance.InstrumentDataGetter());

		auto SampleStream = InstrData->GetCastSampleStream<NIDAQDigitalOutData::SampleStreamType>();
		InstrData->HardwareAdapter->WriteDigitalValues(InstrData->ChannelHandle, SampleStream->ReadSamples(SampleStream->GetStreamSizeRead()));

		return {};
	}

	DynExp::TaskResultType NIDAQDigitalOutTasks::ClearTask::RunChild(DynExp::InstrumentInstance& Instance)
	{
		auto InstrData = DynExp::dynamic_InstrumentData_cast<NIDAQDigitalOut>(Instance.InstrumentDataGetter());
		InstrData->HardwareAdapter->StopTask(InstrData->ChannelHandle);

		return {};
	}

	DynExp::TaskResultType NIDAQDigitalOutTasks::StartTask::RunChild(DynExp::InstrumentInstance& Instance)
	{
		auto InstrData = DynExp::dynamic_InstrumentData_cast<NIDAQDigitalOut>(Instance.InstrumentDataGetter());

		InstrData->HardwareAdapter->StartTask(InstrData->ChannelHandle);

		return {};
	}

	DynExp::TaskResultType NIDAQDigitalOutTasks::StopTask::RunChild(DynExp::InstrumentInstance& Instance)
	{
		auto InstrData = DynExp::dynamic_InstrumentData_cast<NIDAQDigitalOut>(Instance.InstrumentDataGetter());

		InstrData->HardwareAdapter->StopTask(InstrData->ChannelHandle);

		return {};
	}

	DynExp::TaskResultType NIDAQDigitalOutTasks::RestartTask::RunChild(DynExp::InstrumentInstance& Instance)
	{
		auto InstrData = DynExp::dynamic_InstrumentData_cast<NIDAQDigitalOut>(Instance.InstrumentDataGetter());

		InstrData->HardwareAdapter->RestartTask(InstrData->ChannelHandle);

		return {};
	}

	void NIDAQDigitalOutData::ResetImpl(dispatch_tag<DigitalOutData>)
	{
		ResetImpl(dispatch_tag<NIDAQDigitalOutData>());
	}

	void NIDAQDigitalOutParams::ConfigureParamsImpl(dispatch_tag<DigitalOutParams>)
	{
		ConfigureParamsImpl(dispatch_tag<NIDAQDigitalOutParams>());
	}

	NIDAQDigitalOut::NIDAQDigitalOut(const std::thread::id OwnerThreadID, DynExp::ParamsBasePtrType&& Params)
		: DigitalOut(OwnerThreadID, std::move(Params))
	{
	}

	StreamSizeParamsExtension::ValueType NIDAQDigitalOut::GetStreamSizeParams() const
	{
		auto InstrData = DynExp::dynamic_InstrumentData_cast<NIDAQDigitalOut>(GetInstrumentData());
		auto HardwareParams = dynamic_Params_cast<DynExpHardware::NIDAQHardwareAdapter>(InstrData->HardwareAdapter->GetParams());

		return HardwareParams->StreamSizeParams.Values();
	}

	NumericSampleStreamParamsExtension::ValueType NIDAQDigitalOut::GetNumericSampleStreamParams() const
	{
		auto InstrData = DynExp::dynamic_InstrumentData_cast<NIDAQDigitalOut>(GetInstrumentData());
		auto HardwareParams = dynamic_Params_cast<DynExpHardware::NIDAQHardwareAdapter>(InstrData->HardwareAdapter->GetParams());

		return HardwareParams->NumericSampleStreamParams.Values();
	}

	Util::OptionalBool NIDAQDigitalOut::HasFinished() const
	{
		auto InstrData = DynExp::dynamic_InstrumentData_cast<NIDAQDigitalOut>(GetInstrumentData());

		return InstrData->HardwareAdapter->HasFinishedTask(InstrData->ChannelHandle);
	}

	void NIDAQDigitalOut::SetDefault(DynExp::TaskBase::CallbackType CallbackFunc) const
	{
		bool IsMultisample{};

		{
			auto InstrData = DynExp::dynamic_InstrumentData_cast<NIDAQDigitalOut>(GetInstrumentData());
			IsMultisample = InstrData->HardwareAdapter->GetTask(InstrData->ChannelHandle)->IsMultisample();
		} // InstrData unlocked here.

		if (IsMultisample)
		{
			Stop();
			DigitalOut::SetDefault(CallbackFunc);
			Start();
		}
		else
			DigitalOut::SetDefault(CallbackFunc);
	}

	void NIDAQDigitalOut::OnErrorChild() const
	{
		auto InstrData = DynExp::dynamic_InstrumentData_cast<NIDAQDigitalOut>(GetInstrumentData());

		InstrData->HardwareAdapter->DeregisterChannel(InstrData->ChannelHandle);
	}

	void NIDAQDigitalOut::ResetImpl(dispatch_tag<DigitalOut>)
	{
		ResetImpl(dispatch_tag<NIDAQDigitalOut>());
	}
}