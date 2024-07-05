// This file is part of DynExp.

/**
 * @file NetworkDigitalOutModule.h
 * @brief Implementation of a gRPC server module to provide remote access to a digital out
 * meta instrument.
*/

#pragma once

#include "stdafx.h"
#include "../MetaInstruments/DigitalOut.h"
#include "NetworkDataStreamInstrumentModule.h"

#include "NetworkDigitalOut.grpc.pb.h"

namespace DynExpModule
{
	template <typename... gRPCServices>
	class NetworkDigitalOutT;

	template <typename... gRPCServices>
	class NetworkDigitalOutData : public NetworkDataStreamInstrumentData<gRPCServices...>
	{
	public:
		using InstrType = DynExpInstr::DigitalOut;

		NetworkDigitalOutData() { Init(); }
		virtual ~NetworkDigitalOutData() = default;

		auto GetDigitalOut() { return DynExp::dynamic_Object_cast<InstrType>(NetworkDataStreamInstrumentData<gRPCServices...>::GetDataStreamInstrument().get()); }

	private:
		void ResetImpl(DynExp::ModuleDataBase::dispatch_tag<NetworkDataStreamInstrumentData<gRPCServices...>>) override final { Init(); }
		virtual void ResetImpl(DynExp::ModuleDataBase::dispatch_tag<NetworkDigitalOutData>) {};

		void Init() {}
	};

	template <typename... gRPCServices>
	class NetworkDigitalOutParams : public NetworkDataStreamInstrumentParams<gRPCServices...>
	{
	public:
		NetworkDigitalOutParams(DynExp::ItemIDType ID, const DynExp::DynExpCore& Core) : NetworkDataStreamInstrumentParams<gRPCServices...>(ID, Core) {}
		virtual ~NetworkDigitalOutParams() = default;

		virtual const char* GetParamClassTag() const noexcept override { return "NetworkDigitalOutParams"; }

	private:
		void ConfigureParamsImpl(DynExp::ParamsBase::dispatch_tag<NetworkDataStreamInstrumentParams<gRPCServices...>>) override final { ConfigureParamsImpl(DynExp::ParamsBase::dispatch_tag<NetworkDigitalOutParams>()); }
		virtual void ConfigureParamsImpl(DynExp::ParamsBase::dispatch_tag<NetworkDigitalOutParams>) {}

		DynExp::ParamsBase::DummyParam Dummy = { *this };
	};

	template <typename... gRPCServices>
	class NetworkDigitalOutConfigurator : public NetworkDataStreamInstrumentConfigurator<gRPCServices...>
	{
	public:
		using ObjectType = NetworkDigitalOutT<gRPCServices...>;
		using ParamsType = NetworkDigitalOutParams<gRPCServices...>;

		NetworkDigitalOutConfigurator() = default;
		virtual ~NetworkDigitalOutConfigurator() = default;

	private:
		virtual DynExp::ParamsBasePtrType MakeParams(DynExp::ItemIDType ID, const DynExp::DynExpCore& Core) const override { return DynExp::MakeParams<NetworkDigitalOutConfigurator>(ID, Core); }
	};

	template <typename... gRPCServices>
	class NetworkDigitalOutT : public NetworkDataStreamInstrumentT<gRPCServices...>
	{
	public:
		using ParamsType = NetworkDigitalOutParams<gRPCServices...>;
		using ConfigType = NetworkDigitalOutConfigurator<gRPCServices...>;
		using ModuleDataType = NetworkDigitalOutData<gRPCServices...>;
		using ThisServiceType = DynExpProto::NetworkDigitalOut::NetworkDigitalOut;

		constexpr static auto Name() noexcept { return "Network Digital Out"; }

		NetworkDigitalOutT(const std::thread::id OwnerThreadID, DynExp::ParamsBasePtrType&& Params)
			: NetworkDataStreamInstrumentT<gRPCServices...>(OwnerThreadID, std::move(Params)) {}
		virtual ~NetworkDigitalOutT() = default;

		virtual std::string GetName() const override { return Name(); }

	protected:
		class CallDataSetAsync
			: public gRPCModule<gRPCServices...>::template TypedCallDataBase<CallDataSetAsync, ThisServiceType, DynExpProto::NetworkDigitalOut::SampleMessage, DynExpProto::Common::VoidMessage>
		{
			using Base = gRPCModule<gRPCServices...>::template TypedCallDataBase<CallDataSetAsync, ThisServiceType, DynExpProto::NetworkDigitalOut::SampleMessage, DynExpProto::Common::VoidMessage>;
			using Base::RequestMessage;

		public:
			CallDataSetAsync(const gRPCModule<gRPCServices...>* const OwningModule) noexcept
				: Base::TypedCallDataBase(OwningModule, &ThisServiceType::AsyncService::RequestSetAsync) {}
			virtual ~CallDataSetAsync() = default;

		private:
			virtual void ProcessChildImpl(DynExp::ModuleInstance& Instance) override
			{
				auto ModuleData = DynExp::dynamic_ModuleData_cast<NetworkDigitalOutT>(Instance.ModuleDataGetter());

				ModuleData->GetDigitalOut()->Set(RequestMessage.value());
			}
		};

		class CallDataSetSync
			: public gRPCModule<gRPCServices...>::template TypedCallDataBase<CallDataSetSync, ThisServiceType, DynExpProto::NetworkDigitalOut::SampleMessage, DynExpProto::Common::VoidMessage>
		{
			using Base = gRPCModule<gRPCServices...>::template TypedCallDataBase<CallDataSetSync, ThisServiceType, DynExpProto::NetworkDigitalOut::SampleMessage, DynExpProto::Common::VoidMessage>;
			using Base::RequestMessage;

		public:
			CallDataSetSync(const gRPCModule<gRPCServices...>* const OwningModule) noexcept
				: Base::TypedCallDataBase(OwningModule, &DynExpProto::NetworkDigitalOut::NetworkDigitalOut::AsyncService::RequestSetSync) {}
			virtual ~CallDataSetSync() = default;

		private:
			virtual void ProcessChildImpl(DynExp::ModuleInstance& Instance) override
			{
				auto ModuleData = DynExp::dynamic_ModuleData_cast<NetworkDigitalOutT>(Instance.ModuleDataGetter());

				ModuleData->GetDigitalOut()->SetSync(RequestMessage.value());
			}
		};

		class CallDataSetDefault
			: public gRPCModule<gRPCServices...>::template TypedCallDataBase<CallDataSetDefault, ThisServiceType, DynExpProto::Common::VoidMessage, DynExpProto::Common::VoidMessage>
		{
			using Base = gRPCModule<gRPCServices...>::template TypedCallDataBase<CallDataSetDefault, ThisServiceType, DynExpProto::Common::VoidMessage, DynExpProto::Common::VoidMessage>;

		public:
			CallDataSetDefault(const gRPCModule<gRPCServices...>* const OwningModule) noexcept
				: Base::TypedCallDataBase(OwningModule, &DynExpProto::NetworkDigitalOut::NetworkDigitalOut::AsyncService::RequestSetDefault) {}
			virtual ~CallDataSetDefault() = default;

		private:
			virtual void ProcessChildImpl(DynExp::ModuleInstance& Instance) override
			{
				auto ModuleData = DynExp::dynamic_ModuleData_cast<NetworkDigitalOutT>(Instance.ModuleDataGetter());

				ModuleData->GetDigitalOut()->SetDefault();
			}
		};

	private:
		void ResetImpl(DynExp::Object::dispatch_tag<NetworkDataStreamInstrumentT<gRPCServices...>>) override final
		{
			ResetImpl(DynExp::Object::dispatch_tag<NetworkDigitalOutT>());
		}

		virtual void ResetImpl(DynExp::Object::dispatch_tag<NetworkDigitalOutT>) {}

		void CreateInitialCallDataObjectsImpl(DynExp::Object::dispatch_tag<NetworkDataStreamInstrumentT<gRPCServices...>>, DynExp::ModuleInstance& Instance) const override final
		{
			CallDataSetAsync::MakeCall(this, Instance);
			CallDataSetSync::MakeCall(this, Instance);
			CallDataSetDefault::MakeCall(this, Instance);

			CreateInitialCallDataObjectsImpl(DynExp::Object::dispatch_tag<NetworkDigitalOutT>(), Instance);
		}

		virtual void CreateInitialCallDataObjectsImpl(DynExp::Object::dispatch_tag<NetworkDigitalOutT>, DynExp::ModuleInstance& Instance) const {}

		virtual void ValidateInstrType(const DynExpInstr::DataStreamInstrument* Instr) const override { DynExp::dynamic_Object_cast<typename ModuleDataType::InstrType>(Instr); }
	};

	/**
	 * @brief Explicit instantiation of derivable class NetworkDigitalOutT to create the network digital out module.
	*/
	using NetworkDigitalOut = NetworkDigitalOutT<typename NetworkDataStreamInstrument::ThisServiceType, DynExpProto::NetworkDigitalOut::NetworkDigitalOut>;
}