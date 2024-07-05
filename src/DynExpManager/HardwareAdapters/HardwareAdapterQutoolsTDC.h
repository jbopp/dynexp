// This file is part of DynExp.

/**
 * @file HardwareAdapterQutoolsTDC.h
 * @brief Implementation of a hardware adapter to control qutools TDC
 * hardware.
*/

#pragma once

#include "stdafx.h"
#include "HardwareAdapter.h"
#include "../MetaInstruments/TimeTagger.h"

namespace DynExpHardware::QutoolsTDCSyms
{
	#include "../include/qutools/tdcbase.h"
	#include "../include/qutools/tdcmultidev.h"
	#include "../include/qutools/tdchbt.h"
}

namespace DynExpHardware
{
	class QutoolsTDCHardwareAdapter;

	class QutoolsTDCException : public Util::Exception
	{
	public:
		QutoolsTDCException(std::string Description, const int ErrorCode,
			const std::source_location Location = std::source_location::current()) noexcept
			: Exception(std::move(Description), Util::ErrorType::Error, ErrorCode, Location)
		{}
	};

	class QutoolsTDCHardwareAdapterParams : public DynExp::HardwareAdapterParamsBase
	{
	public:
		enum EdgeType { RisingEdge, FallingEdge };

		static Util::TextValueListType<EdgeType> EdgeTypeStrList();

		QutoolsTDCHardwareAdapterParams(DynExp::ItemIDType ID, const DynExp::DynExpCore& Core) : HardwareAdapterParamsBase(ID, Core) {}
		virtual ~QutoolsTDCHardwareAdapterParams() = default;

		virtual const char* GetParamClassTag() const noexcept override { return "QutoolsTDCHardwareAdapterParams"; }

		Param<TextList> DeviceDescriptor = { *this, {}, "DeviceDescriptor", "Device serial",
			"Serial of the qutools TDC device to connect with" };
		Param<ParamsConfigDialog::NumberType> DefaultTimestampBufferSize = { *this, "DefaultTimestampBufferSize", "Default timestamp buffer size",
			"Size of the qutools TDC device's timestamp buffer in samples (default value - can be changed by instruments).",
			false, 1000, 1, 1000000, 1, 0 };
		Param<ParamsConfigDialog::NumberType> DefaultThresholdInVolts = { *this, "DefaultThresholdInVolts", "Default trigger threshold (V)",
			"Determines on which voltage of the connected signal to trigger (default value - can be changed by instruments).",
			false, 1, -2, 3, .1, 3 };
		Param<EdgeType> DefaultTriggerEdge = { *this, EdgeTypeStrList(), "DefaultTriggerEdge", "Default trigger edge",
			"Determines on which edges of the connected signal to trigger (default value - can be changed by instruments).",
			false, EdgeType::RisingEdge};
		Param<ParamsConfigDialog::NumberType> DefaultExposureTime = { *this, "DefaultExposureTimeInMs", "Default exposure time (ms)",
			"Exposure time denoting an interval in which events are accumulated (default value - can be changed by instruments).",
			false, 1000, 1, 65535, 100, 0 };
		Param<ParamsConfigDialog::NumberType> DefaultCoincidenceWindow = { *this, "DefaultCoincidenceWindowInPs", "Default coincidence window (ps)",
			"Time interval in which events are considered to be coincident (default value - can be changed by instruments).",
			false, 1000, 1, std::numeric_limits<ParamsConfigDialog::NumberType>::max(), 100, 0 };

	private:
		void ConfigureParamsImpl(dispatch_tag<HardwareAdapterParamsBase>) override final;
		virtual void ConfigureParamsImpl(dispatch_tag<QutoolsTDCHardwareAdapterParams>) {}
	};

	class QutoolsTDCHardwareAdapterConfigurator : public DynExp::HardwareAdapterConfiguratorBase
	{
	public:
		using ObjectType = QutoolsTDCHardwareAdapter;
		using ParamsType = QutoolsTDCHardwareAdapterParams;

		QutoolsTDCHardwareAdapterConfigurator() = default;
		virtual ~QutoolsTDCHardwareAdapterConfigurator() = default;

	private:
		virtual DynExp::ParamsBasePtrType MakeParams(DynExp::ItemIDType ID, const DynExp::DynExpCore& Core) const override { return DynExp::MakeParams<QutoolsTDCHardwareAdapterConfigurator>(ID, Core); }
	};

	class QutoolsTDCHardwareAdapter : public DynExp::HardwareAdapterBase
	{
		/**
		 * @brief Only one instance of this class is allowed for synchronizing calls to
		 * connected qutools time taggers. This is required since @p TDC_addressDevice()
		 * selects a single device globally.
		 * @warning Always lock the mutex of @p QutoolsTDCHardwareAdapter (by a call to
		 * Util::ILockable::AcquireLock()) before the @p QutoolsTDCSynchronizer (by a call
		 * to QutoolsTDCSynchronizer::Lock()).
		*/
		class QutoolsTDCSynchronizer : public Util::ILockable
		{
		private:
			QutoolsTDCSynchronizer() = default;
			~QutoolsTDCSynchronizer() = default;

		public:
			// Synchronizes every call to qutools TDC library from anywhere.
			static [[nodiscard]] LockType Lock(const std::chrono::milliseconds Timeout = std::chrono::milliseconds(100));

		private:
			static QutoolsTDCSynchronizer& GetInstance() noexcept;
		};

	public:
		struct CoincidenceDataType
		{
			using CountsType = std::vector<QutoolsTDCSyms::Int32>;

			CoincidenceDataType() : NumUpdates(0) {}
			CoincidenceDataType(CountsType&& Counts, QutoolsTDCSyms::Int32 NumUpdates);

			CountsType Counts;
			QutoolsTDCSyms::Int32 NumUpdates;

			// Just to store boolean flags. But std::vector<bool> is evil...
			std::vector<char> HasBeenRead;
		};

		using ParamsType = QutoolsTDCHardwareAdapterParams;
		using ConfigType = QutoolsTDCHardwareAdapterConfigurator;
		using ChannelType = QutoolsTDCSyms::Uint8;
		using ValueType = Util::picoseconds;
		using HBTResultsType = DynExpInstr::TimeTaggerData::HBTResultsType::ResultVectorType;

		constexpr static auto Name() noexcept { return "qutools TDC"; }
		constexpr static auto Category() noexcept { return "I/O"; }
		static auto Enumerate();

		QutoolsTDCHardwareAdapter(const std::thread::id OwnerThreadID, DynExp::ParamsBasePtrType&& Params);
		virtual ~QutoolsTDCHardwareAdapter();

		virtual std::string GetName() const override { return Name(); }
		virtual std::string GetCategory() const override { return Category(); }

		bool IsOpened() const noexcept { return DeviceConnected; }

		void EnableChannels(bool EnableStartChannel, QutoolsTDCSyms::Int32 ChannelMask) const;
		void EnableChannel(ChannelType Channel) const;
		void DisableChannel(ChannelType Channel) const;
		void SetExposureTime(std::chrono::milliseconds ExposureTime) const;
		void SetCoincidenceWindow(ValueType CoincidenceWindow) const;
		void SetChannelDelay(ChannelType Channel, Util::picoseconds ChannelDelay) const;
		void SetTimestampBufferSize(QutoolsTDCSyms::Int32 Size) const;
		void ConfigureSignalConditioning(ChannelType Channel,
			QutoolsTDCSyms::TDC_SignalCond Conditioning = QutoolsTDCSyms::SCOND_MISC, bool UseRisingEdge = true, double ThresholdInVolts = 1) const;
		void ConfigureFilter(ChannelType Channel,
			QutoolsTDCSyms::TDC_FilterType FilterType = QutoolsTDCSyms::TDC_FilterType::FILTER_NONE, QutoolsTDCSyms::Int32 ChannelMask = 0) const;
		void EnableHBT(bool Enable) const;
		void ConfigureHBTChannels(ChannelType FirstChannel, ChannelType SecondChannel) const;
		void ConfigureHBTParams(Util::picoseconds BinWidth, QutoolsTDCSyms::Int32 BinCount) const;
		void ResetHBT() const;
		QutoolsTDCSyms::Int64 GetHBTEventCounts() const;
		std::chrono::microseconds GetHBTIntegrationTime() const;
		HBTResultsType GetHBTResult() const;

		ValueType GetTimebase() const;
		QutoolsTDCSyms::Int32 GetBufferSize() const;
		std::vector<ValueType> GetTimestamps(ChannelType Channel) const;
		size_t GetCountsFromTimestamps(ChannelType Channel) const;
		const CoincidenceDataType& GetCoincidenceCounts() const;
		std::pair<QutoolsTDCSyms::Int32, QutoolsTDCSyms::Int32> GetCoincidenceCounts(ChannelType Channel) const;

		void ClearTimestamps(ChannelType Channel) const;

	private:
		void Init();

		void ResetImpl(dispatch_tag<HardwareAdapterBase>) override final;
		virtual void ResetImpl(dispatch_tag<QutoolsTDCHardwareAdapter>) {}

		void EnsureReadyStateChild() override final;
		bool IsReadyChild() const override final;
		bool IsConnectedChild() const noexcept override final;

		// Not thread-safe, must be called from function calling AcquireLock().
		void CheckError(const int Result, const std::source_location Location = std::source_location::current()) const;

		void OpenUnsafe();
		void CloseUnsafe();

		void AddressThisTDCDeviceUnsafe() const;
		void ReadTimestampsUnsafe() const;
		void EnableChannelsUnsafe(bool EnableStartChannel, QutoolsTDCSyms::Int32 ChannelMask) const;
		std::pair<bool, QutoolsTDCSyms::Int32> GetEnabledChannelsUnsafe() const;
		void SetExposureTimeUnsafe(std::chrono::milliseconds ExposureTime) const;
		void SetCoincidenceWindowUnsafe(ValueType CoincidenceWindow) const;
		void SetChannelDelayUnsafe(ChannelType Channel, Util::picoseconds ChannelDelay) const;
		void SetTimestampBufferSizeUnsafe(QutoolsTDCSyms::Int32 Size) const;
		void ConfigureSignalConditioningUnsafe(ChannelType Channel,
			QutoolsTDCSyms::TDC_SignalCond Conditioning = QutoolsTDCSyms::SCOND_MISC, bool UseRisingEdge = true, double ThresholdInVolts = 1) const;
		void ConfigureFilterUnsafe(ChannelType Channel,
			QutoolsTDCSyms::TDC_FilterType FilterType = QutoolsTDCSyms::TDC_FilterType::FILTER_NONE, QutoolsTDCSyms::Int32 ChannelMask = 0) const;
		void EnableHBTUnsafe(bool Enable) const;
		void ConfigureHBTChannelsUnsafe(ChannelType FirstChannel, ChannelType SecondChannel) const;
		void ConfigureHBTParamsUnsafe(Util::picoseconds BinWidth, QutoolsTDCSyms::Int32 BinCount) const;
		void ResetHBTUnsafe() const;
		QutoolsTDCSyms::Int64 GetHBTEventCountsUnsafe() const;
		std::chrono::microseconds GetHBTIntegrationTimeUnsafe() const;
		HBTResultsType GetHBTResultUnsafe() const;

		std::atomic<bool> DeviceConnected;
		unsigned int DeviceNumber;
		ValueType Timebase;
		mutable QutoolsTDCSyms::Int32 BufferSize;	// Can be changed by SetTimestampBufferSize().
		QutoolsTDCSyms::Int32 ChannelCount;

		mutable CoincidenceDataType CoincidenceData;
		mutable std::unordered_map<ChannelType, std::vector<ValueType>> TimestampsPerChannel;
	};
}