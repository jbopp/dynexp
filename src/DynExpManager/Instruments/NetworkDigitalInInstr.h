// This file is part of DynExp.

/**
 * @file NetworkDigitalInInstr.h
 * @brief Implementation of a gRPC client instrument to access a remote digital in meta instrument.
*/

#pragma once

#include "stdafx.h"
#include "MetaInstruments/DigitalIn.h"
#include "Instruments/NetworkDataStreamInstrument.h"

#include "NetworkDigitalIn.pb.h"
#include "NetworkDigitalIn.grpc.pb.h"

namespace DynExpInstr
{
	template <typename BaseInstr, typename std::enable_if_t<std::is_base_of_v<DigitalIn, BaseInstr>, int>, typename... gRPCStubs>
	class NetworkDigitalInT;

	namespace NetworkDigitalInTasks
	{
		template <typename BaseInstr, typename std::enable_if_t<std::is_base_of_v<DigitalIn, BaseInstr>, int>, typename... gRPCStubs>
		class InitTask : public NetworkDataStreamInstrumentTasks::InitTask<BaseInstr, 0, gRPCStubs...>
		{
			void InitFuncImpl(DynExp::InitTaskBase::dispatch_tag<NetworkDataStreamInstrumentTasks::InitTask<BaseInstr, 0, gRPCStubs...>>, DynExp::InstrumentInstance& Instance) override final
			{
				InitFuncImpl(DynExp::InitTaskBase::dispatch_tag<InitTask>(), Instance);
			}

			virtual void InitFuncImpl(DynExp::InitTaskBase::dispatch_tag<InitTask>, DynExp::InstrumentInstance& Instance) {}
		};

		template <typename BaseInstr, typename std::enable_if_t<std::is_base_of_v<DigitalIn, BaseInstr>, int>, typename... gRPCStubs>
		class ExitTask : public NetworkDataStreamInstrumentTasks::ExitTask<BaseInstr, 0, gRPCStubs...>
		{
			void ExitFuncImpl(DynExp::ExitTaskBase::dispatch_tag<NetworkDataStreamInstrumentTasks::ExitTask<BaseInstr, 0, gRPCStubs...>>, DynExp::InstrumentInstance& Instance) override final
			{
				ExitFuncImpl(DynExp::ExitTaskBase::dispatch_tag<ExitTask>(), Instance);
			}

			virtual void ExitFuncImpl(DynExp::ExitTaskBase::dispatch_tag<ExitTask>, DynExp::InstrumentInstance& Instance) {}
		};

		template <typename BaseInstr, typename std::enable_if_t<std::is_base_of_v<DigitalIn, BaseInstr>, int>, typename... gRPCStubs>
		class UpdateTask : public NetworkDataStreamInstrumentTasks::UpdateTask<BaseInstr, 0, gRPCStubs...>
		{
			void UpdateFuncImpl(DynExp::UpdateTaskBase::dispatch_tag<NetworkDataStreamInstrumentTasks::UpdateTask<BaseInstr, 0, gRPCStubs...>>, DynExp::InstrumentInstance& Instance) override final
			{
				UpdateFuncImpl(DynExp::UpdateTaskBase::dispatch_tag<UpdateTask>(), Instance);
			}

			virtual void UpdateFuncImpl(DynExp::UpdateTaskBase::dispatch_tag<UpdateTask>, DynExp::InstrumentInstance& Instance) {}
		};
	}

	template <typename BaseInstr, typename std::enable_if_t<std::is_base_of_v<DigitalIn, BaseInstr>, int>, typename... gRPCStubs>
	class NetworkDigitalInData : public NetworkDataStreamInstrumentData<BaseInstr, 0, gRPCStubs...>
	{
	public:
		using InstrumentType = NetworkDigitalInT<BaseInstr, 0, gRPCStubs...>;

		NetworkDigitalInData(size_t BufferSizeInSamples = 1)
			: NetworkDataStreamInstrumentData<BaseInstr, 0, gRPCStubs...>(BufferSizeInSamples) {}
		virtual ~NetworkDigitalInData() = default;

	private:
		void ResetImpl(DynExp::InstrumentDataBase::dispatch_tag<NetworkDataStreamInstrumentData<BaseInstr, 0, gRPCStubs...>>) override final
		{
			ResetImpl(DynExp::InstrumentDataBase::dispatch_tag<NetworkDigitalInData>());
		}

		virtual void ResetImpl(DynExp::InstrumentDataBase::dispatch_tag<NetworkDigitalInData>) {};
	};

	template <typename BaseInstr, typename std::enable_if_t<std::is_base_of_v<DigitalIn, BaseInstr>, int>, typename... gRPCStubs>
	class NetworkDigitalInParams : public NetworkDataStreamInstrumentParams<BaseInstr, 0, gRPCStubs...>
	{
	public:
		NetworkDigitalInParams(DynExp::ItemIDType ID, const DynExp::DynExpCore& Core) : NetworkDataStreamInstrumentParams<BaseInstr, 0, gRPCStubs...>(ID, Core) {}
		virtual ~NetworkDigitalInParams() = default;

		virtual const char* GetParamClassTag() const noexcept override { return "NetworkDigitalInParams"; }

	private:
		void ConfigureParamsImpl(DynExp::ParamsBase::dispatch_tag<NetworkDataStreamInstrumentParams<BaseInstr, 0, gRPCStubs...>>) override final
		{
			InputPortParams::DisableUserEditable();
			DigitalInParams::DisableUserEditable();

			ConfigureParamsImpl(DynExp::ParamsBase::dispatch_tag<NetworkDigitalInParams>());
		}
		
		virtual void ConfigureParamsImpl(DynExp::ParamsBase::dispatch_tag<NetworkDigitalInParams>) {}

		DynExp::ParamsBase::DummyParam Dummy = { *this };
	};
	
	template <typename BaseInstr, typename std::enable_if_t<std::is_base_of_v<DigitalIn, BaseInstr>, int>, typename... gRPCStubs>
	class NetworkDigitalInConfigurator : public NetworkDataStreamInstrumentConfigurator<BaseInstr, 0, gRPCStubs...>
	{
	public:
		using ObjectType = NetworkDigitalInT<BaseInstr, 0, gRPCStubs...>;
		using ParamsType = NetworkDigitalInParams<BaseInstr, 0, gRPCStubs...>;

		NetworkDigitalInConfigurator() = default;
		virtual ~NetworkDigitalInConfigurator() = default;

	private:
		virtual DynExp::ParamsBasePtrType MakeParams(DynExp::ItemIDType ID, const DynExp::DynExpCore& Core) const override { return DynExp::MakeParams<NetworkDigitalInConfigurator>(ID, Core); }
	};

	template <typename BaseInstr, typename std::enable_if_t<std::is_base_of_v<DigitalIn, BaseInstr>, int>, typename... gRPCStubs>
	class NetworkDigitalInT : public NetworkDataStreamInstrumentT<BaseInstr, 0, gRPCStubs...>
	{
	public:
		using ParamsType = NetworkDigitalInParams<BaseInstr, 0, gRPCStubs...>;
		using ConfigType = NetworkDigitalInConfigurator<BaseInstr, 0, gRPCStubs...>;
		using InstrumentDataType = NetworkDigitalInData<BaseInstr, 0, gRPCStubs...>;
		using StubType = DynExpProto::NetworkDigitalIn::NetworkDigitalIn;

		constexpr static auto Name() noexcept { return "Network Digital In"; }

		NetworkDigitalInT(const std::thread::id OwnerThreadID, DynExp::ParamsBasePtrType&& Params)
			: NetworkDataStreamInstrumentT<BaseInstr, 0, gRPCStubs...>(OwnerThreadID, std::move(Params)) {}
		virtual ~NetworkDigitalInT() {}

		virtual std::string GetName() const override { return Name(); }

		// Override in order to disable writing to an input instrument.
		virtual void WriteData(DynExp::TaskBase::CallbackType CallbackFunc = nullptr) const override { DynExp::InstrumentBase::MakeAndEnqueueTask<DynExp::DefaultTask>(CallbackFunc); }

		virtual DigitalInData::SampleStreamType::SampleType Get(DynExp::TaskBase::CallbackType CallbackFunc = nullptr) const override
		{
			StubPtrType<StubType> StubPtr;
			{
				auto InstrData = dynamic_InstrumentData_cast<NetworkDigitalInT>(this->GetInstrumentData());
				StubPtr = InstrData->template GetStub<StubType>();
			} // InstrData unlocked here.

			auto Response = InvokeStubFunc(StubPtr, &StubType::Stub::GetAsync, {});

			return Response.value();
		}

		virtual DigitalInData::SampleStreamType::SampleType GetSync() const override
		{
			StubPtrType<StubType> StubPtr;
			{
				auto InstrData = dynamic_InstrumentData_cast<NetworkDigitalInT>(this->GetInstrumentData());
				StubPtr = InstrData->template GetStub<StubType>();
			} // InstrData unlocked here.

			auto Response = InvokeStubFunc(StubPtr, &StubType::Stub::GetSync, {});

			return Response.value();
		}

	private:
		void ResetImpl(DynExp::Object::dispatch_tag<NetworkDataStreamInstrumentT<BaseInstr, 0, gRPCStubs...>>) override final
		{
			ResetImpl(DynExp::Object::dispatch_tag<NetworkDigitalInT>());
		}

		virtual void ResetImpl(DynExp::Object::dispatch_tag<NetworkDigitalInT>) {}

		virtual std::unique_ptr<DynExp::InitTaskBase> MakeInitTask() const override { return DynExp::MakeTask<NetworkDigitalInTasks::InitTask<BaseInstr, 0, gRPCStubs...>>(); }
		virtual std::unique_ptr<DynExp::ExitTaskBase> MakeExitTask() const override { return DynExp::MakeTask<NetworkDigitalInTasks::ExitTask<BaseInstr, 0, gRPCStubs...>>(); }
		virtual std::unique_ptr<DynExp::UpdateTaskBase> MakeUpdateTask() const override { return DynExp::MakeTask<NetworkDigitalInTasks::UpdateTask<BaseInstr, 0, gRPCStubs...>>(); }
	};

	/**
	 * @brief Explicit instantiation of derivable class NetworkDigitalInT to create the network digital in instrument.
	*/
	using NetworkDigitalIn = NetworkDigitalInT<DigitalIn, 0, typename NetworkDataStreamInstrument::StubType, DynExpProto::NetworkDigitalIn::NetworkDigitalIn>;
}