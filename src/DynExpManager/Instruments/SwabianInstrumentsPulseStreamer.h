// This file is part of DynExp.

/**
 * @file SwabianInstrumentsPulseStreamer.h
 * @brief Implementation of an instrument to control a single output of the
 * Swabian Instruments Pulse Streamer 8/2 function generator.
*/

#pragma once

#include "stdafx.h"
#include "DynExpCore.h"
#include "HardwareAdapters/HardwareAdapterSwabianInstrumentsPulseStreamer.h"
#include "MetaInstruments/FunctionGenerator.h"

namespace DynExpInstr
{
	class SwabianInstrumentsPulseStreamer;

	namespace SwabianInstrumentsPulseStreamerTasks
	{
		class InitTask : public FunctionGeneratorTasks::InitTask
		{
			void InitFuncImpl(dispatch_tag<FunctionGeneratorTasks::InitTask>, DynExp::InstrumentInstance& Instance) override final;
			virtual void InitFuncImpl(dispatch_tag<InitTask>, DynExp::InstrumentInstance& Instance) {}
		};

		class ExitTask : public FunctionGeneratorTasks::ExitTask
		{
			void ExitFuncImpl(dispatch_tag<FunctionGeneratorTasks::ExitTask>, DynExp::InstrumentInstance& Instance) override final;
			virtual void ExitFuncImpl(dispatch_tag<ExitTask>, DynExp::InstrumentInstance& Instance) {}
		};

		class UpdateTask : public FunctionGeneratorTasks::UpdateTask
		{
			void UpdateFuncImpl(dispatch_tag<FunctionGeneratorTasks::UpdateTask>,DynExp::InstrumentInstance& Instance) override final;
			virtual void UpdateFuncImpl(dispatch_tag<UpdateTask>, DynExp::InstrumentInstance& Instance) {}
		};

		class WriteTask : public DynExp::TaskBase
		{
		public:
			WriteTask(CallbackType CallbackFunc) noexcept : TaskBase(CallbackFunc) {}

		private:
			virtual DynExp::TaskResultType RunChild(DynExp::InstrumentInstance& Instance) override;
		};

		class ClearTask : public DynExp::TaskBase
		{
		public:
			ClearTask(CallbackType CallbackFunc) noexcept : TaskBase(CallbackFunc) {}

		private:
			virtual DynExp::TaskResultType RunChild(DynExp::InstrumentInstance& Instance) override;
		};

		class StartTask : public DynExp::TaskBase
		{
		public:
			StartTask(CallbackType CallbackFunc) noexcept : TaskBase(CallbackFunc) {}

		private:
			virtual DynExp::TaskResultType RunChild(DynExp::InstrumentInstance& Instance) override;
		};

		class StopTask : public DynExp::TaskBase
		{
		public:
			StopTask(CallbackType CallbackFunc) noexcept : TaskBase(CallbackFunc) {}

		private:
			virtual DynExp::TaskResultType RunChild(DynExp::InstrumentInstance& Instance) override;
		};

		class RestartTask : public DynExp::TaskBase
		{
		public:
			RestartTask(CallbackType CallbackFunc) noexcept : TaskBase(CallbackFunc) {}

		private:
			virtual DynExp::TaskResultType RunChild(DynExp::InstrumentInstance& Instance) override;
		};

		class ResetBufferSizeTask final : public DynExp::TaskBase
		{
		public:
			ResetBufferSizeTask(CallbackType CallbackFunc) noexcept : TaskBase(CallbackFunc) {}

		private:
			virtual DynExp::TaskResultType RunChild(DynExp::InstrumentInstance& Instance) override;
		};

		class ForceTriggerTask : public DynExp::TaskBase
		{
		public:
			ForceTriggerTask(CallbackType CallbackFunc) noexcept : TaskBase(CallbackFunc) {}

		private:
			virtual DynExp::TaskResultType RunChild(DynExp::InstrumentInstance& Instance) override;
		};

		class SetTriggerTask : public DynExp::TaskBase
		{
		public:
			SetTriggerTask(const FunctionGeneratorDefs::TriggerDescType& TriggerDesc, CallbackType CallbackFunc) noexcept
				: TaskBase(CallbackFunc), TriggerDesc(TriggerDesc) {}

		private:
			virtual DynExp::TaskResultType RunChild(DynExp::InstrumentInstance& Instance) override;

			FunctionGeneratorDefs::TriggerDescType TriggerDesc;
		};

		class SetConstantOutputTask : public DynExp::TaskBase
		{
		public:
			SetConstantOutputTask(const DynExpHardware::SIPulseStreamerHardwareAdapter::PulseType& Pulse, CallbackType CallbackFunc) noexcept
				: TaskBase(CallbackFunc), Pulse(Pulse) {}

		private:
			virtual DynExp::TaskResultType RunChild(DynExp::InstrumentInstance& Instance) override;

			DynExpHardware::SIPulseStreamerHardwareAdapter::PulseType Pulse;
		};

		class ForceFinalSampleTask : public DynExp::TaskBase
		{
		public:
			ForceFinalSampleTask(CallbackType CallbackFunc) noexcept : TaskBase(CallbackFunc) {}

		private:
			virtual DynExp::TaskResultType RunChild(DynExp::InstrumentInstance& Instance) override;
		};

		class SetNumRunsTask : public DynExp::TaskBase
		{
		public:
			SetNumRunsTask(int64_t NumRuns, CallbackType CallbackFunc) noexcept
				: TaskBase(CallbackFunc), NumRuns(NumRuns) {}

		private:
			virtual DynExp::TaskResultType RunChild(DynExp::InstrumentInstance& Instance) override;

			const int64_t NumRuns;
		};
	}

	class SwabianInstrumentsPulseStreamerData : public FunctionGeneratorData
	{
		friend class SwabianInstrumentsPulseStreamerTasks::InitTask;
		friend class SwabianInstrumentsPulseStreamerTasks::UpdateTask;

	public:
		using SampleStreamType = BasicSampleStream;

		SwabianInstrumentsPulseStreamerData(size_t BufferSizeInSamples = 0) : FunctionGeneratorData(std::make_unique<SampleStreamType>(BufferSizeInSamples)) {}
		virtual ~SwabianInstrumentsPulseStreamerData() = default;

		auto GetChannel() const noexcept { return Channel; }
		bool IsDigitalChannel() const noexcept;

		// Functions giving information about the instrument (data is obtained asynchronously).
		bool IsStreaming() const noexcept{ return Streaming; }
		bool HasFinished() const noexcept{ return Finished; }

		DynExp::LinkedObjectWrapperContainer<DynExpHardware::SIPulseStreamerHardwareAdapter> HardwareAdapter;

	private:
		void ResetImpl(dispatch_tag<FunctionGeneratorData>) override final;
		virtual void ResetImpl(dispatch_tag<SwabianInstrumentsPulseStreamerData>) {};

		DynExpHardware::SIPulseStreamerHardwareAdapterParams::OutputChannelType Channel = {};
		bool Streaming = false;
		bool Finished = false;

		size_t NumFailedStatusUpdateAttempts = 0;
	};

	class SwabianInstrumentsPulseStreamerParams : public FunctionGeneratorParams
	{
	public:
		SwabianInstrumentsPulseStreamerParams(DynExp::ItemIDType ID, const DynExp::DynExpCore& Core)
			: FunctionGeneratorParams(ID, Core), StreamSizeParams(*this, 100) {}
		virtual ~SwabianInstrumentsPulseStreamerParams() = default;

		virtual const char* GetParamClassTag() const noexcept override { return "SwabianInstrumentsPulseStreamerParams"; }

		Param<DynExp::ObjectLink<DynExpHardware::SIPulseStreamerHardwareAdapter>> HardwareAdapter = { *this, GetCore().GetHardwareAdapterManager(),
			"HardwareAdapter", "Device hardware adapter", "Underlying Swabian Instruments Pulse Streamer device used by this pulse streamer channel instrument",
			DynExpUI::Icons::HardwareAdapter };
		Param<DynExpHardware::SIPulseStreamerHardwareAdapterParams::OutputChannelType> Channel = { *this,
			DynExpHardware::SIPulseStreamerHardwareAdapterParams::OutputChannelTypeStrList(),
			"OutputChannel", "Output channel", "Output channel of the pulse streamer this instrument uses.",
			true, DynExpHardware::SIPulseStreamerHardwareAdapterParams::OutputChannelType::DO0 };

		// Stream size of OutputPortData's sample stream in samples.
		StreamSizeParamsExtension StreamSizeParams;

	private:
		void ConfigureParamsImpl(dispatch_tag<FunctionGeneratorParams>) override final { ConfigureParamsImpl(dispatch_tag<SwabianInstrumentsPulseStreamerParams>()); }
		virtual void ConfigureParamsImpl(dispatch_tag<SwabianInstrumentsPulseStreamerParams>) {}
	};

	class SwabianInstrumentsPulseStreamerConfigurator : public FunctionGeneratorConfigurator
	{
	public:
		using ObjectType = SwabianInstrumentsPulseStreamer;
		using ParamsType = SwabianInstrumentsPulseStreamerParams;

		SwabianInstrumentsPulseStreamerConfigurator() = default;
		virtual ~SwabianInstrumentsPulseStreamerConfigurator() = default;

	private:
		virtual DynExp::ParamsBasePtrType MakeParams(DynExp::ItemIDType ID, const DynExp::DynExpCore& Core) const override { return DynExp::MakeParams<SwabianInstrumentsPulseStreamerConfigurator>(ID, Core); }
	};

	class SwabianInstrumentsPulseStreamer : public FunctionGenerator
	{
	public:
		using ParamsType = SwabianInstrumentsPulseStreamerParams;
		using ConfigType = SwabianInstrumentsPulseStreamerConfigurator;
		using InstrumentDataType = SwabianInstrumentsPulseStreamerData;

		constexpr static auto Name() noexcept { return "Swabian Instruments Pulse Streamer"; }

		SwabianInstrumentsPulseStreamer(const std::thread::id OwnerThreadID, DynExp::ParamsBasePtrType&& Params);
		virtual ~SwabianInstrumentsPulseStreamer() {}

		virtual std::string GetName() const override { return Name(); }

		virtual std::chrono::milliseconds GetTaskQueueDelay() const override { return std::chrono::milliseconds(100); }
		virtual DataStreamInstrumentData::UnitType GetValueUnit() const noexcept override;

		// Tasks
		virtual void WriteData(DynExp::TaskBase::CallbackType CallbackFunc) const override { MakeAndEnqueueTask<SwabianInstrumentsPulseStreamerTasks::WriteTask>(CallbackFunc); }
		virtual void ClearData(DynExp::TaskBase::CallbackType CallbackFunc = nullptr) const override { MakeAndEnqueueTask<SwabianInstrumentsPulseStreamerTasks::ClearTask>(CallbackFunc); }
		virtual void Start(DynExp::TaskBase::CallbackType CallbackFunc = nullptr) const override { MakeAndEnqueueTask<SwabianInstrumentsPulseStreamerTasks::StartTask>(CallbackFunc); }
		virtual void Stop(DynExp::TaskBase::CallbackType CallbackFunc = nullptr) const override { MakeAndEnqueueTask<SwabianInstrumentsPulseStreamerTasks::StopTask>(CallbackFunc); }
		virtual void Restart(DynExp::TaskBase::CallbackType CallbackFunc = nullptr) const override { MakeAndEnqueueTask<SwabianInstrumentsPulseStreamerTasks::RestartTask>(CallbackFunc); }
		virtual void ResetStreamSize(DynExp::TaskBase::CallbackType CallbackFunc = nullptr) const override { MakeAndEnqueueTask<SwabianInstrumentsPulseStreamerTasks::ResetBufferSizeTask>(CallbackFunc); }
		virtual void ForceTrigger(DynExp::TaskBase::CallbackType CallbackFunc = nullptr) const override { return MakeAndEnqueueTask<SwabianInstrumentsPulseStreamerTasks::ForceTriggerTask>(CallbackFunc); }
		virtual void SetConstantOutput(const DynExpHardware::SIPulseStreamerHardwareAdapter::PulseType& Pulse, DynExp::TaskBase::CallbackType CallbackFunc = nullptr) const;
		virtual void ForceFinalSample(DynExp::TaskBase::CallbackType CallbackFunc = nullptr) const { return MakeAndEnqueueTask<SwabianInstrumentsPulseStreamerTasks::ForceFinalSampleTask>(CallbackFunc); }
		virtual void SetNumRuns(int64_t NumRuns, DynExp::TaskBase::CallbackType CallbackFunc = nullptr) const;

		// Functions giving information about the instrument (data is obtained synchronously).
		virtual Util::OptionalBool HasFinished() const override;
		virtual Util::OptionalBool IsRunning() const override;

	private:
		void ResetImpl(dispatch_tag<FunctionGenerator>) override final;
		virtual void ResetImpl(dispatch_tag<SwabianInstrumentsPulseStreamer>) {}

		virtual FunctionGeneratorDefs::FunctionDescType GetMinCapsChild() const override;
		virtual FunctionGeneratorDefs::FunctionDescType GetMaxCapsChild() const override;
		virtual FunctionGeneratorDefs::FunctionDescType GetParamDefaultsChild() const override;
		virtual Util::FeatureTester<WaveformCapsType> GetWaveformCapsChild() const override { return std::array{ WaveformCapsType::UserDefined }; }
		virtual Util::FeatureTester<TriggerCapsType> GetTriggerCapsChild() const override { return std::array{ TriggerCapsType::CanConfigure, TriggerCapsType::CanForce }; }

		virtual void SetTriggerChild(const FunctionGeneratorDefs::TriggerDescType& TriggerDesc,
			bool PersistParams, DynExp::TaskBase::CallbackType CallbackFunc) const override;

		virtual std::unique_ptr<DynExp::InitTaskBase> MakeInitTask() const override { return DynExp::MakeTask<SwabianInstrumentsPulseStreamerTasks::InitTask>(); }
		virtual std::unique_ptr<DynExp::ExitTaskBase> MakeExitTask() const override { return DynExp::MakeTask<SwabianInstrumentsPulseStreamerTasks::ExitTask>(); }
		virtual std::unique_ptr<DynExp::UpdateTaskBase> MakeUpdateTask() const override { return DynExp::MakeTask<SwabianInstrumentsPulseStreamerTasks::UpdateTask>(); }
	};
}