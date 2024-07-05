// This file is part of DynExp.

/**
 * @file NetworkAnalogInInstr.h
 * @brief Implementation of a gRPC client instrument to access a remote analog in meta instrument.
*/

#pragma once

#include "stdafx.h"
#include "MetaInstruments/AnalogIn.h"
#include "Instruments/NetworkDataStreamInstrument.h"

#include "NetworkAnalogIn.pb.h"
#include "NetworkAnalogIn.grpc.pb.h"

namespace DynExpInstr
{
	template <typename BaseInstr, typename std::enable_if_t<std::is_base_of_v<AnalogIn, BaseInstr>, int>, typename... gRPCStubs>
	class NetworkAnalogInT;

	namespace NetworkAnalogInTasks
	{
		template <typename BaseInstr, typename std::enable_if_t<std::is_base_of_v<AnalogIn, BaseInstr>, int>, typename... gRPCStubs>
		class InitTask : public NetworkDataStreamInstrumentTasks::InitTask<BaseInstr, 0, gRPCStubs...>
		{
			void InitFuncImpl(DynExp::InitTaskBase::dispatch_tag<NetworkDataStreamInstrumentTasks::InitTask<BaseInstr, 0, gRPCStubs...>>, DynExp::InstrumentInstance& Instance) override final
			{
				InitFuncImpl(DynExp::InitTaskBase::dispatch_tag<InitTask>(), Instance);
			}

			virtual void InitFuncImpl(DynExp::InitTaskBase::dispatch_tag<InitTask>, DynExp::InstrumentInstance& Instance) {}
		};

		template <typename BaseInstr, typename std::enable_if_t<std::is_base_of_v<AnalogIn, BaseInstr>, int>, typename... gRPCStubs>
		class ExitTask : public NetworkDataStreamInstrumentTasks::ExitTask<BaseInstr, 0, gRPCStubs...>
		{
			void ExitFuncImpl(DynExp::ExitTaskBase::dispatch_tag<NetworkDataStreamInstrumentTasks::ExitTask<BaseInstr, 0, gRPCStubs...>>, DynExp::InstrumentInstance& Instance) override final
			{
				ExitFuncImpl(DynExp::ExitTaskBase::dispatch_tag<ExitTask>(), Instance);
			}

			virtual void ExitFuncImpl(DynExp::ExitTaskBase::dispatch_tag<ExitTask>, DynExp::InstrumentInstance& Instance) {}
		};

		template <typename BaseInstr, typename std::enable_if_t<std::is_base_of_v<AnalogIn, BaseInstr>, int>, typename... gRPCStubs>
		class UpdateTask : public NetworkDataStreamInstrumentTasks::UpdateTask<BaseInstr, 0, gRPCStubs...>
		{
			void UpdateFuncImpl(DynExp::UpdateTaskBase::dispatch_tag<NetworkDataStreamInstrumentTasks::UpdateTask<BaseInstr, 0, gRPCStubs...>>, DynExp::InstrumentInstance& Instance) override final
			{
				UpdateFuncImpl(DynExp::UpdateTaskBase::dispatch_tag<UpdateTask>(), Instance);
			}

			virtual void UpdateFuncImpl(DynExp::UpdateTaskBase::dispatch_tag<UpdateTask>, DynExp::InstrumentInstance& Instance) {}
		};
	}

	template <typename BaseInstr, typename std::enable_if_t<std::is_base_of_v<AnalogIn, BaseInstr>, int>, typename... gRPCStubs>
	class NetworkAnalogInData : public NetworkDataStreamInstrumentData<BaseInstr, 0, gRPCStubs...>
	{
	public:
		using InstrumentType = NetworkAnalogInT<BaseInstr, 0, gRPCStubs...>;

		NetworkAnalogInData(size_t BufferSizeInSamples = 1)
			: NetworkDataStreamInstrumentData<BaseInstr, 0, gRPCStubs...>(BufferSizeInSamples) {}
		virtual ~NetworkAnalogInData() = default;

	private:
		void ResetImpl(DynExp::InstrumentDataBase::dispatch_tag<NetworkDataStreamInstrumentData<BaseInstr, 0, gRPCStubs...>>) override final
		{
			ResetImpl(DynExp::InstrumentDataBase::dispatch_tag<NetworkAnalogInData>());
		}

		virtual void ResetImpl(DynExp::InstrumentDataBase::dispatch_tag<NetworkAnalogInData>) {};
	};

	template <typename BaseInstr, typename std::enable_if_t<std::is_base_of_v<AnalogIn, BaseInstr>, int>, typename... gRPCStubs>
	class NetworkAnalogInParams : public NetworkDataStreamInstrumentParams<BaseInstr, 0, gRPCStubs...>
	{
	public:
		NetworkAnalogInParams(DynExp::ItemIDType ID, const DynExp::DynExpCore& Core) : NetworkDataStreamInstrumentParams<BaseInstr, 0, gRPCStubs...>(ID, Core) {}
		virtual ~NetworkAnalogInParams() = default;

		virtual const char* GetParamClassTag() const noexcept override { return "NetworkAnalogInParams"; }

	private:
		void ConfigureParamsImpl(DynExp::ParamsBase::dispatch_tag<NetworkDataStreamInstrumentParams<BaseInstr, 0, gRPCStubs...>>) override final
		{
			InputPortParams::DisableUserEditable();
			AnalogInParams::DisableUserEditable();

			ConfigureParamsImpl(DynExp::ParamsBase::dispatch_tag<NetworkAnalogInParams>());
		}
		
		virtual void ConfigureParamsImpl(DynExp::ParamsBase::dispatch_tag<NetworkAnalogInParams>) {}

		DynExp::ParamsBase::DummyParam Dummy = { *this };
	};

	template <typename BaseInstr, typename std::enable_if_t<std::is_base_of_v<AnalogIn, BaseInstr>, int>, typename... gRPCStubs>
	class NetworkAnalogInConfigurator : public NetworkDataStreamInstrumentConfigurator<BaseInstr, 0, gRPCStubs...>
	{
	public:
		using ObjectType = NetworkAnalogInT<BaseInstr, 0, gRPCStubs...>;
		using ParamsType = NetworkAnalogInParams<BaseInstr, 0, gRPCStubs...>;

		NetworkAnalogInConfigurator() = default;
		virtual ~NetworkAnalogInConfigurator() = default;

	private:
		virtual DynExp::ParamsBasePtrType MakeParams(DynExp::ItemIDType ID, const DynExp::DynExpCore& Core) const override { return DynExp::MakeParams<NetworkAnalogInConfigurator>(ID, Core); }
	};

	template <typename BaseInstr, typename std::enable_if_t<std::is_base_of_v<AnalogIn, BaseInstr>, int>, typename... gRPCStubs>
	class NetworkAnalogInT : public NetworkDataStreamInstrumentT<BaseInstr, 0, gRPCStubs...>
	{
	public:
		using ParamsType = NetworkAnalogInParams<BaseInstr, 0, gRPCStubs...>;
		using ConfigType = NetworkAnalogInConfigurator<BaseInstr, 0, gRPCStubs...>;
		using InstrumentDataType = NetworkAnalogInData<BaseInstr, 0, gRPCStubs...>;
		using StubType = DynExpProto::NetworkAnalogIn::NetworkAnalogIn;

		constexpr static auto Name() noexcept { return "Network Analog In"; }

		NetworkAnalogInT(const std::thread::id OwnerThreadID, DynExp::ParamsBasePtrType&& Params)
			: NetworkDataStreamInstrumentT<BaseInstr, 0, gRPCStubs...>(OwnerThreadID, std::move(Params)) {}
		virtual ~NetworkAnalogInT() {}

		virtual std::string GetName() const override { return Name(); }

		virtual DataStreamInstrumentData::ValueType GetHardwareMinValue() const override
		{
			StubPtrType<StubType> StubPtr;
			{
				auto InstrData = dynamic_InstrumentData_cast<NetworkAnalogInT>(this->GetInstrumentData());
				StubPtr = InstrData->template GetStub<StubType>();
			} // InstrData unlocked here.

			auto Response = InvokeStubFunc(StubPtr, &StubType::Stub::GetHardwareLimits, {});

			return Response.lowerlimit();
		}

		virtual DataStreamInstrumentData::ValueType GetHardwareMaxValue() const override
		{
			StubPtrType<StubType> StubPtr;
			{
				auto InstrData = dynamic_InstrumentData_cast<NetworkAnalogInT>(this->GetInstrumentData());
				StubPtr = InstrData->template GetStub<StubType>();
			} // InstrData unlocked here.

			auto Response = InvokeStubFunc(StubPtr, &StubType::Stub::GetHardwareLimits, {});

			return Response.upperlimit();
		}

		virtual DataStreamInstrumentData::ValueType GetHardwareResolution() const override final
		{
			StubPtrType<StubType> StubPtr;
			{
				auto InstrData = dynamic_InstrumentData_cast<NetworkAnalogInT>(this->GetInstrumentData());
				StubPtr = InstrData->template GetStub<StubType>();
			} // InstrData unlocked here.

			auto Response = InvokeStubFunc(StubPtr, &StubType::Stub::GetHardwareResolution, {});

			return Response.resolution();
		}

		virtual DataStreamInstrumentData::UnitType GetValueUnit() const override
		{
			StubPtrType<StubType> StubPtr;
			{
				auto InstrData = dynamic_InstrumentData_cast<NetworkAnalogInT>(this->GetInstrumentData());
				StubPtr = InstrData->template GetStub<StubType>();
			} // InstrData unlocked here.

			auto Response = InvokeStubFunc(StubPtr, &StubType::Stub::GetValueUnit, {});

			return ToDataStreamInstrumentUnitType(Response.unit());
		}

		// Override in order to disable writing to an input instrument.
		virtual void WriteData(DynExp::TaskBase::CallbackType CallbackFunc = nullptr) const override { DynExp::InstrumentBase::MakeAndEnqueueTask<DynExp::DefaultTask>(CallbackFunc); }

		virtual AnalogInData::SampleStreamType::SampleType Get(DynExp::TaskBase::CallbackType CallbackFunc = nullptr) const
		{
			StubPtrType<StubType> StubPtr;
			{
				auto InstrData = dynamic_InstrumentData_cast<NetworkAnalogInT>(this->GetInstrumentData());
				StubPtr = InstrData->template GetStub<StubType>();
			} // InstrData unlocked here.

			auto Response = InvokeStubFunc(StubPtr, &StubType::Stub::GetAsync, {});

			return Response.value();
		}

		virtual AnalogInData::SampleStreamType::SampleType GetSync() const
		{
			StubPtrType<StubType> StubPtr;
			{
				auto InstrData = dynamic_InstrumentData_cast<NetworkAnalogInT>(this->GetInstrumentData());
				StubPtr = InstrData->template GetStub<StubType>();
			} // InstrData unlocked here.

			auto Response = InvokeStubFunc(StubPtr, &StubType::Stub::GetSync, {});

			return Response.value();
		}

	private:
		void ResetImpl(DynExp::Object::dispatch_tag<NetworkDataStreamInstrumentT<BaseInstr, 0, gRPCStubs...>>) override final
		{
			ResetImpl(DynExp::Object::dispatch_tag<NetworkAnalogInT>());
		}

		virtual void ResetImpl(DynExp::Object::dispatch_tag<NetworkAnalogInT>) {}

		virtual std::unique_ptr<DynExp::InitTaskBase> MakeInitTask() const override { return DynExp::MakeTask<NetworkAnalogInTasks::InitTask<BaseInstr, 0, gRPCStubs...>>(); }
		virtual std::unique_ptr<DynExp::ExitTaskBase> MakeExitTask() const override { return DynExp::MakeTask<NetworkAnalogInTasks::ExitTask<BaseInstr, 0, gRPCStubs...>>(); }
		virtual std::unique_ptr<DynExp::UpdateTaskBase> MakeUpdateTask() const override { return DynExp::MakeTask<NetworkAnalogInTasks::UpdateTask<BaseInstr, 0, gRPCStubs...>>(); }
	};

	/**
	 * @brief Explicit instantiation of derivable class NetworkAnalogInT to create the network analog in instrument.
	*/
	using NetworkAnalogIn = NetworkAnalogInT<AnalogIn, 0, typename NetworkDataStreamInstrument::StubType, DynExpProto::NetworkAnalogIn::NetworkAnalogIn>;
}