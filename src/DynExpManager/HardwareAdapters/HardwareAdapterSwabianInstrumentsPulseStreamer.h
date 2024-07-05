// This file is part of DynExp.

/**
 * @file HardwareAdapterSwabianInstrumentsPulseStreamer.h
 * @brief Implementation of a hardware adapter to control Swabian Instruments Pulse Streamer 8/2
 * hardware.
*/

#pragma once

#include "stdafx.h"
#include "HardwareAdaptergRPC.h"

#include "pulse_streamer.pb.h"
#include "pulse_streamer.grpc.pb.h"

namespace DynExpHardware
{
	class SIPulseStreamerHardwareAdapter;

	class SIPulseStreamerHardwareAdapterParams : public gRPCHardwareAdapterParams<pulse_streamer::PulseStreamer>
	{
	public:
		// No enum class to be usable with DynExp::ParamsBase::Param<>.
		enum OutputChannelType : uint8_t { DO0 = 0, DO1, DO2, DO3, DO4, DO5, DO6, DO7, AO0, AO1 };
		enum TriggerEdgeType { Immediate, Software, RisingEdge, FallingEdge, RisingAndFallingEdge };
		enum TriggerModeType { Normal, Single };

		static Util::TextValueListType<OutputChannelType> OutputChannelTypeStrList();
		static Util::TextValueListType<TriggerEdgeType> TriggerEdgeTypeStrList();
		static Util::TextValueListType<TriggerModeType> TriggerModeTypeStrList();

		SIPulseStreamerHardwareAdapterParams(DynExp::ItemIDType ID, const DynExp::DynExpCore& Core) : gRPCHardwareAdapterParams(ID, Core) {}
		virtual ~SIPulseStreamerHardwareAdapterParams() = default;

		virtual const char* GetParamClassTag() const noexcept override { return "SIPulseStreamerParams"; }

		Param<TriggerEdgeType> TriggerEdge = { *this, TriggerEdgeTypeStrList(), "TriggerEdge",
			"Trigger Edge", "Signal edge or event which triggers the pulse streamer.",
			true, TriggerEdgeType::Immediate };
		Param<TriggerModeType> TriggerMode = { *this, TriggerModeTypeStrList(), "TriggerMode",
			"Trigger Mode", "Determines whether the puslse streamer waits for subsequent trigger events (normal) or only for one (single).",
			true, TriggerModeType::Normal };
		Param<ParamsConfigDialog::NumberType> NumRuns = { *this, "NumRuns", "Number of Runs",
			"Determines how often the stored pulse sequence should be repeated after triggering. -1 means indefinitely.",
			true, 1, -1 };

	private:
		void ConfigureParamsImpl(dispatch_tag<gRPCHardwareAdapterParams>) override final { ConfigureParamsImpl(dispatch_tag<SIPulseStreamerHardwareAdapterParams>()); }
		virtual void ConfigureParamsImpl(dispatch_tag<SIPulseStreamerHardwareAdapterParams>) {}
	};

	class SIPulseStreamerHardwareAdapterConfigurator : public gRPCHardwareAdapterConfigurator<pulse_streamer::PulseStreamer>
	{
	public:
		using ObjectType = SIPulseStreamerHardwareAdapter;
		using ParamsType = SIPulseStreamerHardwareAdapterParams;

		SIPulseStreamerHardwareAdapterConfigurator() = default;
		virtual ~SIPulseStreamerHardwareAdapterConfigurator() = default;

	private:
		virtual DynExp::ParamsBasePtrType MakeParams(DynExp::ItemIDType ID, const DynExp::DynExpCore& Core) const override { return DynExp::MakeParams<SIPulseStreamerHardwareAdapterConfigurator>(ID, Core); }
	};

	class SIPulseStreamerHardwareAdapter : public gRPCHardwareAdapter<pulse_streamer::PulseStreamer>
	{
	public:
		/**
		 * @brief Swabian Instruments Pulse Streamer 8/2's internal representation of a single pulse.
		*/
		struct PulseType
		{
			uint32_t ticks = 0;		//!< Duration in ns
			uint8_t digi = 0;		//!< Digital out bit mask (LSB is channel 0, MSB is channel 7)
			int16_t ao0 = 0;		//!< Analog out channel 0 (-0x7FFF is -1.0V, 0x7FFF is 1.0V)
			int16_t ao1 = 0;		//!< Analog out channel 1 (-0x7FFF is -1.0V, 0x7FFF is 1.0V)

			std::unique_ptr<pulse_streamer::PulseMessage> ToPulseMessage() const;
		};

		/**
		 * @brief Sample of one output channel.
		*/
		struct SampleType
		{
			static int16_t MakeValueFromVoltage(const double Voltage);

			SIPulseStreamerHardwareAdapterParams::OutputChannelType Channel;
			std::chrono::nanoseconds Timestamp;
			int16_t Value;

			uint8_t ComposeDOValue() const;
		};

		using ParamsType = SIPulseStreamerHardwareAdapterParams;
		using ConfigType = SIPulseStreamerHardwareAdapterConfigurator;

		constexpr static auto Name() noexcept { return "Swabian Instruments Pulse Streamer (gRPC)"; }
		constexpr static auto Category() noexcept { return "I/O"; }

		SIPulseStreamerHardwareAdapter(const std::thread::id OwnerThreadID, DynExp::ParamsBasePtrType&& Params);
		virtual ~SIPulseStreamerHardwareAdapter();

		virtual std::string GetName() const override { return Name(); }
		virtual std::string GetCategory() const override { return Category(); }

		void ResetDevice() const;
		void SetConstantOutput(const PulseType& Pulse = {}) const;
		void ForceFinalSample() const;
		void SetSamples(SIPulseStreamerHardwareAdapterParams::OutputChannelType OutputChannel, const std::vector<SampleType>& NewSamples) const;
		void SetNumRuns(int64_t NumRuns = -1) const;
		void SetTrigger(SIPulseStreamerHardwareAdapterParams::TriggerEdgeType TriggerEdge,
			SIPulseStreamerHardwareAdapterParams::TriggerModeType TriggerMode = SIPulseStreamerHardwareAdapterParams::TriggerModeType::Normal) const;
		void ForceTrigger() const;
		void RearmTrigger() const;

		bool IsStreaming() const;
		bool HasSequence() const;
		bool HasFinished() const;

	private:
		template <typename MessageType>
		using StubFuncPtrType = grpc::Status(pulse_streamer::PulseStreamer::Stub::*)(grpc::ClientContext*, const MessageType&, pulse_streamer::PulseStreamerReply*);

		void ResetImpl(dispatch_tag<gRPCHardwareAdapter>) override final;
		virtual void ResetImpl(dispatch_tag<SIPulseStreamerHardwareAdapter>) {}

		template <typename MessageType>
		uint32_t InvokeStubFunc(StubFuncPtrType<MessageType> Func, const MessageType& Message) const;

		uint32_t InvokeStubFunc(StubFuncPtrType<pulse_streamer::VoidMessage> Func) const { return InvokeStubFunc(Func, pulse_streamer::VoidMessage()); }

		std::vector<PulseType> ComposePulseSequence() const;

		virtual void OpenUnsafeChild() override;

		void ResetDeviceUnsafe() const;
		void SetConstantOutputUnsafe(const PulseType& Pulse = {}) const;
		void ForceFinalSampleUnsafe() const;
		void SetSamplesUnsafe(SIPulseStreamerHardwareAdapterParams::OutputChannelType OutputChannel, const std::vector<SampleType>& NewSamples) const;
		void SetNumRunsUnsafe(int64_t NumRuns = -1) const;
		void SetTriggerUnsafe(SIPulseStreamerHardwareAdapterParams::TriggerEdgeType TriggerEdge,
			SIPulseStreamerHardwareAdapterParams::TriggerModeType TriggerMode = SIPulseStreamerHardwareAdapterParams::TriggerModeType::Normal) const;
		void ForceTriggerUnsafe() const;
		void RearmTriggerUnsafe() const;

		bool IsStreamingUnsafe() const;
		bool HasSequenceUnsafe() const;
		bool HasFinishedUnsafe() const;

		mutable std::vector<SampleType> Samples;	//!< Combined sample vector of all channels. Assumed to be always sorted.
		mutable int64_t NumRuns;					//!< How often to repeat the sample sequence. -1 means indefinitely.
	};

	template <typename MessageType>
	uint32_t SIPulseStreamerHardwareAdapter::InvokeStubFunc(StubFuncPtrType<MessageType> Func, const MessageType& Message) const
	{
		grpc::ClientContext Context;
		pulse_streamer::PulseStreamerReply Reply;

		static const auto Timeout = std::chrono::milliseconds(1000);
		Context.set_deadline(std::chrono::system_clock::now() + Timeout);

		auto Result = (GetStubUnsafe().*Func)(&Context, Message, &Reply);
		CheckError(Result);

		return Reply.value();
	}

	std::strong_ordering operator<=>(const SIPulseStreamerHardwareAdapter::SampleType& lhs, const SIPulseStreamerHardwareAdapter::SampleType& rhs);
}