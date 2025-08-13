// This file is part of DynExp.

#include "stdafx.h"
#include "InterModuleCommunicator.h"

namespace DynExpInstr
{
	void InterModuleCommunicatorTasks::InitTask::InitFuncImpl(dispatch_tag<DynExp::InitTaskBase>, DynExp::InstrumentInstance& Instance)
	{
		// Initialize derived instrument last.
		InitFuncImpl(dispatch_tag<InitTask>(), Instance);
	}

	void InterModuleCommunicatorTasks::ExitTask::ExitFuncImpl(dispatch_tag<DynExp::ExitTaskBase>, DynExp::InstrumentInstance& Instance)
	{
		// Shut down derived instrument first.
		ExitFuncImpl(dispatch_tag<ExitTask>(), Instance);
	}

	void InterModuleCommunicatorTasks::UpdateTask::UpdateFuncImpl(dispatch_tag<DynExp::UpdateTaskBase>, DynExp::InstrumentInstance& Instance)
	{
		// Update derived instrument.
		UpdateFuncImpl(dispatch_tag<UpdateTask>(), Instance);
	}

	void InterModuleCommunicatorData::ResetImpl(dispatch_tag<DynExp::InstrumentDataBase>)
	{
		ResetImpl(dispatch_tag<InterModuleCommunicatorData>());
	}

	InterModuleCommunicator::InterModuleCommunicator(const std::thread::id OwnerThreadID, DynExp::ParamsBasePtrType&& Params)
		: DynExp::InstrumentBase(OwnerThreadID, std::move(Params)), Core(GetParams()->GetCore())
	{
	}

	void InterModuleCommunicator::PostEvent(const DynExp::ModuleBase& Caller, const DynExp::InterModuleEventBase& InterModuleEvent) const
	{
		auto& ModuleMgr = Core.GetModuleManager();
		const auto UserIDs = GetUserIDs();

		for (auto ID : UserIDs)
		{
			// Do not send the event back to the caller.
			if (Caller.GetID() == ID)
				continue;

			auto Resource = ModuleMgr.GetResource(ID);

			// Retry in case of Util::TimeoutException a couple of times to not lose events.
			for (int NumTries = 0; true; NumTries++)
			{
				try
				{
					if (Resource->IsReady())
					{
						// Copies the event for each receiver.
						Resource->EnqueueEvent(InterModuleEvent.Clone(GetID()));
					}
				}
				catch ([[maybe_unused]] const Util::TimeoutException& e)
				{
					if (NumTries >= 100)
						throw;
					else
					{
						std::this_thread::yield();

						continue;
					}
				}

				break;
			}
		}
	}

	void InterModuleCommunicator::ResetImpl(dispatch_tag<DynExp::InstrumentBase>)
	{
		ResetImpl(dispatch_tag<InterModuleCommunicator>());
	}
}