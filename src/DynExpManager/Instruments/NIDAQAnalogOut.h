// This file is part of DynExp.

/**
 * @file NIDAQAnalogOut.h
 * @brief Implementation of an instrument to control a single analog output of the
 * National Instruments NIDAQmx device.
*/

#pragma once

#include "stdafx.h"
#include "DynExpCore.h"
#include "HardwareAdapters/HardwareAdapterNIDAQ.h"
#include "MetaInstruments/AnalogOut.h"

namespace DynExpInstr
{
	class NIDAQAnalogOut;

	namespace NIDAQAnalogOutTasks
	{
		class InitTask : public AnalogOutTasks::InitTask
		{
			void InitFuncImpl(dispatch_tag<AnalogOutTasks::InitTask>, DynExp::InstrumentInstance& Instance) override final;
			virtual void InitFuncImpl(dispatch_tag<InitTask>, DynExp::InstrumentInstance& Instance) {}
		};

		class ExitTask : public AnalogOutTasks::ExitTask
		{
			void ExitFuncImpl(dispatch_tag<AnalogOutTasks::ExitTask>, DynExp::InstrumentInstance& Instance) override final;
			virtual void ExitFuncImpl(dispatch_tag<ExitTask>, DynExp::InstrumentInstance& Instance) {}
		};

		class UpdateTask : public AnalogOutTasks::UpdateTask
		{
			void UpdateFuncImpl(dispatch_tag<AnalogOutTasks::UpdateTask>, DynExp::InstrumentInstance& Instance) override final;
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

	class NIDAQAnalogOutData : public AnalogOutData
	{
	public:
		NIDAQAnalogOutData() = default;
		virtual ~NIDAQAnalogOutData() = default;

		DynExp::LinkedObjectWrapperContainer<DynExpHardware::NIDAQHardwareAdapter> HardwareAdapter;
		DynExpHardware::NIDAQHardwareAdapter::ChannelHandleType ChannelHandle{};

	private:
		void ResetImpl(dispatch_tag<AnalogOutData>) override final;
		virtual void ResetImpl(dispatch_tag<NIDAQAnalogOutData>) {};
	};

	class NIDAQAnalogOutParams : public AnalogOutParams
	{
	public:
		NIDAQAnalogOutParams(DynExp::ItemIDType ID, const DynExp::DynExpCore& Core)
			: AnalogOutParams(ID, Core), NIDAQOutputPortParams(*this) {}
		virtual ~NIDAQAnalogOutParams() = default;

		virtual const char* GetParamClassTag() const noexcept override { return "NIDAQAnalogOutParams"; }

		Param<DynExp::ObjectLink<DynExpHardware::NIDAQHardwareAdapter>> HardwareAdapter = { *this, GetCore().GetHardwareAdapterManager(),
			"HardwareAdapter", "NIDAQmx controller", "Underlying hardware adapter of this instrument", DynExpUI::Icons::HardwareAdapter };
		Param<ParamsConfigDialog::TextType> ChannelName = { *this, "ChannelName", "Channel name",
			"Path of the analog out channel to be used", true, "Dev1/ao0" };

		DynExpHardware::NIDAQOutputPortParamsExtension NIDAQOutputPortParams;

	private:
		void ConfigureParamsImpl(dispatch_tag<AnalogOutParams>) override final;
		virtual void ConfigureParamsImpl(dispatch_tag<NIDAQAnalogOutParams>) {}
	};

	class NIDAQAnalogOutConfigurator : public AnalogOutConfigurator
	{
	public:
		using ObjectType = NIDAQAnalogOut;
		using ParamsType = NIDAQAnalogOutParams;

		NIDAQAnalogOutConfigurator() = default;
		virtual ~NIDAQAnalogOutConfigurator() = default;

	private:
		virtual DynExp::ParamsBasePtrType MakeParams(DynExp::ItemIDType ID, const DynExp::DynExpCore& Core) const override { return DynExp::MakeParams<NIDAQAnalogOutConfigurator>(ID, Core); }
	};

	class NIDAQAnalogOut : public AnalogOut
	{
	public:
		using ParamsType = NIDAQAnalogOutParams;
		using ConfigType = NIDAQAnalogOutConfigurator;
		using InstrumentDataType = NIDAQAnalogOutData;

		constexpr static auto Name() noexcept { return "NIDAQ Analog Out"; }

		NIDAQAnalogOut(const std::thread::id OwnerThreadID, DynExp::ParamsBasePtrType&& Params);
		virtual ~NIDAQAnalogOut() {}

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

		virtual void WriteData(DynExp::TaskBase::CallbackType CallbackFunc) const override { MakeAndEnqueueTask<NIDAQAnalogOutTasks::WriteTask>(CallbackFunc); }
		virtual void ClearData(DynExp::TaskBase::CallbackType CallbackFunc = nullptr) const override { MakeAndEnqueueTask<NIDAQAnalogOutTasks::ClearTask>(CallbackFunc); }
		virtual void Start(DynExp::TaskBase::CallbackType CallbackFunc = nullptr) const override { MakeAndEnqueueTask<NIDAQAnalogOutTasks::StartTask>(CallbackFunc); }
		virtual void Stop(DynExp::TaskBase::CallbackType CallbackFunc = nullptr) const override { MakeAndEnqueueTask<NIDAQAnalogOutTasks::StopTask>(CallbackFunc); }
		virtual void Restart(DynExp::TaskBase::CallbackType CallbackFunc = nullptr) const override { MakeAndEnqueueTask<NIDAQAnalogOutTasks::RestartTask>(CallbackFunc); }
		virtual Util::OptionalBool HasFinished() const override;
		virtual void SetDefault(DynExp::TaskBase::CallbackType CallbackFunc = nullptr) const override;

	private:
		virtual void OnErrorChild() const override;

		void ResetImpl(dispatch_tag<AnalogOut>) override final;
		virtual void ResetImpl(dispatch_tag<NIDAQAnalogOut>) {}

		virtual Util::FeatureTester<WaveformCapsType> GetWaveformCapsChild() const override { return std::array{ WaveformCapsType::UserDefined }; }

		virtual std::unique_ptr<DynExp::InitTaskBase> MakeInitTask() const override { return DynExp::MakeTask<NIDAQAnalogOutTasks::InitTask>(); }
		virtual std::unique_ptr<DynExp::ExitTaskBase> MakeExitTask() const override { return DynExp::MakeTask<NIDAQAnalogOutTasks::ExitTask>(); }
		virtual std::unique_ptr<DynExp::UpdateTaskBase> MakeUpdateTask() const override { return DynExp::MakeTask<NIDAQAnalogOutTasks::UpdateTask>(); }
	};
}