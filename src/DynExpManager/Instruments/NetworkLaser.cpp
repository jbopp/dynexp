// This file is part of DynExp.

#include "stdafx.h"
#include "NetworkLaser.h"

namespace DynExpInstr
{
	void NetworkLaserTasks::InitTask::InitFuncImpl(dispatch_tag<gRPCInstrumentTasks::InitTask<Laser, 0, DynExpProto::NetworkLaser::NetworkLaser>>, DynExp::InstrumentInstance& Instance)
	{
		StubPtrType<DynExpProto::NetworkLaser::NetworkLaser> StubPtr;
		{
			auto InstrData = DynExp::dynamic_InstrumentData_cast<NetworkLaser>(Instance.InstrumentDataGetter());
			StubPtr = InstrData->template GetStub<DynExpProto::NetworkLaser::NetworkLaser>();
		} // InstrData unlocked here.

		auto Response = InvokeStubFunc(StubPtr, &DynExpProto::NetworkLaser::NetworkLaser::Stub::GetDeviceInfo, {});

		{
			auto InstrData = DynExp::dynamic_InstrumentData_cast<NetworkLaser>(Instance.InstrumentDataGetter());

			InstrData->FrequencyUnit = ToLaserUnitType(Response.frequencyunit());
			InstrData->IntensityUnit = ToLaserUnitType(Response.intensityunit());
			InstrData->HardwareMinFrequency = Response.hardwareminfrequency();
			InstrData->HardwareMaxFrequency = Response.hardwaremaxfrequency();
			InstrData->HardwareMinIntensity = Response.hardwareminintensity();
			InstrData->HardwareMaxIntensity = Response.hardwaremaxintensity();
			InstrData->HardwareMinScanRange = Response.hardwareminscanrange();
			InstrData->HardwareMaxScanRange = Response.hardwaremaxscanrange();
			InstrData->HardwareMinScanRate = Response.hardwareminscanrate();
			InstrData->HardwareMaxScanRate = Response.hardwaremaxscanrate();
			InstrData->HardwareModeHopFreeTuningRange = Response.hardwaremodehopfreetuningrange();
		} // InstrData unlocked here.

		// Initialize derived instrument last.
		InitFuncImpl(dispatch_tag<InitTask>(), Instance);
	}

	void NetworkLaserTasks::ExitTask::ExitFuncImpl(dispatch_tag<gRPCInstrumentTasks::ExitTask<Laser, 0, DynExpProto::NetworkLaser::NetworkLaser>>, DynExp::InstrumentInstance& Instance)
	{
		// Shut down derived instrument first.
		ExitFuncImpl(dispatch_tag<ExitTask>(), Instance);

		try
		{
			// Stop laser emission.
			StubPtrType<DynExpProto::NetworkLaser::NetworkLaser> StubPtr;
			{
				auto InstrData = DynExp::dynamic_InstrumentData_cast<NetworkLaser>(Instance.InstrumentDataGetter());
				StubPtr = InstrData->template GetStub<DynExpProto::NetworkLaser::NetworkLaser>();
			} // InstrData unlocked here.

			InvokeStubFunc(StubPtr, &DynExpProto::NetworkLaser::NetworkLaser::Stub::Disable, {});
		}
		catch (...)
		{
			// Swallow any exception which might arise from instrument shutdown since a failure
			// of this function is not considered a severe error.
		}
	}

	void NetworkLaserTasks::UpdateTask::UpdateFuncImpl(dispatch_tag<gRPCInstrumentTasks::UpdateTask<Laser, 0, DynExpProto::NetworkLaser::NetworkLaser>>, DynExp::InstrumentInstance& Instance)
	{
		StubPtrType<DynExpProto::NetworkLaser::NetworkLaser> StubPtr;
		{
			auto InstrData = DynExp::dynamic_InstrumentData_cast<NetworkLaser>(Instance.InstrumentDataGetter());
			StubPtr = InstrData->template GetStub<DynExpProto::NetworkLaser::NetworkLaser>();
		} // InstrData unlocked here.

		auto StateResponse = InvokeStubFunc(StubPtr, &DynExpProto::NetworkLaser::NetworkLaser::Stub::GetState, {});
		auto FrequencyResponse = InvokeStubFunc(StubPtr, &DynExpProto::NetworkLaser::NetworkLaser::Stub::GetFrequency, {});
		auto IntensityResponse = InvokeStubFunc(StubPtr, &DynExpProto::NetworkLaser::NetworkLaser::Stub::GetIntensity, {});
		auto ScanRangeResponse = InvokeStubFunc(StubPtr, &DynExpProto::NetworkLaser::NetworkLaser::Stub::GetScanRange, {});
		auto ScanRateResponse = InvokeStubFunc(StubPtr, &DynExpProto::NetworkLaser::NetworkLaser::Stub::GetScanRate, {});

		{
			auto InstrData = DynExp::dynamic_InstrumentData_cast<NetworkLaser>(Instance.InstrumentDataGetter());

			InstrData->LaserState = ToLaserStateType(StateResponse.state());
			InstrData->SetFrequencyValue(FrequencyResponse.frequency());
			InstrData->SetIntensityValue(IntensityResponse.intensity());
			InstrData->SetScanRangeValue(ScanRangeResponse.scanrangeinfrequnit());
			InstrData->SetScanRateValue(ScanRateResponse.scanrateinfrequnitpersecond());
		} // InstrData unlocked here.

		// Update derived instrument.
		UpdateFuncImpl(dispatch_tag<UpdateTask>(), Instance);
	}

	DynExp::TaskResultType NetworkLaserTasks::SetFrequencyTask::RunChild(DynExp::InstrumentInstance& Instance)
	{
		StubPtrType<DynExpProto::NetworkLaser::NetworkLaser> StubPtr;
		{
			auto InstrData = DynExp::dynamic_InstrumentData_cast<NetworkLaser>(Instance.InstrumentDataGetter());
			StubPtr = InstrData->template GetStub<DynExpProto::NetworkLaser::NetworkLaser>();
		} // InstrData unlocked here.

		DynExpProto::NetworkLaser::FrequencyMessage Message;
		Message.set_frequency(Frequency);

		auto Response = InvokeStubFunc(StubPtr, &DynExpProto::NetworkLaser::NetworkLaser::Stub::SetFrequency, Message);

		static const std::string ErrorMsg = "Failed to set laser frequency.";
		if (Response.result() == DynExpProto::NetworkLaser::ResultType::InvalidValue)	
			Instance.GetOwner().SetWarning(ErrorMsg + " Invalid frequency value.", Util::DynExpErrorCodes::ServiceFailed);
		else if (Response.result() == DynExpProto::NetworkLaser::ResultType::InvalidMethod)
			Instance.GetOwner().SetWarning(ErrorMsg + " Invalid method for this laser.", Util::DynExpErrorCodes::ServiceFailed);
		else if (Response.result() != DynExpProto::NetworkLaser::ResultType::OK)
			Instance.GetOwner().SetWarning(ErrorMsg, Util::DynExpErrorCodes::ServiceFailed);
		
		return {};
	}

	DynExp::TaskResultType NetworkLaserTasks::SetIntensityTask::RunChild(DynExp::InstrumentInstance& Instance)
	{
		StubPtrType<DynExpProto::NetworkLaser::NetworkLaser> StubPtr;
		{
			auto InstrData = DynExp::dynamic_InstrumentData_cast<NetworkLaser>(Instance.InstrumentDataGetter());
			StubPtr = InstrData->template GetStub<DynExpProto::NetworkLaser::NetworkLaser>();
		} // InstrData unlocked here.

		DynExpProto::NetworkLaser::IntensityMessage Message;
		Message.set_intensity(Intensity);

		auto Response = InvokeStubFunc(StubPtr, &DynExpProto::NetworkLaser::NetworkLaser::Stub::SetIntensity, Message);

		static const std::string ErrorMsg = "Failed to set laser intensity.";
		if (Response.result() == DynExpProto::NetworkLaser::ResultType::InvalidValue)
			Instance.GetOwner().SetWarning(ErrorMsg + " Invalid intensity value.", Util::DynExpErrorCodes::ServiceFailed);
		else if (Response.result() == DynExpProto::NetworkLaser::ResultType::InvalidMethod)
			Instance.GetOwner().SetWarning(ErrorMsg + " Invalid method for this laser.", Util::DynExpErrorCodes::ServiceFailed);
		else if (Response.result() != DynExpProto::NetworkLaser::ResultType::OK)
			Instance.GetOwner().SetWarning(ErrorMsg, Util::DynExpErrorCodes::ServiceFailed);
		
		return {};
	}

	DynExp::TaskResultType NetworkLaserTasks::SetScanRangeTask::RunChild(DynExp::InstrumentInstance& Instance)
	{
		StubPtrType<DynExpProto::NetworkLaser::NetworkLaser> StubPtr;
		{
			auto InstrData = DynExp::dynamic_InstrumentData_cast<NetworkLaser>(Instance.InstrumentDataGetter());
			StubPtr = InstrData->template GetStub<DynExpProto::NetworkLaser::NetworkLaser>();
		} // InstrData unlocked here.

		DynExpProto::NetworkLaser::ScanRangeMessage Message;
		Message.set_scanrangeinfrequnit(ScanRange);

		auto Response = InvokeStubFunc(StubPtr, &DynExpProto::NetworkLaser::NetworkLaser::Stub::SetScanRange, Message);

		static const std::string ErrorMsg = "Failed to set laser scan range.";
		if (Response.result() == DynExpProto::NetworkLaser::ResultType::InvalidValue)
			Instance.GetOwner().SetWarning(ErrorMsg + " Invalid scan range value.", Util::DynExpErrorCodes::ServiceFailed);
		else if (Response.result() == DynExpProto::NetworkLaser::ResultType::InvalidMethod)
			Instance.GetOwner().SetWarning(ErrorMsg + " Invalid method for this laser.", Util::DynExpErrorCodes::ServiceFailed);
		else if (Response.result() != DynExpProto::NetworkLaser::ResultType::OK)
			Instance.GetOwner().SetWarning(ErrorMsg, Util::DynExpErrorCodes::ServiceFailed);

		return {};
	}

	DynExp::TaskResultType NetworkLaserTasks::SetScanRateTask::RunChild(DynExp::InstrumentInstance& Instance)
	{
		StubPtrType<DynExpProto::NetworkLaser::NetworkLaser> StubPtr;
		{
			auto InstrData = DynExp::dynamic_InstrumentData_cast<NetworkLaser>(Instance.InstrumentDataGetter());
			StubPtr = InstrData->template GetStub<DynExpProto::NetworkLaser::NetworkLaser>();
		} // InstrData unlocked here.

		DynExpProto::NetworkLaser::ScanRateMessage Message;
		Message.set_scanrateinfrequnitpersecond(ScanRate);

		auto Response = InvokeStubFunc(StubPtr, &DynExpProto::NetworkLaser::NetworkLaser::Stub::SetScanRate, Message);

		static const std::string ErrorMsg = "Failed to set laser scan rate.";
		if (Response.result() == DynExpProto::NetworkLaser::ResultType::InvalidValue)
			Instance.GetOwner().SetWarning(ErrorMsg + " Invalid scan rate value.", Util::DynExpErrorCodes::ServiceFailed);
		else if (Response.result() == DynExpProto::NetworkLaser::ResultType::InvalidMethod)
			Instance.GetOwner().SetWarning(ErrorMsg + " Invalid method for this laser.", Util::DynExpErrorCodes::ServiceFailed);
		else if (Response.result() != DynExpProto::NetworkLaser::ResultType::OK)
			Instance.GetOwner().SetWarning(ErrorMsg, Util::DynExpErrorCodes::ServiceFailed);

		return {};
	}

	DynExp::TaskResultType NetworkLaserTasks::EnableTask::RunChild(DynExp::InstrumentInstance& Instance)
	{
		StubPtrType<DynExpProto::NetworkLaser::NetworkLaser> StubPtr;
		{
			auto InstrData = DynExp::dynamic_InstrumentData_cast<NetworkLaser>(Instance.InstrumentDataGetter());
			StubPtr = InstrData->template GetStub<DynExpProto::NetworkLaser::NetworkLaser>();
		} // InstrData unlocked here.

		auto Response = InvokeStubFunc(StubPtr, &DynExpProto::NetworkLaser::NetworkLaser::Stub::Enable, {});

		static const std::string ErrorMsg = "Failed to enable laser emission.";
		if (Response.result() == DynExpProto::NetworkLaser::ResultType::InvalidMethod)
			Instance.GetOwner().SetWarning(ErrorMsg + " Invalid method for this laser.", Util::DynExpErrorCodes::ServiceFailed);
		else if (Response.result() != DynExpProto::NetworkLaser::ResultType::OK)
			Instance.GetOwner().SetWarning(ErrorMsg, Util::DynExpErrorCodes::ServiceFailed);

		return {};
	}

	DynExp::TaskResultType NetworkLaserTasks::DisableTask::RunChild(DynExp::InstrumentInstance& Instance)
	{
		StubPtrType<DynExpProto::NetworkLaser::NetworkLaser> StubPtr;
		{
			auto InstrData = DynExp::dynamic_InstrumentData_cast<NetworkLaser>(Instance.InstrumentDataGetter());
			StubPtr = InstrData->template GetStub<DynExpProto::NetworkLaser::NetworkLaser>();
		}

		InvokeStubFunc(StubPtr, &DynExpProto::NetworkLaser::NetworkLaser::Stub::Disable, {});

		return {};
	}
		
	DynExp::TaskResultType NetworkLaserTasks::ScanContinuouslyTask::RunChild(DynExp::InstrumentInstance& Instance)
	{
		StubPtrType<DynExpProto::NetworkLaser::NetworkLaser> StubPtr;
		{
			auto InstrData = DynExp::dynamic_InstrumentData_cast<NetworkLaser>(Instance.InstrumentDataGetter());
			StubPtr = InstrData->template GetStub<DynExpProto::NetworkLaser::NetworkLaser>();
		}

		auto Response = InvokeStubFunc(StubPtr, &DynExpProto::NetworkLaser::NetworkLaser::Stub::ScanContinuously, {});

		static const std::string ErrorMsg = "Failed to scan laser emission.";
		if (Response.result() == DynExpProto::NetworkLaser::ResultType::InvalidMethod)
			Instance.GetOwner().SetWarning(ErrorMsg + " Invalid method for this laser.", Util::DynExpErrorCodes::ServiceFailed);
		else if (Response.result() != DynExpProto::NetworkLaser::ResultType::OK)
			Instance.GetOwner().SetWarning(ErrorMsg, Util::DynExpErrorCodes::ServiceFailed);

		return {};
	}

	DynExp::TaskResultType NetworkLaserTasks::DisableScanTask::RunChild(DynExp::InstrumentInstance& Instance)
	{
		StubPtrType<DynExpProto::NetworkLaser::NetworkLaser> StubPtr;
		{
			auto InstrData = DynExp::dynamic_InstrumentData_cast<NetworkLaser>(Instance.InstrumentDataGetter());
			StubPtr = InstrData->template GetStub<DynExpProto::NetworkLaser::NetworkLaser>();
		}

		InvokeStubFunc(StubPtr, &DynExpProto::NetworkLaser::NetworkLaser::Stub::DisableScan, {});

		return {};
	}

	void NetworkLaserData::ResetImpl(dispatch_tag<gRPCInstrumentData<Laser, 0, DynExpProto::NetworkLaser::NetworkLaser>>)
	{
		FrequencyUnit = FrequencyUnitType::Hz;
		IntensityUnit = IntensityUnitType::Power_W;
		HardwareMinFrequency = 0.0;
		HardwareMaxFrequency = 0.0;
		HardwareMinIntensity = 0.0;
		HardwareMaxIntensity = 0.0;
		HardwareMinScanRange = 0.0;
		HardwareMaxScanRange = 0.0;
		HardwareMinScanRate = 0.0;
		HardwareMaxScanRate = 0.0;
		HardwareModeHopFreeTuningRange = 0.0;

		LaserState = LaserStateType::Ready;
		
		ResetImpl(dispatch_tag<NetworkLaserData>());
	}

	NetworkLaser::NetworkLaser(const std::thread::id OwnerThreadID, DynExp::ParamsBasePtrType&& Params)
		: gRPCInstrument<Laser, 0, DynExpProto::NetworkLaser::NetworkLaser>(OwnerThreadID, std::move(Params))
	{
	}

	LaserData::FrequencyUnitType NetworkLaser::GetFrequencyUnit() const
	{
		auto InstrData = DynExp::dynamic_InstrumentData_cast<NetworkLaser>(GetInstrumentData());

		return InstrData->GetFrequencyUnit();
	}

	LaserData::IntensityUnitType NetworkLaser::GetIntensityUnit() const
	{
		auto InstrData = DynExp::dynamic_InstrumentData_cast<NetworkLaser>(GetInstrumentData());

		return InstrData->GetIntensityUnit();
	}

	double NetworkLaser::GetMinFrequency() const
	{
		auto InstrData = DynExp::dynamic_InstrumentData_cast<NetworkLaser>(GetInstrumentData());

		return InstrData->GetMinFrequency();
	}

	double NetworkLaser::GetMaxFrequency() const
	{
		auto InstrData = DynExp::dynamic_InstrumentData_cast<NetworkLaser>(GetInstrumentData());

		return InstrData->GetMaxFrequency();
	}

	double NetworkLaser::GetMinIntensity() const
	{
		auto InstrData = DynExp::dynamic_InstrumentData_cast<NetworkLaser>(GetInstrumentData());

		return InstrData->GetMinIntensity();
	}

	double NetworkLaser::GetMaxIntensity() const
	{
		auto InstrData = DynExp::dynamic_InstrumentData_cast<NetworkLaser>(GetInstrumentData());

		return InstrData->GetMaxIntensity();
	}
	
	double NetworkLaser::GetMinScanRange() const
	{
		auto InstrData = DynExp::dynamic_InstrumentData_cast<NetworkLaser>(GetInstrumentData());

		return InstrData->GetMinScanRange();
	}

	double NetworkLaser::GetMaxScanRange() const
	{
		auto InstrData = DynExp::dynamic_InstrumentData_cast<NetworkLaser>(GetInstrumentData());

		return InstrData->GetMaxScanRange();
	}

	double NetworkLaser::GetMinScanRate() const
	{
		auto InstrData = DynExp::dynamic_InstrumentData_cast<NetworkLaser>(GetInstrumentData());

		return InstrData->GetMinScanRate();
	}

	double NetworkLaser::GetMaxScanRate() const
	{
		auto InstrData = DynExp::dynamic_InstrumentData_cast<NetworkLaser>(GetInstrumentData());

		return InstrData->GetMaxScanRate();
	}

	double NetworkLaser::GetModeHopFreeTuningRange() const
	{
		auto InstrData = DynExp::dynamic_InstrumentData_cast<NetworkLaser>(GetInstrumentData());

		return InstrData->GetModeHopFreeTuningRange();
	}

	void NetworkLaser::ResetImpl(dispatch_tag<gRPCInstrument<Laser, 0, DynExpProto::NetworkLaser::NetworkLaser>>)
	{
		ResetImpl(dispatch_tag<NetworkLaser>());
	}
}