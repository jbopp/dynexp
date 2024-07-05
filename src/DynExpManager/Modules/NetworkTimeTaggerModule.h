// This file is part of DynExp.

/**
 * @file NetworkTimeTaggerModule.h
 * @brief Implementation of a gRPC server module to provide remote access to a time tagger
 * meta instrument.
*/

#pragma once

#include "stdafx.h"
#include "../MetaInstruments/TimeTagger.h"
#include "NetworkDataStreamInstrumentModule.h"

#include "NetworkTimeTagger.grpc.pb.h"

namespace DynExpModule
{
	template <typename... gRPCServices>
	class NetworkTimeTaggerT;

	template <typename... gRPCServices>
	class NetworkTimeTaggerData : public NetworkDataStreamInstrumentData<gRPCServices...>
	{
	public:
		using InstrType = DynExpInstr::TimeTagger;

		NetworkTimeTaggerData() { Init(); }
		virtual ~NetworkTimeTaggerData() = default;

		auto GetTimeTagger()
		{
			// Retry a couple of times since time tagger sometimes blocks
			// for a while due to slow operations like activating the HBT.
			for (int NumTries = 0; true; ++NumTries)
			{
				try
				{
					return DynExp::dynamic_Object_cast<InstrType>(NetworkDataStreamInstrumentData<gRPCServices...>::GetDataStreamInstrument().get());
				}
				catch ([[maybe_unused]] const Util::TimeoutException& e)
				{
					if (NumTries < 5)
						continue;
					else
						throw;
				}
			}
		}

	private:
		void ResetImpl(DynExp::ModuleDataBase::dispatch_tag<NetworkDataStreamInstrumentData<gRPCServices...>>) override final { Init(); }
		virtual void ResetImpl(DynExp::ModuleDataBase::dispatch_tag<NetworkTimeTaggerData>) {};

		void Init() {}
	};

	template <typename... gRPCServices>
	class NetworkTimeTaggerParams : public NetworkDataStreamInstrumentParams<gRPCServices...>
	{
	public:
		NetworkTimeTaggerParams(DynExp::ItemIDType ID, const DynExp::DynExpCore& Core) : NetworkDataStreamInstrumentParams<gRPCServices...>(ID, Core) {}
		virtual ~NetworkTimeTaggerParams() = default;

		virtual const char* GetParamClassTag() const noexcept override { return "NetworkTimeTaggerParams"; }

	private:
		void ConfigureParamsImpl(DynExp::ParamsBase::dispatch_tag<NetworkDataStreamInstrumentParams<gRPCServices...>>) override final { ConfigureParamsImpl(DynExp::ParamsBase::dispatch_tag<NetworkTimeTaggerParams>()); }
		virtual void ConfigureParamsImpl(DynExp::ParamsBase::dispatch_tag<NetworkTimeTaggerParams>) {}

		DynExp::ParamsBase::DummyParam Dummy = { *this };
	};

	template <typename... gRPCServices>
	class NetworkTimeTaggerConfigurator : public NetworkDataStreamInstrumentConfigurator<gRPCServices...>
	{
	public:
		using ObjectType = NetworkTimeTaggerT<gRPCServices...>;
		using ParamsType = NetworkTimeTaggerParams<gRPCServices...>;

		NetworkTimeTaggerConfigurator() = default;
		virtual ~NetworkTimeTaggerConfigurator() = default;

	private:
		virtual DynExp::ParamsBasePtrType MakeParams(DynExp::ItemIDType ID, const DynExp::DynExpCore& Core) const override { return DynExp::MakeParams<NetworkTimeTaggerConfigurator>(ID, Core); }
	};

	template <typename... gRPCServices>
	class NetworkTimeTaggerT : public NetworkDataStreamInstrumentT<gRPCServices...>
	{
	public:
		using ParamsType = NetworkTimeTaggerParams<gRPCServices...>;
		using ConfigType = NetworkTimeTaggerConfigurator<gRPCServices...>;
		using ModuleDataType = NetworkTimeTaggerData<gRPCServices...>;
		using ThisServiceType = DynExpProto::NetworkTimeTagger::NetworkTimeTagger;

		constexpr static auto Name() noexcept { return "Network Time Tagger"; }

		NetworkTimeTaggerT(const std::thread::id OwnerThreadID, DynExp::ParamsBasePtrType&& Params)
			: NetworkDataStreamInstrumentT<gRPCServices...>(OwnerThreadID, std::move(Params)) {}
		virtual ~NetworkTimeTaggerT() = default;

		virtual std::string GetName() const override { return Name(); }

	protected:
		class CallDataGetHardwareInfo
			: public gRPCModule<gRPCServices...>::template TypedCallDataBase<CallDataGetHardwareInfo, ThisServiceType, DynExpProto::Common::VoidMessage, DynExpProto::NetworkTimeTagger::HardwareInfoMessage>
		{
			using Base = gRPCModule<gRPCServices...>::template TypedCallDataBase<CallDataGetHardwareInfo, ThisServiceType, DynExpProto::Common::VoidMessage, DynExpProto::NetworkTimeTagger::HardwareInfoMessage>;
			using Base::ResponseMessage;

		public:
			CallDataGetHardwareInfo(const gRPCModule<gRPCServices...>* const OwningModule) noexcept
				: Base::TypedCallDataBase(OwningModule, &ThisServiceType::AsyncService::RequestGetHardwareInfo) {}
			virtual ~CallDataGetHardwareInfo() = default;

		private:
			virtual void ProcessChildImpl(DynExp::ModuleInstance& Instance) override
			{
				auto StreamSizeMsg = std::make_unique<DynExpProto::NetworkTimeTagger::HardwareInfoMessage>();
				auto ModuleData = DynExp::dynamic_ModuleData_cast<NetworkTimeTaggerT>(Instance.ModuleDataGetter());
				auto Instrument = ModuleData->GetTimeTagger();

				ResponseMessage.set_mininputthresholdinvolts(Instrument->GetMinThresholdInVolts());
				ResponseMessage.set_maxinputthresholdinvolts(Instrument->GetMaxThresholdInVolts());
				ResponseMessage.set_timingresolutioninpicoseconds(Instrument->GetResolution().count());
			}
		};

		class CallDataGetBufferInfo
			: public gRPCModule<gRPCServices...>::template TypedCallDataBase<CallDataGetBufferInfo, ThisServiceType, DynExpProto::Common::VoidMessage, DynExpProto::NetworkTimeTagger::BufferInfoMessage>
		{
			using Base = gRPCModule<gRPCServices...>::template TypedCallDataBase<CallDataGetBufferInfo, ThisServiceType, DynExpProto::Common::VoidMessage, DynExpProto::NetworkTimeTagger::BufferInfoMessage>;
			using Base::ResponseMessage;

		public:
			CallDataGetBufferInfo(const gRPCModule<gRPCServices...>* const OwningModule) noexcept
				: Base::TypedCallDataBase(OwningModule, &ThisServiceType::AsyncService::RequestGetBufferInfo) {}
			virtual ~CallDataGetBufferInfo() = default;

		private:
			virtual void ProcessChildImpl(DynExp::ModuleInstance& Instance) override
			{
				auto StreamSizeMsg = std::make_unique<DynExpProto::NetworkTimeTagger::BufferInfoMessage>();
				auto ModuleData = DynExp::dynamic_ModuleData_cast<NetworkTimeTaggerT>(Instance.ModuleDataGetter());
				auto Instrument = ModuleData->GetTimeTagger();

				ResponseMessage.set_buffersizeinsamples(Instrument->GetBufferSize());
			}
		};

		class CallDataGetStreamMode
			: public gRPCModule<gRPCServices...>::template TypedCallDataBase<CallDataGetStreamMode, ThisServiceType, DynExpProto::Common::VoidMessage, DynExpProto::NetworkTimeTagger::StreamModeMessage>
		{
			using Base = gRPCModule<gRPCServices...>::template TypedCallDataBase<CallDataGetStreamMode, ThisServiceType, DynExpProto::Common::VoidMessage, DynExpProto::NetworkTimeTagger::StreamModeMessage>;
			using Base::ResponseMessage;

		public:
			CallDataGetStreamMode(const gRPCModule<gRPCServices...>* const OwningModule) noexcept
				: Base::TypedCallDataBase(OwningModule, &ThisServiceType::AsyncService::RequestGetStreamMode) {}
			virtual ~CallDataGetStreamMode() = default;

		private:
			virtual void ProcessChildImpl(DynExp::ModuleInstance& Instance) override
			{
				auto StreamSizeMsg = std::make_unique<DynExpProto::NetworkTimeTagger::StreamModeMessage>();
				auto ModuleData = DynExp::dynamic_ModuleData_cast<NetworkTimeTaggerT>(Instance.ModuleDataGetter());
				auto Instrument = ModuleData->GetTimeTagger();
				auto InstrData = DynExp::dynamic_InstrumentData_cast<DynExpInstr::TimeTagger>(Instrument->GetInstrumentData());

				ResponseMessage.set_streammode(InstrData->GetStreamMode() == DynExpInstr::TimeTaggerData::StreamModeType::Counts ?
					DynExpProto::NetworkTimeTagger::StreamModeType::Counts : DynExpProto::NetworkTimeTagger::StreamModeType::Events);
			}
		};

		class CallDataSetStreamMode
			: public gRPCModule<gRPCServices...>::template TypedCallDataBase<CallDataSetStreamMode, ThisServiceType, DynExpProto::NetworkTimeTagger::StreamModeMessage, DynExpProto::Common::VoidMessage>
		{
			using Base = gRPCModule<gRPCServices...>::template TypedCallDataBase<CallDataSetStreamMode, ThisServiceType, DynExpProto::NetworkTimeTagger::StreamModeMessage, DynExpProto::Common::VoidMessage>;
			using Base::RequestMessage;

		public:
			CallDataSetStreamMode(const gRPCModule<gRPCServices...>* const OwningModule) noexcept
				: Base::TypedCallDataBase(OwningModule, &ThisServiceType::AsyncService::RequestSetStreamMode) {}
			virtual ~CallDataSetStreamMode() = default;

		private:
			virtual void ProcessChildImpl(DynExp::ModuleInstance& Instance) override
			{
				auto ModuleData = DynExp::dynamic_ModuleData_cast<NetworkTimeTaggerT>(Instance.ModuleDataGetter());
				auto Instrument = ModuleData->GetTimeTagger();
				auto InstrData = DynExp::dynamic_InstrumentData_cast<DynExpInstr::TimeTagger>(Instrument->GetInstrumentData());

				InstrData->SetStreamMode(RequestMessage.streammode() == DynExpProto::NetworkTimeTagger::StreamModeType::Counts ?
					DynExpInstr::TimeTaggerData::StreamModeType::Counts : DynExpInstr::TimeTaggerData::StreamModeType::Events);
			}
		};

		class CallDataClearBuffer
			: public gRPCModule<gRPCServices...>::template TypedCallDataBase<CallDataClearBuffer, ThisServiceType, DynExpProto::Common::VoidMessage, DynExpProto::Common::VoidMessage>
		{
			using Base = gRPCModule<gRPCServices...>::template TypedCallDataBase<CallDataClearBuffer, ThisServiceType, DynExpProto::Common::VoidMessage, DynExpProto::Common::VoidMessage>;

		public:
			CallDataClearBuffer(const gRPCModule<gRPCServices...>* const OwningModule) noexcept
				: Base::TypedCallDataBase(OwningModule, &ThisServiceType::AsyncService::RequestClearBuffer) {}
			virtual ~CallDataClearBuffer() = default;

		private:
			virtual void ProcessChildImpl(DynExp::ModuleInstance& Instance) override
			{
				auto ModuleData = DynExp::dynamic_ModuleData_cast<NetworkTimeTaggerT>(Instance.ModuleDataGetter());
				auto Instrument = ModuleData->GetTimeTagger();

				Instrument->Clear();
			}
		};

		class CallDataConfigureInput
			: public gRPCModule<gRPCServices...>::template TypedCallDataBase<CallDataConfigureInput, ThisServiceType, DynExpProto::NetworkTimeTagger::ConfigureInputMessage, DynExpProto::Common::VoidMessage>
		{
			using Base = gRPCModule<gRPCServices...>::template TypedCallDataBase<CallDataConfigureInput, ThisServiceType, DynExpProto::NetworkTimeTagger::ConfigureInputMessage, DynExpProto::Common::VoidMessage>;
			using Base::RequestMessage;

		public:
			CallDataConfigureInput(const gRPCModule<gRPCServices...>* const OwningModule) noexcept
				: Base::TypedCallDataBase(OwningModule, &ThisServiceType::AsyncService::RequestConfigureInput) {}
			virtual ~CallDataConfigureInput() = default;

		private:
			virtual void ProcessChildImpl(DynExp::ModuleInstance& Instance) override
			{
				auto ModuleData = DynExp::dynamic_ModuleData_cast<NetworkTimeTaggerT>(Instance.ModuleDataGetter());
				auto Instrument = ModuleData->GetTimeTagger();

				Instrument->ConfigureInput(RequestMessage.userisingedge(), RequestMessage.thresholdinvolts());
			}
		};

		class CallDataSetExposureTime
			: public gRPCModule<gRPCServices...>::template TypedCallDataBase<CallDataSetExposureTime, ThisServiceType, DynExpProto::NetworkTimeTagger::ExposureTimeMessage, DynExpProto::Common::VoidMessage>
		{
			using Base = gRPCModule<gRPCServices...>::template TypedCallDataBase<CallDataSetExposureTime, ThisServiceType, DynExpProto::NetworkTimeTagger::ExposureTimeMessage, DynExpProto::Common::VoidMessage>;
			using Base::RequestMessage;

		public:
			CallDataSetExposureTime(const gRPCModule<gRPCServices...>* const OwningModule) noexcept
				: Base::TypedCallDataBase(OwningModule, &ThisServiceType::AsyncService::RequestSetExposureTime) {}
			virtual ~CallDataSetExposureTime() = default;

		private:
			virtual void ProcessChildImpl(DynExp::ModuleInstance& Instance) override
			{
				auto ModuleData = DynExp::dynamic_ModuleData_cast<NetworkTimeTaggerT>(Instance.ModuleDataGetter());
				auto Instrument = ModuleData->GetTimeTagger();

				Instrument->SetExposureTime(Util::picoseconds(RequestMessage.exposuretimeinpicoseconds()));
			}
		};

		class CallDataSetCoincidenceWindow
			: public gRPCModule<gRPCServices...>::template TypedCallDataBase<CallDataSetCoincidenceWindow, ThisServiceType, DynExpProto::NetworkTimeTagger::CoincidenceWindowMessage, DynExpProto::Common::VoidMessage>
		{
			using Base = gRPCModule<gRPCServices...>::template TypedCallDataBase<CallDataSetCoincidenceWindow, ThisServiceType, DynExpProto::NetworkTimeTagger::CoincidenceWindowMessage, DynExpProto::Common::VoidMessage>;
			using Base::RequestMessage;

		public:
			CallDataSetCoincidenceWindow(const gRPCModule<gRPCServices...>* const OwningModule) noexcept
				: Base::TypedCallDataBase(OwningModule, &ThisServiceType::AsyncService::RequestSetCoincidenceWindow) {}
			virtual ~CallDataSetCoincidenceWindow() = default;

		private:
			virtual void ProcessChildImpl(DynExp::ModuleInstance& Instance) override
			{
				auto ModuleData = DynExp::dynamic_ModuleData_cast<NetworkTimeTaggerT>(Instance.ModuleDataGetter());
				auto Instrument = ModuleData->GetTimeTagger();

				Instrument->SetCoincidenceWindow(Util::picoseconds(RequestMessage.coincidencewindowinpicoseconds()));
			}
		};

		class CallDataSetInputDelay
			: public gRPCModule<gRPCServices...>::template TypedCallDataBase<CallDataSetInputDelay, ThisServiceType, DynExpProto::NetworkTimeTagger::InputDelayMessage, DynExpProto::Common::VoidMessage>
		{
			using Base = gRPCModule<gRPCServices...>::template TypedCallDataBase<CallDataSetInputDelay, ThisServiceType, DynExpProto::NetworkTimeTagger::InputDelayMessage, DynExpProto::Common::VoidMessage>;
			using Base::RequestMessage;

		public:
			CallDataSetInputDelay(const gRPCModule<gRPCServices...>* const OwningModule) noexcept
				: Base::TypedCallDataBase(OwningModule, &ThisServiceType::AsyncService::RequestSetInputDelay) {}
			virtual ~CallDataSetInputDelay() = default;

		private:
			virtual void ProcessChildImpl(DynExp::ModuleInstance& Instance) override
			{
				auto ModuleData = DynExp::dynamic_ModuleData_cast<NetworkTimeTaggerT>(Instance.ModuleDataGetter());
				auto Instrument = ModuleData->GetTimeTagger();

				Instrument->SetDelay(Util::picoseconds(RequestMessage.delayinpicoseconds()));
			}
		};

		class CallDataSetHBTActive
			: public gRPCModule<gRPCServices...>::template TypedCallDataBase<CallDataSetHBTActive, ThisServiceType, DynExpProto::NetworkTimeTagger::HBTActiveMessage, DynExpProto::Common::VoidMessage>
		{
			using Base = gRPCModule<gRPCServices...>::template TypedCallDataBase<CallDataSetHBTActive, ThisServiceType, DynExpProto::NetworkTimeTagger::HBTActiveMessage, DynExpProto::Common::VoidMessage>;
			using Base::RequestMessage;

		public:
			CallDataSetHBTActive(const gRPCModule<gRPCServices...>* const OwningModule) noexcept
				: Base::TypedCallDataBase(OwningModule, &ThisServiceType::AsyncService::RequestSetHBTActive) {}
			virtual ~CallDataSetHBTActive() = default;

		private:
			virtual void ProcessChildImpl(DynExp::ModuleInstance& Instance) override
			{
				auto ModuleData = DynExp::dynamic_ModuleData_cast<NetworkTimeTaggerT>(Instance.ModuleDataGetter());
				auto Instrument = ModuleData->GetTimeTagger();

				Instrument->SetHBTActive(RequestMessage.enable());
			}
		};

		class CallDataConfigureHBT
			: public gRPCModule<gRPCServices...>::template TypedCallDataBase<CallDataConfigureHBT, ThisServiceType, DynExpProto::NetworkTimeTagger::ConfigureHBTMessage, DynExpProto::Common::VoidMessage>
		{
			using Base = gRPCModule<gRPCServices...>::template TypedCallDataBase<CallDataConfigureHBT, ThisServiceType, DynExpProto::NetworkTimeTagger::ConfigureHBTMessage, DynExpProto::Common::VoidMessage>;
			using Base::RequestMessage;

		public:
			CallDataConfigureHBT(const gRPCModule<gRPCServices...>* const OwningModule) noexcept
				: Base::TypedCallDataBase(OwningModule, &ThisServiceType::AsyncService::RequestConfigureHBT) {}
			virtual ~CallDataConfigureHBT() = default;

		private:
			virtual void ProcessChildImpl(DynExp::ModuleInstance& Instance) override
			{
				auto ModuleData = DynExp::dynamic_ModuleData_cast<NetworkTimeTaggerT>(Instance.ModuleDataGetter());
				auto Instrument = ModuleData->GetTimeTagger();

				Instrument->ConfigureHBT(Util::picoseconds(RequestMessage.binwidthinpicoseconds()), RequestMessage.bincount());
			}
		};

		class CallDataResetHBT
			: public gRPCModule<gRPCServices...>::template TypedCallDataBase<CallDataResetHBT, ThisServiceType, DynExpProto::Common::VoidMessage, DynExpProto::Common::VoidMessage>
		{
			using Base = gRPCModule<gRPCServices...>::template TypedCallDataBase<CallDataResetHBT, ThisServiceType, DynExpProto::Common::VoidMessage, DynExpProto::Common::VoidMessage>;

		public:
			CallDataResetHBT(const gRPCModule<gRPCServices...>* const OwningModule) noexcept
				: Base::TypedCallDataBase(OwningModule, &ThisServiceType::AsyncService::RequestResetHBT) {}
			virtual ~CallDataResetHBT() = default;

		private:
			virtual void ProcessChildImpl(DynExp::ModuleInstance& Instance) override
			{
				auto ModuleData = DynExp::dynamic_ModuleData_cast<NetworkTimeTaggerT>(Instance.ModuleDataGetter());
				auto Instrument = ModuleData->GetTimeTagger();

				Instrument->ResetHBT();
			}
		};

		class CallDataGetHBTResults
			: public gRPCModule<gRPCServices...>::template TypedCallDataBase<CallDataGetHBTResults, ThisServiceType, DynExpProto::Common::VoidMessage, DynExpProto::NetworkTimeTagger::HBTResultsMessage>
		{
			using Base = gRPCModule<gRPCServices...>::template TypedCallDataBase<CallDataGetHBTResults, ThisServiceType, DynExpProto::Common::VoidMessage, DynExpProto::NetworkTimeTagger::HBTResultsMessage>;
			using Base::ResponseMessage;

		public:
			CallDataGetHBTResults(const gRPCModule<gRPCServices...>* const OwningModule) noexcept
				: Base::TypedCallDataBase(OwningModule, &ThisServiceType::AsyncService::RequestGetHBTResults) {}
			virtual ~CallDataGetHBTResults() = default;

		private:
			virtual void ProcessChildImpl(DynExp::ModuleInstance& Instance) override
			{
				auto StreamSizeMsg = std::make_unique<DynExpProto::NetworkTimeTagger::BufferInfoMessage>();
				auto ModuleData = DynExp::dynamic_ModuleData_cast<NetworkTimeTaggerT>(Instance.ModuleDataGetter());
				auto Instrument = ModuleData->GetTimeTagger();
				auto InstrData = DynExp::dynamic_InstrumentData_cast<DynExpInstr::TimeTagger>(Instrument->GetInstrumentData());

				ResponseMessage.set_enabled(InstrData->GetHBTResults().Enabled);
				ResponseMessage.set_eventcounts(Util::NumToT<google::protobuf::uint64>(InstrData->GetHBTResults().EventCounts));
				ResponseMessage.set_integrationtimeinmicroseconds(Util::NumToT<google::protobuf::uint64>(InstrData->GetHBTResults().IntegrationTime.count()));

				for (const auto& Sample : InstrData->GetHBTResults().ResultVector)
				{
					auto BasicSampleMsg = ResponseMessage.add_results();
					BasicSampleMsg->set_value(Sample.Value);
					BasicSampleMsg->set_time(Sample.Time);
				}
			}
		};

	private:
		void ResetImpl(DynExp::Object::dispatch_tag<NetworkDataStreamInstrumentT<gRPCServices...>>) override final
		{
			ResetImpl(DynExp::Object::dispatch_tag<NetworkTimeTaggerT>());
		}

		virtual void ResetImpl(DynExp::Object::dispatch_tag<NetworkTimeTaggerT>) {}

		void CreateInitialCallDataObjectsImpl(DynExp::Object::dispatch_tag<NetworkDataStreamInstrumentT<gRPCServices...>>, DynExp::ModuleInstance& Instance) const override final
		{
			CallDataGetHardwareInfo::MakeCall(this, Instance);
			CallDataGetBufferInfo::MakeCall(this, Instance);
			CallDataGetStreamMode::MakeCall(this, Instance);
			CallDataSetStreamMode::MakeCall(this, Instance);
			CallDataClearBuffer::MakeCall(this, Instance);
			CallDataConfigureInput::MakeCall(this, Instance);
			CallDataSetExposureTime::MakeCall(this, Instance);
			CallDataSetCoincidenceWindow::MakeCall(this, Instance);
			CallDataSetInputDelay::MakeCall(this, Instance);
			CallDataSetHBTActive::MakeCall(this, Instance);
			CallDataConfigureHBT::MakeCall(this, Instance);
			CallDataResetHBT::MakeCall(this, Instance);
			CallDataGetHBTResults::MakeCall(this, Instance);

			CreateInitialCallDataObjectsImpl(DynExp::Object::dispatch_tag<NetworkTimeTaggerT>(), Instance);
		}

		virtual void CreateInitialCallDataObjectsImpl(DynExp::Object::dispatch_tag<NetworkTimeTaggerT>, DynExp::ModuleInstance& Instance) const {}

		virtual void ValidateInstrType(const DynExpInstr::DataStreamInstrument* Instr) const override { DynExp::dynamic_Object_cast<typename ModuleDataType::InstrType>(Instr); }
	};

	/**
	 * @brief Explicit instantiation of derivable class NetworkTimeTaggerT to create the network time tagger module.
	*/
	using NetworkTimeTagger = NetworkTimeTaggerT<typename NetworkDataStreamInstrument::ThisServiceType, DynExpProto::NetworkTimeTagger::NetworkTimeTagger>;
}