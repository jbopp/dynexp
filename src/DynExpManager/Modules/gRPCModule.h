// This file is part of DynExp.

/**
 * @file gRPCModule.h
 * @brief Defines a module template for building gRPC servers for network instruments to connect to.
*/

#pragma once

#include "stdafx.h"
#include "DynExpCore.h"
#include "../HardwareAdapters/HardwareAdaptergRPC.h"
#include "../MetaInstruments/gRPCInstrument.h"

namespace DynExpModule
{
	template <typename... gRPCServices>
	class gRPCModule;

	/**
	 * @brief Data class for @p gRPCModule
	 * @copydetails gRPCModule
	*/
	template <typename... gRPCServices>
	class gRPCModuleData : public DynExp::ModuleDataBase
	{
	public:
		gRPCModuleData() { Init(); }
		virtual ~gRPCModuleData() = default;

	private:
		/**
		 * @copydoc DynExp::ModuleDataBase::ResetImpl
		*/
		void ResetImpl(dispatch_tag<ModuleDataBase>) override final { Init(); }

		/**
		 * @copydoc DynExp::ModuleDataBase::ResetImpl
		*/
		virtual void ResetImpl(dispatch_tag<gRPCModuleData>) {};

		/**
		 * @brief Called by @p ResetImpl(dispatch_tag<DynExp::ModuleDataBase>) overridden by this
		 * class to initialize the data class instance. Currently, does not do anything.
		*/
		void Init() {}
	};

	/**
	 * @brief Parameter class for @p gRPCModule
	 * @copydetails gRPCModule
	*/
	template <typename... gRPCServices>
	class gRPCModuleParams : public DynExp::ModuleParamsBase
	{
	public:
		/**
		 * @brief Constructs the parameters for a @p gRPCModule instance.
		 * @copydetails DynExp::ParamsBase::ParamsBase
		*/
		gRPCModuleParams(DynExp::ItemIDType ID, const DynExp::DynExpCore& Core)
			: ModuleParamsBase(ID, Core), NetworkParams(*this) {}

		virtual ~gRPCModuleParams() = default;

		/**
		 * @copydoc DynExp::ParamsBase::GetParamClassTag
		*/
		virtual const char* GetParamClassTag() const noexcept override { return "gRPCModuleParams"; }

		DynExp::NetworkParamsExtension NetworkParams;	//!< Network address the gRPC server listens on

	private:
		/**
		 * @copydoc DynExp::ParamsBase::ConfigureParamsImpl
		*/
		void ConfigureParamsImpl(dispatch_tag<ModuleParamsBase>) override final { ConfigureParamsImpl(dispatch_tag<gRPCModuleParams>()); }

		/**
		 * @copydoc DynExp::ParamsBase::ConfigureParamsImpl
		*/
		virtual void ConfigureParamsImpl(dispatch_tag<gRPCModuleParams>) {}

		/**
		 * @copydoc DynExp::ParamsBase::GetNetworkAddressParams
		*/
		virtual const DynExp::NetworkParamsExtension* GetNetworkAddressParamsChild() const noexcept override { return &NetworkParams; }
	};

	/**
	 * @brief Configurator class for @p gRPCModule
	 * @copydetails gRPCModule
	*/
	template <typename... gRPCServices>
	class gRPCModuleConfigurator : public DynExp::ModuleConfiguratorBase
	{
	public:
		using ObjectType = gRPCModule<gRPCServices...>;
		using ParamsType = gRPCModuleParams<gRPCServices...>;

		gRPCModuleConfigurator() = default;
		virtual ~gRPCModuleConfigurator() = default;
	};

	/**
	 * @brief Module template for building gRPC servers listening on TCP sockets for network instruments
	 * to connect to. Network instruments derive from derived from DynExpInstr::gRPCInstrument. Derive
	 * from this class to build a gRPC server which controls a physical instrument based on gRPC remote
	 * procedure calls as e.g. issued by corresponding network instruments.
	 * @tparam ...gRPCServices List of gRPC service types this gRPC server implements. The order of
	 * service types should match the order of stub types in the @p gRPCStubs list of the respective
	 * DynExpInstr::gRPCInstrument network instrument.
	*/
	template <typename... gRPCServices>
	class gRPCModule : public DynExp::ModuleBase
	{
		/**
		 * @brief Alias for a pointer to a gRPC service
		 * @tparam gRPCService gRPC service type the pointer points to
		*/
		template <typename gRPCService>
		using ServicePtrType = std::unique_ptr<typename gRPCService::AsyncService>;

	public:
		using ParamsType = gRPCModuleParams<gRPCServices...>;								//!< @copydoc DynExp::Object::ParamsType
		using ConfigType = gRPCModuleConfigurator<gRPCServices...>;							//!< @copydoc DynExp::Object::ConfigType
		using ModuleDataType = gRPCModuleData<gRPCServices...>;								//!< @copydoc DynExp::ModuleBase::ModuleDataType

		constexpr static auto Name() noexcept { return "gRPC Module"; }						//!< @copydoc DynExp::SerialCommunicationHardwareAdapter::Name
		constexpr static auto Category() noexcept { return "Network Modules (Servers)"; }	//!< @copydoc DynExp::ModuleBase::Category

		/**
		 * @copydoc DynExp::ModuleBase::ModuleBase
		*/
		gRPCModule(const std::thread::id OwnerThreadID, DynExp::ParamsBasePtrType&& Params)
			: ModuleBase(OwnerThreadID, std::move(Params)),
			ServicePtrs(MakeServicePtrTuple()), ServerRunning(false) {}

		virtual ~gRPCModule() = default;

		virtual std::string GetName() const override { return Name(); }						//!< @copydoc DynExp::ModuleBase::GetName
		virtual std::string GetCategory() const override { return Category(); }				//!< @copydoc DynExp::ModuleBase::GetCategory

		/**
		 * @copydoc DynExp::ModuleBase::TreatModuleExceptionsAsWarnings
		 * @brief If an error occurs during server initialization, terminate the module.
		*/
		bool TreatModuleExceptionsAsWarnings() const override { return ServerRunning; }

		/**
		 * @copydoc DynExp::ModuleBase::GetMainLoopDelay
		 * @brief Be fast answering requests. Return to main loop regularly to handle
		 * e.g. a request that the module should terminate.
		*/
		std::chrono::milliseconds GetMainLoopDelay() const override final { return std::chrono::milliseconds(1); }

		/**
		 * @brief Getter for the gRPC server's request queue
		 * @return Returns the raw pointer contained in #ServerQueue.
		*/
		grpc::ServerCompletionQueue* GetServerQueue() const noexcept { return ServerQueue.get(); }

		/**
		 * @brief Returns a reference to a service this gRPC server implements selected by the service
		 * index in the @p gRPCServices list.
		 * @tparam Index Index of the service pointer to return
		 * @return Element at position @p Index from #ServicePtrs
		*/
		template <size_t Index>
		auto& GetService() const noexcept { return *std::get<Index>(ServicePtrs); }

		/**
		 * @brief Returns a reference to a service this gRPC server implements selected by the service
		 * type @p T.
		 * @tparam T Type of the service to return
		 * @return Service reference of the dereferenced type of @p ServicePtrType pointing to the
		 * service type @p T (as contained in #ServicePtrs)
		*/
		template <typename T>
		auto& GetService() const noexcept { return *std::get<ServicePtrType<T>>(ServicePtrs); }

	protected:
		/**
		 * @brief Base class for all @p TypedCallDataBase classes. Instances of this class manage
		 * the state of a single remote procedure call.
		*/
		class CallDataBase
		{
			/**
			 * @brief Type defining the possible states of remote procedure calls as used by
			 * CallDataBase::StateMachine
			*/
			enum class StateType {
				Init,		//!< The remote procedure call's waiting state directly after construction
				Process,	//!< The remote procedure call's state when it was invoked by a client
				Exit		//!< The remote procedure call's state after it has been handled by the server
			};

			/**
			 * @brief Alias for the state machine state type managed by CallDataBase::StateMachine
			*/
			using StateMachineStateType = Util::StateMachineState<StateType(CallDataBase::*)(DynExp::ModuleInstance&)>;

		protected:
			/**
			 * @brief Constructs a @p CallDataBase instance.
			 * @param OwningModule @copybrief #OwningModule
			*/
			CallDataBase(const gRPCModule* const OwningModule) noexcept
				: OwningModule(OwningModule), StateMachine(InitState, ProcessState, ExitState) {}

		public:
			virtual ~CallDataBase() {}

			/**
			 * @brief Getter for the gRPC server this remote procedure call belongs to
			 * @return Returns #OwningModule.
			*/
			auto GetOwningModule() const noexcept { return OwningModule; }

			/**
			 * @brief Calls the state function of the current state of CallDataBase::StateMachine by a call
			 * to Util::StateMachine::Invoke(). This function is called by gRPCModule::ModuleMainLoop() after
			 * the state of the remote procedure call as contained in gRPCModule::ServerQueue has changed.
			 * @param Instance Handle to the server module thread's data
			*/
			void Proceed(DynExp::ModuleInstance& Instance) { StateMachine.Invoke(*this, Instance); }

		protected:
			/**
			 * @brief Getter for the gRPC server context
			 * @return Returns #ServerContext.
			*/
			auto* GetServerContext() noexcept { return &ServerContext; }

		private:
			/** @name Override
			 * Overridden by @p TypedCallDataBase.
			*/
			///@{
			/**
			 * @brief Tells gRPC that this @p CallDataBase instance is ready to handle a
			 * respective (as determined by @p TypedCallDataBase) remote procedure call.
			 * @param Instance Handle to the server module thread's data
			*/
			virtual void InitChild(DynExp::ModuleInstance& Instance) = 0;

			/**
			 * @brief Creates a new @p TypedCallDataBase instance of the same type to
			 * handle a further remote procedure call, handles this call by invoking
			 * TypedCallDataBase::ProcessChildImpl(), and sends the server's response
			 * back to the client.
			 * @param Instance Handle to the server module thread's data
			*/
			virtual void ProcessChild(DynExp::ModuleInstance& Instance) = 0;
			///@}

			/**
			 * @brief State function for the CallDataBase::InitState state. Calls @p InitChild().
			 * @param Instance Handle to the server module thread's data
			 * @return Returns the new state to transition to (StateType::Process).
			*/
			StateType InitStateFunc(DynExp::ModuleInstance& Instance)
			{
				InitChild(Instance);

				return StateType::Process;
			}

			/**
			 * @brief State function for the CallDataBase::ProcessState state. Calls @p ProcessChild().
			 * @param Instance Handle to the server module thread's data
			 * @return Returns the new state to transition to (StateType::Exit).
			*/
			StateType ProcessStateFunc(DynExp::ModuleInstance& Instance)
			{
				ProcessChild(Instance);

				return StateType::Exit;
			}

			/**
			 * @brief State function for the CallDataBase::ExitState state. Deletes this instance.
			 * @param Instance Handle to the server module thread's data
			 * @return Returns the new state to transition to (StateType::Exit).
			 * This has no meaning since the instance is deleted after this function.
			*/
			StateType ExitStateFunc(DynExp::ModuleInstance& Instance)
			{
				delete this;

				return StateType::Exit;
			}

			/**
			 * @brief State machine state for the StateType::Init state
			*/
			static constexpr auto InitState = Util::StateMachineState(StateType::Init, &CallDataBase::InitStateFunc);

			/**
			 * @brief State machine state for the StateType::Process state
			*/
			static constexpr auto ProcessState = Util::StateMachineState(StateType::Process, &CallDataBase::ProcessStateFunc);

			/**
			 * @brief State machine state for the StateType::Exit state
			*/
			static constexpr auto ExitState = Util::StateMachineState(StateType::Exit, &CallDataBase::ExitStateFunc, "", true);

			const gRPCModule* const OwningModule;						//!< gRPC server this remote procedure call belongs to
			Util::StateMachine<StateMachineStateType> StateMachine;		//!< State machine based on the states listed in @p StateType to manage this remote procedure call's state
			grpc::ServerContext ServerContext;							//!< Information about the remote procedure call. Refer to gRPC documentation.
		};

		/**
		 * @brief Derive from this class to implement a single remote procedure call handled by
		 * this gRPC server @p gRPCModule.
		 * @tparam DerivedType Type of the derived class (Curiously Recurring Template Pattern)
		 * @tparam gRPCService Type of a service implemented by this gRPC server @p gRPCModule for
		 * which this class implements a remote procedure call
		 * @tparam RequestMessageType Type of the gRPC message the client sends to this server
		 * @tparam ResponseMessageType Type of the gRPC message this server sends back to the client
		 * @tparam enable_if_t Ensures that @p gRPCService is contained in the @p gRPCServices
		 * template parameter of the class @p gRPCModule this @p TypedCallDataBase class belongs to.
		*/
		template <typename DerivedType, typename gRPCService, typename RequestMessageType, typename ResponseMessageType,
			typename std::enable_if_t<Util::is_contained_in_v<gRPCService, gRPCServices...>, int> = 0>
		class TypedCallDataBase : public CallDataBase
		{
		public:
			/**
			 * @brief Creates a new remote procedure call of this type which awaits requests from
			 * the client.
			 * @param OwningModule @copybrief DynExpModule::gRPCModule::CallDataBase::OwningModule
			 * @param Instance Handle to the server module thread's data
			*/
			static void MakeCall(const gRPCModule* const OwningModule, DynExp::ModuleInstance& Instance) { (new DerivedType(OwningModule))->Proceed(Instance); }

		private:
			friend DerivedType;

			/**
			 * @brief Alias for the gRPC response writer which sends a message of type
			 * @p ResponseMessageType back to the client after the remote procedure call
			 * is handled.
			*/
			using ResponseWriterType = grpc::ServerAsyncResponseWriter<ResponseMessageType>;

			/**
			 * @brief Alias for the remote procedure call function implemented by this class
			 * as part of an asynchronous gRPC service. Refer to gRPC documentation.
			*/
			using RequestFuncType = std::function<void(typename gRPCService::AsyncService*, grpc::ServerContext*,
				RequestMessageType*, ResponseWriterType*, grpc::CompletionQueue*, grpc::ServerCompletionQueue*, void*)>;

			/**
			 * @brief Constructs a @p TypedCallDataBase instance.
			 * @param OwningModule @copybrief DynExpModule::gRPCModule::CallDataBase::OwningModule
			 * @param RequestFunc Remote procedure call function implemented by this class.
			 * This function is usually part of the < @p ServiceType > :: @p AsyncService gRPC namespace.
			*/
			TypedCallDataBase(const gRPCModule* const OwningModule, const RequestFuncType RequestFunc) noexcept
				: CallDataBase(OwningModule), RequestFunc(RequestFunc), ResponseWriter(this->GetServerContext()) {}
			
			virtual ~TypedCallDataBase() = default;

			/**
			 * @copydoc DynExpModule::gRPCModule::CallDataBase::InitChild
			*/
			void InitChild(DynExp::ModuleInstance& Instance) override final
			{
				// The address of *this* instance serves as the tag to distinguish multiple remote procedure calls.
				RequestFunc(&this->GetOwningModule()->template GetService<gRPCService>(), this->GetServerContext(),
					&RequestMessage, &ResponseWriter, this->GetOwningModule()->GetServerQueue(), this->GetOwningModule()->GetServerQueue(), this);
			}

			/**
			 * @copydoc DynExpModule::gRPCModule::CallDataBase::ProcessChild
			*/
			void ProcessChild(DynExp::ModuleInstance& Instance) override final
			{
				MakeCall(this->GetOwningModule(), Instance);
				
				ProcessChildImpl(Instance);

				ResponseWriter.Finish(ResponseMessage, grpc::Status::OK, this);
			}

			/** @name Override
			 * Override by derived classes.
			*/
			///@{
			/**
			 * @brief Override to implement the server's action to handle this remote procedure
			 * call. Particularly, populate gRPCModule::ResponseMessage with the server's responses.
			 * @param Instance Handle to the server module thread's data
			*/
			virtual void ProcessChildImpl(DynExp::ModuleInstance& Instance) = 0;
			///@}

			const RequestFuncType RequestFunc;		//!< Request function to register the remote procedure call derived from @p TypedCallDataBase with gRPC
			RequestMessageType RequestMessage;		//!< Client's message sent along with its invocation of this remote procedure call
			ResponseMessageType ResponseMessage;	//!< Response the server sends back to the client by finishing the remote procedure call
			ResponseWriterType ResponseWriter;		//!< gRPC response writer to send gRPCModule::ResponseMessage back to the client. Refer to gRPC documentation.
		};

	private:
		/**
		 * @brief Constructs the gRPC services (@p gRPCServices) this gRPC server implements and packs
		 * them as a tuple.
		 * @return Returns the tuple of pointers to newly constructed gRPC services.
		*/
		auto MakeServicePtrTuple() { return std::make_tuple(std::make_unique<typename ServicePtrType<gRPCServices>::element_type>()...); }

		/**
		 * @copydoc DynExp::ModuleBase::ModuleMainLoop
		*/
		Util::DynExpErrorCodes::DynExpErrorCodes ModuleMainLoop(DynExp::ModuleInstance& Instance) override final
		{
			void* Tag;
			bool IsOK;

			auto Result = ServerQueue->AsyncNext(&Tag, &IsOK, std::chrono::system_clock::now() + std::chrono::milliseconds(80));

			if (Result == grpc::CompletionQueue::NextStatus::GOT_EVENT && Tag && IsOK)
				static_cast<CallDataBase*>(Tag)->Proceed(Instance);

			return Util::DynExpErrorCodes::NoError;
		}

		/**
		 * @copydoc DynExp::Object::ResetImpl
		*/
		void ResetImpl(dispatch_tag<ModuleBase>) override final
		{
			ServerQueue.reset();
			Server.reset();

			ServicePtrs = MakeServicePtrTuple();
			ServerRunning = false;

			ResetImpl(dispatch_tag<gRPCModule>());
		}

		/**
		 * @copydoc DynExp::Object::ResetImpl
		*/
		virtual void ResetImpl(DynExp::Object::dispatch_tag<gRPCModule>) {}

		/** @name Override
		 * Override by derived classes.
		*/
		///@{
		/**
		 * @brief Override by derived classes to let them call TypedCallDataBase::MakeCall of the
		 * @p TypedCallDataBase types they are providing. This creates initial listeners awaiting
		 * respective remote procedure calls from a client. Using tag dispatch mechanism (refer to
		 * DynExp::ParamsBase::dispatch_tag) to ensure that @p CreateInitialCallDataObjectsImpl()
		 * of every derived class gets called - starting from @p gRPCModule, descending the
		 * inheritance hierarchy. This allows every derived class to call TypedCallDataBase::MakeCall
		 * for their @p TypedCallDataBase types.
		 * @param Instance Handle to the module thread's data
		*/
		virtual void CreateInitialCallDataObjectsImpl(DynExp::Object::dispatch_tag<gRPCModule>, DynExp::ModuleInstance& Instance) const = 0;

		/**
		 * @brief Allows derived classes to lock instruments they are controlling (e.g. with calls to
		 * DynExp::RunnableInstance::LockObject()) and to perform additional initialization steps.
		 * @param Instance Handle to the module thread's data
		*/
		virtual void OnInitChild(DynExp::ModuleInstance* Instance) const {}

		/**
		 * @brief Allows derived classes to unlock instruments they are controlling (e.g. with calls to
		 * DynExp::RunnableInstance::UnlockObject() and to perform additional cleanup steps.
		 * @param Instance Handle to the module thread's data
		*/
		virtual void OnExitChild(DynExp::ModuleInstance* Instance) const {}
		///@}

		/** @name Events
		 * Event functions running in the module thread.
		*/
		///@{
		/**
		 * @copydoc DynExp::ModuleBase::OnInit
		 * @brief Initializes and starts the gRPC server. Then, calls @p CreateInitialCallDataObjectsImpl()
		 * to create listeners awaiting remote procedure calls and @p OnInitChild() to let derived classes
		 * lock the instruments they are controlling.
		 */
		void OnInit(DynExp::ModuleInstance* Instance) const override final
		{
			std::string Address;
			std::string ObjName;

			{
				auto ModuleParams = DynExp::dynamic_Params_cast<gRPCModule>(Instance->ParamsGetter());
				Address = ModuleParams->NetworkParams.MakeAddress();
				ObjName = ModuleParams->ObjectName;
			} // ModuleParams unlocked here.

			grpc::ServerBuilder ServerBuilder;
			ServerBuilder.AddListeningPort(Address, grpc::InsecureServerCredentials());
			std::apply([&ServerBuilder](auto&... ServicePtr) { (ServerBuilder.RegisterService(ServicePtr.get()), ...); }, ServicePtrs);
			ServerQueue = ServerBuilder.AddCompletionQueue();
			Server = ServerBuilder.BuildAndStart();

			CreateInitialCallDataObjectsImpl(DynExp::Object::dispatch_tag<gRPCModule>(), *Instance);
			OnInitChild(Instance);

			ServerRunning = true;
			Util::EventLog().Log("gRPC server \"" + ObjName + "\" (" + GetCategoryAndName() + ") listening on " + Address + ".");
		}

		/**
		 * @copydoc DynExp::ModuleBase::OnExit
		 * @brief Calls @p OnExitChild() before shutting down the gRPC server calling @p Shutdown().
		*/
		void OnExit(DynExp::ModuleInstance* Instance) const override final
		{
			OnExitChild(Instance);
			Shutdown();

			Util::EventLog().Log("gRPC server \"" + GetObjectName() + "\" (" + GetCategoryAndName() + ") shut down.");
		}
		///@}

		/**
		 * @copydoc DynExp::ModuleBase::OnError
		*/
		void OnErrorChild(DynExp::ModuleInstance& Instance) const override final
		{
			Shutdown();
		}

		/**
		 * @brief Shuts down the gRPC server #Server and its request queue #ServerQueue.
		 * Then, it calls @p DrainServerQueue() to remove pending requests.
		*/
		void Shutdown() const
		{
			if (Server)
				Server->Shutdown();
			if (ServerQueue)
				ServerQueue->Shutdown();

			DrainServerQueue();
		}

		/**
		 * @brief Empties #ServerQueue removing every request and deleting associated
		 * @p CallDataBase instances.
		*/
		void DrainServerQueue() const
		{
			if (!ServerQueue)
				return;

			bool Result = true;
			while (Result)
			{
				void* Tag = nullptr;
				bool IsOK;
				Result = ServerQueue->Next(&Tag, &IsOK);

				if (Result && Tag)
					delete static_cast<CallDataBase*>(Tag);
			}
		}

		/**
		 * @brief Queue holding the pending requests made to the gRPC server #Server
		*/
		mutable std::unique_ptr<grpc::ServerCompletionQueue> ServerQueue;

		/**
		 * @brief Pointer to the actual gRPC server
		*/
		mutable std::unique_ptr<grpc::Server> Server;

		/**
		 * @brief Tuple of pointers to all the services this gRPC server implements
		*/
		std::tuple<ServicePtrType<gRPCServices>...> ServicePtrs;

		/**
		 * @brief Indicates whether #Server is running.
		*/
		mutable std::atomic<bool> ServerRunning;
	};
}