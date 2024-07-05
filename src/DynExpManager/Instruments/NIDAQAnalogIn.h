// This file is part of DynExp.

/**
 * @file NIDAQAnalogIn.h
 * @brief Implementation of an instrument to control a single analog input of the
 * National Instruments NIDAQmx device.
*/

#pragma once

#include "stdafx.h"
#include "DynExpCore.h"
#include "HardwareAdapters/HardwareAdapterNIDAQ.h"
#include "MetaInstruments/AnalogIn.h"

namespace DynExpInstr
{
	class NIDAQAnalogIn;

	namespace NIDAQAnalogInTasks
	{
		class InitTask : public AnalogInTasks::InitTask
		{
			void InitFuncImpl(dispatch_tag<AnalogInTasks::InitTask>, DynExp::InstrumentInstance& Instance) override final;
			virtual void InitFuncImpl(dispatch_tag<InitTask>, DynExp::InstrumentInstance& Instance) {}
		};

		class ExitTask : public AnalogInTasks::ExitTask
		{
			void ExitFuncImpl(dispatch_tag<AnalogInTasks::ExitTask>, DynExp::InstrumentInstance& Instance) override final;
			virtual void ExitFuncImpl(dispatch_tag<ExitTask>, DynExp::InstrumentInstance& Instance) {}
		};

		class UpdateTask : public AnalogInTasks::UpdateTask
		{
			void UpdateFuncImpl(dispatch_tag<AnalogInTasks::UpdateTask>, DynExp::InstrumentInstance& Instance) override final;
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

	class NIDAQAnalogInData : public AnalogInData
	{
	public:
		NIDAQAnalogInData() = default;
		virtual ~NIDAQAnalogInData() = default;

		DynExp::LinkedObjectWrapperContainer<DynExpHardware::NIDAQHardwareAdapter> HardwareAdapter;
		DynExpHardware::NIDAQHardwareAdapter::ChannelHandleType ChannelHandle{};

	private:
		void ResetImpl(dispatch_tag<AnalogInData>) override final;
		virtual void ResetImpl(dispatch_tag<NIDAQAnalogInData>) {};
	};

	class NIDAQAnalogInParams : public AnalogInParams
	{
	public:
		enum TerminalConfigType : int32_t { Default = DAQmx_Val_Cfg_Default, RSE = DAQmx_Val_RSE, NRSE = DAQmx_Val_NRSE, Diff = DAQmx_Val_Diff, PseudoDiff = DAQmx_Val_PseudoDiff };

		static Util::TextValueListType<TerminalConfigType> TerminalConfigTypeStrList();

		NIDAQAnalogInParams(DynExp::ItemIDType ID, const DynExp::DynExpCore& Core)
			: AnalogInParams(ID, Core) {}
		virtual ~NIDAQAnalogInParams() = default;

		virtual const char* GetParamClassTag() const noexcept override { return "NIDAQAnalogInParams"; }

		Param<DynExp::ObjectLink<DynExpHardware::NIDAQHardwareAdapter>> HardwareAdapter = { *this, GetCore().GetHardwareAdapterManager(),
			"HardwareAdapter", "NIDAQmx controller", "Underlying hardware adapter of this instrument", DynExpUI::Icons::HardwareAdapter };
		Param<ParamsConfigDialog::TextType> ChannelName = { *this, "ChannelName", "Channel name",
			"Path of the analog in channel to be used", true, "Dev1/ai0" };
		Param<TerminalConfigType> TerminalConfig = { *this, TerminalConfigTypeStrList(), "TerminalConfig", "Terminal config",
			"Determines the input terminal configuration of the channel.", true, TerminalConfigType::RSE };

	private:
		void ConfigureParamsImpl(dispatch_tag<AnalogInParams>) override final;
		virtual void ConfigureParamsImpl(dispatch_tag<NIDAQAnalogInParams>) {}
	};

	class NIDAQAnalogInConfigurator : public AnalogInConfigurator
	{
	public:
		using ObjectType = NIDAQAnalogIn;
		using ParamsType = NIDAQAnalogInParams;

		NIDAQAnalogInConfigurator() = default;
		virtual ~NIDAQAnalogInConfigurator() = default;

	private:
		virtual DynExp::ParamsBasePtrType MakeParams(DynExp::ItemIDType ID, const DynExp::DynExpCore& Core) const override { return DynExp::MakeParams<NIDAQAnalogInConfigurator>(ID, Core); }
	};

	class NIDAQAnalogIn : public AnalogIn
	{
	public:
		using ParamsType = NIDAQAnalogInParams;
		using ConfigType = NIDAQAnalogInConfigurator;
		using InstrumentDataType = NIDAQAnalogInData;

		constexpr static auto Name() noexcept { return "NIDAQ Analog In"; }

		NIDAQAnalogIn(const std::thread::id OwnerThreadID, DynExp::ParamsBasePtrType&& Params);
		virtual ~NIDAQAnalogIn() {}

		virtual std::string GetName() const override { return Name(); }

		constexpr static DataStreamInstrumentData::ValueType HardwareMinValue() noexcept { return -10; }
		constexpr static DataStreamInstrumentData::ValueType HardwareMaxValue() noexcept { return 10; }
		constexpr static DataStreamInstrumentData::ValueType HardwareResolution() noexcept { return 1e-3; }

		virtual DataStreamInstrumentData::ValueType GetHardwareMinValue() const noexcept override { return HardwareMinValue(); }
		virtual DataStreamInstrumentData::ValueType GetHardwareMaxValue() const noexcept override { return HardwareMaxValue(); }
		virtual DataStreamInstrumentData::ValueType GetHardwareResolution() const noexcept override final { return HardwareResolution(); }
		virtual DataStreamInstrumentData::UnitType GetValueUnit() const noexcept override { return DataStreamInstrumentData::UnitType::Volt; }
		virtual StreamSizeParamsExtension::ValueType GetStreamSizeParams() const override;
		virtual NumericSampleStreamParamsExtension::ValueType GetNumericSampleStreamParams() const override;

		virtual void ReadData(DynExp::TaskBase::CallbackType CallbackFunc) const override { MakeAndEnqueueTask<NIDAQAnalogInTasks::ReadTask>(CallbackFunc); }
		virtual void Start(DynExp::TaskBase::CallbackType CallbackFunc = nullptr) const override { MakeAndEnqueueTask<NIDAQAnalogInTasks::StartTask>(CallbackFunc); }
		virtual void Stop(DynExp::TaskBase::CallbackType CallbackFunc = nullptr) const override { MakeAndEnqueueTask<NIDAQAnalogInTasks::StopTask>(CallbackFunc); }
		virtual void Restart(DynExp::TaskBase::CallbackType CallbackFunc = nullptr) const override { MakeAndEnqueueTask<NIDAQAnalogInTasks::RestartTask>(CallbackFunc); }
		virtual Util::OptionalBool HasFinished() const override;

	private:
		virtual void OnErrorChild() const override;

		void ResetImpl(dispatch_tag<AnalogIn>) override final;
		virtual void ResetImpl(dispatch_tag<NIDAQAnalogIn>) {}

		virtual std::unique_ptr<DynExp::InitTaskBase> MakeInitTask() const override { return DynExp::MakeTask<NIDAQAnalogInTasks::InitTask>(); }
		virtual std::unique_ptr<DynExp::ExitTaskBase> MakeExitTask() const override { return DynExp::MakeTask<NIDAQAnalogInTasks::ExitTask>(); }
		virtual std::unique_ptr<DynExp::UpdateTaskBase> MakeUpdateTask() const override { return DynExp::MakeTask<NIDAQAnalogInTasks::UpdateTask>(); }
	};
}