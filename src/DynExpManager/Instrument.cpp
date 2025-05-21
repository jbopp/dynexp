// This file is part of DynExp.

#include "stdafx.h"
#include "Instrument.h"

namespace DynExp
{
	int InstrumentThreadMain(InstrumentInstance Instance, InstrumentBase* const Instrument)
	{
		bool IsExiting = false;
		bool IsFirstRun = true;
		InstrumentBase::TaskHandlingContinuationType DoContinue = InstrumentBase::TaskHandlingContinuationType::Continue;
		std::chrono::time_point<std::chrono::system_clock> LastUpdate;	// LastUpdate.time_since_epoch() == 0 now.

		try
		{
			auto& NewTaskNotifier = Instrument->GetInstrumentData()->InstrumentThreadOnly.GetNewTaskNotifier();

			while (!IsExiting)
			{
				if (Instrument->IsExiting())
					IsExiting = true;

				// Loop through all pending tasks. Potential change in between loop condition and function call is taken care of by HandleTask()
				while (Instrument->GetInstrumentData()->GetNumEnqueuedTasks())
				{
					DoContinue = Instrument->InstrumentThreadOnly.HandleTask(Instance);
					if (DoContinue != InstrumentBase::TaskHandlingContinuationType::Continue)
					{
						if (DoContinue == InstrumentBase::TaskHandlingContinuationType::Terminate)
							IsExiting = true;

						break;
					}
				}

				if (!IsExiting)
				{
					auto TaskQueueDelay = Instrument->GetTaskQueueDelay();
					auto Now = std::chrono::system_clock::now();

					if (DoContinue != InstrumentBase::TaskHandlingContinuationType::Defer &&
						(Now - LastUpdate >= TaskQueueDelay || TaskQueueDelay == decltype(TaskQueueDelay)::max()))
					{
						Instrument->InstrumentThreadOnly.UpdateData();

						LastUpdate = Now;
						Instrument->GetInstrumentData()->InstrumentThreadOnly.SetLastUpdateTime(Now);
					}

					if (!IsFirstRun)
					{
						if (TaskQueueDelay == decltype(TaskQueueDelay)::max())
							NewTaskNotifier.Wait();
						else if (TaskQueueDelay.count() == 0)
							std::this_thread::yield();
						else
							NewTaskNotifier.Wait(TaskQueueDelay);
					}
				}

				if (IsFirstRun)
				{
					// The initialization task gets enqueued before the instrument thread starts. Hence, it will have been executed at this point.
					Instrument->InstrumentThreadOnly.SetInitialized();
					IsFirstRun = false;
				}
			}
		}
		catch (const Util::Exception& e)
		{
			Util::EventLog().Log("An instrument has been terminated because of the error reported below.", Util::ErrorType::Error);
			Util::EventLog().Log(e);

			// std::abort() is called when (e.g. timeout) exception occurrs while setting the caught exception.
			Instrument->GetInstrumentData()->InstrumentThreadOnly.SetException(std::current_exception());
			Instrument->InstrumentThreadOnly.OnError();

			return e.ErrorCode;
		}
		catch (const std::exception& e)
		{
			Util::EventLog().Log("An instrument has been terminated because of the following error: " + std::string(e.what()), Util::ErrorType::Error);

			// std::abort() is called when (e.g. timeout) exception occurrs while setting the caught exception.
			Instrument->GetInstrumentData()->InstrumentThreadOnly.SetException(std::current_exception());
			Instrument->InstrumentThreadOnly.OnError();

			return Util::DynExpErrorCodes::GeneralError;
		}
		catch (...)
		{
			Util::EventLog().Log("An instrument has been terminated because of an unknown error.", Util::ErrorType::Error);

			// std::abort() is called when (e.g. timeout) exception occurrs while setting the caught exception.
			Instrument->GetInstrumentData()->InstrumentThreadOnly.SetException(std::current_exception());
			Instrument->InstrumentThreadOnly.OnError();

			return Util::DynExpErrorCodes::GeneralError;
		}

		return Util::DynExpErrorCodes::NoError;
	}

	std::unique_ptr<TaskBase> InstrumentDataBase::PopTaskFront()
	{
		if (TaskQueue.empty())
			return nullptr;

		if (TaskQueue.front()->IsLocked())
			throw Util::InvalidStateException("A task cannot be removed from task queue since it is locked (already started running?)");

		auto Task = std::move(TaskQueue.front());
		TaskQueue.pop_front();

		return Task;
	}

	std::unique_ptr<TaskBase> InstrumentDataBase::PopTaskBack()
	{
		if (TaskQueue.empty())
			return nullptr;

		if (TaskQueue.back()->IsLocked())
			throw Util::InvalidStateException("A task cannot be removed from task queue since it is locked (already started running?)");

		auto Task = std::move(TaskQueue.back());
		TaskQueue.pop_back();

		return Task;
	}

	std::unique_ptr<TaskBase> InstrumentDataBase::PopFinishedTask()
	{
		if (FinishedTasks.empty())
			return nullptr;

		auto Task = std::move(FinishedTasks.front());
		FinishedTasks.pop_front();

		return Task;
	}

	void InstrumentDataBase::EnqueueTask(std::unique_ptr<TaskBase>&& Task, bool CallFromInstrThread, bool NotifyReceiver)
	{
		CheckError();
		CheckQueueState(CallFromInstrThread);

		TaskQueue.push_back(std::move(Task));
		if (NotifyReceiver)
			NewTaskNotifier.Notify();
	}

	void InstrumentDataBase::EnqueuePriorityTask(std::unique_ptr<TaskBase>&& Task, bool CallFromInstrThread, bool NotifyReceiver)
	{
		CheckError();
		CheckQueueState(CallFromInstrThread);

		TaskQueue.push_front(std::move(Task));
		if (NotifyReceiver)
			NewTaskNotifier.Notify();
	}

	void InstrumentDataBase::RemoveTaskFromQueue(TaskQueueIteratorType& Task)
	{
		if (Task->get()->InstrumentDataBaseOnly.KeepFinishedTask())
			FinishedTasks.splice(FinishedTasks.cend(), TaskQueue, Task);
		else
			TaskQueue.erase(Task);
	}

	void InstrumentDataBase::RemoveAllTasks()
	{
		TaskQueue.clear();
	}

	void InstrumentDataBase::RemoveAllTasksExceptFront()
	{
		if (TaskQueue.size() < 2)
			return;

		TaskQueue.erase(++TaskQueue.begin(), TaskQueue.end());
	}

	void InstrumentDataBase::Reset()
	{
		QueueClosed = false;
		InstrumentException = nullptr;

		TaskQueue.clear();
		FinishedTasks.clear();

		ResetImpl(dispatch_tag<InstrumentDataBase>());
	}

	void InstrumentDataBase::CheckError() const
	{
		Util::ForwardException(InstrumentException);
	}

	void InstrumentDataBase::CheckQueueState(bool CallFromInstrThread) const
	{
		// Only allow enqueuing a task by other threads if the queue is not closed.
		// Allow always if the instrument itself is enqueuing something (e.g. upon update or exit).
		if (IsQueueClosed() && !CallFromInstrThread)
			throw Util::InvalidStateException("A task cannot be enqueued since the task queue is closed (instrument is shut down?)");
	}

	InstrumentParamsBase::~InstrumentParamsBase()
	{
	}

	InstrumentConfiguratorBase::~InstrumentConfiguratorBase()
	{
	}

	InstrumentBase::InstrumentBase(const std::thread::id OwnerThreadID, ParamsBasePtrType&& Params)
		: RunnableObject(OwnerThreadID, std::move(Params)),
		InstrumentThreadOnly(*this), InstrumentData(std::move(dynamic_Params_cast<InstrumentBase>(GetParams())->InstrumentData))
	{
		if (!InstrumentData)
			throw Util::InvalidArgException("InstrumentData cannot be nullptr.");
	}

	InstrumentBase::~InstrumentBase()
	{
	}

	InstrumentBase::InstrumentDataTypeSyncPtrType InstrumentBase::GetInstrumentData(const std::chrono::milliseconds Timeout)
	{
		return InstrumentDataTypeSyncPtrType(InstrumentData.get(), Timeout);
	}

	InstrumentBase::InstrumentDataTypeSyncPtrConstType InstrumentBase::GetInstrumentData(const std::chrono::milliseconds Timeout) const
	{
		return InstrumentDataTypeSyncPtrConstType(InstrumentData.get(), Timeout);
	}

	InstrumentBase::InstrumentDataTypeSyncPtrType InstrumentBase::GetNonConstInstrumentData(const std::chrono::milliseconds Timeout) const
	{
		return InstrumentDataTypeSyncPtrType(InstrumentData.get(), Timeout);
	}

	void InstrumentBase::UpdateData() const
	{
		auto Task = MakeUpdateTask();
		if (!Task)
			return;

		// Prevent notifying the instrument thread's NewTaskNotifier by update tasks.
		GetNonConstInstrumentData()->InstrumentBaseOnly.EnqueueTask(std::move(Task), IsCallFromRunnableThread(), false);
	}

	void InstrumentBase::EnqueueArriveAtLatchTask(std::latch& Latch) const
	{
		auto Task = MakeTask<ArriveAtLatchTask>(Latch);

		GetNonConstInstrumentData()->EnqueueTask(std::move(Task));
	}

	InstrumentBase::TaskHandlingContinuationType InstrumentBase::HandleTask(InstrumentInstance& Instance)
	{
		EnsureCallFromRunnableThread();

		if (!HandleAdditionalTask())
			return TaskHandlingContinuationType::Terminate;

		InstrumentDataBase::TaskQueueIteratorType Task;

		{
			// Locks InstrumentData
			auto InstrumentDataPtr = GetInstrumentData();

			if (!InstrumentDataPtr->GetNumEnqueuedTasks())
				return TaskHandlingContinuationType::Continue;

			Task = InstrumentDataPtr->GetTaskFront();
			if (Task->get()->IsAborting())
			{
				Task->get()->InstrumentBaseOnly.SetAborted();
				GetInstrumentData()->InstrumentBaseOnly.RemoveTaskFromQueue(Task);

				return TaskHandlingContinuationType::Continue;
			}
			if (Task->get()->GetDeferUntil() > std::chrono::system_clock::now())
				return TaskHandlingContinuationType::Defer;

			Task->get()->InstrumentBaseOnly.Lock();
		} // InstrumentData unlocked here

		auto DoContinue = Task->get()->InstrumentBaseOnly.Run(Instance);
		GetInstrumentData()->InstrumentBaseOnly.RemoveTaskFromQueue(Task);

		return DoContinue;
	}

	void InstrumentBase::UpdateDataInternal()
	{
		EnsureCallFromRunnableThread();

		if (!UpdateAdditionalData())
			return;

		UpdateData();
	}

	void InstrumentBase::OnError()
	{
		EnsureCallFromRunnableThread();

		try
		{
			// Necessary to call tasks' destructors. This is required to avoid deadlocks e.g. by pending ArriveAtLatchTask tasks.
			GetInstrumentData()->InstrumentBaseOnly.RemoveAllTasks();

			OnErrorChild();
		}
		catch (const Util::Exception& e)
		{
			Util::EventLog().Log("Calling an instrument's error handler, the error listed below occurred.", Util::ErrorType::Error);
			Util::EventLog().Log(e);
		}
		catch (const std::exception& e)
		{
			Util::EventLog().Log("Calling an instrument's error handler, the error listed below occurred.", Util::ErrorType::Error);
			Util::EventLog().Log(e.what());
		}
		catch (...)
		{
			Util::EventLog().Log("Calling an instrument's error handler, an unknown error occurred.", Util::ErrorType::Error);
		}
	}

	void InstrumentBase::ResetImpl(dispatch_tag<RunnableObject>)
	{
		// Firstly, reset instrument's data...
		InstrumentData->InstrumentBaseOnly.Reset();
		Initialized = false;

		// ...so that derived instruments can fill the instrument's data again, secondly.
		ResetImpl(dispatch_tag<InstrumentBase>());
	}

	void InstrumentBase::RunChild()
	{
		auto Task = MakeInitTask();
		if (Task)
			InstrumentData->EnqueueTask(std::move(Task));

		StoreThread(std::thread(InstrumentThreadMain, InstrumentInstance(
			*this,
			MakeThreadExitedPromise(),
			{ *this, &InstrumentBase::GetInstrumentData, { InstrumentBase::GetInstrumentDataTimeoutDefault } }
		), this));
	}

	void InstrumentBase::NotifyChild()
	{
		GetInstrumentData()->InstrumentBaseOnly.GetNewTaskNotifier().Notify();
	}

	// Does not call Object::UseCount.AcquireLock() to make sure, no other object is starting to use this instrument
	// while resetting. Object::Reset() or RunnableObject::Terminate() ensure that already.
	void InstrumentBase::TerminateChild(const std::chrono::milliseconds Timeout)
	{
		auto Task = MakeExitTask();

		try
		{
			// Locks InstrumentData.
			// If InstrumentData cannot be locked because it is continuously locked by instrument/module thread, deadlock
			// is avoided since GetInstrumentData throws Util::TimeoutException after timeout in this case.
			auto InstrumentDataPtr = GetInstrumentData();

			if (InstrumentDataPtr->GetNumEnqueuedTasks())
			{
				InstrumentDataPtr->InstrumentBaseOnly.RemoveAllTasksExceptFront();
				InstrumentDataPtr->GetTaskFront()->get()->Abort();
			}

			// Allow to insert tasks which should be executed before the final exit task. Instrument data might be
			// locked recursively here (which is supported).
			if (!InstrumentDataPtr->GetException())
				OnPrepareExit();

			if (Task && !InstrumentDataPtr->GetException())
				InstrumentDataPtr->EnqueueTask(std::move(Task));

			// Do not allow enqueuing further tasks.
			InstrumentDataPtr->InstrumentBaseOnly.CloseQueue();
		} // InstrumentData unlocked here since InstrumentDataPtr gets destroyed.
		catch (const Util::Exception& e)
		{
			// Exception occurring while trying to issue an ExitTask is swallowed and logged. It is tried to terminate
			// the instrument anyway.
			Util::EventLog().Log("An instrument has been terminated whithout cleaning up since the error reported below has occurred while issuing an exit task.",
				Util::ErrorType::Error);
			Util::EventLog().Log(e);
		}
	}

	void InstrumentBase::OnPrepareExit()
	{
		EnsureCallFromOwningThread();

		try
		{
			OnPrepareExitChild();
		}
		catch (const Util::Exception& e)
		{
			Util::EventLog().Log("Calling an instrument's prepare exit handler, the error listed below occurred.", Util::ErrorType::Error);
			Util::EventLog().Log(e);
		}
		catch (const std::exception& e)
		{
			Util::EventLog().Log("Calling an instrument's prepare exit handler, the error listed below occurred.", Util::ErrorType::Error);
			Util::EventLog().Log(e.what());
		}
		catch (...)
		{
			Util::EventLog().Log("Calling an instrument's prepare exit handler, an unknown error occurred.", Util::ErrorType::Error);
		}
	}

	std::exception_ptr InstrumentBase::GetExceptionChild(const std::chrono::milliseconds Timeout) const
	{
		using namespace std::chrono_literals;

		// If InstrumentData cannot be locked because it is continuously locked by instrument/module thread, deadlock
		// is avoided since GetInstrumentData throws Util::TimeoutException after timeout in this case.
		// Short timeout only since main thread should not block.
		return GetExceptionUnsafe(GetInstrumentData(ShortTimeoutDefault));
	}

	bool InstrumentBase::IsReadyChild() const
	{
		using namespace std::chrono_literals;
		auto LockedInstrData = GetInstrumentData(ShortTimeoutDefault);

		auto Exception = GetExceptionUnsafe(LockedInstrData);
		Util::ForwardException(Exception);

		return IsRunning() && !IsExiting() && IsInitialized();
	}

	InstrumentInstance::InstrumentInstance(InstrumentBase& Owner, std::promise<void>&& ThreadExitedPromise,
		const InstrumentBase::InstrumentDataGetterType InstrumentDataGetter)
		: RunnableInstance(Owner, std::move(ThreadExitedPromise)),
		InstrumentDataGetter(InstrumentDataGetter)
	{
	}

	InstrumentInstance::InstrumentInstance(InstrumentInstance&& Other)
		: RunnableInstance(std::move(Other)), InstrumentDataGetter(Other.InstrumentDataGetter)
	{
	}

	constexpr InstrumentBase::TaskHandlingContinuationType TaskResultType::ToTaskHandlingContinuationType() const noexcept
	{
		return Continue == ContinuationType::Continue ?
			InstrumentBase::TaskHandlingContinuationType::Continue : InstrumentBase::TaskHandlingContinuationType::Terminate;
	}

	TaskBase::~TaskBase()
	{
		// Ensure that CallbackFunc gets called in any case.
		if (State == TaskState::Waiting)
		{
			State = TaskState::Aborted;

			try
			{
				if (CallbackFunc)
				{
					// Default-constructed ExceptionContainer does indicate a non-error state.
					ExceptionContainer Exception;
					CallbackFunc(*this, Exception);
				}
			}
			catch (...)
			{
				// Swallow exceptions possibly thrown by CallbackFunc to prevent them leave the destructor.
			}
		}
	}

	bool TaskBase::IsLocked() const noexcept
	{
		// Perform a copy for thread-safety to only load the atomic's state once.
		auto StateCopy = State.load();

		return StateCopy == TaskState::Locked || StateCopy == TaskState::Running;
	}

	void TaskBase::Lock()
	{
		if (State != TaskState::Waiting)
			throw Util::InvalidStateException("An instrument's task cannot be locked since it is not in a pending state.");

		State = TaskState::Locked;
	}

	InstrumentBase::TaskHandlingContinuationType TaskBase::Run(InstrumentInstance& Instance)
	{
		if (State != TaskState::Waiting && State != TaskState::Locked)
			throw Util::InvalidStateException("An instrument's task cannot be started since it is not in a pending or locked state.");

		State = TaskState::Running;

		try
		{
			auto Result = RunChild(Instance);

			State = Result.GetErrorCode() ? TaskState::Failed : (Result.HasAborted() ? TaskState::Aborted : TaskState::Finished);
			ErrorCode = Result.GetErrorCode();

			if (CallbackFunc)
			{
				// Default-constructed ExceptionContainer does indicate a non-error state.
				ExceptionContainer Exception;
				CallbackFunc(*this, Exception);
			}

			return Result.ToTaskHandlingContinuationType();
		}
		catch (...)
		{
			State = TaskState::Failed;
			ErrorCode = Util::DynExpErrorCodes::GeneralError;

			ExceptionContainer Exception(std::current_exception());
			if (CallbackFunc)
			{
				CallbackFunc(*this, Exception);
				if (!Exception.IsError())
					InstrumentBase::TaskHandlingContinuationType::Continue;
			}

			throw;
		}
	}

	TaskResultType InitTaskBase::RunChild(InstrumentInstance& Instance)
	{
		InitFuncImpl(dispatch_tag<InitTaskBase>(), Instance);

#ifdef DYNEXP_DEBUG
		Util::EventLog().Log("Instrument \"" + Instance.ParamsGetter()->ObjectName.Get() + "\" has been initialized.");
#endif // DYNEXP_DEBUG

		return {};
	}

	TaskResultType ExitTaskBase::RunChild(InstrumentInstance& Instance)
	{
		ExitFuncImpl(dispatch_tag<ExitTaskBase>(), Instance);

#ifdef DYNEXP_DEBUG
		Util::EventLog().Log("Instrument \"" + Instance.ParamsGetter()->ObjectName.Get() + "\" has been shut down.");
#endif // DYNEXP_DEBUG
		
		return { TaskResultType::ContinuationType::Terminate };
	}

	TaskResultType UpdateTaskBase::RunChild(InstrumentInstance& Instance)
	{
		UpdateFuncImpl(dispatch_tag<UpdateTaskBase>(), Instance);
		
		return {};
	}

	ArriveAtLatchTask::~ArriveAtLatchTask()
	{
		if (!HasArrived)
			Latch.count_down();
	}

	TaskResultType ArriveAtLatchTask::RunChild(InstrumentInstance& Instance)
	{
		HasArrived = true;
		Latch.count_down();

		return {};
	}
}