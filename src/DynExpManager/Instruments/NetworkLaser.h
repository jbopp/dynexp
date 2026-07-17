// This file is part of DynExp.

/**
 * @file NetworkLaser.h
 * @brief Implementation of a gRPC client instrument to access a remote laser meta instrument.
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

	constexpr DynExp::UnitType ToLaserUnitType(DynExpProto::Common::FrequencyUnitType Unit)
	{
		switch (Unit)
		{
		case DynExpProto::Common::FrequencyUnitType::Hz: return DynExp::UnitType::Freq_Hz;
		case DynExpProto::Common::FrequencyUnitType::nm: return DynExp::UnitType::Wavelength_nm;
		default: throw Util::InvalidDataException("The given unit is not supported here. Did you forget to adjust this function or the FrequencyUnitType enumeration in file \"Common.proto\"?");
		}
	}

	constexpr DynExp::UnitType ToLaserUnitType(DynExpProto::Common::IntensityUnitType Unit)
	{
		switch (Unit)
		{
		case DynExpProto::Common::IntensityUnitType::Power_W: return DynExp::UnitType::Power_W;
		default: throw Util::InvalidDataException("The given unit is not supported here. Did you forget to adjust this function or the IntensityUnitType enumeration in file \"Common.proto\"?");
		}
	}

	constexpr LaserData::LaserStateType ToLaserStateType(DynExpProto::NetworkLaser::StateType State)
	{
		switch (State)
		{
		case DynExpProto::NetworkLaser::StateType::Ready: return LaserData::LaserStateType::Ready;
		case DynExpProto::NetworkLaser::StateType::Startup: return LaserData::LaserStateType::Startup;
		case DynExpProto::NetworkLaser::StateType::EnabledConstant: return LaserData::LaserStateType::EmissionEnabledConstant;
		case DynExpProto::NetworkLaser::StateType::EnabledScanning: return LaserData::LaserStateType::EmissionEnabledScanning;
		case DynExpProto::NetworkLaser::StateType::ErrorState: return LaserData::LaserStateType::Error;
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
			SetFrequencyTask(double Frequency, CallbackType CallbackFunc) noexcept : TaskBase(std::move(CallbackFunc)), Frequency(Frequency) {}

		private:
			virtual DynExp::TaskResultType RunChild(DynExp::InstrumentInstance& Instance) override;

			double Frequency;
		};

		class SetIntensityTask final : public DynExp::TaskBase
		{
		public:
			SetIntensityTask(double Intensity, CallbackType CallbackFunc) noexcept : TaskBase(std::move(CallbackFunc)), Intensity(Intensity) {}

		private:
			virtual DynExp::TaskResultType RunChild(DynExp::InstrumentInstance& Instance) override;

			double Intensity;
		};		
		
		class SetScanRangeTask final : public DynExp::TaskBase
		{
		public:
			SetScanRangeTask(double ScanRange, CallbackType CallbackFunc) noexcept : TaskBase(std::move(CallbackFunc)), ScanRange(ScanRange) {}

		private:
			virtual DynExp::TaskResultType RunChild(DynExp::InstrumentInstance& Instance) override;

			double ScanRange;
		};		
		
		class SetScanRateTask final : public DynExp::TaskBase
		{
		public:
			SetScanRateTask(double ScanRate, CallbackType CallbackFunc) noexcept : TaskBase(std::move(CallbackFunc)), ScanRate(ScanRate) {}

		private:
			virtual DynExp::TaskResultType RunChild(DynExp::InstrumentInstance& Instance) override;

			double ScanRate;
		};

		class EnableTask final : public DynExp::TaskBase
		{
		public:
			EnableTask(CallbackType CallbackFunc) noexcept : TaskBase(std::move(CallbackFunc)) {}

		private:
			virtual DynExp::TaskResultType RunChild(DynExp::InstrumentInstance& Instance) override;
		};

		class ScanContinuouslyTask final : public DynExp::TaskBase
		{
		public:
			ScanContinuouslyTask(CallbackType CallbackFunc) noexcept : TaskBase(std::move(CallbackFunc)) {}

		private:
			virtual DynExp::TaskResultType RunChild(DynExp::InstrumentInstance& Instance) override;
		};

		class DisableScanTask final : public DynExp::TaskBase
		{
		public:
			DisableScanTask(CallbackType CallbackFunc) noexcept : TaskBase(std::move(CallbackFunc)) {}

		private:
			virtual DynExp::TaskResultType RunChild(DynExp::InstrumentInstance& Instance) override;
		};

		class DisableTask final : public DynExp::TaskBase
		{
		public:
			DisableTask(CallbackType CallbackFunc) noexcept : TaskBase(std::move(CallbackFunc)) {}

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
		auto GetMinScanRange() const noexcept { return HardwareMinScanRange; }
		auto GetMaxScanRange() const noexcept { return HardwareMaxScanRange; }
		auto GetMinScanRate() const noexcept { return HardwareMinScanRate; }
		auto GetMaxScanRate() const noexcept { return HardwareMaxScanRate; }
		auto GetModeHopFreeTuningRange() const noexcept { return HardwareModeHopFreeTuningRange; }

	private:
		void ResetImpl(dispatch_tag<gRPCInstrumentData<Laser, 0, DynExpProto::NetworkLaser::NetworkLaser>>) override final;
		virtual void ResetImpl(dispatch_tag<NetworkLaserData>) {};

		virtual LaserStateType GetLaserStateChild() const noexcept override { return LaserState; }

		DynExp::UnitType FrequencyUnit = DynExp::UnitType::Freq_Hz;
		DynExp::UnitType IntensityUnit = DynExp::UnitType::Power_W;
		double HardwareMinFrequency = 0.0;
		double HardwareMaxFrequency = 0.0;
		double HardwareMinIntensity = 0.0;
		double HardwareMaxIntensity = 0.0;
		double HardwareMinScanRange = 0.0;
		double HardwareMaxScanRange = 0.0;
		double HardwareMinScanRate = 0.0;
		double HardwareMaxScanRate = 0.0;
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

		virtual DynExp::UnitType GetFrequencyUnit() const override;
		virtual DynExp::UnitType GetIntensityUnit() const override;
		virtual double GetMinFrequency() const override;
		virtual double GetMaxFrequency() const override;
		virtual double GetMinIntensity() const override;
		virtual double GetMaxIntensity() const override;
		virtual double GetMinScanRange() const override;
		virtual double GetMaxScanRange() const override;
		virtual double GetMinScanRate() const override;
		virtual double GetMaxScanRate() const override;
		virtual double GetModeHopFreeTuningRange() const override;

		// Logical const-ness: const member functions to allow inserting tasks into task queue.
		virtual void SetFrequency(double Frequency, DynExp::TaskBase::CallbackType CallbackFunc = nullptr) const override { MakeAndEnqueueTask<NetworkLaserTasks::SetFrequencyTask>(Frequency, std::move(CallbackFunc)); }
		virtual void SetIntensity(double Intensity, DynExp::TaskBase::CallbackType CallbackFunc = nullptr) const override { MakeAndEnqueueTask<NetworkLaserTasks::SetIntensityTask>(Intensity, std::move(CallbackFunc)); }
		virtual void SetScanRange(double ScanRange, DynExp::TaskBase::CallbackType CallbackFunc = nullptr) const override { MakeAndEnqueueTask<NetworkLaserTasks::SetScanRangeTask>(ScanRange, std::move(CallbackFunc)); }
		virtual void SetScanRate(double ScanRate, DynExp::TaskBase::CallbackType CallbackFunc = nullptr) const override { MakeAndEnqueueTask<NetworkLaserTasks::SetScanRateTask>(ScanRate, std::move(CallbackFunc)); }

		virtual void Enable(DynExp::TaskBase::CallbackType CallbackFunc = nullptr) const override { MakeAndEnqueueTask<NetworkLaserTasks::EnableTask>(std::move(CallbackFunc)); }
		virtual void Disable(DynExp::TaskBase::CallbackType CallbackFunc = nullptr) const override { MakeAndEnqueueTask<NetworkLaserTasks::DisableTask>(std::move(CallbackFunc)); }
		virtual void ScanContinuously(DynExp::TaskBase::CallbackType CallbackFunc = nullptr) const override { MakeAndEnqueueTask<NetworkLaserTasks::ScanContinuouslyTask>(std::move(CallbackFunc)); }
		virtual void DisableScan(DynExp::TaskBase::CallbackType CallbackFunc = nullptr) const override { MakeAndEnqueueTask<NetworkLaserTasks::DisableScanTask>(std::move(CallbackFunc)); }

	private:
		void ResetImpl(dispatch_tag<gRPCInstrument<Laser, 0, DynExpProto::NetworkLaser::NetworkLaser>>) override final;
		virtual void ResetImpl(dispatch_tag<NetworkLaser>) {}

		virtual std::unique_ptr<DynExp::InitTaskBase> MakeInitTask() const override { return DynExp::MakeTask<NetworkLaserTasks::InitTask>(); }
		virtual std::unique_ptr<DynExp::ExitTaskBase> MakeExitTask() const override { return DynExp::MakeTask<NetworkLaserTasks::ExitTask>(); }
		virtual std::unique_ptr<DynExp::UpdateTaskBase> MakeUpdateTask() const override { return DynExp::MakeTask<NetworkLaserTasks::UpdateTask>(); }
	};
}