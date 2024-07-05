// This file is part of DynExp.

#include "stdafx.h"
#include "HardwareAdapterQutoolsTDC.h"

namespace DynExpHardware
{
	// Returns the serial numbers of all available qutool TDC devices as a std::vector
	// sorted by device number in ascending order.
	auto QutoolsTDCHardwareAdapter::Enumerate()
	{
		unsigned int DeviceCount{};
		auto Result = QutoolsTDCSyms::TDC_discover(&DeviceCount);
		if (Result != TDC_Ok)
			throw QutoolsTDCException("Error enumerating qutools TDC devices.", Result);

		std::vector<std::string> DeviceDescriptors;
		for (decltype(DeviceCount) i = 0; i < DeviceCount; ++i)
		{
			std::string DeviceName;
			DeviceName.resize(32);	// Must be at least 16 bytes (given by documentation of TDC_getDeviceInfo()).

			Result = QutoolsTDCSyms::TDC_getDeviceInfo(i, nullptr, nullptr, DeviceName.data(), nullptr);
			if (Result != TDC_Ok)
				throw QutoolsTDCException("Error obtaining TDC device information.", Result);
			DeviceName = Util::TrimTrailingZeros(DeviceName);

			DeviceDescriptors.emplace_back(std::move(DeviceName));
		}

		return DeviceDescriptors;
	}

	Util::TextValueListType<QutoolsTDCHardwareAdapterParams::EdgeType> QutoolsTDCHardwareAdapterParams::EdgeTypeStrList()
	{
		Util::TextValueListType<EdgeType> List = {
			{ "Trigger on rising edge.", EdgeType::RisingEdge },
			{ "Trigger on falling edge.", EdgeType::FallingEdge }
		};

		return List;
	}

	void QutoolsTDCHardwareAdapterParams::ConfigureParamsImpl(dispatch_tag<HardwareAdapterParamsBase>)
	{
		auto QutoolsTDCDevices = QutoolsTDCHardwareAdapter::Enumerate();
		if (!DeviceDescriptor.Get().empty() &&
			std::find(QutoolsTDCDevices.cbegin(), QutoolsTDCDevices.cend(), DeviceDescriptor) == std::cend(QutoolsTDCDevices))
			QutoolsTDCDevices.push_back(DeviceDescriptor);
		if (QutoolsTDCDevices.empty())
			throw Util::EmptyException("There is not any available qutools TDC device.");
		DeviceDescriptor.SetTextList(std::move(QutoolsTDCDevices));

		ConfigureParamsImpl(dispatch_tag<QutoolsTDCHardwareAdapterParams>());
	}

	QutoolsTDCHardwareAdapter::QutoolsTDCSynchronizer::LockType QutoolsTDCHardwareAdapter::QutoolsTDCSynchronizer::Lock(
		const std::chrono::milliseconds Timeout)
	{
		return GetInstance().AcquireLock(Timeout);
	}

	QutoolsTDCHardwareAdapter::QutoolsTDCSynchronizer& QutoolsTDCHardwareAdapter::QutoolsTDCSynchronizer::GetInstance() noexcept
	{
		static QutoolsTDCSynchronizer Instance;

		return Instance;
	}

	QutoolsTDCHardwareAdapter::CoincidenceDataType::CoincidenceDataType(CountsType&& Counts, QutoolsTDCSyms::Int32 NumUpdates)
		: Counts(std::move(Counts)), NumUpdates(NumUpdates)
	{
		HasBeenRead.resize(this->Counts.size(), false);
	}

	QutoolsTDCHardwareAdapter::QutoolsTDCHardwareAdapter(const std::thread::id OwnerThreadID, DynExp::ParamsBasePtrType&& Params)
		: HardwareAdapterBase(OwnerThreadID, std::move(Params))
	{
		Init();
	}

	QutoolsTDCHardwareAdapter::~QutoolsTDCHardwareAdapter()
	{
		// Not locking, since the object is being destroyed. This should be inherently thread-safe.
		CloseUnsafe();
	}

	void QutoolsTDCHardwareAdapter::EnableChannels(bool EnableStartChannel, QutoolsTDCSyms::Int32 ChannelMask) const
	{
		auto TDCLock = QutoolsTDCSynchronizer::Lock();
		AddressThisTDCDeviceUnsafe();

		EnableChannelsUnsafe(EnableStartChannel, ChannelMask);
	}

	void QutoolsTDCHardwareAdapter::EnableChannel(ChannelType Channel) const
	{
		auto TDCLock = QutoolsTDCSynchronizer::Lock();
		AddressThisTDCDeviceUnsafe();

		if (Channel >= ChannelCount)
			ThrowExceptionUnsafe(std::make_exception_ptr(Util::OutOfRangeException(
				"Specify a channel between 0 and " + Util::ToStr(ChannelCount))));

		auto CurrentState = GetEnabledChannelsUnsafe();
		EnableChannelsUnsafe(CurrentState.first, CurrentState.second | (1 << Channel));
	}

	void QutoolsTDCHardwareAdapter::DisableChannel(ChannelType Channel) const
	{
		auto TDCLock = QutoolsTDCSynchronizer::Lock();
		AddressThisTDCDeviceUnsafe();

		if (Channel >= ChannelCount)
			ThrowExceptionUnsafe(std::make_exception_ptr(Util::OutOfRangeException(
				"Specify a channel between 0 and " + Util::ToStr(ChannelCount))));

		auto CurrentState = GetEnabledChannelsUnsafe();
		EnableChannelsUnsafe(CurrentState.first, CurrentState.second & ~(1 << Channel));
	}

	void QutoolsTDCHardwareAdapter::SetExposureTime(std::chrono::milliseconds ExposureTime) const
	{
		auto TDCLock = QutoolsTDCSynchronizer::Lock();
		AddressThisTDCDeviceUnsafe();

		SetExposureTimeUnsafe(ExposureTime);
	}

	void QutoolsTDCHardwareAdapter::SetCoincidenceWindow(ValueType CoincidenceWindow) const
	{
		auto TDCLock = QutoolsTDCSynchronizer::Lock();
		AddressThisTDCDeviceUnsafe();

		SetCoincidenceWindowUnsafe(CoincidenceWindow);
	}

	void QutoolsTDCHardwareAdapter::SetChannelDelay(ChannelType Channel, Util::picoseconds ChannelDelay) const
	{
		auto TDCLock = QutoolsTDCSynchronizer::Lock();
		AddressThisTDCDeviceUnsafe();

		SetChannelDelayUnsafe(Channel, ChannelDelay);
	}

	void QutoolsTDCHardwareAdapter::SetTimestampBufferSize(QutoolsTDCSyms::Int32 Size) const
	{
		auto TDCLock = QutoolsTDCSynchronizer::Lock();
		AddressThisTDCDeviceUnsafe();

		SetTimestampBufferSizeUnsafe(Size);
	}

	void QutoolsTDCHardwareAdapter::ConfigureSignalConditioning(ChannelType Channel,
		QutoolsTDCSyms::TDC_SignalCond Conditioning, bool UseRisingEdge, double ThresholdInVolts) const
	{
		auto TDCLock = QutoolsTDCSynchronizer::Lock();
		AddressThisTDCDeviceUnsafe();

		ConfigureSignalConditioningUnsafe(Channel, Conditioning, UseRisingEdge, ThresholdInVolts);
	}

	void QutoolsTDCHardwareAdapter::ConfigureFilter(ChannelType Channel, QutoolsTDCSyms::TDC_FilterType FilterType,
		QutoolsTDCSyms::Int32 ChannelMask) const
	{
		auto TDCLock = QutoolsTDCSynchronizer::Lock();
		AddressThisTDCDeviceUnsafe();

		ConfigureFilterUnsafe(Channel, FilterType, ChannelMask);
	}

	void QutoolsTDCHardwareAdapter::EnableHBT(bool Enable) const
	{
		auto TDCLock = QutoolsTDCSynchronizer::Lock();
		AddressThisTDCDeviceUnsafe();

		EnableHBTUnsafe(Enable);
	}

	void QutoolsTDCHardwareAdapter::ConfigureHBTChannels(ChannelType FirstChannel, ChannelType SecondChannel) const
	{
		if (FirstChannel >= 31 || SecondChannel >= 31)
			ThrowExceptionUnsafe(std::make_exception_ptr(Util::OutOfRangeException(
				"Specify valid channels between 0 and 31.")));

		auto TDCLock = QutoolsTDCSynchronizer::Lock();
		AddressThisTDCDeviceUnsafe();

		ConfigureHBTChannelsUnsafe(FirstChannel, SecondChannel);
	}

	void QutoolsTDCHardwareAdapter::ConfigureHBTParams(Util::picoseconds BinWidth, QutoolsTDCSyms::Int32 BinCount) const
	{
		auto TDCLock = QutoolsTDCSynchronizer::Lock();
		AddressThisTDCDeviceUnsafe();

		ConfigureHBTParamsUnsafe(BinWidth, BinCount);
	}

	void QutoolsTDCHardwareAdapter::ResetHBT() const
	{
		auto TDCLock = QutoolsTDCSynchronizer::Lock();
		AddressThisTDCDeviceUnsafe();

		ResetHBTUnsafe();
	}

	QutoolsTDCSyms::Int64 QutoolsTDCHardwareAdapter::GetHBTEventCounts() const
	{
		auto TDCLock = QutoolsTDCSynchronizer::Lock();
		AddressThisTDCDeviceUnsafe();

		return GetHBTEventCountsUnsafe();
	}

	std::chrono::microseconds QutoolsTDCHardwareAdapter::GetHBTIntegrationTime() const
	{
		auto TDCLock = QutoolsTDCSynchronizer::Lock();
		AddressThisTDCDeviceUnsafe();

		return GetHBTIntegrationTimeUnsafe();
	}

	QutoolsTDCHardwareAdapter::HBTResultsType QutoolsTDCHardwareAdapter::GetHBTResult() const
	{
		auto TDCLock = QutoolsTDCSynchronizer::Lock();
		AddressThisTDCDeviceUnsafe();

		return GetHBTResultUnsafe();
	}

	QutoolsTDCHardwareAdapter::ValueType QutoolsTDCHardwareAdapter::GetTimebase() const
	{
		auto lock = AcquireLock(HardwareOperationTimeout);

		return Timebase;
	}

	QutoolsTDCSyms::Int32 QutoolsTDCHardwareAdapter::GetBufferSize() const
	{
		auto lock = AcquireLock(HardwareOperationTimeout);

		return BufferSize;
	}

	// Extracts timestamp series for specific channel.
	std::vector<QutoolsTDCHardwareAdapter::ValueType> QutoolsTDCHardwareAdapter::GetTimestamps(ChannelType Channel) const
	{
		auto lock = AcquireLock(HardwareOperationTimeout);

		ReadTimestampsUnsafe();

		auto Timestamps = TimestampsPerChannel.extract(Channel);
		return !Timestamps.empty() ? std::move(Timestamps.mapped()) : std::vector<QutoolsTDCHardwareAdapter::ValueType>();
	}

	// Just sums up timestamp series for specific channel while keeping the series.
	size_t QutoolsTDCHardwareAdapter::GetCountsFromTimestamps(ChannelType Channel) const
	{
		auto lock = AcquireLock(HardwareOperationTimeout);

		ReadTimestampsUnsafe();

		return TimestampsPerChannel[Channel].size();
	}

	const QutoolsTDCHardwareAdapter::CoincidenceDataType& QutoolsTDCHardwareAdapter::GetCoincidenceCounts() const
	{
		auto TDCLock = QutoolsTDCSynchronizer::Lock();
		AddressThisTDCDeviceUnsafe();

		// See documentation for TDC_getCoincCounters()
		std::vector<QutoolsTDCSyms::Int32> Counts(59);
		QutoolsTDCSyms::Int32 NumUpdates{};

		auto Result = QutoolsTDCSyms::TDC_getCoincCounters(Counts.data(), &NumUpdates);
		CheckError(Result);

		if (NumUpdates)
			CoincidenceData = { std::move(Counts), NumUpdates };

		return CoincidenceData;
	}

	// First of pair denotes the coincidences of the specified channel (combination), second of pair denotes the number
	// of updates the qutools TDC device has made since the last call to TDC_getCoincCounters().
	std::pair<QutoolsTDCSyms::Int32, QutoolsTDCSyms::Int32> QutoolsTDCHardwareAdapter::GetCoincidenceCounts(ChannelType Channel) const
	{
		// See documentation for TDC_getCoincCounters()
		if (Channel < 0 || Channel >= 59)
			ThrowExceptionUnsafe(std::make_exception_ptr(Util::OutOfRangeException(
				"Specify a channel between 0 and 59.")));

		GetCoincidenceCounts();

		// Just after CoincidenceData has been default-constructed, CoincidenceData.Counts is empty.
		if (CoincidenceData.Counts.empty() || CoincidenceData.Counts.size() != CoincidenceData.HasBeenRead.size() || CoincidenceData.Counts.size() <= Channel)
			return { 0, 0 };

		auto RetVal = std::make_pair(CoincidenceData.Counts[Channel], CoincidenceData.HasBeenRead[Channel] ? 0 : CoincidenceData.NumUpdates);
		CoincidenceData.HasBeenRead[Channel] = true;

		return RetVal;
	}

	void QutoolsTDCHardwareAdapter::ClearTimestamps(ChannelType Channel) const
	{
		auto lock = AcquireLock(HardwareOperationTimeout);

		TimestampsPerChannel[Channel].clear();
	}

	void QutoolsTDCHardwareAdapter::Init()
	{
		DeviceConnected = false;
		DeviceNumber = std::numeric_limits<decltype(DeviceNumber)>::max();
		Timebase = ValueType(-1);
		BufferSize = 0;
		ChannelCount = 0;
		TimestampsPerChannel.clear();
		CoincidenceData = {};
	}

	void QutoolsTDCHardwareAdapter::ResetImpl(dispatch_tag<HardwareAdapterBase>)
	{
		// auto lock = AcquireLock(); not necessary here, since DynExp ensures that Object::Reset() can only
		// be called if respective object is not in use.

		CloseUnsafe();
		Init();

		ResetImpl(dispatch_tag<QutoolsTDCHardwareAdapter>());
	}

	void QutoolsTDCHardwareAdapter::EnsureReadyStateChild()
	{
		auto lock = AcquireLock(HardwareOperationTimeout);

		OpenUnsafe();
	}

	bool QutoolsTDCHardwareAdapter::IsReadyChild() const
	{
		auto lock = AcquireLock(HardwareOperationTimeout);

		auto Exception = GetExceptionUnsafe();
		Util::ForwardException(Exception);

		return IsOpened();
	}

	bool QutoolsTDCHardwareAdapter::IsConnectedChild() const noexcept
	{
		return IsOpened();
	}

	void QutoolsTDCHardwareAdapter::CheckError(const int Result, const std::source_location Location) const
	{
		if (Result == TDC_Ok)
			return;

		std::string ErrorString(QutoolsTDCSyms::TDC_perror(Result));

		// AcquireLock() has already been called by an (in)direct caller of this function.
		ThrowExceptionUnsafe(std::make_exception_ptr(QutoolsTDCException(std::move(ErrorString), Result, Location)));
	}

	void QutoolsTDCHardwareAdapter::OpenUnsafe()
	{
		if (IsOpened())
			return;

		std::ptrdiff_t DeviceNumber{ -1 };
		QutoolsTDCSyms::Int32 TimestampBufferSize{};
		bool UseRisingEdge{};
		double ThresholdInVolts{};
		std::chrono::milliseconds ExposureTime{};
		ValueType CoincidenceWindow{};

		{
			auto DerivedParams = dynamic_Params_cast<QutoolsTDCHardwareAdapter>(GetParams());

			auto DeviceSerials = Enumerate();
			auto DeviceSerialIt = std::find(DeviceSerials.cbegin(), DeviceSerials.cend(), DerivedParams->DeviceDescriptor.Get());
			DeviceNumber = std::distance(DeviceSerials.cbegin(), DeviceSerialIt);

			if (DeviceNumber < 0 || Util::NumToT<size_t>(DeviceNumber) >= DeviceSerials.size())
			{
				std::string Serial = DeviceSerialIt == DeviceSerials.cend() ? "< unknown >" : *DeviceSerialIt;

				ThrowExceptionUnsafe(std::make_exception_ptr(Util::NotFoundException(
					"The qutools TDC device with serial " + Serial + " has not been found.")));
			}

			TimestampBufferSize = DerivedParams->DefaultTimestampBufferSize;
			UseRisingEdge = DerivedParams->DefaultTriggerEdge == QutoolsTDCHardwareAdapterParams::EdgeType::RisingEdge;
			ThresholdInVolts = DerivedParams->DefaultThresholdInVolts;
			ExposureTime = std::chrono::milliseconds(DerivedParams->DefaultExposureTime);
			CoincidenceWindow = ValueType(DerivedParams->DefaultCoincidenceWindow);
		} // DerivedParams unlocked here.

		this->DeviceNumber = DeviceNumber;
		auto Result = QutoolsTDCSyms::TDC_connect(this->DeviceNumber);
		CheckError(Result);
		DeviceConnected = true;

		auto TDCLock = QutoolsTDCSynchronizer::Lock();
		AddressThisTDCDeviceUnsafe();

		double Timebase{};	// in s
		Result = QutoolsTDCSyms::TDC_getTimebase(&Timebase);
		CheckError(Result);
		this->Timebase = ValueType(Timebase * 1e12);

		QutoolsTDCSyms::Int32 BufferSize{};
		Result = QutoolsTDCSyms::TDC_getTimestampBufferSize(&BufferSize);
		CheckError(Result);
		this->BufferSize = BufferSize;

		ChannelCount = QutoolsTDCSyms::TDC_getChannelCount();

		SetTimestampBufferSizeUnsafe(TimestampBufferSize);
		SetExposureTimeUnsafe(ExposureTime);
		SetCoincidenceWindowUnsafe(CoincidenceWindow);
		for (decltype(ChannelCount) i = 0; i < ChannelCount; ++i)
			ConfigureSignalConditioningUnsafe(i, QutoolsTDCSyms::SCOND_MISC, UseRisingEdge, ThresholdInVolts);
	}

	void QutoolsTDCHardwareAdapter::CloseUnsafe()
	{
		if (IsOpened())
		{
			// Handle now considered invalid, even if TDC_disconnect() fails.
			DeviceConnected = false;

			auto Result = QutoolsTDCSyms::TDC_disconnect(DeviceNumber);
			CheckError(Result);
		}
	}

	void QutoolsTDCHardwareAdapter::AddressThisTDCDeviceUnsafe() const
	{
		auto Result = QutoolsTDCSyms::TDC_addressDevice(DeviceNumber);
		CheckError(Result);
	}

	void QutoolsTDCHardwareAdapter::ReadTimestampsUnsafe() const
	{
		std::vector<QutoolsTDCSyms::Int64> Timestamps(BufferSize);
		std::vector<ChannelType> Channels(BufferSize);
		QutoolsTDCSyms::Int32 NumValid{};

		{
			auto TDCLock = QutoolsTDCSynchronizer::Lock();
			AddressThisTDCDeviceUnsafe();

			auto Result = QutoolsTDCSyms::TDC_getLastTimestamps(true, Timestamps.data(), Channels.data(), &NumValid);
			CheckError(Result);
		} // TDCLock unlocked here.

		if (Timestamps.size() != Channels.size() || NumValid < 0 || NumValid > Timestamps.size())
			ThrowExceptionUnsafe(std::make_exception_ptr(Util::InvalidDataException(
				"Received invalid data from TDC_getLastTimestamps().")));

		for (size_t i = 0; i < NumValid; ++i)
			TimestampsPerChannel[Channels[i]].push_back(ValueType(Timestamps[i]));
	}

	// QutoolsTDCSynchronizer::Lock() and AddressThisTDCDeviceUnsafe() must be called manually before calling this function!
	void QutoolsTDCHardwareAdapter::EnableChannelsUnsafe(bool EnableStartChannel, QutoolsTDCSyms::Int32 ChannelMask) const
	{
		auto Result = QutoolsTDCSyms::TDC_enableChannels(EnableStartChannel, ChannelMask);
		CheckError(Result);
	}

	// QutoolsTDCSynchronizer::Lock() and AddressThisTDCDeviceUnsafe() must be called manually before calling this function!
	// First of pair denotes whether the start channel is enabled, second of pair denotes the mask of enabled channels.
	std::pair<bool, QutoolsTDCSyms::Int32> QutoolsTDCHardwareAdapter::GetEnabledChannelsUnsafe() const
	{
		QutoolsTDCSyms::Bln32 StartEnabled{};
		QutoolsTDCSyms::Int32 ChannelMask{};

		auto Result = QutoolsTDCSyms::TDC_getChannelsEnabled(&StartEnabled, &ChannelMask);
		CheckError(Result);

		return std::make_pair(static_cast<bool>(StartEnabled), ChannelMask);
	}

	void QutoolsTDCHardwareAdapter::SetExposureTimeUnsafe(std::chrono::milliseconds ExposureTime) const
	{
		auto Result = QutoolsTDCSyms::TDC_setExposureTime(Util::NumToT<QutoolsTDCSyms::Int32>(ExposureTime.count()));
		CheckError(Result);
	}

	void QutoolsTDCHardwareAdapter::SetCoincidenceWindowUnsafe(ValueType CoincidenceWindow) const
	{
		auto Result = QutoolsTDCSyms::TDC_setCoincidenceWindow(Util::NumToT<QutoolsTDCSyms::Int32>(CoincidenceWindow / Timebase));
		CheckError(Result);
	}

	void QutoolsTDCHardwareAdapter::SetChannelDelayUnsafe(ChannelType Channel, Util::picoseconds ChannelDelay) const
	{
		auto Result = QutoolsTDCSyms::TDC_setChannelDelay(Channel, Util::NumToT<QutoolsTDCSyms::Int32>(ChannelDelay.count()));
		CheckError(Result);
	}

	void QutoolsTDCHardwareAdapter::SetTimestampBufferSizeUnsafe(QutoolsTDCSyms::Int32 Size) const
	{
		auto Result = QutoolsTDCSyms::TDC_setTimestampBufferSize(Size);
		CheckError(Result);

		BufferSize = Size;
	}

	void QutoolsTDCHardwareAdapter::ConfigureSignalConditioningUnsafe(ChannelType Channel,
		QutoolsTDCSyms::TDC_SignalCond Conditioning,bool UseRisingEdge, double ThresholdInVolts) const
	{
		auto Result = QutoolsTDCSyms::TDC_configureSignalConditioning(Channel, Conditioning, UseRisingEdge, ThresholdInVolts);
		CheckError(Result);
	}

	void QutoolsTDCHardwareAdapter::ConfigureFilterUnsafe(ChannelType Channel,
		QutoolsTDCSyms::TDC_FilterType FilterType, QutoolsTDCSyms::Int32 ChannelMask) const
	{
		auto Result = QutoolsTDCSyms::TDC_configureFilter(Channel + 1, FilterType, ChannelMask);
		CheckError(Result);
	}

	void QutoolsTDCHardwareAdapter::EnableHBTUnsafe(bool Enable) const
	{
		auto Result = QutoolsTDCSyms::TDC_enableHbt(Enable);
		CheckError(Result);
	}

	void QutoolsTDCHardwareAdapter::ConfigureHBTChannelsUnsafe(ChannelType FirstChannel, ChannelType SecondChannel) const
	{
		auto Result = QutoolsTDCSyms::TDC_setHbtInput(FirstChannel + 1, SecondChannel + 1);
		CheckError(Result);
	}

	void QutoolsTDCHardwareAdapter::ConfigureHBTParamsUnsafe(Util::picoseconds BinWidth, QutoolsTDCSyms::Int32 BinCount) const
	{
		auto Result = QutoolsTDCSyms::TDC_setHbtParams(Util::NumToT<QutoolsTDCSyms::Int32>(BinWidth / Timebase), BinCount);
		CheckError(Result);
	}

	void QutoolsTDCHardwareAdapter::ResetHBTUnsafe() const
	{
		auto Result = QutoolsTDCSyms::TDC_resetHbtCorrelations();
		CheckError(Result);
	}

	QutoolsTDCSyms::Int64 QutoolsTDCHardwareAdapter::GetHBTEventCountsUnsafe() const
	{
		QutoolsTDCSyms::Int64 TotalCount{}, LastCount{};
		double LastRate{};

		auto Result = QutoolsTDCSyms::TDC_getHbtEventCount(&TotalCount, &LastCount, &LastRate);
		CheckError(Result);

		return TotalCount;
	}

	std::chrono::microseconds QutoolsTDCHardwareAdapter::GetHBTIntegrationTimeUnsafe() const
	{
		double IntegrationTime{};

		auto Result = QutoolsTDCSyms::TDC_getHbtIntegrationTime(&IntegrationTime);
		CheckError(Result);

		return std::chrono::microseconds(static_cast<std::chrono::microseconds::rep>(std::round(IntegrationTime * std::micro::den)));
	}

	QutoolsTDCHardwareAdapter::HBTResultsType QutoolsTDCHardwareAdapter::GetHBTResultUnsafe() const
	{
		auto HBTFunc = QutoolsTDCSyms::TDC_createHbtFunction();
		if (!HBTFunc)
			CheckError(TDC_Error);

		try
		{
			auto Result = QutoolsTDCSyms::TDC_calcHbtG2(HBTFunc);
			CheckError(Result);

			HBTResultsType HBTResult;
			for (decltype(HBTFunc->size) i = 0; i < HBTFunc->size; ++i)
				HBTResult.emplace_back(HBTFunc->values[i], ((i - HBTFunc->indexOffset) * Timebase * HBTFunc->binWidth).count() / std::pico::den);

			QutoolsTDCSyms::TDC_releaseHbtFunction(HBTFunc);
			HBTFunc = nullptr;

			return HBTResult;
		}
		catch (...)
		{
			if (HBTFunc)
				QutoolsTDCSyms::TDC_releaseHbtFunction(HBTFunc);

			throw;
		}
	}
}