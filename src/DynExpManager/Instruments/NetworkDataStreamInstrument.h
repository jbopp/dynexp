// This file is part of DynExp.

/**
 * @file NetworkDataStreamInstrument.h
 * @brief Implementation of a gRPC client instrument to access a remote data stream meta instrument.
*/

#pragma once

#include "stdafx.h"
#include "MetaInstruments/DataStreamInstrument.h"
#include "MetaInstruments/gRPCInstrument.h"

#include "NetworkDataStreamInstrument.pb.h"
#include "NetworkDataStreamInstrument.grpc.pb.h"

namespace DynExpInstr
{
	template <typename BaseInstr, typename std::enable_if_t<std::is_base_of_v<DataStreamInstrument, BaseInstr>, int>, typename... gRPCStubs>
	class NetworkDataStreamInstrumentT;

	using NetworkDataStreamInstrumentDataSampleStreamType = BasicSampleStream;

	constexpr DynExpProto::Common::UnitType ToPrototUnitType(DataStreamInstrumentData::UnitType Unit)
	{
		switch (Unit)
		{
		case DataStreamInstrumentData::UnitType::Arbitrary: return DynExpProto::Common::UnitType::Arbitrary;
		case DataStreamInstrumentData::UnitType::LogicLevel: return DynExpProto::Common::UnitType::LogicLevel;
		case DataStreamInstrumentData::UnitType::Counts: return DynExpProto::Common::UnitType::Counts;
		case DataStreamInstrumentData::UnitType::Volt: return DynExpProto::Common::UnitType::Volt;
		case DataStreamInstrumentData::UnitType::Ampere: return DynExpProto::Common::UnitType::Ampere;
		case DataStreamInstrumentData::UnitType::Power_W: return DynExpProto::Common::UnitType::Power_W;
		case DataStreamInstrumentData::UnitType::Power_dBm: return DynExpProto::Common::UnitType::Power_dBm;
		default: throw Util::InvalidDataException("The given unit does not exist in the DataStreamInstrumentData::UnitType enumeration. Did you forget to adjust the UnitType enumeration in class \"DataStreamInstrumentData\"?");
		}
	}

	constexpr DataStreamInstrumentData::UnitType ToDataStreamInstrumentUnitType(DynExpProto::Common::UnitType Unit)
	{
		switch (Unit)
		{
		case DynExpProto::Common::UnitType::Arbitrary: return DataStreamInstrumentData::UnitType::Arbitrary;
		case DynExpProto::Common::UnitType::LogicLevel: return DataStreamInstrumentData::UnitType::LogicLevel;
		case DynExpProto::Common::UnitType::Counts: return DataStreamInstrumentData::UnitType::Counts;
		case DynExpProto::Common::UnitType::Volt: return DataStreamInstrumentData::UnitType::Volt;
		case DynExpProto::Common::UnitType::Ampere: return DataStreamInstrumentData::UnitType::Ampere;
		case DynExpProto::Common::UnitType::Power_W: return DataStreamInstrumentData::UnitType::Power_W;
		case DynExpProto::Common::UnitType::Power_dBm: return DataStreamInstrumentData::UnitType::Power_dBm;
		default: throw Util::InvalidDataException("The given unit does not exist in the DynExpProto::Common::UnitType enumeration. Did you forget to adjust the UnitType enumeration in file \"Common.proto\"?");
		}
	}

	namespace NetworkDataStreamInstrumentTasks
	{
		template <typename BaseInstr, typename std::enable_if_t<std::is_base_of_v<DataStreamInstrument, BaseInstr>, int>, typename... gRPCStubs>
		class InitTask : public gRPCInstrumentTasks::InitTask<BaseInstr, 0, gRPCStubs...>
		{
			void InitFuncImpl(DynExp::InitTaskBase::dispatch_tag<gRPCInstrumentTasks::InitTask<BaseInstr, 0, gRPCStubs...>>, DynExp::InstrumentInstance& Instance) override final
			{
				StubPtrType<DynExpProto::NetworkDataStreamInstrument::NetworkDataStreamInstrument> StubPtr;
				{
					auto InstrData = dynamic_InstrumentData_cast<NetworkDataStreamInstrumentT<BaseInstr, 0, gRPCStubs...>>(Instance.InstrumentDataGetter());

					StubPtr = InstrData->template GetStub<DynExpProto::NetworkDataStreamInstrument::NetworkDataStreamInstrument>();
				} // InstrData unlocked here.

				auto Response = InvokeStubFunc(StubPtr, &DynExpProto::NetworkDataStreamInstrument::NetworkDataStreamInstrument::Stub::GetStreamInfo, {});

				{
					auto InstrData = dynamic_InstrumentData_cast<NetworkDataStreamInstrumentT<BaseInstr, 0, gRPCStubs...>>(Instance.InstrumentDataGetter());

					InstrData->RemoteStreamInfo.ValueUnit = ToDataStreamInstrumentUnitType(Response.valueunit());
					InstrData->RemoteStreamInfo.HardwareMinValue = Response.hardwareminvalue();
					InstrData->RemoteStreamInfo.HardwareMaxValue = Response.hardwaremaxvalue();
					InstrData->RemoteStreamInfo.IsBasicSampleTimeUsed = Response.isbasicsampletimeused();
					InstrData->RemoteStreamInfo.StreamSizeRead = Util::NumToT<size_t>(Response.streamsizemsg().streamsizeread());
					InstrData->RemoteStreamInfo.StreamSizeWrite = Util::NumToT<size_t>(Response.streamsizemsg().streamsizewrite());

					InstrData->GetSampleStream()->SetStreamSize(InstrData->RemoteStreamInfo.StreamSizeWrite);
				}  // InstrData unlocked here.

				InitFuncImpl(DynExp::InitTaskBase::dispatch_tag<InitTask>(), Instance);
			}

			virtual void InitFuncImpl(DynExp::InitTaskBase::dispatch_tag<InitTask>, DynExp::InstrumentInstance& Instance) {}
		};

		template <typename BaseInstr, typename std::enable_if_t<std::is_base_of_v<DataStreamInstrument, BaseInstr>, int>, typename... gRPCStubs>
		class ExitTask : public gRPCInstrumentTasks::ExitTask<BaseInstr, 0, gRPCStubs...>
		{
			void ExitFuncImpl(DynExp::ExitTaskBase::dispatch_tag<gRPCInstrumentTasks::ExitTask<BaseInstr, 0, gRPCStubs...>>, DynExp::InstrumentInstance& Instance) override final
			{
				ExitFuncImpl(DynExp::ExitTaskBase::dispatch_tag<ExitTask>(), Instance);
			}

			virtual void ExitFuncImpl(DynExp::ExitTaskBase::dispatch_tag<ExitTask>, DynExp::InstrumentInstance& Instance) {}
		};

		template <typename BaseInstr, typename std::enable_if_t<std::is_base_of_v<DataStreamInstrument, BaseInstr>, int>, typename... gRPCStubs>
		class UpdateTask : public gRPCInstrumentTasks::UpdateTask<BaseInstr, 0, gRPCStubs...>
		{
			void UpdateFuncImpl(DynExp::UpdateTaskBase::dispatch_tag<gRPCInstrumentTasks::UpdateTask<BaseInstr, 0, gRPCStubs...>>, DynExp::InstrumentInstance& Instance) override final
			{
				StubPtrType<DynExpProto::NetworkDataStreamInstrument::NetworkDataStreamInstrument> StubPtr;
				{
					auto InstrData = dynamic_InstrumentData_cast<NetworkDataStreamInstrumentT<BaseInstr, 0, gRPCStubs...>>(Instance.InstrumentDataGetter());
					StubPtr = InstrData->template GetStub<DynExpProto::NetworkDataStreamInstrument::NetworkDataStreamInstrument>();
				} // InstrData unlocked here.

				auto StreamSizeResponse = InvokeStubFunc(StubPtr, &DynExpProto::NetworkDataStreamInstrument::NetworkDataStreamInstrument::Stub::GetStreamSize, {});
				auto FinishedResponse = InvokeStubFunc(StubPtr, &DynExpProto::NetworkDataStreamInstrument::NetworkDataStreamInstrument::Stub::HasFinished, {});
				auto RunningResponse = InvokeStubFunc(StubPtr, &DynExpProto::NetworkDataStreamInstrument::NetworkDataStreamInstrument::Stub::IsRunning, {});

				{
					auto InstrData = dynamic_InstrumentData_cast<NetworkDataStreamInstrumentT<BaseInstr, 0, gRPCStubs...>>(Instance.InstrumentDataGetter());

					InstrData->RemoteStreamInfo.StreamSizeRead = Util::NumToT<size_t>(StreamSizeResponse.streamsizeread());
					InstrData->RemoteStreamInfo.StreamSizeWrite = Util::NumToT<size_t>(StreamSizeResponse.streamsizewrite());
					InstrData->Finished = FinishedResponse.has_value() ? Util::OptionalBool(FinishedResponse.value()) : Util::OptionalBool::Values::Unknown;
					InstrData->Running = RunningResponse.has_value() ? Util::OptionalBool(RunningResponse.value()) : Util::OptionalBool::Values::Unknown;

					if (InstrData->GetSampleStream()->GetStreamSizeWrite() != InstrData->RemoteStreamInfo.StreamSizeWrite)
					{
						InstrData->GetSampleStream()->SetStreamSize(InstrData->RemoteStreamInfo.StreamSizeWrite);
						InstrData->SetLastReadRemoteSampleID(0);
					}
				}  // InstrData unlocked here.

				UpdateFuncImpl(DynExp::UpdateTaskBase::dispatch_tag<UpdateTask>(), Instance);
			}

			virtual void UpdateFuncImpl(DynExp::UpdateTaskBase::dispatch_tag<UpdateTask>, DynExp::InstrumentInstance& Instance) {}
		};

		template <typename BaseInstr, typename std::enable_if_t<std::is_base_of_v<DataStreamInstrument, BaseInstr>, int>, typename... gRPCStubs>
		class ReadTask : public DynExp::TaskBase
		{
		public:
			ReadTask(CallbackType CallbackFunc) noexcept : TaskBase(CallbackFunc) {}

		private:
			virtual DynExp::TaskResultType RunChild(DynExp::InstrumentInstance& Instance) override
			{
				DynExpProto::NetworkDataStreamInstrument::ReadMessage ReadMsg;
				StubPtrType<DynExpProto::NetworkDataStreamInstrument::NetworkDataStreamInstrument> StubPtr;
				{
					auto InstrData = dynamic_InstrumentData_cast<NetworkDataStreamInstrumentT<BaseInstr, 0, gRPCStubs...>>(Instance.InstrumentDataGetter());

					StubPtr = InstrData->template GetStub<DynExpProto::NetworkDataStreamInstrument::NetworkDataStreamInstrument>();
					ReadMsg.set_startsampleid(Util::NumToT<google::protobuf::uint64>(InstrData->GetLastReadRemoteSampleID()));
				} // InstrData unlocked here.

				auto ReadResultMsg = InvokeStubFunc(StubPtr, &DynExpProto::NetworkDataStreamInstrument::NetworkDataStreamInstrument::Stub::Read, ReadMsg);

				auto InstrData = dynamic_InstrumentData_cast<NetworkDataStreamInstrumentT<BaseInstr, 0, gRPCStubs...>>(Instance.InstrumentDataGetter());
				for (decltype(ReadResultMsg.samples_size()) i = 0; i < ReadResultMsg.samples_size(); ++i)
					InstrData->GetSampleStream()->WriteBasicSample({ ReadResultMsg.samples(i).value(), ReadResultMsg.samples(i).time() });

				InstrData->SetLastReadRemoteSampleID(Util::NumToT<size_t>(ReadResultMsg.lastsampleid()));

				return {};
			}
		};

		template <typename BaseInstr, typename std::enable_if_t<std::is_base_of_v<DataStreamInstrument, BaseInstr>, int>, typename... gRPCStubs>
		class WriteTask : public DynExp::TaskBase
		{
		public:
			WriteTask(CallbackType CallbackFunc) noexcept : TaskBase(CallbackFunc) {}

		private:
			virtual DynExp::TaskResultType RunChild(DynExp::InstrumentInstance& Instance) override
			{
				StubPtrType<DynExpProto::NetworkDataStreamInstrument::NetworkDataStreamInstrument> StubPtr;
				std::vector<NetworkDataStreamInstrumentDataSampleStreamType::SampleType> Samples;
				{
					auto InstrData = dynamic_InstrumentData_cast<NetworkDataStreamInstrumentT<BaseInstr, 0, gRPCStubs...>>(Instance.InstrumentDataGetter());
					auto SampleStream = InstrData->template GetCastSampleStream<NetworkDataStreamInstrumentDataSampleStreamType>();

					if (SampleStream->GetNumSamplesWritten() == InstrData->GetLastWrittenSampleID())
						return {};
					if (SampleStream->GetNumSamplesWritten() < InstrData->GetLastWrittenSampleID())
						InstrData->SetLastWrittenSampleID(0);	// e.g. if SampleStream has been cleared. Transmit the entire buffer then.

					StubPtr = InstrData->template GetStub<DynExpProto::NetworkDataStreamInstrument::NetworkDataStreamInstrument>();

					Samples = SampleStream->ReadRecentBasicSamples(InstrData->GetLastWrittenSampleID());
					InstrData->SetLastWrittenSampleID(SampleStream->GetNumSamplesWritten());
				} // InstrData unlocked here.

				DynExpProto::NetworkDataStreamInstrument::WriteMessage WriteMsg;
				for (const auto& Sample : Samples)
				{
					auto BasicSampleMsg = WriteMsg.add_samples();
					BasicSampleMsg->set_value(Sample.Value);
					BasicSampleMsg->set_time(Sample.Time);
				}

				InvokeStubFunc(StubPtr, &DynExpProto::NetworkDataStreamInstrument::NetworkDataStreamInstrument::Stub::Write, WriteMsg);

				return {};
			}
		};

		template <typename BaseInstr, typename std::enable_if_t<std::is_base_of_v<DataStreamInstrument, BaseInstr>, int>, typename... gRPCStubs>
		class ClearTask : public DynExp::TaskBase
		{
		public:
			ClearTask(CallbackType CallbackFunc) noexcept : TaskBase(CallbackFunc) {}

		private:
			virtual DynExp::TaskResultType RunChild(DynExp::InstrumentInstance& Instance) override
			{
				StubPtrType<DynExpProto::NetworkDataStreamInstrument::NetworkDataStreamInstrument> StubPtr;
				{
					auto InstrData = dynamic_InstrumentData_cast<NetworkDataStreamInstrumentT<BaseInstr, 0, gRPCStubs...>>(Instance.InstrumentDataGetter());
					StubPtr = InstrData->template GetStub<DynExpProto::NetworkDataStreamInstrument::NetworkDataStreamInstrument>();
				} // InstrData unlocked here.

				InvokeStubFunc(StubPtr, &DynExpProto::NetworkDataStreamInstrument::NetworkDataStreamInstrument::Stub::ClearData, {});

				return {};
			}
		};

		template <typename BaseInstr, typename std::enable_if_t<std::is_base_of_v<DataStreamInstrument, BaseInstr>, int>, typename... gRPCStubs>
		class StartTask : public DynExp::TaskBase
		{
		public:
			StartTask(CallbackType CallbackFunc) noexcept : TaskBase(CallbackFunc) {}

		private:
			virtual DynExp::TaskResultType RunChild(DynExp::InstrumentInstance& Instance) override
			{
				StubPtrType<DynExpProto::NetworkDataStreamInstrument::NetworkDataStreamInstrument> StubPtr;
				{
					auto InstrData = dynamic_InstrumentData_cast<NetworkDataStreamInstrumentT<BaseInstr, 0, gRPCStubs...>>(Instance.InstrumentDataGetter());
					StubPtr = InstrData->template GetStub<DynExpProto::NetworkDataStreamInstrument::NetworkDataStreamInstrument>();
				} // InstrData unlocked here.

				InvokeStubFunc(StubPtr, &DynExpProto::NetworkDataStreamInstrument::NetworkDataStreamInstrument::Stub::Start, {});

				return {};
			}
		};

		template <typename BaseInstr, typename std::enable_if_t<std::is_base_of_v<DataStreamInstrument, BaseInstr>, int>, typename... gRPCStubs>
		class StopTask : public DynExp::TaskBase
		{
		public:
			StopTask(CallbackType CallbackFunc) noexcept : TaskBase(CallbackFunc) {}

		private:
			virtual DynExp::TaskResultType RunChild(DynExp::InstrumentInstance& Instance) override
			{
				StubPtrType<DynExpProto::NetworkDataStreamInstrument::NetworkDataStreamInstrument> StubPtr;
				{
					auto InstrData = dynamic_InstrumentData_cast<NetworkDataStreamInstrumentT<BaseInstr, 0, gRPCStubs...>>(Instance.InstrumentDataGetter());
					StubPtr = InstrData->template GetStub<DynExpProto::NetworkDataStreamInstrument::NetworkDataStreamInstrument>();
				} // InstrData unlocked here.

				InvokeStubFunc(StubPtr, &DynExpProto::NetworkDataStreamInstrument::NetworkDataStreamInstrument::Stub::Stop, {});

				return {};
			}
		};

		template <typename BaseInstr, typename std::enable_if_t<std::is_base_of_v<DataStreamInstrument, BaseInstr>, int>, typename... gRPCStubs>
		class RestartTask : public DynExp::TaskBase
		{
		public:
			RestartTask(CallbackType CallbackFunc) noexcept : TaskBase(CallbackFunc) {}

		private:
			virtual DynExp::TaskResultType RunChild(DynExp::InstrumentInstance& Instance) override
			{
				StubPtrType<DynExpProto::NetworkDataStreamInstrument::NetworkDataStreamInstrument> StubPtr;
				{
					auto InstrData = dynamic_InstrumentData_cast<NetworkDataStreamInstrumentT<BaseInstr, 0, gRPCStubs...>>(Instance.InstrumentDataGetter());
					StubPtr = InstrData->template GetStub<DynExpProto::NetworkDataStreamInstrument::NetworkDataStreamInstrument>();
				} // InstrData unlocked here.

				InvokeStubFunc(StubPtr, &DynExpProto::NetworkDataStreamInstrument::NetworkDataStreamInstrument::Stub::Restart, {});

				return {};
			}
		};

		template <typename BaseInstr, typename std::enable_if_t<std::is_base_of_v<DataStreamInstrument, BaseInstr>, int>, typename... gRPCStubs>
		class SetStreamSizeTask : public DynExp::TaskBase
		{
		public:
			SetStreamSizeTask(size_t StreamSizeInSamples, CallbackType CallbackFunc) noexcept
				: TaskBase(CallbackFunc), StreamSizeInSamples(StreamSizeInSamples) {}

		private:
			virtual DynExp::TaskResultType RunChild(DynExp::InstrumentInstance& Instance) override
			{
				DynExpProto::NetworkDataStreamInstrument::StreamSizeMessage StreamSizeMsg;
				StreamSizeMsg.set_streamsizewrite(Util::NumToT<google::protobuf::uint64>(StreamSizeInSamples));

				StubPtrType<DynExpProto::NetworkDataStreamInstrument::NetworkDataStreamInstrument> StubPtr;
				{
					auto InstrData = dynamic_InstrumentData_cast<NetworkDataStreamInstrumentT<BaseInstr, 0, gRPCStubs...>>(Instance.InstrumentDataGetter());
					StubPtr = InstrData->template GetStub<DynExpProto::NetworkDataStreamInstrument::NetworkDataStreamInstrument>();
				} // InstrData unlocked here.

				auto Response = InvokeStubFunc(StubPtr, &DynExpProto::NetworkDataStreamInstrument::NetworkDataStreamInstrument::Stub::SetStreamSize, StreamSizeMsg);

				{
					auto InstrData = dynamic_InstrumentData_cast<NetworkDataStreamInstrumentT<BaseInstr, 0, gRPCStubs...>>(Instance.InstrumentDataGetter());
					if (InstrData->GetSampleStream()->GetStreamSizeWrite() != Response.streamsizewrite())
					{
						InstrData->GetSampleStream()->SetStreamSize(Response.streamsizewrite());
						InstrData->SetLastReadRemoteSampleID(0);
					}
				} // InstrData unlocked here.

				return {};
			}

			const size_t StreamSizeInSamples;
		};

		template <typename BaseInstr, typename std::enable_if_t<std::is_base_of_v<DataStreamInstrument, BaseInstr>, int>, typename... gRPCStubs>
		class ResetStreamSizeTask : public DynExp::TaskBase
		{
		public:
			ResetStreamSizeTask(CallbackType CallbackFunc) noexcept : TaskBase(CallbackFunc) {}

		private:
			virtual DynExp::TaskResultType RunChild(DynExp::InstrumentInstance& Instance) override
			{
				StubPtrType<DynExpProto::NetworkDataStreamInstrument::NetworkDataStreamInstrument> StubPtr;
				{
					auto InstrData = dynamic_InstrumentData_cast<NetworkDataStreamInstrumentT<BaseInstr, 0, gRPCStubs...>>(Instance.InstrumentDataGetter());
					StubPtr = InstrData->template GetStub<DynExpProto::NetworkDataStreamInstrument::NetworkDataStreamInstrument>();
				} // InstrData unlocked here.

				auto Response = InvokeStubFunc(StubPtr, &DynExpProto::NetworkDataStreamInstrument::NetworkDataStreamInstrument::Stub::ResetStreamSize, {});

				{
					auto InstrData = dynamic_InstrumentData_cast<NetworkDataStreamInstrumentT<BaseInstr, 0, gRPCStubs...>>(Instance.InstrumentDataGetter());
					if (InstrData->GetSampleStream()->GetStreamSizeWrite() != Response.streamsizewrite())
					{
						InstrData->GetSampleStream()->SetStreamSize(Response.streamsizewrite());
						InstrData->SetLastReadRemoteSampleID(0);
					}
				} // InstrData unlocked here.

				return {};
			}
		};
	}

	template <typename BaseInstr, typename std::enable_if_t<std::is_base_of_v<DataStreamInstrument, BaseInstr>, int>, typename... gRPCStubs>
	class NetworkDataStreamInstrumentData : public gRPCInstrumentData<BaseInstr, 0, gRPCStubs...>
	{
		friend class NetworkDataStreamInstrumentTasks::InitTask<BaseInstr, 0, gRPCStubs...>;
		friend class NetworkDataStreamInstrumentTasks::UpdateTask<BaseInstr, 0, gRPCStubs...>;

	public:
		using SampleStreamType = NetworkDataStreamInstrumentDataSampleStreamType;
		using InstrumentType = NetworkDataStreamInstrumentT<BaseInstr, 0, gRPCStubs...>;

		struct RemoteStreamInfoType
		{
			DataStreamInstrumentData::UnitType ValueUnit = DataStreamInstrumentData::UnitType::Ampere;
			double HardwareMinValue = 0;
			double HardwareMaxValue = 0;
			bool IsBasicSampleTimeUsed = false;
			size_t StreamSizeRead = 0;
			size_t StreamSizeWrite = 0;
		};

		NetworkDataStreamInstrumentData(size_t BufferSizeInSamples = 1)
			: gRPCInstrumentData<BaseInstr, 0, gRPCStubs...>(std::make_unique<SampleStreamType>(BufferSizeInSamples)) {}
		virtual ~NetworkDataStreamInstrumentData() = default;

		const auto& GetRemoteStreamInfo() const noexcept { return RemoteStreamInfo; }
		auto HasFinished() const noexcept { return Finished; }
		auto IsRunning() const noexcept { return Running; }

		auto GetLastReadRemoteSampleID() const noexcept { return LastReadRemoteSampleID; }
		void SetLastReadRemoteSampleID(size_t SampleID) noexcept { LastReadRemoteSampleID = SampleID; }
		auto GetLastWrittenSampleID() const noexcept { return LastWrittenSampleID; }
		void SetLastWrittenSampleID(size_t SampleID) noexcept { LastWrittenSampleID = SampleID; }

	private:
		void ResetImpl(DynExp::InstrumentDataBase::dispatch_tag<gRPCInstrumentData<BaseInstr, 0, gRPCStubs...>>) override final
		{
			RemoteStreamInfo = {};
			Finished = Util::OptionalBool::Values::Unknown;
			Running = Util::OptionalBool::Values::Unknown;

			LastReadRemoteSampleID = 0;
			LastWrittenSampleID = 0;

			ResetImpl(DynExp::InstrumentDataBase::dispatch_tag<NetworkDataStreamInstrumentData>());
		}

		virtual void ResetImpl(DynExp::InstrumentDataBase::dispatch_tag<NetworkDataStreamInstrumentData>) {};

		RemoteStreamInfoType RemoteStreamInfo;
		Util::OptionalBool Finished;
		Util::OptionalBool Running;

		size_t LastReadRemoteSampleID = 0;	//!< ID of the last sample read from the remote site and written to the assigned data stream.
		size_t LastWrittenSampleID = 0;		//!< ID of the last sample read from the assigned data stream and written to the remote site.
	};

	template <typename BaseInstr, typename std::enable_if_t<std::is_base_of_v<DataStreamInstrument, BaseInstr>, int>, typename... gRPCStubs>
	class NetworkDataStreamInstrumentParams : public gRPCInstrumentParams<BaseInstr, 0, gRPCStubs...>
	{
	public:
		NetworkDataStreamInstrumentParams(DynExp::ItemIDType ID, const DynExp::DynExpCore& Core) : gRPCInstrumentParams<BaseInstr, 0, gRPCStubs...>(ID, Core) {}
		virtual ~NetworkDataStreamInstrumentParams() = default;

		virtual const char* GetParamClassTag() const noexcept override { return "NetworkDataStreamInstrumentParams"; }

	private:
		void ConfigureParamsImpl(DynExp::ParamsBase::dispatch_tag<gRPCInstrumentParams<BaseInstr, 0, gRPCStubs...>>) override final { ConfigureParamsImpl(DynExp::ParamsBase::dispatch_tag<NetworkDataStreamInstrumentParams>()); }
		virtual void ConfigureParamsImpl(DynExp::ParamsBase::dispatch_tag<NetworkDataStreamInstrumentParams>) {}

		DynExp::ParamsBase::DummyParam Dummy = { *this };
	};

	template <typename BaseInstr, typename std::enable_if_t<std::is_base_of_v<DataStreamInstrument, BaseInstr>, int>, typename... gRPCStubs>
	class NetworkDataStreamInstrumentConfigurator : public gRPCInstrumentConfigurator<BaseInstr, 0, gRPCStubs...>
	{
	public:
		using ObjectType = NetworkDataStreamInstrumentT<BaseInstr, 0, gRPCStubs...>;
		using ParamsType = NetworkDataStreamInstrumentParams<BaseInstr, 0, gRPCStubs...>;

		NetworkDataStreamInstrumentConfigurator() = default;
		virtual ~NetworkDataStreamInstrumentConfigurator() = default;

	private:
		virtual DynExp::ParamsBasePtrType MakeParams(DynExp::ItemIDType ID, const DynExp::DynExpCore& Core) const override { return DynExp::MakeParams<NetworkDataStreamInstrumentConfigurator>(ID, Core); }
	};

	/**
	 * @brief Data stream instrument for bidirectional gRPC communication.
	 * @tparam BaseInstr BaseInstr Meta instrument the parent class @p DynExpInstr::gRPCInstrument derives from.
	 * @tparam enable_if_t Internal check whether @p BaseInstr is derived from @p DynExpInstr::DataStreamInstrument. Pass 0.
	 * @tparam ...gRPCStubs gRPC stub classes of derived instruments which are used by them to transfer data. 
	*/
	template <typename BaseInstr, typename std::enable_if_t<std::is_base_of_v<DataStreamInstrument, BaseInstr>, int>, typename... gRPCStubs>
	class NetworkDataStreamInstrumentT : public gRPCInstrument<BaseInstr, 0, gRPCStubs...>
	{
	public:
		using ParamsType = NetworkDataStreamInstrumentParams<BaseInstr, 0, gRPCStubs...>;
		using ConfigType = NetworkDataStreamInstrumentConfigurator<BaseInstr, 0, gRPCStubs...>;
		using InstrumentDataType = NetworkDataStreamInstrumentData<BaseInstr, 0, gRPCStubs...>;

		constexpr static auto Name() noexcept { return "Network Data Stream Instrument"; }

		NetworkDataStreamInstrumentT(const std::thread::id OwnerThreadID, DynExp::ParamsBasePtrType&& Params)
			: gRPCInstrument<BaseInstr, 0, gRPCStubs...>(OwnerThreadID, std::move(Params)) {}
		virtual ~NetworkDataStreamInstrumentT() {}

		virtual std::string GetName() const override { return Name(); }

		/**
		 * @brief Read remote instrument's state periodically.
		 * @return See DynExp::InstrumentBase::GetTaskQueueDelay().
		*/
		virtual std::chrono::milliseconds GetTaskQueueDelay() const { return std::chrono::milliseconds(500); }

		virtual Util::OptionalBool HasFinished() const override
		{
			auto InstrData = dynamic_InstrumentData_cast<NetworkDataStreamInstrumentT>(this->GetInstrumentData());
			return InstrData->HasFinished();
		}

		virtual Util::OptionalBool IsRunning() const override
		{
			auto InstrData = dynamic_InstrumentData_cast<NetworkDataStreamInstrumentT>(this->GetInstrumentData());
			return InstrData->IsRunning();
		}

		// Tasks
		virtual void ReadData(DynExp::TaskBase::CallbackType CallbackFunc = nullptr) const override { DynExp::InstrumentBase::MakeAndEnqueueTask<NetworkDataStreamInstrumentTasks::ReadTask<BaseInstr, 0, gRPCStubs...>>(CallbackFunc); }
		virtual void WriteData(DynExp::TaskBase::CallbackType CallbackFunc = nullptr) const override { DynExp::InstrumentBase::MakeAndEnqueueTask<NetworkDataStreamInstrumentTasks::WriteTask<BaseInstr, 0, gRPCStubs...>>(CallbackFunc); }
		virtual void ClearData(DynExp::TaskBase::CallbackType CallbackFunc = nullptr) const override { DynExp::InstrumentBase::MakeAndEnqueueTask<NetworkDataStreamInstrumentTasks::ClearTask<BaseInstr, 0, gRPCStubs...>>(CallbackFunc); }
		virtual void Start(DynExp::TaskBase::CallbackType CallbackFunc = nullptr) const override { DynExp::InstrumentBase::MakeAndEnqueueTask<NetworkDataStreamInstrumentTasks::StartTask<BaseInstr, 0, gRPCStubs...>>(CallbackFunc); }
		virtual void Stop(DynExp::TaskBase::CallbackType CallbackFunc = nullptr) const override { DynExp::InstrumentBase::MakeAndEnqueueTask<NetworkDataStreamInstrumentTasks::StopTask<BaseInstr, 0, gRPCStubs...>>(CallbackFunc); }
		virtual void Restart(DynExp::TaskBase::CallbackType CallbackFunc = nullptr) const override { DynExp::InstrumentBase::MakeAndEnqueueTask<NetworkDataStreamInstrumentTasks::RestartTask<BaseInstr, 0, gRPCStubs...>>(CallbackFunc); }
		virtual void SetStreamSize(size_t BufferSizeInSamples, DynExp::TaskBase::CallbackType CallbackFunc = nullptr) const override { DynExp::InstrumentBase::MakeAndEnqueueTask<NetworkDataStreamInstrumentTasks::SetStreamSizeTask<BaseInstr, 0, gRPCStubs...>>(BufferSizeInSamples, CallbackFunc); }
		virtual void ResetStreamSize(DynExp::TaskBase::CallbackType CallbackFunc = nullptr) const override { DynExp::InstrumentBase::MakeAndEnqueueTask<NetworkDataStreamInstrumentTasks::ResetStreamSizeTask<BaseInstr, 0, gRPCStubs...>>(CallbackFunc); }

	private:
		void ResetImpl(DynExp::Object::dispatch_tag<gRPCInstrument<BaseInstr, 0, gRPCStubs...>>) override final
		{
			ResetImpl(DynExp::Object::dispatch_tag<NetworkDataStreamInstrumentT>());
		}

		virtual void ResetImpl(DynExp::Object::dispatch_tag<NetworkDataStreamInstrumentT>) {}

		virtual std::unique_ptr<DynExp::InitTaskBase> MakeInitTask() const override { return DynExp::MakeTask<NetworkDataStreamInstrumentTasks::InitTask<BaseInstr, 0, gRPCStubs...>>(); }
		virtual std::unique_ptr<DynExp::ExitTaskBase> MakeExitTask() const override { return DynExp::MakeTask<NetworkDataStreamInstrumentTasks::ExitTask<BaseInstr, 0, gRPCStubs...>>(); }
		virtual std::unique_ptr<DynExp::UpdateTaskBase> MakeUpdateTask() const override { return DynExp::MakeTask<NetworkDataStreamInstrumentTasks::UpdateTask<BaseInstr, 0, gRPCStubs...>>(); }
	};

	/**
	 * @brief Explicit instantiation of derivable class NetworkDataStreamInstrumentT to create the network data stream instrument.
	*/
	class NetworkDataStreamInstrument : public NetworkDataStreamInstrumentT<DataStreamInstrument, 0, DynExpProto::NetworkDataStreamInstrument::NetworkDataStreamInstrument>
	{
	public:
		using StubType = DynExpProto::NetworkDataStreamInstrument::NetworkDataStreamInstrument;

		NetworkDataStreamInstrument(const std::thread::id OwnerThreadID, DynExp::ParamsBasePtrType&& Params)
			: NetworkDataStreamInstrumentT<DataStreamInstrument, 0, StubType>(OwnerThreadID, std::move(Params)) {}
		virtual ~NetworkDataStreamInstrument() {}

		// Only available if derived directly from DataStreamInstrument since other BaseInstr might already override this function differently.
		virtual DataStreamInstrumentData::UnitType GetValueUnit() const override
		{
			auto InstrData = dynamic_InstrumentData_cast<NetworkDataStreamInstrumentT>(this->GetInstrumentData());
			return InstrData->GetValueUnit();
		}
	};
}