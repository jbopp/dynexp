// This file is part of DynExp.

/**
 * @file HardwareAdapterNIDAQ.h
 * @brief Implementation of a hardware adapter to control National Instruments NIDAQmx
 * hardware.
*/

#pragma once

#include "stdafx.h"
#include "HardwareAdapter.h"
#include "../MetaInstruments/DataStreamInstrument.h"

namespace DynExpHardware::NIDAQSyms
{
	#include "../include/NIDAQ/NIDAQmx.h"
}

namespace DynExpHardware
{
	class NIDAQHardwareAdapter;

	class NIDAQException : public Util::Exception
	{
	public:
		NIDAQException(std::string Description, const int ErrorCode, Util::ErrorType Type = Util::ErrorType::Error,
			const std::source_location Location = std::source_location::current()) noexcept
			: Exception(std::move(Description), Type, ErrorCode, Location)
		{}
	};

	class NIDAQOutputPortParamsExtension
	{
	public:
		enum UseOnlyOnBrdMemType { UseMemoryBuffer, OnlyOnboardMemory };

	private:
		static constexpr UseOnlyOnBrdMemType DefaultUseOnlyOnBrdMem = UseOnlyOnBrdMemType::UseMemoryBuffer;

	public:
		static Util::TextValueListType<UseOnlyOnBrdMemType> UseOnlyOnBrdMemTypeStrList();

		NIDAQOutputPortParamsExtension(DynExp::ParamsBase& Owner)
			: UseOnlyOnBrdMem{ Owner, UseOnlyOnBrdMemTypeStrList(), "UseOnlyOnBrdMem", "Onboard memory usage",
				"Determines whether samples are written using a buffered transfer or a direct transfer to the device's onboard memory.", true, DefaultUseOnlyOnBrdMem }
		{}

		void DisableUserEditable();

		/**
		 * @brief Write directly to device's onboard memory?
		*/
		DynExp::ParamsBase::Param<UseOnlyOnBrdMemType> UseOnlyOnBrdMem;
	};

	class NIDAQHardwareAdapterParams : public DynExp::HardwareAdapterParamsBase
	{
	public:
		enum ChannelModeType { TaskPerChannel, CombineChannels };
		enum TriggerModeType { Disabled, RisingEdge, FallingEdge };

		static Util::TextValueListType<ChannelModeType> ChannelModeTypeStrList();
		static Util::TextValueListType<TriggerModeType> TriggerModeTypeStrList();

		NIDAQHardwareAdapterParams(DynExp::ItemIDType ID, const DynExp::DynExpCore& Core)
			: HardwareAdapterParamsBase(ID, Core), StreamSizeParams(*this), NumericSampleStreamParams(*this) {}
		virtual ~NIDAQHardwareAdapterParams() = default;

		virtual const char* GetParamClassTag() const noexcept override { return "NIDAQHardwareAdapterParams"; }

		DynExp::ParamsBase::Param<ChannelModeType> ChannelMode = { *this, ChannelModeTypeStrList(), "ChannelMode", "Channel mode",
			"If timing is configured for multiple channels (stream size > 1 sample), they have to be combined into a number of NI-DAQmx tasks limited by the amount of the respective device's clocks.",
			true, ChannelModeType::TaskPerChannel };

		DynExpInstr::StreamSizeParamsExtension StreamSizeParams;
		DynExpInstr::NumericSampleStreamParamsExtension NumericSampleStreamParams;

		DynExp::ParamsBase::Param<TriggerModeType> TriggerMode = { *this, TriggerModeTypeStrList(), "TriggerMode", "Trigger mode",
			"Trigger assigned to this channel to start sample generation/acquisition", true, TriggerModeType::Disabled };
		DynExp::ParamsBase::Param<ParamsConfigDialog::TextType> TriggerChannel = { *this, "TriggerChannelName", "Trigger channel name",
			"Path of the trigger channel to be used for triggering (ignored if triggering is disabled)", true, "/Dev1/PFI0" };

	private:
		void ConfigureParamsImpl(dispatch_tag<HardwareAdapterParamsBase>) override final { ConfigureParamsImpl(dispatch_tag<NIDAQHardwareAdapterParams>()); }
		virtual void ConfigureParamsImpl(dispatch_tag<NIDAQHardwareAdapterParams>) {}
	};

	class NIDAQHardwareAdapterConfigurator : public DynExp::HardwareAdapterConfiguratorBase
	{
	public:
		using ObjectType = NIDAQHardwareAdapter;
		using ParamsType = NIDAQHardwareAdapterParams;

		NIDAQHardwareAdapterConfigurator() = default;
		virtual ~NIDAQHardwareAdapterConfigurator() = default;

	private:
		virtual DynExp::ParamsBasePtrType MakeParams(DynExp::ItemIDType ID, const DynExp::DynExpCore& Core) const override { return DynExp::MakeParams<NIDAQHardwareAdapterConfigurator>(ID, Core); }
	};

	class NIDAQTask
	{
		friend class NIDAQHardwareAdapter;

	public:
		using ChannelHandleType = size_t;
		using AnalogValueType = double;
		using DigitalValueType = uint8_t;

		enum class ChannelType { DigialIn, DigitalOut, AnalogIn, AnalogOut };
		enum class SamplingModeType { Single, Continuous };

		NIDAQTask(ChannelType Type, NIDAQSyms::TaskHandle NITask, NIDAQHardwareAdapterParams::ChannelModeType ChannelMode = NIDAQHardwareAdapterParams::ChannelModeType::TaskPerChannel);
		~NIDAQTask();

		auto GetType() const noexcept { return Type; }
		bool IsCombined() const noexcept { return ChannelMode == NIDAQHardwareAdapterParams::ChannelModeType::CombineChannels; }
		auto GetTimeout() const noexcept { return Timeout; }
		auto GetSamplingRate() const noexcept { return SamplingRate; }
		auto GetSamplingMode() const noexcept { return SamplingMode; }
		auto GetNumSamples() const noexcept { return NumSamples; }
		bool IsMultisample() const noexcept { return NumSamples > 1; }
		auto GetNumChannels() const noexcept { return NumChannels; }
		auto GetBufferSizeInSamples() const noexcept { return NumSamples * NumChannels; }
		auto GetSampleSizeInBytes() const noexcept { return (Type == ChannelType::DigialIn || Type == ChannelType::DigitalOut) ? sizeof(DigitalValueType) : sizeof(AnalogValueType); }
		auto GetChannelIndex(ChannelHandleType ChannelHandle) const { return ChannelIndexMap.at(ChannelHandle); }

	private:
		struct CircularStream
		{
			CircularStream(size_t BufferSize);

			void Clear();

			Util::circularbuf Buffer;
			std::iostream Stream;
		};

		void AddChannel(ChannelHandleType ChannelHandle, uint64_t NumSamples);

		const ChannelType Type;
		const NIDAQSyms::TaskHandle NITask;
		const NIDAQHardwareAdapterParams::ChannelModeType ChannelMode;

		double Timeout = 0;			// in seconds
		double SamplingRate = 0;	// in samples per second
		SamplingModeType SamplingMode = NIDAQTask::SamplingModeType::Single;
		uint64_t NumSamples = 1;
		uint32_t NumChannels = 0;

		std::map<ChannelHandleType, uint32_t> ChannelIndexMap;
		std::vector<DigitalValueType> DigitalValues;
		std::vector<AnalogValueType> AnalogValues;
		std::vector<std::unique_ptr<CircularStream>> ReadStreamPerChannel;
	};

	class NIDAQHardwareAdapter : public DynExp::HardwareAdapterBase
	{
	public:
		using ParamsType = NIDAQHardwareAdapterParams;
		using ConfigType = NIDAQHardwareAdapterConfigurator;
		using ChannelHandleType = NIDAQTask::ChannelHandleType;

		constexpr static auto Name() noexcept { return "NI-DAQmx"; }
		constexpr static auto Category() noexcept { return "I/O"; }
		static auto Enumerate();

		NIDAQHardwareAdapter(const std::thread::id OwnerThreadID, DynExp::ParamsBasePtrType&& Params);
		virtual ~NIDAQHardwareAdapter();

		virtual std::string GetName() const override { return Name(); }
		virtual std::string GetCategory() const override { return Category(); }

		// Timeout in seconds, MinValue and MaxValue in volts.
		ChannelHandleType InitializeDigitalInChannel(std::string_view ChannelName, double Timeout = 0) const;
		ChannelHandleType InitializeDigitalOutChannel(std::string_view ChannelName,
			NIDAQOutputPortParamsExtension::UseOnlyOnBrdMemType UseOnlyOnBrdMem, double Timeout = 0) const;
		ChannelHandleType InitializeAnalogInChannel(std::string_view ChannelName, double MinValue, double MaxValue, double Timeout = 0,
			int32_t TerminalConfig = DAQmx_Val_RSE) const;
		ChannelHandleType InitializeAnalogOutChannel(std::string_view ChannelName, double MinValue, double MaxValue,
			NIDAQOutputPortParamsExtension::UseOnlyOnBrdMemType UseOnlyOnBrdMem, double Timeout = 0) const;
		bool DeregisterChannel(ChannelHandleType ChannelHandle) const;

		std::vector<NIDAQTask::DigitalValueType> ReadDigitalValues(ChannelHandleType ChannelHandle) const;
		int32_t WriteDigitalValues(ChannelHandleType ChannelHandle, const std::vector<NIDAQTask::DigitalValueType>& Values) const;
		std::vector<NIDAQTask::AnalogValueType> ReadAnalogValues(ChannelHandleType ChannelHandle) const;
		int32_t WriteAnalogValues(ChannelHandleType ChannelHandle, const std::vector<NIDAQTask::AnalogValueType>& Values) const;

		void StartTask(ChannelHandleType ChannelHandle) const;
		void StopTask(ChannelHandleType ChannelHandle) const;
		void RestartTask(ChannelHandleType ChannelHandle) const;
		bool HasFinishedTask(ChannelHandleType ChannelHandle) const;
		const NIDAQTask* GetTask(ChannelHandleType ChannelHandle) const;

	private:
		using TasksMapType = std::unordered_map<ChannelHandleType, std::shared_ptr<NIDAQTask>>;

		void ResetImpl(dispatch_tag<HardwareAdapterBase>) override final;
		virtual void ResetImpl(dispatch_tag<NIDAQHardwareAdapter>) {}

		void EnsureReadyStateChild() override final;
		bool IsReadyChild() const override final;
		bool IsConnectedChild() const noexcept override final;

		// Not thread-safe, must be called from function calling AcquireLock().
		void CheckError(const int32_t Result, const std::source_location Location = std::source_location::current()) const;
		void CheckReadError(NIDAQTask* Task, const int32_t Result, const std::source_location Location = std::source_location::current()) const;

		NIDAQSyms::TaskHandle CreateTaskUnsafe() const;
		void InitializeTaskTimingUnsafe(NIDAQTask* Task,
			double Timeout, double SamplingRate, DynExpInstr::NumericSampleStreamParamsExtension::SamplingModeType SamplingMode) const;
		void InitializeTriggerUnsafe(NIDAQTask* Task, NIDAQHardwareAdapterParams::TriggerModeType TriggerMode, std::string_view TriggerChannelName) const;
		void StartTaskUnsafe(NIDAQTask* Task) const;
		void StopTaskUnsafe(NIDAQTask* Task) const;
		void RestartTaskUnsafe(NIDAQTask* Task) const;
		bool HasFinishedTaskUnsafe(NIDAQTask* Task) const;

		ChannelHandleType ChannelNameToChannelHandle(std::string_view ChannelName) const;
		bool TaskExistsUnsafe(ChannelHandleType ChannelHandle) const;
		bool TaskExistsUnsafe(std::string_view ChannelName) const;
		NIDAQTask* GetTaskUnsafe(ChannelHandleType ChannelHandle) const;
		NIDAQTask* GetTaskUnsafe(std::string_view ChannelName) const;
		ChannelHandleType InsertTaskUnsafe(std::string_view ChannelName, std::shared_ptr<NIDAQTask>&& TaskHandle) const;
		ChannelHandleType CreateTaskIfNotExistsUnsafe(std::string_view ChannelName, NIDAQTask::ChannelType Type) const;
		bool RemoveTaskUnsafe(ChannelHandleType ChannelHandle) const;

		// Map holding NIDAQ's tasks indexed by std::hash of NIDAQTask::ChannelName
		// Logical const-ness: mutable, so that Instruments can (indirectly) make use of task map.
		mutable TasksMapType Tasks;
	};
}