// This file is part of DynExp.

/**
 * @file RS_SMC100A.h
 * @brief Implementation of an instrument to control the Rohde & Schwarz SMC100A
 * function generator.
*/

#pragma once

#include "stdafx.h"
#include "DynExpCore.h"
#include "MetaInstruments/FunctionGenerator.h"

namespace DynExpInstr
{
	class RS_SMC100A;

	namespace RS_SMC100ATasks
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
			void UpdateFuncImpl(dispatch_tag<FunctionGeneratorTasks::UpdateTask>, DynExp::InstrumentInstance& Instance) override final;
			virtual void UpdateFuncImpl(dispatch_tag<UpdateTask>, DynExp::InstrumentInstance& Instance) {}
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

		class SetSineFunctionTask : public DynExp::TaskBase
		{
		public:
			SetSineFunctionTask(const FunctionGeneratorDefs::SineFunctionDescType& FunctionDesc, bool Autostart, CallbackType CallbackFunc) noexcept
				: TaskBase(CallbackFunc), FunctionDesc(FunctionDesc), Autostart(Autostart) {}

		private:
			virtual DynExp::TaskResultType RunChild(DynExp::InstrumentInstance& Instance) override;

			FunctionGeneratorDefs::SineFunctionDescType FunctionDesc;
			bool Autostart;
		};

		class SetModulationTask : public DynExp::TaskBase
		{
		public:
			SetModulationTask(const FunctionGeneratorDefs::ModulationDescType& ModulationDesc, CallbackType CallbackFunc) noexcept
				: TaskBase(CallbackFunc), ModulationDesc(ModulationDesc) {}

		private:
			virtual DynExp::TaskResultType RunChild(DynExp::InstrumentInstance& Instance) override;

			FunctionGeneratorDefs::ModulationDescType ModulationDesc;
		};

		class SetSweepTask : public DynExp::TaskBase
		{
		public:
			SetSweepTask(const FunctionGeneratorDefs::SweepDescType& SweepDesc, CallbackType CallbackFunc) noexcept
				: TaskBase(CallbackFunc), SweepDesc(SweepDesc) {}

		private:
			virtual DynExp::TaskResultType RunChild(DynExp::InstrumentInstance& Instance) override;

			FunctionGeneratorDefs::SweepDescType SweepDesc;
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
	}

	class RS_SMC100AData : public FunctionGeneratorData
	{
		friend class RS_SMC100ATasks::UpdateTask;
		friend class RS_SMC100ATasks::SetSweepTask;

	public:
		RS_SMC100AData(size_t BufferSizeInSamples = 0) : FunctionGeneratorData(std::make_unique<BasicSampleStream>(BufferSizeInSamples)) {}
		virtual ~RS_SMC100AData() = default;

		bool IsRunning() const noexcept { return Running; }
		auto GetCurrentSweepType() const noexcept { return CurrentSweepType; }

		DynExp::LinkedObjectWrapperContainer<DynExp::SerialCommunicationHardwareAdapter> HardwareAdapter;

	private:
		void ResetImpl(dispatch_tag<FunctionGeneratorData>) override final;
		virtual void ResetImpl(dispatch_tag<RS_SMC100AData>) {};

		size_t NumFailedStatusUpdateAttempts = 0;
		bool Running = false;
		FunctionGeneratorDefs::SweepDescType::SweepType CurrentSweepType = FunctionGeneratorDefs::SweepDescType::SweepType::Disabled;
	};

	class RS_SMC100AParams : public FunctionGeneratorParams
	{
	public:
		RS_SMC100AParams(DynExp::ItemIDType ID, const DynExp::DynExpCore& Core) : FunctionGeneratorParams(ID, Core) {}
		virtual ~RS_SMC100AParams() = default;

		virtual const char* GetParamClassTag() const noexcept override { return "RS_SMC100AParams"; }

		Param<DynExp::ObjectLink<DynExp::SerialCommunicationHardwareAdapter>> HardwareAdapter = { *this, GetCore().GetHardwareAdapterManager(),
			"HardwareAdapter", "R&S SMC100A controller", "Underlying hardware adapter of this instrument", DynExpUI::Icons::HardwareAdapter };

	private:
		void ConfigureParamsImpl(dispatch_tag<FunctionGeneratorParams>) override final { ConfigureParamsImpl(dispatch_tag<RS_SMC100AParams>()); }
		virtual void ConfigureParamsImpl(dispatch_tag<RS_SMC100AParams>) {}
	};

	class RS_SMC100AConfigurator : public FunctionGeneratorConfigurator
	{
	public:
		using ObjectType = RS_SMC100A;
		using ParamsType = RS_SMC100AParams;

		RS_SMC100AConfigurator() = default;
		virtual ~RS_SMC100AConfigurator() = default;

	private:
		virtual DynExp::ParamsBasePtrType MakeParams(DynExp::ItemIDType ID, const DynExp::DynExpCore& Core) const override { return DynExp::MakeParams<RS_SMC100AConfigurator>(ID, Core); }
	};

	class RS_SMC100A : public FunctionGenerator
	{
	public:
		using ParamsType = RS_SMC100AParams;
		using ConfigType = RS_SMC100AConfigurator;
		using InstrumentDataType = RS_SMC100AData;

		static const char* TriggerModeToCmdString(FunctionGeneratorDefs::TriggerDescType::TriggerModeType TriggerMode) noexcept;

		constexpr static auto Name() noexcept { return "R&S SMC100A"; }

		RS_SMC100A(const std::thread::id OwnerThreadID, DynExp::ParamsBasePtrType&& Params);
		virtual ~RS_SMC100A() {}

		virtual std::string GetName() const override { return Name(); }

		virtual std::chrono::milliseconds GetTaskQueueDelay() const override { return std::chrono::milliseconds(100); }
		virtual DataStreamInstrumentData::UnitType GetValueUnit() const noexcept override { return DataStreamInstrumentData::UnitType::Power_dBm; }
		virtual bool IsPhaseAdjustable() const noexcept override { return false; }

		// Tasks
		virtual void Start(DynExp::TaskBase::CallbackType CallbackFunc = nullptr) const override { MakeAndEnqueueTask<RS_SMC100ATasks::StartTask>(CallbackFunc); }
		virtual void Stop(DynExp::TaskBase::CallbackType CallbackFunc = nullptr) const override { MakeAndEnqueueTask<RS_SMC100ATasks::StopTask>(CallbackFunc); }

		virtual void SetSineFunction(const FunctionGeneratorDefs::SineFunctionDescType& FunctionDesc,
			bool PersistParams = false, bool Autostart = false, DynExp::TaskBase::CallbackType CallbackFunc = nullptr) const override;
		virtual void SetModulation(const FunctionGeneratorDefs::ModulationDescType& ModulationDesc,
			bool PersistParams = false, DynExp::TaskBase::CallbackType CallbackFunc = nullptr) const override;
		virtual void SetSweep(const FunctionGeneratorDefs::SweepDescType& SweepDesc,
			bool PersistParams = false, DynExp::TaskBase::CallbackType CallbackFunc = nullptr) const override;
		virtual void ForceTrigger(DynExp::TaskBase::CallbackType CallbackFunc = nullptr) const override;

		// Functions giving information about the instrument.
		virtual Util::OptionalBool IsRunning() const override;

	private:
		void ResetImpl(dispatch_tag<FunctionGenerator>) override final;
		virtual void ResetImpl(dispatch_tag<RS_SMC100A>) {}

		virtual FunctionGeneratorDefs::FunctionDescType GetMinCapsChild() const override { return { 8e3, -120, 0 }; }
		virtual FunctionGeneratorDefs::FunctionDescType GetMaxCapsChild() const override { return { 3.2e9, 19, 0}; }
		virtual FunctionGeneratorDefs::FunctionDescType GetParamDefaultsChild() const override { return { 2.87e9, -10, 0}; }
		virtual Util::FeatureTester<WaveformCapsType> GetWaveformCapsChild() const override { return std::array{ WaveformCapsType::Sine }; }
		virtual Util::FeatureTester<QuantityCapsType> GetModulationCapsChild() const override { return std::array{ QuantityCapsType::Amplitude, QuantityCapsType::Frequency, QuantityCapsType::Phase }; }
		virtual Util::FeatureTester<QuantityCapsType> GetSweepCapsChild() const override { return std::array{ QuantityCapsType::Amplitude, QuantityCapsType::Frequency }; }
		virtual Util::FeatureTester<TriggerCapsType> GetTriggerCapsChild() const override { return std::array{ TriggerCapsType::CanConfigure, TriggerCapsType::CanForce }; }

		virtual void SetTriggerChild(const FunctionGeneratorDefs::TriggerDescType& TriggerDesc,
			bool PersistParams, DynExp::TaskBase::CallbackType CallbackFunc) const override;

		virtual std::unique_ptr<DynExp::InitTaskBase> MakeInitTask() const override { return DynExp::MakeTask<RS_SMC100ATasks::InitTask>(); }
		virtual std::unique_ptr<DynExp::ExitTaskBase> MakeExitTask() const override { return DynExp::MakeTask<RS_SMC100ATasks::ExitTask>(); }
		virtual std::unique_ptr<DynExp::UpdateTaskBase> MakeUpdateTask() const override { return DynExp::MakeTask<RS_SMC100ATasks::UpdateTask>(); }
	};
}
