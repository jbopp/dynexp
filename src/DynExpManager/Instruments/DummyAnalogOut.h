// This file is part of DynExp.

/**
 * @file DummyAnalogOut.h
 * @brief Implementation of an analog output port instrument without any related physical hardware.
*/

#pragma once

#include "stdafx.h"
#include "DynExpCore.h"
#include "MetaInstruments/AnalogOut.h"

namespace DynExpInstr
{
	class DummyAnalogOut;

	namespace DummyAnalogOutTasks
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
	}

	class DummyAnalogOutData : public AnalogOutData
	{

	public:
		DummyAnalogOutData() = default;
		virtual ~DummyAnalogOutData() = default;

	private:
		void ResetImpl(dispatch_tag<AnalogOutData>) override final;
		virtual void ResetImpl(dispatch_tag<DummyAnalogOutData>) {};
	};

	class DummyAnalogOutParams : public AnalogOutParams
	{
	public:
		DummyAnalogOutParams(DynExp::ItemIDType ID, const DynExp::DynExpCore& Core) : AnalogOutParams(ID, Core) {}
		virtual ~DummyAnalogOutParams() = default;

		virtual const char* GetParamClassTag() const noexcept override { return "DummyAnalogOutParams"; }

	private:
		void ConfigureParamsImpl(dispatch_tag<AnalogOutParams>) override final { ConfigureParamsImpl(dispatch_tag<DummyAnalogOutParams>()); }
		virtual void ConfigureParamsImpl(dispatch_tag<DummyAnalogOutParams>) {}

		DummyParam Dummy = { *this };
	};

	class DummyAnalogOutConfigurator : public AnalogOutConfigurator
	{
	public:
		using ObjectType = DummyAnalogOut;
		using ParamsType = DummyAnalogOutParams;

		DummyAnalogOutConfigurator() = default;
		virtual ~DummyAnalogOutConfigurator() = default;

	private:
		virtual DynExp::ParamsBasePtrType MakeParams(DynExp::ItemIDType ID, const DynExp::DynExpCore& Core) const override { return DynExp::MakeParams<DummyAnalogOutConfigurator>(ID, Core); }
	};

	class DummyAnalogOut : public AnalogOut
	{
	public:
		using ParamsType = DummyAnalogOutParams;
		using ConfigType = DummyAnalogOutConfigurator;
		using InstrumentDataType = DummyAnalogOutData;

		constexpr static auto Name() noexcept { return "Dummy Analog Out"; }

		DummyAnalogOut(const std::thread::id OwnerThreadID, DynExp::ParamsBasePtrType&& Params);
		virtual ~DummyAnalogOut() {}

		virtual std::string GetName() const override { return Name(); }

		constexpr static DataStreamInstrumentData::ValueType HardwareMinValue() noexcept { return std::numeric_limits<DataStreamInstrumentData::ValueType>::lowest(); }
		constexpr static DataStreamInstrumentData::ValueType HardwareMaxValue() noexcept { return std::numeric_limits<DataStreamInstrumentData::ValueType>::max(); }
		constexpr static DataStreamInstrumentData::ValueType HardwareResolution() noexcept { return std::numeric_limits<DataStreamInstrumentData::ValueType>::min(); }

		virtual DataStreamInstrumentData::ValueType GetHardwareMinValue() const noexcept override { return HardwareMinValue(); }
		virtual DataStreamInstrumentData::ValueType GetHardwareMaxValue() const noexcept override { return HardwareMaxValue(); }
		virtual DataStreamInstrumentData::ValueType GetHardwareResolution() const noexcept override final { return HardwareResolution(); }

		virtual void WriteData(DynExp::TaskBase::CallbackType CallbackFunc) const override {}
		virtual Util::OptionalBool HasFinished() const override { return true; }

	private:
		void ResetImpl(dispatch_tag<AnalogOut>) override final;
		virtual void ResetImpl(dispatch_tag<DummyAnalogOut>) {}

		virtual Util::FeatureTester<WaveformCapsType> GetWaveformCapsChild() const override { return std::array{ WaveformCapsType::UserDefined }; }

		virtual std::unique_ptr<DynExp::InitTaskBase> MakeInitTask() const override { return DynExp::MakeTask<DummyAnalogOutTasks::InitTask>(); }
		virtual std::unique_ptr<DynExp::ExitTaskBase> MakeExitTask() const override { return DynExp::MakeTask<DummyAnalogOutTasks::ExitTask>(); }
		virtual std::unique_ptr<DynExp::UpdateTaskBase> MakeUpdateTask() const override { return DynExp::MakeTask<DummyAnalogOutTasks::UpdateTask>(); }
	};
}