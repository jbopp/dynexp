// This file is part of DynExp.

/**
 * @file NetworkAnalogInModule.h
 * @brief Implementation of a gRPC server module to provide remote access to an analog in
 * meta instrument.
*/

#pragma once

#include "stdafx.h"
#include "../MetaInstruments/AnalogIn.h"
#include "NetworkDataStreamInstrumentModule.h"

#include "NetworkAnalogIn.grpc.pb.h"

namespace DynExpModule
{
	template <typename... gRPCServices>
	class NetworkAnalogInT;

	template <typename... gRPCServices>
	class NetworkAnalogInData : public NetworkDataStreamInstrumentData<gRPCServices...>
	{
	public:
		using InstrType = DynExpInstr::AnalogIn;

		NetworkAnalogInData() { Init(); }
		virtual ~NetworkAnalogInData() = default;

		auto GetAnalogIn() { return DynExp::dynamic_Object_cast<InstrType>(NetworkDataStreamInstrumentData<gRPCServices...>::GetDataStreamInstrument().get()); }

	private:
		void ResetImpl(DynExp::ModuleDataBase::dispatch_tag<NetworkDataStreamInstrumentData<gRPCServices...>>) override final { Init(); }
		virtual void ResetImpl(DynExp::ModuleDataBase::dispatch_tag<NetworkAnalogInData>) {};

		void Init() {}
	};

	template <typename... gRPCServices>
	class NetworkAnalogInParams : public NetworkDataStreamInstrumentParams<gRPCServices...>
	{
	public:
		NetworkAnalogInParams(DynExp::ItemIDType ID, const DynExp::DynExpCore& Core) : NetworkDataStreamInstrumentParams<gRPCServices...>(ID, Core) {}
		virtual ~NetworkAnalogInParams() = default;

		virtual const char* GetParamClassTag() const noexcept override { return "NetworkAnalogInParams"; }

	private:
		void ConfigureParamsImpl(DynExp::ParamsBase::dispatch_tag<NetworkDataStreamInstrumentParams<gRPCServices...>>) override final { ConfigureParamsImpl(DynExp::ParamsBase::dispatch_tag<NetworkAnalogInParams>()); }
		virtual void ConfigureParamsImpl(DynExp::ParamsBase::dispatch_tag<NetworkAnalogInParams>) {}

		DynExp::ParamsBase::DummyParam Dummy = { *this };
	};

	template <typename... gRPCServices>
	class NetworkAnalogInConfigurator : public NetworkDataStreamInstrumentConfigurator<gRPCServices...>
	{
	public:
		using ObjectType = NetworkAnalogInT<gRPCServices...>;
		using ParamsType = NetworkAnalogInParams<gRPCServices...>;

		NetworkAnalogInConfigurator() = default;
		virtual ~NetworkAnalogInConfigurator() = default;

	private:
		virtual DynExp::ParamsBasePtrType MakeParams(DynExp::ItemIDType ID, const DynExp::DynExpCore& Core) const override { return DynExp::MakeParams<NetworkAnalogInConfigurator>(ID, Core); }
	};

	template <typename... gRPCServices>
	class NetworkAnalogInT : public NetworkDataStreamInstrumentT<gRPCServices...>
	{
	public:
		using ParamsType = NetworkAnalogInParams<gRPCServices...>;
		using ConfigType = NetworkAnalogInConfigurator<gRPCServices...>;
		using ModuleDataType = NetworkAnalogInData<gRPCServices...>;
		using ThisServiceType = DynExpProto::NetworkAnalogIn::NetworkAnalogIn;

		constexpr static auto Name() noexcept { return "Network Analog In"; }

		NetworkAnalogInT(const std::thread::id OwnerThreadID, DynExp::ParamsBasePtrType&& Params)
			: NetworkDataStreamInstrumentT<gRPCServices...>(OwnerThreadID, std::move(Params)) {}
		virtual ~NetworkAnalogInT() = default;

		virtual std::string GetName() const override { return Name(); }

	protected:
		class CallDataGetHardwareLimits
			: public gRPCModule<gRPCServices...>::template TypedCallDataBase<CallDataGetHardwareLimits, ThisServiceType, DynExpProto::Common::VoidMessage, DynExpProto::NetworkAnalogIn::ValueLimitsMessage>
		{
			using Base = gRPCModule<gRPCServices...>::template TypedCallDataBase<CallDataGetHardwareLimits, ThisServiceType, DynExpProto::Common::VoidMessage, DynExpProto::NetworkAnalogIn::ValueLimitsMessage>;
			using Base::ResponseMessage;

		public:
			CallDataGetHardwareLimits(const gRPCModule<gRPCServices...>* const OwningModule) noexcept
				: Base::TypedCallDataBase(OwningModule, &DynExpProto::NetworkAnalogIn::NetworkAnalogIn::AsyncService::RequestGetHardwareLimits) {}
			virtual ~CallDataGetHardwareLimits() = default;

		private:
			virtual void ProcessChildImpl(DynExp::ModuleInstance& Instance) override
			{
				auto ModuleData = DynExp::dynamic_ModuleData_cast<NetworkAnalogInT>(Instance.ModuleDataGetter());

				ResponseMessage.set_lowerlimit(ModuleData->GetAnalogIn()->GetHardwareMinValue());
				ResponseMessage.set_upperlimit(ModuleData->GetAnalogIn()->GetHardwareMaxValue());
			}
		};

		class CallDataHardwareResolution
			: public gRPCModule<gRPCServices...>::template TypedCallDataBase<CallDataHardwareResolution, ThisServiceType, DynExpProto::Common::VoidMessage, DynExpProto::NetworkAnalogIn::ValueResolutionMessage>
		{
			using Base = gRPCModule<gRPCServices...>::template TypedCallDataBase<CallDataHardwareResolution, ThisServiceType, DynExpProto::Common::VoidMessage, DynExpProto::NetworkAnalogIn::ValueResolutionMessage>;
			using Base::ResponseMessage;

		public:
			CallDataHardwareResolution(const gRPCModule<gRPCServices...>* const OwningModule) noexcept
				: Base::TypedCallDataBase(OwningModule, &DynExpProto::NetworkAnalogIn::NetworkAnalogIn::AsyncService::RequestGetHardwareResolution) {}
			virtual ~CallDataHardwareResolution() = default;

		private:
			virtual void ProcessChildImpl(DynExp::ModuleInstance& Instance) override
			{
				auto ModuleData = DynExp::dynamic_ModuleData_cast<NetworkAnalogInT>(Instance.ModuleDataGetter());

				ResponseMessage.set_resolution(ModuleData->GetAnalogIn()->GetHardwareResolution());
			}
		};

		class CallDataGetValueUnit
			: public gRPCModule<gRPCServices...>::template TypedCallDataBase<CallDataGetValueUnit, ThisServiceType, DynExpProto::Common::VoidMessage, DynExpProto::NetworkAnalogIn::ValueUnitMessage>
		{
			using Base = gRPCModule<gRPCServices...>::template TypedCallDataBase<CallDataGetValueUnit, ThisServiceType, DynExpProto::Common::VoidMessage, DynExpProto::NetworkAnalogIn::ValueUnitMessage>;
			using Base::ResponseMessage;

		public:
			CallDataGetValueUnit(const gRPCModule<gRPCServices...>* const OwningModule) noexcept
				: Base::TypedCallDataBase(OwningModule, &DynExpProto::NetworkAnalogIn::NetworkAnalogIn::AsyncService::RequestGetValueUnit) {}
			virtual ~CallDataGetValueUnit() = default;

		private:
			virtual void ProcessChildImpl(DynExp::ModuleInstance& Instance) override
			{
				auto ModuleData = DynExp::dynamic_ModuleData_cast<NetworkAnalogInT>(Instance.ModuleDataGetter());

				ResponseMessage.set_unit(DynExpInstr::ToPrototUnitType(ModuleData->GetAnalogIn()->GetValueUnit()));
			}
		};

		class CallDataGetAsync
			: public gRPCModule<gRPCServices...>::template TypedCallDataBase<CallDataGetAsync, ThisServiceType, DynExpProto::Common::VoidMessage, DynExpProto::NetworkAnalogIn::SampleMessage>
		{
			using Base = gRPCModule<gRPCServices...>::template TypedCallDataBase<CallDataGetAsync, ThisServiceType, DynExpProto::Common::VoidMessage, DynExpProto::NetworkAnalogIn::SampleMessage>;
			using Base::ResponseMessage;

		public:
			CallDataGetAsync(const gRPCModule<gRPCServices...>* const OwningModule) noexcept
				: Base::TypedCallDataBase(OwningModule, &DynExpProto::NetworkAnalogIn::NetworkAnalogIn::AsyncService::RequestGetAsync) {}
			virtual ~CallDataGetAsync() = default;

		private:
			virtual void ProcessChildImpl(DynExp::ModuleInstance& Instance) override
			{
				auto ModuleData = DynExp::dynamic_ModuleData_cast<NetworkAnalogInT>(Instance.ModuleDataGetter());

				ResponseMessage.set_value(ModuleData->GetAnalogIn()->Get());
			}
		};

		class CallDataGetSync
			: public gRPCModule<gRPCServices...>::template TypedCallDataBase<CallDataGetSync, ThisServiceType, DynExpProto::Common::VoidMessage, DynExpProto::NetworkAnalogIn::SampleMessage>
		{
			using Base = gRPCModule<gRPCServices...>::template TypedCallDataBase<CallDataGetSync, ThisServiceType, DynExpProto::Common::VoidMessage, DynExpProto::NetworkAnalogIn::SampleMessage>;
			using Base::ResponseMessage;

		public:
			CallDataGetSync(const gRPCModule<gRPCServices...>* const OwningModule) noexcept
				: Base::TypedCallDataBase(OwningModule, &DynExpProto::NetworkAnalogIn::NetworkAnalogIn::AsyncService::RequestGetSync) {}
			virtual ~CallDataGetSync() = default;

		private:
			virtual void ProcessChildImpl(DynExp::ModuleInstance& Instance) override
			{
				auto ModuleData = DynExp::dynamic_ModuleData_cast<NetworkAnalogInT>(Instance.ModuleDataGetter());

				ResponseMessage.set_value(ModuleData->GetAnalogIn()->GetSync());
			}
		};

	private:
		void ResetImpl(DynExp::Object::dispatch_tag<NetworkDataStreamInstrumentT<gRPCServices...>>) override final
		{
			ResetImpl(DynExp::Object::dispatch_tag<NetworkAnalogInT>());
		}

		virtual void ResetImpl(DynExp::Object::dispatch_tag<NetworkAnalogInT>) {}

		void CreateInitialCallDataObjectsImpl(DynExp::Object::dispatch_tag<NetworkDataStreamInstrumentT<gRPCServices...>>, DynExp::ModuleInstance& Instance) const override final
		{
			CallDataGetHardwareLimits::MakeCall(this, Instance);
			CallDataHardwareResolution::MakeCall(this, Instance);
			CallDataGetValueUnit::MakeCall(this, Instance);
			CallDataGetAsync::MakeCall(this, Instance);
			CallDataGetSync::MakeCall(this, Instance);

			CreateInitialCallDataObjectsImpl(DynExp::Object::dispatch_tag<NetworkAnalogInT>(), Instance);
		}

		virtual void CreateInitialCallDataObjectsImpl(DynExp::Object::dispatch_tag<NetworkAnalogInT>, DynExp::ModuleInstance& Instance) const {}

		virtual void ValidateInstrType(const DynExpInstr::DataStreamInstrument* Instr) const override { DynExp::dynamic_Object_cast<typename ModuleDataType::InstrType>(Instr); }
	};

	/**
	 * @brief Explicit instantiation of derivable class NetworkAnalogInT to create the network analog in module.
	*/
	using NetworkAnalogIn = NetworkAnalogInT<typename NetworkDataStreamInstrument::ThisServiceType, DynExpProto::NetworkAnalogIn::NetworkAnalogIn>;
}