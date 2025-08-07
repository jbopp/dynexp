// This file is part of DynExp.

/**
 * @file NetworkLaser.h
 * @brief Implementation of a gRPC client instrument to access a remote Laser meta instrument.
*/

#pragma once

#include "stdafx.h"
#include "DynExpCore.h"
#include "MetaInstruments/Laser.h"
#include "MetaInstruments/gRPCInstrument.h"

#include "NetworkLaser.pb.h"
#include "NetworkLaser.grpc.pb.h"

namespace DynExpInstr
{
	class NetworkLaser;

	constexpr DynExpProto::Common::FrequencyUnitType ToPrototUnitType(LaserData::FrequencyUnitType Unit)
	{
		switch (Unit)
		{
		case LaserData::FrequencyUnitType::Hz: return DynExpProto::Common::FrequencyUnitType::Hz;
		case LaserData::FrequencyUnitType::nm: return DynExpProto::Common::FrequencyUnitType::nm;
		default: throw Util::InvalidDataException("The given unit does not exist in the LaserData::FrequencyUnitType enumeration. Did you forget to adjust the FrequencyUnitType enumeration in class \"LaserData\"?");
		}
	}

	constexpr LaserData::FrequencyUnitType ToLaserUnitType(DynExpProto::Common::FrequencyUnitType Unit)
	{
		switch (Unit)
		{
		case DynExpProto::Common::FrequencyUnitType::Hz: return LaserData::FrequencyUnitType::Hz;
		case DynExpProto::Common::FrequencyUnitType::nm: return LaserData::FrequencyUnitType::nm;
		default: throw Util::InvalidDataException("The given unit does not exist in the DynExpProto::Common::FrequencyUnitType enumeration. Did you forget to adjust the FrequencyUnitType enumeration in file \"Common.proto\"?");
		}
	}

	constexpr DynExpProto::Common::UnitType ToPrototUnitType(LaserData::IntensityUnitType Unit)
	{
		switch (Unit)
		{
		case LaserData::IntensityUnitType::Power_W: return DynExpProto::Common::UnitType::Power_W;
		default: throw Util::InvalidDataException("The given unit does not exist in the LaserData::IntensityUnitType enumeration. Did you forget to adjust the IntensityUnitType enumeration in class \"LaserData\"?");
		}
	}

	constexpr LaserData::IntensityUnitType ToLaserUnitType(DynExpProto::Common::UnitType Unit)
	{
		switch (Unit)
		{
		case DynExpProto::Common::UnitType::Power_W: return LaserData::IntensityUnitType::Power_W;
		default: throw Util::InvalidDataException("The given unit is not defined in Laser.h. Please add to enumeration in Laser.h or use Power_W.");
		}
	}

	constexpr LaserData::LaserStateType ToLaserStateType(DynExpProto::NetworkLaser::StateType State)
	{
		switch (State)
		{
		case DynExpProto::NetworkLaser::StateType::Ready: return LaserData::LaserStateType::Ready;
		case DynExpProto::NetworkLaser::StateType::Startup: return LaserData::LaserStateType::Startup;
		case DynExpProto::NetworkLaser::StateType::EmissionEnabledConstant: return LaserData::LaserStateType::EmissionEnabledConstant;
		case DynExpProto::NetworkLaser::StateType::EmissionEnabledScanning: return LaserData::LaserStateType::EmissionEnabledScanning;
		case DynExpProto::NetworkLaser::StateType::Error: return LaserData::LaserStateType::Error;
		default: throw Util::InvalidDataException("The given state does not exist in the DynExpProto::NetworkLaser::StateType enumeration.");
		}
	}

	namespace NetworkLaserTasks
	{
		class InitTask : public gRPCInstrumentTasks::InitTask<Laser, 0, DynExpProto::NetworkLaser::NetworkLaser>
		{
			void InitFuncImpl(dispatch_tag<gRPCInstrumentTasks::InitTask<Laser, 0, DynExpProto::NetworkLaser::NetworkLaser>>, DynExp::InstrumentInstance& Instance) override final;

			virtual void InitFuncImpl(dispatch_tag<InitTask>, DynExp::InstrumentInstance& Instance) {}
		};

		class ExitTask : public gRPCInstrumentTasks::ExitTask<Laser, 0, DynExpProto::NetworkLaser::NetworkLaser>
		{
			void ExitFuncImpl(dispatch_tag<gRPCInstrumentTasks::ExitTask<Laser, 0, DynExpProto::NetworkLaser::NetworkLaser>>, DynExp::InstrumentInstance& Instance) override final;

			virtual void ExitFuncImpl(dispatch_tag<ExitTask>, DynExp::InstrumentInstance& Instance) {}
		};

		class UpdateTask : public gRPCInstrumentTasks::UpdateTask<Laser, 0, DynExpProto::NetworkLaser::NetworkLaser>
		{
			void UpdateFuncImpl(dispatch_tag<gRPCInstrumentTasks::UpdateTask<Laser, 0, DynExpProto::NetworkLaser::NetworkLaser>>, DynExp::InstrumentInstance& Instance) override final;

			virtual void UpdateFuncImpl(dispatch_tag<UpdateTask>, DynExp::InstrumentInstance& Instance) {}
		};

		class SetFrequencyTask final : public DynExp::TaskBase
		{
		public:
			SetFrequencyTask(double Frequency, CallbackType CallbackFunc) noexcept : TaskBase(CallbackFunc), Frequency(Frequency) {}

		private:
			virtual DynExp::TaskResultType RunChild(DynExp::InstrumentInstance& Instance) override;

			double Frequency;
		};

		class SetIntensityTask final : public DynExp::TaskBase
		{
		public:
			SetIntensityTask(double Intensity, CallbackType CallbackFunc) noexcept : TaskBase(CallbackFunc), Intensity(Intensity) {}

		private:
			virtual DynExp::TaskResultType RunChild(DynExp::InstrumentInstance& Instance) override;

			double Intensity;
		};		
		
		class SetScanRangeTask final : public DynExp::TaskBase
		{
		public:
			SetScanRangeTask(double ScanRange, CallbackType CallbackFunc) noexcept : TaskBase(CallbackFunc), ScanRange(ScanRange) {}

		private:
			virtual DynExp::TaskResultType RunChild(DynExp::InstrumentInstance& Instance) override;

			double ScanRange;
		};		
		
		class SetScanRateTask final : public DynExp::TaskBase
		{
		public:
			SetScanRateTask(double ScanRate, CallbackType CallbackFunc) noexcept : TaskBase(CallbackFunc), ScanRate(ScanRate) {}

		private:
			virtual DynExp::TaskResultType RunChild(DynExp::InstrumentInstance& Instance) override;

			double ScanRate;
		};

		class EnableTask final : public DynExp::TaskBase
		{
		public:
			EnableTask(CallbackType CallbackFunc) noexcept : TaskBase(CallbackFunc) {}

		private:
			virtual DynExp::TaskResultType RunChild(DynExp::InstrumentInstance& Instance) override;
		};

		class ScanContinuouslyTask final : public DynExp::TaskBase
		{
		public:
			ScanContinuouslyTask(CallbackType CallbackFunc) noexcept : TaskBase(CallbackFunc) {}

		private:
			virtual DynExp::TaskResultType RunChild(DynExp::InstrumentInstance& Instance) override;
		};

		class DisableScanTask final : public DynExp::TaskBase
		{
		public:
			DisableScanTask(CallbackType CallbackFunc) noexcept : TaskBase(CallbackFunc) {}

		private:
			virtual DynExp::TaskResultType RunChild(DynExp::InstrumentInstance& Instance) override;
		};

		class DisableTask final : public DynExp::TaskBase
		{
		public:
			DisableTask(CallbackType CallbackFunc) noexcept : TaskBase(CallbackFunc) {}

		private:
			virtual DynExp::TaskResultType RunChild(DynExp::InstrumentInstance& Instance) override;
		};
	}

	class NetworkLaserData : public gRPCInstrumentData<Laser, 0, DynExpProto::NetworkLaser::NetworkLaser>
	{
		friend class NetworkLaserTasks::InitTask;
		friend class NetworkLaserTasks::UpdateTask;

	public:
		NetworkLaserData() = default;
		virtual ~NetworkLaserData() = default;

		auto GetFrequencyUnit() const noexcept { return FrequencyUnit; }
		auto GetIntensityUnit() const noexcept { return IntensityUnit; }
		auto GetMinFrequency() const noexcept { return HardwareMinFrequency; }
		auto GetMaxFrequency() const noexcept { return HardwareMaxFrequency; }
		auto GetMinIntensity() const noexcept { return HardwareMinIntensity; }
		auto GetMaxIntensity() const noexcept { return HardwareMaxIntensity; }
		auto GetMinBandwidth() const noexcept { return HardwareMinBandwidth; }
		auto GetMaxBandwidth() const noexcept { return HardwareMaxBandwidth; }
		auto GetMaxRate() const noexcept { return HardwareMaxRate; }
		auto GetModeHopFreeTuningRange() const noexcept { return HardwareModeHopFreeTuningRange; }

	private:
		void ResetImpl(dispatch_tag<gRPCInstrumentData<Laser, 0, DynExpProto::NetworkLaser::NetworkLaser>>) override final;
		virtual void ResetImpl(dispatch_tag<NetworkLaserData>) {};

		virtual LaserStateType GetLaserStateChild() const noexcept override { return LaserState; }

		FrequencyUnitType FrequencyUnit = FrequencyUnitType::Hz;
		IntensityUnitType IntensityUnit = IntensityUnitType::Power_W;
		double HardwareMinFrequency = 0.0;
		double HardwareMaxFrequency = 0.0;
		double HardwareMinIntensity = 0.0;
		double HardwareMaxIntensity = 0.0;
		double HardwareMinBandwidth = 0.0;
		double HardwareMaxBandwidth = 0.0;
		double HardwareMaxRate = 0.0;
		double HardwareModeHopFreeTuningRange = 0.0;

		LaserStateType LaserState = LaserStateType::Ready;
	};

	class NetworkLaserParams : public gRPCInstrumentParams<Laser, 0, DynExpProto::NetworkLaser::NetworkLaser>
	{
	public:
		NetworkLaserParams(DynExp::ItemIDType ID, const DynExp::DynExpCore& Core) : gRPCInstrumentParams<Laser, 0, DynExpProto::NetworkLaser::NetworkLaser>(ID, Core) {}
		virtual ~NetworkLaserParams() = default;

		virtual const char* GetParamClassTag() const noexcept override { return "NetworkLaserParams"; }

	private:
		void ConfigureParamsImpl(dispatch_tag<gRPCInstrumentParams<Laser, 0, DynExpProto::NetworkLaser::NetworkLaser>>) override final { ConfigureParamsImpl(dispatch_tag<NetworkLaserParams>()); }
		virtual void ConfigureParamsImpl(dispatch_tag<NetworkLaserParams>) {}

		DummyParam Dummy = { *this };
	};

	class NetworkLaserConfigurator : public gRPCInstrumentConfigurator<Laser, 0, DynExpProto::NetworkLaser::NetworkLaser>
	{
	public:
		using ObjectType = NetworkLaser;
		using ParamsType = NetworkLaserParams;

		NetworkLaserConfigurator() = default;
		virtual ~NetworkLaserConfigurator() = default;

	private:
		virtual DynExp::ParamsBasePtrType MakeParams(DynExp::ItemIDType ID, const DynExp::DynExpCore& Core) const override { return DynExp::MakeParams<NetworkLaserConfigurator>(ID, Core); }
	};

	class NetworkLaser : public gRPCInstrument<Laser, 0, DynExpProto::NetworkLaser::NetworkLaser>
	{
	public:
		using ParamsType = NetworkLaserParams;
		using ConfigType = NetworkLaserConfigurator;
		using InstrumentDataType = NetworkLaserData;

		constexpr static auto Name() noexcept { return "Network Laser"; }

		NetworkLaser(const std::thread::id OwnerThreadID, DynExp::ParamsBasePtrType&& Params);
		virtual ~NetworkLaser() {}

		virtual std::string GetName() const override { return Name(); }

		virtual LaserData::FrequencyUnitType GetFrequencyUnit() const;
		virtual LaserData::IntensityUnitType GetIntensityUnit() const;
		virtual double GetMinFrequency() const;
		virtual double GetMaxFrequency() const;
		virtual double GetMinIntensity() const;
		virtual double GetMaxIntensity() const;
		virtual double GetMinBandwidth() const;
		virtual double GetMaxBandwidth() const;
		virtual double GetMaxRate() const;
		virtual double GetModeHopFreeTuningRange() const;

		// Logical const-ness: const member functions to allow inserting tasks into task queue.
		virtual void SetFrequency(double Frequency, DynExp::TaskBase::CallbackType CallbackFunc = nullptr) const override { MakeAndEnqueueTask<NetworkLaserTasks::SetFrequencyTask>(Frequency, CallbackFunc); }
		virtual void SetIntensity(double Intensity, DynExp::TaskBase::CallbackType CallbackFunc = nullptr) const override { MakeAndEnqueueTask<NetworkLaserTasks::SetIntensityTask>(Intensity, CallbackFunc); }
		virtual void SetScanRange(double ScanRange, DynExp::TaskBase::CallbackType CallbackFunc = nullptr) const override { MakeAndEnqueueTask<NetworkLaserTasks::SetScanRangeTask>(ScanRange, CallbackFunc); }
		virtual void SetScanRate(double ScanRate, DynExp::TaskBase::CallbackType CallbackFunc = nullptr) const override { MakeAndEnqueueTask<NetworkLaserTasks::SetScanRateTask>(ScanRate, CallbackFunc); }

		virtual void Enable(DynExp::TaskBase::CallbackType CallbackFunc = nullptr) const override { MakeAndEnqueueTask<NetworkLaserTasks::EnableTask>(CallbackFunc); }
		virtual void Disable(DynExp::TaskBase::CallbackType CallbackFunc = nullptr) const override { MakeAndEnqueueTask<NetworkLaserTasks::DisableTask>(CallbackFunc); }
		virtual void ScanContinuously(DynExp::TaskBase::CallbackType CallbackFunc = nullptr) const override { MakeAndEnqueueTask<NetworkLaserTasks::ScanContinuouslyTask>(CallbackFunc); }
		virtual void DisableScan(DynExp::TaskBase::CallbackType CallbackFunc = nullptr) const override { MakeAndEnqueueTask<NetworkLaserTasks::DisableScanTask>(CallbackFunc); }

	private:
		void ResetImpl(dispatch_tag<gRPCInstrument<Laser, 0, DynExpProto::NetworkLaser::NetworkLaser>>) override final;
		virtual void ResetImpl(dispatch_tag<NetworkLaser>) {}

		virtual std::unique_ptr<DynExp::InitTaskBase> MakeInitTask() const override { return DynExp::MakeTask<NetworkLaserTasks::InitTask>(); }
		virtual std::unique_ptr<DynExp::ExitTaskBase> MakeExitTask() const override { return DynExp::MakeTask<NetworkLaserTasks::ExitTask>(); }
		virtual std::unique_ptr<DynExp::UpdateTaskBase> MakeUpdateTask() const override { return DynExp::MakeTask<NetworkLaserTasks::UpdateTask>(); }
	};
}