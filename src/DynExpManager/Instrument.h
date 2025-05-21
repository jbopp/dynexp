// This file is part of DynExp.

/**
 * @file Instrument.h
 * @brief Implementation of %DynExp instrument objects.
*/

#pragma once

#include "stdafx.h"
#include "Object.h"

namespace DynExp
{
	class InstrumentBase;
	class InstrumentInstance;
	class ExceptionContainer;
	class TaskBase;
	class InitTaskBase;
	class ExitTaskBase;
	class UpdateTaskBase;

	/**
	 * @brief Pointer type to store an instrument (DynExp::InstrumentBase) with
	*/
	using InstrumentPtrType = std::shared_ptr<InstrumentBase>;

	/**
	 * @brief Factory function to generate a configurator for a specific instrument type
	 * @tparam InstrumentT Type of the instrument to generate a configurator for
	 * @return Pointer to the configurator for the specified instrument type
	*/
	template <typename InstrumentT>
	ConfiguratorBasePtrType MakeInstrumentConfig()
	{
		return std::make_shared<typename InstrumentT::ConfigType>();
	}

	/**
	 * @brief Factory function to generate an instrument of a specific type
	 * @tparam InstrumentT Type of the instrument to generate
	 * @param OwnerThreadID ID of the thread owning the instrument
	 * @param Params Reference to theinstrument's parameters to take ownership of
	 * @return Pointer to the instrument of the specified type
	*/
	template <typename InstrumentT>
	InstrumentPtrType MakeInstrument(const std::thread::id OwnerThreadID, ParamsBasePtrType&& Params)
	{
		dynamic_Params_cast<InstrumentT>(Params.get())->InstrumentData = std::make_unique<typename InstrumentT::InstrumentDataType>();

		return std::make_shared<InstrumentT>(OwnerThreadID, std::move(Params));
	}

	/**
	 * @brief Factory function to create a task to be enqueued in an instrument's
	 * task queue.
	 * @tparam TaskT Type of a task derived from class @p TaskBase
	 * @tparam ...ArgTs Types of the arguments to forward to the task's constructor
	 * @param ...Args Arguments to forward to the task's constructor
	 * @return Pointer to the task of the specified type
	*/
	template <typename TaskT, typename... ArgTs>
	std::unique_ptr<TaskT> MakeTask(ArgTs&& ...Args)
	{
		return std::make_unique<TaskT>(std::forward<ArgTs>(Args)...);
	}

	/**
	 * @brief Instruments run in their own thread. This is the instrument thread's main
	 * function.
	 * @param Instance Handle to the instrument thread's data related to the instrument
	 * running this thread. The instrument thread is expected to let the lifetime of
	 * @p Instance expire upon termination.
	 * @param Instrument Pointer to the instrument running this thread
	 * @return Util::DynExpErrorCodes::NoError if the thread terminated without an error,
	 * the respective error code otherwise.
	*/
	int InstrumentThreadMain(InstrumentInstance Instance, InstrumentBase* const Instrument);

	/**
	 * @brief Wrapper holding a pointer to an exception and providing functionality for
	 * accessing it. Used to transfer exceptions between an instrument's task and the
	 * @p ModuleBase module enqueuing the task. Refer to TaskBase::CallbackType.
	*/
	class ExceptionContainer
	{
	public:
		/**
		 * @brief Constructs an @p ExceptionContainer instance.
		 * @param Exception Pointer to an exception to store in the wrapper
		*/
		ExceptionContainer(const std::exception_ptr& Exception = nullptr) noexcept
			: Exception(Exception) {}

		/**
		 * @brief Throws the stored exception. Doesn't do anything if there isn't a stored exception.
		*/
		void Throw() const { if (Exception) std::rethrow_exception(Exception); }

		/**
		 * @brief Checks whether the wrapper holds an exception.
		 * @return Returns true if it holds an exception, false otherwise.
		*/
		bool IsError() const { return static_cast<bool>(Exception); }

		/**
		 * @brief Getter for #Exception
		 * @return Returns #Exception.
		*/
		auto GetException() const { return Exception; }

		/**
		 * @brief Removes the stored exception. If this method is called by a task's CallbackFunc
		 * in case of an exception having ocurred while running the task, the exception is
		 * considered handled. In this case, no exception leaves TaskBase::Run().
		*/
		void ClearError() { Exception = nullptr; }

	private:
		std::exception_ptr Exception;	//!< Exception stored in the wrapper.
	};

	/**
	 * @brief Data structure to contain data which is synchronized in between different threads.
	 * This is needed since the instrument thread, the threads of different modules, as well as
	 * the main thread might access the instrument at the same time. Every class (indirectly)
	 * derived from class @p InstrumentBase must be accompanied by a instrument data class
	 * derived from @p InstrumentDataBase. @p InstrumentDataBase and derived classes contain
	 * data shared by the respective instrument, module and main threads. Data only used by
	 * the instrument itself should be private members of the instrument class.
	 * @warning For the same @p Object, always lock the mutex of the corresponding parameter class
	 * before the mutex of the corresponding data class (or only one of them).
	*/
	class InstrumentDataBase : public Util::ISynchronizedPointerLockable
	{
	public:
		using TaskQueueType = std::list<std::unique_ptr<TaskBase>>;		//!< Type of an instrument task queue owning the tasks within
		using TaskQueueIteratorType = TaskQueueType::const_iterator;	//!< Const iterator type to elements of @p TaskQueueType

	protected:
		/**
		 * @brief Refer to ParamsBase::dispatch_tag.
		 * @tparam Type (derived from class @p InstrumentDataBase) to instantiate the dispatch tag template with.
		*/
		template <typename>
		struct dispatch_tag {};

	private:
		/**
		 * @brief Allow exclusive access to some of @p InstrumentDataBase's private methods to @p InstrumentBase.
		*/
		class InstrumentBaseOnlyType
		{
			friend class InstrumentDataBase;
			friend class InstrumentBase;

			/**
			 * @brief Construcs an instance - one for each @p InstrumentDataBase instance
			 * @param Parent Owning @p InstrumentDataBase instance
			*/
			constexpr InstrumentBaseOnlyType(InstrumentDataBase& Parent) noexcept : Parent(Parent) {}

			void Reset() { Parent.Reset(); }																																								//!< @copydoc InstrumentDataBase::Reset
			void EnqueueTask(std::unique_ptr<TaskBase>&& Task, bool CallFromInstrThread, bool NotifyReceiver) { Parent.EnqueueTask(std::move(Task), CallFromInstrThread, NotifyReceiver); }					//!< @copydoc InstrumentDataBase::EnqueueTask(std::unique_ptr<TaskBase>&&, bool, bool)
			void EnqueuePriorityTask(std::unique_ptr<TaskBase>&& Task, bool CallFromInstrThread, bool NotifyReceiver) { Parent.EnqueuePriorityTask(std::move(Task), CallFromInstrThread, NotifyReceiver); }	//!< @copydoc InstrumentDataBase::EnqueuePriorityTask(std::unique_ptr<TaskBase>&&, bool, bool)
			void RemoveTaskFromQueue(TaskQueueIteratorType& Task) { Parent.RemoveTaskFromQueue(Task); }																										//!< @copydoc InstrumentDataBase::RemoveTaskFromQueue
			void RemoveAllTasks() { Parent.RemoveAllTasks(); }																																				//!< @copydoc InstrumentDataBase::RemoveAllTasks
			void RemoveAllTasksExceptFront() { Parent.RemoveAllTasksExceptFront(); }																														//!< @copydoc InstrumentDataBase::RemoveAllTasksExceptFront
			void CloseQueue() { Parent.CloseQueue(); }																																						//!< @copydoc InstrumentDataBase::CloseQueue

			auto& GetNewTaskNotifier() noexcept { return Parent.GetNewTaskNotifier(); }																														//!< @copydoc InstrumentDataBase::GetNewTaskNotifier

			InstrumentDataBase& Parent;		//!< Owning @p InstrumentDataBase instance
		};

		/**
		 * @brief Allow exclusive access to some of @p InstrumentDataBase's private methods to the instrument thread
		 * @p InstrumentThreadMain().
		*/
		class InstrumenThreadOnlyType
		{
			friend class InstrumentDataBase;
			friend int InstrumentThreadMain(InstrumentInstance, InstrumentBase* const);

			/**
			 * @brief Construcs an instance - one for each @p InstrumentDataBase instance
			 * @param Parent Owning @p InstrumentDataBase instance
			*/
			constexpr InstrumenThreadOnlyType(InstrumentDataBase& Parent) noexcept : Parent(Parent) {}

			auto& GetNewTaskNotifier() noexcept { return Parent.GetNewTaskNotifier(); }													//!< @copydoc InstrumentDataBase::GetNewTaskNotifier()
			void SetLastUpdateTime(std::chrono::system_clock::time_point LastUpdate) { Parent.LastUpdate = LastUpdate; }				//!< Setter for InstrumentDataBase::LastUpdate
			void SetException(std::exception_ptr InstrumentException) noexcept { Parent.InstrumentException = InstrumentException; }	//!< Setter for InstrumentDataBase::InstrumentException

			InstrumentDataBase& Parent;		//!< Owning @p InstrumentDataBase instance
		};

	public:
		InstrumentDataBase() : InstrumentBaseOnly(*this), InstrumentThreadOnly(*this), QueueClosed(false) {}
		virtual ~InstrumentDataBase() {}

	public:
		/** @name Instrument task queue
		 * Methods to access and manipulate the task queue belonging to the instrument which owns
		 * the respective @p InstrumentDataBase's instance
		*/
		///@{
		/**
		 * @brief Enqueues a task at the back of an instrument's task queue and
		 * notifies the instrument about the new task.
		 * @param Task Task to enqueue, the queue takes ownership of the task.
		*/
		void EnqueueTask(std::unique_ptr<TaskBase>&& Task) { EnqueueTask(std::move(Task), false, true); }

		/**
		 * @brief Enqueues a task at the front of an instrument's task queue and
		 * notifies the instrument about the new task.
		 * @param Task Task to enqueue, the queue takes ownership of the task.
		*/
		void EnqueuePriorityTask(std::unique_ptr<TaskBase>&& Task) { EnqueuePriorityTask(std::move(Task), false, true); }

		/**
		 * @brief Removes a task from the front of an instrument's task queue.
		 * @return Returns the task releasing ownership of it. Returns nullptr
		 * if the queue is empty.
		 * @throws Util::InvalidStateException is thrown if the task to be removed is locked.
		 * Refer to TaskBase::TaskState.
		*/
		std::unique_ptr<TaskBase> PopTaskFront();

		/**
		 * @brief Removes a task from the back of an instrument's task queue.
		 * @return Returns the task releasing ownership of it. Returns nullptr
		 * if the queue is empty.
		 * @throws Util::InvalidStateException is thrown if the task to be removed is locked.
		 * Refer to TaskBase::TaskState.
		*/
		std::unique_ptr<TaskBase> PopTaskBack();

		/**
		 * @brief Getter for first enqueued task
		 * @return Returns an iterator to the instrument task queue's beginning.
		*/
		auto GetTaskFront() noexcept { return TaskQueue.begin(); }

		/**
		 * @brief Getter for last enqueued task
		 * @return Returns an iterator to the instrument task queue's end.
		*/
		auto GetTaskBack() noexcept { return TaskQueue.end(); }

		/**
		 * @brief Getter for the instrument task queue's length
		 * @return Returns the number of enqueued tasks.
		*/
		size_t GetNumEnqueuedTasks() const noexcept { return TaskQueue.size(); }

		/**
		 * @brief Removes a task from the front of an instrument's list of
		 * finished tasks.
		 * @return Returns the task releasing ownership of it. Returns nullptr
		 * if the list is empty.
		*/
		std::unique_ptr<TaskBase> PopFinishedTask();

		/**
		 * @brief Getter for the length of the instrument's list of finished tasks
		 * @return Returns the number of finished tasks.
		*/
		size_t GetNumFinishedTasks() const noexcept { return FinishedTasks.size(); }

		/**
		 * @brief Determines whether the instrument task queue is closed
		 * @return Returns #QueueClosed.
		*/
		bool IsQueueClosed() const noexcept { return QueueClosed; }
		///@}

		/**
		 * @brief Getter for #LastUpdate
		 * @return Returns the time point when the instrument issued a task to update its data last.
		*/
		auto GetLastUpdateTime() const { return LastUpdate; }

		/**
		 * @brief Getter for InstrumentDataBase::InstrumentException
		 * @return Returns the exception being responsible for the instrument's current state.
		*/
		auto GetException() const noexcept { return InstrumentException; }

		InstrumentBaseOnlyType InstrumentBaseOnly;		//!< @copydoc InstrumentBaseOnlyType
		InstrumenThreadOnlyType InstrumentThreadOnly;	//!< @copydoc InstrumenThreadOnlyType

	private:
		/**
		 * @copydoc EnqueueTask(std::unique_ptr<TaskBase>&& Task)
		 * @param CallFromInstrThread Pass true if the instrument itself enqueues a task.
		 * This argument is forwarded to @p CheckQueueState().
		 * @param NotifyReceiver Determines whether the instrument thread is notified that
		 * a new task has been enqueued. This is e.g. not desired when enqueuing update tasks
		 * in InstrumentBase::UpdateData().
		*/
		void EnqueueTask(std::unique_ptr<TaskBase>&& Task, bool CallFromInstrThread, bool NotifyReceiver);
		
		/**
		 * @copydoc EnqueuePriorityTask(std::unique_ptr<TaskBase>&& Task)
		 * @param CallFromInstrThread Pass true if the instrument itself enqueues a task.
		 * This argument is forwarded to @p CheckQueueState().
		 * @param NotifyReceiver Determines whether the instrument thread is notified that
		 * a new task has been enqueued. This is e.g. not desired when enqueuing update tasks
		 * in InstrumentBase::UpdateData().
		*/
		void EnqueuePriorityTask(std::unique_ptr<TaskBase>&& Task, bool CallFromInstrThread, bool NotifyReceiver);
		
		/**
		 * @brief Removes a task from the instrument's task queue and inserts it into the
		 * instrument's list of finished tasks if TaskBase::KeepFinishedTask() returns true.
		 * @param Task Iterator to an enqueued task.
		*/
		void RemoveTaskFromQueue(TaskQueueIteratorType& Task);

		/**
		 * @brief Clears the instrument's task queue.
		*/
		void RemoveAllTasks();

		/**
		 * @brief Clears the instrument's task queue but keeps the front task (the task
		 * with highest priority which is handled next).
		*/
		void RemoveAllTasksExceptFront();

		/**
		 * @brief Clsoes the instrument's task queue setting #QueueClosed to true.
		*/
		void CloseQueue() { QueueClosed = true; }

		/**
		 * @brief Getter for #NewTaskNotifier
		 * @return Returns #NewTaskNotifier to notify the instrument thread about new tasks.
		*/
		Util::OneToOneNotifier& GetNewTaskNotifier() noexcept { return NewTaskNotifier; }

		/** @name Stopped instrument only
		 * Must only be called when the instrument thread is not running.
		*/
		///@{
		/**
		 * @brief Resets the @p InstrumentDataBase's instance and calls
		 * ResetImpl(dispatch_tag<InstrumentDataBase>) subsequently.
		*/
		void Reset();

		/**
		 * @brief Refer to DynExp::InstrumentDataBase::Reset(). Using tag dispatch mechanism to ensure that @p ResetImpl()
		 * of every derived class gets called - starting from @p InstrumentDataBase, descending the inheritance hierarchy.
		 * Override in order to reset derived classes.
		*/
		virtual void ResetImpl(dispatch_tag<InstrumentDataBase>) {}
		///@}

		/**
		 * @brief Throws #InstrumentException if it is not nullptr using Util::ForwardException().
		*/
		void CheckError() const;

		/**
		 * @brief Checks whether it is currently allowed to enqueue tasks into the instrument
		 * task queue.
		 * @param CallFromInstrThread Pass true if the instrument itself enqueues a task.
		 * @throws Util::InvalidStateException is thrown if the instrument task queue is closed
		 * and @p CallFromInstrThread is false.
		*/
		void CheckQueueState(bool CallFromInstrThread) const;

		TaskQueueType TaskQueue;							//!< FIFO task queue of the instrument owning this @p InstrumentDataBase instance
		TaskQueueType FinishedTasks;						//!< List of the instrument's finished tasks. Tasks are moved here from #TaskQueue after completion.
		bool QueueClosed;									//!< If set to true, no new tasks can be enqueued (useful if an instrument is e.g. stopped).

		/**
		 * @brief Used to notify the instrument thread about new tasks when enqueuing tasks
		 * into the task queue. This allows the instrument thread to sleep until new tasks
		 * have to be handled.
		*/
		Util::OneToOneNotifier NewTaskNotifier;

		std::chrono::system_clock::time_point LastUpdate;	//!< Time point when the instrument thread called InstrumentBase::UpdateDataInternal() the last time.

		/**
		 * @brief Used to transfer exceptions from the instrument thread to the main thread.
		 * Stores the exception responsible for the error state if the instrument which owns
		 * the respective @p InstrumentDataBase's instance is in such an error state.
		*/
		std::exception_ptr InstrumentException;
	};

	/**
	 * @brief Parameter class for @p InstrumentBase
	*/
	class InstrumentParamsBase : public RunnableObjectParams
	{
		friend class InstrumentBase;

		template <typename>
		friend InstrumentPtrType MakeInstrument(const std::thread::id, ParamsBasePtrType&&);

	public:
		/**
		 * @brief Constructs the parameters for a @p InstrumentBase instance.
		 * @copydetails ParamsBase::ParamsBase
		*/
		InstrumentParamsBase(ItemIDType ID, const DynExpCore& Core) : RunnableObjectParams(ID, Core) {}

		virtual ~InstrumentParamsBase() = 0;

		virtual const char* GetParamClassTag() const noexcept override { return "InstrumentParamsBase"; }

	private:
		void ConfigureParamsImpl(dispatch_tag<RunnableObjectParams>) override final { ConfigureParamsImpl(dispatch_tag<InstrumentParamsBase>()); }
		virtual void ConfigureParamsImpl(dispatch_tag<InstrumentParamsBase>) {}		//!< @copydoc ConfigureParamsImpl(dispatch_tag<DynExp::RunnableObjectParams>)

		/**
		 * @brief Just used temporarily during the construction of an instrument.
		 * Refer to @p MakeInstrument() and to InstrumentBase::InstrumentBase().
		*/
		std::unique_ptr<InstrumentDataBase> InstrumentData;

		DummyParam Dummy = { *this };												//!< @copydoc DynExp::ParamsBase::DummyParam
	};

	/**
	 * @brief Configurator class for @p InstrumentBase
	*/
	class InstrumentConfiguratorBase : public RunnableObjectConfigurator
	{
	public:
		using ObjectType = InstrumentBase;
		using ParamsType = InstrumentParamsBase;

		InstrumentConfiguratorBase() = default;
		virtual ~InstrumentConfiguratorBase() = 0;
	};

	/**
	 * @brief Base class for instruments. Instruments comprise virtual devices (meta instruments)
	 * and physial devices (instruments). While meta instruments are used by modules (@p ModuleBase)
	 * as an abstraction layer, physical instruments derive from meta instruments and make usually use
	 * of one hardware adapter (@p HardwareAdapterBase) to communicate with the underlying hardware. 
	*/
	class InstrumentBase : public RunnableObject
	{
		/**
		 * @brief Allow exclusive access to some of @p InstrumentBase's private methods to the instrument thread
		 * @p InstrumentThreadMain().
		*/
		class InstrumenThreadOnlyType
		{
			friend class InstrumentBase;
			friend int InstrumentThreadMain(InstrumentInstance, InstrumentBase* const);

			/**
			 * @brief Construcs an instance - one for each @p InstrumentBase instance
			 * @param Parent Owning @p InstrumentBase instance
			*/
			constexpr InstrumenThreadOnlyType(InstrumentBase& Parent) noexcept : Parent(Parent) {}

			auto HandleTask(InstrumentInstance& Instance) { return Parent.HandleTask(Instance); }			//!< @copydoc InstrumentBase::HandleTask
			void UpdateData() { Parent.UpdateDataInternal(); }												//!< @copydoc InstrumentBase::UpdateDataInternal
			void OnError() { Parent.OnError(); }															//!< @copydoc InstrumentBase::OnError
			void SetInitialized() { Parent.Initialized = true; }											//!< Sets InstrumentBase::Initialized to true.

			InstrumentBase& Parent;		//!< Owning @p InstrumentBase instance
		};

	public:
		/**
		 * @brief Indicates how an instrument should proceed after handling a task.
		*/
		enum class TaskHandlingContinuationType { Continue, Terminate, Defer };
		/**
		 * @var InstrumentBase::TaskHandlingContinuationType InstrumentBase::Continue
		 * Task handling should continue, the instrument does not terminate.
		*/
		/**
		 * @var InstrumentBase::TaskHandlingContinuationType InstrumentBase::Terminate
		 * Task handling should not continue, the instrument should terminate.
		*/
		/**
		 * @var InstrumentBase::TaskHandlingContinuationType InstrumentBase::Defer
		 * Task handling should continue. However, a deferred task currently blocks the
		 * instrument queue. Hence, do not enqueue update tasks.
		*/

		using ParamsType = InstrumentParamsBase;															//!< @copydoc Object::ParamsType
		using ConfigType = InstrumentConfiguratorBase;														//!< @copydoc Object::ConfigType
		
		/**
		 * @brief Type of the data class belonging to this @p InstrumentBase type. Declare this alias in every
		 * derived class with the respective data class accompanying the derived @p InstrumentBase.
		*/
		using InstrumentDataType = InstrumentDataBase;

		/**
		 * @brief Alias for the return type of InstrumentBase::GetInstrumentData(). Data class instances
		 * wrapped into Util::SynchronizedPointer can be accessed in a thread-safe way.
		*/
		using InstrumentDataTypeSyncPtrType = Util::SynchronizedPointer<InstrumentDataType>;

		/**
		 * @brief Alias for the return type of InstrumentBase::GetInstrumentData() const. Data class 
		 * instances wrapped into Util::SynchronizedPointer can be accessed in a thread-safe way.
		*/
		using InstrumentDataTypeSyncPtrConstType = Util::SynchronizedPointer<const InstrumentDataType>;

		
		/** @name gRPC aliases
		 * Redefine in derived meta instrument classes to use them in DynExpInstr::gRPCInstrument.
		*/
		///@{
		using InitTaskType = InitTaskBase;			//!< @copydoc InitTaskBase
		using ExitTaskType = ExitTaskBase;			//!< @copydoc ExitTaskBase
		using UpdateTaskType = UpdateTaskBase;		//!< @copydoc UpdateTaskBase
		///@}

		/**
		 * @brief Every derived class has to redefine this function.
		 * @return Returns the category of this instrument type.
		*/
		constexpr static auto Category() noexcept { return "General"; }

		/**
		 * @brief Constructs an instrument instance.
		 * @copydetails DynExp::Object::Object
		 * @throws Util::InvalidArgException is thrown if InstrumentParamsBase::InstrumentData of @p Params is nullptr.
		*/
		InstrumentBase(const std::thread::id OwnerThreadID, ParamsBasePtrType&& Params);

		virtual ~InstrumentBase() = 0;

		virtual std::string GetCategory() const override { return Category(); }

		InstrumenThreadOnlyType InstrumentThreadOnly;		//!< @copydoc InstrumenThreadOnlyType

		/** @name Override
		 * Override by derived classes.
		*/
		///@{
		/**
		 * @brief Specifies in which time intervals the instrument's task queue runs to handle pending tasks.
		 * @return Delay time in between task queue executions. Return @p std::chrono::milliseconds::max() to
		 * make the instrument thread only wake up (to handle tasks and to update data) when a task is enqueued.
		 * Return 0 to make the task queue delay as small as as possible.
		*/
		virtual std::chrono::milliseconds GetTaskQueueDelay() const { return std::chrono::milliseconds(std::chrono::milliseconds::max()); }
		///@}

		/**
		 * @brief Determines the default timeout for @p GetInstrumentData() to lock the mutex synchronizing
		 * the instrument's data #InstrumentData.
		*/
		static constexpr auto GetInstrumentDataTimeoutDefault = std::chrono::milliseconds(1000);

		/** @name Thread-safe public functions
		 * Methods can be called from any thread.
		*/
		///@{
		/**
		 * @brief Locks the mutex of the instrument data class instance #InstrumentData assigned to this @p InstrumentBase
		 * instance and returns a pointer to the locked #InstrumentData. Instrument data should not be locked by having called
		 * this function while subsequently calling a derived instrument's method which also makes use of the instrument's data
		 * by locking it. If this happens (e.g. in a module thread), the instrument data's mutex is locked recursively. In principle,
		 * this does no harm since Util::ISynchronizedPointerLockable supports that. But, it is not considered good practice.
		 * @param Timeout Time to wait for locking the mutex of #InstrumentData.
		 * @return Returns a pointer to @p InstrumentDataType (non-const) to allow access all of its members.
		*/
		InstrumentDataTypeSyncPtrType GetInstrumentData(const std::chrono::milliseconds Timeout = GetInstrumentDataTimeoutDefault);

		/**
		 * @copybrief GetInstrumentData(const std::chrono::milliseconds)
		 * @param Timeout Time to wait for locking the mutex of #InstrumentData.
		 * @return Returns a pointer to const @p InstrumentDataType, since users of this @p InstrumentBase instance are not
		 * allowed to access non-const members of @p InstrumentDataType. These are only to be accessed by the main thread!
		*/
		InstrumentDataTypeSyncPtrConstType GetInstrumentData(const std::chrono::milliseconds Timeout = GetInstrumentDataTimeoutDefault) const;

		/**
		 * @brief Invoking an instance of this alias is supposed to call InstrumentBase::GetInstrumentData() of the
		 * instance the Util::CallableMemberWrapper has been constructed with.
		*/
		using InstrumentDataGetterType = Util::CallableMemberWrapper<InstrumentBase,
			InstrumentDataTypeSyncPtrType (InstrumentBase::*)(const std::chrono::milliseconds)>;

		/**
		 * @brief Enqueues an update task (instance of class @p UpdateTaskBase).
		*/
		void UpdateData() const;

		/**
		 * @brief Enqueues a task which arrives at a latch when executed (instance of class @p ArriveAtLatchTask).
		 * @param Latch Latch to arrive at.
		*/
		void EnqueueArriveAtLatchTask(std::latch& Latch) const;

		/**
		 * @brief Getter for #Initialized
		 * @return Returns true when the instrument is initialized and ready to handle tasks, false othweise.
		*/
		bool IsInitialized() const { return Initialized; }
		///@}

	private:
		/** @name Private functions for logical const-ness
		 * Logical const-ness: refer to @p MakeAndEnqueueTask().
		*/
		///@{
		/**
		 * @brief Always allows @p InstrumentBase to obtain a non-const pointer to the instrument's data -
		 * even in const task functions.
		 * @param Timeout Time to wait for locking the mutex of #InstrumentData.
		 * @return Returns a pointer to @p InstrumentDataType (non-const) to allow access all of its members.
		*/
		InstrumentDataTypeSyncPtrType GetNonConstInstrumentData(const std::chrono::milliseconds Timeout = GetInstrumentDataTimeoutDefault) const;
		///@}

	protected:
		/**
		 * @copydoc InstrumentDataBase::GetException
		 * @param InstrumentDataPtr Reference to a pointer to the locked instrument data
		*/
		static auto GetExceptionUnsafe(const InstrumentDataTypeSyncPtrConstType& InstrumentDataPtr) { return InstrumentDataPtr->GetException(); }

		/**
		 * @brief Calls @p MakeTask() to construct a new task and subsequently enqueues the task into the instrument's
		 * task queue.
		 * Logical const-ness: this is a const member function to allow pointers to @p const @p InstrumentBase inserting
		 * tasks into the instrument's task queue. These kind of pointers are e.g. returned by
		 * RunnableInstance::GetOwner() which can be called by tasks' TaskBase::RunChild() functions. For
		 * @p const @p InstrumentBase*, it is possible to insert tasks into the task queue, but not to
		 * change the @p InstrumentBase object itself (e.g. calling Object::Reset()).
		 * @tparam TaskT Type of a task derived from class @p TaskBase
		 * @tparam ...ArgTs Types of the arguments to forward to the task's constructor
		 * @param ...Args Arguments to forward to the task's constructor
		*/
		template <typename TaskT, typename... ArgTs>
		void MakeAndEnqueueTask(ArgTs&& ...Args) const
		{
			auto Task = MakeTask<TaskT>(std::forward<ArgTs>(Args)...);

			// Locks InstrumentData
			GetNonConstInstrumentData()->InstrumentBaseOnly.EnqueueTask(std::move(Task), IsCallFromRunnableThread(), true);
		}

	public:
		/**
		 * @brief Calls a (derived) instrument's function which inserts a task into the instrument's task queue
		 * synchronously. This means that @p AsSyncTask() blocks until the task has been executed or aborted.
		 * This is achieved by passing a callback function to the task which in turn sets a flag after the task
		 * execution. The thread calling @p AsSyncTask() blocks (via @p std::this_thread::yield()) until this flag is set.
		 * @tparam DerivedInstrT Instrument type (derived from class @p InstrumentBase).
		 * @tparam ...TaskFuncArgTs Types the task inserting function expects as arguments.
		 * @tparam ...ArgTs Types of the arguments passed to @p AsSyncTask().
		 * @param TaskFunc Member function pointer to (derived) instrument's task-inserting function.
		 * @param ...Args  Arguments to be perfectly forwarded to the task function.
		 * @return Return the exception possibly thrown by the task.
		*/
		template <typename DerivedInstrT, typename... TaskFuncArgTs, typename... ArgTs>
		ExceptionContainer AsSyncTask(void (DerivedInstrT::* TaskFunc)(TaskFuncArgTs...) const, ArgTs&& ...Args) const
		{
			std::atomic<bool> FinishedFlag = false;
			ExceptionContainer Exception;
			auto CallbackFunc = [&FinishedFlag, &Exception](const TaskBase& Task, auto E) {
				Exception = E;

				// Must come last!
				FinishedFlag = true;
			};

			(dynamic_cast<const DerivedInstrT&>(*this).*TaskFunc)(std::forward<ArgTs>(Args)..., CallbackFunc);

			while (!FinishedFlag)
				std::this_thread::yield();

			return Exception;
		}

	private:
		/** @name Instrument thread only
		 * These functions must be called by instrument thread only.
		*/
		///@{
		/**
		 * @brief Executes and removes the next pending task from the instrument's task queue.
		 * @param Instance Handle to the instrument thread's data
		 * @return Returns how the instrument should proceed after handling a task at front of the
		 * task queue. Always indicates @p InstrumentBase::TaskHandlingContinuationType::Continue if there
		 * is no task to be handled.
		*/
		TaskHandlingContinuationType HandleTask(InstrumentInstance& Instance);

		/**
		 * @brief Inserts an update task (@p UpdateTaskBase) into the instrument's task queue.
		 * Override @p UpdateAdditionalData() to adjust behavior.
		*/
		void UpdateDataInternal();

		/**
		 * @brief Derived classes can perform critical shutdown actions after an error has occurred.
		 * Override @p OnErrorChild() to adjust behavior.
		*/
		void OnError();
		///@}

		void ResetImpl(dispatch_tag<RunnableObject>) override final;
		virtual void ResetImpl(dispatch_tag<InstrumentBase>) = 0;		//!< @copydoc ResetImpl(dispatch_tag<DynExp::RunnableObject>)

		/** @name Main thread only
		 * These functions must not be called by instrument thread.
		*/
		///@{
		void RunChild() override final;
		void NotifyChild() override final;
		void TerminateChild(const std::chrono::milliseconds Timeout) override final;

		/**
		 * @brief This function enables derived classes to enqueue tasks to be executed directly
		 * before the final exit task (@p ExitTaskBase).
		 * Override @p OnPrepareExitChild() to adjust behavior.
		*/
		void OnPrepareExit();
		///@}

		std::exception_ptr GetExceptionChild([[maybe_unused]] const std::chrono::milliseconds Timeout) const override final;
		bool IsReadyChild() const override final;

		/** @name Override
		 * Override by derived classes.
		*/
		///@{
		virtual void OnErrorChild() const {}							//!< @copydoc OnError
		virtual void OnPrepareExitChild() const {}						//!< @copydoc OnPrepareExit

		/**
		 * @brief Determines whether task handling should continue.
		 * @return Returns false if task handling (the instrument) should stop, true otherwise.
		*/
		virtual bool HandleAdditionalTask() { return true; }

		/**
		 * @brief Determines whether to enqueue update tasks (@p UpdateTaskBase).
		 * @return Returns true if update tasks should be appended to the instrument's
		 * task queue periodically after handling all pending tasks, false otherwise.
		*/
		virtual bool UpdateAdditionalData() { return true; }

		/**
		 * @brief Factory function for an init task (@p InitTaskBase). Override to define the
		 * desired initialization task in derived classes only if the respective task really
		 * does something and if it has no pure virtual function.
		 * @return Pointer to the task (transferring ownership) or nullptr if no init task
		 * is required. A task can be created with @p MakeTask().
		*/
		virtual std::unique_ptr<InitTaskBase> MakeInitTask() const { return nullptr; }

		/**
		 * @brief Factory function for an exit task (@p ExitTaskBase). Override to define the
		 * desired deinitialization task in derived classes only if the respective task really
		 * does something and if it has no pure virtual function.
		 * @return Pointer to the task (transferring ownership) or nullptr if no exit task
		 * is required. A task can be created with @p MakeTask().
		*/
		virtual std::unique_ptr<ExitTaskBase> MakeExitTask() const { return nullptr; }

		/**
		 * @brief Factory function for an update task (@p UpdateTaskBase). Override to define the
		 * desired update task in derived classes only if the respective task really
		 * does something and if it has no pure virtual function.
		 * @return Pointer to the task (transferring ownership) or nullptr if no update task
		 * is required. A task can be created with @p MakeTask().
		*/
		virtual std::unique_ptr<UpdateTaskBase> MakeUpdateTask() const { return nullptr; }
		///@}

		const std::unique_ptr<InstrumentDataType> InstrumentData;	//!< Instrument data belonging to this @p InstrumentBase instance.
		std::atomic<bool> Initialized = false;						//!< Determines whether the init task (@p InitTaskBase) has run.
	};

	/**
	 * @brief Defines data for a thread belonging to a @p InstrumentBase instance.
	 * Refer to @p RunnableInstance.
	*/
	class InstrumentInstance : public RunnableInstance
	{
	public:
		/**
		 * @copydoc RunnableInstance::RunnableInstance(RunnableObject&, std::promise<void>&&)
		 * @param InstrumentDataGetter Getter for instrument's data. Refer to InstrumentBase::InstrumentDataGetterType.
		*/
		InstrumentInstance(InstrumentBase& Owner, std::promise<void>&& ThreadExitedPromise,
			const InstrumentBase::InstrumentDataGetterType InstrumentDataGetter);

		/**
		 * @copydoc RunnableInstance::RunnableInstance(RunnableInstance&&)
		*/
		InstrumentInstance(InstrumentInstance&& Other);

		~InstrumentInstance() = default;

		/**
		 * @brief Getter for instrument's data. Refer to InstrumentBase::InstrumentDataGetterType.
		*/
		const InstrumentBase::InstrumentDataGetterType InstrumentDataGetter;
	};

	/**
	 * @brief Casts the data base class @p From into a derived @p InstrumentBase's (@p To) data
	 * class keeping the data locked by Util::SynchronizedPointer for thread-safe casting.
	 * @tparam To Type derived from @p InstrumentBase into whose data class type to cast.
	 * @p const is added to @p To if @p From is @p const.
	 * @tparam From Data class type to cast from (must be @p InstrumentDataBase).
	 * This type is automatically deduced.
	 * @param InstrumentDataPtr Locked data class to cast from. @p InstrumentDataPtr is empty
	 * after the cast.
	 * @return Locked data class cast to the data class belonging to @p To.
	 * @throws Util::InvalidArgException is thrown if @p InstrumentDataPtr is empty.
	 * @throws Util::TypeErrorException is thrown if the cast fails.
	*/
	template <typename To, typename From, std::enable_if_t<
		std::is_same_v<InstrumentDataBase, std::remove_cv_t<From>>, int> = 0
	>
	auto dynamic_InstrumentData_cast(Util::SynchronizedPointer<From>&& InstrumentDataPtr)
	{
		if (!InstrumentDataPtr)
			throw Util::InvalidArgException("InstrumentDataPtr must not be nullptr.");

		return Util::SynchronizedPointer<
			std::conditional_t<std::is_const_v<From>, std::add_const_t<typename To::InstrumentDataType>, typename To::InstrumentDataType>
		>(std::move(InstrumentDataPtr));
	}

	/**
	 * @brief Defines the return type of task functions.
	*/
	class TaskResultType
	{
	public:
		/**
		 * @brief Determines whether an instrument should terminate after handling a task.
		*/
		enum class ContinuationType : bool {
			Continue,	//!< Task handling should continue, the instrument does not terminate.
			Terminate	//!< Task handling should not continue, the instrument should terminate.
		};

		/**
		 * @brief Determines whether a task has been aborted.
		*/
		enum class AbortedType : bool {
			NotAborted,	//!< The task has not been aborted.
			Aborted		//!< The task has been aborted.
		};

		/**
		 * @brief Constructs a @p TaskResultType instance.
		 * @param Continue @copybrief ContinuationType
		 * @param Aborted @copybrief AbortedType
		 * @param ErrorCode @copybrief #ErrorCode
		*/
		constexpr TaskResultType(const ContinuationType Continue = ContinuationType::Continue, const AbortedType Aborted = AbortedType::NotAborted,
			const int ErrorCode = 0) noexcept
			: Continue(Continue), Aborted(Aborted), ErrorCode(ErrorCode) {}

		/**
		 * @brief Determines whether the instrument having handled this task should continue or terminate.
		 * @return Returns true if the instrument should continue handling other tasks or false if the instrument should terminate.
		*/
		constexpr bool ShouldContinue() const noexcept { return Continue == ContinuationType::Continue; }

		/**
		 * @brief Converts #Continue to @p InstrumentBase::TaskHandlingContinuationType.
		 * @return Returns an instance of @p InstrumentBase::TaskHandlingContinuationType.
		*/
		constexpr InstrumentBase::TaskHandlingContinuationType ToTaskHandlingContinuationType() const noexcept;

		/**
		 * @brief Determines whether this task has been aborted.
		 * @return Returns true if the task has been aborted, false otherwise.
		*/
		constexpr bool HasAborted() const noexcept { return Aborted == AbortedType::Aborted; }

		/**
		 * @brief Getter for the error code of an error which occurred during execution of the task function.
		 * @return Returns #ErrorCode.
		*/
		constexpr int GetErrorCode() const noexcept { return ErrorCode; }

	private:
		const ContinuationType Continue;	//!< @copybrief ContinuationType
		const AbortedType Aborted;			//!< @copybrief AbortedType
		const int ErrorCode;				//!< %DynExp error code from DynExpErrorCodes::DynExpErrorCodes. Anything else than 0 indicates an error.
	};

	/**
	 * @brief Base class for all tasks being processed by instruments.
	 * The class must not contain public virtual functions since TaskBase::CallbackFunc could call
	 * them in @p TaskBase's destructor.
	*/
	class TaskBase
	{
		/**
		 * @brief Allow exclusive access to some of @p TaskBase's private methods to @p InstrumentBase.
		*/
		class InstrumentBaseOnlyType
		{
			friend class TaskBase;
			friend class InstrumentBase;

			/**
			 * @brief Construcs an instance - one for each @p TaskBase instance
			 * @param Parent Owning @p TaskBase instance
			*/
			constexpr InstrumentBaseOnlyType(TaskBase& Parent) noexcept : Parent(Parent) {}

			void Lock() { Parent.Lock(); }													//!< @copydoc TaskBase::Lock
			auto Run(InstrumentInstance& Instance) { return Parent.Run(Instance); }			//!< @copydoc TaskBase::Run
			void SetAborted() { Parent.State = TaskState::Aborted; }						//!< Sets @p TaskBase::State to @p TaskBase::TaskState::Aborted.

			TaskBase& Parent;		//!< Owning @p TaskBase instance
		};

		/**
		 * @brief Allow exclusive access to some of @p TaskBase's private methods to @p InstrumentDataBase.
		*/
		class InstrumentDataBaseOnlyType
		{
			friend class TaskBase;
			friend class InstrumentDataBase;

			/**
			 * @brief Construcs an instance - one for each @p TaskBase instance
			 * @param Parent Owning @p TaskBase instance
			*/
			constexpr InstrumentDataBaseOnlyType(TaskBase& Parent) noexcept : Parent(Parent) {}

			bool KeepFinishedTask() const noexcept { return Parent.KeepFinishedTask(); }	//!< @copydoc TaskBase::KeepFinishedTask

			TaskBase& Parent;		//!< Owning @p TaskBase instance
		};

	public:
		/**
		 * @brief Type of a callback function which is invoked when a task has finished,
		 * failed or has been aborted. The function receives a reference to the task it
		 * originates from as well as a reference to a wrapper holding an exception
		 * which might have occurred while executing the task.
		*/
		using CallbackType = std::function<void(const TaskBase&, ExceptionContainer&)>;

		/**
		 * @brief Defines states an instrument's task can undergo.
		 * Possible state transitions are:
		 *	-  Waiting -> Locked/Running/Aborted
		 *	-  Locked -> Running
		 *	-  Running -> Finished/Failed/Aborted
		*/
		enum class TaskState { Waiting, Locked, Running, Finished, Failed, Aborted };
		/**
		 * @var TaskBase::TaskState TaskBase::Waiting
		 * The task is enqueued and waiting for execution. This is the starting state after construction.
		*/
		/**
		 * @var TaskBase::TaskState TaskBase::Locked
		 * The task is locked since its execution is being prepared. It will be the next task to be executed.
		*/
		/**
		 * @var TaskBase::TaskState TaskBase::Running
		 * The task is currently being executed.
		*/
		/**
		 * @var TaskBase::TaskState TaskBase::Finished
		 * The task execution has finished. No error has occurred.
		*/
		/**
		 * @var TaskBase::TaskState TaskBase::Failed
		 * The task execution has failed due to an exception leaving @p RunChild().
		*/
		/**
		 * @var TaskBase::TaskState TaskBase::Aborted
		 * The task has been aborted either by @p ~TaskBase() or by returning a @p TaskResultType instance with
		 * TaskResultType::Aborted set to TaskResultType::AbortedType::Aborted from @p RunChild().
		*/

		/**
		 * @brief Constructs an instrument task.
		 * @param CallbackFunc @copybrief #CallbackFunc
		 * @param DeferUntil @copybrief #DeferUntil
		 * If the task does not support being deferred, the @p DeferUntil parameter is not available in the task's constructor.
		*/
		TaskBase(CallbackType CallbackFunc = nullptr, std::chrono::system_clock::time_point DeferUntil = {}) noexcept
			: InstrumentBaseOnly(*this), InstrumentDataBaseOnly(*this),
			CallbackFunc(std::move(CallbackFunc)), DeferUntil(DeferUntil),
			State(TaskState::Waiting), ErrorCode(0), ShouldAbort(false) {}

		/**
		 * @brief The destructor aborts a waiting task setting #State to TaskState::Aborted. Then, it
		 * calls #CallbackFunc with a default-constructed @p ExceptionContainer instance.
		*/
		virtual ~TaskBase() = 0;

		/** @name Thread-safe public functions
		 * Methods can be called from any thread. Only atomic operations are performed.
		*/
		///@{
		/**
		 * @brief Getter for the instrument task's earliest execution time point.
		 * @return Returns #DeferUntil.
		*/
		auto GetDeferUntil() const noexcept { return DeferUntil; }

		/**
		 * @brief Getter for the instrument task's current state.
		 * @return Returns #State.
		*/
		TaskState GetState() const noexcept { return State; }

		/**
		 * @brief Determines whether the task is locked.
		 * @return Returns true if the task is in the state TaskState::Locked or TaskState::Running, false otherwise.
		*/
		bool IsLocked() const noexcept;

		/**
		 * @brief Determines whether the task should abort. A derived task is encouraged to call this method before
		 * performing any action or in between the individual steps of a more complex action.
		 * @return Returns #ShouldAbort.
		*/
		bool IsAborting() const noexcept { return ShouldAbort; }

		/**
		 * @brief Getter for the error code related to an error possibly occurred while the task was executed.
		 * @return Returns #ErrorCode.
		*/
		int GetErrorCode() const noexcept { return ErrorCode; }

		/**
		 * @brief Requests the task to abort. There is no guarantee that the derived task does call
		 * @p IsAborting() to check whether it should abort. The task could have finished already or
		 * the implementation of @p RunChild() in the derived class does not allow for aborting.
		*/
		void Abort() { ShouldAbort = true; }
		///@}

		InstrumentBaseOnlyType InstrumentBaseOnly;							//!< @copydoc InstrumentBaseOnlyType
		InstrumentDataBaseOnlyType InstrumentDataBaseOnly;					//!< @copydoc InstrumentDataBaseOnlyType
		
	private:
		/**
		 * @brief Locks the task to prepare it for execution.
		 * @throws Util::InvalidStateException is thrown if the task is not in the TaskState::Waiting state.
		*/
		void Lock();

		/**
		 * @brief Runs the task. Override @p RunChild() to define a derived task's action(s).
		 * Any exception leaving @p RunChild() will terminate the instrument thread. In this case, the
		 * instrument has to be reset in order to restart it. For minor errors occurring in @p RunChild(),
		 * return a @p TaskResultType instance with TaskResultType::ErrorCode set to an appropriate
		 * (non-zero) value. This sets #State to TaskState::Failed. Note that in this case
		 * there might be no way to retrieve the error if finished tasks are not kept (i.e.
		 * @p KeepFinishedTask() returns false) and if no #CallbackFunc has been set.
		 * @p RunChild() is supposed to check the return value of @p IsAborting().
		 * @param Instance Handle to the instrument thread's data
		 * @return Returns whether task handling should continue or whether the instrument thread should terminate.
		 * @throws Util::InvalidStateException is thrown if the task is not in the TaskState::Waiting
		 * or TaskState::Locked state.
		*/
		InstrumentBase::TaskHandlingContinuationType Run(InstrumentInstance& Instance);

		/** @name Override
		 * Override by derived classes.
		*/
		///@{
		virtual TaskResultType RunChild(InstrumentInstance& Instance) = 0;	//!< @copydoc Run

		/**
		 * @brief Determines whether the task should be kept to check its results after execution.
		 * InstrumentDataBase::RemoveTaskFromQueue().
		 * @return Return true to append the task to InstrumentDataBase::FinishedTasks after execution.
		 * Return false to destroy the task directly after execution.
		*/
		virtual bool KeepFinishedTask() const noexcept { return false; }
		///@}

		/**
		 * @brief This callback function is called after the task has finished (either successfully or not)
		 * with a reference to the current task and with a reference to the exception which occurred during
		 * the task execution (if an exception has occurred).
		*/
		const CallbackType CallbackFunc;

		/**
		 * @brief The execution of this task is deferred until the specified point in time is reached if
		 * @p time_since_epoch() of #DeferUntil is non-zero. Tasks being deferred block the task queue of the
		 * instrument they are assigned to, i.e., other enqueued tasks do not run before this task to maintain
		 * the task order.
		*/
		const std::chrono::system_clock::time_point DeferUntil;

		/** @name Instrument-to-other communication
		 * These variables are for communication from the instrument thread to other thread(s) only.
		*/
		///@{
		std::atomic<TaskState> State;		//!< Holds the task's current state. Refer to TaskBase::TaskState.
		std::atomic<int> ErrorCode;			//!< Holds the error code of an error which occurred during execution of the task function.
		///@}

		/** @name Other-to-instrument communication
		 * These variables are for communication from other thread(s) to the instrument thread only.
		*/
		///@{
		std::atomic<bool> ShouldAbort;		//!< Indicates whether the task should abort. Refer to @p Abort() and @p IsAborting().
		///@}
	};

	/**
	 * @brief Default task which does not do anything. Though, calling it ensures that TaskBase::CallbackFunc
	 * gets called. This is required to avoid InstrumentBase::AsSyncTask() getting stuck in an infinite loop.
	 * All functions overridden from meta instruments, which are expected to enqueue a task, must at least
	 * enqueue a @p DefaultTask (by calling @p MakeAndEnqueueTask< DynExp::DefaultTask >(CallbackFunc);)
	*/
	class DefaultTask final : public TaskBase
	{
	public:
		DefaultTask(CallbackType CallbackFunc) noexcept : TaskBase(CallbackFunc) {}				//!< @copydoc TaskBase::TaskBase

	private:
		virtual TaskResultType RunChild(InstrumentInstance& Instance) override { return {}; }
	};

	/**
	 * @brief Defines a task for initializing an instrument within an instrument inheritance hierarchy.
	 * Each instrument (indirectly) derived from class @p InstrumentBase must be accompanied by an
	 * initialization task class derived from @p InitTaskBase. Even if the task does not do anything,
	 * at least it has to call InitTaskBase::InitFuncImpl() of the derived instrument's initialization
	 * task class.
	*/
	class InitTaskBase : public TaskBase
	{
	protected:
		/**
		 * @brief Refer to DynExp::ParamsBase::dispatch_tag.
		 * @tparam Type (derived from DynExp::InitTaskBase) to instantiate the dispatch tag template with.
		*/
		template <typename>
		struct dispatch_tag {};

	private:
		TaskResultType RunChild(InstrumentInstance& Instance) override final;

		/**
		 * @brief Initializes the respective instrument within the instrument inheritance hierarchy.
		 * Call @p InitFuncImpl() of a derived instrument's initialization task as the last command in a base
		 * instrument's initialization task @p InitFuncImpl() to ensure that derived instruments are initialized
		 * after base instruments.
		 * Overriding functions must throw an exception in case of an error.
		 * @param Instance Handle to the instrument thread's data
		*/
		virtual void InitFuncImpl(dispatch_tag<InitTaskBase>, InstrumentInstance& Instance) = 0;
	};

	/**
	 * @brief Defines a task for deinitializing an instrument within an instrument inheritance hierarchy.
	 * Each instrument (indirectly) derived from class @p InstrumentBase must be accompanied by a
	 * deinitialization task class derived from @p ExitTaskBase. Even if the task does not do anything,
	 * at least it has to call ExitTaskBase::ExitFuncImpl() of the derived instrument's deinitialization
	 * task class.
	*/
	class ExitTaskBase : public TaskBase
	{
	protected:
		/**
		 * @brief Refer to DynExp::ParamsBase::dispatch_tag.
		 * @tparam Type (derived from DynExp::ExitTaskBase) to instantiate the dispatch tag template with.
		*/
		template <typename>
		struct dispatch_tag {};

	private:
		TaskResultType RunChild(InstrumentInstance& Instance) override final;

		/**
		 * @brief Deinitializes the respective instrument within the instrument inheritance hierarchy.
		 * Call @p ExitFuncImpl() of a derived instrument's exit task as the first command in a base
		 * instrument's exit task @p ExitFuncImpl() to ensure that derived instruments can shut down
		 * before base instruments.
		 * Overriding functions must throw an exception in case of an error.
		 * @param Instance Handle to the instrument thread's data
		*/
		virtual void ExitFuncImpl(dispatch_tag<ExitTaskBase>, InstrumentInstance& Instance) = 0;
	};

	/**
	 * @brief Defines a task for updating an instrument within an instrument inheritance hierarchy.
	 * Each instrument (indirectly) derived from class @p InstrumentBase must be accompanied by an
	 * update task class derived from @p UpdateTaskBase. Even if the task does not do anything,
	 * at least it has to call UpdateTaskBase::UpdateFuncImpl() of the derived instrument's update
	 * task class.
	*/
	class UpdateTaskBase : public TaskBase
	{
	protected:
		/**
		 * @brief Refer to DynExp::ParamsBase::dispatch_tag.
		 * @tparam Type (derived from DynExp::UpdateTaskBase) to instantiate the dispatch tag template with.
		*/
		template <typename>
		struct dispatch_tag {};

	private:
		TaskResultType RunChild(InstrumentInstance& Instance) override final;

		/**
		 * @brief Updates the respective instrument within the instrument inheritance hierarchy.
		 * Call @p UpdateFuncImpl() of a derived instrument's update task as the last command in a base
		 * instrument's update task @p UpdateFuncImpl() to ensure that derived instruments are updated
		 * after base instruments.
		 * Overriding functions must throw an exception in case of an error.
		 * @param Instance Handle to the instrument thread's data
		*/
		virtual void UpdateFuncImpl(dispatch_tag<UpdateTaskBase>, InstrumentInstance& Instance) = 0;
	};

	/**
	 * @brief Defines a task which arrives at a @p std::latch when it is executed.
	 * This is useful to synchronize multiple instruments and make their execution block
	 * until a set of instruments has arrived at the latch.
	 * Refer to InstrumentBase::EnqueueArriveAtLatchTask() and to @p WaitForInstruments().
	*/
	class ArriveAtLatchTask final : public TaskBase
	{
	public:
		/**
		 * @copydoc TaskBase::TaskBase
		 * @param Latch @copybrief #Latch
		*/
		ArriveAtLatchTask(std::latch& Latch, CallbackType CallbackFunc = nullptr)
			: TaskBase(CallbackFunc), Latch(Latch) {}

		/**
		 * @brief If the task has been aborted or never executed, the destructor arrives
		 * at the latch in order to avoid deadlocks.
		*/
		~ArriveAtLatchTask();

	private:
		virtual TaskResultType RunChild(InstrumentInstance& Instance) override;

		std::latch& Latch;				//!< Latch the task arrives at when it is executed.
		bool HasArrived = false;		//!< Indicates whether the task has already arrived at the latch.
	};

	/**
	 * @brief Blocks until every instrument passed to the function as a reference parameter
	 * has arrived at a synchronization point.
	 * @tparam ...InstrTs Types of the instruments passed to this function.
	 * @param ...Instruments Instruments to synchronize.
	*/
	template <typename... InstrTs>
	void WaitForInstruments(InstrTs&... Instruments)
	{
		static_assert(std::conjunction_v<std::is_base_of<InstrumentBase, InstrTs>...>);
		static_assert(sizeof...(InstrTs) > 0);
		static_assert(sizeof...(InstrTs) <= std::latch::max());

		std::latch Latch(sizeof...(InstrTs));
		std::array<std::reference_wrapper<const InstrumentBase>, sizeof...(InstrTs)> Instrs{ Instruments... };

		for (auto& Instr : Instrs)
			Instr.get().EnqueueArriveAtLatchTask(Latch);

		Latch.wait();
	}
}

/**
 * @brief %DynExp's instrument namespace contains the implementation of %DynExp instruments
 * which extend %DynExp's core functionality in a modular way.
*/
namespace DynExpInstr {};