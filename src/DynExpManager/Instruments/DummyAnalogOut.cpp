// This file is part of DynExp.

#include "stdafx.h"
#include "DummyAnalogOut.h"

namespace DynExpInstr
{
	void DummyAnalogOutTasks::InitTask::InitFuncImpl(dispatch_tag<AnalogOutTasks::InitTask>, DynExp::InstrumentInstance& Instance)
	{
		// Initialize derived instrument last.
		InitFuncImpl(dispatch_tag<InitTask>(), Instance);
	}

	void DummyAnalogOutTasks::ExitTask::ExitFuncImpl(dispatch_tag<AnalogOutTasks::ExitTask>, DynExp::InstrumentInstance& Instance)
	{
		// Shut down derived instrument first.
		ExitFuncImpl(dispatch_tag<ExitTask>(), Instance);
	}

	void DummyAnalogOutTasks::UpdateTask::UpdateFuncImpl(dispatch_tag<AnalogOutTasks::UpdateTask>, DynExp::InstrumentInstance& Instance)
	{
		// Update derived instrument.
		UpdateFuncImpl(dispatch_tag<UpdateTask>(), Instance);
	}

	void DummyAnalogOutData::ResetImpl(dispatch_tag<AnalogOutData>)
	{
		ResetImpl(dispatch_tag<DummyAnalogOutData>());
	}

	DummyAnalogOut::DummyAnalogOut(const std::thread::id OwnerThreadID, DynExp::ParamsBasePtrType&& Params)
		: AnalogOut(OwnerThreadID, std::move(Params))
	{
	}

	void DummyAnalogOut::ResetImpl(dispatch_tag<AnalogOut>)
	{
		ResetImpl(dispatch_tag<DummyAnalogOut>());
	}
}