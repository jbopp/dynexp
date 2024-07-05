// This file is part of DynExp.

/**
 * @file NetworkDigitalOutInstr.h
 * @brief Implementation of a gRPC client instrument to access a remote digital out meta instrument.
*/

#pragma once

#include "stdafx.h"
#include "MetaInstruments/DigitalOut.h"
#include "Instruments/NetworkDataStreamInstrument.h"

#include "NetworkDigitalOut.pb.h"
#include "NetworkDigitalOut.grpc.pb.h"

namespace DynExpInstr
{
	template <typename BaseInstr, typename std::enable_if_t<std::is_base_of_v<DigitalOut, BaseInstr>, int>, typename... gRPCStubs>
	class NetworkDigitalOutT;

	namespace NetworkDigitalOutTasks
	{
		template <typename BaseInstr, typename std::enable_if_t<std::is_base_of_v<DigitalOut, BaseInstr>, int>, typename... gRPCStubs>
		class InitTask : public NetworkDataStreamInstrumentTasks::InitTask<BaseInstr, 0, gRPCStubs...>
		{
			void InitFuncImpl(DynExp::InitTaskBase::dispatch_tag<NetworkDataStreamInstrumentTasks::InitTask<BaseInstr, 0, gRPCStubs...>>, DynExp::InstrumentInstance& Instance) override final
			{
				InitFuncImpl(DynExp::InitTaskBase::dispatch_tag<InitTask>(), Instance);
			}

			virtual void InitFuncImpl(DynExp::InitTaskBase::dispatch_tag<InitTask>, DynExp::InstrumentInstance& Instance) {}
		};

		template <typename BaseInstr, typename std::enable_if_t<std::is_base_of_v<DigitalOut, BaseInstr>, int>, typename... gRPCStubs>
		class ExitTask : public NetworkDataStreamInstrumentTasks::ExitTask<BaseInstr, 0, gRPCStubs...>
		{
			void ExitFuncImpl(DynExp::ExitTaskBase::dispatch_tag<NetworkDataStreamInstrumentTasks::ExitTask<BaseInstr, 0, gRPCStubs...>>, DynExp::InstrumentInstance& Instance) override final
			{
				ExitFuncImpl(DynExp::ExitTaskBase::dispatch_tag<ExitTask>(), Instance);
			}

			virtual void ExitFuncImpl(DynExp::ExitTaskBase::dispatch_tag<ExitTask>, DynExp::InstrumentInstance& Instance) {}
		};

		template <typename BaseInstr, typename std::enable_if_t<std::is_base_of_v<DigitalOut, BaseInstr>, int>, typename... gRPCStubs>
		class UpdateTask : public NetworkDataStreamInstrumentTasks::UpdateTask<BaseInstr, 0, gRPCStubs...>
		{
			void UpdateFuncImpl(DynExp::UpdateTaskBase::dispatch_tag<NetworkDataStreamInstrumentTasks::UpdateTask<BaseInstr, 0, gRPCStubs...>>, DynExp::InstrumentInstance& Instance) override final
			{
				UpdateFuncImpl(DynExp::UpdateTaskBase::dispatch_tag<UpdateTask>(), Instance);
			}

			virtual void UpdateFuncImpl(DynExp::UpdateTaskBase::dispatch_tag<UpdateTask>, DynExp::InstrumentInstance& Instance) {}
		};

		template <typename BaseInstr, typename std::enable_if_t<std::is_base_of_v<DigitalOut, BaseInstr>, int>, typename... gRPCStubs>
		class SetTask : public DynExp::TaskBase
		{
		public:
			SetTask(DigitalOutData::SampleStreamType::SampleType Sample, CallbackType CallbackFunc) noexcept : TaskBase(CallbackFunc), Sample(Sample) {}

		private:
			virtual DynExp::TaskResultType RunChild(DynExp::InstrumentInstance& Instance) override
			{
				DynExpProto::NetworkDigitalOut::SampleMessage SampleMsg;
				SampleMsg.set_value(Sample != 0);

				StubPtrType<DynExpProto::NetworkDigitalOut::NetworkDigitalOut> StubPtr;
				{
					auto InstrData = DynExp::dynamic_InstrumentData_cast<NetworkDigitalOutT<BaseInstr, 0, gRPCStubs...>>(Instance.InstrumentDataGetter());
					StubPtr = InstrData->template GetStub<DynExpProto::NetworkDigitalOut::NetworkDigitalOut>();
				} // InstrData unlocked here.

				InvokeStubFunc(StubPtr, &DynExpProto::NetworkDigitalOut::NetworkDigitalOut::Stub::SetAsync, SampleMsg);

				return {};
			}

			const DigitalOutData::SampleStreamType::SampleType Sample;
		};

		template <typename BaseInstr, typename std::enable_if_t<std::is_base_of_v<DigitalOut, BaseInstr>, int>, typename... gRPCStubs>
		class SetDefaultTask : public DynExp::TaskBase
		{
		public:
			SetDefaultTask(CallbackType CallbackFunc) noexcept : TaskBase(CallbackFunc) {}

		private:
			virtual DynExp::TaskResultType RunChild(DynExp::InstrumentInstance& Instance) override
			{
				StubPtrType<DynExpProto::NetworkDigitalOut::NetworkDigitalOut> StubPtr;
				{
					auto InstrData = DynExp::dynamic_InstrumentData_cast<NetworkDigitalOutT<BaseInstr, 0, gRPCStubs...>>(Instance.InstrumentDataGetter());
					StubPtr = InstrData->template GetStub<DynExpProto::NetworkDigitalOut::NetworkDigitalOut>();
				} // InstrData unlocked here.

				InvokeStubFunc(StubPtr, &DynExpProto::NetworkDigitalOut::NetworkDigitalOut::Stub::SetDefault, {});

				return {};
			}
		};
	}

	template <typename BaseInstr, typename std::enable_if_t<std::is_base_of_v<DigitalOut, BaseInstr>, int>, typename... gRPCStubs>
	class NetworkDigitalOutData : public NetworkDataStreamInstrumentData<BaseInstr, 0, gRPCStubs...>
	{
	public:
		using InstrumentType = NetworkDigitalOutT<BaseInstr, 0, gRPCStubs...>;

		NetworkDigitalOutData(size_t BufferSizeInSamples = 1)
			: NetworkDataStreamInstrumentData<BaseInstr, 0, gRPCStubs...>(BufferSizeInSamples) {}
		virtual ~NetworkDigitalOutData() = default;

	private:
		void ResetImpl(DynExp::InstrumentDataBase::dispatch_tag<NetworkDataStreamInstrumentData<BaseInstr, 0, gRPCStubs...>>) override final
		{
			ResetImpl(DynExp::InstrumentDataBase::dispatch_tag<NetworkDigitalOutData>());
		}

		virtual void ResetImpl(DynExp::InstrumentDataBase::dispatch_tag<NetworkDigitalOutData>) {};
	};

	template <typename BaseInstr, typename std::enable_if_t<std::is_base_of_v<DigitalOut, BaseInstr>, int>, typename... gRPCStubs>
	class NetworkDigitalOutParams : public NetworkDataStreamInstrumentParams<BaseInstr, 0, gRPCStubs...>
	{
	public:
		NetworkDigitalOutParams(DynExp::ItemIDType ID, const DynExp::DynExpCore& Core) : NetworkDataStreamInstrumentParams<BaseInstr, 0, gRPCStubs...>(ID, Core) {}
		virtual ~NetworkDigitalOutParams() = default;

		virtual const char* GetParamClassTag() const noexcept override { return "NetworkDigitalOutParams"; }

	private:
		void ConfigureParamsImpl(DynExp::ParamsBase::dispatch_tag<NetworkDataStreamInstrumentParams<BaseInstr, 0, gRPCStubs...>>) override final
		{
			OutputPortParams::DisableUserEditable();
			DigitalOutParams::DisableUserEditable();

			ConfigureParamsImpl(DynExp::ParamsBase::dispatch_tag<NetworkDigitalOutParams>());
		}
		
		virtual void ConfigureParamsImpl(DynExp::ParamsBase::dispatch_tag<NetworkDigitalOutParams>) {}

		DynExp::ParamsBase::DummyParam Dummy = { *this };
	};
	
	template <typename BaseInstr, typename std::enable_if_t<std::is_base_of_v<DigitalOut, BaseInstr>, int>, typename... gRPCStubs>
	class NetworkDigitalOutConfigurator : public NetworkDataStreamInstrumentConfigurator<BaseInstr, 0, gRPCStubs...>
	{
	public:
		using ObjectType = NetworkDigitalOutT<BaseInstr, 0, gRPCStubs...>;
		using ParamsType = NetworkDigitalOutParams<BaseInstr, 0, gRPCStubs...>;

		NetworkDigitalOutConfigurator() = default;
		virtual ~NetworkDigitalOutConfigurator() = default;

	private:
		virtual DynExp::ParamsBasePtrType MakeParams(DynExp::ItemIDType ID, const DynExp::DynExpCore& Core) const override { return DynExp::MakeParams<NetworkDigitalOutConfigurator>(ID, Core); }
	};

	template <typename BaseInstr, typename std::enable_if_t<std::is_base_of_v<DigitalOut, BaseInstr>, int>, typename... gRPCStubs>
	class NetworkDigitalOutT : public NetworkDataStreamInstrumentT<BaseInstr, 0, gRPCStubs...>
	{
	public:
		using ParamsType = NetworkDigitalOutParams<BaseInstr, 0, gRPCStubs...>;
		using ConfigType = NetworkDigitalOutConfigurator<BaseInstr, 0, gRPCStubs...>;
		using InstrumentDataType = NetworkDigitalOutData<BaseInstr, 0, gRPCStubs...>;
		using StubType = DynExpProto::NetworkDigitalOut::NetworkDigitalOut;

		constexpr static auto Name() noexcept { return "Network Digital Out"; }

		NetworkDigitalOutT(const std::thread::id OwnerThreadID, DynExp::ParamsBasePtrType&& Params)
			: NetworkDataStreamInstrumentT<BaseInstr, 0, gRPCStubs...>(OwnerThreadID, std::move(Params)) {}
		virtual ~NetworkDigitalOutT() {}

		virtual std::string GetName() const override { return Name(); }

		// Override in order to disable reading from an output instrument.
		virtual void ReadData(DynExp::TaskBase::CallbackType CallbackFunc = nullptr) const override { DynExp::InstrumentBase::MakeAndEnqueueTask<DynExp::DefaultTask>(CallbackFunc); }

		virtual void Set(DigitalOutData::SampleStreamType::SampleType Sample, DynExp::TaskBase::CallbackType CallbackFunc = nullptr) const override { DynExp::InstrumentBase::MakeAndEnqueueTask<NetworkDigitalOutTasks::SetTask<BaseInstr, 0, gRPCStubs...>>(Sample, CallbackFunc); }
		virtual void SetDefault(DynExp::TaskBase::CallbackType CallbackFunc = nullptr) const override { DynExp::InstrumentBase::MakeAndEnqueueTask<NetworkDigitalOutTasks::SetDefaultTask<BaseInstr, 0, gRPCStubs...>>(CallbackFunc); }
		
		virtual void SetSync(DigitalOutData::SampleStreamType::SampleType Sample) const override
		{
			DynExpProto::NetworkDigitalOut::SampleMessage SampleMsg;
			SampleMsg.set_value(Sample != 0);

			StubPtrType<StubType> StubPtr;
			{
				auto InstrData = dynamic_InstrumentData_cast<NetworkDigitalOutT>(this->GetInstrumentData());
				StubPtr = InstrData->template GetStub<StubType>();
			} // InstrData unlocked here.

			InvokeStubFunc(StubPtr, &StubType::Stub::SetSync, SampleMsg);
		}

	private:
		void ResetImpl(DynExp::Object::dispatch_tag<NetworkDataStreamInstrumentT<BaseInstr, 0, gRPCStubs...>>) override final
		{
			ResetImpl(DynExp::Object::dispatch_tag<NetworkDigitalOutT>());
		}

		virtual void ResetImpl(DynExp::Object::dispatch_tag<NetworkDigitalOutT>) {}

		virtual Util::FeatureTester<FunctionGenerator::WaveformCapsType> GetWaveformCapsChild() const override { return std::array{ FunctionGenerator::WaveformCapsType::UserDefined }; }

		virtual std::unique_ptr<DynExp::InitTaskBase> MakeInitTask() const override { return DynExp::MakeTask<NetworkDigitalOutTasks::InitTask<BaseInstr, 0, gRPCStubs...>>(); }
		virtual std::unique_ptr<DynExp::ExitTaskBase> MakeExitTask() const override { return DynExp::MakeTask<NetworkDigitalOutTasks::ExitTask<BaseInstr, 0, gRPCStubs...>>(); }
		virtual std::unique_ptr<DynExp::UpdateTaskBase> MakeUpdateTask() const override { return DynExp::MakeTask<NetworkDigitalOutTasks::UpdateTask<BaseInstr, 0, gRPCStubs...>>(); }
	};

	/**
	 * @brief Explicit instantiation of derivable class NetworkDigitalOutT to create the network digital out instrument.
	*/
	using NetworkDigitalOut = NetworkDigitalOutT<DigitalOut, 0, typename NetworkDataStreamInstrument::StubType, DynExpProto::NetworkDigitalOut::NetworkDigitalOut>;
}