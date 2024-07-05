// This file is part of DynExp.

#include "stdafx.h"
#include "SwabianInstrumentsPulseStreamer.h"

namespace DynExpInstr
{
	void SwabianInstrumentsPulseStreamerTasks::InitTask::InitFuncImpl(dispatch_tag<FunctionGeneratorTasks::InitTask>, DynExp::InstrumentInstance& Instance)
	{
		auto InstrParams = DynExp::dynamic_Params_cast<SwabianInstrumentsPulseStreamer>(Instance.ParamsGetter());
		auto InstrData = DynExp::dynamic_InstrumentData_cast<SwabianInstrumentsPulseStreamer>(Instance.InstrumentDataGetter());

		InstrData->Channel = InstrParams->Channel;
		Instance.LockObject(InstrParams->HardwareAdapter, InstrData->HardwareAdapter);
		InstrData->GetSampleStream()->SetStreamSize(InstrParams->StreamSizeParams.StreamSize);

		InitFuncImpl(dispatch_tag<InitTask>(), Instance);
	}

	void SwabianInstrumentsPulseStreamerTasks::ExitTask::ExitFuncImpl(dispatch_tag<FunctionGeneratorTasks::ExitTask>, DynExp::InstrumentInstance& Instance)
	{
		ExitFuncImpl(dispatch_tag<ExitTask>(), Instance);

		auto InstrData = DynExp::dynamic_InstrumentData_cast<SwabianInstrumentsPulseStreamer>(Instance.InstrumentDataGetter());

		Instance.UnlockObject(InstrData->HardwareAdapter);
	}

	void SwabianInstrumentsPulseStreamerTasks::UpdateTask::UpdateFuncImpl(dispatch_tag<FunctionGeneratorTasks::UpdateTask>, DynExp::InstrumentInstance& Instance)
	{
		try
		{
			auto InstrData = DynExp::dynamic_InstrumentData_cast<SwabianInstrumentsPulseStreamer>(Instance.InstrumentDataGetter());
			bool UpdateError = false;

			try
			{
				auto InstrParams = DynExp::dynamic_Params_cast<SwabianInstrumentsPulseStreamer>(Instance.ParamsGetter());
				auto InstrData = DynExp::dynamic_InstrumentData_cast<SwabianInstrumentsPulseStreamer>(Instance.InstrumentDataGetter());

				InstrData->Streaming = InstrData->HardwareAdapter->IsStreaming();
				InstrData->Finished = InstrData->HardwareAdapter->HasFinished();
			}
			catch ([[maybe_unused]] const DynExpHardware::gRPCException& e)
			{
				UpdateError = true;

				// Swallow if just one or two subsequent updates failed.
				if (InstrData->NumFailedStatusUpdateAttempts++ >= 3)
					throw;
			}

			if (!UpdateError)
				InstrData->NumFailedStatusUpdateAttempts = 0;
		}
		// Issued if a mutex is blocked by another operation.
		catch (const Util::TimeoutException& e)
		{
			Instance.GetOwner().SetWarning(e);

			return;
		}

		UpdateFuncImpl(dispatch_tag<UpdateTask>(), Instance);
	}

	DynExp::TaskResultType SwabianInstrumentsPulseStreamerTasks::WriteTask::RunChild(DynExp::InstrumentInstance& Instance)
	{
		auto InstrData = DynExp::dynamic_InstrumentData_cast<SwabianInstrumentsPulseStreamer>(Instance.InstrumentDataGetter());

		const auto SampleStream = InstrData->GetCastSampleStream<SwabianInstrumentsPulseStreamerData::SampleStreamType>();
		const auto Samples = SampleStream->ReadSamples(SampleStream->GetStreamSizeRead());
		if (Samples.empty())
			return {};

		std::vector<DynExpHardware::SIPulseStreamerHardwareAdapter::SampleType> TransformedSamples;
		for (const auto& Sample : Samples)
		{
			const auto Timestamp = std::chrono::nanoseconds(Util::NumToT<std::chrono::nanoseconds::rep>(Sample.Time * std::nano::den));

			if (InstrData->IsDigitalChannel())
				TransformedSamples.emplace_back(InstrData->GetChannel(), Timestamp, Sample.Value != 0);
			else
				TransformedSamples.emplace_back(InstrData->GetChannel(), Timestamp,
					DynExpHardware::SIPulseStreamerHardwareAdapter::SampleType::MakeValueFromVoltage(Sample.Value));
		}

		InstrData->HardwareAdapter->SetSamples(InstrData->GetChannel(), TransformedSamples);

		return {};
	}

	DynExp::TaskResultType SwabianInstrumentsPulseStreamerTasks::ClearTask::RunChild(DynExp::InstrumentInstance& Instance)
	{
		auto InstrData = DynExp::dynamic_InstrumentData_cast<SwabianInstrumentsPulseStreamer>(Instance.InstrumentDataGetter());

		InstrData->HardwareAdapter->SetSamples(InstrData->GetChannel(), { { InstrData->GetChannel(), std::chrono::nanoseconds(0), 0 } });

		return {};
	}

	DynExp::TaskResultType SwabianInstrumentsPulseStreamerTasks::StartTask::RunChild(DynExp::InstrumentInstance& Instance)
	{
		auto InstrData = DynExp::dynamic_InstrumentData_cast<SwabianInstrumentsPulseStreamer>(Instance.InstrumentDataGetter());

		InstrData->HardwareAdapter->RearmTrigger();

		return {};
	}

	DynExp::TaskResultType SwabianInstrumentsPulseStreamerTasks::StopTask::RunChild(DynExp::InstrumentInstance& Instance)
	{
		auto InstrData = DynExp::dynamic_InstrumentData_cast<SwabianInstrumentsPulseStreamer>(Instance.InstrumentDataGetter());

		InstrData->HardwareAdapter->SetConstantOutput();

		return {};
	}

	DynExp::TaskResultType SwabianInstrumentsPulseStreamerTasks::RestartTask::RunChild(DynExp::InstrumentInstance& Instance)
	{
		auto InstrData = DynExp::dynamic_InstrumentData_cast<SwabianInstrumentsPulseStreamer>(Instance.InstrumentDataGetter());

		InstrData->HardwareAdapter->RearmTrigger();

		return {};
	}

	DynExp::TaskResultType SwabianInstrumentsPulseStreamerTasks::ResetBufferSizeTask::RunChild(DynExp::InstrumentInstance& Instance)
	{
		auto InstrParams = DynExp::dynamic_Params_cast<SwabianInstrumentsPulseStreamer>(Instance.ParamsGetter());
		auto InstrData = DynExp::dynamic_InstrumentData_cast<SwabianInstrumentsPulseStreamer>(Instance.InstrumentDataGetter());

		InstrData->GetSampleStream()->SetStreamSize(InstrParams->StreamSizeParams.StreamSize);

		return {};
	}

	DynExp::TaskResultType SwabianInstrumentsPulseStreamerTasks::ForceTriggerTask::RunChild(DynExp::InstrumentInstance& Instance)
	{
		auto InstrData = DynExp::dynamic_InstrumentData_cast<SwabianInstrumentsPulseStreamer>(Instance.InstrumentDataGetter());

		InstrData->HardwareAdapter->ForceTrigger();

		return {};
	}

	DynExp::TaskResultType SwabianInstrumentsPulseStreamerTasks::SetTriggerTask::RunChild(DynExp::InstrumentInstance& Instance)
	{
		auto InstrData = DynExp::dynamic_InstrumentData_cast<SwabianInstrumentsPulseStreamer>(Instance.InstrumentDataGetter());

		auto TriggerMode = TriggerDesc.TriggerMode == FunctionGeneratorDefs::TriggerDescType::TriggerModeType::Continuous ?
			DynExpHardware::SIPulseStreamerHardwareAdapterParams::TriggerModeType::Normal : DynExpHardware::SIPulseStreamerHardwareAdapterParams::TriggerModeType::Single;
		auto TriggerEdge = DynExpHardware::SIPulseStreamerHardwareAdapterParams::TriggerEdgeType::Software;
		switch (TriggerDesc.TriggerMode)
		{
		case FunctionGeneratorDefs::TriggerDescType::TriggerModeType::Continuous:
			TriggerEdge = DynExpHardware::SIPulseStreamerHardwareAdapterParams::TriggerEdgeType::Immediate;
			break;
		case FunctionGeneratorDefs::TriggerDescType::TriggerModeType::ExternSingle:
		case FunctionGeneratorDefs::TriggerDescType::TriggerModeType::ExternStep:
			TriggerEdge = TriggerDesc.TriggerEdge == FunctionGeneratorDefs::TriggerDescType::TriggerEdgeType::Fall ?
				DynExpHardware::SIPulseStreamerHardwareAdapterParams::TriggerEdgeType::FallingEdge : DynExpHardware::SIPulseStreamerHardwareAdapterParams::TriggerEdgeType::RisingEdge;
		}

		InstrData->HardwareAdapter->SetTrigger(TriggerEdge, TriggerMode);

		return {};
	}

	DynExp::TaskResultType SwabianInstrumentsPulseStreamerTasks::SetConstantOutputTask::RunChild(DynExp::InstrumentInstance& Instance)
	{
		auto InstrData = DynExp::dynamic_InstrumentData_cast<SwabianInstrumentsPulseStreamer>(Instance.InstrumentDataGetter());

		InstrData->HardwareAdapter->SetConstantOutput(Pulse);

		return {};
	}

	DynExp::TaskResultType SwabianInstrumentsPulseStreamerTasks::ForceFinalSampleTask::RunChild(DynExp::InstrumentInstance& Instance)
	{
		auto InstrData = DynExp::dynamic_InstrumentData_cast<SwabianInstrumentsPulseStreamer>(Instance.InstrumentDataGetter());

		InstrData->HardwareAdapter->ForceFinalSample();

		return {};
	}

	DynExp::TaskResultType SwabianInstrumentsPulseStreamerTasks::SetNumRunsTask::RunChild(DynExp::InstrumentInstance& Instance)
	{
		auto InstrData = DynExp::dynamic_InstrumentData_cast<SwabianInstrumentsPulseStreamer>(Instance.InstrumentDataGetter());

		InstrData->HardwareAdapter->SetNumRuns(NumRuns);

		return {};
	}

	bool SwabianInstrumentsPulseStreamerData::IsDigitalChannel() const noexcept
	{
		return GetChannel() != DynExpHardware::SIPulseStreamerHardwareAdapterParams::OutputChannelType::AO0 &&
			GetChannel() != DynExpHardware::SIPulseStreamerHardwareAdapterParams::OutputChannelType::AO1;
	}

	void SwabianInstrumentsPulseStreamerData::ResetImpl(dispatch_tag<FunctionGeneratorData>)
	{
		Channel = {};
		Streaming = false;
		Finished = false;
		NumFailedStatusUpdateAttempts = 0;

		ResetImpl(dispatch_tag<SwabianInstrumentsPulseStreamerData>());
	}

	SwabianInstrumentsPulseStreamer::SwabianInstrumentsPulseStreamer(const std::thread::id OwnerThreadID, DynExp::ParamsBasePtrType&& Params)
		: FunctionGenerator(OwnerThreadID, std::move(Params))
	{
	}

	DataStreamInstrumentData::UnitType SwabianInstrumentsPulseStreamer::GetValueUnit() const noexcept
	{
		auto InstrData = DynExp::dynamic_InstrumentData_cast<SwabianInstrumentsPulseStreamer>(GetInstrumentData());

		return InstrData->IsDigitalChannel() ? DataStreamInstrumentData::UnitType::LogicLevel : DataStreamInstrumentData::UnitType::Volt;
	}

	void SwabianInstrumentsPulseStreamer::SetConstantOutput(const DynExpHardware::SIPulseStreamerHardwareAdapter::PulseType& Pulse, DynExp::TaskBase::CallbackType CallbackFunc) const
	{
		return MakeAndEnqueueTask<SwabianInstrumentsPulseStreamerTasks::SetConstantOutputTask>(Pulse, CallbackFunc);
	}

	void SwabianInstrumentsPulseStreamer::SetNumRuns(int64_t NumRuns, DynExp::TaskBase::CallbackType CallbackFunc) const
	{
		return MakeAndEnqueueTask<SwabianInstrumentsPulseStreamerTasks::SetNumRunsTask>(NumRuns, CallbackFunc);
	}

	Util::OptionalBool SwabianInstrumentsPulseStreamer::HasFinished() const
	{
		auto InstrData = DynExp::dynamic_InstrumentData_cast<SwabianInstrumentsPulseStreamer>(GetInstrumentData());

		return InstrData->HardwareAdapter->HasFinished();
	}

	Util::OptionalBool SwabianInstrumentsPulseStreamer::IsRunning() const
	{
		auto InstrData = DynExp::dynamic_InstrumentData_cast<SwabianInstrumentsPulseStreamer>(GetInstrumentData());

		return InstrData->HardwareAdapter->IsStreaming();
	}

	void SwabianInstrumentsPulseStreamer::ResetImpl(dispatch_tag<FunctionGenerator>)
	{
		ResetImpl(dispatch_tag<SwabianInstrumentsPulseStreamer>());
	}

	FunctionGeneratorDefs::FunctionDescType SwabianInstrumentsPulseStreamer::GetMinCapsChild() const
	{
		auto InstrData = DynExp::dynamic_InstrumentData_cast<SwabianInstrumentsPulseStreamer>(GetInstrumentData());

		return InstrData->IsDigitalChannel() ?
			FunctionGeneratorDefs::FunctionDescType{ 1e-3, 0, 0 } : FunctionGeneratorDefs::FunctionDescType{ 1e-3, -1, -1 };
	}

	FunctionGeneratorDefs::FunctionDescType SwabianInstrumentsPulseStreamer::GetMaxCapsChild() const
	{
		auto InstrData = DynExp::dynamic_InstrumentData_cast<SwabianInstrumentsPulseStreamer>(GetInstrumentData());

		return InstrData->IsDigitalChannel() ?
			FunctionGeneratorDefs::FunctionDescType{ 300e6, 3, 0 } : FunctionGeneratorDefs::FunctionDescType{ 35e6, 1, 1 };
	}

	FunctionGeneratorDefs::FunctionDescType SwabianInstrumentsPulseStreamer::GetParamDefaultsChild() const
	{
		auto InstrData = DynExp::dynamic_InstrumentData_cast<SwabianInstrumentsPulseStreamer>(GetInstrumentData());

		return InstrData->IsDigitalChannel() ?
			FunctionGeneratorDefs::FunctionDescType{ 1e6, 3, 0 } : FunctionGeneratorDefs::FunctionDescType{ 1e6, 1, 0 };
	}

	void SwabianInstrumentsPulseStreamer::SetTriggerChild(const FunctionGeneratorDefs::TriggerDescType& TriggerDesc,
		bool PersistParams, DynExp::TaskBase::CallbackType CallbackFunc) const
	{
		MakeAndEnqueueTask<SwabianInstrumentsPulseStreamerTasks::SetTriggerTask>(TriggerDesc, CallbackFunc);
	}
}