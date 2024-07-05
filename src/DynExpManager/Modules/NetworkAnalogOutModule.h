// This file is part of DynExp.

/**
 * @file NetworkAnalogOutModule.h
 * @brief Implementation of a gRPC server module to provide remote access to an analog out
 * meta instrument.
*/

#pragma once

#include "stdafx.h"
#include "../MetaInstruments/AnalogOut.h"
#include "NetworkDataStreamInstrumentModule.h"

#include "NetworkAnalogOut.grpc.pb.h"

namespace DynExpModule
{
	template <typename... gRPCServices>
	class NetworkAnalogOutT;

	template <typename... gRPCServices>
	class NetworkAnalogOutData : public NetworkDataStreamInstrumentData<gRPCServices...>
	{
	public:
		using InstrType = DynExpInstr::AnalogOut;

		NetworkAnalogOutData() { Init(); }
		virtual ~NetworkAnalogOutData() = default;

		auto GetAnalogOut() { return DynExp::dynamic_Object_cast<InstrType>(NetworkDataStreamInstrumentData<gRPCServices...>::GetDataStreamInstrument().get()); }

	private:
		void ResetImpl(DynExp::ModuleDataBase::dispatch_tag<NetworkDataStreamInstrumentData<gRPCServices...>>) override final { Init(); }
		virtual void ResetImpl(DynExp::ModuleDataBase::dispatch_tag<NetworkAnalogOutData>) {};

		void Init() {}
	};

	template <typename... gRPCServices>
	class NetworkAnalogOutParams : public NetworkDataStreamInstrumentParams<gRPCServices...>
	{
	public:
		NetworkAnalogOutParams(DynExp::ItemIDType ID, const DynExp::DynExpCore& Core) : NetworkDataStreamInstrumentParams<gRPCServices...>(ID, Core) {}
		virtual ~NetworkAnalogOutParams() = default;

		virtual const char* GetParamClassTag() const noexcept override { return "NetworkAnalogOutParams"; }

	private:
		void ConfigureParamsImpl(DynExp::ParamsBase::dispatch_tag<NetworkDataStreamInstrumentParams<gRPCServices...>>) override final { ConfigureParamsImpl(DynExp::ParamsBase::dispatch_tag<NetworkAnalogOutParams>()); }
		virtual void ConfigureParamsImpl(DynExp::ParamsBase::dispatch_tag<NetworkAnalogOutParams>) {}

		DynExp::ParamsBase::DummyParam Dummy = { *this };
	};

	template <typename... gRPCServices>
	class NetworkAnalogOutConfigurator : public NetworkDataStreamInstrumentConfigurator<gRPCServices...>
	{
	public:
		using ObjectType = NetworkAnalogOutT<gRPCServices...>;
		using ParamsType = NetworkAnalogOutParams<gRPCServices...>;

		NetworkAnalogOutConfigurator() = default;
		virtual ~NetworkAnalogOutConfigurator() = default;

	private:
		virtual DynExp::ParamsBasePtrType MakeParams(DynExp::ItemIDType ID, const DynExp::DynExpCore& Core) const override { return DynExp::MakeParams<NetworkAnalogOutConfigurator>(ID, Core); }
	};

	template <typename... gRPCServices>
	class NetworkAnalogOutT : public NetworkDataStreamInstrumentT<gRPCServices...>
	{
	public:
		using ParamsType = NetworkAnalogOutParams<gRPCServices...>;
		using ConfigType = NetworkAnalogOutConfigurator<gRPCServices...>;
		using ModuleDataType = NetworkAnalogOutData<gRPCServices...>;
		using ThisServiceType = DynExpProto::NetworkAnalogOut::NetworkAnalogOut;

		constexpr static auto Name() noexcept { return "Network Analog Out"; }

		NetworkAnalogOutT(const std::thread::id OwnerThreadID, DynExp::ParamsBasePtrType&& Params)
			: NetworkDataStreamInstrumentT<gRPCServices...>(OwnerThreadID, std::move(Params)) {}
		virtual ~NetworkAnalogOutT() = default;

		virtual std::string GetName() const override { return Name(); }

	protected:
		class CallDataGetHardwareLimits
			: public gRPCModule<gRPCServices...>::template TypedCallDataBase<CallDataGetHardwareLimits, ThisServiceType, DynExpProto::Common::VoidMessage, DynExpProto::NetworkAnalogOut::ValueLimitsMessage>
		{
			using Base = gRPCModule<gRPCServices...>::template TypedCallDataBase<CallDataGetHardwareLimits, ThisServiceType, DynExpProto::Common::VoidMessage, DynExpProto::NetworkAnalogOut::ValueLimitsMessage>;
			using Base::ResponseMessage;

		public:
			CallDataGetHardwareLimits(const gRPCModule<gRPCServices...>* const OwningModule) noexcept
				: Base::TypedCallDataBase(OwningModule, &DynExpProto::NetworkAnalogOut::NetworkAnalogOut::AsyncService::RequestGetHardwareLimits) {}
			virtual ~CallDataGetHardwareLimits() = default;

		private:
			virtual void ProcessChildImpl(DynExp::ModuleInstance& Instance) override
			{
				auto ModuleData = DynExp::dynamic_ModuleData_cast<NetworkAnalogOutT>(Instance.ModuleDataGetter());

				ResponseMessage.set_lowerlimit(ModuleData->GetAnalogOut()->GetHardwareMinValue());
				ResponseMessage.set_upperlimit(ModuleData->GetAnalogOut()->GetHardwareMaxValue());
			}
		};

		class CallDataGetUserLimits
			: public gRPCModule<gRPCServices...>::template TypedCallDataBase<CallDataGetUserLimits, ThisServiceType, DynExpProto::Common::VoidMessage, DynExpProto::NetworkAnalogOut::ValueLimitsMessage>
		{
			using Base = gRPCModule<gRPCServices...>::template TypedCallDataBase<CallDataGetUserLimits, ThisServiceType, DynExpProto::Common::VoidMessage, DynExpProto::NetworkAnalogOut::ValueLimitsMessage>;
			using Base::ResponseMessage;

		public:
			CallDataGetUserLimits(const gRPCModule<gRPCServices...>* const OwningModule) noexcept
				: Base::TypedCallDataBase(OwningModule, &DynExpProto::NetworkAnalogOut::NetworkAnalogOut::AsyncService::RequestGetUserLimits) {}
			virtual ~CallDataGetUserLimits() = default;

		private:
			virtual void ProcessChildImpl(DynExp::ModuleInstance& Instance) override
			{
				auto ModuleData = DynExp::dynamic_ModuleData_cast<NetworkAnalogOutT>(Instance.ModuleDataGetter());

				ResponseMessage.set_lowerlimit(ModuleData->GetAnalogOut()->GetUserMinValue());
				ResponseMessage.set_upperlimit(ModuleData->GetAnalogOut()->GetUserMaxValue());
			}
		};

		class CallDataHardwareResolution
			: public gRPCModule<gRPCServices...>::template TypedCallDataBase<CallDataHardwareResolution, ThisServiceType, DynExpProto::Common::VoidMessage, DynExpProto::NetworkAnalogOut::ValueResolutionMessage>
		{
			using Base = gRPCModule<gRPCServices...>::template TypedCallDataBase<CallDataHardwareResolution, ThisServiceType, DynExpProto::Common::VoidMessage, DynExpProto::NetworkAnalogOut::ValueResolutionMessage>;
			using Base::ResponseMessage;

		public:
			CallDataHardwareResolution(const gRPCModule<gRPCServices...>* const OwningModule) noexcept
				: Base::TypedCallDataBase(OwningModule, &DynExpProto::NetworkAnalogOut::NetworkAnalogOut::AsyncService::RequestGetHardwareResolution) {}
			virtual ~CallDataHardwareResolution() = default;

		private:
			virtual void ProcessChildImpl(DynExp::ModuleInstance& Instance) override
			{
				auto ModuleData = DynExp::dynamic_ModuleData_cast<NetworkAnalogOutT>(Instance.ModuleDataGetter());

				ResponseMessage.set_resolution(ModuleData->GetAnalogOut()->GetHardwareResolution());
			}
		};

		class CallDataGetValueUnit
			: public gRPCModule<gRPCServices...>::template TypedCallDataBase<CallDataGetValueUnit, ThisServiceType, DynExpProto::Common::VoidMessage, DynExpProto::NetworkAnalogOut::ValueUnitMessage>
		{
			using Base = gRPCModule<gRPCServices...>::template TypedCallDataBase<CallDataGetValueUnit, ThisServiceType, DynExpProto::Common::VoidMessage, DynExpProto::NetworkAnalogOut::ValueUnitMessage>;
			using Base::ResponseMessage;

		public:
			CallDataGetValueUnit(const gRPCModule<gRPCServices...>* const OwningModule) noexcept
				: Base::TypedCallDataBase(OwningModule, &DynExpProto::NetworkAnalogOut::NetworkAnalogOut::AsyncService::RequestGetValueUnit) {}
			virtual ~CallDataGetValueUnit() = default;

		private:
			virtual void ProcessChildImpl(DynExp::ModuleInstance& Instance) override
			{
				auto ModuleData = DynExp::dynamic_ModuleData_cast<NetworkAnalogOutT>(Instance.ModuleDataGetter());

				ResponseMessage.set_unit(DynExpInstr::ToPrototUnitType(ModuleData->GetAnalogOut()->GetValueUnit()));
			}
		};

		class CallDataSetAsync
			: public gRPCModule<gRPCServices...>::template TypedCallDataBase<CallDataSetAsync, ThisServiceType, DynExpProto::NetworkAnalogOut::SampleMessage, DynExpProto::Common::VoidMessage>
		{
			using Base = gRPCModule<gRPCServices...>::template TypedCallDataBase<CallDataSetAsync, ThisServiceType, DynExpProto::NetworkAnalogOut::SampleMessage, DynExpProto::Common::VoidMessage>;
			using Base::RequestMessage;

		public:
			CallDataSetAsync(const gRPCModule<gRPCServices...>* const OwningModule) noexcept
				: Base::TypedCallDataBase(OwningModule, &DynExpProto::NetworkAnalogOut::NetworkAnalogOut::AsyncService::RequestSetAsync) {}
			virtual ~CallDataSetAsync() = default;

		private:
			virtual void ProcessChildImpl(DynExp::ModuleInstance& Instance) override
			{
				auto ModuleData = DynExp::dynamic_ModuleData_cast<NetworkAnalogOutT>(Instance.ModuleDataGetter());

				ModuleData->GetAnalogOut()->Set(RequestMessage.value());
			}
		};

		class CallDataSetSync
			: public gRPCModule<gRPCServices...>::template TypedCallDataBase<CallDataSetSync, ThisServiceType, DynExpProto::NetworkAnalogOut::SampleMessage, DynExpProto::Common::VoidMessage>
		{
			using Base = gRPCModule<gRPCServices...>::template TypedCallDataBase<CallDataSetSync, ThisServiceType, DynExpProto::NetworkAnalogOut::SampleMessage, DynExpProto::Common::VoidMessage>;
			using Base::RequestMessage;

		public:
			CallDataSetSync(const gRPCModule<gRPCServices...>* const OwningModule) noexcept
				: Base::TypedCallDataBase(OwningModule, &DynExpProto::NetworkAnalogOut::NetworkAnalogOut::AsyncService::RequestSetSync) {}
			virtual ~CallDataSetSync() = default;

		private:
			virtual void ProcessChildImpl(DynExp::ModuleInstance& Instance) override
			{
				auto ModuleData = DynExp::dynamic_ModuleData_cast<NetworkAnalogOutT>(Instance.ModuleDataGetter());

				ModuleData->GetAnalogOut()->SetSync(RequestMessage.value());
			}
		};

		class CallDataSetDefault
			: public gRPCModule<gRPCServices...>::template TypedCallDataBase<CallDataSetDefault, ThisServiceType, DynExpProto::Common::VoidMessage, DynExpProto::Common::VoidMessage>
		{
			using Base = gRPCModule<gRPCServices...>::template TypedCallDataBase<CallDataSetDefault, ThisServiceType, DynExpProto::Common::VoidMessage, DynExpProto::Common::VoidMessage>;

		public:
			CallDataSetDefault(const gRPCModule<gRPCServices...>* const OwningModule) noexcept
				: Base::TypedCallDataBase(OwningModule, &DynExpProto::NetworkAnalogOut::NetworkAnalogOut::AsyncService::RequestSetDefault) {}
			virtual ~CallDataSetDefault() = default;

		private:
			virtual void ProcessChildImpl(DynExp::ModuleInstance& Instance) override
			{
				auto ModuleData = DynExp::dynamic_ModuleData_cast<NetworkAnalogOutT>(Instance.ModuleDataGetter());

				ModuleData->GetAnalogOut()->SetDefault();
			}
		};

	private:
		void ResetImpl(DynExp::Object::dispatch_tag<NetworkDataStreamInstrumentT<gRPCServices...>>) override final
		{
			ResetImpl(DynExp::Object::dispatch_tag<NetworkAnalogOutT>());
		}

		virtual void ResetImpl(DynExp::Object::dispatch_tag<NetworkAnalogOutT>) {}

		void CreateInitialCallDataObjectsImpl(DynExp::Object::dispatch_tag<NetworkDataStreamInstrumentT<gRPCServices...>>, DynExp::ModuleInstance& Instance) const override final
		{
			CallDataGetHardwareLimits::MakeCall(this, Instance);
			CallDataGetUserLimits::MakeCall(this, Instance);
			CallDataHardwareResolution::MakeCall(this, Instance);
			CallDataGetValueUnit::MakeCall(this, Instance);
			CallDataSetAsync::MakeCall(this, Instance);
			CallDataSetSync::MakeCall(this, Instance);
			CallDataSetDefault::MakeCall(this, Instance);

			CreateInitialCallDataObjectsImpl(DynExp::Object::dispatch_tag<NetworkAnalogOutT>(), Instance);
		}

		virtual void CreateInitialCallDataObjectsImpl(DynExp::Object::dispatch_tag<NetworkAnalogOutT>, DynExp::ModuleInstance& Instance) const {}

		virtual void ValidateInstrType(const DynExpInstr::DataStreamInstrument* Instr) const override { DynExp::dynamic_Object_cast<typename ModuleDataType::InstrType>(Instr); }
	};

	/**
	 * @brief Explicit instantiation of derivable class NetworkAnalogOutT to create the network analog out module.
	*/
	using NetworkAnalogOut = NetworkAnalogOutT<typename NetworkDataStreamInstrument::ThisServiceType, DynExpProto::NetworkAnalogOut::NetworkAnalogOut>;
}