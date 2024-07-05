// This file is part of DynExp.

/**
 * @file NetworkDataStreamInstrumentModule.h
 * @brief Implementation of a gRPC server module to provide remote access to a data stream instrument
 * meta instrument.
*/

#pragma once

#include "stdafx.h"
#include "../Instruments/NetworkDataStreamInstrument.h"
#include "gRPCModule.h"

#include "NetworkDataStreamInstrument.grpc.pb.h"

namespace DynExpModule
{
	template <typename... gRPCServices>
	class NetworkDataStreamInstrumentT;

	template <typename... gRPCServices>
	class NetworkDataStreamInstrumentData : public gRPCModuleData<gRPCServices...>
	{
	public:
		NetworkDataStreamInstrumentData() { Init(); }
		virtual ~NetworkDataStreamInstrumentData() = default;

		auto& GetDataStreamInstrument() noexcept { return DataStreamInstrument; }

	private:
		void ResetImpl(DynExp::ModuleDataBase::dispatch_tag<gRPCModuleData<gRPCServices...>>) override final { Init(); }
		virtual void ResetImpl(DynExp::ModuleDataBase::dispatch_tag<NetworkDataStreamInstrumentData>) {};

		void Init() {}

		DynExp::LinkedObjectWrapperContainer<DynExpInstr::DataStreamInstrument> DataStreamInstrument;
	};

	template <typename... gRPCServices>
	class NetworkDataStreamInstrumentParams : public gRPCModuleParams<gRPCServices...>
	{
	public:
		NetworkDataStreamInstrumentParams(DynExp::ItemIDType ID, const DynExp::DynExpCore& Core) : gRPCModuleParams<gRPCServices...>(ID, Core) {}
		virtual ~NetworkDataStreamInstrumentParams() = default;

		virtual const char* GetParamClassTag() const noexcept override { return "NetworkDataStreamInstrumentParams"; }

		DynExp::ParamsBase::Param<DynExp::ObjectLink<DynExpInstr::DataStreamInstrument>> DataStreamInstrument = { *this, DynExp::ParamsBase::GetCore().GetInstrumentManager(),
			"DataStreamInstrument", "Data stream instrument", "Data stream instrument to receive data from with this network module", DynExpUI::Icons::Instrument };

	private:
		void ConfigureParamsImpl(DynExp::ParamsBase::dispatch_tag<gRPCModuleParams<gRPCServices...>>) override final { ConfigureParamsImpl(DynExp::ParamsBase::dispatch_tag<NetworkDataStreamInstrumentParams>()); }
		virtual void ConfigureParamsImpl(DynExp::ParamsBase::dispatch_tag<NetworkDataStreamInstrumentParams>) {}
	};

	template <typename... gRPCServices>
	class NetworkDataStreamInstrumentConfigurator : public gRPCModuleConfigurator<gRPCServices...>
	{
	public:
		using ObjectType = NetworkDataStreamInstrumentT<gRPCServices...>;
		using ParamsType = NetworkDataStreamInstrumentParams<gRPCServices...>;

		NetworkDataStreamInstrumentConfigurator() = default;
		virtual ~NetworkDataStreamInstrumentConfigurator() = default;

	private:
		virtual DynExp::ParamsBasePtrType MakeParams(DynExp::ItemIDType ID, const DynExp::DynExpCore& Core) const override { return DynExp::MakeParams<NetworkDataStreamInstrumentConfigurator>(ID, Core); }
	};

	template <typename... gRPCServices>
	class NetworkDataStreamInstrumentT : public gRPCModule<gRPCServices...>
	{
	public:
		using ParamsType = NetworkDataStreamInstrumentParams<gRPCServices...>;
		using ConfigType = NetworkDataStreamInstrumentConfigurator<gRPCServices...>;
		using ModuleDataType = NetworkDataStreamInstrumentData<gRPCServices...>;
		using ThisServiceType = DynExpProto::NetworkDataStreamInstrument::NetworkDataStreamInstrument;

		constexpr static auto Name() noexcept { return "Network Data Stream Instrument"; }

		NetworkDataStreamInstrumentT(const std::thread::id OwnerThreadID, DynExp::ParamsBasePtrType&& Params)
			: gRPCModule<gRPCServices...>(OwnerThreadID, std::move(Params)) {}
		virtual ~NetworkDataStreamInstrumentT() = default;

		virtual std::string GetName() const override { return Name(); }

	protected:
		class CallDataGetStreamInfo
			: public gRPCModule<gRPCServices...>::template TypedCallDataBase<CallDataGetStreamInfo, ThisServiceType, DynExpProto::Common::VoidMessage, DynExpProto::NetworkDataStreamInstrument::StreamInfoMessage>
		{
			using Base = gRPCModule<gRPCServices...>::template TypedCallDataBase<CallDataGetStreamInfo, ThisServiceType, DynExpProto::Common::VoidMessage, DynExpProto::NetworkDataStreamInstrument::StreamInfoMessage>;
			using Base::ResponseMessage;

		public:
			CallDataGetStreamInfo(const gRPCModule<gRPCServices...>* const OwningModule) noexcept
				: Base::TypedCallDataBase(OwningModule, &ThisServiceType::AsyncService::RequestGetStreamInfo) {}
			virtual ~CallDataGetStreamInfo() = default;

		private:
			virtual void ProcessChildImpl(DynExp::ModuleInstance& Instance) override
			{
				auto StreamSizeMsg = std::make_unique<DynExpProto::NetworkDataStreamInstrument::StreamSizeMessage>();
				auto ModuleData = DynExp::dynamic_ModuleData_cast<NetworkDataStreamInstrumentT>(Instance.ModuleDataGetter());
				auto Instrument = ModuleData->GetDataStreamInstrument().get();
				auto InstrData = DynExp::dynamic_InstrumentData_cast<DynExpInstr::DataStreamInstrument>(Instrument->GetInstrumentData());

				ResponseMessage.set_valueunit(DynExpInstr::ToPrototUnitType(Instrument->GetValueUnit()));
				ResponseMessage.set_hardwareminvalue(InstrData->GetHardwareMinValue());
				ResponseMessage.set_hardwaremaxvalue(InstrData->GetHardwareMaxValue());
				ResponseMessage.set_isbasicsampletimeused(InstrData->GetSampleStream()->IsBasicSampleTimeUsed());

				StreamSizeMsg->set_streamsizeread(Util::NumToT<google::protobuf::uint64>(InstrData->GetSampleStream()->GetStreamSizeRead()));
				StreamSizeMsg->set_streamsizewrite(Util::NumToT<google::protobuf::uint64>(InstrData->GetSampleStream()->GetStreamSizeWrite()));
				ResponseMessage.set_allocated_streamsizemsg(StreamSizeMsg.release());
			}
		};

		class CallDataRead
			: public gRPCModule<gRPCServices...>::template TypedCallDataBase<CallDataRead, ThisServiceType, DynExpProto::NetworkDataStreamInstrument::ReadMessage, DynExpProto::NetworkDataStreamInstrument::ReadResultMessage>
		{
			using Base = gRPCModule<gRPCServices...>::template TypedCallDataBase<CallDataRead, ThisServiceType, DynExpProto::NetworkDataStreamInstrument::ReadMessage, DynExpProto::NetworkDataStreamInstrument::ReadResultMessage>;
			using Base::RequestMessage;
			using Base::ResponseMessage;

		public:
			CallDataRead(const gRPCModule<gRPCServices...>* const OwningModule) noexcept
				: Base::TypedCallDataBase(OwningModule, &ThisServiceType::AsyncService::RequestRead) {}
			virtual ~CallDataRead() = default;

		private:
			virtual void ProcessChildImpl(DynExp::ModuleInstance& Instance) override
			{
				auto StartSample = Util::NumToT<size_t>(RequestMessage.startsampleid());
				auto ModuleData = DynExp::dynamic_ModuleData_cast<NetworkDataStreamInstrumentT>(Instance.ModuleDataGetter());
				auto Instrument = ModuleData->GetDataStreamInstrument().get();
				auto InstrData = DynExp::dynamic_InstrumentData_cast<DynExpInstr::DataStreamInstrument>(Instrument->GetInstrumentData());
				auto SampleStream = InstrData->template GetCastSampleStream<DynExpInstr::CircularDataStreamBase>();

				Instrument->ReadData();

				if (SampleStream->GetNumSamplesWritten() == StartSample)
				{
					ResponseMessage.set_lastsampleid(Util::NumToT<google::protobuf::uint64>(StartSample));
					return;
				}
				if (SampleStream->GetNumSamplesWritten() < StartSample)
					StartSample = 0;	// e.g. if SampleStream has been cleared. Transmit the entire buffer then.

				auto Samples = SampleStream->ReadRecentBasicSamples(StartSample);

				ResponseMessage.set_lastsampleid(Util::NumToT<google::protobuf::uint64>(SampleStream->GetNumSamplesWritten()));
				for (const auto& Sample : Samples)
				{
					auto BasicSampleMsg = ResponseMessage.add_samples();
					BasicSampleMsg->set_value(Sample.Value);
					BasicSampleMsg->set_time(Sample.Time);
				}
			}
		};

		class CallDataWrite
			: public gRPCModule<gRPCServices...>::template TypedCallDataBase<CallDataWrite, ThisServiceType, DynExpProto::NetworkDataStreamInstrument::WriteMessage, DynExpProto::NetworkDataStreamInstrument::WriteResultMessage>
		{
			using Base = gRPCModule<gRPCServices...>::template TypedCallDataBase<CallDataWrite, ThisServiceType, DynExpProto::NetworkDataStreamInstrument::WriteMessage, DynExpProto::NetworkDataStreamInstrument::WriteResultMessage>;
			using Base::RequestMessage;
			using Base::ResponseMessage;

		public:
			CallDataWrite(const gRPCModule<gRPCServices...>* const OwningModule) noexcept
				: Base::TypedCallDataBase(OwningModule, &ThisServiceType::AsyncService::RequestWrite) {}
			virtual ~CallDataWrite() = default;

		private:
			virtual void ProcessChildImpl(DynExp::ModuleInstance& Instance) override
			{
				auto ModuleData = DynExp::dynamic_ModuleData_cast<NetworkDataStreamInstrumentT>(Instance.ModuleDataGetter());
				auto Instrument = ModuleData->GetDataStreamInstrument().get();
				auto InstrData = DynExp::dynamic_InstrumentData_cast<DynExpInstr::DataStreamInstrument>(Instrument->GetInstrumentData());

				for (decltype(RequestMessage.samples_size()) i = 0; i < RequestMessage.samples_size(); ++i)
					InstrData->GetSampleStream()->WriteBasicSample({ RequestMessage.samples(i).value(), RequestMessage.samples(i).time() });
				Instrument->WriteData();

				ResponseMessage.set_lastsampleid(InstrData->GetSampleStream()->GetNumSamplesWritten());
			}
		};

		class CallDataClearData
			: public gRPCModule<gRPCServices...>::template TypedCallDataBase<CallDataClearData, ThisServiceType, DynExpProto::Common::VoidMessage, DynExpProto::Common::VoidMessage>
		{
			using Base = gRPCModule<gRPCServices...>::template TypedCallDataBase<CallDataClearData, ThisServiceType, DynExpProto::Common::VoidMessage, DynExpProto::Common::VoidMessage>;

		public:
			CallDataClearData(const gRPCModule<gRPCServices...>* const OwningModule) noexcept
				: Base::TypedCallDataBase(OwningModule, &ThisServiceType::AsyncService::RequestClearData) {}
			virtual ~CallDataClearData() = default;

		private:
			virtual void ProcessChildImpl(DynExp::ModuleInstance& Instance) override
			{
				auto ModuleData = DynExp::dynamic_ModuleData_cast<NetworkDataStreamInstrumentT>(Instance.ModuleDataGetter());

				ModuleData->GetDataStreamInstrument()->Clear();
			}
		};

		class CallDataStart
			: public gRPCModule<gRPCServices...>::template TypedCallDataBase<CallDataStart, ThisServiceType, DynExpProto::Common::VoidMessage, DynExpProto::Common::VoidMessage>
		{
			using Base = gRPCModule<gRPCServices...>::template TypedCallDataBase<CallDataStart, ThisServiceType, DynExpProto::Common::VoidMessage, DynExpProto::Common::VoidMessage>;

		public:
			CallDataStart(const gRPCModule<gRPCServices...>* const OwningModule) noexcept
				: Base::TypedCallDataBase(OwningModule, &ThisServiceType::AsyncService::RequestStart) {}
			virtual ~CallDataStart() = default;

		private:
			virtual void ProcessChildImpl(DynExp::ModuleInstance& Instance) override
			{
				auto ModuleData = DynExp::dynamic_ModuleData_cast<NetworkDataStreamInstrumentT>(Instance.ModuleDataGetter());

				ModuleData->GetDataStreamInstrument()->Start();
			}
		};

		class CallDataStop
			: public gRPCModule<gRPCServices...>::template TypedCallDataBase<CallDataStop, ThisServiceType, DynExpProto::Common::VoidMessage, DynExpProto::Common::VoidMessage>
		{
			using Base = gRPCModule<gRPCServices...>::template TypedCallDataBase<CallDataStop, ThisServiceType, DynExpProto::Common::VoidMessage, DynExpProto::Common::VoidMessage>;

		public:
			CallDataStop(const gRPCModule<gRPCServices...>* const OwningModule) noexcept
				: Base::TypedCallDataBase(OwningModule, &ThisServiceType::AsyncService::RequestStop) {}
			virtual ~CallDataStop() = default;

		private:
			virtual void ProcessChildImpl(DynExp::ModuleInstance& Instance) override
			{
				auto ModuleData = DynExp::dynamic_ModuleData_cast<NetworkDataStreamInstrumentT>(Instance.ModuleDataGetter());

				ModuleData->GetDataStreamInstrument()->Stop();
			}
		};

		class CallDataRestart
			: public gRPCModule<gRPCServices...>::template TypedCallDataBase<CallDataRestart, ThisServiceType, DynExpProto::Common::VoidMessage, DynExpProto::Common::VoidMessage>
		{
			using Base = gRPCModule<gRPCServices...>::template TypedCallDataBase<CallDataRestart, ThisServiceType, DynExpProto::Common::VoidMessage, DynExpProto::Common::VoidMessage>;

		public:
			CallDataRestart(const gRPCModule<gRPCServices...>* const OwningModule) noexcept
				: Base::TypedCallDataBase(OwningModule, &ThisServiceType::AsyncService::RequestRestart) {}
			virtual ~CallDataRestart() = default;

		private:
			virtual void ProcessChildImpl(DynExp::ModuleInstance& Instance) override
			{
				auto ModuleData = DynExp::dynamic_ModuleData_cast<NetworkDataStreamInstrumentT>(Instance.ModuleDataGetter());

				ModuleData->GetDataStreamInstrument()->Restart();
			}
		};

		class CallDataGetStreamSize
			: public gRPCModule<gRPCServices...>::template TypedCallDataBase<CallDataGetStreamSize, ThisServiceType, DynExpProto::Common::VoidMessage, DynExpProto::NetworkDataStreamInstrument::StreamSizeMessage>
		{
			using Base = gRPCModule<gRPCServices...>::template TypedCallDataBase<CallDataGetStreamSize, ThisServiceType, DynExpProto::Common::VoidMessage, DynExpProto::NetworkDataStreamInstrument::StreamSizeMessage>;
			using Base::ResponseMessage;

		public:
			CallDataGetStreamSize(const gRPCModule<gRPCServices...>* const OwningModule) noexcept
				: Base::TypedCallDataBase(OwningModule, &ThisServiceType::AsyncService::RequestGetStreamSize) {}
			virtual ~CallDataGetStreamSize() = default;

		private:
			virtual void ProcessChildImpl(DynExp::ModuleInstance& Instance) override
			{
				auto ModuleData = DynExp::dynamic_ModuleData_cast<NetworkDataStreamInstrumentT>(Instance.ModuleDataGetter());
				auto Instrument = ModuleData->GetDataStreamInstrument().get();
				auto InstrData = DynExp::dynamic_InstrumentData_cast<DynExpInstr::DataStreamInstrument>(Instrument->GetInstrumentData());

				ResponseMessage.set_streamsizeread(Util::NumToT<google::protobuf::uint64>(InstrData->GetSampleStream()->GetStreamSizeRead()));
				ResponseMessage.set_streamsizewrite(Util::NumToT<google::protobuf::uint64>(InstrData->GetSampleStream()->GetStreamSizeWrite()));
			}
		};

		class CallDataSetStreamSize
			: public gRPCModule<gRPCServices...>::template TypedCallDataBase<CallDataSetStreamSize, ThisServiceType, DynExpProto::NetworkDataStreamInstrument::StreamSizeMessage, DynExpProto::NetworkDataStreamInstrument::StreamSizeMessage>
		{
			using Base = gRPCModule<gRPCServices...>::template TypedCallDataBase<CallDataSetStreamSize, ThisServiceType, DynExpProto::NetworkDataStreamInstrument::StreamSizeMessage, DynExpProto::NetworkDataStreamInstrument::StreamSizeMessage>;
			using Base::RequestMessage;
			using Base::ResponseMessage;

		public:
			CallDataSetStreamSize(const gRPCModule<gRPCServices...>* const OwningModule) noexcept
				: Base::TypedCallDataBase(OwningModule, &ThisServiceType::AsyncService::RequestSetStreamSize) {}
			virtual ~CallDataSetStreamSize() = default;

		private:
			virtual void ProcessChildImpl(DynExp::ModuleInstance& Instance) override
			{
				auto ModuleData = DynExp::dynamic_ModuleData_cast<NetworkDataStreamInstrumentT>(Instance.ModuleDataGetter());
				ModuleData->GetDataStreamInstrument()->AsSyncTask(&DynExpInstr::DataStreamInstrument::SetStreamSize, Util::NumToT<size_t>(RequestMessage.streamsizewrite()));

				auto Instrument = ModuleData->GetDataStreamInstrument().get();
				auto InstrData = DynExp::dynamic_InstrumentData_cast<DynExpInstr::DataStreamInstrument>(Instrument->GetInstrumentData());
				ResponseMessage.set_streamsizeread(Util::NumToT<google::protobuf::uint64>(InstrData->GetSampleStream()->GetStreamSizeRead()));
				ResponseMessage.set_streamsizewrite(Util::NumToT<google::protobuf::uint64>(InstrData->GetSampleStream()->GetStreamSizeWrite()));
			}
		};

		class CallDataResetStreamSize
			: public gRPCModule<gRPCServices...>::template TypedCallDataBase<CallDataResetStreamSize, ThisServiceType, DynExpProto::Common::VoidMessage, DynExpProto::NetworkDataStreamInstrument::StreamSizeMessage>
		{
			using Base = gRPCModule<gRPCServices...>::template TypedCallDataBase<CallDataResetStreamSize, ThisServiceType, DynExpProto::Common::VoidMessage, DynExpProto::NetworkDataStreamInstrument::StreamSizeMessage>;
			using Base::ResponseMessage;

		public:
			CallDataResetStreamSize(const gRPCModule<gRPCServices...>* const OwningModule) noexcept
				: Base::TypedCallDataBase(OwningModule, &ThisServiceType::AsyncService::RequestResetStreamSize) {}
			virtual ~CallDataResetStreamSize() = default;

		private:
			virtual void ProcessChildImpl(DynExp::ModuleInstance& Instance) override
			{
				auto ModuleData = DynExp::dynamic_ModuleData_cast<NetworkDataStreamInstrumentT>(Instance.ModuleDataGetter());
				ModuleData->GetDataStreamInstrument()->AsSyncTask(&DynExpInstr::DataStreamInstrument::ResetStreamSize);

				auto Instrument = ModuleData->GetDataStreamInstrument().get();
				auto InstrData = DynExp::dynamic_InstrumentData_cast<DynExpInstr::DataStreamInstrument>(Instrument->GetInstrumentData());
				ResponseMessage.set_streamsizeread(Util::NumToT<google::protobuf::uint64>(InstrData->GetSampleStream()->GetStreamSizeRead()));
				ResponseMessage.set_streamsizewrite(Util::NumToT<google::protobuf::uint64>(InstrData->GetSampleStream()->GetStreamSizeWrite()));
			}
		};

		class CallDataHasFinished
			: public gRPCModule<gRPCServices...>::template TypedCallDataBase<CallDataHasFinished, ThisServiceType, DynExpProto::Common::VoidMessage, DynExpProto::Common::OptionalBoolMessage>
		{
			using Base = gRPCModule<gRPCServices...>::template TypedCallDataBase<CallDataHasFinished, ThisServiceType, DynExpProto::Common::VoidMessage, DynExpProto::Common::OptionalBoolMessage>;
			using Base::ResponseMessage;

		public:
			CallDataHasFinished(const gRPCModule<gRPCServices...>* const OwningModule) noexcept
				: Base::TypedCallDataBase(OwningModule, &ThisServiceType::AsyncService::RequestHasFinished) {}
			virtual ~CallDataHasFinished() = default;

		private:
			virtual void ProcessChildImpl(DynExp::ModuleInstance& Instance) override
			{
				auto ModuleData = DynExp::dynamic_ModuleData_cast<NetworkDataStreamInstrumentT>(Instance.ModuleDataGetter());

				auto Finished = ModuleData->GetDataStreamInstrument()->HasFinished();
				if (Finished != Util::OptionalBool::Values::Unknown)
					ResponseMessage.set_value(Finished);
			}
		};

		class CallDataIsRunning
			: public gRPCModule<gRPCServices...>::template TypedCallDataBase<CallDataIsRunning, ThisServiceType, DynExpProto::Common::VoidMessage, DynExpProto::Common::OptionalBoolMessage>
		{
			using Base = gRPCModule<gRPCServices...>::template TypedCallDataBase<CallDataIsRunning, ThisServiceType, DynExpProto::Common::VoidMessage, DynExpProto::Common::OptionalBoolMessage>;
			using Base::ResponseMessage;

		public:
			CallDataIsRunning(const gRPCModule<gRPCServices...>* const OwningModule) noexcept
				: Base::TypedCallDataBase(OwningModule, &ThisServiceType::AsyncService::RequestIsRunning) {}
			virtual ~CallDataIsRunning() = default;

		private:
			virtual void ProcessChildImpl(DynExp::ModuleInstance& Instance) override
			{
				auto ModuleData = DynExp::dynamic_ModuleData_cast<NetworkDataStreamInstrumentT>(Instance.ModuleDataGetter());

				auto Running = ModuleData->GetDataStreamInstrument()->IsRunning();
				if (Running != Util::OptionalBool::Values::Unknown)
					ResponseMessage.set_value(Running);
			}
		};

	private:
		void ResetImpl(DynExp::Object::dispatch_tag<gRPCModule<gRPCServices...>>) override final
		{
			ResetImpl(DynExp::Object::dispatch_tag<NetworkDataStreamInstrumentT>());
		}

		virtual void ResetImpl(DynExp::Object::dispatch_tag<NetworkDataStreamInstrumentT>) {}

		virtual void CreateInitialCallDataObjectsImpl(DynExp::Object::dispatch_tag<gRPCModule<gRPCServices...>>, DynExp::ModuleInstance& Instance) const override
		{
			CallDataGetStreamInfo::MakeCall(this, Instance);
			CallDataRead::MakeCall(this, Instance);
			CallDataWrite::MakeCall(this, Instance);
			CallDataClearData::MakeCall(this, Instance);
			CallDataStart::MakeCall(this, Instance);
			CallDataStop::MakeCall(this, Instance);
			CallDataRestart::MakeCall(this, Instance);
			CallDataGetStreamSize::MakeCall(this, Instance);
			CallDataSetStreamSize::MakeCall(this, Instance);
			CallDataResetStreamSize::MakeCall(this, Instance);
			CallDataHasFinished::MakeCall(this, Instance);
			CallDataIsRunning::MakeCall(this, Instance);

			CreateInitialCallDataObjectsImpl(DynExp::Object::dispatch_tag<NetworkDataStreamInstrumentT>(), Instance);
		}

		virtual void CreateInitialCallDataObjectsImpl(DynExp::Object::dispatch_tag<NetworkDataStreamInstrumentT>, DynExp::ModuleInstance& Instance) const {}

		virtual void OnInitChild(DynExp::ModuleInstance* Instance) const override
		{
			auto ModuleParams = DynExp::dynamic_Params_cast<NetworkDataStreamInstrumentT>(Instance->ParamsGetter());
			auto ModuleData = DynExp::dynamic_ModuleData_cast<NetworkDataStreamInstrumentT>(Instance->ModuleDataGetter());

			Instance->LockObject(ModuleParams->DataStreamInstrument, ModuleData->GetDataStreamInstrument());

			try
			{
				ValidateInstrType(ModuleData->GetDataStreamInstrument().get());
			}
			catch ([[maybe_unused]] const Util::TypeErrorException& e)
			{
				Util::EventLog().Log("Terminating module \"" + DynExp::Object::GetObjectName() + "\" (" + DynExp::Object::GetCategoryAndName()
					+ ") because an instrument assigned to this gRPC data stream server does not match the expected type.", Util::ErrorType::Error);

				throw;
			}
		}

		virtual void OnExitChild(DynExp::ModuleInstance* Instance) const override
		{
			auto ModuleData = DynExp::dynamic_ModuleData_cast<NetworkDataStreamInstrumentT>(Instance->ModuleDataGetter());

			Instance->UnlockObject(ModuleData->GetDataStreamInstrument());
		}

		virtual void ValidateInstrType(const DynExpInstr::DataStreamInstrument* Instr) const {}
	};

	/**
	 * @brief Explicit instantiation of derivable class NetworkDataStreamInstrumentT to create the network data stream module.
	*/
	using NetworkDataStreamInstrument = NetworkDataStreamInstrumentT<DynExpProto::NetworkDataStreamInstrument::NetworkDataStreamInstrument>;
}