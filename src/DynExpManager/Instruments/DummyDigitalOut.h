// This file is part of DynExp.

/**
 * @file DummyDigitalOut.h
 * @brief Implementation of a digital output port instrument without any related physical hardware.
*/

#pragma once

#include "stdafx.h"
#include "DynExpCore.h"
#include "MetaInstruments/DigitalOut.h"

namespace DynExpInstr
{
	class DummyDigitalOut;

	namespace DummyDigitalOutTasks
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
	}

	class DummyDigitalOutData : public DigitalOutData
	{

	public:
		DummyDigitalOutData() = default;
		virtual ~DummyDigitalOutData() = default;

	private:
		void ResetImpl(dispatch_tag<DigitalOutData>) override final;
		virtual void ResetImpl(dispatch_tag<DummyDigitalOutData>) {};
	};

	class DummyDigitalOutParams : public DigitalOutParams
	{
	public:
		DummyDigitalOutParams(DynExp::ItemIDType ID, const DynExp::DynExpCore& Core) : DigitalOutParams(ID, Core) {}
		virtual ~DummyDigitalOutParams() = default;

		virtual const char* GetParamClassTag() const noexcept override { return "DummyDigitalOutParams"; }

	private:
		void ConfigureParamsImpl(dispatch_tag<DigitalOutParams>) override final { ConfigureParamsImpl(dispatch_tag<DummyDigitalOutParams>()); }
		virtual void ConfigureParamsImpl(dispatch_tag<DummyDigitalOutParams>) {}

		DummyParam Dummy = { *this };
	};

	class DummyDigitalOutConfigurator : public DigitalOutConfigurator
	{
	public:
		using ObjectType = DummyDigitalOut;
		using ParamsType = DummyDigitalOutParams;

		DummyDigitalOutConfigurator() = default;
		virtual ~DummyDigitalOutConfigurator() = default;

	private:
		virtual DynExp::ParamsBasePtrType MakeParams(DynExp::ItemIDType ID, const DynExp::DynExpCore& Core) const override { return DynExp::MakeParams<DummyDigitalOutConfigurator>(ID, Core); }
	};

	class DummyDigitalOut : public DigitalOut
	{
	public:
		using ParamsType = DummyDigitalOutParams;
		using ConfigType = DummyDigitalOutConfigurator;
		using InstrumentDataType = DummyDigitalOutData;

		constexpr static auto Name() noexcept { return "Dummy Digital Out"; }

		DummyDigitalOut(const std::thread::id OwnerThreadID, DynExp::ParamsBasePtrType&& Params);
		virtual ~DummyDigitalOut() {}

		virtual std::string GetName() const override { return Name(); }

		virtual void WriteData(DynExp::TaskBase::CallbackType CallbackFunc) const override {}
		virtual Util::OptionalBool HasFinished() const override { return true; }

	private:
		void ResetImpl(dispatch_tag<DigitalOut>) override final;
		virtual void ResetImpl(dispatch_tag<DummyDigitalOut>) {}

		virtual Util::FeatureTester<WaveformCapsType> GetWaveformCapsChild() const override { return std::array{ WaveformCapsType::UserDefined }; }

		virtual std::unique_ptr<DynExp::InitTaskBase> MakeInitTask() const override { return DynExp::MakeTask<DummyDigitalOutTasks::InitTask>(); }
		virtual std::unique_ptr<DynExp::ExitTaskBase> MakeExitTask() const override { return DynExp::MakeTask<DummyDigitalOutTasks::ExitTask>(); }
		virtual std::unique_ptr<DynExp::UpdateTaskBase> MakeUpdateTask() const override { return DynExp::MakeTask<DummyDigitalOutTasks::UpdateTask>(); }
	};
}