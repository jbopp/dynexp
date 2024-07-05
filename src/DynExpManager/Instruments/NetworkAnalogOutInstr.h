// This file is part of DynExp.

/**
 * @file NetworkAnalogOutInstr.h
 * @brief Implementation of a gRPC client instrument to access a remote analog out meta instrument.
*/

#pragma once

#include "stdafx.h"
#include "MetaInstruments/AnalogOut.h"
#include "Instruments/NetworkDataStreamInstrument.h"

#include "NetworkAnalogOut.pb.h"
#include "NetworkAnalogOut.grpc.pb.h"

namespace DynExpInstr
{
	template <typename BaseInstr, typename std::enable_if_t<std::is_base_of_v<AnalogOut, BaseInstr>, int>, typename... gRPCStubs>
	class NetworkAnalogOutT;

	namespace NetworkAnalogOutTasks
	{
		template <typename BaseInstr, typename std::enable_if_t<std::is_base_of_v<AnalogOut, BaseInstr>, int>, typename... gRPCStubs>
		class InitTask : public NetworkDataStreamInstrumentTasks::InitTask<BaseInstr, 0, gRPCStubs...>
		{
			void InitFuncImpl(DynExp::InitTaskBase::dispatch_tag<NetworkDataStreamInstrumentTasks::InitTask<BaseInstr, 0, gRPCStubs...>>, DynExp::InstrumentInstance& Instance) override final
			{
				InitFuncImpl(DynExp::InitTaskBase::dispatch_tag<InitTask>(), Instance);
			}

			virtual void InitFuncImpl(DynExp::InitTaskBase::dispatch_tag<InitTask>, DynExp::InstrumentInstance& Instance) {}

			/**
			 * @brief Disable setting data stream limits in AnalogOutTasks::InitTask::InitFuncImpl() since limits are
			 * managed at the remote site.
			*/
			virtual bool ApplyLimits() const noexcept override { return false; }
		};

		template <typename BaseInstr, typename std::enable_if_t<std::is_base_of_v<AnalogOut, BaseInstr>, int>, typename... gRPCStubs>
		class ExitTask : public NetworkDataStreamInstrumentTasks::ExitTask<BaseInstr, 0, gRPCStubs...>
		{
			void ExitFuncImpl(DynExp::ExitTaskBase::dispatch_tag<NetworkDataStreamInstrumentTasks::ExitTask<BaseInstr, 0, gRPCStubs...>>, DynExp::InstrumentInstance& Instance) override final
			{
				ExitFuncImpl(DynExp::ExitTaskBase::dispatch_tag<ExitTask>(), Instance);
			}

			virtual void ExitFuncImpl(DynExp::ExitTaskBase::dispatch_tag<ExitTask>, DynExp::InstrumentInstance& Instance) {}
		};

		template <typename BaseInstr, typename std::enable_if_t<std::is_base_of_v<AnalogOut, BaseInstr>, int>, typename... gRPCStubs>
		class UpdateTask : public NetworkDataStreamInstrumentTasks::UpdateTask<BaseInstr, 0, gRPCStubs...>
		{
			void UpdateFuncImpl(DynExp::UpdateTaskBase::dispatch_tag<NetworkDataStreamInstrumentTasks::UpdateTask<BaseInstr, 0, gRPCStubs...>>, DynExp::InstrumentInstance& Instance) override final
			{
				UpdateFuncImpl(DynExp::UpdateTaskBase::dispatch_tag<UpdateTask>(), Instance);
			}

			virtual void UpdateFuncImpl(DynExp::UpdateTaskBase::dispatch_tag<UpdateTask>, DynExp::InstrumentInstance& Instance) {}
		};

		template <typename BaseInstr, typename std::enable_if_t<std::is_base_of_v<AnalogOut, BaseInstr>, int>, typename... gRPCStubs>
		class SetTask : public DynExp::TaskBase
		{
		public:
			SetTask(AnalogOutData::SampleStreamType::SampleType Sample, CallbackType CallbackFunc) noexcept : TaskBase(CallbackFunc), Sample(Sample) {}

		private:
			virtual DynExp::TaskResultType RunChild(DynExp::InstrumentInstance& Instance) override
			{
				DynExpProto::NetworkAnalogOut::SampleMessage SampleMsg;
				SampleMsg.set_value(Sample);

				StubPtrType<DynExpProto::NetworkAnalogOut::NetworkAnalogOut> StubPtr;
				{
					auto InstrData = DynExp::dynamic_InstrumentData_cast<NetworkAnalogOutT<BaseInstr, 0, gRPCStubs...>>(Instance.InstrumentDataGetter());
					StubPtr = InstrData->template GetStub<DynExpProto::NetworkAnalogOut::NetworkAnalogOut>();
				} // InstrData unlocked here.

				InvokeStubFunc(StubPtr, &DynExpProto::NetworkAnalogOut::NetworkAnalogOut::Stub::SetAsync, SampleMsg);

				return {};
			}

			const AnalogOutData::SampleStreamType::SampleType Sample;
		};

		template <typename BaseInstr, typename std::enable_if_t<std::is_base_of_v<AnalogOut, BaseInstr>, int>, typename... gRPCStubs>
		class SetDefaultTask : public DynExp::TaskBase
		{
		public:
			SetDefaultTask(CallbackType CallbackFunc) noexcept : TaskBase(CallbackFunc) {}

		private:
			virtual DynExp::TaskResultType RunChild(DynExp::InstrumentInstance& Instance) override
			{
				StubPtrType<DynExpProto::NetworkAnalogOut::NetworkAnalogOut> StubPtr;
				{
					auto InstrData = DynExp::dynamic_InstrumentData_cast<NetworkAnalogOutT<BaseInstr, 0, gRPCStubs...>>(Instance.InstrumentDataGetter());
					StubPtr = InstrData->template GetStub<DynExpProto::NetworkAnalogOut::NetworkAnalogOut>();
				} // InstrData unlocked here.

				InvokeStubFunc(StubPtr, &DynExpProto::NetworkAnalogOut::NetworkAnalogOut::Stub::SetDefault, {});

				return {};
			}
		};
	}

	template <typename BaseInstr, typename std::enable_if_t<std::is_base_of_v<AnalogOut, BaseInstr>, int>, typename... gRPCStubs>
	class NetworkAnalogOutData : public NetworkDataStreamInstrumentData<BaseInstr, 0, gRPCStubs...>
	{
	public:
		using InstrumentType = NetworkAnalogOutT<BaseInstr, 0, gRPCStubs...>;

		NetworkAnalogOutData(size_t BufferSizeInSamples = 1)
			: NetworkDataStreamInstrumentData<BaseInstr, 0, gRPCStubs...>(BufferSizeInSamples) {}
		virtual ~NetworkAnalogOutData() = default;

	private:
		void ResetImpl(DynExp::InstrumentDataBase::dispatch_tag<NetworkDataStreamInstrumentData<BaseInstr, 0, gRPCStubs...>>) override final
		{
			ResetImpl(DynExp::InstrumentDataBase::dispatch_tag<NetworkAnalogOutData>());
		}

		virtual void ResetImpl(DynExp::InstrumentDataBase::dispatch_tag<NetworkAnalogOutData>) {};
	};

	template <typename BaseInstr, typename std::enable_if_t<std::is_base_of_v<AnalogOut, BaseInstr>, int>, typename... gRPCStubs>
	class NetworkAnalogOutParams : public NetworkDataStreamInstrumentParams<BaseInstr, 0, gRPCStubs...>
	{
	public:
		NetworkAnalogOutParams(DynExp::ItemIDType ID, const DynExp::DynExpCore& Core) : NetworkDataStreamInstrumentParams<BaseInstr, 0, gRPCStubs...>(ID, Core) {}
		virtual ~NetworkAnalogOutParams() = default;

		virtual const char* GetParamClassTag() const noexcept override { return "NetworkAnalogOutParams"; }

	private:
		void ConfigureParamsImpl(DynExp::ParamsBase::dispatch_tag<NetworkDataStreamInstrumentParams<BaseInstr, 0, gRPCStubs...>>) override final
		{
			OutputPortParams::DisableUserEditable();
			AnalogOutParams::DisableUserEditable();

			ConfigureParamsImpl(DynExp::ParamsBase::dispatch_tag<NetworkAnalogOutParams>());
		}
		
		virtual void ConfigureParamsImpl(DynExp::ParamsBase::dispatch_tag<NetworkAnalogOutParams>) {}

		DynExp::ParamsBase::DummyParam Dummy = { *this };
	};

	template <typename BaseInstr, typename std::enable_if_t<std::is_base_of_v<AnalogOut, BaseInstr>, int>, typename... gRPCStubs>
	class NetworkAnalogOutConfigurator : public NetworkDataStreamInstrumentConfigurator<BaseInstr, 0, gRPCStubs...>
	{
	public:
		using ObjectType = NetworkAnalogOutT<BaseInstr, 0, gRPCStubs...>;
		using ParamsType = NetworkAnalogOutParams<BaseInstr, 0, gRPCStubs...>;

		NetworkAnalogOutConfigurator() = default;
		virtual ~NetworkAnalogOutConfigurator() = default;

	private:
		virtual DynExp::ParamsBasePtrType MakeParams(DynExp::ItemIDType ID, const DynExp::DynExpCore& Core) const override { return DynExp::MakeParams<NetworkAnalogOutConfigurator>(ID, Core); }
	};

	template <typename BaseInstr, typename std::enable_if_t<std::is_base_of_v<AnalogOut, BaseInstr>, int>, typename... gRPCStubs>
	class NetworkAnalogOutT : public NetworkDataStreamInstrumentT<BaseInstr, 0, gRPCStubs...>
	{
	public:
		using ParamsType = NetworkAnalogOutParams<BaseInstr, 0, gRPCStubs...>;
		using ConfigType = NetworkAnalogOutConfigurator<BaseInstr, 0, gRPCStubs...>;
		using InstrumentDataType = NetworkAnalogOutData<BaseInstr, 0, gRPCStubs...>;
		using StubType = DynExpProto::NetworkAnalogOut::NetworkAnalogOut;

		constexpr static auto Name() noexcept { return "Network Analog Out"; }

		NetworkAnalogOutT(const std::thread::id OwnerThreadID, DynExp::ParamsBasePtrType&& Params)
			: NetworkDataStreamInstrumentT<BaseInstr, 0, gRPCStubs...>(OwnerThreadID, std::move(Params)) {}
		virtual ~NetworkAnalogOutT() {}

		virtual std::string GetName() const override { return Name(); }

		virtual DataStreamInstrumentData::ValueType GetHardwareMinValue() const override
		{
			StubPtrType<StubType> StubPtr;
			{
				auto InstrData = dynamic_InstrumentData_cast<NetworkAnalogOutT>(this->GetInstrumentData());
				StubPtr = InstrData->template GetStub<StubType>();
			} // InstrData unlocked here.

			auto Response = InvokeStubFunc(StubPtr, &StubType::Stub::GetHardwareLimits, {});

			return Response.lowerlimit();
		}

		virtual DataStreamInstrumentData::ValueType GetHardwareMaxValue() const override
		{
			StubPtrType<StubType> StubPtr;
			{
				auto InstrData = dynamic_InstrumentData_cast<NetworkAnalogOutT>(this->GetInstrumentData());
				StubPtr = InstrData->template GetStub<StubType>();
			} // InstrData unlocked here.

			auto Response = InvokeStubFunc(StubPtr, &StubType::Stub::GetHardwareLimits, {});

			return Response.upperlimit();
		}

		virtual DataStreamInstrumentData::ValueType GetHardwareResolution() const override final
		{
			StubPtrType<StubType> StubPtr;
			{
				auto InstrData = dynamic_InstrumentData_cast<NetworkAnalogOutT>(this->GetInstrumentData());
				StubPtr = InstrData->template GetStub<StubType>();
			} // InstrData unlocked here.

			auto Response = InvokeStubFunc(StubPtr, &StubType::Stub::GetHardwareResolution, {});

			return Response.resolution();
		}

		virtual DataStreamInstrumentData::UnitType GetValueUnit() const override
		{
			StubPtrType<StubType> StubPtr;
			{
				auto InstrData = dynamic_InstrumentData_cast<NetworkAnalogOutT>(this->GetInstrumentData());
				StubPtr = InstrData->template GetStub<StubType>();
			} // InstrData unlocked here.

			auto Response = InvokeStubFunc(StubPtr, &StubType::Stub::GetValueUnit, {});

			return ToDataStreamInstrumentUnitType(Response.unit());
		}

		virtual DataStreamInstrumentData::ValueType GetUserMinValue() const override
		{
			StubPtrType<StubType> StubPtr;
			{
				auto InstrData = dynamic_InstrumentData_cast<NetworkAnalogOutT>(this->GetInstrumentData());
				StubPtr = InstrData->template GetStub<StubType>();
			} // InstrData unlocked here.

			auto Response = InvokeStubFunc(StubPtr, &StubType::Stub::GetUserLimits, {});

			return Response.lowerlimit();
		}

		virtual DataStreamInstrumentData::ValueType GetUserMaxValue() const override
		{
			StubPtrType<StubType> StubPtr;
			{
				auto InstrData = dynamic_InstrumentData_cast<NetworkAnalogOutT>(this->GetInstrumentData());
				StubPtr = InstrData->template GetStub<StubType>();
			} // InstrData unlocked here.

			auto Response = InvokeStubFunc(StubPtr, &StubType::Stub::GetUserLimits, {});

			return Response.upperlimit();
		}

		// Override in order to disable reading from an output instrument.
		virtual void ReadData(DynExp::TaskBase::CallbackType CallbackFunc = nullptr) const override { DynExp::InstrumentBase::MakeAndEnqueueTask<DynExp::DefaultTask>(CallbackFunc); }

		virtual void Set(AnalogOutData::SampleStreamType::SampleType Sample, DynExp::TaskBase::CallbackType CallbackFunc = nullptr) const override { DynExp::InstrumentBase::MakeAndEnqueueTask<NetworkAnalogOutTasks::SetTask<BaseInstr, 0, gRPCStubs...>>(Sample, CallbackFunc); }
		virtual void SetDefault(DynExp::TaskBase::CallbackType CallbackFunc = nullptr) const override { DynExp::InstrumentBase::MakeAndEnqueueTask<NetworkAnalogOutTasks::SetDefaultTask<BaseInstr, 0, gRPCStubs...>>(CallbackFunc); }

		virtual void SetSync(AnalogOutData::SampleStreamType::SampleType Sample) const override
		{
			DynExpProto::NetworkAnalogOut::SampleMessage SampleMsg;
			SampleMsg.set_value(Sample);

			StubPtrType<StubType> StubPtr;
			{
				auto InstrData = dynamic_InstrumentData_cast<NetworkAnalogOutT>(this->GetInstrumentData());
				StubPtr = InstrData->template GetStub<StubType>();
			} // InstrData unlocked here.

			InvokeStubFunc(StubPtr, &StubType::Stub::SetSync, SampleMsg);
		}

	private:
		void ResetImpl(DynExp::Object::dispatch_tag<NetworkDataStreamInstrumentT<BaseInstr, 0, gRPCStubs...>>) override final
		{
			ResetImpl(DynExp::Object::dispatch_tag<NetworkAnalogOutT>());
		}

		virtual void ResetImpl(DynExp::Object::dispatch_tag<NetworkAnalogOutT>) {}

		virtual Util::FeatureTester<FunctionGenerator::WaveformCapsType> GetWaveformCapsChild() const override { return std::array{ FunctionGenerator::WaveformCapsType::UserDefined }; }

		virtual std::unique_ptr<DynExp::InitTaskBase> MakeInitTask() const override { return DynExp::MakeTask<NetworkAnalogOutTasks::InitTask<BaseInstr, 0, gRPCStubs...>>(); }
		virtual std::unique_ptr<DynExp::ExitTaskBase> MakeExitTask() const override { return DynExp::MakeTask<NetworkAnalogOutTasks::ExitTask<BaseInstr, 0, gRPCStubs...>>(); }
		virtual std::unique_ptr<DynExp::UpdateTaskBase> MakeUpdateTask() const override { return DynExp::MakeTask<NetworkAnalogOutTasks::UpdateTask<BaseInstr, 0, gRPCStubs...>>(); }
	};

	/**
	 * @brief Explicit instantiation of derivable class NetworkAnalogOutT to create the network analog out instrument.
	*/
	using NetworkAnalogOut = NetworkAnalogOutT<AnalogOut, 0, typename NetworkDataStreamInstrument::StubType, DynExpProto::NetworkAnalogOut::NetworkAnalogOut>;
}