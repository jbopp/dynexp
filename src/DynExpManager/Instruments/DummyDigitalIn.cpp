// This file is part of DynExp.

#include "stdafx.h"
#include "DummyDigitalIn.h"

namespace DynExpInstr
{
	void DummyDigitalInTasks::InitTask::InitFuncImpl(dispatch_tag<DigitalInTasks::InitTask>, DynExp::InstrumentInstance& Instance)
	{
		// Initialize derived instrument last.
		InitFuncImpl(dispatch_tag<InitTask>(), Instance);
	}

	void DummyDigitalInTasks::ExitTask::ExitFuncImpl(dispatch_tag<DigitalInTasks::ExitTask>, DynExp::InstrumentInstance& Instance)
	{
		// Shut down derived instrument first.
		ExitFuncImpl(dispatch_tag<ExitTask>(), Instance);
	}

	void DummyDigitalInTasks::UpdateTask::UpdateFuncImpl(dispatch_tag<DigitalInTasks::UpdateTask>, DynExp::InstrumentInstance& Instance)
	{
		// Update derived instrument.
		UpdateFuncImpl(dispatch_tag<UpdateTask>(), Instance);
	}

	void DummyDigitalInData::ResetImpl(dispatch_tag<DigitalInData>)
	{
		ResetImpl(dispatch_tag<DummyDigitalInData>());
	}

	DummyDigitalIn::DummyDigitalIn(const std::thread::id OwnerThreadID, DynExp::ParamsBasePtrType&& Params)
		: DigitalIn(OwnerThreadID, std::move(Params))
	{
	}

	void DummyDigitalIn::ResetImpl(dispatch_tag<DigitalIn>)
	{
		ResetImpl(dispatch_tag<DummyDigitalIn>());
	}
}