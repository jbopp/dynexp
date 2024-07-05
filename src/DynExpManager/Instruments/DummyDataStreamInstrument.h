// This file is part of DynExp.

/**
 * @file DummyDataStreamInstrument.h
 * @brief Implementation of a data stream instrument without any related physical hardware.
*/

#pragma once

#include "stdafx.h"
#include "DynExpCore.h"
#include "MetaInstruments/FunctionGenerator.h"

namespace DynExpInstr
{
	class DummyDataStreamInstrument;

	namespace DummyDataStreamInstrumentTasks
	{
		class InitTask : public FunctionGeneratorTasks::InitTask
		{
			void InitFuncImpl(dispatch_tag<FunctionGeneratorTasks::InitTask>, DynExp::InstrumentInstance& Instance) override final;
			virtual void InitFuncImpl(dispatch_tag<InitTask>, DynExp::InstrumentInstance& Instance) {}
		};

		class ExitTask : public FunctionGeneratorTasks::ExitTask
		{
			void ExitFuncImpl(dispatch_tag<FunctionGeneratorTasks::ExitTask>, DynExp::InstrumentInstance& Instance) override final { ExitFuncImpl(dispatch_tag<ExitTask>(), Instance); }
			virtual void ExitFuncImpl(dispatch_tag<ExitTask>, DynExp::InstrumentInstance& Instance) {}
		};

		class UpdateTask : public FunctionGeneratorTasks::UpdateTask
		{
			void UpdateFuncImpl(dispatch_tag<FunctionGeneratorTasks::UpdateTask>, DynExp::InstrumentInstance& Instance) override final { UpdateFuncImpl(dispatch_tag<UpdateTask>(), Instance); }
			virtual void UpdateFuncImpl(dispatch_tag<UpdateTask>, DynExp::InstrumentInstance& Instance) {}
		};

		class ResetStreamSizeTask : public DynExp::TaskBase
		{
		public:
			ResetStreamSizeTask(CallbackType CallbackFunc) noexcept : TaskBase(CallbackFunc) {}

		private:
			virtual DynExp::TaskResultType RunChild(DynExp::InstrumentInstance& Instance) override;
		};
	}

	class DummyDataStreamInstrumentData : public FunctionGeneratorData
	{
	public:
		DummyDataStreamInstrumentData(size_t BufferSizeInSamples = 0) : FunctionGeneratorData(std::make_unique<BasicSampleStream>(BufferSizeInSamples)) {}
		virtual ~DummyDataStreamInstrumentData() = default;

	private:
		void ResetImpl(dispatch_tag<FunctionGeneratorData>) override final;
		virtual void ResetImpl(dispatch_tag<DummyDataStreamInstrumentData>) {};
	};

	class DummyDataStreamInstrumentParams : public FunctionGeneratorParams
	{
	public:
		DummyDataStreamInstrumentParams(DynExp::ItemIDType ID, const DynExp::DynExpCore& Core)
			: FunctionGeneratorParams(ID, Core), StreamSizeParams(*this) {}
		virtual ~DummyDataStreamInstrumentParams() = default;

		virtual const char* GetParamClassTag() const noexcept override { return "DummyDataStreamInstrumentParams"; }

		// Stream size of the instrument's sample stream in samples.
		StreamSizeParamsExtension StreamSizeParams;

	private:
		void ConfigureParamsImpl(dispatch_tag<FunctionGeneratorParams>) override final { ConfigureParamsImpl(dispatch_tag<DummyDataStreamInstrumentParams>()); }
		virtual void ConfigureParamsImpl(dispatch_tag<DummyDataStreamInstrumentParams>) {}
	};

	class DummyDataStreamInstrumentConfigurator : public FunctionGeneratorConfigurator
	{
	public:
		using ObjectType = DummyDataStreamInstrument;
		using ParamsType = DummyDataStreamInstrumentParams;

		DummyDataStreamInstrumentConfigurator() = default;
		virtual ~DummyDataStreamInstrumentConfigurator() = default;

	private:
		virtual DynExp::ParamsBasePtrType MakeParams(DynExp::ItemIDType ID, const DynExp::DynExpCore& Core) const override { return DynExp::MakeParams<DummyDataStreamInstrumentConfigurator>(ID, Core); }
	};

	class DummyDataStreamInstrument : public FunctionGenerator
	{
	public:
		using ParamsType = DummyDataStreamInstrumentParams;
		using ConfigType = DummyDataStreamInstrumentConfigurator;
		using InstrumentDataType = DummyDataStreamInstrumentData;

		constexpr static auto Name() noexcept { return "Dummy Data Stream Instrument"; }

		DummyDataStreamInstrument(const std::thread::id OwnerThreadID, DynExp::ParamsBasePtrType&& Params);
		virtual ~DummyDataStreamInstrument() {}

		virtual std::string GetName() const override { return Name(); }

		virtual DataStreamInstrumentData::UnitType GetValueUnit() const noexcept override { return DataStreamInstrumentData::UnitType::Arbitrary; }

		// Override in order to suppress Util::NotImplementedException.
		virtual void WriteData(DynExp::TaskBase::CallbackType CallbackFunc = nullptr) const override { MakeAndEnqueueTask<DynExp::DefaultTask>(CallbackFunc); }

	private:
		void ResetImpl(dispatch_tag<FunctionGenerator>) override final;
		virtual void ResetImpl(dispatch_tag<DummyDataStreamInstrument>) {}

		virtual Util::FeatureTester<WaveformCapsType> GetWaveformCapsChild() const override { return std::array{ WaveformCapsType::UserDefined }; }

		virtual std::unique_ptr<DynExp::InitTaskBase> MakeInitTask() const override { return DynExp::MakeTask<DummyDataStreamInstrumentTasks::InitTask>(); }
		virtual std::unique_ptr<DynExp::ExitTaskBase> MakeExitTask() const override { return DynExp::MakeTask<DummyDataStreamInstrumentTasks::ExitTask>(); }
		virtual std::unique_ptr<DynExp::UpdateTaskBase> MakeUpdateTask() const override { return DynExp::MakeTask<DummyDataStreamInstrumentTasks::UpdateTask>(); }
		virtual void ResetStreamSize(DynExp::TaskBase::CallbackType CallbackFunc = nullptr) const override { MakeAndEnqueueTask<DummyDataStreamInstrumentTasks::ResetStreamSizeTask>(CallbackFunc); }
	};
}
