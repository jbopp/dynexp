// This file is part of DynExp.

/**
 * @file Module.h
 * @brief Implementation of %DynExp module objects.
*/

#pragma once

#include "stdafx.h"
#include "Object.h"

namespace DynExp
{
	class ModuleBase;
	class ModuleInstance;
	class EventListenersBase;
	class QModuleBase;

	/**
	 * @brief Pointer type to store a module (DynExp::ModuleBase) with
	*/
	using ModulePtrType = std::unique_ptr<ModuleBase>;

	/**
	 * @brief Factory function to generate a configurator for a specific module type
	 * @tparam ModuleT Type of the module to generate a configurator for
	 * @return Pointer to the configurator for the specified module type
	*/
	template <typename ModuleT>
	ConfiguratorBasePtrType MakeModuleConfig()
	{
		return std::make_shared<typename ModuleT::ConfigType>();
	}

	/**
	 * @brief Factory function to generate a module of a specific type
	 * @tparam ModuleT Type of the module to generate
	 * @param OwnerThreadID ID of the thread owning the module
	 * @param Params Reference to the module's parameters to take ownership of
	 * @return Pointer to the module of the specified type
	*/
	template <typename ModuleT>
	ModulePtrType MakeModule(const std::thread::id OwnerThreadID, ParamsBasePtrType&& Params)
	{
		dynamic_Params_cast<ModuleT>(Params.get())->ModuleData = std::make_unique<typename ModuleT::ModuleDataType>();

		return std::make_unique<ModuleT>(OwnerThreadID, std::move(Params));
	}

	/**
	 * @brief Modules run in their own thread. This is the module thread's main
	 * function.
	 * @param Instance Handle to the module thread's data related to the module
	 * running this thread. The module thread is expected to let the lifetime of
	 * @p Instance expire upon termination.
	 * @param Module Pointer to the module running this thread
	 * @return Util::DynExpErrorCodes::NoError if the thread terminated without an error,
	 * the respective error code otherwise.
	*/
	int ModuleThreadMain(ModuleInstance Instance, ModuleBase* const Module);

	/**
	 * @brief Common base class for all events to store them in a FIFO queue to be invoked later.
	*/
	class EventBase
	{
	public:
		EventBase() = default;
		virtual ~EventBase() = 0;

		/**
		 * @brief Invokes the event passing the receiving module's instance reference to it.
		 * @param Instance Module instance handle.
		*/
		void Invoke(ModuleInstance& Instance) const { InvokeChild(Instance); }

	private:
		/** @name Override
		 * Override by derived class to make public versions of these functions behave as described above.
		*/
		///@{
		virtual void InvokeChild(ModuleInstance& Instance) const = 0;		//!< @copydoc Invoke()
		///@}
	};

	/**
	 * @brief Describes an event which consists of a receiver's member function and a set of arguments to call this function with.
	 * The first argument is expected to be of type @p ModuleInstance*. This argument is not included in @p ArgTupleType).
	 * @tparam ReceiverType Type of the receiving module.
	 * @tparam ArgTupleType Type of a tuple of the arguments' types the receiver's member function expects.
	*/
	template <typename ReceiverType, typename ArgTupleType>
	class DefaultEvent : public EventBase
	{
	private:
		/**
		 * @brief Helper struct to allow accessing elements within @p ArgTupleType.
		 * @tparam ...Indices Index sequence [0, number of elements within @p ArgTupleType).
		*/
		template <size_t... Indices>
		struct EventFuncTraits
		{
			constexpr EventFuncTraits() = default;

			/**
			 * @brief Signature of an event function.
			 * Pointer to @p ModuleInstance to be passed as parameter instead of reference to render function
			 * pointers storable in Util::CallableMemberWrapper (with nullptr as default argument)
			*/
			using EventFuncPtrType = void (ReceiverType::*)(ModuleInstance*, std::tuple_element_t<Indices, ArgTupleType>...) const;
			
			/**
			 * @brief Type of a Util::CallableMemberWrapper to store the event.
			*/
			using EventFuncType = Util::CallableMemberWrapper<const ReceiverType, EventFuncPtrType>;
		};

		/**
		 * @brief Instantiates @p EventFuncTraits. This function is not meant to be called. It is used for type deduction only.
		 * @tparam ...Indices Index sequence for the (pseudo-)instantiation of @p EventFuncTraits.
		 * @return Returns an instance of @p EventFuncTraits.
		*/
		template <size_t... Indices>
		constexpr static auto MakeEventFuncTraits(std::index_sequence<Indices...>) { return EventFuncTraits<Indices...>(); }

		/**
		 * @brief Index sequence for the (pseudo-)instantiation of @p EventFuncTraits.
		*/
		using ArgTupleIndexSequenceType = decltype(std::make_index_sequence<std::tuple_size_v<ArgTupleType>>());
		
		/**
		 * @brief Type of the instantiated @p EventFuncTraits - not actually instantiating it.
		*/
		using InstantiatedEventFuncTraitsType = decltype(DefaultEvent<ReceiverType, ArgTupleType>::MakeEventFuncTraits(std::declval<ArgTupleIndexSequenceType>()));

	public:
		using EventFuncPtrType = typename InstantiatedEventFuncTraitsType::EventFuncPtrType;	//!< @copydoc EventFuncTraits::EventFuncPtrType
		using EventFuncType = typename InstantiatedEventFuncTraitsType::EventFuncType;			//!< @copydoc EventFuncTraits::EventFuncType

		/**
		 * @brief Constructs a @p DefaultEvent instance.
		 * @param EventFunc @copydoc EventFunc
		*/
		DefaultEvent(EventFuncType EventFunc) noexcept : EventFunc(EventFunc) {}

		virtual ~DefaultEvent() {}

	private:
		virtual void InvokeChild(ModuleInstance& Instance) const override { EventFunc(&Instance); }

		/**
		 * @brief Util::CallableMemberWrapper to store the event receiver, the event function, and its arguments.
		*/
		const EventFuncType EventFunc;
	};

	/**
	 * @brief Data structure to contain data which is synchronized in between different threads.
	 * This is needed since the module thread as well as the main thread might access the module
	 * at the same time. Every class (indirectly) derived from class @p ModuleBase must be
	 * accompanied by a module data class derived from @p ModuleDataBase. @p ModuleDataBase and
	 * derived classes contain data shared by the respective module thread and the user interface
	 * (main) thread to e.g. visualize data. Data only used by the module thread should be private
	 * members of the module class. Data only used by the user interface should be private members
	 * of the module's widget class (derived from @p QModuleWidget), respectively.
	 * @warning For the same @p Object, always lock the mutex of the corresponding parameter class
	 * before the mutex of the corresponding data class (or only one of them).
	*/
	class ModuleDataBase : public Util::ISynchronizedPointerLockable
	{
	public:
		/**
		 * @brief Pointer owning an event
		*/
		using EventPtrType = std::unique_ptr<EventBase>;

		/**
		 * @brief A module's event queue is a FIFO queue owning the enqueued events.
		*/
		using EventQueueType = std::queue<EventPtrType>;

	protected:
		/**
		 * @brief Refer to ParamsBase::dispatch_tag.
		 * @tparam Type (derived from class @p ModuleDataBase) to instantiate the dispatch tag template with.
		*/
		template <typename>
		struct dispatch_tag {};

	private:
		/**
		 * @brief Allow exclusive access to some of @p ModuleDataBase's private methods to @p ModuleBase.
		*/
		class ModuleBaseOnlyType
		{
			friend class ModuleDataBase;
			friend class ModuleBase;

			/**
			 * @brief Construcs an instance - one for each @p ModuleDataBase instance
			 * @param Parent Owning @p ModuleDataBase instance
			*/
			constexpr ModuleBaseOnlyType(ModuleDataBase& Parent) noexcept : Parent(Parent) {}

			void Reset() { Parent.Reset(); }												//!< @copydoc ModuleDataBase::Reset

			auto& GetNewEventNotifier() noexcept { return Parent.GetNewEventNotifier(); }	//!< @copydoc ModuleDataBase::GetNewEventNotifier

			ModuleDataBase& Parent;			//!< Owning @p ModuleDataBase instance
		};

		/**
		 * @brief Allow exclusive access to some of @p ModuleDataBase's private methods to the module thread
		 * @p ModuleThreadMain().
		*/
		class ModuleThreadOnlyType
		{
			friend class ModuleDataBase;
			friend int ModuleThreadMain(ModuleInstance, ModuleBase* const);

			/**
			 * @brief Construcs an instance - one for each @p ModuleDataBase instance
			 * @param Parent Owning @p ModuleDataBase instance
			*/
			constexpr ModuleThreadOnlyType(ModuleDataBase& Parent) noexcept : Parent(Parent) {}

			auto& GetNewEventNotifier() noexcept { return Parent.GetNewEventNotifier(); }									//!< @copydoc ModuleDataBase::GetNewEventNotifier
			void SetException(std::exception_ptr ModuleException) noexcept { Parent.ModuleException = ModuleException; }	//!< Setter for ModuleDataBase::ModuleException

			ModuleDataBase& Parent;			//!< Owning @p ModuleDataBase instance
		};

	public:
		ModuleDataBase() : ModuleBaseOnly(*this), ModuleThreadOnly(*this) {}
		virtual ~ModuleDataBase() {}

		/** @name Module event queue
		 * Methods to access and manipulate the event queue belonging to the module which owns
		 * the respective @p ModuleDataBase's instance
		*/
		///@{
		/**
		 * @brief Enqueues @p Event at the module event queue's back. Takes ownership of the event.
		 * Notifies the module owning the respective @p ModuleDataBase's instance that a new event has
		 * been enqueued.
		 * @param Event Pointer to the event to be enqueued. Must not be nullptr.
		 * @throws Util::InvalidArgException is thrown if @p Event is nullptr.
		*/
		void EnqueueEvent(EventPtrType&& Event);
		
		/**
		 * @brief Removes one event from the event queue's front and returns the event.
		 * Ownership of the event is transferred to the caller of this method.
		 * @return Pointer to the popped event or nullptr if the event queue is empty.
		*/
		EventPtrType PopEvent();
		
		/**
		 * @brief Returns a pointer to the event in the front of the module's event queue
		 * without transferring ownership and without removing the event from the queue.
		 * @return Constant pointer to the event
		*/
		const auto& GetEventFront() const noexcept { return *EventQueue.front().get(); }

		/**
		 * @copybrief ModuleDataBase::GetEventFront() const
		 * @return Pointer to the event
		*/
		auto& GetEventFront() noexcept { return *EventQueue.front().get(); }

		/**
		 * @brief Getter for the module event queue's length
		 * @return Returns the number of currently enqueued events.
		*/
		size_t GetNumEnqueuedEvents() const noexcept { return EventQueue.size(); }
		///@}

		/**
		 * @brief Getter for #ModuleException
		 * @return Returns the exception being responsible for the module's current state.
		*/
		auto GetException() const noexcept { return ModuleException; }

		ModuleBaseOnlyType ModuleBaseOnly;				//!< @copydoc ModuleBaseOnlyType
		ModuleThreadOnlyType ModuleThreadOnly;			//!< @copydoc ModuleThreadOnlyType

	private:
		/**
		 * @brief Getter for #NewEventNotifier
		 * @return Returns #NewEventNotifier to notify the module thread about new events.
		*/
		Util::OneToOneNotifier& GetNewEventNotifier() noexcept { return NewEventNotifier; }
		
		/** @name Stopped module only
		 * Must only be called when the module thread is not running.
		*/
		///@{
		/**
		 * @brief Resets the @p ModuleDataBase's instance and calls
		 * ResetImpl(dispatch_tag<ModuleDataBase>) subsequently.
		*/
		void Reset();

		/**
		 * @brief Refer to DynExp::ModuleDataBase::Reset(). Using tag dispatch mechanism to ensure that @p ResetImpl()
		 * of every derived class gets called - starting from @p ModuleDataBase, descending the inheritance hierarchy.
		 * Override in order to reset derived classes.
		*/
		virtual void ResetImpl(dispatch_tag<ModuleDataBase>) {}
		///@}

		/**
		 * @brief FIFO event queue of the module which owns the respective @p ModuleDataBase's instance
		*/
		EventQueueType EventQueue;

		/**
		 * @brief Notifies the thread of the module which owns the respective @p ModuleDataBase's
		 * instance when an event has been enqueued into the module's event queue. This allows the
		 * module thread to sleep until new events have to be handled.
		*/
		Util::OneToOneNotifier NewEventNotifier;

		/**
		 * @brief Used to transfer exceptions from the module thread to the main (user interface)
		 * thread. Stores the exception responsible for the error state if the module which owns
		 * the respective @p ModuleDataBase's instance is in such an error state.
		*/
		std::exception_ptr ModuleException;
	};

	/**
	 * @brief Parameter class for @p ModuleBase
	*/
	class ModuleParamsBase : public RunnableObjectParams
	{
		friend class ModuleBase;

		template <typename>
		friend ModulePtrType MakeModule(const std::thread::id, ParamsBasePtrType&&);

	public:
		/**
		 * @brief Constructs the parameters for a @p ModuleBase instance.
		 * @copydetails ParamsBase::ParamsBase
		*/
		ModuleParamsBase(ItemIDType ID, const DynExpCore& Core) : RunnableObjectParams(ID, Core) {}

		virtual ~ModuleParamsBase() = 0;

		virtual const char* GetParamClassTag() const noexcept override { return "ModuleParamsBase"; }

	private:
		void ConfigureParamsImpl(dispatch_tag<RunnableObjectParams>) override final { ConfigureParamsImpl(dispatch_tag<ModuleParamsBase>()); }
		virtual void ConfigureParamsImpl(dispatch_tag<ModuleParamsBase>) {}			//!< @copydoc ConfigureParamsImpl(dispatch_tag<DynExp::RunnableObjectParams>)

		// Usage parameters may be used in the future also by modules.
		bool ConfigureUsageTypeChild() const noexcept override final { return false; }

		/**
		 * @brief Just used temporarily during the construction of a module.
		 * Refer to MakeModule() and ModuleBase::ModuleBase().
		*/
		std::unique_ptr<ModuleDataBase> ModuleData;

		DummyParam Dummy = { *this };												//!< @copydoc DynExp::ParamsBase::DummyParam
	};

	/**
	 * @brief Configurator class for @p ModuleBase
	*/
	class ModuleConfiguratorBase : public RunnableObjectConfigurator
	{
	public:
		using ObjectType = ModuleBase;
		using ParamsType = ModuleParamsBase;

		ModuleConfiguratorBase() = default;
		virtual ~ModuleConfiguratorBase() = 0;
	};

	/**
	 * @brief Base class for modules. Modules implement programs on their own
	 * (e.g. measurement protocols or servers for ethernet communication). They might have a
	 * user interface. Modules make use of meta instruments (@p InstrumentBase) to implement
	 * e.g. measurement routines on a higher level, which renders the modules independent
	 * from physical devices from specific manufacturers.
	 * Derive from this class to implement modules without a user interface.
	*/
	class ModuleBase : public RunnableObject
	{
		/**
		 * @brief Allow exclusive access to some of @p ModuleBase's private methods to the module thread
		 * @p ModuleThreadMain().
		*/
		class ModuleThreadOnlyType
		{
			friend class ModuleBase;
			friend int ModuleThreadMain(ModuleInstance, ModuleBase* const);

			/**
			 * @brief Construcs an instance - one for each @p ModuleBase instance
			 * @param Parent Owning @p ModuleBase instance
			*/
			constexpr ModuleThreadOnlyType(ModuleBase& Parent) noexcept : Parent(Parent) {}

			void HandleEvent(ModuleInstance& Instance) { Parent.HandleEvent(Instance); }														//<! @copydoc ModuleBase::HandleEvent
			Util::DynExpErrorCodes::DynExpErrorCodes ModuleMainLoop(ModuleInstance& Instance) { return Parent.ExecModuleMainLoop(Instance); }	//<! @copydoc ModuleBase::ExecModuleMainLoop
			void OnPause(ModuleInstance& Instance) { Parent.OnPause(Instance); }																//<! @copydoc ModuleBase::OnPause
			void OnResume(ModuleInstance& Instance) { Parent.OnResume(Instance); }																//<! @copydoc ModuleBase::OnResume
			void OnError(ModuleInstance& Instance) { Parent.OnError(Instance); }																//<! @copydoc ModuleBase::OnError

			void SetReasonWhyPaused(std::string Description) { Parent.ModuleSetReasonWhyPaused(std::move(Description)); }						//<! @copydoc ModuleBase::ModuleSetReasonWhyPaused

			ModuleBase& Parent;		//!< Owning @p ModuleBase instance
		};

		/**
		 * @brief Allow exclusive access to some of @p ModuleBase's private methods to any
		 * @p TypedEventListeners class.
		*/
		class EventListenersOnlyType
		{
			friend class ModuleBase;

			template <typename...>
			friend class TypedEventListeners;

			/**
			 * @brief Construcs an instance - one for each @p ModuleBase instance
			 * @param Parent Owning @p ModuleBase instance
			*/
			constexpr EventListenersOnlyType(ModuleBase& Parent) noexcept : Parent(Parent) {}

			void AddRegisteredEvent(EventListenersBase& EventListeners) const { Parent.AddRegisteredEvent(EventListeners); }					//!< @copydoc ModuleBase::AddRegisteredEvent
			void RemoveRegisteredEvent(EventListenersBase& EventListeners) const { Parent.RemoveRegisteredEvent(EventListeners); }				//!< @copydoc ModuleBase::RemoveRegisteredEvent

			ModuleBase& Parent;		//!< Owning @p ModuleBase instance
		};

	public:
		using ParamsType = ModuleParamsBase;																//!< @copydoc Object::ParamsType
		using ConfigType = ModuleConfiguratorBase;															//!< @copydoc Object::ConfigType
		
		/**
		 * @brief Type of the data class belonging to this @p ModuleBase type. Declare this alias in every
		 * derived class with the respective data class accompanying the derived @p ModuleBase.
		*/
		using ModuleDataType = ModuleDataBase;

		/**
		 * @brief Alias for the return type of ModuleBase::GetModuleData(). Data class instances
		 * wrapped into Util::SynchronizedPointer can be accessed in a thread-safe way.
		*/
		using ModuleDataTypeSyncPtrType = Util::SynchronizedPointer<ModuleDataType>;

		/**
		 * @brief Alias for the return type of ModuleBase::GetModuleData() const. Data class
		 * instances wrapped into Util::SynchronizedPointer can be accessed in a thread-safe way.
		*/
		using ModuleDataTypeSyncPtrConstType = Util::SynchronizedPointer<const ModuleDataType>;

		/**
		 * @brief Every derived class has to redefine this function.
		 * @return Returns the category of this module type.
		*/
		constexpr static auto Category() noexcept { return ""; }

		/**
		 * @brief Constructs a @p ModuleBase instance.
		 * @copydetails DynExp::Object::Object
		 * @throws Util::InvalidArgException is thrown if ModuleParamsBase::ModuleData of @p Params is nullptr.
		*/
		ModuleBase(const std::thread::id OwnerThreadID, ParamsBasePtrType&& Params);

		virtual ~ModuleBase() = 0;

		virtual std::string GetCategory() const override { return Category(); }

		/** @name Override
		 * Override by derived classes.
		*/
		///@{
		/**
		 * @brief Determines whether this module possesses a user interface (UI) which is shown
		 * in a window dedicated to the module.
		 * @return Return true if the module possesses an UI, false otherwise.
		*/
		virtual bool HasUI() const noexcept { return false; }

		/**
		 * @brief Determines whether this module should be terminated if an exception leaves
		 * the module's main loop or respective event handlers.
		 * @return Return false if the module should be terminated in case of an uncaught
		 * exception, true to only record the exception as the module's warning.
		*/
		virtual bool TreatModuleExceptionsAsWarnings() const { return true; }

		/**
		 * @brief Specifies in which time intervals the module's event queue runs to handle pending events.
		 * @return Delay time in between event queue executions. Return @p std::chrono::milliseconds::max()
		 * to make the module thread only wake up to execute the module main loop (after handling enqueued
		 * events) when an event is enqueued. Return 0 to make the module main loop execute as fast as possible.
		*/
		virtual std::chrono::milliseconds GetMainLoopDelay() const { return std::chrono::milliseconds(10); }
		///@}

		/**
		 * @brief Determines the default timeout for @p GetModuleData() to lock the mutex synchronizing
		 * the module's data #ModuleData.
		*/
		static constexpr auto GetModuleDataTimeoutDefault = std::chrono::milliseconds(1000);

		ModuleThreadOnlyType ModuleThreadOnly;			//!< @copydoc ModuleThreadOnlyType
		EventListenersOnlyType EventListenersOnly;		//!< @copydoc EventListenersOnlyType

		/** @name Thread-safe public functions
		 * Methods can be called from any thread.
		*/
		///@{
		/**
		 * @brief @p RestoreWindowStatesFromParams() only calls @p RestoreWindowStatesFromParamsChild().
		 * Override @p RestoreWindowStatesFromParamsChild() to restore the styles of possibly owned
		 * windows according to the style saved in the module's parameters.
		*/
		void RestoreWindowStatesFromParams() { RestoreWindowStatesFromParamsChild(); }

		/**
		 * @brief @p UpdateParamsFromWindowStates() only calls @p UpdateParamsFromWindowStatesChild().
		 * Override @p UpdateParamsFromWindowStatesChild() to store the styles of possibly owned
		 * windows in the module's parameters.
		*/
		void UpdateParamsFromWindowStates() { UpdateParamsFromWindowStatesChild(); }

		/**
		 * @brief Locks the mutex of the module data class instance #ModuleData assigned to this @p ModuleBase
		 * instance and returns a pointer to the locked #ModuleData. Module data should not be locked by having called
		 * this function while subsequently calling a derived module's method which also makes use of the module's data
		 * by locking it. If this happens, the module data's mutex is locked recursively. In principle, this does no
		 * harm since Util::ISynchronizedPointerLockable supports that. But, it is not considered good practice.
		 * @param Timeout Time to wait for locking the mutex of #ModuleData.
		 * @return Returns a pointer to @p ModuleDataType (non-const) to allow access all of its members.
		*/
		ModuleDataTypeSyncPtrType GetModuleData(const std::chrono::milliseconds Timeout = GetModuleDataTimeoutDefault);

		/**
		 * @copybrief GetModuleData(const std::chrono::milliseconds)
		 * @param Timeout Time to wait for locking the mutex of #ModuleData.
		 * @return Returns a pointer to const @p ModuleDataType, since access to non-const members of @p ModuleDataType
		 * is only allowed to the main thread!
		*/
		ModuleDataTypeSyncPtrConstType GetModuleData(const std::chrono::milliseconds Timeout = GetModuleDataTimeoutDefault) const;

		/**
		 * @copydoc ModuleDataBase::EnqueueEvent
		*/
		void EnqueueEvent(ModuleDataBase::EventPtrType&& Event) const;

		/**
		 * @brief Calls @p MakeEvent() to construct a new event and subsequently enqueues the event into the module's
		 * event queue.
		 * Logical const-ness: this is a const member function to allow pointers to @p const @p ModuleBase inserting
		 * events into the module's event queue. These kind of pointers are e.g. returned by
		 * RunnableInstance::GetOwner() which can be called by events' EventBase::InvokeChild() functions. For
		 * @p const @p ModuleBase*, it is possible to insert events into the event queue, but not to
		 * change the @p ModuleBase object itself (e.g. calling Object::Reset()).
		 * @tparam ReceiverType Module type to receive this event.
		 * @tparam EventType Type (signature) of the event function.
		 * @tparam ...ArgsTs Types of arguments passed to the event function.
		 * @param Receiver Pointer to the module receiving this event. Pointer @p this is not directly used since it is
		 * not a pointer to the derived type. Passing a pointer to the derived type saves a @p dynamic_cast operation.
		 * @param EventFuncPtr Event function to be invoked (member function of @p ReceiverType expected).
		 * @param ...Args Arguments to pass to the event function.
		 * @throws Util::InvalidArgException is thrown if @p Receiver is not a pointer to the @p ModuleBase instance
		 * ModuleBase::MakeAndEnqueueEvent() is called on.
		*/
		template <typename ReceiverType, typename EventType, typename... ArgsTs>
		void MakeAndEnqueueEvent(ReceiverType* Receiver, EventType EventFuncPtr, ArgsTs&& ...Args) const;
		///@}

		/**
		 * @brief Invoking an instance of this alias is supposed to call ModuleBase::GetModuleData() of the
		 * instance the Util::CallableMemberWrapper has been constructed with.
		*/
		using ModuleDataGetterType = Util::CallableMemberWrapper<ModuleBase,
			ModuleDataTypeSyncPtrType (ModuleBase::*)(const std::chrono::milliseconds)>;

	private:
		/** @name Private functions for logical const-ness
		 * Logical const-ness: refer to ModuleBase::MakeAndEnqueueEvent().
		*/
		///@{
		/**
		 * @brief Always allows @p ModuleBase to obtain a non-const pointer to the module's data -
		 * even in const event functions.
		 * @param Timeout Time to wait for locking the mutex of #ModuleData.
		 * @return Returns a pointer to @p ModuleDataType (non-const) to allow access all of its members.
		*/
		ModuleDataTypeSyncPtrType GetNonConstModuleData(const std::chrono::milliseconds Timeout = GetModuleDataTimeoutDefault) const;
		///@}

	protected:
		/**
		 * @brief Getter for ModuleDataBase::ModuleException assuming that the module data
		 * has already been locked.
		 * @param ModuleDataPtr Synchronized pointer to the locked @p ModuleDataBase instance.
		 * @copydetails ModuleDataBase::GetException
		*/
		static auto GetExceptionUnsafe(const ModuleDataTypeSyncPtrConstType& ModuleDataPtr) { return ModuleDataPtr->GetException(); }

	private:
		/** @name Module thread only
		 * These functions must be called by module thread only.
		*/
		///@{
		/**
		 * @brief Executes and removes the next pending event from the module's event queue.
		 * @param Instance Handle to the module thread's data
		*/
		void HandleEvent(ModuleInstance& Instance);

		/**
		 * @brief Adds a manager of event listeners to #RegisteredEvents if it was not already added before.
		 * Called indirectly by TypedEventListeners::Register().
		 * @param EventListeners Manager of event listeners to add
		*/
		void AddRegisteredEvent(EventListenersBase& EventListeners);

		/**
		 * @brief Removes a manager of event listeners from #RegisteredEvents if it was added before.
		 * Called indirectly by TypedEventListeners::Deregister().
		 * @param EventListeners Manager of event listeners to remove
		*/
		void RemoveRegisteredEvent(EventListenersBase& EventListeners);

		/**
		 * @brief Runs @p ModuleMainLoop().
		 * @copydetails ModuleMainLoop
		*/
		Util::DynExpErrorCodes::DynExpErrorCodes ExecModuleMainLoop(ModuleInstance& Instance);

		/**
		 * @brief This handler gets called just after the module pauses due to e.g. an
		 * error in an instrument the module depends on. Override @p OnPauseChild() to
		 * e.g. abort a measurement or to store internal states for resuming later.
		 * @param Instance Handle to the module thread's data
		*/
		void OnPause(ModuleInstance& Instance);

		/**
		 * @brief This handler gets called just after the module resumed from a paused
		 * state. Override @p OnResumeChild() to reinitialize instruments or a measurement
		 * routine.
		 * @param Instance Handle to the module thread's data
		*/
		void OnResume(ModuleInstance& Instance);

		/**
		 * @brief This handler gets called just before the module thread terminates due to
		 * an exception. Override @p OnErrorChild() to perform shutdown and cleanup actions.
		 * @param Instance Handle to the module thread's data
		*/
		void OnError(ModuleInstance& Instance);
		///@}

		/** @name Override
		 * Override by derived classes.
		*/
		///@{
		virtual void OnPauseChild(ModuleInstance& Instance) const {}	//!< @copydoc OnPause
		virtual void OnResumeChild(ModuleInstance& Instance) const {}	//!< @copydoc OnResume
		virtual void OnErrorChild(ModuleInstance& Instance) const {}	//!< @copydoc OnError

		virtual void RestoreWindowStatesFromParamsChild() {}			//!< @copydoc RestoreWindowStatesFromParams
		virtual void UpdateParamsFromWindowStatesChild() {}				//!< @copydoc UpdateParamsFromWindowStates

		/**
		 * @brief Module main loop. The function is executed periodically by the module thread.
		 * Also refer to @p GetMainLoopDelay().
		 * @param Instance Handle to the module thread's data
		 * @return Return Util::DynExpErrorCodes::NoError if module loop should continue, annother error code otherwise.
		*/
		virtual Util::DynExpErrorCodes::DynExpErrorCodes ModuleMainLoop(ModuleInstance& Instance) = 0;
		///@}

		void ResetImpl(dispatch_tag<RunnableObject>) override final;
		virtual void ResetImpl(dispatch_tag<ModuleBase>) = 0;			//!< @copydoc ResetImpl(dispatch_tag<DynExp::RunnableObject>)

		/**
		 * @brief Sets the reason why this @p ModuleBase instance has been paused.
		 * @copydetails RunnableObject::SetReasonWhyPaused
		*/
		void ModuleSetReasonWhyPaused(std::string Description) { SetReasonWhyPaused(std::move(Description)); }

		/** @name Main thread only
		 * These functions must not be called by module thread.
		*/
		///@{
		void RunChild() override final;
		void NotifyChild() override final;
		void TerminateChild(const std::chrono::milliseconds Timeout) override final;
		std::unique_ptr<BusyDialog> MakeStartupBusyDialogChild(QWidget* ParentWidget) const override final;
		///@}

		std::exception_ptr GetExceptionChild([[maybe_unused]] const std::chrono::milliseconds Timeout) const override final;
		bool IsReadyChild() const override final;

		/** @name Events
		 * Event functions running in the module thread.
		*/
		///@{
		/**
		 * @brief This event is triggered right before the module thread starts.
		 * Override it to lock instruments this module depends on (via RunnableInstance::LockObject())
		 * and to register/subscribe for events (via InterModuleEvent::Register()).
		 * @param Instance Handle to the module thread's data
		*/
		virtual void OnInit(ModuleInstance* Instance) const {}

		/**
		 * @brief This event is triggered right before the module thread terminates
		 * (not due to an exception, in this case refer to ModuleBase::OnError).
		 * Override it to unlock instruments this module depends on (via RunnableInstance::UnlockObject())
		 * and to deregister/unsubscribe from events (via InterModuleEvent::Deregister()).
		 * @param Instance Handle to the module thread's data
		*/
		virtual void OnExit(ModuleInstance* Instance) const {}

		/**
		 * @brief This event is triggered after the @p OnExit event.
		 * It calls EventListenersBase::Deregister() for all managers of event listeners
		 * stored in #RegisteredEvents since we do not trust in derived modules
		 * deregistering/unsubscribing from all registered events via InterModuleEvent::Deregister().
		 * @param Instance Handle to the module thread's data
		*/
		void OnDeregisterEvents(ModuleInstance* Instance) const;
		///@}

		/**
		 * @brief Module data belonging to this @p ModuleBase instance.
		*/
		const std::unique_ptr<ModuleDataType> ModuleData;

		/**
		 * @brief Holds a list of pointers to managers of event listeners of the events this module
		 * has registered/subscribed to. The stored pointers always outlive all module
		 * instances since they are static members of types derived from @p InterModuleEvent.
		*/
		std::vector<EventListenersBase*> RegisteredEvents;
	};

	/**
	 * @brief Creates an event of type @p DefaultEvent.
	 * @tparam ReceiverType Module type to receive this event.
	 * @tparam ...ArgsTs Types of arguments passed to the event function.
	 * @param Receiver Pointer to module receiving this event.
	 * @param EventFuncPtr Event function to be invoked (member function of ReceiverType expected).
	 * @param ...Args Arguments to pass to the event function.
	 * @throws Util::InvalidArgException is thrown if @p EventFuncPtr is nullptr.
	*/
	template <typename ReceiverType, typename... ArgsTs>
	auto MakeEvent(ReceiverType* Receiver,
		typename DefaultEvent<ReceiverType, std::tuple<std::remove_reference_t<ArgsTs>...>>::EventFuncPtrType EventFuncPtr,
		ArgsTs&& ...Args)
	{
		if (!EventFuncPtr)
			throw Util::InvalidArgException("EventFuncPtr cannot be nullptr.");

		// Use EventArgTs instead of std::tuple<ArgsTs...> since the latter would create a tuple of references!
		using EventArgTs = Util::remove_first_from_tuple_t<Util::argument_of_t<decltype(EventFuncPtr)>>;

		auto EventFunc = typename DefaultEvent<ReceiverType, EventArgTs>::EventFuncType(*Receiver, EventFuncPtr,
			Util::argument_of_t<decltype(EventFuncPtr)>(nullptr, std::forward<ArgsTs>(Args)...));

		return std::make_unique<DefaultEvent<ReceiverType, EventArgTs>>(EventFunc);
	}

	template <typename ReceiverType, typename EventType, typename... ArgsTs>
	void ModuleBase::MakeAndEnqueueEvent(ReceiverType* Receiver, EventType EventFuncPtr, ArgsTs&& ...Args) const
	{
		if (Receiver != this)
			throw Util::InvalidArgException("Receiver cannot point to a module different from the instance MakeAndEnqueueEvent() is called on.");

		EnqueueEvent(MakeEvent(Receiver, EventFuncPtr, std::forward<ArgsTs>(Args)...));
	}

	/**
	 * @brief Defines data for a thread belonging to a @p ModuleBase instance.
	 * Refer to @p RunnableInstance.
	*/
	class ModuleInstance : public RunnableInstance
	{
	public:
		/**
		 * @copydoc RunnableInstance::RunnableInstance(RunnableObject&, std::promise<void>&&)
		 * @param ModuleDataGetter Getter for module's data. Refer to ModuleBase::ModuleDataGetterType.
		*/
		ModuleInstance(ModuleBase& Owner, std::promise<void>&& ThreadExitedPromise,
			const ModuleBase::ModuleDataGetterType ModuleDataGetter);

		/**
		 * @copydoc RunnableInstance::RunnableInstance(RunnableInstance&&)
		*/
		ModuleInstance(ModuleInstance&& Other);

		~ModuleInstance() = default;

		/**
		 * @brief Might be called from anywhere where this @p ModuleInstance instance is accessible
		 * to make the associated module terminate.
		*/
		void Exit() noexcept { ShouldExit = true; }

		/**
		 * @brief Getter for #ShouldExit.
		 * @return Returns true if the module associated with this @p ModuleInstance should
		 * terminate, false otherwise.
		*/
		bool IsExiting() const noexcept { return ShouldExit; }

		/**
		 * @brief Getter for module's data. Refer to ModuleBase::ModuleDataGetterType.
		*/
		const ModuleBase::ModuleDataGetterType ModuleDataGetter;

	private:
		/**
		 * @brief Indicates whether the module thread using this @p ModuleInstance instance
		 * should terminate.
		*/
		bool ShouldExit = false;
	};

	/**
	 * @brief Casts the data base class @p From into a derived @p ModuleBase's (@p To) data
	 * class keeping the data locked by Util::SynchronizedPointer for thread-safe casting.
	 * @tparam To Type derived from @p ModuleBase into whose data class type to cast.
	 * @p const is added to @p To if @p From is @p const.
	 * @tparam From Data class type to cast from (must be @p ModuleDataBase).
	 * This type is automatically deduced.
	 * @param ModuleDataPtr Locked data class to cast from. @p ModuleDataPtr is empty
	 * after the cast.
	 * @return Locked data class cast to the data class belonging to @p To.
	 * @throws Util::InvalidArgException is thrown if @p ModuleDataPtr is empty.
	 * @throws Util::TypeErrorException is thrown if the cast fails.
	*/
	template <typename To, typename From, std::enable_if_t<
		std::is_same_v<ModuleDataBase, std::remove_cv_t<From>>, int> = 0
	>
	auto dynamic_ModuleData_cast(Util::SynchronizedPointer<From>&& ModuleDataPtr)
	{
		if (!ModuleDataPtr)
			throw Util::InvalidArgException("ModuleDataPtr must not be nullptr.");

		return Util::SynchronizedPointer<
			std::conditional_t<std::is_const_v<From>, std::add_const_t<typename To::ModuleDataType>, typename To::ModuleDataType>
		>(std::move(ModuleDataPtr));
	}

	/**
	 * @brief Common base class for all managers of event listeners of type @p TypedEventListeners.
	 * @p ModuleBase instances are considered event listeners.
	*/
	class EventListenersBase : public Util::ILockable
	{
	protected:
		EventListenersBase() = default;
		virtual ~EventListenersBase() {};

	public:
		/**
		 * @brief Deregisters/unsubscribes module @p Listener from the event.
		 * Indirectly calls ModuleBase::RemoveRegisteredEvent().
		 * @param Listener Module to deregister/unsubscribe.
		 * @param Timeout Time to wait for locking the mutex of this @p EventListenersBase instance.
		*/
		virtual void Deregister(const ModuleBase& Listener, const std::chrono::milliseconds Timeout = std::chrono::milliseconds(0)) = 0;
	};

	/**
	 * @brief Typed managers of event listeners class whose instances are owned by classes
	 * derived from @p InterModuleEvent. The class maps modules registering/subscribing to
	 * inter-module events to the respective event functions which are invoked when
	 * the related event occurs.
	 * @tparam ...EventFuncArgs Types of the arguments to be passed to event functions
	 * registered/subscribed to @p DerivedEvent.
	*/
	template <typename... EventFuncArgs>
	class TypedEventListeners : public EventListenersBase
	{
	public:
		/**
		 * @brief Type of event functions to be invoked when the event is triggered.
		 * The first @p ModuleInstance argument is a pointer to the module data of the
		 * respective module the event function is invoked on. The further parameters
		 * are determined by the class @p InterModuleEvent owning this @p TypedEventListeners.
		*/
		using EventFunctionType = std::function<void(ModuleInstance*, EventFuncArgs...)>;

		TypedEventListeners() = default;
		virtual ~TypedEventListeners() = default;

		/**
		 * @brief Registers/Subscribes module @p Listener to the event with the event function @p EventFunc.
		 * Indirectly calls ModuleBase::AddRegisteredEvent().
		 * @tparam CallableT Type of the event function (member function of @p Listener's derived type) to
		 * invoke for @p Listener when the event is triggered.
		 * @param Listener Module to register/subscribe.
		 * @param EventFunc Event function to invoke on module @p Listener when the event is triggered.
		 * @param Timeout Time to wait for locking the mutex of this @p EventListenersBase instance.
		*/
		template <typename CallableT>
		void Register(const ModuleBase& Listener, CallableT EventFunc, const std::chrono::milliseconds Timeout = std::chrono::milliseconds(0))
		{
			auto lock = AcquireLock(Timeout);
			RegisterUnsafe(Listener, EventFunc);
		}

		virtual void Deregister(const ModuleBase& Listener, const std::chrono::milliseconds Timeout = std::chrono::milliseconds(0)) override
		{
			auto lock = AcquireLock(Timeout);
			DeregisterUnsafe(Listener);
		}

		/**
		 * @brief Looks up the event function the module @p Listener has registered/subscribed with.
		 * @param Listener Module to look up the registered event function for
		 * @param Timeout Time to wait for locking the mutex of this @p EventListenersBase instance.
		 * @return Returns the registered event function or @p nullptr if @p Listener has not
		 * registered/subscribed for this event.
		*/
		EventFunctionType GetFunc(const ModuleBase& Listener, const std::chrono::milliseconds Timeout = DefaultTimeout) const
		{
			auto lock = AcquireLock(Timeout);
			return GetFuncUnsafe(Listener);
		}

	private:
		/**
		 * @brief This function is the version of @p Register() which is not thread-safe (assuming
		 * @p EventListenersBase's mutex has already been locked before.
		 * @tparam CallableT Type of the event function (member function of @p Listener's derived type) to
		 * invoke for @p Listener when the event is triggered.
		 * @param Listener Module to register/subscribe.
		 * @param EventFunc Event function to invoke on module @p Listener when the event is triggered.
		*/
		template <typename CallableT>
		void RegisterUnsafe(const ModuleBase& Listener, CallableT EventFunc) 
		{
			Listeners[&Listener] = [&Listener, EventFunc](ModuleInstance* Instance, EventFuncArgs... Args) {
				(dynamic_cast<std::add_const_t<typename Util::member_fn_ptr_traits<CallableT>::instance_type>&>(Listener).*EventFunc)(Instance, Args...);
			};

			Listener.EventListenersOnly.AddRegisteredEvent(*this);
		}

		/**
		 * @brief This function is the version of @p Deregister() which is not thread-safe (assuming
		 * @p EventListenersBase's mutex has already been locked before.
		 * @param Listener Module to deregister/unsubscribe.
		*/
		void DeregisterUnsafe(const ModuleBase& Listener)
		{
			auto ListenerIt = Listeners.find(&Listener);
			if (ListenerIt == Listeners.cend())
				return;

			Listeners.erase(ListenerIt);
			Listener.EventListenersOnly.RemoveRegisteredEvent(*this);
		}

		/**
		 * @brief This function is the version of @p GetFunc() which is not thread-safe (assuming
		 * @p EventListenersBase's mutex has already been locked before.
		 * @param Listener Module to look up the registered event function for
		 * @return Returns the registered event function or @p nullptr if @p Listener has not
		 * registered/subscribed for this event.
		*/
		EventFunctionType GetFuncUnsafe(const ModuleBase& Listener) const
		{
			auto ListenerIt = Listeners.find(&Listener);
			return ListenerIt != Listeners.cend() ? ListenerIt->second : nullptr;
		}

		/**
		 * @brief Each module can register to each inter-module event with one event
		 * function of type @p EventFunctionType. This mapping is stored here.
		*/
		std::unordered_map<const ModuleBase*, EventFunctionType> Listeners;
	};

	/**
	 * @brief Common base class for all inter-module events.
	*/
	class InterModuleEventBase : public EventBase
	{
	public:
		InterModuleEventBase() = default;
		virtual ~InterModuleEventBase() = 0;
	};

	/**
	 * @brief Typed base class for inter-module events to realize CRTP.
	 * @tparam DerivedEvent Type of the class derived from @p InterModuleEvent
	 * @tparam ...EventFuncArgs Types of the arguments to be passed to event functions
	 * registered/subscribed to @p DerivedEvent.
	*/
	template <typename DerivedEvent, typename... EventFuncArgs>
	class InterModuleEvent : public InterModuleEventBase
	{
	public:
		/**
		 * @brief Type of the manager of event listeners, which relates event listeners
		 * (instances of @p ModuleBase) to their event functions. The event functions
		 * expect @p EventFuncArgs as further arguments.
		*/
		using EventListenersType = TypedEventListeners<EventFuncArgs...>;

		InterModuleEvent() = default;
		virtual ~InterModuleEvent() {}

		/**
		 * @copybrief DynExp::TypedEventListeners::Register
		 * @copydetails TypedEventListeners::RegisterUnsafe
		*/
		template <typename CallableT>
		static void Register(const ModuleBase& Listener, CallableT EventFunc) { Listeners.Register(Listener, EventFunc); }

		/**
		 * @copybrief DynExp::EventListenersBase::Deregister
		 * @copydetails TypedEventListeners::DeregisterUnsafe
		*/
		static void Deregister(const ModuleBase& Listener) { Listeners.Deregister(Listener); }

	private:
		virtual void InvokeChild(ModuleInstance& Instance) const override final
		{
			auto EventFunc = Listeners.GetFunc(static_cast<const ModuleBase&>(Instance.GetOwner()));

			if (EventFunc)
				InvokeWithParamsChild(Instance, EventFunc);
		}

		/** @name Override
		 * Override by derived classes.
		*/
		///@{
		/**
		 * @brief Called by @p InvokeChild(). Override to call @p EventFunc with @p Instance
		 * as the first argument and with further arguments of type @p EventFuncArgs.
		 * @param Instance Handle to the module thread's data of the module @p EventFunc
		 * is invoked on.
		 * @param EventFunc Event function to invoke.
		*/
		virtual void InvokeWithParamsChild(ModuleInstance& Instance, EventListenersType::EventFunctionType EventFunc) const = 0;
		///@}

		/**
		 * @brief Holds one @p EventListenersType instance per derived event, which
		 * manages all the subscribers of @p DerivedEvent.
		*/
		static EventListenersType Listeners;
	};

	/**
	 * @brief Instantiate the respective static InterModuleEvent::Listeners variable to avoid linker errors.
	 * @copydetails InterModuleEvent
	*/
	template <typename DerivedEvent, typename... EventFuncArgs>
	typename InterModuleEvent<DerivedEvent, EventFuncArgs...>::EventListenersType InterModuleEvent<DerivedEvent, EventFuncArgs...>::Listeners;

	/**
	 * @brief Window class for Qt-based user interfaces belonging to %DynExp modules.
	 * User interface Qt window classes belonging to a module derived from @p ModuleBase
	 * have to derive from this class.
	*/
	class QModuleWidget : public QWidget
	{
		Q_OBJECT

		friend class QModuleBase;

	public:
		/**
		 * @brief Default Qt window flags for resizable module windows.
		 * @return Returns a combination of @p Qt::WindowFlags.
		*/
		constexpr static Qt::WindowFlags GetQtWindowFlagsResizable();

		/**
		 * @brief Default Qt window flags for non-resizable module windows.
		 * @return Returns a combination of @p Qt::WindowFlags.
		*/
		constexpr static Qt::WindowFlags GetQtWindowFlagsNonResizable();

		/**
		 * @brief Constructs a @p QModuleWidget instance.
		 * @param Owner Module owning this user interface window.
		 * @param Parent Parent Qt widget of this user interface window.
		 * There is no need to pass anything else than @p nullptr here.
		*/
		QModuleWidget(QModuleBase& Owner, QWidget* Parent = nullptr);

		~QModuleWidget() = default;

		/**
		 * @brief Getter for the owning module.
		 * @return Returns #Owner.
		 */
		const auto& GetOwner() const noexcept { return Owner; }

		/** @name Override
		 * Override by derived classes.
		*/
		///@{
		/**
		 * @brief Indicates the resizing behavior of the user interface window. Override to adjust.
		 * @return Return true when the window should be resizable by the user, false otherwise.
		*/
		virtual bool AllowResize() const noexcept { return false; }
		///@}

		/**
		 * @brief Depending on thr return value of @p AllowResize(), returns either the return
		 * value of @p GetQtWindowFlagsResizable() or @p GetQtWindowFlagsNonResizable().
		 * @return Returns a combination of @p Qt::WindowFlags.
		*/
		Qt::WindowFlags GetQtWindowFlags() const noexcept;

		/**
		 * @brief Recalls a path where modules might save recorded data to.
		 * Used by Util::PromptSaveFilePathModule() to recall the directory the user has chosen last
		 * for saving a file. This directory is the same across all modules.
		 * @return Path to show by default in the modules' file open/save dialogs.
		*/
		std::string GetDataSaveDirectory() const;

		/**
		 * @brief Sets a path where modules might save recorded data to.
		 * Used by Util::PromptSaveFilePathModule() to store the directory the user has chosen last
		 * for saving a file. This directory is the same across all modules.
		 * @param Directory Path to show by default in the modules' file open/save dialogs.
		*/
		void SetDataSaveDirectory(std::string_view Directory) const;

	private:
		/**
		 * @brief Enables/disables #DockWindowShortcut.
		 * @param Enable Pass true to enable #DockWindowShortcut, false to disable #DockWindowShortcut.
		*/
		void EnableDockWindowShortcut(bool Enable) noexcept;

		/**
		 * @brief Qt event indicating that the module window is closed.
		 * Asks the user whether to terminate the module.
		 * This Qt event is only triggered when the module window is contained in @p DynExpManager's QMdiArea.
		 * @param Event Refer to Qt documentation.
		*/
		virtual void closeEvent(QCloseEvent* Event) override;

		QModuleBase& Owner;						//!< Module owning this user interface window (reference, because it should never change nor be @p nullptr).
		DynExpManager* DynExpMgr;				//!< %DynExp's main window (pointer, because it is set later by QModuleBase::InitUI()).

		QShortcut* DockWindowShortcut;			//!< Shortcut (Ctrl + D) to dock/undock a module window to/from %DynExp's main window.
		QShortcut* FocusMainWindowShortcut;		//!< Shortcut (Ctrl + 0) to activate/focus %DynExp's main window.

	private slots:
		void OnDockWindow();					//!< Qt slot called when #DockWindowShortcut is activated, calls QModuleBase::DockUndockWindow().
		void OnFocusWindow();					//!< Qt slot called when QModuleBase::ModuleWindowFocusAction is triggered, calls QModuleBase::SetFocus().
		void OnFocusMainWindow();				//!< Qt slot called when #FocusMainWindowShortcut is activated, calls QModuleBase::FocusMainWindow().
	};

	/**
	 * @brief Provides a frame for @p QModuleWidget windows, which are undocked
	 * from the @p DynExpManager's QMdiArea.
	*/
	class QModuleDockWidget : public QDockWidget
	{
		Q_OBJECT

	public:
		/**
		 * @brief Constructs a @p QModuleDockWidget instance.
		 * @param Owner Module owning this user interface window.
		 * @param Parent Parent Qt widget of this user interface window.
		 * There is no need to pass anything else than @p nullptr here.
		 * @param Flags Combination of @p Qt::WindowFlags to decorate the @p QModuleDockWidget
		 * instance with. Refer to QModuleWidget::GetQtWindowFlags().
		*/
		QModuleDockWidget(QModuleBase& Owner, QWidget* Parent, Qt::WindowFlags Flags);

		~QModuleDockWidget() = default;

	private:
		/**
		 * @brief Qt event indicating that the @p QModuleDockWidget window is closed.
		 * Docks the module window again to %DynExp's main window.
		 * This Qt event is only triggered when the module window is contained in the @p QModuleDockWidget.
		 * @param Event Refer to Qt documentation.
		*/
		virtual void closeEvent(QCloseEvent* Event) override;

		/**
		 * @brief Qt event indicating that keys were pressed when the @p QModuleDockWidget
		 * window was focused. Redirects "Ctrl + <Number key>" key sequences to %DynExp's main window.
		 * Refer to QModuleBase::FocusMainWindow() and QModuleBase::SendKeyPressEventToMainWindow().
		 * This Qt event is only triggered when the module window is contained in the @p QModuleDockWidget.
		 * @param Event Qt event to redirect. Refer to Qt documentation.
		*/
		virtual void keyPressEvent(QKeyEvent* Event) override;

		QModuleBase& Owner;						//!< Module owning this user interface window.
	};

	/**
	 * @brief Data class for @p QModuleBase.
	*/
	class QModuleDataBase : public ModuleDataBase
	{
	public:
		QModuleDataBase() = default;
		virtual ~QModuleDataBase() = default;

	private:
		void ResetImpl(dispatch_tag<ModuleDataBase>) override final;
		virtual void ResetImpl(dispatch_tag<QModuleDataBase>) {};		//!< @copydoc ResetImpl(dispatch_tag<DynExp::ModuleDataBase>)
	};

	/**
	 * @brief Bundles several parameters to describe a UI window's style. Use in parameter classes.
	*/
	class WindowStyleParamsExtension
	{
	public:
		/**
		 * @brief Indicates the window state for showing it.
		*/
		enum WindowStateType { Normal, Minimized, Maximized };
		/**
		 * @var WindowStyleParamsExtension::WindowStateType WindowStyleParamsExtension::Normal
		 * The window is shown in the default way.
		*/
		/**
		 * @var WindowStyleParamsExtension::WindowStateType WindowStyleParamsExtension::Minimized
		 * The window is shown minimized.
		*/
		/**
		 * @var WindowStyleParamsExtension::WindowStateType WindowStyleParamsExtension::Maximized
		 * The window is shown maximized.
		*/

		/**
		 * @brief Indicates the window docking state.
		*/
		enum WindowDockingStateType { Docked, Undocked };
		/**
		 * @var WindowStyleParamsExtension::WindowDockingStateType WindowStyleParamsExtension::Docked
		 * The window is docked to the %DynExp's main window.
		*/
		/**
		 * @var WindowStyleParamsExtension::WindowDockingStateType WindowStyleParamsExtension::Undocked
		 * The window is undocked from the %DynExp's main window.
		*/

		/**
		 * @brief Constructs a @p WindowStyleParamsExtension instance.
		 * @param Owner Parameter class owning the parameters bundled by this instance.
		 * @param Prefix String to prepend to the bundled parameters' names in case
		 * @p Owner owns multiple @p WindowStyleParamsExtension instances.
		*/
		WindowStyleParamsExtension(ParamsBase& Owner, const std::string& Prefix = "")
			: WindowPosX{ Owner, Prefix + "WindowPosX" },
			WindowPosY{ Owner, Prefix + "WindowPosY" },
			WindowWidth{ Owner, Prefix + "WindowWidth" },
			WindowHeight{ Owner, Prefix + "WindowHeight" },
			WindowState{ Owner, Prefix + "WindowState", WindowStateType::Normal },
			WindowDockingState{ Owner, Prefix + "WindowDockingState", WindowDockingStateType::Docked }
		{}

		/**
		 * @brief Applies the bundled style parameters to @p Widget.
		 * @param Widget Widget to apply the style parameters to.
		 * @param Show Pass true to show @p Widget, false not to affect the @p Widget's show state.
		*/
		void ApplyTo(QWidget& Widget, bool Show = true) const;

		/**
		 * @brief Assigns styles applied to @p Widget to the bundled parameters.
		 * @param Widget Widget to retrieve the style from.
		*/
		void FromWidget(const QWidget& Widget);

		ParamsBase::Param<int> WindowPosX;								//!< Window x coordinate
		ParamsBase::Param<int> WindowPosY;								//!< Window y coordinate
		ParamsBase::Param<unsigned int> WindowWidth;					//!< Window width
		ParamsBase::Param<unsigned int> WindowHeight;					//!< Window height
		ParamsBase::Param<WindowStateType> WindowState;					//!< Window show state
		ParamsBase::Param<WindowDockingStateType> WindowDockingState;	//!< Window docking state
	};

	/**
	 * @brief Parameter class for @p QModuleBase.
	*/
	class QModuleParamsBase : public ModuleParamsBase
	{
	public:
		/**
		 * @brief Constructs the parameters for a @p QModuleBase instance.
		 * @copydetails ParamsBase::ParamsBase
		*/
		QModuleParamsBase(ItemIDType ID, const DynExpCore& Core)
			: ModuleParamsBase(ID, Core), WindowStyleParams(*this) {}

		virtual ~QModuleParamsBase() = 0;

		virtual const char* GetParamClassTag() const noexcept override { return "QModuleParamsBase"; }

		WindowStyleParamsExtension WindowStyleParams;							//!< @copydoc WindowStyleParamsExtension

	private:
		void ConfigureParamsImpl(dispatch_tag<ModuleParamsBase>) override final { ConfigureParamsImpl(dispatch_tag<QModuleParamsBase>()); }
		virtual void ConfigureParamsImpl(dispatch_tag<QModuleParamsBase>) {}	//!< @copydoc ConfigureParamsImpl(dispatch_tag<DynExp::ModuleParamsBase>)
	};

	/**
	 * @brief Configurator class for @p QModuleBase
	*/
	class QModuleConfiguratorBase : public ModuleConfiguratorBase
	{
	public:
		using ObjectType = QModuleBase;
		using ParamsType = QModuleParamsBase;

		QModuleConfiguratorBase() = default;
		virtual ~QModuleConfiguratorBase() = 0;
	};

	/**
	 * @brief Base class for modules with a Qt-based user interface.
	 * Derive from this class to implement modules with a user interface.
	*/
	class QModuleBase : public ModuleBase
	{
	public:
		using ParamsType = QModuleParamsBase;									//!< @copydoc Object::ParamsType
		using ConfigType = QModuleConfiguratorBase;								//!< @copydoc Object::ConfigType
		using ModuleDataType = QModuleDataBase;									//!< @copydoc ModuleBase::ModuleDataType

		/**
		 * @brief Constructs a @p QModuleBase instance.
		 * @copydetails ModuleBase::ModuleBase
		*/
		QModuleBase(const std::thread::id OwnerThreadID, DynExp::ParamsBasePtrType&& Params);

		virtual ~QModuleBase() = 0;

		bool HasUI() const noexcept override final { return true; }

		/** @name Main thread only
		 * These functions must not be called by module thread.
		 * Logical const-ness: Not const to hide from the module thread.
		*/
		///@{
		/**
		 * @brief Sets up the module's user interface widgets and window frames.
		 * Called by the main user interface thread in DynExpManager::RegisterModuleUI().
		 * @param DynExpMgr Pointer to %DynExp's main window
		 * @param MdiArea @copybrief QModuleBase::MdiArea
		 * @return Returns the initialized #ModuleWindowFocusAction.
		 * @throws Util::InvalidArgException is thrown if @p MdiArea is @p nullptr.
		 * @throws Util::InvalidDataException is thrown if @p MakeUIWidget() returns @p nullptr.
		*/
		QAction& InitUI(DynExpManager& DynExpMgr, QMdiArea* const MdiArea);

		void HideUI();						//!< Removes #MdiSubWindow, #DockWidget, and #ModuleWindowFocusAction setting them to @p nullptr.
		void DisableUI();					//!< Disables all user interface controls in #Widget. Does nothing if #Widget is @p nullptr.
		void UpdateUI();					//!< Enables the user interface controls in #Widget. Does nothing if #Widget is @p nullptr. Calls @p UpdateUIChild().

		/**
		 * @brief Updates the icon assigned to #ModuleWindowFocusAction depending on whether #Widget
		 * is docked to or undocked from #MdiArea.
		 * Called by the main user interface thread in DynExpManager::UpdateModuleWindowsActionIcons().
		 * @throws Util::InvalidStateException is thrown if any of #Widget, #MdiSubWindow, or
		 * #ModuleWindowFocusAction are @p nullptr since the user interface has not been created yet.
		*/
		void UpdateModuleWindowFocusAction();

		void DockWindow() noexcept;			//!< Docks #Widget to #MdiArea. Does nothing if any of #Widget, #MdiArea, #MdiSubWindow, or #DockWidget are @p nullptr.
		void UndockWindow() noexcept;		//!< Undocks #Widget from #MdiArea. Does nothing if any of #Widget, #MdiArea, #MdiSubWindow, or #DockWidget are @p nullptr.
		void DockUndockWindow() noexcept;	//!< Toggles the docking state of #Widget. Does nothing if any of #Widget, #MdiArea, #MdiSubWindow, or #DockWidget are @p nullptr.
		void SetFocus() noexcept;			//!< Focuses/activates #Widget and moves it on top of other windows if possible. Does nothing if any of #Widget, #MdiArea, #MdiSubWindow, or #DockWidget are @p nullptr.
		void FocusMainWindow() noexcept;	//!< Focuses/activates %DynExp's main window calling DynExpManager::FocusMainWindow(). Does nothing if #Widget is @p nullptr.

		/**
		 * @brief Sends a Qt key press event to %DynExp's main window calling
		 * DynExpManager::PostKeyPressEvent(). Does nothing if #Widget is @p nullptr.
		 * @param Event Qt event to send. Refer to Qt documentation.
		*/
		void SendKeyPressEventToMainWindow(QKeyEvent* Event) noexcept;
		
		/**
		 * @brief Checks whether #Widget is docked.
		 * @return Returns true if #Widget is docked to #MdiArea, false otherwise.
		 * Also returns true if either #Widget or #MdiSubWindow are @p nullptr
		 * since being docked is the default for a module's user interface window.
		*/
		bool IsWindowDocked() noexcept;

		/**
		 * @brief Checks whether #Widget is docked and active.
		 * @return Returns true if #Widget is docked to #MdiArea and if it is active (i.e. having
		 * the focus), false otherwise.
		 * @throws Util::InvalidStateException is thrown if either #Widget or #MdiSubWindow are
		 * @p nullptr since the user interface has not been created yet.
		*/
		bool IsActiveWindow();
		///@}

	protected:
		/**
		 * @brief Uses Qt's connect mechanism to connect a @p QObject's signal to a %DynExp module's event.
		 * By directly connecting slots to signals via @p QObject::connect(), the slot function runs in
		 * the user interface thread. In contrast, this function connects a signal to a lambda function, which
		 * itself enqueues an event in the module's event queue. The event runs in the module thread.
		 * Arguments passed from the signal to the lambda function are forwarded to the event.
		 * @tparam SenderType Class derived from @p QObject.
		 * @tparam SignalType Type of the function pointer pointing to the signal to be connected.
		 * @tparam ReceiverType Class derived from @p ModuleBase.
		 * @tparam EventType Type of the function pointer pointing to the event to which the signal is connected.
		 * It is expected that @p EventType points to a const member function of class @p ReceiverType returning void.
		 * The member function is expected to accept a pointer to the receiver's @p ModuleInstance as a first argument.
		 * Subsequent arguments must be compatible with the arguments of @p SignalType. They must not be references
		 * or pointers since @p Event is invoked later when references or pointers might have become invalid. Use
		 * pass-by-value.
		 * @param Sender Pointer to a @p QObject instance whose signal to connect to an event.
		 * @param Signal @p QObject's signal to connect to the event.
		 * @param Receiver Pointer to a module to receive the event when the @p QObject's signal is emitted.
		 * @param Event Event function to which the @p QObject's signal is connected.
		 * @throws Util::InvalidArgException is thrown if either @p Sender or @p Receiver are @p nullptr
		 * or if @p QObject::connect() fails.
		*/
		template <typename SenderType, typename SignalType, typename ReceiverType, typename EventType>
		void Connect(SenderType* Sender, SignalType Signal, ReceiverType* Receiver, EventType Event);

		/**
		 * @brief Getter for #Widget.
		 * @tparam WidgetType Type derived from @p QModuleWidget to cast #Widget to.
		 * @return Returns #Widget cast with @p dynamic_cast to @p WidgetType.
		 * @throws Util::InvalidStateException is thrown if #Widget is @p nullptr since it has not been created yet.
		*/
		template <typename WidgetType>
		WidgetType* GetWidget() const;

	private:
		virtual void RestoreWindowStatesFromParamsChild() override;
		virtual void UpdateParamsFromWindowStatesChild() override;

		/**
		 * @brief Used by @p InitUI() to set the widget properties (such as title, icon, size
		 * constraints) of #MdiSubWindow and #DockWidget.
		 * @param WindowWidget Pointer to a Qt window widget whose properties to set
		 * @throws Util::InvalidArgException is thrown if @p WindowWidget is @p nullptr.
		*/
		void SetWidgetProperties(QWidget* const WindowWidget) const;

		void ResetImpl(dispatch_tag<ModuleBase>) override final;
		virtual void ResetImpl(dispatch_tag<QModuleBase>) = 0;	//!< @copydoc ResetImpl(dispatch_tag<DynExp::ModuleBase>)

		/** @name Override
		 * Override by derived classes.
		*/
		///@{
		/**
		 * @brief Used by @p InitUI() as a factory function for the module's user interface widget.
		 * Create the widget here using @p std::make_unique and then use @p Connect() to connect
		 * Qt signals to the module's event functions.
		 * @return Return the created user interface widget.
		*/
		virtual std::unique_ptr<QModuleWidget> MakeUIWidget() = 0;

		/**
		 * @brief Use this function to update the module's user interface widget according to
		 * the module data obtained from @p ModuleDataGetter. This function is called periodically
		 * from %DynExp's main user interface thread. Do not access the module's widget anywhere else!
		 * @param ModuleDataGetter Getter to lock and obtain the module data
		*/
		virtual void UpdateUIChild(const ModuleBase::ModuleDataGetterType& ModuleDataGetter) {}
		///@}

		QModuleWidget* Widget;									//!< User interface widget belonging to the module.
		std::unique_ptr<QMdiSubWindow> MdiSubWindow;			//!< Frame for #Widget when the module window is docked to #MdiArea.
		std::unique_ptr<QModuleDockWidget> DockWidget;			//!< Frame for #Widget when the module window is undocked from #MdiArea.
		std::unique_ptr<QAction> ModuleWindowFocusAction;		//!< Qt action to push module window into focus. When triggered, QModuleWidget::OnFocusWindow() in invoked.

		QMdiArea* MdiArea;										//!< Pointer to @p DynExpManager's QMdiArea
	};

	template <typename SenderType, typename SignalType, typename ReceiverType, typename EventType>
	void QModuleBase::Connect(SenderType* Sender, SignalType Signal, ReceiverType* Receiver, EventType Event)
	{
		EnsureCallFromOwningThread();

		if (!Sender || !Receiver)
			throw Util::InvalidArgException("Sender and Receiver cannot be nullptr.");

		// Arguments must not be passed by reference or by pointer to Event since Event is invoked later when
		// references/pointers might not be valid anymore.
		auto Connection = QObject::connect(Sender, Signal, [this, Receiver, Event](auto... Args) {
			try
			{
				MakeAndEnqueueEvent(Receiver, Event, std::move(Args)...);
			}
			catch (const Util::Exception& e)
			{
				Util::EventLog().Log("Posting an event from a module's UI to the module's thread, the error listed below occurred.", Util::ErrorType::Error);
				Util::EventLog().Log(e);
			}
			catch (const std::exception& e)
			{
				Util::EventLog().Log("Posting an event from a module's UI to the module's thread, the error listed below occurred.", Util::ErrorType::Error);
				Util::EventLog().Log(e.what());
			}
			catch (...)
			{
				Util::EventLog().Log("Posting an event from a module's UI to the module's thread, an unknown error occurred.", Util::ErrorType::Error);
			}
		});

		if (!Connection)
			throw Util::InvalidArgException("QObject::connect() cannot establish a connection for the given sender and signal types.");
	}

	template <typename WidgetType>
	WidgetType* QModuleBase::GetWidget() const
	{
		// EnsureCallFromOwningThread(); considered here, but it also prohibits legitimate calls
		// to thread-safe (probably const) member functions of the widget. Note, accessing the UI
		// objects from another thread than the application's main thread is strongly forbidden!

		if (!Widget)
			throw Util::InvalidStateException("UI widget has not been created yet.");

		// Throws if it fails.
		return &dynamic_cast<WidgetType&>(*Widget);
	}
}

/**
 * @brief %DynExp's module namespace contains the implementation of %DynExp modules
 * which extend %DynExp's core functionality in a modular way.
*/
namespace DynExpModule {};