// This file is part of DynExp.

#include "stdafx.h"
#include "HardwareAdapterSwabianInstrumentsPulseStreamer.h"

namespace DynExpHardware
{
	Util::TextValueListType<SIPulseStreamerHardwareAdapterParams::OutputChannelType> SIPulseStreamerHardwareAdapterParams::OutputChannelTypeStrList()
	{
		Util::TextValueListType<OutputChannelType> List = {
			{ "DO 0", OutputChannelType::DO0 },
			{ "DO 1", OutputChannelType::DO1 },
			{ "DO 2", OutputChannelType::DO2 },
			{ "DO 3", OutputChannelType::DO3 },
			{ "DO 4", OutputChannelType::DO4 },
			{ "DO 5", OutputChannelType::DO5 },
			{ "DO 6", OutputChannelType::DO6 },
			{ "DO 7", OutputChannelType::DO7 },
			{ "AO 0", OutputChannelType::AO0 },
			{ "AO 1", OutputChannelType::AO1 }
		};

		return List;
	}

	Util::TextValueListType<SIPulseStreamerHardwareAdapterParams::TriggerEdgeType> SIPulseStreamerHardwareAdapterParams::TriggerEdgeTypeStrList()
	{
		Util::TextValueListType<TriggerEdgeType> List = {
			{ "Immediate", TriggerEdgeType::Immediate },
			{ "Software", TriggerEdgeType::Software },
			{ "Rising Edge", TriggerEdgeType::RisingEdge },
			{ "Falling Edge", TriggerEdgeType::FallingEdge },
			{ "Rising and Falling Edge", TriggerEdgeType::RisingAndFallingEdge }
		};

		return List;
	}

	Util::TextValueListType<SIPulseStreamerHardwareAdapterParams::TriggerModeType> SIPulseStreamerHardwareAdapterParams::TriggerModeTypeStrList()
	{
		Util::TextValueListType<TriggerModeType> List = {
			{ "Normal", TriggerModeType::Normal },
			{ "Single", TriggerModeType::Single }
		};

		return List;
	}

	std::unique_ptr<pulse_streamer::PulseMessage> SIPulseStreamerHardwareAdapter::PulseType::ToPulseMessage() const
	{
		auto Message = std::make_unique<pulse_streamer::PulseMessage>();

		Message->set_ticks(ticks);
		Message->set_digi(digi);
		Message->set_ao0(ao0);
		Message->set_ao1(ao1);

		return Message;
	}

	int16_t SIPulseStreamerHardwareAdapter::SampleType::MakeValueFromVoltage(const double Voltage)
	{
		auto ClippedVoltage = std::min(Voltage, 1.0);
		ClippedVoltage = std::max(ClippedVoltage, -1.0);
		ClippedVoltage *= static_cast<double>(0x7ff);

		bool IsNeg = ClippedVoltage < 0.0;
		int16_t Value = static_cast<int16_t>(std::floor(std::abs(ClippedVoltage)));
		Value *= 16;	// Shift 4 bits to the left.

		return Value * (IsNeg ? int16_t(-1) : int16_t(1));
	}

	uint8_t SIPulseStreamerHardwareAdapter::SampleType::ComposeDOValue() const
	{
		uint8_t DOValue = 0;

		if (static_cast<std::underlying_type_t<SIPulseStreamerHardwareAdapterParams::OutputChannelType>>(Channel) <= 7)
			DOValue |= (static_cast<bool>(Value) << static_cast<std::underlying_type_t<SIPulseStreamerHardwareAdapterParams::OutputChannelType>>(Channel));

		return DOValue;
	}

	SIPulseStreamerHardwareAdapter::SIPulseStreamerHardwareAdapter(const std::thread::id OwnerThreadID, DynExp::ParamsBasePtrType&& Params)
		: gRPCHardwareAdapter(OwnerThreadID, std::move(Params)), NumRuns(-1)
	{
	}

	SIPulseStreamerHardwareAdapter::~SIPulseStreamerHardwareAdapter()
	{
		try
		{
			// Not locking, since the object is being destroyed. This should be inherently thread-safe.
			if (IsOpenedUnsafe() && !GetExceptionUnsafe())
				SetConstantOutputUnsafe();
		}
		catch (...)
		{
			// Swallow any exception in order not to cause abort on error.
		}
	}

	void SIPulseStreamerHardwareAdapter::ResetDevice() const
	{
		auto lock = AcquireLock(HardwareOperationTimeout);

		ResetDeviceUnsafe();
	}

	void SIPulseStreamerHardwareAdapter::SetConstantOutput(const PulseType& Pulse) const
	{
		auto lock = AcquireLock(HardwareOperationTimeout);

		SetConstantOutputUnsafe(Pulse);
	}

	void SIPulseStreamerHardwareAdapter::ForceFinalSample() const
	{
		auto lock = AcquireLock(HardwareOperationTimeout);

		ForceFinalSampleUnsafe();
	}

	void SIPulseStreamerHardwareAdapter::SetSamples(SIPulseStreamerHardwareAdapterParams::OutputChannelType OutputChannel,
		const std::vector<SampleType>& NewSamples) const
	{
		auto lock = AcquireLock(HardwareOperationTimeout);

		SetSamplesUnsafe(OutputChannel, NewSamples);
	}

	void SIPulseStreamerHardwareAdapter::SetNumRuns(int64_t NumRuns) const
	{
		auto lock = AcquireLock(HardwareOperationTimeout);

		SetNumRunsUnsafe(NumRuns);
	}

	void SIPulseStreamerHardwareAdapter::SetTrigger(SIPulseStreamerHardwareAdapterParams::TriggerEdgeType TriggerEdge,
		SIPulseStreamerHardwareAdapterParams::TriggerModeType TriggerMode) const
	{
		auto lock = AcquireLock(HardwareOperationTimeout);

		SetTriggerUnsafe(TriggerEdge, TriggerMode);
	}

	void SIPulseStreamerHardwareAdapter::ForceTrigger() const
	{
		auto lock = AcquireLock(HardwareOperationTimeout);

		ForceTriggerUnsafe();
	}

	void SIPulseStreamerHardwareAdapter::RearmTrigger() const
	{
		auto lock = AcquireLock(HardwareOperationTimeout);

		RearmTriggerUnsafe();
	}

	bool SIPulseStreamerHardwareAdapter::IsStreaming() const
	{
		auto lock = AcquireLock(HardwareOperationTimeout);

		return IsStreamingUnsafe();
	}

	bool SIPulseStreamerHardwareAdapter::HasSequence() const
	{
		auto lock = AcquireLock(HardwareOperationTimeout);

		return HasSequenceUnsafe();
	}

	bool SIPulseStreamerHardwareAdapter::HasFinished() const
	{
		auto lock = AcquireLock(HardwareOperationTimeout);

		return HasFinishedUnsafe();
	}

	void SIPulseStreamerHardwareAdapter::ResetImpl(dispatch_tag<gRPCHardwareAdapter>)
	{
		// auto lock = AcquireLock(); not necessary here, since DynExp ensures that Object::Reset() can only
		// be called if respective object is not in use.

		Samples.clear();
		NumRuns = -1;		// By default, repeat pulse sequence forever.

		if (IsOpenedUnsafe())
			ResetDeviceUnsafe();

		ResetImpl(dispatch_tag<SIPulseStreamerHardwareAdapter>());
	}

	std::vector<SIPulseStreamerHardwareAdapter::PulseType> SIPulseStreamerHardwareAdapter::ComposePulseSequence() const
	{
		std::vector<PulseType> Pulses;

		for (auto SampleIt = Samples.cbegin(); SampleIt != Samples.cend(); ++SampleIt)
		{
			// Duration does not matter for final pulse, thus set to zero in that case.
			uint32_t Duration = (SampleIt + 1) != Samples.cend() ? Util::NumToT<uint32_t>(((SampleIt + 1)->Timestamp - SampleIt->Timestamp).count()) : 0;

			if (Pulses.empty())
			{
				int16_t AO0Value = SampleIt->Channel == SIPulseStreamerHardwareAdapterParams::OutputChannelType::AO0 ? SampleIt->Value : 0;
				int16_t AO1Value = SampleIt->Channel == SIPulseStreamerHardwareAdapterParams::OutputChannelType::AO1 ? SampleIt->Value : 0;

				Pulses.emplace_back(Duration, SampleIt->ComposeDOValue(), AO0Value, AO1Value);
			}
			else
			{
				int16_t AO0Value = SampleIt->Channel == SIPulseStreamerHardwareAdapterParams::OutputChannelType::AO0 ? SampleIt->Value : Pulses.back().ao0;
				int16_t AO1Value = SampleIt->Channel == SIPulseStreamerHardwareAdapterParams::OutputChannelType::AO1 ? SampleIt->Value : Pulses.back().ao1;

				uint8_t DOValue = Pulses.back().digi;
				if (static_cast<std::underlying_type_t<SIPulseStreamerHardwareAdapterParams::OutputChannelType>>(SampleIt->Channel) <= 7)
					DOValue = (DOValue & ~(1 << static_cast<std::underlying_type_t<SIPulseStreamerHardwareAdapterParams::OutputChannelType>>(SampleIt->Channel))) | SampleIt->ComposeDOValue();

				if (Pulses.back().ao0 == AO0Value && Pulses.back().ao1 == AO1Value && Pulses.back().digi == DOValue && (SampleIt + 1) != Samples.cend())
					Pulses.back().ticks += Duration;
				else
					Pulses.emplace_back(Duration, DOValue, AO0Value, AO1Value);
			}
		}

		return Pulses;
	}

	void SIPulseStreamerHardwareAdapter::OpenUnsafeChild()
	{
		auto DerivedParams = dynamic_Params_cast<SIPulseStreamerHardwareAdapter>(GetParams());

		SetTriggerUnsafe(DerivedParams->TriggerEdge, DerivedParams->TriggerMode);
		SetNumRunsUnsafe(DerivedParams->NumRuns);
	}

	void SIPulseStreamerHardwareAdapter::ResetDeviceUnsafe() const
	{
		InvokeStubFunc(&pulse_streamer::PulseStreamer::Stub::reset);
	}

	void SIPulseStreamerHardwareAdapter::SetConstantOutputUnsafe(const PulseType& Pulse) const
	{
		Samples.clear();

		auto Message = Pulse.ToPulseMessage();

		InvokeStubFunc(&pulse_streamer::PulseStreamer::Stub::constant, *Message);
	}

	void SIPulseStreamerHardwareAdapter::ForceFinalSampleUnsafe() const
	{
		InvokeStubFunc(&pulse_streamer::PulseStreamer::Stub::forceFinal);
	}

	void SIPulseStreamerHardwareAdapter::SetSamplesUnsafe(SIPulseStreamerHardwareAdapterParams::OutputChannelType OutputChannel,
		const std::vector<SampleType>& NewSamples) const
	{
		if (NewSamples.empty())
			ThrowExceptionUnsafe(std::make_exception_ptr(Util::EmptyException("NewSamples must not be empty.")));
		if (std::find_if(NewSamples.cbegin(), NewSamples.cend(), [OutputChannel](const SampleType& Sample) { return Sample.Channel != OutputChannel; }) !=
			NewSamples.cend())
			ThrowExceptionUnsafe(std::make_exception_ptr(Util::InvalidDataException(
				"The channel of at least one sample in NewSamples does not match the specified OutputChannel.")));

		std::erase_if(Samples, [OutputChannel](const SampleType& Sample) { return Sample.Channel == OutputChannel; });
		Samples.insert(Samples.end(), NewSamples.cbegin(), NewSamples.cend());
		std::sort(Samples.begin(), Samples.end());

		auto Pulses = ComposePulseSequence();
		const auto FinalPulse = Pulses.back();
		Pulses.pop_back();

		if (!Pulses.empty())
		{
			pulse_streamer::SequenceMessage SequenceMessage;
			for (const auto& Pulse : Pulses)
			{
				auto PulseMessage = SequenceMessage.add_pulse();
				*PulseMessage = *Pulse.ToPulseMessage();		// TODO: Maybe some potential for speed optimization.
			}

			SequenceMessage.set_n_runs(NumRuns);
			SequenceMessage.set_allocated_final(FinalPulse.ToPulseMessage().release());

			InvokeStubFunc(&pulse_streamer::PulseStreamer::Stub::stream, SequenceMessage);
		}
		else
			SetConstantOutputUnsafe(FinalPulse);
	}

	void SIPulseStreamerHardwareAdapter::SetNumRunsUnsafe(int64_t NumRuns) const
	{
		this->NumRuns = NumRuns;
	}

	void SIPulseStreamerHardwareAdapter::SetTriggerUnsafe(SIPulseStreamerHardwareAdapterParams::TriggerEdgeType TriggerEdge,
		SIPulseStreamerHardwareAdapterParams::TriggerModeType TriggerMode) const
	{
		pulse_streamer::TriggerMessage Message;

		switch (TriggerEdge)
		{
		case SIPulseStreamerHardwareAdapterParams::TriggerEdgeType::Software: Message.set_start(pulse_streamer::TriggerMessage_Start::TriggerMessage_Start_SOFTWARE); break;
		case SIPulseStreamerHardwareAdapterParams::TriggerEdgeType::RisingEdge: Message.set_start(pulse_streamer::TriggerMessage_Start::TriggerMessage_Start_HARDWARE_RISING); break;
		case SIPulseStreamerHardwareAdapterParams::TriggerEdgeType::FallingEdge: Message.set_start(pulse_streamer::TriggerMessage_Start::TriggerMessage_Start_HARDWARE_FALLING); break;
		case SIPulseStreamerHardwareAdapterParams::TriggerEdgeType::RisingAndFallingEdge: Message.set_start(pulse_streamer::TriggerMessage_Start::TriggerMessage_Start_HARDWARE_RISING_AND_FALLING); break;
		default: Message.set_start(pulse_streamer::TriggerMessage_Start::TriggerMessage_Start_IMMEDIATE);
		}

		switch (TriggerMode)
		{
		case SIPulseStreamerHardwareAdapterParams::TriggerModeType::Normal: Message.set_mode(pulse_streamer::TriggerMessage_Mode::TriggerMessage_Mode_NORMAL); break;
		default: Message.set_mode(pulse_streamer::TriggerMessage_Mode::TriggerMessage_Mode_SINGLE);
		}

		InvokeStubFunc(&pulse_streamer::PulseStreamer::Stub::setTrigger, Message);

	}

	void SIPulseStreamerHardwareAdapter::ForceTriggerUnsafe() const
	{
		InvokeStubFunc(&pulse_streamer::PulseStreamer::Stub::startNow);
	}

	void SIPulseStreamerHardwareAdapter::RearmTriggerUnsafe() const
	{
		InvokeStubFunc(&pulse_streamer::PulseStreamer::Stub::rearm);
	}

	bool SIPulseStreamerHardwareAdapter::IsStreamingUnsafe() const
	{
		return InvokeStubFunc(&pulse_streamer::PulseStreamer::Stub::isStreaming);
	}

	bool SIPulseStreamerHardwareAdapter::HasSequenceUnsafe() const
	{
		return InvokeStubFunc(&pulse_streamer::PulseStreamer::Stub::hasSequence);
	}

	bool SIPulseStreamerHardwareAdapter::HasFinishedUnsafe() const
	{
		return InvokeStubFunc(&pulse_streamer::PulseStreamer::Stub::hasFinished);
	}

	std::strong_ordering operator<=>(const SIPulseStreamerHardwareAdapter::SampleType& lhs, const SIPulseStreamerHardwareAdapter::SampleType& rhs)
	{
		if (lhs.Timestamp == rhs.Timestamp)
			return std::strong_ordering::equal;
		else if (lhs.Timestamp > rhs.Timestamp)
			return std::strong_ordering::greater;
		else
			return std::strong_ordering::less;
	}
}