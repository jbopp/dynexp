// This file is part of DynExp.

/**
 * @file DummyDigitalIn.h
 * @brief Implementation of a digital input port instrument without any related physical hardware.
*/

#pragma once

#include "stdafx.h"
#include "DynExpCore.h"
#include "MetaInstruments/DigitalIn.h"

namespace DynExpInstr
{
	class DummyDigitalIn;

	namespace DummyDigitalInTasks
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
	}

	class DummyDigitalInData : public DigitalInData
	{

	public:
		DummyDigitalInData() = default;
		virtual ~DummyDigitalInData() = default;

	private:
		void ResetImpl(dispatch_tag<DigitalInData>) override final;
		virtual void ResetImpl(dispatch_tag<DummyDigitalInData>) {};
	};

	class DummyDigitalInParams : public DigitalInParams
	{
	public:
		DummyDigitalInParams(DynExp::ItemIDType ID, const DynExp::DynExpCore& Core) : DigitalInParams(ID, Core) {}
		virtual ~DummyDigitalInParams() = default;

		virtual const char* GetParamClassTag() const noexcept override { return "DummyDigitalInParams"; }

	private:
		void ConfigureParamsImpl(dispatch_tag<DigitalInParams>) override final { ConfigureParamsImpl(dispatch_tag<DummyDigitalInParams>()); }
		virtual void ConfigureParamsImpl(dispatch_tag<DummyDigitalInParams>) {}

		DummyParam Dummy = { *this };
	};

	class DummyDigitalInConfigurator : public DigitalInConfigurator
	{
	public:
		using ObjectType = DummyDigitalIn;
		using ParamsType = DummyDigitalInParams;

		DummyDigitalInConfigurator() = default;
		virtual ~DummyDigitalInConfigurator() = default;

	private:
		virtual DynExp::ParamsBasePtrType MakeParams(DynExp::ItemIDType ID, const DynExp::DynExpCore& Core) const override { return DynExp::MakeParams<DummyDigitalInConfigurator>(ID, Core); }
	};

	class DummyDigitalIn : public DigitalIn
	{
	public:
		using ParamsType = DummyDigitalInParams;
		using ConfigType = DummyDigitalInConfigurator;
		using InstrumentDataType = DummyDigitalInData;

		constexpr static auto Name() noexcept { return "Dummy Digital In"; }

		DummyDigitalIn(const std::thread::id OwnerThreadID, DynExp::ParamsBasePtrType&& Params);
		virtual ~DummyDigitalIn() {}

		virtual std::string GetName() const override { return Name(); }

		virtual Util::OptionalBool HasFinished() const override { return true; }

	private:
		void ResetImpl(dispatch_tag<DigitalIn>) override final;
		virtual void ResetImpl(dispatch_tag<DummyDigitalIn>) {}

		virtual std::unique_ptr<DynExp::InitTaskBase> MakeInitTask() const override { return DynExp::MakeTask<DummyDigitalInTasks::InitTask>(); }
		virtual std::unique_ptr<DynExp::ExitTaskBase> MakeExitTask() const override { return DynExp::MakeTask<DummyDigitalInTasks::ExitTask>(); }
		virtual std::unique_ptr<DynExp::UpdateTaskBase> MakeUpdateTask() const override { return DynExp::MakeTask<DummyDigitalInTasks::UpdateTask>(); }
	};
}