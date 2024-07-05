// This file is part of DynExp.

/**
 * @file NetworkDigitalInModule.h
 * @brief Implementation of a gRPC server module to provide remote access to a digital in
 * meta instrument.
*/

#pragma once

#include "stdafx.h"
#include "../MetaInstruments/DigitalIn.h"
#include "NetworkDataStreamInstrumentModule.h"

#include "NetworkDigitalIn.grpc.pb.h"

namespace DynExpModule
{
	template <typename... gRPCServices>
	class NetworkDigitalInT;

	template <typename... gRPCServices>
	class NetworkDigitalInData : public NetworkDataStreamInstrumentData<gRPCServices...>
	{
	public:
		using InstrType = DynExpInstr::DigitalIn;

		NetworkDigitalInData() { Init(); }
		virtual ~NetworkDigitalInData() = default;

		auto GetDigitalIn() { return DynExp::dynamic_Object_cast<InstrType>(NetworkDataStreamInstrumentData<gRPCServices...>::GetDataStreamInstrument().get()); }

	private:
		void ResetImpl(DynExp::ModuleDataBase::dispatch_tag<NetworkDataStreamInstrumentData<gRPCServices...>>) override final { Init(); }
		virtual void ResetImpl(DynExp::ModuleDataBase::dispatch_tag<NetworkDigitalInData>) {};

		void Init() {}
	};

	template <typename... gRPCServices>
	class NetworkDigitalInParams : public NetworkDataStreamInstrumentParams<gRPCServices...>
	{
	public:
		NetworkDigitalInParams(DynExp::ItemIDType ID, const DynExp::DynExpCore& Core) : NetworkDataStreamInstrumentParams<gRPCServices...>(ID, Core) {}
		virtual ~NetworkDigitalInParams() = default;

		virtual const char* GetParamClassTag() const noexcept override { return "NetworkDigitalInParams"; }

	private:
		void ConfigureParamsImpl(DynExp::ParamsBase::dispatch_tag<NetworkDataStreamInstrumentParams<gRPCServices...>>) override final { ConfigureParamsImpl(DynExp::ParamsBase::dispatch_tag<NetworkDigitalInParams>()); }
		virtual void ConfigureParamsImpl(DynExp::ParamsBase::dispatch_tag<NetworkDigitalInParams>) {}

		DynExp::ParamsBase::DummyParam Dummy = { *this };
	};

	template <typename... gRPCServices>
	class NetworkDigitalInConfigurator : public NetworkDataStreamInstrumentConfigurator<gRPCServices...>
	{
	public:
		using ObjectType = NetworkDigitalInT<gRPCServices...>;
		using ParamsType = NetworkDigitalInParams<gRPCServices...>;

		NetworkDigitalInConfigurator() = default;
		virtual ~NetworkDigitalInConfigurator() = default;

	private:
		virtual DynExp::ParamsBasePtrType MakeParams(DynExp::ItemIDType ID, const DynExp::DynExpCore& Core) const override { return DynExp::MakeParams<NetworkDigitalInConfigurator>(ID, Core); }
	};

	template <typename... gRPCServices>
	class NetworkDigitalInT : public NetworkDataStreamInstrumentT<gRPCServices...>
	{
	public:
		using ParamsType = NetworkDigitalInParams<gRPCServices...>;
		using ConfigType = NetworkDigitalInConfigurator<gRPCServices...>;
		using ModuleDataType = NetworkDigitalInData<gRPCServices...>;
		using ThisServiceType = DynExpProto::NetworkDigitalIn::NetworkDigitalIn;

		constexpr static auto Name() noexcept { return "Network Digital In"; }

		NetworkDigitalInT(const std::thread::id OwnerThreadID, DynExp::ParamsBasePtrType&& Params)
			: NetworkDataStreamInstrumentT<gRPCServices...>(OwnerThreadID, std::move(Params)) {}
		virtual ~NetworkDigitalInT() = default;

		virtual std::string GetName() const override { return Name(); }

	protected:
		class CallDataGetAsync
			: public gRPCModule<gRPCServices...>::template TypedCallDataBase<CallDataGetAsync, ThisServiceType, DynExpProto::Common::VoidMessage, DynExpProto::NetworkDigitalIn::SampleMessage>
		{
			using Base = gRPCModule<gRPCServices...>::template TypedCallDataBase<CallDataGetAsync, ThisServiceType, DynExpProto::Common::VoidMessage, DynExpProto::NetworkDigitalIn::SampleMessage>;
			using Base::ResponseMessage;

		public:
			CallDataGetAsync(const gRPCModule<gRPCServices...>* const OwningModule) noexcept
				: Base::TypedCallDataBase(OwningModule, &ThisServiceType::AsyncService::RequestGetAsync) {}
			virtual ~CallDataGetAsync() = default;

		private:
			virtual void ProcessChildImpl(DynExp::ModuleInstance& Instance) override
			{
				auto ModuleData = DynExp::dynamic_ModuleData_cast<NetworkDigitalInT>(Instance.ModuleDataGetter());

				ResponseMessage.set_value(static_cast<bool>(ModuleData->GetDigitalIn()->Get()));
			}
		};

		class CallDataGetSync
			: public gRPCModule<gRPCServices...>::template TypedCallDataBase<CallDataGetSync, ThisServiceType, DynExpProto::Common::VoidMessage, DynExpProto::NetworkDigitalIn::SampleMessage>
		{
			using Base = gRPCModule<gRPCServices...>::template TypedCallDataBase<CallDataGetSync, ThisServiceType, DynExpProto::Common::VoidMessage, DynExpProto::NetworkDigitalIn::SampleMessage>;
			using Base::ResponseMessage;

		public:
			CallDataGetSync(const gRPCModule<gRPCServices...>* const OwningModule) noexcept
				: Base::TypedCallDataBase(OwningModule, &DynExpProto::NetworkDigitalIn::NetworkDigitalIn::AsyncService::RequestGetSync) {}
			virtual ~CallDataGetSync() = default;

		private:
			virtual void ProcessChildImpl(DynExp::ModuleInstance& Instance) override
			{
				auto ModuleData = DynExp::dynamic_ModuleData_cast<NetworkDigitalInT>(Instance.ModuleDataGetter());

				ResponseMessage.set_value(static_cast<bool>(ModuleData->GetDigitalIn()->GetSync()));
			}
		};

	private:
		void ResetImpl(DynExp::Object::dispatch_tag<NetworkDataStreamInstrumentT<gRPCServices...>>) override final
		{
			ResetImpl(DynExp::Object::dispatch_tag<NetworkDigitalInT>());
		}

		virtual void ResetImpl(DynExp::Object::dispatch_tag<NetworkDigitalInT>) {}

		void CreateInitialCallDataObjectsImpl(DynExp::Object::dispatch_tag<NetworkDataStreamInstrumentT<gRPCServices...>>, DynExp::ModuleInstance& Instance) const override final
		{
			CallDataGetAsync::MakeCall(this, Instance);
			CallDataGetSync::MakeCall(this, Instance);

			CreateInitialCallDataObjectsImpl(DynExp::Object::dispatch_tag<NetworkDigitalInT>(), Instance);
		}

		virtual void CreateInitialCallDataObjectsImpl(DynExp::Object::dispatch_tag<NetworkDigitalInT>, DynExp::ModuleInstance& Instance) const {}

		virtual void ValidateInstrType(const DynExpInstr::DataStreamInstrument* Instr) const override { DynExp::dynamic_Object_cast<typename ModuleDataType::InstrType>(Instr); }
	};

	/**
	 * @brief Explicit instantiation of derivable class NetworkDigitalInT to create the network digital in module.
	*/
	using NetworkDigitalIn = NetworkDigitalInT<typename NetworkDataStreamInstrument::ThisServiceType, DynExpProto::NetworkDigitalIn::NetworkDigitalIn>;
}