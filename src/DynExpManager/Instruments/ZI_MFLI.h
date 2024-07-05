// This file is part of DynExp.

/**
 * @file ZI_MFLI.h
 * @brief Implementation of an instrument to control the Zurich Instruments MFLI
 * lock-in amplifier.
*/

#pragma once

#include "stdafx.h"
#include "DynExpCore.h"
#include "HardwareAdapters/HardwareAdapterZILabOne.h"
#include "MetaInstruments/LockinAmplifier.h"

namespace DynExpInstr
{
	class ZI_MFLI;

	namespace ZI_MFLITasks
	{
		class ZI_MFLITaskBase
		{
		public:
			ZI_MFLITaskBase(DynExpHardware::ZILabOneHardwareAdapter::SignalInputType UsedSignalInput, uint8_t UsedDemodulator) noexcept
				: UsedSignalInput(UsedSignalInput), UsedDemodulator(UsedDemodulator) {}
			virtual ~ZI_MFLITaskBase() {}

			auto GetUsedSignalInput() const noexcept { return UsedSignalInput; }
			auto GetUsedDemodulator() const noexcept { return UsedDemodulator; }

		private:
			const DynExpHardware::ZILabOneHardwareAdapter::SignalInputType UsedSignalInput;
			const uint8_t UsedDemodulator;
		};

		class InitTask : public LockinAmplifierTasks::InitTask, ZI_MFLITaskBase
		{
		public:
			InitTask(DynExpHardware::ZILabOneHardwareAdapter::SignalInputType UsedSignalInput, uint8_t UsedDemodulator) noexcept :
				ZI_MFLITaskBase(UsedSignalInput, UsedDemodulator) {}

		private:
			void InitFuncImpl(dispatch_tag<LockinAmplifierTasks::InitTask>, DynExp::InstrumentInstance& Instance) override final;
			virtual void InitFuncImpl(dispatch_tag<InitTask>, DynExp::InstrumentInstance& Instance) {}
		};

		class ExitTask : public LockinAmplifierTasks::ExitTask, ZI_MFLITaskBase
		{
		public:
			ExitTask(DynExpHardware::ZILabOneHardwareAdapter::SignalInputType UsedSignalInput, uint8_t UsedDemodulator) noexcept :
				ZI_MFLITaskBase(UsedSignalInput, UsedDemodulator) {}

		private:
			void ExitFuncImpl(dispatch_tag<LockinAmplifierTasks::ExitTask>, DynExp::InstrumentInstance& Instance) override final;
			virtual void ExitFuncImpl(dispatch_tag<ExitTask>, DynExp::InstrumentInstance& Instance) {}
		};

		class UpdateTask : public LockinAmplifierTasks::UpdateTask, ZI_MFLITaskBase
		{
		public:
			UpdateTask(DynExpHardware::ZILabOneHardwareAdapter::SignalInputType UsedSignalInput, uint8_t UsedDemodulator) noexcept
				: ZI_MFLITaskBase(UsedSignalInput, UsedDemodulator) {}

		private:
			void UpdateFuncImpl(dispatch_tag<LockinAmplifierTasks::UpdateTask>, DynExp::InstrumentInstance& Instance) override final;
			virtual void UpdateFuncImpl(dispatch_tag<UpdateTask>, DynExp::InstrumentInstance& Instance) {}
		};

		class ReadTask final : public DynExp::TaskBase, ZI_MFLITaskBase
		{
		public:
			ReadTask(DynExpHardware::ZILabOneHardwareAdapter::SignalInputType UsedSignalInput, uint8_t UsedDemodulator, CallbackType CallbackFunc) noexcept
				: TaskBase(CallbackFunc), ZI_MFLITaskBase(UsedSignalInput, UsedDemodulator) {}

		private:
			virtual DynExp::TaskResultType RunChild(DynExp::InstrumentInstance& Instance) override;
		};

		class ClearDataTask final : public DynExp::TaskBase, ZI_MFLITaskBase
		{
		public:
			ClearDataTask(DynExpHardware::ZILabOneHardwareAdapter::SignalInputType UsedSignalInput, uint8_t UsedDemodulator, CallbackType CallbackFunc) noexcept
				: TaskBase(CallbackFunc), ZI_MFLITaskBase(UsedSignalInput, UsedDemodulator) {}

		private:
			virtual DynExp::TaskResultType RunChild(DynExp::InstrumentInstance& Instance) override;
		};

		class StartTask final : public DynExp::TaskBase, ZI_MFLITaskBase
		{
		public:
			StartTask(DynExpHardware::ZILabOneHardwareAdapter::SignalInputType UsedSignalInput, uint8_t UsedDemodulator, CallbackType CallbackFunc) noexcept
				: TaskBase(CallbackFunc), ZI_MFLITaskBase(UsedSignalInput, UsedDemodulator) {}

		private:
			virtual DynExp::TaskResultType RunChild(DynExp::InstrumentInstance& Instance) override;
		};

		class StopTask final : public DynExp::TaskBase, ZI_MFLITaskBase
		{
		public:
			StopTask(DynExpHardware::ZILabOneHardwareAdapter::SignalInputType UsedSignalInput, uint8_t UsedDemodulator, CallbackType CallbackFunc) noexcept
				: TaskBase(CallbackFunc), ZI_MFLITaskBase(UsedSignalInput, UsedDemodulator) {}

		private:
			virtual DynExp::TaskResultType RunChild(DynExp::InstrumentInstance& Instance) override;
		};

		class SetSensitivityTask final : public DynExp::TaskBase, ZI_MFLITaskBase
		{
		public:
			SetSensitivityTask(double Sensitivity,
				DynExpHardware::ZILabOneHardwareAdapter::SignalInputType UsedSignalInput, uint8_t UsedDemodulator, CallbackType CallbackFunc) noexcept
				: TaskBase(CallbackFunc), ZI_MFLITaskBase(UsedSignalInput, UsedDemodulator), Sensitivity(Sensitivity) {}

		private:
			virtual DynExp::TaskResultType RunChild(DynExp::InstrumentInstance& Instance) override;

			double Sensitivity;
		};

		class AutoAdjustSensitivityTask final : public DynExp::TaskBase, ZI_MFLITaskBase
		{
		public:
			AutoAdjustSensitivityTask(DynExpHardware::ZILabOneHardwareAdapter::SignalInputType UsedSignalInput, uint8_t UsedDemodulator, CallbackType CallbackFunc) noexcept
				: TaskBase(CallbackFunc), ZI_MFLITaskBase(UsedSignalInput, UsedDemodulator) {}

		private:
			virtual DynExp::TaskResultType RunChild(DynExp::InstrumentInstance& Instance) override;
		};

		class SetPhaseTask final : public DynExp::TaskBase, ZI_MFLITaskBase
		{
		public:
			SetPhaseTask(double Phase,
				DynExpHardware::ZILabOneHardwareAdapter::SignalInputType UsedSignalInput, uint8_t UsedDemodulator, CallbackType CallbackFunc) noexcept
				: TaskBase(CallbackFunc), ZI_MFLITaskBase(UsedSignalInput, UsedDemodulator), Phase(Phase) {}

		private:
			virtual DynExp::TaskResultType RunChild(DynExp::InstrumentInstance& Instance) override;

			double Phase;
		};

		class AutoAdjustPhaseTask final : public DynExp::TaskBase, ZI_MFLITaskBase
		{
		public:
			AutoAdjustPhaseTask(DynExpHardware::ZILabOneHardwareAdapter::SignalInputType UsedSignalInput, uint8_t UsedDemodulator, CallbackType CallbackFunc) noexcept
				: TaskBase(CallbackFunc), ZI_MFLITaskBase(UsedSignalInput, UsedDemodulator) {}

		private:
			virtual DynExp::TaskResultType RunChild(DynExp::InstrumentInstance& Instance) override;
		};

		class SetTimeConstantTask final : public DynExp::TaskBase, ZI_MFLITaskBase
		{
		public:
			SetTimeConstantTask(double TimeConstant,
				DynExpHardware::ZILabOneHardwareAdapter::SignalInputType UsedSignalInput, uint8_t UsedDemodulator, CallbackType CallbackFunc) noexcept
				: TaskBase(CallbackFunc), ZI_MFLITaskBase(UsedSignalInput, UsedDemodulator), TimeConstant(TimeConstant) {}

		private:
			virtual DynExp::TaskResultType RunChild(DynExp::InstrumentInstance& Instance) override;

			double TimeConstant;
		};

		class SetFilterOrderTask final : public DynExp::TaskBase, ZI_MFLITaskBase
		{
		public:
			SetFilterOrderTask(uint8_t FilterOrder,
				DynExpHardware::ZILabOneHardwareAdapter::SignalInputType UsedSignalInput, uint8_t UsedDemodulator, CallbackType CallbackFunc) noexcept
				: TaskBase(CallbackFunc), ZI_MFLITaskBase(UsedSignalInput, UsedDemodulator), FilterOrder(FilterOrder) {}

		private:
			virtual DynExp::TaskResultType RunChild(DynExp::InstrumentInstance& Instance) override;

			uint8_t FilterOrder;
		};

		class SetTriggerModeTask final : public DynExp::TaskBase, ZI_MFLITaskBase
		{
		public:
			SetTriggerModeTask(LockinAmplifierDefs::TriggerModeType TriggerMode, uint8_t TriggerChannel,
				DynExpHardware::ZILabOneHardwareAdapter::SignalInputType UsedSignalInput, uint8_t UsedDemodulator, CallbackType CallbackFunc) noexcept
				: TaskBase(CallbackFunc), ZI_MFLITaskBase(UsedSignalInput, UsedDemodulator), TriggerMode(TriggerMode), TriggerChannel(TriggerChannel) {}

		private:
			virtual DynExp::TaskResultType RunChild(DynExp::InstrumentInstance& Instance) override;

			LockinAmplifierDefs::TriggerModeType TriggerMode;
			uint8_t TriggerChannel;
		};

		class SetTriggerEdgeTask final : public DynExp::TaskBase, ZI_MFLITaskBase
		{
		public:
			SetTriggerEdgeTask(LockinAmplifierDefs::TriggerEdgeType TriggerEdge,
				DynExpHardware::ZILabOneHardwareAdapter::SignalInputType UsedSignalInput, uint8_t UsedDemodulator, CallbackType CallbackFunc) noexcept
				: TaskBase(CallbackFunc), ZI_MFLITaskBase(UsedSignalInput, UsedDemodulator), TriggerEdge(TriggerEdge) {}

		private:
			virtual DynExp::TaskResultType RunChild(DynExp::InstrumentInstance& Instance) override;

			LockinAmplifierDefs::TriggerEdgeType TriggerEdge;
		};

		class SetSignalTypeTask final : public DynExp::TaskBase, ZI_MFLITaskBase
		{
		public:
			SetSignalTypeTask(LockinAmplifierDefs::SignalType SignalType,
				DynExpHardware::ZILabOneHardwareAdapter::SignalInputType UsedSignalInput, uint8_t UsedDemodulator, CallbackType CallbackFunc) noexcept
				: TaskBase(CallbackFunc), ZI_MFLITaskBase(UsedSignalInput, UsedDemodulator), SignalType(SignalType) {}

		private:
			virtual DynExp::TaskResultType RunChild(DynExp::InstrumentInstance& Instance) override;

			LockinAmplifierDefs::SignalType SignalType;
		};

		class SetSamplingRateTask final : public DynExp::TaskBase, ZI_MFLITaskBase
		{
		public:
			SetSamplingRateTask(double SamplingRate,
				DynExpHardware::ZILabOneHardwareAdapter::SignalInputType UsedSignalInput, uint8_t UsedDemodulator, CallbackType CallbackFunc) noexcept
				: TaskBase(CallbackFunc), ZI_MFLITaskBase(UsedSignalInput, UsedDemodulator), SamplingRate(SamplingRate) {}

		private:
			virtual DynExp::TaskResultType RunChild(DynExp::InstrumentInstance& Instance) override;

			double SamplingRate;
		};

		class SetEnableTask final : public DynExp::TaskBase, ZI_MFLITaskBase
		{
		public:
			SetEnableTask(bool Enable,
				DynExpHardware::ZILabOneHardwareAdapter::SignalInputType UsedSignalInput, uint8_t UsedDemodulator, CallbackType CallbackFunc) noexcept
				: TaskBase(CallbackFunc), ZI_MFLITaskBase(UsedSignalInput, UsedDemodulator), Enable(Enable) {}

		private:
			virtual DynExp::TaskResultType RunChild(DynExp::InstrumentInstance& Instance) override;

			bool Enable;
		};

		class ForceTriggerTask final : public DynExp::TaskBase, ZI_MFLITaskBase
		{
		public:
			ForceTriggerTask(DynExpHardware::ZILabOneHardwareAdapter::SignalInputType UsedSignalInput, uint8_t UsedDemodulator, CallbackType CallbackFunc) noexcept
				: TaskBase(CallbackFunc), ZI_MFLITaskBase(UsedSignalInput, UsedDemodulator) {}

		private:
			virtual DynExp::TaskResultType RunChild(DynExp::InstrumentInstance& Instance) override;
		};
	}

	class ZI_MFLIData : public LockinAmplifierData
	{
		friend class ZI_MFLITasks::UpdateTask;

	public:
		ZI_MFLIData(size_t BufferSizeInSamples = 1) : LockinAmplifierData(BufferSizeInSamples) {}
		virtual ~ZI_MFLIData() = default;

		virtual double GetSensitivity() const noexcept override { return Sensitivity; }
		virtual void SetSensitivity(double Sensitivity) noexcept override { this->Sensitivity = Sensitivity; }
		virtual double GetPhase() const noexcept override { return Phase; }
		virtual void SetPhase(double Phase) noexcept override { this->Phase = Phase; }
		virtual double GetTimeConstant() const noexcept override { return TimeConstant; }
		virtual void SetTimeConstant(double TimeConstant) noexcept override { this->TimeConstant = TimeConstant; }
		virtual uint8_t GetFilterOrder() const noexcept override { return FilterOrder; }
		virtual void SetFilterOrder(uint8_t FilterOrder) noexcept override { this->FilterOrder = FilterOrder; }
		virtual LockinAmplifierDefs::TriggerModeType GetTriggerMode() const noexcept override { return TriggerMode; }
		virtual void SetTriggerMode(LockinAmplifierDefs::TriggerModeType TriggerMode) noexcept override { this->TriggerMode = TriggerMode; }
		virtual LockinAmplifierDefs::TriggerEdgeType GetTriggerEdge() const noexcept override { return TriggerEdge; }
		virtual void SetTriggerEdge(LockinAmplifierDefs::TriggerEdgeType TriggerEdge) noexcept override { this->TriggerEdge = TriggerEdge; }
		virtual LockinAmplifierDefs::SignalType GetSignalType() const noexcept override { return Signal; }
		virtual void SetSignalType(LockinAmplifierDefs::SignalType SignalType) noexcept override { Signal = SignalType; }
		virtual double GetSamplingRate() const noexcept override { return SamplingRate; }
		virtual void SetSamplingRate(double SamplingRate) noexcept override { this->SamplingRate = SamplingRate; }
		virtual bool IsEnabled() const noexcept override { return Enable; }
		virtual void SetEnable(uint8_t Enable) noexcept override { this->Enable = Enable; }

		virtual bool IsOverloaded() const noexcept override { return Overload; }
		virtual std::pair<double, double> GetInputLoad() const noexcept override { return std::make_pair(NegInputLoad, PosInputLoad); }
		virtual double GetOscillatorFrequency() const noexcept override { return OscillatorFrequency; }
		virtual double GetAcquisitionProgress() const noexcept override { return AcquisitionProgress; }

		DynExp::LinkedObjectWrapperContainer<DynExpHardware::ZILabOneHardwareAdapter> HardwareAdapter;

	private:
		void ResetImpl(dispatch_tag<LockinAmplifierData>) override final;
		virtual void ResetImpl(dispatch_tag<ZI_MFLIData>) {};

		// Reflecting parameters
		double Sensitivity{};
		double Phase{};			// in rad
		double TimeConstant{};	// in s
		uint8_t FilterOrder{};
		LockinAmplifierDefs::TriggerModeType TriggerMode{};
		LockinAmplifierDefs::TriggerEdgeType TriggerEdge{};
		LockinAmplifierDefs::SignalType Signal{};
		double SamplingRate{};	// in samples/s
		bool Enable{};

		// Other
		bool Overload{};
		double NegInputLoad{};
		double PosInputLoad{};
		double OscillatorFrequency{};
		double AcquisitionProgress{};

		size_t NumFailedStatusUpdateAttempts = 0;
	};

	class ZI_MFLIParams : public LockinAmplifierParams
	{
	public:
		static Util::TextValueListType<DynExpHardware::ZILabOneHardwareAdapter::SignalInputType> SignalInputTypeStrList();

		ZI_MFLIParams(DynExp::ItemIDType ID, const DynExp::DynExpCore& Core) : LockinAmplifierParams(ID, Core) {}
		virtual ~ZI_MFLIParams() = default;

		virtual const char* GetParamClassTag() const noexcept override { return "ZI_MFLIParams"; }

		Param<DynExp::ObjectLink<DynExpHardware::ZILabOneHardwareAdapter>> HardwareAdapter = { *this, GetCore().GetHardwareAdapterManager(),
			"HardwareAdapter", "ZI LabOne instance", "Underlying hardware adapter of this instrument", DynExpUI::Icons::HardwareAdapter };
		Param<DynExpHardware::ZILabOneHardwareAdapter::SignalInputType> SignalInput = {
			*this, SignalInputTypeStrList(), "SignalInput", "Input signal type",
			"Determines which signal input to use", true, DynExpHardware::ZILabOneHardwareAdapter::SignalInputType::Voltage };
		Param<ParamsConfigDialog::NumberType> Demodulator = { *this, "Demodulator", "Demodulator",
			"Demodulator of the lock-in amplifier to read out", true, 0, 0, std::numeric_limits<uint8_t>::max(), 1, 0 };
		Param<ParamsConfigDialog::NumberType> TriggerChannel = { *this, "TriggerChannel", "Trigger channel",
			"Trigger channel to use for external trigger modes", true, 1, 1, std::numeric_limits<uint8_t>::max(), 1, 0 };

	private:
		void ConfigureParamsImpl(dispatch_tag<LockinAmplifierParams>) override final { ConfigureParamsImpl(dispatch_tag<ZI_MFLIParams>()); }
		virtual void ConfigureParamsImpl(dispatch_tag<ZI_MFLIParams>) {}
	};

	class ZI_MFLIConfigurator : public LockinAmplifierConfigurator
	{
	public:
		using ObjectType = ZI_MFLI;
		using ParamsType = ZI_MFLIParams;

		ZI_MFLIConfigurator() = default;
		virtual ~ZI_MFLIConfigurator() = default;

	private:
		virtual DynExp::ParamsBasePtrType MakeParams(DynExp::ItemIDType ID, const DynExp::DynExpCore& Core) const override { return DynExp::MakeParams<ZI_MFLIConfigurator>(ID, Core); }
	};

	class ZI_MFLI : public LockinAmplifier
	{
	public:
		using ParamsType = ZI_MFLIParams;
		using ConfigType = ZI_MFLIConfigurator;
		using InstrumentDataType = ZI_MFLIData;

		constexpr static auto Name() noexcept { return "ZI MFLI"; }

		ZI_MFLI(const std::thread::id OwnerThreadID, DynExp::ParamsBasePtrType&& Params);
		virtual ~ZI_MFLI() {}

		virtual std::string GetName() const override { return Name(); }

		// ZI MFLI does not like polling data too frequently...
		virtual std::chrono::milliseconds GetTaskQueueDelay() const override { return std::chrono::milliseconds(200); }

		// Further information about the instrument
		virtual DataStreamInstrumentData::UnitType GetValueUnit() const noexcept override { return SignalInput == DynExpHardware::ZILabOneHardwareAdapter::SignalInputType::Current ? DataStreamInstrumentData::UnitType::Ampere : DataStreamInstrumentData::UnitType::Volt; }
		virtual const char* GetSensitivityUnitString() const noexcept override { return SignalInput == DynExpHardware::ZILabOneHardwareAdapter::SignalInputType::Current ? "A" : "V"; }

		virtual Util::OptionalBool HasFinished() const override;
		virtual Util::OptionalBool IsRunning() const override;

		// Tasks
		virtual void ReadData(DynExp::TaskBase::CallbackType CallbackFunc = nullptr) const override { MakeAndEnqueueTask<ZI_MFLITasks::ReadTask>(SignalInput, UsedDemodulator, CallbackFunc); }
		virtual void ClearData(DynExp::TaskBase::CallbackType CallbackFunc = nullptr) const override { MakeAndEnqueueTask<ZI_MFLITasks::ClearDataTask>(SignalInput, UsedDemodulator, CallbackFunc); }
		virtual void Start(DynExp::TaskBase::CallbackType CallbackFunc = nullptr) const override { MakeAndEnqueueTask<ZI_MFLITasks::StartTask>(SignalInput, UsedDemodulator, CallbackFunc); }
		virtual void Stop(DynExp::TaskBase::CallbackType CallbackFunc = nullptr) const override { MakeAndEnqueueTask<ZI_MFLITasks::StopTask>(SignalInput, UsedDemodulator, CallbackFunc); }

		virtual void SetSensitivity(double Sensitivity, DynExp::TaskBase::CallbackType CallbackFunc = nullptr) const override { MakeAndEnqueueTask<ZI_MFLITasks::SetSensitivityTask>(Sensitivity, SignalInput, UsedDemodulator, CallbackFunc); }
		virtual void AutoAdjustSensitivity(DynExp::TaskBase::CallbackType CallbackFunc = nullptr) const override { MakeAndEnqueueTask<ZI_MFLITasks::AutoAdjustSensitivityTask>(SignalInput, UsedDemodulator, CallbackFunc); }
		virtual void SetPhase(double Phase, DynExp::TaskBase::CallbackType CallbackFunc = nullptr) const override { MakeAndEnqueueTask<ZI_MFLITasks::SetPhaseTask>(Phase, SignalInput, UsedDemodulator, CallbackFunc); }
		virtual void AutoAdjustPhase(DynExp::TaskBase::CallbackType CallbackFunc = nullptr) const override { MakeAndEnqueueTask<ZI_MFLITasks::AutoAdjustPhaseTask>(SignalInput, UsedDemodulator, CallbackFunc); }
		virtual void SetTimeConstant(double TimeConstant, DynExp::TaskBase::CallbackType CallbackFunc = nullptr) const override { MakeAndEnqueueTask<ZI_MFLITasks::SetTimeConstantTask>(TimeConstant, SignalInput, UsedDemodulator, CallbackFunc); }
		virtual void SetFilterOrder(uint8_t FilterOrder, DynExp::TaskBase::CallbackType CallbackFunc = nullptr) const override { MakeAndEnqueueTask<ZI_MFLITasks::SetFilterOrderTask>(FilterOrder, SignalInput, UsedDemodulator, CallbackFunc); }
		virtual void SetTriggerMode(LockinAmplifierDefs::TriggerModeType TriggerMode, DynExp::TaskBase::CallbackType CallbackFunc = nullptr) const override { MakeAndEnqueueTask<ZI_MFLITasks::SetTriggerModeTask>(TriggerMode, TriggerChannel, SignalInput, UsedDemodulator, CallbackFunc); }
		virtual void SetTriggerEdge(LockinAmplifierDefs::TriggerEdgeType TriggerEdge, DynExp::TaskBase::CallbackType CallbackFunc = nullptr) const override { MakeAndEnqueueTask<ZI_MFLITasks::SetTriggerEdgeTask>(TriggerEdge, SignalInput, UsedDemodulator, CallbackFunc); }
		virtual void SetSignalType(LockinAmplifierDefs::SignalType SignalType, DynExp::TaskBase::CallbackType CallbackFunc = nullptr) const override { MakeAndEnqueueTask<ZI_MFLITasks::SetSignalTypeTask>(SignalType, SignalInput, UsedDemodulator, CallbackFunc); }
		virtual void SetSamplingRate(double SamplingRate, DynExp::TaskBase::CallbackType CallbackFunc = nullptr) const override { MakeAndEnqueueTask<ZI_MFLITasks::SetSamplingRateTask>(SamplingRate, SignalInput, UsedDemodulator, CallbackFunc); }
		virtual void SetEnable(bool Enable, DynExp::TaskBase::CallbackType CallbackFunc = nullptr) const override { MakeAndEnqueueTask<ZI_MFLITasks::SetEnableTask>(Enable, SignalInput, UsedDemodulator, CallbackFunc); }
		virtual void ForceTrigger(DynExp::TaskBase::CallbackType CallbackFunc = nullptr) const override { MakeAndEnqueueTask<ZI_MFLITasks::ForceTriggerTask>(SignalInput, UsedDemodulator, CallbackFunc); }

	private:
		void ResetImpl(dispatch_tag<LockinAmplifier>) override final;
		virtual void ResetImpl(dispatch_tag<ZI_MFLI>) {}

		virtual void ApplyFromParamsImpl(dispatch_tag<LockinAmplifier>) const override;

		virtual std::unique_ptr<DynExp::InitTaskBase> MakeInitTask() const override { return DynExp::MakeTask<ZI_MFLITasks::InitTask>(SignalInput, UsedDemodulator); }
		virtual std::unique_ptr<DynExp::ExitTaskBase> MakeExitTask() const override { return DynExp::MakeTask<ZI_MFLITasks::ExitTask>(SignalInput, UsedDemodulator); }
		virtual std::unique_ptr<DynExp::UpdateTaskBase> MakeUpdateTask() const override { return DynExp::MakeTask<ZI_MFLITasks::UpdateTask>(SignalInput, UsedDemodulator); }

		// atomic since used in GetValueUnit().
		std::atomic<DynExpHardware::ZILabOneHardwareAdapter::SignalInputType> SignalInput;

		uint8_t UsedDemodulator;
		uint8_t TriggerChannel;
	};
}