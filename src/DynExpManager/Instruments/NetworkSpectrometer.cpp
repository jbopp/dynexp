// This file is part of DynExp.

#include "stdafx.h"
#include "NetworkSpectrometer.h"

namespace DynExpInstr
{
	void NetworkSpectrometerTasks::InitTask::InitFuncImpl(dispatch_tag<gRPCInstrumentTasks::InitTask<Spectrometer, 0, DynExpProto::NetworkSpectrometer::NetworkSpectrometer>>, DynExp::InstrumentInstance& Instance)
	{
		StubPtrType<DynExpProto::NetworkSpectrometer::NetworkSpectrometer> StubPtr;
		{
			auto InstrData = DynExp::dynamic_InstrumentData_cast<NetworkSpectrometer>(Instance.InstrumentDataGetter());
			StubPtr = InstrData->template GetStub<DynExpProto::NetworkSpectrometer::NetworkSpectrometer>();
		} // InstrData unlocked here.

		auto Response = InvokeStubFunc(StubPtr, &DynExpProto::NetworkSpectrometer::NetworkSpectrometer::Stub::GetDeviceInfo, {});

		{
			auto InstrData = DynExp::dynamic_InstrumentData_cast<NetworkSpectrometer>(Instance.InstrumentDataGetter());

			InstrData->FrequencyUnit = ToSpectrometerUnitType(Response.frequencyunit());
			InstrData->IntensityUnit = ToSpectrometerUnitType(Response.intensityunit());
			InstrData->MinFrequency = Response.hardwareminfrequency();
			InstrData->MaxFrequency = Response.hardwaremaxfrequency();

			InstrData->SetMinExposureTime(Response.hardwareminexposuretime_ms());
			InstrData->SetMaxExposureTime(Response.hardwaremaxexposuretime_ms());
		} // InstrData unlocked here.

		// Initialize derived instrument last.
		InitFuncImpl(dispatch_tag<InitTask>(), Instance);
	}

	void NetworkSpectrometerTasks::ExitTask::ExitFuncImpl(dispatch_tag<gRPCInstrumentTasks::ExitTask<Spectrometer, 0, DynExpProto::NetworkSpectrometer::NetworkSpectrometer>>, DynExp::InstrumentInstance& Instance)
	{
		// Shut down derived instrument first.
		ExitFuncImpl(dispatch_tag<ExitTask>(), Instance);

		try
		{
			// Stop spectrum acquisition.
			StubPtrType<DynExpProto::NetworkSpectrometer::NetworkSpectrometer> StubPtr;
			{
				auto InstrData = DynExp::dynamic_InstrumentData_cast<NetworkSpectrometer>(Instance.InstrumentDataGetter());
				StubPtr = InstrData->template GetStub<DynExpProto::NetworkSpectrometer::NetworkSpectrometer>();
			} // InstrData unlocked here.

			InvokeStubFunc(StubPtr, &DynExpProto::NetworkSpectrometer::NetworkSpectrometer::Stub::Abort, {});
		}
		catch (...)
		{
			// Swallow any exception which might arise from instrument shutdown since a failure
			// of this function is not considered a severe error.
		}
	}

	void NetworkSpectrometerTasks::UpdateTask::UpdateFuncImpl(dispatch_tag<gRPCInstrumentTasks::UpdateTask<Spectrometer, 0, DynExpProto::NetworkSpectrometer::NetworkSpectrometer>>, DynExp::InstrumentInstance& Instance)
	{
		StubPtrType<DynExpProto::NetworkSpectrometer::NetworkSpectrometer> StubPtr;
		{
			auto InstrData = DynExp::dynamic_InstrumentData_cast<NetworkSpectrometer>(Instance.InstrumentDataGetter());
			StubPtr = InstrData->template GetStub<DynExpProto::NetworkSpectrometer::NetworkSpectrometer>();
		} // InstrData unlocked here.

		auto StateResponse = InvokeStubFunc(StubPtr, &DynExpProto::NetworkSpectrometer::NetworkSpectrometer::Stub::GetState, {});
		auto ExposureTimeResponse = InvokeStubFunc(StubPtr, &DynExpProto::NetworkSpectrometer::NetworkSpectrometer::Stub::GetExposureTime, {});
		auto FrequencyRangeResponse = InvokeStubFunc(StubPtr, &DynExpProto::NetworkSpectrometer::NetworkSpectrometer::Stub::GetFrequencyRange, {});
		auto SilentModeResponse = InvokeStubFunc(StubPtr, &DynExpProto::NetworkSpectrometer::NetworkSpectrometer::Stub::GetSilentMode, {});

		DynExpProto::NetworkSpectrometer::SpectrumMessage SpectrumResponse;
		if (StateResponse.spectrumavailable())
			SpectrumResponse = InvokeStubFunc(StubPtr, &DynExpProto::NetworkSpectrometer::NetworkSpectrometer::Stub::GetSpectrum, {});
		
		{
			auto InstrData = DynExp::dynamic_InstrumentData_cast<NetworkSpectrometer>(Instance.InstrumentDataGetter());

			InstrData->CapturingState = ToSpectrometerStateType(StateResponse.state());
			InstrData->CapturingProgress = StateResponse.has_progress() ? StateResponse.progress() : 0;
			InstrData->SetCurrentExposureTime(ExposureTimeResponse.time_ms());
			InstrData->SetCurrentLowerFrequency(FrequencyRangeResponse.lowerfrequency());
			InstrData->SetCurrentUpperFrequency(FrequencyRangeResponse.upperfrequency());
			InstrData->SetSilentModeEnabled(SilentModeResponse.enable());

			if (StateResponse.spectrumavailable() && SpectrumResponse.spectrumavailable())
			{
				if (SpectrumResponse.resultmsg().result() != DynExpProto::NetworkSpectrometer::ResultType::OK)
					Instance.GetOwner().SetWarning("Spectrum acquisition failed.", Util::DynExpErrorCodes::ServiceFailed);
				else
				{
					SpectrometerData::SpectrumType Spectrum(InstrData->GetFrequencyUnit(), InstrData->GetIntensityUnit());
					for (decltype(SpectrumResponse.samples_size()) i = 0; i < SpectrumResponse.samples_size(); ++i)
						Spectrum.GetSpectrum().insert({ SpectrumResponse.samples(i).frequency(), SpectrumResponse.samples(i).value() });

					InstrData->SetSpectrum(std::move(Spectrum));
				}
			}
		} // InstrData unlocked here.

		// Update derived instrument.
		UpdateFuncImpl(dispatch_tag<UpdateTask>(), Instance);
	}

	DynExp::TaskResultType NetworkSpectrometerTasks::SetExposureTimeTask::RunChild(DynExp::InstrumentInstance& Instance)
	{
		auto InstrData = DynExp::dynamic_InstrumentData_cast<NetworkSpectrometer>(Instance.InstrumentDataGetter());

		StubPtrType<DynExpProto::NetworkSpectrometer::NetworkSpectrometer> StubPtr;
		{
			auto InstrData = DynExp::dynamic_InstrumentData_cast<NetworkSpectrometer>(Instance.InstrumentDataGetter());
			StubPtr = InstrData->template GetStub<DynExpProto::NetworkSpectrometer::NetworkSpectrometer>();
		} // InstrData unlocked here.

		DynExpProto::NetworkSpectrometer::ExposureTimeMessage Message;
		Message.set_time_ms(ExposureTime.count());

		InvokeStubFunc(StubPtr, &DynExpProto::NetworkSpectrometer::NetworkSpectrometer::Stub::SetExposureTime, Message);

		return {};
	}

	DynExp::TaskResultType NetworkSpectrometerTasks::SetFrequencyRangeTask::RunChild(DynExp::InstrumentInstance& Instance)
	{
		StubPtrType<DynExpProto::NetworkSpectrometer::NetworkSpectrometer> StubPtr;
		{
			auto InstrData = DynExp::dynamic_InstrumentData_cast<NetworkSpectrometer>(Instance.InstrumentDataGetter());
			StubPtr = InstrData->template GetStub<DynExpProto::NetworkSpectrometer::NetworkSpectrometer>();
		} // InstrData unlocked here.

		DynExpProto::NetworkSpectrometer::FrequencyRangeMessage Message;
		Message.set_lowerfrequency(LowerFrequency);
		Message.set_upperfrequency(UpperFrequency);

		InvokeStubFunc(StubPtr, &DynExpProto::NetworkSpectrometer::NetworkSpectrometer::Stub::SetFrequencyRange, Message);

		return {};
	}

	DynExp::TaskResultType NetworkSpectrometerTasks::SetSetSilentModeTask::RunChild(DynExp::InstrumentInstance& Instance)
	{
		StubPtrType<DynExpProto::NetworkSpectrometer::NetworkSpectrometer> StubPtr;
		{
			auto InstrData = DynExp::dynamic_InstrumentData_cast<NetworkSpectrometer>(Instance.InstrumentDataGetter());
			StubPtr = InstrData->template GetStub<DynExpProto::NetworkSpectrometer::NetworkSpectrometer>();
		} // InstrData unlocked here.

		DynExpProto::NetworkSpectrometer::SilentModeMessage Message;
		Message.set_enable(Enable);

		InvokeStubFunc(StubPtr, &DynExpProto::NetworkSpectrometer::NetworkSpectrometer::Stub::SetSilentMode, Message);

		return {};
	}

	DynExp::TaskResultType NetworkSpectrometerTasks::RecordTask::RunChild(DynExp::InstrumentInstance& Instance)
	{
		StubPtrType<DynExpProto::NetworkSpectrometer::NetworkSpectrometer> StubPtr;
		{
			auto InstrData = DynExp::dynamic_InstrumentData_cast<NetworkSpectrometer>(Instance.InstrumentDataGetter());
			StubPtr = InstrData->template GetStub<DynExpProto::NetworkSpectrometer::NetworkSpectrometer>();
		} // InstrData unlocked here.

		auto Response = InvokeStubFunc(StubPtr, &DynExpProto::NetworkSpectrometer::NetworkSpectrometer::Stub::RecordSpectrumAsync, {});
		if (Response.result() != DynExpProto::NetworkSpectrometer::ResultType::OK)
			Instance.GetOwner().SetWarning("Failed to start spectrum acquisition.", Util::DynExpErrorCodes::ServiceFailed);

		return {};
	}

	DynExp::TaskResultType NetworkSpectrometerTasks::AbortTask::RunChild(DynExp::InstrumentInstance& Instance)
	{
		StubPtrType<DynExpProto::NetworkSpectrometer::NetworkSpectrometer> StubPtr;
		{
			auto InstrData = DynExp::dynamic_InstrumentData_cast<NetworkSpectrometer>(Instance.InstrumentDataGetter());
			StubPtr = InstrData->template GetStub<DynExpProto::NetworkSpectrometer::NetworkSpectrometer>();
		} // InstrData unlocked here.

		InvokeStubFunc(StubPtr, &DynExpProto::NetworkSpectrometer::NetworkSpectrometer::Stub::Abort, {});

		return {};
	}

	void NetworkSpectrometerData::ResetImpl(dispatch_tag<gRPCInstrumentData<Spectrometer, 0, DynExpProto::NetworkSpectrometer::NetworkSpectrometer>>)
	{
		FrequencyUnit = FrequencyUnitType::Hz;
		IntensityUnit = IntensityUnitType::Counts;
		MinFrequency = 0.0;
		MaxFrequency = 0.0;

		CapturingState = CapturingStateType::Ready;
		CapturingProgress = 0.0;

		ResetImpl(dispatch_tag<NetworkSpectrometerData>());
	}

	NetworkSpectrometer::NetworkSpectrometer(const std::thread::id OwnerThreadID, DynExp::ParamsBasePtrType&& Params)
		: gRPCInstrument<Spectrometer, 0, DynExpProto::NetworkSpectrometer::NetworkSpectrometer>(OwnerThreadID, std::move(Params))
	{
	}

	SpectrometerData::FrequencyUnitType NetworkSpectrometer::GetFrequencyUnit() const
	{
		auto InstrData = DynExp::dynamic_InstrumentData_cast<NetworkSpectrometer>(GetInstrumentData());

		return InstrData->GetFrequencyUnit();
	}

	SpectrometerData::IntensityUnitType NetworkSpectrometer::GetIntensityUnit() const
	{
		auto InstrData = DynExp::dynamic_InstrumentData_cast<NetworkSpectrometer>(GetInstrumentData());

		return InstrData->GetIntensityUnit();
	}

	double NetworkSpectrometer::GetMinFrequency() const
	{
		auto InstrData = DynExp::dynamic_InstrumentData_cast<NetworkSpectrometer>(GetInstrumentData());

		return InstrData->GetMinFrequency();
	}

	double NetworkSpectrometer::GetMaxFrequency() const
	{
		auto InstrData = DynExp::dynamic_InstrumentData_cast<NetworkSpectrometer>(GetInstrumentData());

		return InstrData->GetMaxFrequency();
	}

	void NetworkSpectrometer::ResetImpl(dispatch_tag<gRPCInstrument<Spectrometer, 0, DynExpProto::NetworkSpectrometer::NetworkSpectrometer>>)
	{
		ResetImpl(dispatch_tag<NetworkSpectrometer>());
	}
}