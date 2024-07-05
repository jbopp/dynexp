// This file is part of DynExp.

/**
 * @file NIDAQDigitalIn.h
 * @brief Implementation of an instrument to control a single digital input of the
 * National Instruments NIDAQmx device.
*/

#pragma once

#include "stdafx.h"
#include "DynExpCore.h"
#include "HardwareAdapters/HardwareAdapterNIDAQ.h"
#include "MetaInstruments/DigitalIn.h"

namespace DynExpInstr
{
	class NIDAQDigitalIn;

	namespace NIDAQDigitalInTasks
	{
		class InitTask : public DigitalInTasks::InitTask
		{
			void InitFuncImpl(dispatch_tag<DigitalInTasks::InitTask>, DynExp::InstrumentInstance& Instance) override final;
			virtual void InitFuncImpl(dispatch_tag<InitTask>, DynExp::InstrumentInstance& Instance) {}
		};

		class ExitTask : public DigitalInTasks::ExitTask
		{
			void ExitFuncImpl(dispatch_tag<DigitalInTasks::ExitTask>, DynExp::InstrumentInstance& Instance) override final;
			virtual void ExitFuncImpl(dispatch_tag<ExitTask>, DynExp::InstrumentInstance& Instance) {}
		};

		class UpdateTask : public DigitalInTasks::UpdateTask
		{
			void UpdateFuncImpl(dispatch_tag<DigitalInTasks::UpdateTask>, DynExp::InstrumentInstance& Instance) override final;
			virtual void UpdateFuncImpl(dispatch_tag<UpdateTask>, DynExp::InstrumentInstance& Instance) {}
		};

		class ReadTask : public DynExp::TaskBase
		{
		public:
			ReadTask(CallbackType CallbackFunc) noexcept : TaskBase(CallbackFunc) {}

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

	class NIDAQDigitalInData : public DigitalInData
	{
	public:
		NIDAQDigitalInData() = default;
		virtual ~NIDAQDigitalInData() = default;

		DynExp::LinkedObjectWrapperContainer<DynExpHardware::NIDAQHardwareAdapter> HardwareAdapter;
		DynExpHardware::NIDAQHardwareAdapter::ChannelHandleType ChannelHandle{};

	private:
		void ResetImpl(dispatch_tag<DigitalInData>) override final;
		virtual void ResetImpl(dispatch_tag<NIDAQDigitalInData>) {};
	};

	class NIDAQDigitalInParams : public DigitalInParams
	{
	public:
		NIDAQDigitalInParams(DynExp::ItemIDType ID, const DynExp::DynExpCore& Core)
			: DigitalInParams(ID, Core) {}
		virtual ~NIDAQDigitalInParams() = default;

		virtual const char* GetParamClassTag() const noexcept override { return "NIDAQDigitalInParams"; }

		Param<DynExp::ObjectLink<DynExpHardware::NIDAQHardwareAdapter>> HardwareAdapter = { *this, GetCore().GetHardwareAdapterManager(),
			"HardwareAdapter", "NIDAQmx controller", "Underlying hardware adapter of this instrument", DynExpUI::Icons::HardwareAdapter };
		Param<ParamsConfigDialog::TextType> ChannelName = { *this, "ChannelName", "Channel name",
			"Path of the digital in channel to be used", true, "Dev1/port0/line0" };

	private:
		void ConfigureParamsImpl(dispatch_tag<DigitalInParams>) override final;
		virtual void ConfigureParamsImpl(dispatch_tag<NIDAQDigitalInParams>) {}
	};

	class NIDAQDigitalInConfigurator : public DigitalInConfigurator
	{
	public:
		using ObjectType = NIDAQDigitalIn;
		using ParamsType = NIDAQDigitalInParams;

		NIDAQDigitalInConfigurator() = default;
		virtual ~NIDAQDigitalInConfigurator() = default;

	private:
		virtual DynExp::ParamsBasePtrType MakeParams(DynExp::ItemIDType ID, const DynExp::DynExpCore& Core) const override { return DynExp::MakeParams<NIDAQDigitalInConfigurator>(ID, Core); }
	};

	class NIDAQDigitalIn : public DigitalIn
	{
	public:
		using ParamsType = NIDAQDigitalInParams;
		using ConfigType = NIDAQDigitalInConfigurator;
		using InstrumentDataType = NIDAQDigitalInData;

		constexpr static auto Name() noexcept { return "NIDAQ Digital In"; }

		NIDAQDigitalIn(const std::thread::id OwnerThreadID, DynExp::ParamsBasePtrType&& Params);
		virtual ~NIDAQDigitalIn() {}

		virtual std::string GetName() const override { return Name(); }

		virtual StreamSizeParamsExtension::ValueType GetStreamSizeParams() const override;
		virtual NumericSampleStreamParamsExtension::ValueType GetNumericSampleStreamParams() const override;

		virtual void ReadData(DynExp::TaskBase::CallbackType CallbackFunc) const override { MakeAndEnqueueTask<NIDAQDigitalInTasks::ReadTask>(CallbackFunc); }
		virtual void Start(DynExp::TaskBase::CallbackType CallbackFunc = nullptr) const override { MakeAndEnqueueTask<NIDAQDigitalInTasks::StartTask>(CallbackFunc); }
		virtual void Stop(DynExp::TaskBase::CallbackType CallbackFunc = nullptr) const override { MakeAndEnqueueTask<NIDAQDigitalInTasks::StopTask>(CallbackFunc); }
		virtual void Restart(DynExp::TaskBase::CallbackType CallbackFunc = nullptr) const override { MakeAndEnqueueTask<NIDAQDigitalInTasks::RestartTask>(CallbackFunc); }
		virtual Util::OptionalBool HasFinished() const override;

	private:
		virtual void OnErrorChild() const override;

		void ResetImpl(dispatch_tag<DigitalIn>) override final;
		virtual void ResetImpl(dispatch_tag<NIDAQDigitalIn>) {}

		virtual std::unique_ptr<DynExp::InitTaskBase> MakeInitTask() const override { return DynExp::MakeTask<NIDAQDigitalInTasks::InitTask>(); }
		virtual std::unique_ptr<DynExp::ExitTaskBase> MakeExitTask() const override { return DynExp::MakeTask<NIDAQDigitalInTasks::ExitTask>(); }
		virtual std::unique_ptr<DynExp::UpdateTaskBase> MakeUpdateTask() const override { return DynExp::MakeTask<NIDAQDigitalInTasks::UpdateTask>(); }
	};
}