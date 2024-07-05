// This file is part of DynExp.

/**
 * @file gRPCInstrument.h
 * @brief Defines a meta instrument template for transforming meta instruments into network
 * instruments, which connect to gRPC servers.
*/

#pragma once

#include "stdafx.h"
#include "DynExpCore.h"
#include "HardwareAdapters/HardwareAdaptergRPC.h"
#include "MetaInstruments/DataStreamInstrument.h"

#include "Common.pb.h"
#include "Common.grpc.pb.h"

namespace DynExpInstr
{
	template <typename BaseInstr, typename std::enable_if_t<std::is_base_of_v<DynExp::InstrumentBase, BaseInstr>, int>, typename... gRPCStubs>
	class gRPCInstrument;

	/**
	 * @brief Tasks for @p gRPCInstrument
	*/
	namespace gRPCInstrumentTasks
	{
		/**
		 * @copydoc DynExp::InitTaskBase
		 * @copydetails gRPCInstrument 
		*/
		template <typename BaseInstr, std::enable_if_t<std::is_base_of_v<DynExp::InstrumentBase, BaseInstr>, int>, typename... gRPCStubs>
		class InitTask : public BaseInstr::InitTaskType
		{
		protected:
			/**
			 * @copydoc DynExp::InitTaskBase::dispatch_tag
			*/
			template <typename Type>
			using dispatch_tag = DynExp::InitTaskBase::dispatch_tag<Type>;

		private:
			/**
			 * @copydoc DynExp::InitTaskBase::InitFuncImpl
			 * @todo Implement secure SSL communication.
			*/
			void InitFuncImpl(dispatch_tag<typename BaseInstr::InitTaskType>, DynExp::InstrumentInstance& Instance) override final
			{
				{
					auto InstrParams = DynExp::dynamic_Params_cast<gRPCInstrument<BaseInstr, 0, gRPCStubs...>>(Instance.ParamsGetter());
					auto InstrData = DynExp::dynamic_InstrumentData_cast<gRPCInstrument<BaseInstr, 0, gRPCStubs...>>(Instance.InstrumentDataGetter());

					// TODO: Offer SSL credentials.
					InstrData->StubPtrs = std::make_tuple(gRPCStubs::NewStub(grpc::CreateChannel(InstrParams->NetworkParams.MakeAddress(), grpc::InsecureChannelCredentials()))...);
				} // InstrParams and InstrData unlocked here.

				InitFuncImpl(dispatch_tag<InitTask>(), Instance);
			}

			/**
			 * @copydoc DynExp::InitTaskBase::InitFuncImpl
			*/
			virtual void InitFuncImpl(dispatch_tag<InitTask<BaseInstr, 0, gRPCStubs...>>, DynExp::InstrumentInstance& Instance) {}
		};

		/**
		 * @copydoc DynExp::ExitTaskBase
		 * @copydetails gRPCInstrument
		*/
		template <typename BaseInstr, std::enable_if_t<std::is_base_of_v<DynExp::InstrumentBase, BaseInstr>, int>, typename... gRPCStubs>
		class ExitTask : public BaseInstr::ExitTaskType
		{
		protected:
			/**
			 * @copydoc DynExp::ExitTaskBase::dispatch_tag
			*/
			template <typename Type>
			using dispatch_tag = DynExp::ExitTaskBase::dispatch_tag<Type>;

		private:
			/**
			 * @copydoc DynExp::ExitTaskBase::ExitFuncImpl
			*/
			void ExitFuncImpl(dispatch_tag<typename BaseInstr::ExitTaskType>, DynExp::InstrumentInstance& Instance) override final
			{
				try
				{
					ExitFuncImpl(dispatch_tag<ExitTask>(), Instance);

					auto InstrData = DynExp::dynamic_InstrumentData_cast<gRPCInstrument<BaseInstr, 0, gRPCStubs...>>(Instance.InstrumentDataGetter());

					InstrData->ResetStubPtrs();
				} // InstrData unlocked here.
				catch (...)
				{
					// Swallow any exception which might arise from instrument shutdown since a failure
					// of this function is not considered a severe error.
				}
			}

			/**
			 * @copydoc DynExp::ExitTaskBase::ExitFuncImpl
			*/
			virtual void ExitFuncImpl(dispatch_tag<ExitTask>, DynExp::InstrumentInstance& Instance) {}
		};

		/**
		 * @copydoc DynExp::UpdateTaskBase
		 * @copydetails gRPCInstrument
		*/
		template <typename BaseInstr, std::enable_if_t<std::is_base_of_v<DynExp::InstrumentBase, BaseInstr>, int>, typename... gRPCStubs>
		class UpdateTask : public BaseInstr::UpdateTaskType
		{
		protected:
			/**
			 * @copydoc DynExp::UpdateTaskBase::dispatch_tag
			*/
			template <typename Type>
			using dispatch_tag = DynExp::UpdateTaskBase::dispatch_tag<Type>;

		private:
			/**
			 * @copydoc DynExp::UpdateTaskBase::UpdateFuncImpl
			*/
			void UpdateFuncImpl(dispatch_tag<typename BaseInstr::UpdateTaskType>, DynExp::InstrumentInstance& Instance) override final
			{
				UpdateFuncImpl(dispatch_tag<UpdateTask>(), Instance);
			}

			/**
			 * @copydoc DynExp::UpdateTaskBase::UpdateFuncImpl
			*/
			virtual void UpdateFuncImpl(dispatch_tag<UpdateTask>, DynExp::InstrumentInstance& Instance) {}
		};
	}

	/**
	 * @brief Alias for a pointer to a gRPC stub
	 * @tparam gRPCStub gRPC stub type the pointer points to
	*/
	template <typename gRPCStub>
	using StubPtrType = std::shared_ptr<typename gRPCStub::Stub>;

	/**
	 * @brief Data class for @p gRPCInstrument
	 * @copydetails gRPCInstrument
	*/
	template <typename BaseInstr, typename std::enable_if_t<std::is_base_of_v<DynExp::InstrumentBase, BaseInstr>, int>, typename... gRPCStubs>
	class gRPCInstrumentData : public BaseInstr::InstrumentDataType
	{
		friend class gRPCInstrumentTasks::InitTask<BaseInstr, 0, gRPCStubs...>;
		friend class gRPCInstrumentTasks::ExitTask<BaseInstr, 0, gRPCStubs...>;

	public:
		/**
		 * @brief Constructs a @p gRPCInstrumentData instance and forwards all arguments passed
		 * to the constructor to the constructor of the instrument data type of @p BaseInstr.
		 * @tparam ...ArgTs Types of the arguments passed to the constructor
		 * @param ...Args Arguments to forward
		*/
		template <typename... ArgTs>
		gRPCInstrumentData(ArgTs&& ...Args) : BaseInstr::InstrumentDataType(std::forward<ArgTs>(Args)...) {}

		virtual ~gRPCInstrumentData() = default;

		/**
		 * @brief Returns a stub pointer this @p gRPCInstrument uses selected by the stub index in
		 * the @p gRPCStubs list of @p gRPCInstrument.
		 * @tparam Index Index of the stub pointer to return
		 * @return Element at position @p Index from #StubPtrs
		*/
		template <size_t Index>
		auto GetStub() const noexcept { return std::get<Index>(StubPtrs); }

		/**
		 * @brief Returns a stub pointer this @p gRPCInstrument uses selected by the stub type @p T.
		 * @tparam T Type of the stub to return
		 * @return Stub pointer of type @p StubPtrType pointing to the stub type @p T (as contained
		 * in #StubPtrs)
		*/
		template <typename T>
		auto GetStub() const noexcept { return std::get<StubPtrType<T>>(StubPtrs); }

	private:
		/**
		 * @brief Sets all pointers contained in #StubPtrs to @p nullptr.
		*/
		void ResetStubPtrs() { std::apply([](auto&... StubPtr) { (StubPtr.reset(), ...); }, StubPtrs); }

		/**
		 * @copydoc DynExp::InstrumentDataBase::ResetImpl
		*/
		void ResetImpl(DynExp::InstrumentDataBase::dispatch_tag<typename BaseInstr::InstrumentDataType>) override final
		{
			ResetStubPtrs();

			ResetImpl(DynExp::InstrumentDataBase::dispatch_tag<gRPCInstrumentData>());
		}

		/**
		 * @copydoc DynExp::InstrumentDataBase::ResetImpl
		*/
		virtual void ResetImpl(DynExp::InstrumentDataBase::dispatch_tag<gRPCInstrumentData>) {};

		/**
		 * @brief Tuple of pointers to all the stubs this @p gRPCInstrument uses
		*/
		std::tuple<StubPtrType<gRPCStubs>...> StubPtrs;
	};

	/**
	 * @brief Parameter class for @p gRPCInstrument
	 * @copydetails gRPCInstrument
	*/
	template <typename BaseInstr, typename std::enable_if_t<std::is_base_of_v<DynExp::InstrumentBase, BaseInstr>, int>, typename... gRPCStubs>
	class gRPCInstrumentParams : public BaseInstr::ParamsType
	{
	public:
		/**
		 * @brief Constructs the parameters for a @p gRPCInstrument instance.
		 * @copydetails DynExp::ParamsBase::ParamsBase
		*/
		gRPCInstrumentParams(DynExp::ItemIDType ID, const DynExp::DynExpCore& Core)
			: BaseInstr::ParamsType(ID, Core), NetworkParams(*this) {}

		virtual ~gRPCInstrumentParams() = default;

		/**
		 * @copydoc DynExp::ParamsBase::GetParamClassTag
		*/
		virtual const char* GetParamClassTag() const noexcept override { return "gRPCInstrumentParams"; }

		DynExp::NetworkParamsExtension NetworkParams;	//!< Network address of the gRPC server to connect to

	private:
		/**
		 * @copydoc DynExp::ParamsBase::ConfigureParamsImpl
		*/
		void ConfigureParamsImpl(DynExp::InstrumentParamsBase::dispatch_tag<typename BaseInstr::ParamsType>) override final
		{
			ConfigureParamsImpl(DynExp::InstrumentParamsBase::dispatch_tag<gRPCInstrumentParams>());
		}
		
		/**
		 * @copydoc DynExp::ParamsBase::ConfigureParamsImpl
		*/
		virtual void ConfigureParamsImpl(DynExp::InstrumentParamsBase::dispatch_tag<gRPCInstrumentParams>) {}

		/**
		 * @copydoc DynExp::ParamsBase::GetNetworkAddressParams
		*/
		virtual const DynExp::NetworkParamsExtension* GetNetworkAddressParamsChild() const noexcept override { return &NetworkParams; }
	};

	/**
	 * @brief Configurator class for @p gRPCInstrument
	 * @copydetails gRPCInstrument
	*/
	template <typename BaseInstr, typename std::enable_if_t<std::is_base_of_v<DynExp::InstrumentBase, BaseInstr>, int>, typename... gRPCStubs>
	class gRPCInstrumentConfigurator : public BaseInstr::ConfigType
	{
	public:
		using ObjectType = gRPCInstrument<BaseInstr, 0, gRPCStubs...>;
		using ParamsType = gRPCInstrumentParams<BaseInstr, 0, gRPCStubs...>;

		gRPCInstrumentConfigurator() = default;
		virtual ~gRPCInstrumentConfigurator() = default;

	private:
		/**
		 * @copydoc DynExp::ConfiguratorBase::MakeParams
		*/
		virtual DynExp::ParamsBasePtrType MakeParams(DynExp::ItemIDType ID, const DynExp::DynExpCore& Core) const override { return DynExp::MakeParams<gRPCInstrumentConfigurator>(ID, Core); }
	};

	/**
	 * @brief Meta instrument template for transforming meta instruments into network instruments,
	 * which connect to TCP sockets of gRPC servers. Derive from this class to build a network instrument
	 * based on a meta instrument. The network instrument connects to a corresponding server implemented
	 * with the DynExpModule::gRPCModule gRPC server module. The server controls the physical instrument
	 * itself. The respective meta instrument is selected as a template parameter. This class derives
	 * from the instrument. It is possible to use DynExp::InstrumentBase as the meta instrument to design
	 * network instruments, which allow accessing gRPC services directly.
	 * @tparam BaseInstr Meta instrument this class derives from.
	 * @tparam enable_if_t Internal check whether @p BaseInstr is derived from DynExp::InstrumentBase. Pass 0.
	 * @tparam ...gRPCStubs gRPC stub types which this gRPC client expects from the server to be provided.
	 * The order of stub types should match the order of service types in the @p gRPCServices list of the
	 * respective DynExpModule::gRPCModule gRPC server.
	*/
	template <typename BaseInstr, typename std::enable_if_t<std::is_base_of_v<DynExp::InstrumentBase, BaseInstr>, int>, typename... gRPCStubs>
	class gRPCInstrument : public BaseInstr
	{
	public:
		using ParamsType = gRPCInstrumentParams<BaseInstr, 0, gRPCStubs...>;					//!< @copydoc DynExp::Object::ParamsType
		using ConfigType = gRPCInstrumentConfigurator<BaseInstr, 0, gRPCStubs...>;				//!< @copydoc DynExp::Object::ConfigType
		using InstrumentDataType = gRPCInstrumentData<BaseInstr, 0, gRPCStubs...>;				//!< @copydoc DynExp::InstrumentBase::InstrumentDataType

		constexpr static auto Name() noexcept { return "gRPC Instrument"; }						//!< @copydoc DynExp::SerialCommunicationHardwareAdapter::Name
		constexpr static auto Category() noexcept { return "Network Instruments (Clients)"; }	//!< @copydoc DynExp::InstrumentBase::Category

	protected:
		/**
		 * @copydoc DynExp::InstrumentBase::InstrumentBase
		*/
		gRPCInstrument(const std::thread::id OwnerThreadID, DynExp::ParamsBasePtrType&& Params)
			: BaseInstr(OwnerThreadID, std::move(Params)) {}

	public:
		virtual ~gRPCInstrument() {}

		virtual std::string GetName() const override { return Name(); }							//!< @copydoc DynExp::InstrumentBase::GetName
		virtual std::string GetCategory() const override { return Category(); }					//!< @copydoc DynExp::InstrumentBase::GetCategory

	private:
		/**
		 * @copydoc DynExp::Object::ResetImpl
		*/
		void ResetImpl(DynExp::Object::dispatch_tag<BaseInstr>) override final { ResetImpl(DynExp::Object::dispatch_tag<gRPCInstrument>()); }

		/**
		 * @copydoc DynExp::Object::ResetImpl
		*/
		virtual void ResetImpl(DynExp::Object::dispatch_tag<gRPCInstrument>) {}

		/**
		 * @copydoc DynExp::InstrumentBase::MakeInitTask
		*/
		virtual std::unique_ptr<DynExp::InitTaskBase> MakeInitTask() const override { return DynExp::MakeTask<gRPCInstrumentTasks::InitTask<BaseInstr, 0, gRPCStubs...>>(); }

		/**
		 * @copydoc DynExp::InstrumentBase::MakeExitTask
		*/
		virtual std::unique_ptr<DynExp::ExitTaskBase> MakeExitTask() const override { return DynExp::MakeTask<gRPCInstrumentTasks::ExitTask<BaseInstr, 0, gRPCStubs...>>(); }

		/**
		 * @copydoc DynExp::InstrumentBase::MakeUpdateTask
		*/
		virtual std::unique_ptr<DynExp::UpdateTaskBase> MakeUpdateTask() const override { return DynExp::MakeTask<gRPCInstrumentTasks::UpdateTask<BaseInstr, 0, gRPCStubs...>>(); }
	};

	/**
	 * @brief Alias for a pointer to a gRPC stub function, which can be invoked as a remote
	 * procedure call
	 * @tparam gRPCStub gRPC stub type the function is a member of
	 * @tparam RequestMsgType Type of the gRPC message to be sent to the server along with the
	 * remote procedure call
	 * @tparam ResponseMsgType Type of the gRPC message which is received from the server after
	 * the remote procedure call
	*/
	template <typename gRPCStub, typename RequestMsgType, typename ResponseMsgType>
	using StubFuncPtrType = grpc::Status(gRPCStub::*)(grpc::ClientContext*, const RequestMsgType&, ResponseMsgType*);

	/**
	 * @brief Invokes a gRPC stub function as a remote procedure call. Waits for a fixed
	 * amount of time (2 seconds) for a reply from the server until the call is considered
	 * to have exceeded its deadline.
	 * @copydetails StubFuncPtrType
	 * @param StubPtr gRPC stub pointer to invoke the remote procedure call on
	 * @param StubFunc Pointer to the stub function to be invoked on @p StubPtr
	 * @param RequestMsg gRPC message to be sent to the server along with the remote
	 * procedure call
	 * @return Returns the gRPC message which is received from the server after the
	 * remote procedure call.
	 * @throws Util::InvalidStateException is thrown if @p StubPtr is @p nullptr
	 * (likely, the @p gRPCInstrument this stub pointer belongs to has not been
	 * initialized/started yet).
	 * @throws Util::InvalidArgException is thrown if @p StubFunc is @p nullptr.
	 * @throws DynExpHardware::gRPCException is thrown if the remote procedure call has
	 * failed (e.g. the connection has been lost or the server did not reply in time).
	*/
	template <typename gRPCStub, typename RequestMsgType, typename ResponseMsgType>
	inline ResponseMsgType InvokeStubFunc(StubPtrType<gRPCStub> StubPtr, StubFuncPtrType<gRPCStub, RequestMsgType, ResponseMsgType> StubFunc, const RequestMsgType& RequestMsg)
	{
		grpc::ClientContext Context;
		ResponseMsgType ReplyMsg;

		if (!StubPtr)
			throw Util::InvalidStateException("A stub pointer has not been initialized yet.");
		if (!StubFunc)
			throw Util::InvalidArgException("StubFunc must not be nullptr.");

		static const auto Timeout = std::chrono::milliseconds(2000);
		Context.set_deadline(std::chrono::system_clock::now() + Timeout);
		
		auto Result = (*StubPtr.*StubFunc)(&Context, RequestMsg, &ReplyMsg);
		if (!Result.ok())
			throw DynExpHardware::gRPCException(Result);

		return ReplyMsg;
	}
}