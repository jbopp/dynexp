// This file is part of DynExp.

/**
 * @file NetworkSpectrometer.h
 * @brief Implementation of a gRPC client instrument to access a remote spectrometer meta instrument.
*/

#pragma once

#include "stdafx.h"
#include "DynExpCore.h"
#include "MetaInstruments/Spectrometer.h"
#include "MetaInstruments/gRPCInstrument.h"

#include "NetworkSpectrometer.pb.h"
#include "NetworkSpectrometer.grpc.pb.h"

namespace DynExpInstr
{
	class NetworkSpectrometer;

	constexpr DynExpProto::Common::FrequencyUnitType ToPrototUnitType(SpectrometerData::FrequencyUnitType Unit)
	{
		switch (Unit)
		{
		case SpectrometerData::FrequencyUnitType::Hz: return DynExpProto::Common::FrequencyUnitType::Hz;
		case SpectrometerData::FrequencyUnitType::nm: return DynExpProto::Common::FrequencyUnitType::nm;
		case SpectrometerData::FrequencyUnitType::Inv_cm: return DynExpProto::Common::FrequencyUnitType::Inv_cm;
		default: throw Util::InvalidDataException("The given unit does not exist in the SpectrometerData::FrequencyUnitType enumeration. Did you forget to adjust the FrequencyUnitType enumeration in class \"SpectrometerData\"?");
		}
	}

	constexpr SpectrometerData::FrequencyUnitType ToSpectrometerUnitType(DynExpProto::Common::FrequencyUnitType Unit)
	{
		switch (Unit)
		{
		case DynExpProto::Common::FrequencyUnitType::Hz: return SpectrometerData::FrequencyUnitType::Hz;
		case DynExpProto::Common::FrequencyUnitType::nm: return SpectrometerData::FrequencyUnitType::nm;
		case DynExpProto::Common::FrequencyUnitType::Inv_cm: return SpectrometerData::FrequencyUnitType::Inv_cm;
		default: throw Util::InvalidDataException("The given unit does not exist in the DynExpProto::Common::FrequencyUnitType enumeration. Did you forget to adjust the FrequencyUnitType enumeration in file \"Common.proto\"?");
		}
	}

	constexpr DynExpProto::Common::IntensityUnitType ToPrototUnitType(SpectrometerData::IntensityUnitType Unit)
	{
		switch (Unit)
		{
		case SpectrometerData::IntensityUnitType::Counts: return DynExpProto::Common::IntensityUnitType::IntensityCounts;
		default: throw Util::InvalidDataException("The given unit does not exist in the SpectrometerData::IntensityUnitType enumeration. Did you forget to adjust the IntensityUnitType enumeration in class \"SpectrometerData\"?");
		}
	}

	constexpr SpectrometerData::IntensityUnitType ToSpectrometerUnitType(DynExpProto::Common::IntensityUnitType Unit)
	{
		switch (Unit)
		{
		case DynExpProto::Common::IntensityUnitType::IntensityCounts: return SpectrometerData::IntensityUnitType::Counts;
		default: throw Util::InvalidDataException("The given unit does not exist in the DynExpProto::Common::IntensityUnitType enumeration. Did you forget to adjust the IntensityUnitType enumeration in file \"Common.proto\"?");
		}
	}

	constexpr SpectrometerData::CapturingStateType ToSpectrometerStateType(DynExpProto::NetworkSpectrometer::StateType State)
	{
		switch (State)
		{
		case DynExpProto::NetworkSpectrometer::StateType::Ready: return SpectrometerData::CapturingStateType::Ready;
		case DynExpProto::NetworkSpectrometer::StateType::WarningState: return SpectrometerData::CapturingStateType::Warning;
		case DynExpProto::NetworkSpectrometer::StateType::ErrorState: return SpectrometerData::CapturingStateType::Error;
		case DynExpProto::NetworkSpectrometer::StateType::Recording: return SpectrometerData::CapturingStateType::Capturing;
		default: throw Util::InvalidDataException("The given state does not exist in the DynExpProto::NetworkSpectrometer::StateType enumeration.");
		}
	}

	namespace NetworkSpectrometerTasks
	{
		class InitTask : public gRPCInstrumentTasks::InitTask<Spectrometer, 0, DynExpProto::NetworkSpectrometer::NetworkSpectrometer>
		{
			void InitFuncImpl(dispatch_tag<gRPCInstrumentTasks::InitTask<Spectrometer, 0, DynExpProto::NetworkSpectrometer::NetworkSpectrometer>>, DynExp::InstrumentInstance& Instance) override final;

			virtual void InitFuncImpl(dispatch_tag<InitTask>, DynExp::InstrumentInstance& Instance) {}
		};

		class ExitTask : public gRPCInstrumentTasks::ExitTask<Spectrometer, 0, DynExpProto::NetworkSpectrometer::NetworkSpectrometer>
		{
			void ExitFuncImpl(dispatch_tag<gRPCInstrumentTasks::ExitTask<Spectrometer, 0, DynExpProto::NetworkSpectrometer::NetworkSpectrometer>>, DynExp::InstrumentInstance& Instance) override final;

			virtual void ExitFuncImpl(dispatch_tag<ExitTask>, DynExp::InstrumentInstance& Instance) {}
		};

		class UpdateTask : public gRPCInstrumentTasks::UpdateTask<Spectrometer, 0, DynExpProto::NetworkSpectrometer::NetworkSpectrometer>
		{
			void UpdateFuncImpl(dispatch_tag<gRPCInstrumentTasks::UpdateTask<Spectrometer, 0, DynExpProto::NetworkSpectrometer::NetworkSpectrometer>>, DynExp::InstrumentInstance& Instance) override final;

			virtual void UpdateFuncImpl(dispatch_tag<UpdateTask>, DynExp::InstrumentInstance& Instance) {}
		};

		class SetExposureTimeTask final : public DynExp::TaskBase
		{
		public:
			SetExposureTimeTask(SpectrometerData::TimeType ExposureTime, CallbackType CallbackFunc) noexcept : TaskBase(CallbackFunc), ExposureTime(ExposureTime) {}

		private:
			virtual DynExp::TaskResultType RunChild(DynExp::InstrumentInstance& Instance) override;

			SpectrometerData::TimeType ExposureTime;
		};

		class SetFrequencyRangeTask final : public DynExp::TaskBase
		{
		public:
			SetFrequencyRangeTask(double LowerFrequency, double UpperFrequency, CallbackType CallbackFunc) noexcept : TaskBase(CallbackFunc), LowerFrequency(LowerFrequency), UpperFrequency(UpperFrequency) {}

		private:
			virtual DynExp::TaskResultType RunChild(DynExp::InstrumentInstance& Instance) override;

			double LowerFrequency;
			double UpperFrequency;
		};

		class SetSetSilentModeTask final : public DynExp::TaskBase
		{
		public:
			SetSetSilentModeTask(bool Enable, CallbackType CallbackFunc) noexcept : TaskBase(CallbackFunc), Enable(Enable) {}

		private:
			virtual DynExp::TaskResultType RunChild(DynExp::InstrumentInstance& Instance) override;

			bool Enable;
		};

		class RecordTask final : public DynExp::TaskBase
		{
		public:
			RecordTask(CallbackType CallbackFunc) noexcept : TaskBase(CallbackFunc) {}

		private:
			virtual DynExp::TaskResultType RunChild(DynExp::InstrumentInstance& Instance) override;
		};

		class AbortTask final : public DynExp::TaskBase
		{
		public:
			AbortTask(CallbackType CallbackFunc) noexcept : TaskBase(CallbackFunc) {}

		private:
			virtual DynExp::TaskResultType RunChild(DynExp::InstrumentInstance& Instance) override;
		};
	}

	class NetworkSpectrometerData : public gRPCInstrumentData<Spectrometer, 0, DynExpProto::NetworkSpectrometer::NetworkSpectrometer>
	{
		friend class NetworkSpectrometerTasks::InitTask;
		friend class NetworkSpectrometerTasks::UpdateTask;

	public:
		NetworkSpectrometerData() = default;
		virtual ~NetworkSpectrometerData() = default;

		auto GetFrequencyUnit() const noexcept { return FrequencyUnit; }
		auto GetIntensityUnit() const noexcept { return IntensityUnit; }
		auto GetMinFrequency() const noexcept { return MinFrequency; }
		auto GetMaxFrequency() const noexcept { return MaxFrequency; }

	private:
		void ResetImpl(dispatch_tag<gRPCInstrumentData<Spectrometer, 0, DynExpProto::NetworkSpectrometer::NetworkSpectrometer>>) override final;
		virtual void ResetImpl(dispatch_tag<NetworkSpectrometerData>) {};

		virtual CapturingStateType GetCapturingStateChild() const noexcept override { return CapturingState; }
		virtual double GetCapturingProgressChild() const noexcept override { return CapturingProgress; }

		FrequencyUnitType FrequencyUnit = FrequencyUnitType::Hz;
		IntensityUnitType IntensityUnit = IntensityUnitType::Counts;
		double MinFrequency = 0.0;
		double MaxFrequency = 0.0;

		CapturingStateType CapturingState = CapturingStateType::Ready;
		double CapturingProgress = 0.0;
	};

	class NetworkSpectrometerParams : public gRPCInstrumentParams<Spectrometer, 0, DynExpProto::NetworkSpectrometer::NetworkSpectrometer>
	{
	public:
		NetworkSpectrometerParams(DynExp::ItemIDType ID, const DynExp::DynExpCore& Core) : gRPCInstrumentParams<Spectrometer, 0, DynExpProto::NetworkSpectrometer::NetworkSpectrometer>(ID, Core) {}
		virtual ~NetworkSpectrometerParams() = default;

		virtual const char* GetParamClassTag() const noexcept override { return "NetworkSpectrometerParams"; }

	private:
		void ConfigureParamsImpl(dispatch_tag<gRPCInstrumentParams<Spectrometer, 0, DynExpProto::NetworkSpectrometer::NetworkSpectrometer>>) override final { ConfigureParamsImpl(dispatch_tag<NetworkSpectrometerParams>()); }
		virtual void ConfigureParamsImpl(dispatch_tag<NetworkSpectrometerParams>) {}

		DummyParam Dummy = { *this };
	};

	class NetworkSpectrometerConfigurator : public gRPCInstrumentConfigurator<Spectrometer, 0, DynExpProto::NetworkSpectrometer::NetworkSpectrometer>
	{
	public:
		using ObjectType = NetworkSpectrometer;
		using ParamsType = NetworkSpectrometerParams;

		NetworkSpectrometerConfigurator() = default;
		virtual ~NetworkSpectrometerConfigurator() = default;

	private:
		virtual DynExp::ParamsBasePtrType MakeParams(DynExp::ItemIDType ID, const DynExp::DynExpCore& Core) const override { return DynExp::MakeParams<NetworkSpectrometerConfigurator>(ID, Core); }
	};

	class NetworkSpectrometer : public gRPCInstrument<Spectrometer, 0, DynExpProto::NetworkSpectrometer::NetworkSpectrometer>
	{
	public:
		using ParamsType = NetworkSpectrometerParams;
		using ConfigType = NetworkSpectrometerConfigurator;
		using InstrumentDataType = NetworkSpectrometerData;

		constexpr static auto Name() noexcept { return "Network Spectrometer"; }

		NetworkSpectrometer(const std::thread::id OwnerThreadID, DynExp::ParamsBasePtrType&& Params);
		virtual ~NetworkSpectrometer() {}

		virtual std::string GetName() const override { return Name(); }

		virtual SpectrometerData::FrequencyUnitType GetFrequencyUnit() const;
		virtual SpectrometerData::IntensityUnitType GetIntensityUnit() const;
		virtual double GetMinFrequency() const;
		virtual double GetMaxFrequency() const;

		// Logical const-ness: const member functions to allow inserting tasks into task queue.
		virtual void SetExposureTime(SpectrometerData::TimeType ExposureTime, DynExp::TaskBase::CallbackType CallbackFunc = nullptr) const override { MakeAndEnqueueTask<NetworkSpectrometerTasks::SetExposureTimeTask>(ExposureTime, CallbackFunc); }
		virtual void SetFrequencyRange(double LowerFrequency, double UpperFrequency, DynExp::TaskBase::CallbackType CallbackFunc = nullptr) const override { MakeAndEnqueueTask<NetworkSpectrometerTasks::SetFrequencyRangeTask>(LowerFrequency, UpperFrequency, CallbackFunc); }
		virtual void SetSilentMode(bool Enable, DynExp::TaskBase::CallbackType CallbackFunc = nullptr) const override { MakeAndEnqueueTask<NetworkSpectrometerTasks::SetSetSilentModeTask>(Enable, CallbackFunc); }

		virtual void Record(DynExp::TaskBase::CallbackType CallbackFunc = nullptr) const override { MakeAndEnqueueTask<NetworkSpectrometerTasks::RecordTask>(CallbackFunc); }
		virtual void Abort(DynExp::TaskBase::CallbackType CallbackFunc = nullptr) const override { MakeAndEnqueueTask<NetworkSpectrometerTasks::AbortTask>(CallbackFunc); }

	private:
		void ResetImpl(dispatch_tag<gRPCInstrument<Spectrometer, 0, DynExpProto::NetworkSpectrometer::NetworkSpectrometer>>) override final;
		virtual void ResetImpl(dispatch_tag<NetworkSpectrometer>) {}

		virtual std::unique_ptr<DynExp::InitTaskBase> MakeInitTask() const override { return DynExp::MakeTask<NetworkSpectrometerTasks::InitTask>(); }
		virtual std::unique_ptr<DynExp::ExitTaskBase> MakeExitTask() const override { return DynExp::MakeTask<NetworkSpectrometerTasks::ExitTask>(); }
		virtual std::unique_ptr<DynExp::UpdateTaskBase> MakeUpdateTask() const override { return DynExp::MakeTask<NetworkSpectrometerTasks::UpdateTask>(); }
	};
}