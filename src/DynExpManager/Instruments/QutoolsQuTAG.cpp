// This file is part of DynExp.

#include "stdafx.h"
#include "QutoolsQuTAG.h"

namespace DynExpInstr
{
	void QutoolsQuTAGTasks::UpdateStreamMode(Util::SynchronizedPointer<QutoolsQuTAGData>& InstrData)
	{
		// In case one is only interested in the counts per exposure time window, there is no need to transfer all
		// single count events to the computer. So, mute the channel in that case.
		if (InstrData->GetStreamMode() == TimeTaggerData::StreamModeType::Counts)
			InstrData->HardwareAdapter->ConfigureFilter(InstrData->GetChannel(), DynExpHardware::QutoolsTDCSyms::TDC_FilterType::FILTER_MUTE);
		else
			InstrData->HardwareAdapter->ConfigureFilter(InstrData->GetChannel());

		InstrData->ClearStreamModeChanged();
	}

	void QutoolsQuTAGTasks::InitTask::InitFuncImpl(dispatch_tag<TimeTaggerTasks::InitTask>, DynExp::InstrumentInstance& Instance)
	{
		auto InstrParams = DynExp::dynamic_Params_cast<QutoolsQuTAG>(Instance.ParamsGetter());
		auto InstrData = DynExp::dynamic_InstrumentData_cast<QutoolsQuTAG>(Instance.InstrumentDataGetter());

		InstrData->SetChannel(InstrParams->Channel);

		Instance.LockObject(InstrParams->HardwareAdapter, InstrData->HardwareAdapter);
		UpdateStreamMode(InstrData);
		InstrData->HardwareAdapter->EnableChannel(InstrData->GetChannel());

		InitFuncImpl(dispatch_tag<InitTask>(), Instance);
	}

	void QutoolsQuTAGTasks::ExitTask::ExitFuncImpl(dispatch_tag<TimeTaggerTasks::ExitTask>, DynExp::InstrumentInstance& Instance)
	{
		ExitFuncImpl(dispatch_tag<ExitTask>(), Instance);

		auto InstrData = DynExp::dynamic_InstrumentData_cast<QutoolsQuTAG>(Instance.InstrumentDataGetter());

		InstrData->HardwareAdapter->DisableChannel(InstrData->GetChannel());
		Instance.UnlockObject(InstrData->HardwareAdapter);
	}

	void QutoolsQuTAGTasks::UpdateTask::UpdateFuncImpl(dispatch_tag<TimeTaggerTasks::UpdateTask>, DynExp::InstrumentInstance& Instance)
	{
		{
			auto InstrData = DynExp::dynamic_InstrumentData_cast<QutoolsQuTAG>(Instance.InstrumentDataGetter());

			if (InstrData->GetStreamModeChanged())
				UpdateStreamMode(InstrData);
		} // InstrData unlocked here.

		UpdateFuncImpl(dispatch_tag<UpdateTask>(), Instance);
	}

	DynExp::TaskResultType QutoolsQuTAGTasks::ReadDataTask::RunChild(DynExp::InstrumentInstance& Instance)
	{
		auto InstrData = DynExp::dynamic_InstrumentData_cast<QutoolsQuTAG>(Instance.InstrumentDataGetter());
		auto SampleStream = InstrData->GetCastSampleStream<TimeTaggerData::SampleStreamType>();

		if (InstrData->GetStreamMode() == TimeTaggerData::StreamModeType::Counts)
		{
			auto Counts = InstrData->HardwareAdapter->GetCoincidenceCounts(InstrData->GetChannel() + 1);
			if (Counts.second)
				SampleStream->WriteSample(Counts.first);
		}
		else
			SampleStream->WriteSamples(InstrData->HardwareAdapter->GetTimestamps(InstrData->GetChannel()));

		if (InstrData->GetHBTResults().Enabled)
		{
			InstrData->GetHBTResults().EventCounts = InstrData->HardwareAdapter->GetHBTEventCounts();
			InstrData->GetHBTResults().IntegrationTime = InstrData->HardwareAdapter->GetHBTIntegrationTime();
			InstrData->GetHBTResults().ResultVector = InstrData->HardwareAdapter->GetHBTResult();
		}

		return {};
	}

	DynExp::TaskResultType QutoolsQuTAGTasks::SetStreamSizeTask::RunChild(DynExp::InstrumentInstance& Instance)
	{
		auto InstrData = DynExp::dynamic_InstrumentData_cast<QutoolsQuTAG>(Instance.InstrumentDataGetter());

		InstrData->HardwareAdapter->SetTimestampBufferSize(Util::NumToT<DynExpHardware::QutoolsTDCSyms::Int32>(BufferSizeInSamples));
		InstrData->GetSampleStream()->SetStreamSize(BufferSizeInSamples);

		return {};
	}

	DynExp::TaskResultType QutoolsQuTAGTasks::ClearTask::RunChild(DynExp::InstrumentInstance& Instance)
	{
		auto InstrData = DynExp::dynamic_InstrumentData_cast<QutoolsQuTAG>(Instance.InstrumentDataGetter());

		InstrData->HardwareAdapter->ClearTimestamps(InstrData->GetChannel());

		return {};
	}

	DynExp::TaskResultType QutoolsQuTAGTasks::ConfigureInputTask::RunChild(DynExp::InstrumentInstance& Instance)
	{
		auto InstrData = DynExp::dynamic_InstrumentData_cast<QutoolsQuTAG>(Instance.InstrumentDataGetter());

		InstrData->HardwareAdapter->ConfigureSignalConditioning(InstrData->GetChannel(), DynExpHardware::QutoolsTDCSyms::SCOND_MISC, UseRisingEdge, ThresholdInVolts);

		return {};
	}

	DynExp::TaskResultType QutoolsQuTAGTasks::SetExposureTimeTask::RunChild(DynExp::InstrumentInstance& Instance)
	{
		auto InstrData = DynExp::dynamic_InstrumentData_cast<QutoolsQuTAG>(Instance.InstrumentDataGetter());

		InstrData->HardwareAdapter->SetExposureTime(std::chrono::duration_cast<std::chrono::milliseconds>(ExposureTime));

		return {};
	}

	DynExp::TaskResultType QutoolsQuTAGTasks::SetCoincidenceWindowTask::RunChild(DynExp::InstrumentInstance& Instance)
	{
		auto InstrData = DynExp::dynamic_InstrumentData_cast<QutoolsQuTAG>(Instance.InstrumentDataGetter());

		InstrData->HardwareAdapter->SetCoincidenceWindow(CoincidenceWindow);

		return {};
	}

	DynExp::TaskResultType QutoolsQuTAGTasks::SetDelayTask::RunChild(DynExp::InstrumentInstance& Instance)
	{
		auto InstrData = DynExp::dynamic_InstrumentData_cast<QutoolsQuTAG>(Instance.InstrumentDataGetter());

		InstrData->HardwareAdapter->SetChannelDelay(InstrData->GetChannel(), Delay);

		return {};
	}

	DynExp::TaskResultType QutoolsQuTAGTasks::SetHBTActiveTask::RunChild(DynExp::InstrumentInstance& Instance)
	{
		auto InstrParams = DynExp::dynamic_Params_cast<QutoolsQuTAG>(Instance.ParamsGetter());
		auto InstrData = DynExp::dynamic_InstrumentData_cast<QutoolsQuTAG>(Instance.InstrumentDataGetter());

		InstrData->HardwareAdapter->EnableHBT(Enable);
		InstrData->GetHBTResults().Enabled = Enable;

		if (Enable)
		{
			InstrData->HardwareAdapter->ConfigureHBTChannels(InstrData->GetChannel(), InstrParams->CrossCorrChannel);

			// Disable filters for HBT.
			InstrData->HardwareAdapter->ConfigureFilter(InstrData->GetChannel());
			InstrData->HardwareAdapter->ConfigureFilter(InstrParams->CrossCorrChannel);
			
			// Avoid going back to pre-defined stream mode if the flag was set and not cleared before.
			InstrData->ClearStreamModeChanged();
		}

		return {};
	}

	DynExp::TaskResultType QutoolsQuTAGTasks::ConfigureHBTTask::RunChild(DynExp::InstrumentInstance& Instance)
	{
		auto InstrData = DynExp::dynamic_InstrumentData_cast<QutoolsQuTAG>(Instance.InstrumentDataGetter());

		if (BinWidth / InstrData->HardwareAdapter->GetTimebase() < 1 || BinWidth / InstrData->HardwareAdapter->GetTimebase() > 1e6)
			throw Util::OutOfRangeException("The bin width exceeds the allowed range (see tdchbt.h's reference).");
		if (BinCount < 16 || BinCount > 64000)
			throw Util::OutOfRangeException("The number of bins exceeds the allowed range (see tdchbt.h's reference).");

		InstrData->HardwareAdapter->ConfigureHBTParams(BinWidth, Util::NumToT<DynExpHardware::QutoolsTDCSyms::Int32>(BinCount));

		return {};
	}

	DynExp::TaskResultType QutoolsQuTAGTasks::ResetHBTTask::RunChild(DynExp::InstrumentInstance& Instance)
	{
		auto InstrData = DynExp::dynamic_InstrumentData_cast<QutoolsQuTAG>(Instance.InstrumentDataGetter());

		if (InstrData->GetHBTResults().Enabled)
			InstrData->HardwareAdapter->ResetHBT();

		return {};
	}

	void QutoolsQuTAGData::ResetImpl(dispatch_tag<TimeTaggerData>)
	{
		Channel = 0;

		ResetImpl(dispatch_tag<QutoolsQuTAGData>());
	}

	QutoolsQuTAG::QutoolsQuTAG(const std::thread::id OwnerThreadID, DynExp::ParamsBasePtrType&& Params)
		: TimeTagger(OwnerThreadID, std::move(Params))
	{
	}

	Util::picoseconds QutoolsQuTAG::GetResolution() const
	{
		auto InstrData = DynExp::dynamic_InstrumentData_cast<QutoolsQuTAG>(GetInstrumentData());

		return InstrData->HardwareAdapter->GetTimebase();
	}

	size_t QutoolsQuTAG::GetBufferSize() const
	{
		auto InstrData = DynExp::dynamic_InstrumentData_cast<QutoolsQuTAG>(GetInstrumentData());

		return InstrData->HardwareAdapter->GetBufferSize();
	}

	void QutoolsQuTAG::ResetImpl(dispatch_tag<TimeTagger>)
	{
		ResetImpl(dispatch_tag<QutoolsQuTAG>());
	}
}