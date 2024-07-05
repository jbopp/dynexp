// This file is part of DynExp.

#include "stdafx.h"
#include "NIDAQAnalogOut.h"

namespace DynExpInstr
{
	void NIDAQAnalogOutTasks::InitTask::InitFuncImpl(dispatch_tag<AnalogOutTasks::InitTask>, DynExp::InstrumentInstance& Instance)
	{
		auto InstrParams = DynExp::dynamic_Params_cast<NIDAQAnalogOut>(Instance.ParamsGetter());
		auto InstrData = DynExp::dynamic_InstrumentData_cast<NIDAQAnalogOut>(Instance.InstrumentDataGetter());

		Instance.LockObject(InstrParams->HardwareAdapter, InstrData->HardwareAdapter);
		InstrData->ChannelHandle = InstrData->HardwareAdapter->InitializeAnalogOutChannel(InstrParams->ChannelName.Get(),
			NIDAQAnalogOut::HardwareMinValue(), NIDAQAnalogOut::HardwareMaxValue(), InstrParams->NIDAQOutputPortParams.UseOnlyOnBrdMem);

		InitFuncImpl(dispatch_tag<InitTask>(), Instance);
	}

	void NIDAQAnalogOutTasks::ExitTask::ExitFuncImpl(dispatch_tag<AnalogOutTasks::ExitTask>, DynExp::InstrumentInstance& Instance)
	{
		ExitFuncImpl(dispatch_tag<ExitTask>(), Instance);

		auto InstrData = DynExp::dynamic_InstrumentData_cast<NIDAQAnalogOut>(Instance.InstrumentDataGetter());

		InstrData->HardwareAdapter->DeregisterChannel(InstrData->ChannelHandle);
		Instance.UnlockObject(InstrData->HardwareAdapter);
	}

	void NIDAQAnalogOutTasks::UpdateTask::UpdateFuncImpl(dispatch_tag<AnalogOutTasks::UpdateTask>, DynExp::InstrumentInstance& Instance)
	{
		UpdateFuncImpl(dispatch_tag<UpdateTask>(), Instance);
	}

	DynExp::TaskResultType NIDAQAnalogOutTasks::WriteTask::RunChild(DynExp::InstrumentInstance& Instance)
	{
		auto InstrData = DynExp::dynamic_InstrumentData_cast<NIDAQAnalogOut>(Instance.InstrumentDataGetter());

		auto SampleStream = InstrData->GetCastSampleStream<NIDAQAnalogOutData::SampleStreamType>();
		InstrData->HardwareAdapter->WriteAnalogValues(InstrData->ChannelHandle, SampleStream->ReadSamples(SampleStream->GetStreamSizeRead()));

		return {};
	}

	DynExp::TaskResultType NIDAQAnalogOutTasks::ClearTask::RunChild(DynExp::InstrumentInstance& Instance)
	{
		auto InstrData = DynExp::dynamic_InstrumentData_cast<NIDAQAnalogOut>(Instance.InstrumentDataGetter());
		InstrData->HardwareAdapter->StopTask(InstrData->ChannelHandle);

		return {};
	}

	DynExp::TaskResultType NIDAQAnalogOutTasks::StartTask::RunChild(DynExp::InstrumentInstance& Instance)
	{
		auto InstrData = DynExp::dynamic_InstrumentData_cast<NIDAQAnalogOut>(Instance.InstrumentDataGetter());

		InstrData->HardwareAdapter->StartTask(InstrData->ChannelHandle);

		return {};
	}

	DynExp::TaskResultType NIDAQAnalogOutTasks::StopTask::RunChild(DynExp::InstrumentInstance& Instance)
	{
		auto InstrData = DynExp::dynamic_InstrumentData_cast<NIDAQAnalogOut>(Instance.InstrumentDataGetter());

		InstrData->HardwareAdapter->StopTask(InstrData->ChannelHandle);

		return {};
	}

	DynExp::TaskResultType NIDAQAnalogOutTasks::RestartTask::RunChild(DynExp::InstrumentInstance& Instance)
	{
		auto InstrData = DynExp::dynamic_InstrumentData_cast<NIDAQAnalogOut>(Instance.InstrumentDataGetter());

		InstrData->HardwareAdapter->RestartTask(InstrData->ChannelHandle);

		return {};
	}

	void NIDAQAnalogOutData::ResetImpl(dispatch_tag<AnalogOutData>)
	{
		ResetImpl(dispatch_tag<NIDAQAnalogOutData>());
	}

	void NIDAQAnalogOutParams::ConfigureParamsImpl(dispatch_tag<AnalogOutParams>)
	{
		ConfigureParamsImpl(dispatch_tag<NIDAQAnalogOutParams>());
	}

	NIDAQAnalogOut::NIDAQAnalogOut(const std::thread::id OwnerThreadID, DynExp::ParamsBasePtrType&& Params)
		: AnalogOut(OwnerThreadID, std::move(Params))
	{
	}

	StreamSizeParamsExtension::ValueType NIDAQAnalogOut::GetStreamSizeParams() const
	{
		auto InstrData = DynExp::dynamic_InstrumentData_cast<NIDAQAnalogOut>(GetInstrumentData());
		auto HardwareParams = dynamic_Params_cast<DynExpHardware::NIDAQHardwareAdapter>(InstrData->HardwareAdapter->GetParams());

		return HardwareParams->StreamSizeParams.Values();
	}

	NumericSampleStreamParamsExtension::ValueType NIDAQAnalogOut::GetNumericSampleStreamParams() const
	{
		auto InstrData = DynExp::dynamic_InstrumentData_cast<NIDAQAnalogOut>(GetInstrumentData());
		auto HardwareParams = dynamic_Params_cast<DynExpHardware::NIDAQHardwareAdapter>(InstrData->HardwareAdapter->GetParams());

		return HardwareParams->NumericSampleStreamParams.Values();
	}

	Util::OptionalBool NIDAQAnalogOut::HasFinished() const
	{
		auto InstrData = DynExp::dynamic_InstrumentData_cast<NIDAQAnalogOut>(GetInstrumentData());

		return InstrData->HardwareAdapter->HasFinishedTask(InstrData->ChannelHandle);
	}

	void NIDAQAnalogOut::SetDefault(DynExp::TaskBase::CallbackType CallbackFunc) const
	{
		bool IsMultisample{};

		{
			auto InstrData = DynExp::dynamic_InstrumentData_cast<NIDAQAnalogOut>(GetInstrumentData());
			IsMultisample = InstrData->HardwareAdapter->GetTask(InstrData->ChannelHandle)->IsMultisample();
		} // InstrData unlocked here.

		if (IsMultisample)
		{
			Stop();
			AnalogOut::SetDefault(CallbackFunc);
			Start();
		}
		else
			AnalogOut::SetDefault(CallbackFunc);
	}

	void NIDAQAnalogOut::OnErrorChild() const
	{
		auto InstrData = DynExp::dynamic_InstrumentData_cast<NIDAQAnalogOut>(GetInstrumentData());

		InstrData->HardwareAdapter->DeregisterChannel(InstrData->ChannelHandle);
	}

	void NIDAQAnalogOut::ResetImpl(dispatch_tag<AnalogOut>)
	{
		ResetImpl(dispatch_tag<NIDAQAnalogOut>());
	}
}