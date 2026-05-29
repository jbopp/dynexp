// This file is part of DynExp.

#include "stdafx.h"
#include "DummyDigitalOut.h"

namespace DynExpInstr
{
	void DummyDigitalOutTasks::InitTask::InitFuncImpl(dispatch_tag<DigitalOutTasks::InitTask>, DynExp::InstrumentInstance& Instance)
	{
		// Initialize derived instrument last.
		InitFuncImpl(dispatch_tag<InitTask>(), Instance);
	}

	void DummyDigitalOutTasks::ExitTask::ExitFuncImpl(dispatch_tag<DigitalOutTasks::ExitTask>, DynExp::InstrumentInstance& Instance)
	{
		// Shut down derived instrument first.
		ExitFuncImpl(dispatch_tag<ExitTask>(), Instance);
	}

	void DummyDigitalOutTasks::UpdateTask::UpdateFuncImpl(dispatch_tag<DigitalOutTasks::UpdateTask>, DynExp::InstrumentInstance& Instance)
	{
		// Update derived instrument.
		UpdateFuncImpl(dispatch_tag<UpdateTask>(), Instance);
	}

	void DummyDigitalOutData::ResetImpl(dispatch_tag<DigitalOutData>)
	{
		ResetImpl(dispatch_tag<DummyDigitalOutData>());
	}

	DummyDigitalOut::DummyDigitalOut(const std::thread::id OwnerThreadID, DynExp::ParamsBasePtrType&& Params)
		: DigitalOut(OwnerThreadID, std::move(Params))
	{
	}

	void DummyDigitalOut::ResetImpl(dispatch_tag<DigitalOut>)
	{
		ResetImpl(dispatch_tag<DummyDigitalOut>());
	}
}