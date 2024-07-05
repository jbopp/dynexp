// This file is part of DynExp.

#include "stdafx.h"
#include "HardwareAdapterNIDAQ.h"

namespace DynExpHardware
{
	Util::TextValueListType<NIDAQOutputPortParamsExtension::UseOnlyOnBrdMemType> NIDAQOutputPortParamsExtension::UseOnlyOnBrdMemTypeStrList()
	{
		Util::TextValueListType<UseOnlyOnBrdMemType> List = {
			{ "Buffered transfer", UseOnlyOnBrdMemType::UseMemoryBuffer },
			{ "Direct transfer", UseOnlyOnBrdMemType::OnlyOnboardMemory }
		};

		return List;
	}

	void NIDAQOutputPortParamsExtension::DisableUserEditable()
	{
		DynExp::ParamsBase::DisableUserEditable(UseOnlyOnBrdMem);
	}

	Util::TextValueListType<NIDAQHardwareAdapterParams::ChannelModeType> NIDAQHardwareAdapterParams::ChannelModeTypeStrList()
	{
		Util::TextValueListType<ChannelModeType> List = {
			{ "One task per channel", ChannelModeType::TaskPerChannel },
			{ "Combine channels into one task", ChannelModeType::CombineChannels }
		};

		return List;
	}

	Util::TextValueListType<NIDAQHardwareAdapterParams::TriggerModeType> NIDAQHardwareAdapterParams::TriggerModeTypeStrList()
	{
		Util::TextValueListType<TriggerModeType> List = {
			{ "Trigger disabled (start immediately)", TriggerModeType::Disabled },
			{ "Trigger on rising edge", TriggerModeType::RisingEdge },
			{ "Trigger on falling edge", TriggerModeType::FallingEdge }
		};

		return List;
	}

	auto NIDAQHardwareAdapter::Enumerate()
	{
		// Get required buffer size.
		auto RequiredSize = NIDAQSyms::DAQmxGetSysDevNames(nullptr, 0);
		if (RequiredSize < 0)
			throw NIDAQException("Error obtaining buffer size for enumerating NIDAQmx devices.", RequiredSize);

		std::string DeviceList;
		DeviceList.resize(RequiredSize);

		// Since C++17, writing to std::string's internal buffer is allowed.
		auto Result = NIDAQSyms::DAQmxGetSysDevNames(DeviceList.data(), Util::NumToT<NIDAQSyms::uInt32>(DeviceList.size()));
		if (Result < 0)
			throw NIDAQException("Error enumerating NIDAQmx devices.", Result);
		DeviceList = Util::TrimTrailingZeros(DeviceList);

		// All descriptors are separated by ','. Replace that by ' ' and use std::istringstream to split by ' '.
		std::replace(DeviceList.begin(), DeviceList.end(), ',', ' ');
		std::istringstream ss(DeviceList);
		std::vector<std::string> DeviceDescriptors{ std::istream_iterator<std::string>(ss), std::istream_iterator<std::string>() };

		return DeviceDescriptors;
	}

	NIDAQTask::CircularStream::CircularStream(size_t BufferSize)
		: Buffer(BufferSize), Stream(&Buffer)
	{
		Stream.exceptions(std::iostream::failbit | std::iostream::badbit);
	}

	void NIDAQTask::CircularStream::Clear()
	{
		Stream.clear();
		Buffer.clear();
	}

	NIDAQTask::NIDAQTask(ChannelType Type, NIDAQSyms::TaskHandle NITask, NIDAQHardwareAdapterParams::ChannelModeType ChannelMode)
		: Type(Type), NITask(NITask), ChannelMode(ChannelMode)
	{
		if (!NITask)
			throw Util::InvalidArgException("NITask cannot be nullptr.");
	}

	NIDAQTask::~NIDAQTask()
	{
		if (NITask)
		{
			// Ignore errors here since they cannot be handled properly (tasks should always be clearable).
			NIDAQSyms::DAQmxStopTask(NITask);
			NIDAQSyms::DAQmxClearTask(NITask);
		}
	}

	void NIDAQTask::AddChannel(ChannelHandleType ChannelHandle, uint64_t NumSamples)
	{
		ChannelIndexMap[ChannelHandle] = NumChannels++;
		this->NumSamples = NumSamples;

		if (!IsCombined())
			return;

		if (Type == ChannelType::DigialIn || Type == ChannelType::DigitalOut)
			DigitalValues.resize(GetBufferSizeInSamples(), 0);
		else
			AnalogValues.resize(GetBufferSizeInSamples(), .0);

		if (Type == ChannelType::DigialIn || Type == ChannelType::AnalogIn)
		{
			ReadStreamPerChannel.clear();
			for (decltype(NumChannels) i = 0; i < NumChannels; ++i)
				ReadStreamPerChannel.emplace_back(std::make_unique<CircularStream>(GetSampleSizeInBytes() * NumSamples));
		}
	}

	NIDAQHardwareAdapter::NIDAQHardwareAdapter(const std::thread::id OwnerThreadID, DynExp::ParamsBasePtrType&& Params)
		: HardwareAdapterBase(OwnerThreadID, std::move(Params))
	{
	}

	NIDAQHardwareAdapter::~NIDAQHardwareAdapter()
	{
		// Nothing to do (refer to IsConnectedChild()).
		// NIDAQTask objects will be destroyed when Tasks is destroyed.
	}

	NIDAQHardwareAdapter::ChannelHandleType NIDAQHardwareAdapter::InitializeDigitalInChannel(std::string_view ChannelName,
		double Timeout) const
	{
		auto lock = AcquireLock(HardwareOperationTimeout);

		auto Handle = CreateTaskIfNotExistsUnsafe(ChannelName, NIDAQTask::ChannelType::DigialIn);
		auto Task = GetTaskUnsafe(Handle);
		
		auto Result = NIDAQSyms::DAQmxCreateDIChan(Task->NITask, ChannelName.data(), "", DAQmx_Val_ChanPerLine);
		CheckError(Result);

		{
			auto DerivedParams = dynamic_Params_cast<NIDAQHardwareAdapter>(GetParams());
			Task->AddChannel(Handle, DerivedParams->StreamSizeParams.StreamSize);

			Result = NIDAQSyms::DAQmxSetReadReadAllAvailSamp(Task->NITask, true);
			CheckError(Result);
			Result = NIDAQSyms::DAQmxSetReadOverWrite(Task->NITask, DAQmx_Val_OverwriteUnreadSamps);
			CheckError(Result);

			InitializeTaskTimingUnsafe(Task, Timeout, DerivedParams->NumericSampleStreamParams.SamplingRate,
				DerivedParams->NumericSampleStreamParams.SamplingMode);
			InitializeTriggerUnsafe(Task, DerivedParams->TriggerMode, DerivedParams->TriggerChannel.Get());
		} // DerivedParams unlocked here.

		StartTaskUnsafe(Task);

		return Handle;
	}

	NIDAQHardwareAdapter::ChannelHandleType NIDAQHardwareAdapter::InitializeDigitalOutChannel(std::string_view ChannelName,
		NIDAQOutputPortParamsExtension::UseOnlyOnBrdMemType UseOnlyOnBrdMem, double Timeout) const
	{
		auto lock = AcquireLock(HardwareOperationTimeout);

		auto Handle = CreateTaskIfNotExistsUnsafe(ChannelName, NIDAQTask::ChannelType::DigitalOut);
		auto Task = GetTaskUnsafe(Handle);
		
		auto Result = NIDAQSyms::DAQmxCreateDOChan(Task->NITask, ChannelName.data(), "", DAQmx_Val_ChanPerLine);
		CheckError(Result);
		
		auto DerivedParams = dynamic_Params_cast<NIDAQHardwareAdapter>(GetParams());
		Task->AddChannel(Handle, DerivedParams->StreamSizeParams.StreamSize);

		if (DerivedParams->StreamSizeParams.StreamSize > 1)
		{
			Result = NIDAQSyms::DAQmxSetBufOutputBufSize(Task->NITask, DerivedParams->StreamSizeParams.StreamSize);
			CheckError(Result);
			Result = NIDAQSyms::DAQmxSetWriteRelativeTo(Task->NITask, DAQmx_Val_FirstSample);
			CheckError(Result);
		}

		Result = NIDAQSyms::DAQmxSetDOUseOnlyOnBrdMem(Task->NITask, ChannelName.data(),
			UseOnlyOnBrdMem == NIDAQOutputPortParamsExtension::UseOnlyOnBrdMemType::OnlyOnboardMemory);
		CheckError(Result);
			
		InitializeTaskTimingUnsafe(Task, Timeout, DerivedParams->NumericSampleStreamParams.SamplingRate,
			DerivedParams->NumericSampleStreamParams.SamplingMode);
		InitializeTriggerUnsafe(Task, DerivedParams->TriggerMode, DerivedParams->TriggerChannel.Get());
		
		if (DerivedParams->StreamSizeParams.StreamSize <= 1)
			StartTaskUnsafe(Task);

		return Handle;
	}

	NIDAQHardwareAdapter::ChannelHandleType NIDAQHardwareAdapter::InitializeAnalogInChannel(std::string_view ChannelName,
		double MinValue, double MaxValue, double Timeout, int32_t TerminalConfig) const
	{
		auto lock = AcquireLock(HardwareOperationTimeout);

		auto Handle = CreateTaskIfNotExistsUnsafe(ChannelName, NIDAQTask::ChannelType::AnalogIn);
		auto Task = GetTaskUnsafe(Handle);
		
		auto Result = NIDAQSyms::DAQmxCreateAIVoltageChan(Task->NITask, ChannelName.data(), "", TerminalConfig,
			MinValue, MaxValue, DAQmx_Val_Volts, nullptr);
		CheckError(Result);

		{
			auto DerivedParams = dynamic_Params_cast<NIDAQHardwareAdapter>(GetParams());
			Task->AddChannel(Handle, DerivedParams->StreamSizeParams.StreamSize);
		
			Result = NIDAQSyms::DAQmxSetReadReadAllAvailSamp(Task->NITask, true);
			CheckError(Result);
			Result = NIDAQSyms::DAQmxSetReadOverWrite(Task->NITask, DAQmx_Val_OverwriteUnreadSamps);
			CheckError(Result);

			InitializeTaskTimingUnsafe(Task, Timeout, DerivedParams->NumericSampleStreamParams.SamplingRate,
				DerivedParams->NumericSampleStreamParams.SamplingMode);
			InitializeTriggerUnsafe(Task, DerivedParams->TriggerMode, DerivedParams->TriggerChannel.Get());
		} // DerivedParams unlocked here.

		StartTaskUnsafe(Task);

		return Handle;
	}

	NIDAQHardwareAdapter::ChannelHandleType NIDAQHardwareAdapter::InitializeAnalogOutChannel(std::string_view ChannelName,
		double MinValue, double MaxValue, NIDAQOutputPortParamsExtension::UseOnlyOnBrdMemType UseOnlyOnBrdMem, double Timeout) const
	{
		auto lock = AcquireLock(HardwareOperationTimeout);

		auto Handle = CreateTaskIfNotExistsUnsafe(ChannelName, NIDAQTask::ChannelType::AnalogOut);
		auto Task = GetTaskUnsafe(Handle);
		
		auto Result = NIDAQSyms::DAQmxCreateAOVoltageChan(Task->NITask, ChannelName.data(), "",
			MinValue, MaxValue, DAQmx_Val_Volts, nullptr);
		CheckError(Result);

		auto DerivedParams = dynamic_Params_cast<NIDAQHardwareAdapter>(GetParams());
		Task->AddChannel(Handle, DerivedParams->StreamSizeParams.StreamSize);

		if (DerivedParams->StreamSizeParams.StreamSize > 1)
		{
			Result = NIDAQSyms::DAQmxSetBufOutputBufSize(Task->NITask, DerivedParams->StreamSizeParams.StreamSize);
			CheckError(Result);
			Result = NIDAQSyms::DAQmxSetWriteRelativeTo(Task->NITask, DAQmx_Val_FirstSample);
			CheckError(Result);
		}

		Result = NIDAQSyms::DAQmxSetAOUseOnlyOnBrdMem(Task->NITask, ChannelName.data(),
			UseOnlyOnBrdMem == NIDAQOutputPortParamsExtension::UseOnlyOnBrdMemType::OnlyOnboardMemory);
		CheckError(Result);

		InitializeTaskTimingUnsafe(Task, Timeout, DerivedParams->NumericSampleStreamParams.SamplingRate,
			DerivedParams->NumericSampleStreamParams.SamplingMode);
		InitializeTriggerUnsafe(Task, DerivedParams->TriggerMode, DerivedParams->TriggerChannel.Get());

		if (DerivedParams->StreamSizeParams.StreamSize <= 1)
			StartTaskUnsafe(Task);

		return Handle;
	}

	bool NIDAQHardwareAdapter::DeregisterChannel(ChannelHandleType ChannelHandle) const
	{
		auto lock = AcquireLock(HardwareOperationTimeout);

		return RemoveTaskUnsafe(ChannelHandle);
	}

	std::vector<NIDAQTask::DigitalValueType> NIDAQHardwareAdapter::ReadDigitalValues(ChannelHandleType ChannelHandle) const
	{
		auto lock = AcquireLock(HardwareOperationTimeout);

		auto Task = GetTaskUnsafe(ChannelHandle);
		if (Task->GetNumSamples() > std::numeric_limits<NIDAQSyms::int32>::max())
			ThrowExceptionUnsafe(std::make_exception_ptr(Util::OverflowException(
				"Number of samples to read must not exceed " + std::to_string(std::numeric_limits<NIDAQSyms::int32>::max()) + ".")));

		// Restart if finished
		NIDAQSyms::uInt64 Position{};
		auto Result = NIDAQSyms::DAQmxGetReadCurrReadPos(Task->NITask, &Position);
		CheckError(Result);
		if (Position == Task->GetNumSamples() && HasFinishedTaskUnsafe(Task))
			RestartTaskUnsafe(Task);

		std::vector<NIDAQTask::DigitalValueType> Data;
		Data.resize(Task->GetBufferSizeInSamples(), 0);

		NIDAQSyms::int32 NumSamplesRead = 0;
		NIDAQSyms::int32 BytesPerSample = 0;
		Result = NIDAQSyms::DAQmxReadDigitalLines(Task->NITask, DAQmx_Val_Auto, Task->GetTimeout(), DAQmx_Val_GroupByChannel,
			Data.data(), Util::NumToT<NIDAQSyms::uInt32>(Task->GetBufferSizeInSamples() * Task->GetSampleSizeInBytes()),
			&NumSamplesRead, &BytesPerSample, nullptr);
		if (Result != DAQmxErrorOperationTimedOut)	// Ignore spurious timeout erros. Returns an empty vector if this error occurs.
			CheckReadError(Task, Result);

		// BytesPerSample is the amount of bytes that one channel consists of.
		if (BytesPerSample != Task->GetSampleSizeInBytes())
			ThrowExceptionUnsafe(std::make_exception_ptr(Util::InvalidDataException(
				"Received data from DAQmxReadDigitalLines() which does not correspond to the expected memory layout.")));

		if (!Task->IsCombined())
			Data.resize(NumSamplesRead);
		else
		{
			for (decltype(Task->NumChannels) i = 0; i < Task->NumChannels; ++i)
				Task->ReadStreamPerChannel[i]->Stream.write(reinterpret_cast<const char*>(Data.data() + i * NumSamplesRead), NumSamplesRead * Task->GetSampleSizeInBytes());

			const auto NumBytesToRead = Task->ReadStreamPerChannel[Task->GetChannelIndex(ChannelHandle)]->Buffer.gsize();
			Data.clear();
			Data.resize(NumBytesToRead / Task->GetSampleSizeInBytes());
			Task->ReadStreamPerChannel[Task->GetChannelIndex(ChannelHandle)]->Stream.read(reinterpret_cast<char*>(Data.data()), NumBytesToRead);
			Task->ReadStreamPerChannel[Task->GetChannelIndex(ChannelHandle)]->Clear();
		}

		return Data;
	}

	// Returns number of samples successfully written.
	int32_t NIDAQHardwareAdapter::WriteDigitalValues(ChannelHandleType ChannelHandle, const std::vector<NIDAQTask::DigitalValueType>& Values) const
	{
		if (Values.empty())
			return 0;

		auto lock = AcquireLock(HardwareOperationTimeout);

		auto Task = GetTaskUnsafe(ChannelHandle);
		if (Values.size() > std::numeric_limits<NIDAQSyms::int32>::max())
			ThrowExceptionUnsafe(std::make_exception_ptr(Util::OverflowException(
				"Number of samples to write must not exceed " + std::to_string(std::numeric_limits<NIDAQSyms::int32>::max()) + ".")));

		NIDAQSyms::int32 NumSamplesWritten = 0;
		if (!Task->IsCombined())
		{
			auto Result = NIDAQSyms::DAQmxWriteDigitalLines(Task->NITask, Util::NumToT<NIDAQSyms::int32>(Values.size()), false, Task->GetTimeout(),
				DAQmx_Val_GroupByChannel, Values.data(), &NumSamplesWritten, nullptr);
			CheckError(Result);
		}
		else
		{
			auto Destiny = Task->DigitalValues | std::views::drop(Task->GetChannelIndex(ChannelHandle) * Task->GetNumSamples());
			auto NewValues = Values | std::views::take(Task->GetNumSamples());
			std::ranges::copy(NewValues, Destiny.begin());
			if (NewValues.size() < Task->GetNumSamples())
				std::ranges::fill_n(Destiny.begin() + NewValues.size(), Task->GetNumSamples() - NewValues.size(), 0);

			auto Result = NIDAQSyms::DAQmxWriteDigitalLines(Task->NITask, Util::NumToT<NIDAQSyms::int32>(Task->GetNumSamples()), false, Task->GetTimeout(),
				DAQmx_Val_GroupByChannel, Task->DigitalValues.data(), &NumSamplesWritten, nullptr);
			CheckError(Result);
		}

		return NumSamplesWritten;
	}

	std::vector<NIDAQTask::AnalogValueType> NIDAQHardwareAdapter::ReadAnalogValues(ChannelHandleType ChannelHandle) const
	{
		auto lock = AcquireLock(HardwareOperationTimeout);

		auto Task = GetTaskUnsafe(ChannelHandle);
		if (Task->GetNumSamples() > std::numeric_limits<NIDAQSyms::int32>::max())
			ThrowExceptionUnsafe(std::make_exception_ptr(Util::OverflowException(
				"Number of samples to read must not exceed " + std::to_string(std::numeric_limits<NIDAQSyms::int32>::max()) + ".")));

		// Restart if finished
		NIDAQSyms::uInt64 Position{};
		auto Result = NIDAQSyms::DAQmxGetReadCurrReadPos(Task->NITask, &Position);
		CheckError(Result);
		if (Position == Task->GetNumSamples() && HasFinishedTaskUnsafe(Task))
			RestartTaskUnsafe(Task);

		std::vector<NIDAQTask::AnalogValueType> Data;
		Data.resize(Task->GetBufferSizeInSamples(), .0);

		NIDAQSyms::int32 NumSamplesRead = 0;
		Result = NIDAQSyms::DAQmxReadAnalogF64(Task->NITask, DAQmx_Val_Auto, Task->GetTimeout(), DAQmx_Val_GroupByChannel,
			Data.data(), Util::NumToT<NIDAQSyms::uInt32>(Task->GetNumSamples()), &NumSamplesRead, nullptr);
		if (Result != DAQmxErrorOperationTimedOut)	// Ignore spurious timeout erros. Returns an empty vector if this error occurs.
			CheckReadError(Task, Result);

		if (!Task->IsCombined())
			Data.resize(NumSamplesRead);
		else
		{
			for (decltype(Task->NumChannels) i = 0; i < Task->NumChannels; ++i)
				Task->ReadStreamPerChannel[i]->Stream.write(reinterpret_cast<const char*>(Data.data() + i * NumSamplesRead), NumSamplesRead * Task->GetSampleSizeInBytes());

			const auto NumBytesToRead = Task->ReadStreamPerChannel[Task->GetChannelIndex(ChannelHandle)]->Buffer.gsize();
			Data.clear();
			Data.resize(NumBytesToRead / Task->GetSampleSizeInBytes());
			Task->ReadStreamPerChannel[Task->GetChannelIndex(ChannelHandle)]->Stream.read(reinterpret_cast<char*>(Data.data()), NumBytesToRead);
			Task->ReadStreamPerChannel[Task->GetChannelIndex(ChannelHandle)]->Clear();
		}

		return Data;
	}

	// Returns number of samples successfully written.
	int32_t NIDAQHardwareAdapter::WriteAnalogValues(ChannelHandleType ChannelHandle, const std::vector<NIDAQTask::AnalogValueType>& Values) const
	{
		if (Values.empty())
			return 0;

		auto lock = AcquireLock(HardwareOperationTimeout);

		auto Task = GetTaskUnsafe(ChannelHandle);
		if (Values.size() > std::numeric_limits<NIDAQSyms::int32>::max())
			ThrowExceptionUnsafe(std::make_exception_ptr(Util::OverflowException(
				"Number of samples to write must not exceed " + std::to_string(std::numeric_limits<NIDAQSyms::int32>::max()) + ".")));

		NIDAQSyms::int32 NumSamplesWritten = 0;
		if (!Task->IsCombined())
		{
			auto Result = NIDAQSyms::DAQmxWriteAnalogF64(Task->NITask, Util::NumToT<NIDAQSyms::int32>(Values.size()), false, Task->GetTimeout(),
				DAQmx_Val_GroupByChannel, Values.data(), &NumSamplesWritten, nullptr);
			CheckError(Result);
		}
		else
		{
			auto Destiny = Task->AnalogValues | std::views::drop(Task->GetChannelIndex(ChannelHandle) * Task->GetNumSamples());
			auto NewValues = Values | std::views::take(Task->GetNumSamples());
			std::ranges::copy(NewValues, Destiny.begin());
			if (NewValues.size() < Task->GetNumSamples())
				std::ranges::fill_n(Destiny.begin() + NewValues.size(), Task->GetNumSamples() - NewValues.size(), .0);

			auto Result = NIDAQSyms::DAQmxWriteAnalogF64(Task->NITask, Util::NumToT<NIDAQSyms::int32>(Task->GetNumSamples()), false, Task->GetTimeout(),
				DAQmx_Val_GroupByChannel, Task->AnalogValues.data(), &NumSamplesWritten, nullptr);
			CheckError(Result);
		}

		return NumSamplesWritten;
	}

	void NIDAQHardwareAdapter::StartTask(ChannelHandleType ChannelHandle) const
	{
		auto lock = AcquireLock(HardwareOperationTimeout);

		StartTaskUnsafe(GetTaskUnsafe(ChannelHandle));
	}

	void NIDAQHardwareAdapter::StopTask(ChannelHandleType ChannelHandle) const
	{
		auto lock = AcquireLock(HardwareOperationTimeout);

		StopTaskUnsafe(GetTaskUnsafe(ChannelHandle));
	}

	void NIDAQHardwareAdapter::RestartTask(ChannelHandleType ChannelHandle) const
	{
		auto lock = AcquireLock(HardwareOperationTimeout);

		RestartTaskUnsafe(GetTaskUnsafe(ChannelHandle));
	}

	bool NIDAQHardwareAdapter::HasFinishedTask(ChannelHandleType ChannelHandle) const
	{
		auto lock = AcquireLock(HardwareOperationTimeout);

		return HasFinishedTaskUnsafe(GetTaskUnsafe(ChannelHandle));
	}

	const NIDAQTask* NIDAQHardwareAdapter::GetTask(ChannelHandleType ChannelHandle) const
	{
		auto lock = AcquireLock(HardwareOperationTimeout);

		return GetTaskUnsafe(ChannelHandle);
	}

	void NIDAQHardwareAdapter::ResetImpl(dispatch_tag<HardwareAdapterBase>)
	{
		// auto lock = AcquireLock(); not necessary here, since DynExp ensures that Object::Reset() can only
		// be called if respective object is not in use.

		// NIDAQTask objects are destroyed here.
		Tasks.clear();

		ResetImpl(dispatch_tag<NIDAQHardwareAdapter>());
	}

	void NIDAQHardwareAdapter::EnsureReadyStateChild()
	{
		// Nothing to do (refer to IsConnectedChild()). Always ready ;)
	}

	bool NIDAQHardwareAdapter::IsReadyChild() const
	{
		auto lock = AcquireLock(HardwareOperationTimeout);

		auto Exception = GetExceptionUnsafe();
		Util::ForwardException(Exception);

		return true;
	}

	bool NIDAQHardwareAdapter::IsConnectedChild() const noexcept
	{
		// This is always the case since all work is done based on tasks. Those tasks itself refer
		// to a specific device. Tasks may refer to distinct devices. Thus, this class just provides
		// functionality to create the tasks. It does not manage physical NIDAQ devices itself.
		return true;
	}

	void NIDAQHardwareAdapter::CheckError(const int32_t Result, const std::source_location Location) const
	{
		if (Result == 0)
			return;

		uint32_t RequiredSize = NIDAQSyms::DAQmxGetExtendedErrorInfo(nullptr, 0);
		std::string ErrorString(RequiredSize, '\0');
		NIDAQSyms::DAQmxGetExtendedErrorInfo(ErrorString.data(), RequiredSize);

		if (Result < 0)
		{
			// AcquireLock() has already been called by an (in)direct caller of this function.
			ThrowExceptionUnsafe(std::make_exception_ptr(NIDAQException(ErrorString, Result, Util::ErrorType::Error, Location)));
		}
		else
		{
			// Only a warning has occurred.
			Util::EventLogger().Log(NIDAQException(ErrorString, Result, Util::ErrorType::Warning, Location));
		}
	}

	void NIDAQHardwareAdapter::CheckReadError(NIDAQTask* Task, const int32_t Result, const std::source_location Location) const
	{
		// If there are too many samples to be read at once, restart the task in order to clear the input buffer.
		if (Result == DAQmxErrorSamplesNoLongerAvailable)
			RestartTaskUnsafe(Task);
		else
		{
			// It is not an error if not all samples have been acquired yet.
			if (Result != DAQmxErrorSamplesNotYetAvailable)
				CheckError(Result, Location);
		}
	}

	NIDAQSyms::TaskHandle NIDAQHardwareAdapter::CreateTaskUnsafe() const
	{
		NIDAQSyms::TaskHandle Handle = nullptr;

		auto Result = NIDAQSyms::DAQmxCreateTask("", &Handle);
		CheckError(Result);
		if (!Handle)
			ThrowExceptionUnsafe(std::make_exception_ptr(Util::InvalidDataException(
				"Unexpected error creating an NIDAQ task: nullptr received from DAQmxCreateTask().")));

		return Handle;
	}

	void NIDAQHardwareAdapter::InitializeTaskTimingUnsafe(NIDAQTask* Task,
		double Timeout, double SamplingRate, DynExpInstr::NumericSampleStreamParamsExtension::SamplingModeType SamplingMode) const
	{
		if (Task->GetNumSamples() > 1)
		{
			auto NIDAQSamplingMode = SamplingMode == DynExpInstr::NumericSampleStreamParamsExtension::SamplingModeType::Single ?
				NIDAQTask::SamplingModeType::Single : NIDAQTask::SamplingModeType::Continuous;

			auto Result = NIDAQSyms::DAQmxCfgSampClkTiming(Task->NITask, "", SamplingRate, DAQmx_Val_Rising,
				NIDAQSamplingMode == NIDAQTask::SamplingModeType::Single ? DAQmx_Val_FiniteSamps : DAQmx_Val_ContSamps, Task->GetNumSamples());
			CheckError(Result);

			Task->Timeout = Timeout;
			Task->SamplingRate = SamplingRate;
			Task->SamplingMode = NIDAQSamplingMode;
		}
	}

	void NIDAQHardwareAdapter::InitializeTriggerUnsafe(NIDAQTask* Task, NIDAQHardwareAdapterParams::TriggerModeType TriggerMode, std::string_view TriggerChannelName) const
	{
		if (TriggerMode != NIDAQHardwareAdapterParams::TriggerModeType::RisingEdge && TriggerMode != NIDAQHardwareAdapterParams::TriggerModeType::FallingEdge)
			return;

		auto Result = NIDAQSyms::DAQmxCfgDigEdgeStartTrig(Task->NITask, TriggerChannelName.data(),
			TriggerMode == NIDAQHardwareAdapterParams::TriggerModeType::RisingEdge ? DAQmx_Val_Rising : DAQmx_Val_Falling);
		CheckError(Result);
	}

	void NIDAQHardwareAdapter::StartTaskUnsafe(NIDAQTask* Task) const
	{
		std::ranges::for_each(Task->ReadStreamPerChannel, [](auto& Stream) { Stream->Clear(); });

		auto Result = NIDAQSyms::DAQmxStartTask(Task->NITask);
		CheckError(Result);
	}

	void NIDAQHardwareAdapter::StopTaskUnsafe(NIDAQTask* Task) const
	{
		auto Result = NIDAQSyms::DAQmxStopTask(Task->NITask);
		CheckError(Result);
	}

	void NIDAQHardwareAdapter::RestartTaskUnsafe(NIDAQTask* Task) const
	{
		StopTaskUnsafe(Task);
		StartTaskUnsafe(Task);
	}

	bool NIDAQHardwareAdapter::HasFinishedTaskUnsafe(NIDAQTask* Task) const
	{
		NIDAQSyms::bool32 IsDone{};
		auto Result = NIDAQSyms::DAQmxIsTaskDone(Task->NITask, &IsDone);
		CheckError(Result);

		return IsDone;
	}

	NIDAQHardwareAdapter::ChannelHandleType NIDAQHardwareAdapter::ChannelNameToChannelHandle(std::string_view ChannelName) const
	{
		return std::hash<std::string_view>()(ChannelName);
	}

	bool NIDAQHardwareAdapter::TaskExistsUnsafe(ChannelHandleType ChannelHandle) const
	{
		return Tasks.find(ChannelHandle) != Tasks.cend();
	}

	bool NIDAQHardwareAdapter::TaskExistsUnsafe(std::string_view ChannelName) const
	{
		return TaskExistsUnsafe(ChannelNameToChannelHandle(ChannelName));
	}

	NIDAQTask* NIDAQHardwareAdapter::GetTaskUnsafe(ChannelHandleType ChannelHandle) const
	{
		auto Task = Tasks.find(ChannelHandle);
		if (Task == Tasks.cend())
			ThrowExceptionUnsafe(std::make_exception_ptr(Util::NotFoundException(
				"The specified ChannelHandle does not exist.")));

		return Task->second.get();
	}

	NIDAQTask* NIDAQHardwareAdapter::GetTaskUnsafe(std::string_view ChannelName) const
	{
		return GetTaskUnsafe(ChannelNameToChannelHandle(ChannelName));
	}

	NIDAQHardwareAdapter::ChannelHandleType NIDAQHardwareAdapter::InsertTaskUnsafe(std::string_view ChannelName, std::shared_ptr<NIDAQTask>&& TaskHandle) const
	{
		auto Handle = ChannelNameToChannelHandle(ChannelName);

		if (TaskExistsUnsafe(Handle))
			ThrowExceptionUnsafe(std::make_exception_ptr(Util::NotAvailableException(
				"There is already a task for channel " + std::string(ChannelName) + ".")));

		try
		{
			Tasks.emplace(std::make_pair(Handle, std::move(TaskHandle)));
		}
		catch (...)
		{
			ThrowExceptionUnsafe(std::current_exception());
		}

		return Handle;
	}

	NIDAQHardwareAdapter::ChannelHandleType NIDAQHardwareAdapter::CreateTaskIfNotExistsUnsafe(std::string_view ChannelName, NIDAQTask::ChannelType Type) const
	{
		auto DerivedParams = dynamic_Params_cast<NIDAQHardwareAdapter>(GetParams());

		if (DerivedParams->ChannelMode == NIDAQHardwareAdapterParams::ChannelModeType::CombineChannels && DerivedParams->StreamSizeParams.StreamSize > 1)
		{
			if (Tasks.empty())
				return InsertTaskUnsafe(ChannelName, std::make_shared<NIDAQTask>(Type, CreateTaskUnsafe(), NIDAQHardwareAdapterParams::ChannelModeType::CombineChannels));
			else
			{
				// Intended copy of shared_ptr.
				auto Task = Tasks.begin()->second;
				if (Task->GetType() != Type)
					ThrowExceptionUnsafe(std::make_exception_ptr(Util::InvalidArgException(
						"Only channels of the same type can be combined into one task.")));

				StopTaskUnsafe(Task.get());

				return InsertTaskUnsafe(ChannelName, std::move(Task));
			}		
		}
		else
		{
			return InsertTaskUnsafe(ChannelName, std::make_shared<NIDAQTask>(Type, CreateTaskUnsafe()));
		}
	}

	bool NIDAQHardwareAdapter::RemoveTaskUnsafe(ChannelHandleType ChannelHandle) const
	{
		// Does nothing if respective task does not exist and returns true if a taks has been removed.
		// For multiple channels combined into one task, removing one channel does only remove the
		// corresponding task if no further channels relate to that task. This is the case since
		// NIDAQmx does not support removing channels from tasks.
		return Tasks.erase(ChannelHandle) > 0;
	}
}