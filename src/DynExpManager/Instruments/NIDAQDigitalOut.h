// This file is part of DynExp.

/**
 * @file NIDAQDigitalOut.h
 * @brief Implementation of an instrument to control a single digital output of the
 * National Instruments NIDAQmx device.
*/

#pragma once

#include "stdafx.h"
#include "DynExpCore.h"
#include "HardwareAdapters/HardwareAdapterNIDAQ.h"
#include "MetaInstruments/DigitalOut.h"

namespace DynExpInstr
{
	class NIDAQDigitalOut;

	namespace NIDAQDigitalOutTasks
	{
		class InitTask : public DigitalOutTasks::InitTask
		{
			void InitFuncImpl(dispatch_tag<DigitalOutTasks::InitTask>, DynExp::InstrumentInstance& Instance) override final;
			virtual void InitFuncImpl(dispatch_tag<InitTask>, DynExp::InstrumentInstance& Instance) {}
		};

		class ExitTask : public DigitalOutTasks::ExitTask
		{
			void ExitFuncImpl(dispatch_tag<DigitalOutTasks::ExitTask>, DynExp::InstrumentInstance& Instance) override final;
			virtual void ExitFuncImpl(dispatch_tag<ExitTask>, DynExp::InstrumentInstance& Instance) {}
		};

		class UpdateTask : public DigitalOutTasks::UpdateTask
		{
			void UpdateFuncImpl(dispatch_tag<DigitalOutTasks::UpdateTask>, DynExp::InstrumentInstance& Instance) override final;
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
	}

	class NIDAQDigitalOutData : public DigitalOutData
	{
	public:
		NIDAQDigitalOutData() = default;
		virtual ~NIDAQDigitalOutData() = default;

		DynExp::LinkedObjectWrapperContainer<DynExpHardware::NIDAQHardwareAdapter> HardwareAdapter;
		DynExpHardware::NIDAQHardwareAdapter::ChannelHandleType ChannelHandle{};

	private:
		void ResetImpl(dispatch_tag<DigitalOutData>) override final;
		virtual void ResetImpl(dispatch_tag<NIDAQDigitalOutData>) {};
	};

	class NIDAQDigitalOutParams : public DigitalOutParams
	{
	public:
		NIDAQDigitalOutParams(DynExp::ItemIDType ID, const DynExp::DynExpCore& Core)
			: DigitalOutParams(ID, Core), NIDAQOutputPortParams(*this) {}
		virtual ~NIDAQDigitalOutParams() = default;

		virtual const char* GetParamClassTag() const noexcept override { return "NIDAQDigitalOutParams"; }

		Param<DynExp::ObjectLink<DynExpHardware::NIDAQHardwareAdapter>> HardwareAdapter = { *this, GetCore().GetHardwareAdapterManager(),
			"HardwareAdapter", "NIDAQmx controller", "Underlying hardware adapter of this instrument", DynExpUI::Icons::HardwareAdapter };
		Param<ParamsConfigDialog::TextType> ChannelName = { *this, "ChannelName", "Channel name",
			"Path of the digital out channel to be used", true, "Dev1/port0/line0" };

		DynExpHardware::NIDAQOutputPortParamsExtension NIDAQOutputPortParams;

	private:
		void ConfigureParamsImpl(dispatch_tag<DigitalOutParams>) override final;
		virtual void ConfigureParamsImpl(dispatch_tag<NIDAQDigitalOutParams>) {}
	};

	class NIDAQDigitalOutConfigurator : public DigitalOutConfigurator
	{
	public:
		using ObjectType = NIDAQDigitalOut;
		using ParamsType = NIDAQDigitalOutParams;

		NIDAQDigitalOutConfigurator() = default;
		virtual ~NIDAQDigitalOutConfigurator() = default;

	private:
		virtual DynExp::ParamsBasePtrType MakeParams(DynExp::ItemIDType ID, const DynExp::DynExpCore& Core) const override { return DynExp::MakeParams<NIDAQDigitalOutConfigurator>(ID, Core); }
	};

	class NIDAQDigitalOut : public DigitalOut
	{
	public:
		using ParamsType = NIDAQDigitalOutParams;
		using ConfigType = NIDAQDigitalOutConfigurator;
		using InstrumentDataType = NIDAQDigitalOutData;

		constexpr static auto Name() noexcept { return "NIDAQ Digital Out"; }

		NIDAQDigitalOut(const std::thread::id OwnerThreadID, DynExp::ParamsBasePtrType&& Params);
		virtual ~NIDAQDigitalOut() {}

		virtual std::string GetName() const override { return Name(); }

		virtual StreamSizeParamsExtension::ValueType GetStreamSizeParams() const override;
		virtual NumericSampleStreamParamsExtension::ValueType GetNumericSampleStreamParams() const override;

		virtual void WriteData(DynExp::TaskBase::CallbackType CallbackFunc) const override { MakeAndEnqueueTask<NIDAQDigitalOutTasks::WriteTask>(CallbackFunc); }
		virtual void ClearData(DynExp::TaskBase::CallbackType CallbackFunc = nullptr) const override { MakeAndEnqueueTask<NIDAQDigitalOutTasks::ClearTask>(CallbackFunc); }
		virtual void Start(DynExp::TaskBase::CallbackType CallbackFunc = nullptr) const override { MakeAndEnqueueTask<NIDAQDigitalOutTasks::StartTask>(CallbackFunc); }
		virtual void Stop(DynExp::TaskBase::CallbackType CallbackFunc = nullptr) const override { MakeAndEnqueueTask<NIDAQDigitalOutTasks::StopTask>(CallbackFunc); }
		void Restart(DynExp::TaskBase::CallbackType CallbackFunc = nullptr) const override { MakeAndEnqueueTask<NIDAQDigitalOutTasks::RestartTask>(CallbackFunc); }
		virtual Util::OptionalBool HasFinished() const override;
		virtual void SetDefault(DynExp::TaskBase::CallbackType CallbackFunc = nullptr) const override;

	private:
		virtual void OnErrorChild() const override;

		void ResetImpl(dispatch_tag<DigitalOut>) override final;
		virtual void ResetImpl(dispatch_tag<NIDAQDigitalOut>) {}

		virtual Util::FeatureTester<WaveformCapsType> GetWaveformCapsChild() const override { return std::array{ WaveformCapsType::UserDefined }; }

		virtual std::unique_ptr<DynExp::InitTaskBase> MakeInitTask() const override { return DynExp::MakeTask<NIDAQDigitalOutTasks::InitTask>(); }
		virtual std::unique_ptr<DynExp::ExitTaskBase> MakeExitTask() const override { return DynExp::MakeTask<NIDAQDigitalOutTasks::ExitTask>(); }
		virtual std::unique_ptr<DynExp::UpdateTaskBase> MakeUpdateTask() const override { return DynExp::MakeTask<NIDAQDigitalOutTasks::UpdateTask>(); }
	};
}