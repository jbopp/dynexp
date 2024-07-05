// This file is part of DynExp.

/**
 * @file NetworkTimeTagger.h
 * @brief Implementation of a gRPC client instrument to access a remote time tagger meta instrument.
*/

#pragma once

#include "stdafx.h"
#include "MetaInstruments/TimeTagger.h"
#include "Instruments/NetworkDataStreamInstrument.h"

#include "NetworkTimeTagger.pb.h"
#include "NetworkTimeTagger.grpc.pb.h"

namespace DynExpInstr
{
	template <typename BaseInstr, typename std::enable_if_t<std::is_base_of_v<TimeTagger, BaseInstr>, int>, typename... gRPCStubs>
	class NetworkTimeTaggerT;

	namespace NetworkTimeTaggerTasks
	{
		template <typename BaseInstr, typename std::enable_if_t<std::is_base_of_v<TimeTagger, BaseInstr>, int>, typename... gRPCStubs>
		class InitTask : public NetworkDataStreamInstrumentTasks::InitTask<BaseInstr, 0, gRPCStubs...>
		{
			void InitFuncImpl(DynExp::InitTaskBase::dispatch_tag<NetworkDataStreamInstrumentTasks::InitTask<BaseInstr, 0, gRPCStubs...>>, DynExp::InstrumentInstance& Instance) override final
			{
				StubPtrType<DynExpProto::NetworkTimeTagger::NetworkTimeTagger> StubPtr;
				{
					auto InstrData = dynamic_InstrumentData_cast<NetworkTimeTaggerT<BaseInstr, 0, gRPCStubs...>>(Instance.InstrumentDataGetter());
					StubPtr = InstrData->template GetStub<DynExpProto::NetworkTimeTagger::NetworkTimeTagger>();
				} // InstrData unlocked here.

				auto HardwareInfoResponse = InvokeStubFunc(StubPtr, &DynExpProto::NetworkTimeTagger::NetworkTimeTagger::Stub::GetHardwareInfo, {});

				{
					auto InstrData = dynamic_InstrumentData_cast<NetworkTimeTaggerT<BaseInstr, 0, gRPCStubs...>>(Instance.InstrumentDataGetter());

					InstrData->MinThresholdInVolts = HardwareInfoResponse.mininputthresholdinvolts();
					InstrData->MaxThresholdInVolts = HardwareInfoResponse.maxinputthresholdinvolts();
					InstrData->Resolution = Util::picoseconds(HardwareInfoResponse.timingresolutioninpicoseconds());
				}  // InstrData unlocked here.

				InitFuncImpl(DynExp::InitTaskBase::dispatch_tag<InitTask>(), Instance);
			}

			virtual void InitFuncImpl(DynExp::InitTaskBase::dispatch_tag<InitTask>, DynExp::InstrumentInstance& Instance) {}
		};

		template <typename BaseInstr, typename std::enable_if_t<std::is_base_of_v<TimeTagger, BaseInstr>, int>, typename... gRPCStubs>
		class ExitTask : public NetworkDataStreamInstrumentTasks::ExitTask<BaseInstr, 0, gRPCStubs...>
		{
			void ExitFuncImpl(DynExp::ExitTaskBase::dispatch_tag<NetworkDataStreamInstrumentTasks::ExitTask<BaseInstr, 0, gRPCStubs...>>, DynExp::InstrumentInstance& Instance) override final
			{
				ExitFuncImpl(DynExp::ExitTaskBase::dispatch_tag<ExitTask>(), Instance);
			}

			virtual void ExitFuncImpl(DynExp::ExitTaskBase::dispatch_tag<ExitTask>, DynExp::InstrumentInstance& Instance) {}
		};

		template <typename BaseInstr, typename std::enable_if_t<std::is_base_of_v<TimeTagger, BaseInstr>, int>, typename... gRPCStubs>
		class UpdateTask : public NetworkDataStreamInstrumentTasks::UpdateTask<BaseInstr, 0, gRPCStubs...>
		{
			void UpdateFuncImpl(DynExp::UpdateTaskBase::dispatch_tag<NetworkDataStreamInstrumentTasks::UpdateTask<BaseInstr, 0, gRPCStubs...>>, DynExp::InstrumentInstance& Instance) override final
			{
				StubPtrType<DynExpProto::NetworkTimeTagger::NetworkTimeTagger> StubPtr;
				bool StreamModeChanged = false;
				TimeTaggerData::StreamModeType StreamMode{};

				{
					auto InstrData = dynamic_InstrumentData_cast<NetworkTimeTaggerT<BaseInstr, 0, gRPCStubs...>>(Instance.InstrumentDataGetter());
					StubPtr = InstrData->template GetStub<DynExpProto::NetworkTimeTagger::NetworkTimeTagger>();

					StreamModeChanged = InstrData->GetStreamModeChanged();
					StreamMode = InstrData->GetStreamMode();
					InstrData->ClearStreamModeChanged();
				} // InstrData unlocked here.

				if (StreamModeChanged)
				{
					DynExpProto::NetworkTimeTagger::StreamModeMessage StreamModeMsg;
					StreamModeMsg.set_streammode(StreamMode == TimeTaggerData::StreamModeType::Counts ?
						DynExpProto::NetworkTimeTagger::StreamModeType::Counts : DynExpProto::NetworkTimeTagger::StreamModeType::Events);

					InvokeStubFunc(StubPtr, &DynExpProto::NetworkTimeTagger::NetworkTimeTagger::Stub::SetStreamMode, StreamModeMsg);
				}

				auto BufferInfoResponse = InvokeStubFunc(StubPtr, &DynExpProto::NetworkTimeTagger::NetworkTimeTagger::Stub::GetBufferInfo, {});
				auto StreamModeResponse = InvokeStubFunc(StubPtr, &DynExpProto::NetworkTimeTagger::NetworkTimeTagger::Stub::GetStreamMode, {});

				{
					auto InstrData = dynamic_InstrumentData_cast<NetworkTimeTaggerT<BaseInstr, 0, gRPCStubs...>>(Instance.InstrumentDataGetter());

					InstrData->BufferSizeInSamples = Util::NumToT<size_t>(BufferInfoResponse.buffersizeinsamples());

					// StreamMode might have changed in between since InstrData was unlocked during remote procedure call.
					// Only update if this was not the case. SetStreamMode() sets the 'changed' flag which is reset by
					// ClearStreamModeChanged(). This is necessary in order not to write the read stream mode back to the server.
					if (!InstrData->GetStreamModeChanged())
					{
						InstrData->SetStreamMode(StreamModeResponse.streammode() == DynExpProto::NetworkTimeTagger::StreamModeType::Counts ?
							TimeTaggerData::StreamModeType::Counts : TimeTaggerData::StreamModeType::Events);
						InstrData->ClearStreamModeChanged();
					}
				}  // InstrData unlocked here.

				UpdateFuncImpl(DynExp::UpdateTaskBase::dispatch_tag<UpdateTask>(), Instance);
			}

			virtual void UpdateFuncImpl(DynExp::UpdateTaskBase::dispatch_tag<UpdateTask>, DynExp::InstrumentInstance& Instance) {}
		};

		template <typename BaseInstr, typename std::enable_if_t<std::is_base_of_v<TimeTagger, BaseInstr>, int>, typename... gRPCStubs>
		class ReadTask : public DynExp::TaskBase
		{
		public:
			ReadTask(CallbackType CallbackFunc) noexcept : TaskBase(CallbackFunc) {}

		private:
			virtual DynExp::TaskResultType RunChild(DynExp::InstrumentInstance& Instance) override
			{
				StubPtrType<DynExpProto::NetworkTimeTagger::NetworkTimeTagger> StubPtr;
				{
					auto InstrData = dynamic_InstrumentData_cast<NetworkTimeTaggerT<BaseInstr, 0, gRPCStubs...>>(Instance.InstrumentDataGetter());
					StubPtr = InstrData->template GetStub<DynExpProto::NetworkTimeTagger::NetworkTimeTagger>();
				} // InstrData unlocked here.

				auto HBTResultsResponse = InvokeStubFunc(StubPtr, &DynExpProto::NetworkTimeTagger::NetworkTimeTagger::Stub::GetHBTResults, {});

				{
					auto InstrData = dynamic_InstrumentData_cast<NetworkTimeTaggerT<BaseInstr, 0, gRPCStubs...>>(Instance.InstrumentDataGetter());

					InstrData->GetHBTResults().Enabled = HBTResultsResponse.enabled();
					InstrData->GetHBTResults().EventCounts = Util::NumToT<decltype(TimeTaggerData::HBTResultsType::EventCounts)>(HBTResultsResponse.eventcounts());
					InstrData->GetHBTResults().IntegrationTime = std::chrono::microseconds(HBTResultsResponse.integrationtimeinmicroseconds());
					
					InstrData->GetHBTResults().ResultVector.clear();
					for (decltype(HBTResultsResponse.results_size()) i = 0; i < HBTResultsResponse.results_size(); ++i)
						InstrData->GetHBTResults().ResultVector.emplace_back(HBTResultsResponse.results(i).value(), HBTResultsResponse.results(i).time());
				}  // InstrData unlocked here.

				return {};
			}
		};

		template <typename BaseInstr, typename std::enable_if_t<std::is_base_of_v<TimeTagger, BaseInstr>, int>, typename... gRPCStubs>
		class ClearTask : public DynExp::TaskBase
		{
		public:
			ClearTask(CallbackType CallbackFunc) noexcept : TaskBase(CallbackFunc) {}

		private:
			virtual DynExp::TaskResultType RunChild(DynExp::InstrumentInstance& Instance) override
			{
				StubPtrType<DynExpProto::NetworkTimeTagger::NetworkTimeTagger> StubPtr;
				{
					auto InstrData = dynamic_InstrumentData_cast<NetworkTimeTaggerT<BaseInstr, 0, gRPCStubs...>>(Instance.InstrumentDataGetter());
					StubPtr = InstrData->template GetStub<DynExpProto::NetworkTimeTagger::NetworkTimeTagger>();
				} // InstrData unlocked here.

				InvokeStubFunc(StubPtr, &DynExpProto::NetworkTimeTagger::NetworkTimeTagger::Stub::ClearBuffer, {});

				return {};
			}
		};

		template <typename BaseInstr, typename std::enable_if_t<std::is_base_of_v<TimeTagger, BaseInstr>, int>, typename... gRPCStubs>
		class ConfigureInputTask : public DynExp::TaskBase
		{
		public:
			ConfigureInputTask(bool UseRisingEdge, double ThresholdInVolts, CallbackType CallbackFunc) noexcept
				: TaskBase(CallbackFunc), UseRisingEdge(UseRisingEdge), ThresholdInVolts(ThresholdInVolts) {}

		private:
			virtual DynExp::TaskResultType RunChild(DynExp::InstrumentInstance& Instance) override
			{
				DynExpProto::NetworkTimeTagger::ConfigureInputMessage ConfigureInputMsg;
				ConfigureInputMsg.set_userisingedge(UseRisingEdge);
				ConfigureInputMsg.set_thresholdinvolts(ThresholdInVolts);

				StubPtrType<DynExpProto::NetworkTimeTagger::NetworkTimeTagger> StubPtr;
				{
					auto InstrData = dynamic_InstrumentData_cast<NetworkTimeTaggerT<BaseInstr, 0, gRPCStubs...>>(Instance.InstrumentDataGetter());
					StubPtr = InstrData->template GetStub<DynExpProto::NetworkTimeTagger::NetworkTimeTagger>();
				} // InstrData unlocked here.

				InvokeStubFunc(StubPtr, &DynExpProto::NetworkTimeTagger::NetworkTimeTagger::Stub::ConfigureInput, ConfigureInputMsg);

				return {};
			}

			const bool UseRisingEdge;
			const double ThresholdInVolts;
		};

		template <typename BaseInstr, typename std::enable_if_t<std::is_base_of_v<TimeTagger, BaseInstr>, int>, typename... gRPCStubs>
		class SetExposureTimeTask : public DynExp::TaskBase
		{
		public:
			SetExposureTimeTask(Util::picoseconds ExposureTime, CallbackType CallbackFunc) noexcept
				: TaskBase(CallbackFunc), ExposureTime(ExposureTime) {}

		private:
			virtual DynExp::TaskResultType RunChild(DynExp::InstrumentInstance& Instance) override
			{
				DynExpProto::NetworkTimeTagger::ExposureTimeMessage ExposureTimeMsg;
				ExposureTimeMsg.set_exposuretimeinpicoseconds(ExposureTime.count());

				StubPtrType<DynExpProto::NetworkTimeTagger::NetworkTimeTagger> StubPtr;
				{
					auto InstrData = dynamic_InstrumentData_cast<NetworkTimeTaggerT<BaseInstr, 0, gRPCStubs...>>(Instance.InstrumentDataGetter());
					StubPtr = InstrData->template GetStub<DynExpProto::NetworkTimeTagger::NetworkTimeTagger>();
				} // InstrData unlocked here.

				InvokeStubFunc(StubPtr, &DynExpProto::NetworkTimeTagger::NetworkTimeTagger::Stub::SetExposureTime, ExposureTimeMsg);

				return {};
			}

			const Util::picoseconds ExposureTime;
		};

		template <typename BaseInstr, typename std::enable_if_t<std::is_base_of_v<TimeTagger, BaseInstr>, int>, typename... gRPCStubs>
		class SetCoincidenceWindowTask : public DynExp::TaskBase
		{
		public:
			SetCoincidenceWindowTask(Util::picoseconds CoincidenceWindow, CallbackType CallbackFunc) noexcept
				: TaskBase(CallbackFunc), CoincidenceWindow(CoincidenceWindow) {}

		private:
			virtual DynExp::TaskResultType RunChild(DynExp::InstrumentInstance& Instance) override
			{
				DynExpProto::NetworkTimeTagger::CoincidenceWindowMessage CoincidenceWindowMsg;
				CoincidenceWindowMsg.set_coincidencewindowinpicoseconds(CoincidenceWindow.count());

				StubPtrType<DynExpProto::NetworkTimeTagger::NetworkTimeTagger> StubPtr;
				{
					auto InstrData = dynamic_InstrumentData_cast<NetworkTimeTaggerT<BaseInstr, 0, gRPCStubs...>>(Instance.InstrumentDataGetter());
					StubPtr = InstrData->template GetStub<DynExpProto::NetworkTimeTagger::NetworkTimeTagger>();
				} // InstrData unlocked here.

				InvokeStubFunc(StubPtr, &DynExpProto::NetworkTimeTagger::NetworkTimeTagger::Stub::SetCoincidenceWindow, CoincidenceWindowMsg);

				return {};
			}

			const Util::picoseconds CoincidenceWindow;
		};

		template <typename BaseInstr, typename std::enable_if_t<std::is_base_of_v<TimeTagger, BaseInstr>, int>, typename... gRPCStubs>
		class SetDelayTask : public DynExp::TaskBase
		{
		public:
			SetDelayTask(Util::picoseconds Delay, CallbackType CallbackFunc) noexcept
				: TaskBase(CallbackFunc), Delay(Delay) {}

		private:
			virtual DynExp::TaskResultType RunChild(DynExp::InstrumentInstance& Instance) override
			{
				DynExpProto::NetworkTimeTagger::InputDelayMessage InputDelayMsg;
				InputDelayMsg.set_delayinpicoseconds(Delay.count());

				StubPtrType<DynExpProto::NetworkTimeTagger::NetworkTimeTagger> StubPtr;
				{
					auto InstrData = dynamic_InstrumentData_cast<NetworkTimeTaggerT<BaseInstr, 0, gRPCStubs...>>(Instance.InstrumentDataGetter());
					StubPtr = InstrData->template GetStub<DynExpProto::NetworkTimeTagger::NetworkTimeTagger>();
				} // InstrData unlocked here.

				InvokeStubFunc(StubPtr, &DynExpProto::NetworkTimeTagger::NetworkTimeTagger::Stub::SetInputDelay, InputDelayMsg);

				return {};
			}

			const Util::picoseconds Delay;
		};

		template <typename BaseInstr, typename std::enable_if_t<std::is_base_of_v<TimeTagger, BaseInstr>, int>, typename... gRPCStubs>
		class SetHBTActiveTask : public DynExp::TaskBase
		{
		public:
			SetHBTActiveTask(bool Enable, CallbackType CallbackFunc) noexcept
				: TaskBase(CallbackFunc), Enable(Enable) {}

		private:
			virtual DynExp::TaskResultType RunChild(DynExp::InstrumentInstance& Instance) override
			{
				DynExpProto::NetworkTimeTagger::HBTActiveMessage HBTActiveMsg;
				HBTActiveMsg.set_enable(Enable);

				StubPtrType<DynExpProto::NetworkTimeTagger::NetworkTimeTagger> StubPtr;
				{
					auto InstrData = dynamic_InstrumentData_cast<NetworkTimeTaggerT<BaseInstr, 0, gRPCStubs...>>(Instance.InstrumentDataGetter());
					StubPtr = InstrData->template GetStub<DynExpProto::NetworkTimeTagger::NetworkTimeTagger>();
				} // InstrData unlocked here.

				InvokeStubFunc(StubPtr, &DynExpProto::NetworkTimeTagger::NetworkTimeTagger::Stub::SetHBTActive, HBTActiveMsg);

				return {};
			}

			const bool Enable;
		};

		template <typename BaseInstr, typename std::enable_if_t<std::is_base_of_v<TimeTagger, BaseInstr>, int>, typename... gRPCStubs>
		class ConfigureHBTTask : public DynExp::TaskBase
		{
		public:
			ConfigureHBTTask(Util::picoseconds BinWidth, size_t BinCount, CallbackType CallbackFunc) noexcept
				: TaskBase(CallbackFunc), BinWidth(BinWidth), BinCount(BinCount) {}

		private:
			virtual DynExp::TaskResultType RunChild(DynExp::InstrumentInstance& Instance) override
			{
				DynExpProto::NetworkTimeTagger::ConfigureHBTMessage ConfigureHBTMsg;
				ConfigureHBTMsg.set_binwidthinpicoseconds(BinWidth.count());
				ConfigureHBTMsg.set_bincount(Util::NumToT<google::protobuf::uint64>(BinCount));

				StubPtrType<DynExpProto::NetworkTimeTagger::NetworkTimeTagger> StubPtr;
				{
					auto InstrData = dynamic_InstrumentData_cast<NetworkTimeTaggerT<BaseInstr, 0, gRPCStubs...>>(Instance.InstrumentDataGetter());
					StubPtr = InstrData->template GetStub<DynExpProto::NetworkTimeTagger::NetworkTimeTagger>();
				} // InstrData unlocked here.

				InvokeStubFunc(StubPtr, &DynExpProto::NetworkTimeTagger::NetworkTimeTagger::Stub::ConfigureHBT, ConfigureHBTMsg);

				return {};
			}

			const Util::picoseconds BinWidth;
			const size_t BinCount;
		};

		template <typename BaseInstr, typename std::enable_if_t<std::is_base_of_v<TimeTagger, BaseInstr>, int>, typename... gRPCStubs>
		class ResetHBTTask : public DynExp::TaskBase
		{
		public:
			ResetHBTTask(CallbackType CallbackFunc) noexcept : TaskBase(CallbackFunc) {}

		private:
			virtual DynExp::TaskResultType RunChild(DynExp::InstrumentInstance& Instance) override
			{
				StubPtrType<DynExpProto::NetworkTimeTagger::NetworkTimeTagger> StubPtr;
				{
					auto InstrData = dynamic_InstrumentData_cast<NetworkTimeTaggerT<BaseInstr, 0, gRPCStubs...>>(Instance.InstrumentDataGetter());
					StubPtr = InstrData->template GetStub<DynExpProto::NetworkTimeTagger::NetworkTimeTagger>();
				} // InstrData unlocked here.

				InvokeStubFunc(StubPtr, &DynExpProto::NetworkTimeTagger::NetworkTimeTagger::Stub::ResetHBT, {});

				return {};
			}
		};
	}

	template <typename BaseInstr, typename std::enable_if_t<std::is_base_of_v<TimeTagger, BaseInstr>, int>, typename... gRPCStubs>
	class NetworkTimeTaggerData : public NetworkDataStreamInstrumentData<BaseInstr, 0, gRPCStubs...>
	{
		friend class NetworkTimeTaggerTasks::InitTask<BaseInstr, 0, gRPCStubs...>;
		friend class NetworkTimeTaggerTasks::UpdateTask<BaseInstr, 0, gRPCStubs...>;

	public:
		using InstrumentType = NetworkTimeTaggerT<BaseInstr, 0, gRPCStubs...>;

		NetworkTimeTaggerData(size_t BufferSizeInSamples = 1)
			: NetworkDataStreamInstrumentData<BaseInstr, 0, gRPCStubs...>(BufferSizeInSamples) {}
		virtual ~NetworkTimeTaggerData() = default;

		auto GetMinThresholdInVolts() const noexcept { return MinThresholdInVolts; }
		auto GetMaxThresholdInVolts() const noexcept { return MaxThresholdInVolts; }
		auto GetResolution() const noexcept { return Resolution; }
		auto GetBufferSizeInSamples() const noexcept { return BufferSizeInSamples; }

	private:
		void ResetImpl(DynExp::InstrumentDataBase::dispatch_tag<NetworkDataStreamInstrumentData<BaseInstr, 0, gRPCStubs...>>) override final
		{
			MinThresholdInVolts = 0;
			MaxThresholdInVolts = 0;
			Resolution = {};
			BufferSizeInSamples = 0;

			ResetImpl(DynExp::InstrumentDataBase::dispatch_tag<NetworkTimeTaggerData>());
		}

		virtual void ResetImpl(DynExp::InstrumentDataBase::dispatch_tag<NetworkTimeTaggerData>) {};

		double MinThresholdInVolts;
		double MaxThresholdInVolts;
		Util::picoseconds Resolution;
		size_t BufferSizeInSamples;
	};

	template <typename BaseInstr, typename std::enable_if_t<std::is_base_of_v<TimeTagger, BaseInstr>, int>, typename... gRPCStubs>
	class NetworkTimeTaggerParams : public NetworkDataStreamInstrumentParams<BaseInstr, 0, gRPCStubs...>
	{
	public:
		NetworkTimeTaggerParams(DynExp::ItemIDType ID, const DynExp::DynExpCore& Core) : NetworkDataStreamInstrumentParams<BaseInstr, 0, gRPCStubs...>(ID, Core) {}
		virtual ~NetworkTimeTaggerParams() = default;

		virtual const char* GetParamClassTag() const noexcept override { return "NetworkTimeTaggerParams"; }

	private:
		void ConfigureParamsImpl(DynExp::ParamsBase::dispatch_tag<NetworkDataStreamInstrumentParams<BaseInstr, 0, gRPCStubs...>>) override final
		{
			ConfigureParamsImpl(DynExp::ParamsBase::dispatch_tag<NetworkTimeTaggerParams>());
		}
		
		virtual void ConfigureParamsImpl(DynExp::ParamsBase::dispatch_tag<NetworkTimeTaggerParams>) {}

		DynExp::ParamsBase::DummyParam Dummy = { *this };
	};

	template <typename BaseInstr, typename std::enable_if_t<std::is_base_of_v<TimeTagger, BaseInstr>, int>, typename... gRPCStubs>
	class NetworkTimeTaggerConfigurator : public NetworkDataStreamInstrumentConfigurator<BaseInstr, 0, gRPCStubs...>
	{
	public:
		using ObjectType = NetworkTimeTaggerT<BaseInstr, 0, gRPCStubs...>;
		using ParamsType = NetworkTimeTaggerParams<BaseInstr, 0, gRPCStubs...>;

		NetworkTimeTaggerConfigurator() = default;
		virtual ~NetworkTimeTaggerConfigurator() = default;

	private:
		virtual DynExp::ParamsBasePtrType MakeParams(DynExp::ItemIDType ID, const DynExp::DynExpCore& Core) const override { return DynExp::MakeParams<NetworkTimeTaggerConfigurator>(ID, Core); }
	};

	template <typename BaseInstr, typename std::enable_if_t<std::is_base_of_v<TimeTagger, BaseInstr>, int>, typename... gRPCStubs>
	class NetworkTimeTaggerT : public NetworkDataStreamInstrumentT<BaseInstr, 0, gRPCStubs...>
	{
	public:
		using ParamsType = NetworkTimeTaggerParams<BaseInstr, 0, gRPCStubs...>;
		using ConfigType = NetworkTimeTaggerConfigurator<BaseInstr, 0, gRPCStubs...>;
		using InstrumentDataType = NetworkTimeTaggerData<BaseInstr, 0, gRPCStubs...>;
		using StubType = DynExpProto::NetworkTimeTagger::NetworkTimeTagger;

		constexpr static auto Name() noexcept { return "Network Time Tagger"; }

		NetworkTimeTaggerT(const std::thread::id OwnerThreadID, DynExp::ParamsBasePtrType&& Params)
			: NetworkDataStreamInstrumentT<BaseInstr, 0, gRPCStubs...>(OwnerThreadID, std::move(Params)) {}
		virtual ~NetworkTimeTaggerT() {}

		virtual std::string GetName() const override { return Name(); }

		virtual double GetMinThresholdInVolts() const noexcept override
		{
			auto InstrData = dynamic_InstrumentData_cast<NetworkTimeTaggerT>(this->GetInstrumentData());
			return InstrData->GetMinThresholdInVolts();
		}

		virtual double GetMaxThresholdInVolts() const noexcept override
		{
			auto InstrData = dynamic_InstrumentData_cast<NetworkTimeTaggerT>(this->GetInstrumentData());
			return InstrData->GetMaxThresholdInVolts();
		}

		virtual Util::picoseconds GetResolution() const override
		{
			auto InstrData = dynamic_InstrumentData_cast<NetworkTimeTaggerT>(this->GetInstrumentData());
			return InstrData->GetResolution();
		}

		virtual size_t GetBufferSize() const override
		{
			auto InstrData = dynamic_InstrumentData_cast<NetworkTimeTaggerT>(this->GetInstrumentData());
			return InstrData->GetBufferSizeInSamples();
		}

		// Logical const-ness: const member functions to allow inserting tasks into task queue.
		// void ResetStreamSize(DynExp::TaskBase::CallbackType) const implemented by base class NetworkDataStreamInstrument.
		virtual void ReadData(DynExp::TaskBase::CallbackType CallbackFunc = nullptr) const override
		{
			NetworkDataStreamInstrumentT<BaseInstr, 0, gRPCStubs...>::ReadData();
			DynExp::InstrumentBase::MakeAndEnqueueTask<NetworkTimeTaggerTasks::ReadTask<BaseInstr, 0, gRPCStubs...>>(CallbackFunc);
		}
		
		virtual void Clear(DynExp::TaskBase::CallbackType CallbackFunc = nullptr) const override { DynExp::InstrumentBase::MakeAndEnqueueTask<NetworkTimeTaggerTasks::ClearTask<BaseInstr, 0, gRPCStubs...>>(CallbackFunc); }
		virtual void ConfigureInput(bool UseRisingEdge, double ThresholdInVolts, DynExp::TaskBase::CallbackType CallbackFunc = nullptr) const override { DynExp::InstrumentBase::MakeAndEnqueueTask<NetworkTimeTaggerTasks::ConfigureInputTask<BaseInstr, 0, gRPCStubs...>>(UseRisingEdge, ThresholdInVolts, CallbackFunc); }
		virtual void SetExposureTime(Util::picoseconds ExposureTime, DynExp::TaskBase::CallbackType CallbackFunc = nullptr) const override { DynExp::InstrumentBase::MakeAndEnqueueTask<NetworkTimeTaggerTasks::SetExposureTimeTask<BaseInstr, 0, gRPCStubs...>>(ExposureTime, CallbackFunc); }
		virtual void SetCoincidenceWindow(Util::picoseconds CoincidenceWindow, DynExp::TaskBase::CallbackType CallbackFunc = nullptr) const override { DynExp::InstrumentBase::MakeAndEnqueueTask<NetworkTimeTaggerTasks::SetCoincidenceWindowTask<BaseInstr, 0, gRPCStubs...>>(CoincidenceWindow, CallbackFunc); }
		virtual void SetDelay(Util::picoseconds Delay, DynExp::TaskBase::CallbackType CallbackFunc = nullptr) const override { DynExp::InstrumentBase::MakeAndEnqueueTask<NetworkTimeTaggerTasks::SetDelayTask<BaseInstr, 0, gRPCStubs...>>(Delay, CallbackFunc); }
		virtual void SetHBTActive(bool Enable, DynExp::TaskBase::CallbackType CallbackFunc = nullptr) const override { DynExp::InstrumentBase::MakeAndEnqueueTask<NetworkTimeTaggerTasks::SetHBTActiveTask<BaseInstr, 0, gRPCStubs...>>(Enable, CallbackFunc); }
		virtual void ConfigureHBT(Util::picoseconds BinWidth, size_t BinCount, DynExp::TaskBase::CallbackType CallbackFunc = nullptr) const override { DynExp::InstrumentBase::MakeAndEnqueueTask<NetworkTimeTaggerTasks::ConfigureHBTTask<BaseInstr, 0, gRPCStubs...>>(BinWidth, BinCount, CallbackFunc); }
		virtual void ResetHBT(DynExp::TaskBase::CallbackType CallbackFunc = nullptr) const override { DynExp::InstrumentBase::MakeAndEnqueueTask<NetworkTimeTaggerTasks::ResetHBTTask<BaseInstr, 0, gRPCStubs...>>(CallbackFunc); }

	private:
		void ResetImpl(DynExp::Object::dispatch_tag<NetworkDataStreamInstrumentT<BaseInstr, 0, gRPCStubs...>>) override final
		{
			ResetImpl(DynExp::Object::dispatch_tag<NetworkTimeTaggerT>());
		}

		virtual void ResetImpl(DynExp::Object::dispatch_tag<NetworkTimeTaggerT>) {}

		virtual std::unique_ptr<DynExp::InitTaskBase> MakeInitTask() const override { return DynExp::MakeTask<NetworkTimeTaggerTasks::InitTask<BaseInstr, 0, gRPCStubs...>>(); }
		virtual std::unique_ptr<DynExp::ExitTaskBase> MakeExitTask() const override { return DynExp::MakeTask<NetworkTimeTaggerTasks::ExitTask<BaseInstr, 0, gRPCStubs...>>(); }
		virtual std::unique_ptr<DynExp::UpdateTaskBase> MakeUpdateTask() const override { return DynExp::MakeTask<NetworkTimeTaggerTasks::UpdateTask<BaseInstr, 0, gRPCStubs...>>(); }
	};

	/**
	 * @brief Explicit instantiation of derivable class NetworkTimeTaggerT to create the network time tagger instrument.
	*/
	using NetworkTimeTagger = NetworkTimeTaggerT<TimeTagger, 0, typename NetworkDataStreamInstrument::StubType, DynExpProto::NetworkTimeTagger::NetworkTimeTagger>;
}