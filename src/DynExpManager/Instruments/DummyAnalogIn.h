// This file is part of DynExp.

/**
 * @file DummyAnalogIn.h
 * @brief Implementation of an analog input port instrument without any related physical hardware.
*/

#pragma once

#include "stdafx.h"
#include "DynExpCore.h"
#include "MetaInstruments/AnalogIn.h"

namespace DynExpInstr
{
	class DummyAnalogIn;

	namespace DummyAnalogInTasks
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
	}

	class DummyAnalogInData : public AnalogInData
	{

	public:
		DummyAnalogInData() = default;
		virtual ~DummyAnalogInData() = default;

	private:
		void ResetImpl(dispatch_tag<AnalogInData>) override final;
		virtual void ResetImpl(dispatch_tag<DummyAnalogInData>) {};
	};

	class DummyAnalogInParams : public AnalogInParams
	{
	public:
		DummyAnalogInParams(DynExp::ItemIDType ID, const DynExp::DynExpCore& Core) : AnalogInParams(ID, Core) {}
		virtual ~DummyAnalogInParams() = default;

		virtual const char* GetParamClassTag() const noexcept override { return "DummyAnalogInParams"; }

	private:
		void ConfigureParamsImpl(dispatch_tag<AnalogInParams>) override final { ConfigureParamsImpl(dispatch_tag<DummyAnalogInParams>()); }
		virtual void ConfigureParamsImpl(dispatch_tag<DummyAnalogInParams>) {}

		DummyParam Dummy = { *this };
	};

	class DummyAnalogInConfigurator : public AnalogInConfigurator
	{
	public:
		using ObjectType = DummyAnalogIn;
		using ParamsType = DummyAnalogInParams;

		DummyAnalogInConfigurator() = default;
		virtual ~DummyAnalogInConfigurator() = default;

	private:
		virtual DynExp::ParamsBasePtrType MakeParams(DynExp::ItemIDType ID, const DynExp::DynExpCore& Core) const override { return DynExp::MakeParams<DummyAnalogInConfigurator>(ID, Core); }
	};

	class DummyAnalogIn : public AnalogIn
	{
	public:
		using ParamsType = DummyAnalogInParams;
		using ConfigType = DummyAnalogInConfigurator;
		using InstrumentDataType = DummyAnalogInData;

		constexpr static auto Name() noexcept { return "Dummy Analog In"; }

		DummyAnalogIn(const std::thread::id OwnerThreadID, DynExp::ParamsBasePtrType&& Params);
		virtual ~DummyAnalogIn() {}

		virtual std::string GetName() const override { return Name(); }

		constexpr static DataStreamInstrumentData::ValueType HardwareMinValue() noexcept { return std::numeric_limits<DataStreamInstrumentData::ValueType>::lowest(); }
		constexpr static DataStreamInstrumentData::ValueType HardwareMaxValue() noexcept { return std::numeric_limits<DataStreamInstrumentData::ValueType>::max(); }
		constexpr static DataStreamInstrumentData::ValueType HardwareResolution() noexcept { return std::numeric_limits<DataStreamInstrumentData::ValueType>::min(); }

		virtual DataStreamInstrumentData::ValueType GetHardwareMinValue() const noexcept override { return HardwareMinValue(); }
		virtual DataStreamInstrumentData::ValueType GetHardwareMaxValue() const noexcept override { return HardwareMaxValue(); }
		virtual DataStreamInstrumentData::ValueType GetHardwareResolution() const noexcept override final { return HardwareResolution(); }

		virtual Util::OptionalBool HasFinished() const override { return true; }

	private:
		void ResetImpl(dispatch_tag<AnalogIn>) override final;
		virtual void ResetImpl(dispatch_tag<DummyAnalogIn>) {}

		virtual std::unique_ptr<DynExp::InitTaskBase> MakeInitTask() const override { return DynExp::MakeTask<DummyAnalogInTasks::InitTask>(); }
		virtual std::unique_ptr<DynExp::ExitTaskBase> MakeExitTask() const override { return DynExp::MakeTask<DummyAnalogInTasks::ExitTask>(); }
		virtual std::unique_ptr<DynExp::UpdateTaskBase> MakeUpdateTask() const override { return DynExp::MakeTask<DummyAnalogInTasks::UpdateTask>(); }
	};
}