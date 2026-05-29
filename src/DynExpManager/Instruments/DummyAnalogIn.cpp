// This file is part of DynExp.

#include "stdafx.h"
#include "DummyAnalogIn.h"

namespace DynExpInstr
{
	void DummyAnalogInTasks::InitTask::InitFuncImpl(dispatch_tag<AnalogInTasks::InitTask>, DynExp::InstrumentInstance& Instance)
	{
		// Initialize derived instrument last.
		InitFuncImpl(dispatch_tag<InitTask>(), Instance);
	}

	void DummyAnalogInTasks::ExitTask::ExitFuncImpl(dispatch_tag<AnalogInTasks::ExitTask>, DynExp::InstrumentInstance& Instance)
	{
		// Shut down derived instrument first.
		ExitFuncImpl(dispatch_tag<ExitTask>(), Instance);
	}

	void DummyAnalogInTasks::UpdateTask::UpdateFuncImpl(dispatch_tag<AnalogInTasks::UpdateTask>, DynExp::InstrumentInstance& Instance)
	{
		// Update derived instrument.
		UpdateFuncImpl(dispatch_tag<UpdateTask>(), Instance);
	}

	void DummyAnalogInData::ResetImpl(dispatch_tag<AnalogInData>)
	{
		ResetImpl(dispatch_tag<DummyAnalogInData>());
	}

	DummyAnalogIn::DummyAnalogIn(const std::thread::id OwnerThreadID, DynExp::ParamsBasePtrType&& Params)
		: AnalogIn(OwnerThreadID, std::move(Params))
	{
	}

	void DummyAnalogIn::ResetImpl(dispatch_tag<AnalogIn>)
	{
		ResetImpl(dispatch_tag<DummyAnalogIn>());
	}
}