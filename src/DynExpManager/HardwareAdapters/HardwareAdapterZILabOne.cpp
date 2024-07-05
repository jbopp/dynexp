// This file is part of DynExp.

#include "stdafx.h"
#include "HardwareAdapterZILabOne.h"

namespace DynExpHardware
{
	auto ZILabOneHardwareAdapter::Enumerate()
	{
		ZILabOneHardwareAdapterSyms::ZIConnection TempConnection;
		auto Result = ZILabOneHardwareAdapterSyms::ziAPIInit(&TempConnection);
		if (Result != ZILabOneHardwareAdapterSyms::ZI_INFO_SUCCESS)
			throw ZILabOneException("Error enumerating ZI instruments.", Result);

		std::string DeviceList;
		for (int i = 1; i < 100; ++i)
		{
			DeviceList.resize(i * 256);
			
			// Since C++17, writing to std::string's internal buffer is allowed.
			Result = ZILabOneHardwareAdapterSyms::ziAPIDiscoveryFindAll(TempConnection, DeviceList.data(), Util::NumToT<uint32_t>(DeviceList.size()));
			if (Result != ZILabOneHardwareAdapterSyms::ZI_ERROR_LENGTH)
				break;
		}

		ZILabOneHardwareAdapterSyms::ziAPIDestroy(TempConnection);

		// Check result of ziAPIDiscoveryFindAll() here.
		if (Result != ZILabOneHardwareAdapterSyms::ZI_INFO_SUCCESS)
			throw ZILabOneException("Error enumerating ZI instruments.", Result);

		// All descriptors are separated by '\n'. Replace that by ' ' and use std::istringstream to split by ' '.
		std::vector<std::string> DeviceDescriptors;
		if (DeviceList[0] != '\0')
		{
			std::replace(DeviceList.begin(), DeviceList.end(), '\n', ' ');
			std::string DeviceListRemovedZeros;
			std::remove_copy(DeviceList.begin(), DeviceList.end(), std::back_inserter(DeviceListRemovedZeros), '\0');
			std::istringstream ss(DeviceListRemovedZeros);
			DeviceDescriptors = { std::istream_iterator<std::string>(ss), std::istream_iterator<std::string>() };
		}

		return DeviceDescriptors;
	}

	void ZILabOneHardwareAdapterParams::ConfigureParamsImpl(dispatch_tag<HardwareAdapterParamsBase>)
	{
		auto ZILabOneHardwareAdapterDevices = ZILabOneHardwareAdapter::Enumerate();
		if (!DeviceDescriptor.Get().empty() &&
			std::find(ZILabOneHardwareAdapterDevices.cbegin(), ZILabOneHardwareAdapterDevices.cend(), DeviceDescriptor) == std::cend(ZILabOneHardwareAdapterDevices))
			ZILabOneHardwareAdapterDevices.push_back(DeviceDescriptor);
		if (ZILabOneHardwareAdapterDevices.empty())
			throw Util::EmptyException("There is not any available ZILabOneHardwareAdapter controller.");

		DeviceDescriptor.SetTextList(std::move(ZILabOneHardwareAdapterDevices));

		ConfigureParamsImpl(dispatch_tag<ZILabOneHardwareAdapterParams>());
	}

	ZILabOneHardwareAdapter::ZILabOneHardwareAdapter(const std::thread::id OwnerThreadID, DynExp::ParamsBasePtrType&& Params)
		: HardwareAdapterBase(OwnerThreadID, std::move(Params))
	{
		Init();
	}

	ZILabOneHardwareAdapter::~ZILabOneHardwareAdapter()
	{
		// Not locking, since the object is being destroyed. This should be inherently thread-safe.
		CloseUnsafe();
		ZILabOneHardwareAdapterSyms::ziAPIDestroy(ZIConnection);
	}

	void ZILabOneHardwareAdapter::ConfigureInput(SignalInputType SignalInput, uint8_t Demodulator) const
	{
		auto lock = AcquireLock(HardwareOperationTimeout);

		ConfigureInputUnsafe(SignalInput, Demodulator);
	}

	double ZILabOneHardwareAdapter::StartAcquisition(uint8_t Demodulator, size_t NumSamples, size_t NumRuns, bool AverageRuns) const
	{
		auto lock = AcquireLock(HardwareOperationTimeout);

		StartAcquisitionUnsafe(Demodulator, NumSamples, NumRuns, AverageRuns);

		return static_cast<double>(NumSamples) / GetDemodSamplingRateUnsafe(Demodulator);
	}

	void ZILabOneHardwareAdapter::StopAcquisition() const
	{
		auto lock = AcquireLock(HardwareOperationTimeout);

		StopAcquisitionUnsafe();
	}

	bool ZILabOneHardwareAdapter::HasFinishedAcquisition() const
	{
		auto lock = AcquireLock(HardwareOperationTimeout);

		return HasFinishedAcquisitionUnsafe();
	}

	double ZILabOneHardwareAdapter::GetAcquisitionProgress() const
	{
		auto lock = AcquireLock(HardwareOperationTimeout);

		return GetAcquisitionProgressUnsafe();
	}

	void ZILabOneHardwareAdapter::ForceTrigger() const
	{
		auto lock = AcquireLock(HardwareOperationTimeout);

		ForceTriggerUnsafe();
	}

	void ZILabOneHardwareAdapter::ClearAcquiredData() const
	{
		auto lock = AcquireLock(HardwareOperationTimeout);

		ClearAcquiredDataUnsafe();
	}

	std::vector<DynExpInstr::LockinAmplifierDefs::LockinSample> ZILabOneHardwareAdapter::GetAcquiredData() const
	{
		auto lock = AcquireLock(HardwareOperationTimeout);

		return GetAcquiredDataUnsafe();
	}

	double ZILabOneHardwareAdapter::GetInputRange(SignalInputType SignalInput) const
	{
		auto lock = AcquireLock(HardwareOperationTimeout);

		return GetInputRangeUnsafe(SignalInput);
	}

	void ZILabOneHardwareAdapter::SetInputRange(SignalInputType SignalInput, double InputRange) const
	{
		auto lock = AcquireLock(HardwareOperationTimeout);

		SetInputRangeUnsafe(SignalInput, InputRange);
	}

	void ZILabOneHardwareAdapter::AutoAdjustInputRange(SignalInputType SignalInput) const
	{
		auto lock = AcquireLock(HardwareOperationTimeout);

		AutoAdjustInputRangeUnsafe(SignalInput);
	}

	bool ZILabOneHardwareAdapter::IsInputOverload(SignalInputType SignalInput) const
	{
		auto lock = AcquireLock(HardwareOperationTimeout);

		return IsInputOverloadUnsafe(SignalInput);
	}

	double ZILabOneHardwareAdapter::GetNegInputLoad(SignalInputType SignalInput) const
	{
		auto lock = AcquireLock(HardwareOperationTimeout);

		return GetNegInputLoadUnsafe(SignalInput);
	}

	double ZILabOneHardwareAdapter::GetPosInputLoad(SignalInputType SignalInput) const
	{
		auto lock = AcquireLock(HardwareOperationTimeout);

		return GetPosInputLoadUnsafe(SignalInput);
	}

	double ZILabOneHardwareAdapter::GetDemodPhase(uint8_t Demodulator) const
	{
		auto lock = AcquireLock(HardwareOperationTimeout);

		return GetDemodPhaseUnsafe(Demodulator);
	}

	void ZILabOneHardwareAdapter::SetDemodPhase(uint8_t Demodulator, double Phase) const
	{
		auto lock = AcquireLock(HardwareOperationTimeout);

		SetDemodPhaseUnsafe(Demodulator, Phase);
	}

	void ZILabOneHardwareAdapter::AutoAdjustDemodPhase(uint8_t Demodulator) const
	{
		auto lock = AcquireLock(HardwareOperationTimeout);

		AutoAdjustDemodPhaseUnsafe(Demodulator);
	}

	double ZILabOneHardwareAdapter::GetDemodTimeConstant(uint8_t Demodulator) const
	{
		auto lock = AcquireLock(HardwareOperationTimeout);

		return GetDemodTimeConstantUnsafe(Demodulator);
	}

	void ZILabOneHardwareAdapter::SetDemodTimeConstant(uint8_t Demodulator, double TimeConstant) const
	{
		auto lock = AcquireLock(HardwareOperationTimeout);

		SetDemodTimeConstantUnsafe(Demodulator, TimeConstant);
	}

	uint8_t ZILabOneHardwareAdapter::GetDemodFilterOrder(uint8_t Demodulator) const
	{
		auto lock = AcquireLock(HardwareOperationTimeout);

		return GetDemodFilterOrderUnsafe(Demodulator);
	}

	void ZILabOneHardwareAdapter::SetDemodFilterOrder(uint8_t Demodulator, uint8_t FilterOrder) const
	{
		auto lock = AcquireLock(HardwareOperationTimeout);

		SetDemodFilterOrderUnsafe(Demodulator, FilterOrder);
	}

	DynExpInstr::LockinAmplifierDefs::TriggerModeType ZILabOneHardwareAdapter::GetTriggerMode() const
	{
		auto lock = AcquireLock(HardwareOperationTimeout);

		return GetTriggerModeUnsafe();
	}

	void ZILabOneHardwareAdapter::SetTriggerMode(DynExpInstr::LockinAmplifierDefs::TriggerModeType TriggerMode, uint8_t Demodulator, uint8_t TriggerChannel) const
	{
		auto lock = AcquireLock(HardwareOperationTimeout);

		SetTriggerModeUnsafe(TriggerMode, Demodulator, TriggerChannel);
	}

	DynExpInstr::LockinAmplifierDefs::TriggerEdgeType ZILabOneHardwareAdapter::GetTriggerEdge() const
	{
		auto lock = AcquireLock(HardwareOperationTimeout);

		return GetTriggerEdgeUnsafe();
	}

	void ZILabOneHardwareAdapter::SetTriggerEdge(DynExpInstr::LockinAmplifierDefs::TriggerEdgeType TriggerEdge) const
	{
		auto lock = AcquireLock(HardwareOperationTimeout);

		SetTriggerEdgeUnsafe(TriggerEdge);
	}

	double ZILabOneHardwareAdapter::GetDemodSamplingRate(uint8_t Demodulator) const
	{
		auto lock = AcquireLock(HardwareOperationTimeout);

		return GetDemodSamplingRateUnsafe(Demodulator);
	}

	void ZILabOneHardwareAdapter::SetDemodSamplingRate(uint8_t Demodulator, double SamplingRate) const
	{
		auto lock = AcquireLock(HardwareOperationTimeout);

		SetDemodSamplingRateUnsafe(Demodulator, SamplingRate);
	}

	bool ZILabOneHardwareAdapter::GetEnabled(uint8_t Demodulator) const
	{
		auto lock = AcquireLock(HardwareOperationTimeout);

		return GetEnabledUnsafe(Demodulator);
	}

	void ZILabOneHardwareAdapter::SetEnabled(uint8_t Demodulator, bool Enabled) const
	{
		auto lock = AcquireLock(HardwareOperationTimeout);

		SetEnabledUnsafe(Demodulator, Enabled);
	}

	double ZILabOneHardwareAdapter::GetOscillatorFrequency(uint8_t Oscillator) const
	{
		auto lock = AcquireLock(HardwareOperationTimeout);

		return GetOscillatorFrequencyUnsafe(Oscillator);
	}

	void ZILabOneHardwareAdapter::Init()
	{
		auto Result = ZILabOneHardwareAdapterSyms::ziAPIInit(&ZIConnection);
		CheckError(Result);

		Opened = false;

		auto DerivedParams = dynamic_Params_cast<ZILabOneHardwareAdapter>(GetParams());
		DeviceDescriptor = DerivedParams->DeviceDescriptor;
		Interface = DerivedParams->Interface;
		Clockbase = 1;
	}

	void ZILabOneHardwareAdapter::ResetImpl(dispatch_tag<HardwareAdapterBase>)
	{
		// auto lock = AcquireLock(); not necessary here, since DynExp ensures that Object::Reset() can only
		// be called if respective object is not in use.

		CloseUnsafe();
		ZILabOneHardwareAdapterSyms::ziAPIDestroy(ZIConnection);

		Init();

		ResetImpl(dispatch_tag<ZILabOneHardwareAdapter>());
	}

	void ZILabOneHardwareAdapter::EnsureReadyStateChild()
	{
		auto lock = AcquireLock(HardwareOperationTimeout);

		OpenUnsafe();
	}

	bool ZILabOneHardwareAdapter::IsReadyChild() const
	{
		auto lock = AcquireLock(HardwareOperationTimeout);

		auto Exception = GetExceptionUnsafe();
		Util::ForwardException(Exception);

		return IsOpened();
	}

	bool ZILabOneHardwareAdapter::IsConnectedChild() const noexcept
	{
		return IsOpened();
	}

	void ZILabOneHardwareAdapter::CheckError(const ZILabOneHardwareAdapterSyms::ZIResult_enum Result, const std::source_location Location) const
	{
		if (Result == ZILabOneHardwareAdapterSyms::ZI_INFO_SUCCESS)
			return;

		char* ErrorString = nullptr;
		ZILabOneHardwareAdapterSyms::ziAPIGetError(Result, &ErrorString, nullptr);

		// AcquireLock() has already been called by an (in)direct caller of this function.
		if (ErrorString)
			ThrowExceptionUnsafe(std::make_exception_ptr(ZILabOneException(ErrorString, Result, Location)));
		else
			ThrowExceptionUnsafe(std::make_exception_ptr(ZILabOneException("< No description available. >", Result, Location)));
	}

	void ZILabOneHardwareAdapter::OpenUnsafe()
	{
		if (IsOpened())
			return;

		const char* DeviceID = nullptr;
		const char* ServerAddress;
		ZILabOneHardwareAdapterSyms::ZIIntegerData Port = 0;
		ZILabOneHardwareAdapterSyms::ZIIntegerData APILevel = 6;

		auto Result = ZILabOneHardwareAdapterSyms::ziAPIDiscoveryFind(ZIConnection, DeviceDescriptor.c_str(), &DeviceID);
		CheckError(Result);
		if (DeviceID)
			DeviceDescriptor = DeviceID;	// To get rid of some trailing zeros.
		
		Result = ZILabOneHardwareAdapterSyms::ziAPIDiscoveryGetValueS(ZIConnection, DeviceDescriptor.c_str(), "serveraddress", &ServerAddress);
		CheckError(Result);
		Result = ZILabOneHardwareAdapterSyms::ziAPIDiscoveryGetValueI(ZIConnection, DeviceDescriptor.c_str(), "serverport", &Port);
		CheckError(Result);
		Result = ZILabOneHardwareAdapterSyms::ziAPIDiscoveryGetValueI(ZIConnection, DeviceDescriptor.c_str(), "apilevel", &APILevel);
		CheckError(Result);
		Result = ZILabOneHardwareAdapterSyms::ziAPIConnectEx(ZIConnection, ServerAddress, static_cast<uint16_t>(Port),
			static_cast<ZILabOneHardwareAdapterSyms::ZIAPIVersion_enum>(APILevel), nullptr);
		CheckError(Result);

		const char* Connected;
		Result = ZILabOneHardwareAdapterSyms::ziAPIDiscoveryGetValueS(ZIConnection, DeviceDescriptor.c_str(), "connected", &Connected);
		CheckError(Result);
		if (std::string(Connected).empty())
		{
			// Connect to data server if not connected yet.
			const char* Interfaces;
			ZILabOneHardwareAdapterSyms::ziAPIDiscoveryGetValueS(ZIConnection, DeviceDescriptor.c_str(), "interfaces", &Interfaces);
			CheckError(Result);

			// All descriptors are separated by '\n'. Replace that by ' ' and use std::istringstream to split by ' '.
			std::string InterfacesStr(Interfaces);
			std::replace(InterfacesStr.begin(), InterfacesStr.end(), '\n', ' ');
			std::istringstream ss(InterfacesStr);
			std::vector<std::string> InterfacesList{ std::istream_iterator<std::string>(ss), std::istream_iterator<std::string>() };
			if (InterfacesList.empty())
				ThrowExceptionUnsafe(std::make_exception_ptr(Util::EmptyException(
					"The interface list of the selected ZI instrument is empty.")));
			if (std::find(InterfacesList.cbegin(), InterfacesList.cend(), Interface) == InterfacesList.cend())
				ThrowExceptionUnsafe(std::make_exception_ptr(Util::NotFoundException(
					"The ZI instrument does not support the specified interface for establishing a connection.")));

			Result = ZILabOneHardwareAdapterSyms::ziAPIConnectDevice(ZIConnection, DeviceDescriptor.c_str(), Util::ToLower(Interface).c_str(), nullptr);
			CheckError(Result);
		}

		Result = ZILabOneHardwareAdapterSyms::ziAPIModCreate(ZIConnection, &DAQModuleHandle, "dataAcquisitionModule");
		CheckError(Result);
		Result = ZILabOneHardwareAdapterSyms::ziAPIModSetString(ZIConnection, DAQModuleHandle, "device", DeviceDescriptor.c_str());
		CheckError(Result);
		Result = ZILabOneHardwareAdapterSyms::ziAPIModSetIntegerData(ZIConnection, DAQModuleHandle, "historylength", 1);
		CheckError(Result);
		Result = ZILabOneHardwareAdapterSyms::ziAPIModSetDoubleData(ZIConnection, DAQModuleHandle, "bandwidth", 0);
		CheckError(Result);
		Result = ZILabOneHardwareAdapterSyms::ziAPIModSetDoubleData(ZIConnection, DAQModuleHandle, "level", 1);
		CheckError(Result);
		Result = ZILabOneHardwareAdapterSyms::ziAPIModSetDoubleData(ZIConnection, DAQModuleHandle, "hysteresis", .01);
		CheckError(Result);
		Result = ZILabOneHardwareAdapterSyms::ziAPIModSetIntegerData(ZIConnection, DAQModuleHandle, "count", 1);
		CheckError(Result);
		Result = ZILabOneHardwareAdapterSyms::ziAPIModSetIntegerData(ZIConnection, DAQModuleHandle, "preview", 1);
		CheckError(Result);
		
		StopAcquisitionUnsafe();

		Clockbase = ReadDoubleUnsafe("clockbase");

		Opened = true;
	}

	void ZILabOneHardwareAdapter::CloseUnsafe()
	{
		if (IsOpened())
		{
			// Handles now considered invalid, even if disconnecting fails.
			Opened = false;

			auto Result = ZILabOneHardwareAdapterSyms::ziAPIModClear(ZIConnection, DAQModuleHandle);
			CheckError(Result);
			Result = ZILabOneHardwareAdapterSyms::ziAPIDisconnectDevice(ZIConnection, DeviceDescriptor.c_str());
			CheckError(Result);
			Result = ZILabOneHardwareAdapterSyms::ziAPIDisconnect(ZIConnection);
			CheckError(Result);
		}
	}

	void ZILabOneHardwareAdapter::ConfigureInputUnsafe(SignalInputType SignalInput, uint8_t Demodulator) const
	{
		WriteIntUnsafe("demods/" + Util::ToStr(Demodulator) + "/adcselect", SignalInput == SignalInputType::Current);
		if (SignalInput != SignalInputType::Current)
			WriteIntUnsafe(SignalInputTypeToCmdStr(SignalInput) + "/0/diff", SignalInput == SignalInputType::DifferentialVoltage);
	}

	void ZILabOneHardwareAdapter::StartAcquisitionUnsafe(uint8_t Demodulator, size_t NumSamples, size_t NumRuns, bool AverageRuns) const
	{
		auto Result = ZILabOneHardwareAdapterSyms::ziAPIModSetIntegerData(ZIConnection, DAQModuleHandle, "grid/mode", 4);	// Exact (on-grid)
		CheckError(Result);
		Result = ZILabOneHardwareAdapterSyms::ziAPIModSetIntegerData(ZIConnection, DAQModuleHandle, "grid/cols", NumSamples);
		CheckError(Result);
		Result = ZILabOneHardwareAdapterSyms::ziAPIModSetIntegerData(ZIConnection, DAQModuleHandle, "grid/rows", 1);
		CheckError(Result);
		Result = ZILabOneHardwareAdapterSyms::ziAPIModSetIntegerData(ZIConnection, DAQModuleHandle, "grid/repetitions", NumRuns);
		CheckError(Result);
		Result = ZILabOneHardwareAdapterSyms::ziAPIModSetIntegerData(ZIConnection, DAQModuleHandle, "grid/overwrite", !AverageRuns);
		CheckError(Result);
		Result = ZILabOneHardwareAdapterSyms::ziAPIModSetIntegerData(ZIConnection, DAQModuleHandle, "endless", 0);
		CheckError(Result);

		Result = ZILabOneHardwareAdapterSyms::ziAPIModSubscribe(ZIConnection, DAQModuleHandle,
			("/" + DeviceDescriptor + "/demods/" + Util::ToStr(Demodulator) + "/sample.x").c_str());
		Result = ZILabOneHardwareAdapterSyms::ziAPIModSubscribe(ZIConnection, DAQModuleHandle,
			("/" + DeviceDescriptor + "/demods/" + Util::ToStr(Demodulator) + "/sample.y").c_str());
		CheckError(Result);
		Result = ZILabOneHardwareAdapterSyms::ziAPIModExecute(ZIConnection, DAQModuleHandle);
		CheckError(Result);
	}

	void ZILabOneHardwareAdapter::StopAcquisitionUnsafe() const
	{
		auto Result = ZILabOneHardwareAdapterSyms::ziAPIModFinish(ZIConnection, DAQModuleHandle);
		CheckError(Result);
		Result = ZILabOneHardwareAdapterSyms::ziAPIModUnSubscribe(ZIConnection, DAQModuleHandle, "*");
		CheckError(Result);
	}

	bool ZILabOneHardwareAdapter::HasFinishedAcquisitionUnsafe() const
	{
		ZILabOneHardwareAdapterSyms::ZIIntegerData Finished = false;
		auto Result = ZILabOneHardwareAdapterSyms::ziAPIModFinished(ZIConnection, DAQModuleHandle, &Finished);
		CheckError(Result);

		return Finished;
	}

	double ZILabOneHardwareAdapter::GetAcquisitionProgressUnsafe() const
	{
		ZILabOneHardwareAdapterSyms::ZIDoubleData Progress{};
		auto Result = ZILabOneHardwareAdapterSyms::ziAPIModProgress(ZIConnection, DAQModuleHandle, &Progress);
		CheckError(Result);

		return Progress;
	}

	void ZILabOneHardwareAdapter::ForceTriggerUnsafe() const
	{
		auto Result = ZILabOneHardwareAdapterSyms::ziAPIModSetIntegerData(ZIConnection, DAQModuleHandle, "forcetrigger", 1);
		CheckError(Result);
	}

	void ZILabOneHardwareAdapter::ClearAcquiredDataUnsafe() const
	{
		auto Result = ZILabOneHardwareAdapterSyms::ziAPIModSetIntegerData(ZIConnection, DAQModuleHandle, "clearhistory", 1);
		CheckError(Result);
	}

	std::vector<DynExpInstr::LockinAmplifierDefs::LockinSample> ZILabOneHardwareAdapter::GetAcquiredDataUnsafe() const
	{
		std::map<double, DynExpInstr::LockinAmplifierDefs::LockinSample> RawSamples;
		auto Result = ZILabOneHardwareAdapterSyms::ziAPIModRead(ZIConnection, DAQModuleHandle, "");
		CheckError(Result);

		std::string NodePath(1024, '\0');
		ZILabOneHardwareAdapterSyms::ZIModuleEventPtr EventPtr = nullptr;
		ZILabOneHardwareAdapterSyms::ZIValueType_enum EventValueType{};
		uint64_t NumChunks{};
		bool ZeroTimestampSet = false;
		ZILabOneHardwareAdapterSyms::ZITimeStamp ZeroTimestamp{ 0 };

		try
		{
			while (true)
			{
				Result = ZILabOneHardwareAdapterSyms::ziAPIModNextNode(ZIConnection, DAQModuleHandle,
					NodePath.data(), Util::NumToT<uint32_t>(NodePath.length()), &EventValueType, &NumChunks);
				if (Result == ZILabOneHardwareAdapterSyms::ZI_WARNING_NOTFOUND)
					break;
				CheckError(Result);

				for (decltype(NumChunks) CurrentChunk = 0; CurrentChunk < NumChunks; ++CurrentChunk)
				{
					Result = ZILabOneHardwareAdapterSyms::ziAPIModGetChunk(ZIConnection, DAQModuleHandle, CurrentChunk, &EventPtr);
					CheckError(Result);

					if (EventPtr->value->valueType == ZILabOneHardwareAdapterSyms::ZI_VALUE_TYPE_DOUBLE_DATA_TS)
					{
						ZILabOneHardwareAdapterSyms::ZIEvent& e = *EventPtr->value;

						// Obtain demodulator number from ".../demods/<demodulator number>/sample.<x/y>"
						std::string Path(e.path, e.path + sizeof(e.path) / sizeof(e.path[0]));
						auto StartDemodStr = Path.find("demods/") + 7;
						auto EndDemodStr = Path.find("/sample");
						auto Demodulator = std::stoul(Path.substr(StartDemodStr, EndDemodStr - StartDemodStr));
						if (Demodulator > std::numeric_limits<uint8_t>::max())
							ThrowExceptionUnsafe(std::make_exception_ptr(Util::OverflowException(
								"A received demodulator number is invalid.")));

						// Obtain x/y channel from ".../demods/<demodulator number>/sample.<x/y>"
						auto StartChannelStr = Path.find("sample.") + 7;
						auto Channel = Path.substr(StartChannelStr, 1);
						if (Channel != "x" && Channel != "y")
							ThrowExceptionUnsafe(std::make_exception_ptr(Util::InvalidDataException(
								"A received demodulator channel is invalid.")));
						bool IsX = Channel == "x";

						for (size_t i = 0; i < e.count; ++i)
						{
							if (!ZeroTimestampSet)
							{
								ZeroTimestamp = e.value.doubleDataTS[i].timeStamp;
								ZeroTimestampSet = true;
							}

							auto Timestamp = static_cast<double>(e.value.doubleDataTS[i].timeStamp - ZeroTimestamp) / Clockbase;
							RawSamples[Timestamp].Time = Timestamp;
							RawSamples[Timestamp].Channel = static_cast<uint8_t>(Demodulator);

							if (IsX)
								RawSamples[Timestamp].CartesianResult.X = e.value.doubleDataTS[i].value;
							else
								RawSamples[Timestamp].CartesianResult.Y = e.value.doubleDataTS[i].value;
						}
					}
				}
			}
		}
		catch (...)
		{
			// Ignore errors occurring here.
			if (EventPtr)
				ZILabOneHardwareAdapterSyms::ziAPIModEventDeallocate(ZIConnection, DAQModuleHandle, EventPtr);

			throw;
		}

		if (EventPtr)
		{
			Result = ZILabOneHardwareAdapterSyms::ziAPIModEventDeallocate(ZIConnection, DAQModuleHandle, EventPtr);
			CheckError(Result);
		}

		std::vector<DynExpInstr::LockinAmplifierDefs::LockinSample> Samples;
		for (auto& Sample : RawSamples)
		{
			Sample.second.UpdatePolar();
			Samples.push_back(Sample.second);
		}

		return Samples;
	}

	std::string ZILabOneHardwareAdapter::SignalInputTypeToCmdStr(SignalInputType SignalInput) const
	{
		switch (SignalInput)
		{
		case SignalInputType::Current: return "currins";
		default: return "sigins";
		}
	}

	double ZILabOneHardwareAdapter::GetInputRangeUnsafe(SignalInputType SignalInput) const
	{
		return ReadDoubleUnsafe(SignalInputTypeToCmdStr(SignalInput) + "/0/range");
	}

	void ZILabOneHardwareAdapter::SetInputRangeUnsafe(SignalInputType SignalInput, double InputRange) const
	{
		WriteDoubleUnsafe(SignalInputTypeToCmdStr(SignalInput) + "/0/range", InputRange);
	}

	void ZILabOneHardwareAdapter::AutoAdjustInputRangeUnsafe(SignalInputType SignalInput) const
	{
		WriteIntUnsafe(SignalInputTypeToCmdStr(SignalInput) + "/0/autorange", 1);
	}

	bool ZILabOneHardwareAdapter::IsInputOverloadUnsafe(SignalInputType SignalInput) const
	{
		return DetermineOverload(GetPosInputLoadUnsafe(SignalInput), GetNegInputLoadUnsafe(SignalInput));
	}

	double ZILabOneHardwareAdapter::GetNegInputLoadUnsafe(SignalInputType SignalInput) const
	{
		// Reading this value seems to take relatively much time.
		return std::abs(ReadDoubleUnsafe(SignalInputTypeToCmdStr(SignalInput) + "/0/min"));
	}

	double ZILabOneHardwareAdapter::GetPosInputLoadUnsafe(SignalInputType SignalInput) const
	{
		// Reading this value seems to take relatively much time.
		return std::abs(ReadDoubleUnsafe(SignalInputTypeToCmdStr(SignalInput) + "/0/max"));
	}

	double ZILabOneHardwareAdapter::GetDemodPhaseUnsafe(uint8_t Demodulator) const
	{
		return ReadDoubleUnsafe("demods/" + Util::ToStr(Demodulator) + "/phaseshift") / 180.0 * std::numbers::pi + std::numbers::pi;
	}

	void ZILabOneHardwareAdapter::SetDemodPhaseUnsafe(uint8_t Demodulator, double Phase) const
	{
		WriteDoubleUnsafe("demods/" + Util::ToStr(Demodulator) + "/phaseshift", Phase / std::numbers::pi * 180.0 - 180.0);
	}

	void ZILabOneHardwareAdapter::AutoAdjustDemodPhaseUnsafe(uint8_t Demodulator) const
	{
		WriteIntUnsafe("demods/" + Util::ToStr(Demodulator) + "/phaseadjust", 1);
	}

	double ZILabOneHardwareAdapter::GetDemodTimeConstantUnsafe(uint8_t Demodulator) const
	{
		return ReadDoubleUnsafe("demods/" + Util::ToStr(Demodulator) + "/timeconstant");
	}

	void ZILabOneHardwareAdapter::SetDemodTimeConstantUnsafe(uint8_t Demodulator, double TimeConstant) const
	{
		WriteDoubleUnsafe("demods/" + Util::ToStr(Demodulator) + "/timeconstant", TimeConstant);
	}

	uint8_t ZILabOneHardwareAdapter::GetDemodFilterOrderUnsafe(uint8_t Demodulator) const
	{
		return ReadIntUnsafe("demods/" + Util::ToStr(Demodulator) + "/order");
	}

	void ZILabOneHardwareAdapter::SetDemodFilterOrderUnsafe(uint8_t Demodulator, uint8_t FilterOrder) const
	{
		WriteIntUnsafe("demods/" + Util::ToStr(Demodulator) + "/order", FilterOrder);
	}

	DynExpInstr::LockinAmplifierDefs::TriggerModeType ZILabOneHardwareAdapter::GetTriggerModeUnsafe() const
	{
		std::string Data(1024, '\0');
		unsigned int Length{};

		auto Result = ZILabOneHardwareAdapterSyms::ziAPIModGetString(ZIConnection, DAQModuleHandle, "triggernode", Data.data(), &Length, Util::NumToT<unsigned int>(Data.length()));
		CheckError(Result);

		Data.resize(Length);

		return Data.find("/sample.TrigIn") == std::string::npos ? DynExpInstr::LockinAmplifierDefs::TriggerModeType::Continuous
			: DynExpInstr::LockinAmplifierDefs::TriggerModeType::ExternSingle;
	}

	void ZILabOneHardwareAdapter::SetTriggerModeUnsafe(DynExpInstr::LockinAmplifierDefs::TriggerModeType TriggerMode, uint8_t Demodulator, uint8_t TriggerChannel) const
	{
		if (TriggerMode == DynExpInstr::LockinAmplifierDefs::TriggerModeType::ExternSingle)
		{
			auto Result = ZILabOneHardwareAdapterSyms::ziAPIModSetString(ZIConnection, DAQModuleHandle, "triggernode",
				("/" + DeviceDescriptor + "/demods/" + Util::ToStr(Demodulator) + "/sample.TrigIn" + Util::ToStr(TriggerChannel)).c_str());
			CheckError(Result);
			Result = ZILabOneHardwareAdapterSyms::ziAPIModSetIntegerData(ZIConnection, DAQModuleHandle, "type", 6);	// Trigger on HW (!) edge
			CheckError(Result);
		}
		else if (TriggerMode == DynExpInstr::LockinAmplifierDefs::TriggerModeType::Continuous)
		{
			auto Result = ZILabOneHardwareAdapterSyms::ziAPIModSetString(ZIConnection, DAQModuleHandle, "triggernode",
				("/" + DeviceDescriptor + "/demods/" + Util::ToStr(Demodulator) + "/sample.r").c_str());
			CheckError(Result);
			Result = ZILabOneHardwareAdapterSyms::ziAPIModSetIntegerData(ZIConnection, DAQModuleHandle, "type", 0);	// Trigger continuously
			CheckError(Result);
		}
		else
			ThrowExceptionUnsafe(std::make_exception_ptr(Util::InvalidArgException("The given trigger mode is invalid.")));
	}

	DynExpInstr::LockinAmplifierDefs::TriggerEdgeType ZILabOneHardwareAdapter::GetTriggerEdgeUnsafe() const
	{
		ZILabOneHardwareAdapterSyms::ZIIntegerData Data;
		auto Result = ZILabOneHardwareAdapterSyms::ziAPIModGetInteger(ZIConnection, DAQModuleHandle, "edge", &Data);
		CheckError(Result);

		return Data == 2 ? DynExpInstr::LockinAmplifierDefs::TriggerEdgeType::Fall : DynExpInstr::LockinAmplifierDefs::TriggerEdgeType::Rise;
	}

	void ZILabOneHardwareAdapter::SetTriggerEdgeUnsafe(DynExpInstr::LockinAmplifierDefs::TriggerEdgeType TriggerEdge) const
	{
		if (TriggerEdge != DynExpInstr::LockinAmplifierDefs::TriggerEdgeType::Rise && TriggerEdge != DynExpInstr::LockinAmplifierDefs::TriggerEdgeType::Fall)
			ThrowExceptionUnsafe(std::make_exception_ptr(Util::InvalidArgException("The given trigger edge is invalid.")));

		auto Result = ZILabOneHardwareAdapterSyms::ziAPIModSetIntegerData(ZIConnection, DAQModuleHandle, "edge",
			TriggerEdge == DynExpInstr::LockinAmplifierDefs::TriggerEdgeType::Fall ? 2 : 1);
		CheckError(Result);
	}

	double ZILabOneHardwareAdapter::GetDemodSamplingRateUnsafe(uint8_t Demodulator) const
	{
		return ReadDoubleUnsafe("demods/" + Util::ToStr(Demodulator) + "/rate");
	}

	void ZILabOneHardwareAdapter::SetDemodSamplingRateUnsafe(uint8_t Demodulator, double SamplingRate) const
	{
		WriteDoubleUnsafe("demods/" + Util::ToStr(Demodulator) + "/rate", SamplingRate);
	}

	bool ZILabOneHardwareAdapter::GetEnabledUnsafe(uint8_t Demodulator) const
	{
		return ReadIntUnsafe("demods/" + Util::ToStr(Demodulator) + "/enable");
	}

	void ZILabOneHardwareAdapter::SetEnabledUnsafe(uint8_t Demodulator, bool Enabled) const
	{
		WriteIntUnsafe("demods/" + Util::ToStr(Demodulator) + "/enable", Enabled);
	}

	double ZILabOneHardwareAdapter::GetOscillatorFrequencyUnsafe(uint8_t Oscillator) const
	{
		return ReadDoubleUnsafe("oscs/" + Util::ToStr(Oscillator) + "/freq");
	}

	double ZILabOneHardwareAdapter::ReadDoubleUnsafe(const std::string& Path) const
	{
		ZILabOneHardwareAdapterSyms::ZIDoubleData Data;
		auto Result = ZILabOneHardwareAdapterSyms::ziAPIGetValueD(ZIConnection, ("/" + DeviceDescriptor + "/" + Path).c_str(), &Data);
		CheckError(Result);

		return static_cast<double>(Data);
	}

	long long ZILabOneHardwareAdapter::ReadIntUnsafe(const std::string& Path) const
	{
		ZILabOneHardwareAdapterSyms::ZIIntegerData Data;
		auto Result = ZILabOneHardwareAdapterSyms::ziAPIGetValueI(ZIConnection, ("/" + DeviceDescriptor + "/" + Path).c_str(), &Data);
		CheckError(Result);

		return static_cast<long long>(Data);
	}

	void ZILabOneHardwareAdapter::WriteDoubleUnsafe(const std::string& Path, double Value) const
	{
		auto Result = ZILabOneHardwareAdapterSyms::ziAPISetValueD(ZIConnection, ("/" + DeviceDescriptor + "/" + Path).c_str(), Value);
		CheckError(Result);
	}

	void ZILabOneHardwareAdapter::WriteIntUnsafe(const std::string& Path, long long Value) const
	{
		auto Result = ZILabOneHardwareAdapterSyms::ziAPISetValueI(ZIConnection, ("/" + DeviceDescriptor + "/" + Path).c_str(), Value);
		CheckError(Result);
	}
}