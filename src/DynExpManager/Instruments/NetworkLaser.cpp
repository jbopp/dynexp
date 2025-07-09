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
			InstrData->HardwareMinBandwidth = Response.hardwareminbandwidth();
			InstrData->HardwareMaxBandwidth = Response.hardwaremaxbandwidth();
			InstrData->HardwareMaxRate = Response.hardwaremaxrate();

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
			// Close laser shutter to stop emission.
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

		// Update derived instrument.
		UpdateFuncImpl(dispatch_tag<UpdateTask>(), Instance);
	}

	// specific tasks that appear in RPC protocol

	DynExp::TaskResultType NetworkLaserTasks::SetFrequencyTask::RunChild(DynExp::InstrumentInstance& Instance)
	{
		StubPtrType<DynExpProto::NetworkLaser::NetworkLaser> StubPtr;
		{
			auto InstrData = DynExp::dynamic_InstrumentData_cast<NetworkLaser>(Instance.InstrumentDataGetter());
			StubPtr = InstrData->template GetStub<DynExpProto::NetworkLaser::NetworkLaser>();
		} // InstrData unlocked here.

		auto Response = InvokeStubFunc(StubPtr, &DynExpProto::NetworkLaser::NetworkLaser::Stub::SetFrequency, {});
		if (Response.status() == DynExpProto::NetworkLaser::ValidationStatus::InvalidValue)
		{	
			Instance.GetOwner().SetWarning("Invalid frequency value.", Util::DynExpErrorCodes::InvalidArg);
			return {};
		}
		else if (Response.status() == DynExpProto::NetworkLaser::ValidationStatus::InvalidMethod)
		{
			Instance.GetOwner().SetWarning("Invalid method for this laser.", Util::DynExpErrorCodes::NotAvailable);
			return {};
		}
		DynExpProto::NetworkLaser::FrequencyMessage Message;
		Message.set_frequency(Frequency);

		InvokeStubFunc(StubPtr, &DynExpProto::NetworkLaser::NetworkLaser::Stub::SetFrequency, Message);

		return {};
	}

	DynExp::TaskResultType NetworkLaserTasks::SetIntensityTask::RunChild(DynExp::InstrumentInstance& Instance)
	{
		StubPtrType<DynExpProto::NetworkLaser::NetworkLaser> StubPtr;
		{
			auto InstrData = DynExp::dynamic_InstrumentData_cast<NetworkLaser>(Instance.InstrumentDataGetter());
			StubPtr = InstrData->template GetStub<DynExpProto::NetworkLaser::NetworkLaser>();
		} // InstrData unlocked here.

		auto Response = InvokeStubFunc(StubPtr, &DynExpProto::NetworkLaser::NetworkLaser::Stub::SetIntensity, {});
		if (Response.status() == DynExpProto::NetworkLaser::ValidationStatus::InvalidValue)
		{	
			Instance.GetOwner().SetWarning("Invalid intensity value.", Util::DynExpErrorCodes::InvalidArg);
			return {};
		}
		else if (Response.status() == DynExpProto::NetworkLaser::ValidationStatus::InvalidMethod)
		{
			Instance.GetOwner().SetWarning("Invalid method for this laser.", Util::DynExpErrorCodes::NotAvailable);
			return {};
		}

		DynExpProto::NetworkLaser::IntensityMessage Message;
		Message.set_intensity(Intensity);

		InvokeStubFunc(StubPtr, &DynExpProto::NetworkLaser::NetworkLaser::Stub::SetIntensity, Message);

		return {};
	}

	DynExp::TaskResultType NetworkLaserTasks::SetScanRangeTask::RunChild(DynExp::InstrumentInstance& Instance)
	{
		StubPtrType<DynExpProto::NetworkLaser::NetworkLaser> StubPtr;
		{
			auto InstrData = DynExp::dynamic_InstrumentData_cast<NetworkLaser>(Instance.InstrumentDataGetter());
			StubPtr = InstrData->template GetStub<DynExpProto::NetworkLaser::NetworkLaser>();
		} // InstrData unlocked here.

		auto Response = InvokeStubFunc(StubPtr, &DynExpProto::NetworkLaser::NetworkLaser::Stub::SetScanRange, {});
		if (Response.status() == DynExpProto::NetworkLaser::ValidationStatus::InvalidValue)
		{	
			Instance.GetOwner().SetWarning("Invalid scan range value.", Util::DynExpErrorCodes::InvalidArg);
			return {};
		}
		else if (Response.status() == DynExpProto::NetworkLaser::ValidationStatus::InvalidMethod)
		{
			Instance.GetOwner().SetWarning("Invalid method for this laser.", Util::DynExpErrorCodes::NotAvailable);
			return {};
		}

		DynExpProto::NetworkLaser::RangeMessage Message;
		Message.set_bandwidthinfrequnit(ScanRange);

		InvokeStubFunc(StubPtr, &DynExpProto::NetworkLaser::NetworkLaser::Stub::SetScanRange, Message);

		return {};
	}

	DynExp::TaskResultType NetworkLaserTasks::SetScanRateTask::RunChild(DynExp::InstrumentInstance& Instance)
	{
		StubPtrType<DynExpProto::NetworkLaser::NetworkLaser> StubPtr;
		{
			auto InstrData = DynExp::dynamic_InstrumentData_cast<NetworkLaser>(Instance.InstrumentDataGetter());
			StubPtr = InstrData->template GetStub<DynExpProto::NetworkLaser::NetworkLaser>();
		} // InstrData unlocked here.

		auto Response = InvokeStubFunc(StubPtr, &DynExpProto::NetworkLaser::NetworkLaser::Stub::SetScanRate, {});
		if (Response.status() == DynExpProto::NetworkLaser::ValidationStatus::InvalidValue)
		{	
			Instance.GetOwner().SetWarning("Invalid scan rate value.", Util::DynExpErrorCodes::InvalidArg);
			return {};
		}
		else if (Response.status() == DynExpProto::NetworkLaser::ValidationStatus::InvalidMethod)
		{
			Instance.GetOwner().SetWarning("Invalid method for this laser.", Util::DynExpErrorCodes::NotAvailable);
			return {};
		}

		DynExpProto::NetworkLaser::RateMessage Message;
		Message.set_speedinfrequnitpersecond(ScanRate);

		InvokeStubFunc(StubPtr, &DynExpProto::NetworkLaser::NetworkLaser::Stub::SetScanRate, Message);

		return {};
	}


	DynExp::TaskResultType NetworkLaserTasks::EnableTask::RunChild(DynExp::InstrumentInstance& Instance)
	{
		StubPtrType<DynExpProto::NetworkLaser::NetworkLaser> StubPtr;
		{
			auto InstrData = DynExp::dynamic_InstrumentData_cast<NetworkLaser>(Instance.InstrumentDataGetter());
			StubPtr = InstrData->template GetStub<DynExpProto::NetworkLaser::NetworkLaser>();
		} // InstrData unlocked here.

		InvokeStubFunc(StubPtr, &DynExpProto::NetworkLaser::NetworkLaser::Stub::Enable, {});

		auto StateResponse = InvokeStubFunc(StubPtr, &DynExpProto::NetworkLaser::NetworkLaser::Stub::GetState, {});
		if (StateResponse.state() != DynExpProto::NetworkLaser::StateType::EmissionEnabledConstant)
			Instance.GetOwner().SetWarning("Emission could not be enabled.", Util::DynExpErrorCodes::ServiceFailed);

		return {};
	}

		DynExp::TaskResultType NetworkLaserTasks::DisableTask::RunChild(DynExp::InstrumentInstance& Instance)
	{
		StubPtrType<DynExpProto::NetworkLaser::NetworkLaser> StubPtr;
		{
			auto InstrData = DynExp::dynamic_InstrumentData_cast<NetworkLaser>(Instance.InstrumentDataGetter());
			StubPtr = InstrData->template GetStub<DynExpProto::NetworkLaser::NetworkLaser>();
		} // InstrData unlocked here.

		InvokeStubFunc(StubPtr, &DynExpProto::NetworkLaser::NetworkLaser::Stub::Disable, {});

		return {};
	}
		
		DynExp::TaskResultType NetworkLaserTasks::ScanContinuouslyTask::RunChild(DynExp::InstrumentInstance& Instance)
	{
		StubPtrType<DynExpProto::NetworkLaser::NetworkLaser> StubPtr;
		{
			auto InstrData = DynExp::dynamic_InstrumentData_cast<NetworkLaser>(Instance.InstrumentDataGetter());
			StubPtr = InstrData->template GetStub<DynExpProto::NetworkLaser::NetworkLaser>();
		} // InstrData unlocked here.

		InvokeStubFunc(StubPtr, &DynExpProto::NetworkLaser::NetworkLaser::Stub::ScanContinuously, {});

		auto StateResponse = InvokeStubFunc(StubPtr, &DynExpProto::NetworkLaser::NetworkLaser::Stub::GetState, {});
		if (StateResponse.state() != DynExpProto::NetworkLaser::StateType::EmissionEnabledScanning)
			Instance.GetOwner().SetWarning("Emission in scan mode could not be enabled.", Util::DynExpErrorCodes::ServiceFailed);

		return {};
	}

		DynExp::TaskResultType NetworkLaserTasks::DisableScanTask::RunChild(DynExp::InstrumentInstance& Instance)
	{
		StubPtrType<DynExpProto::NetworkLaser::NetworkLaser> StubPtr;
		{
			auto InstrData = DynExp::dynamic_InstrumentData_cast<NetworkLaser>(Instance.InstrumentDataGetter());
			StubPtr = InstrData->template GetStub<DynExpProto::NetworkLaser::NetworkLaser>();
		} // InstrData unlocked here.

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
		HardwareMinBandwidth = 0.0;
		HardwareMaxBandwidth = 0.0;
		HardwareMaxRate = 0.0;

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
	
	double NetworkLaser::GetMinBandwidth() const
	{
		auto InstrData = DynExp::dynamic_InstrumentData_cast<NetworkLaser>(GetInstrumentData());

		return InstrData->GetMinBandwidth();
	}

	double NetworkLaser::GetMaxBandwidth() const
	{
		auto InstrData = DynExp::dynamic_InstrumentData_cast<NetworkLaser>(GetInstrumentData());

		return InstrData->GetMaxBandwidth();
	}

	double NetworkLaser::GetMaxRate() const
	{
		auto InstrData = DynExp::dynamic_InstrumentData_cast<NetworkLaser>(GetInstrumentData());

		return InstrData->GetMaxRate();
	}

	void NetworkLaser::ResetImpl(dispatch_tag<gRPCInstrument<Laser, 0, DynExpProto::NetworkLaser::NetworkLaser>>)
	{
		ResetImpl(dispatch_tag<NetworkLaser>());
	}
}