// This file is part of DynExp.

#include "stdafx.h"
#include "DummyDataStreamInstrument.h"

namespace DynExpInstr
{
	void DummyDataStreamInstrumentTasks::InitTask::InitFuncImpl(dispatch_tag<FunctionGeneratorTasks::InitTask>, DynExp::InstrumentInstance& Instance)
	{
		auto InstrParams = DynExp::dynamic_Params_cast<DummyDataStreamInstrument>(Instance.ParamsGetter());
		auto InstrData = DynExp::dynamic_InstrumentData_cast<DummyDataStreamInstrument>(Instance.InstrumentDataGetter());

		InstrData->GetSampleStream()->SetStreamSize(InstrParams->StreamSizeParams.StreamSize);

		InitFuncImpl(dispatch_tag<InitTask>(), Instance);
	}

	DynExp::TaskResultType DummyDataStreamInstrumentTasks::ResetStreamSizeTask::RunChild(DynExp::InstrumentInstance& Instance)
	{
		auto InstrParams = DynExp::dynamic_Params_cast<DummyDataStreamInstrument>(Instance.ParamsGetter());
		auto InstrData = DynExp::dynamic_InstrumentData_cast<DummyDataStreamInstrument>(Instance.InstrumentDataGetter());

		InstrData->GetSampleStream()->SetStreamSize(InstrParams->StreamSizeParams.StreamSize);

		return {};
	}

	void DummyDataStreamInstrumentData::ResetImpl(dispatch_tag<FunctionGeneratorData>)
	{
		ResetImpl(dispatch_tag<DummyDataStreamInstrumentData>());
	}

	DummyDataStreamInstrument::DummyDataStreamInstrument(const std::thread::id OwnerThreadID, DynExp::ParamsBasePtrType&& Params)
		: FunctionGenerator(OwnerThreadID, std::move(Params))
	{
	}

	void DummyDataStreamInstrument::ResetImpl(dispatch_tag<FunctionGenerator>)
	{
		ResetImpl(dispatch_tag<DummyDataStreamInstrument>());
	}
}