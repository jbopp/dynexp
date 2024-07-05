// This file is part of DynExp.

#include "stdafx.h"
#include "AnalogIn.h"

namespace DynExpInstr
{
	void AnalogInData::ResetImpl(dispatch_tag<InputPortData>)
	{
		ResetImpl(dispatch_tag<AnalogInData>());
	}

	AnalogInParams::~AnalogInParams()
	{
	}

	void AnalogInParams::DisableUserEditable()
	{
	}

	AnalogInConfigurator::~AnalogInConfigurator()
	{
	}

	AnalogIn::~AnalogIn()
	{
	}

	AnalogInData::SampleStreamType::SampleType AnalogIn::Get(DynExp::TaskBase::CallbackType CallbackFunc) const
	{
		ReadData(CallbackFunc);

		auto InstrData = dynamic_InstrumentData_cast<AnalogIn>(GetInstrumentData());
		auto Sample = InstrData->GetSampleStream()->ReadBasicSample().Value;

		return Sample;
	}

	AnalogInData::SampleStreamType::SampleType AnalogIn::GetSync() const
	{
		AsSyncTask(&AnalogIn::ReadData);

		auto InstrData = dynamic_InstrumentData_cast<AnalogIn>(GetInstrumentData());
		auto Sample = InstrData->GetSampleStream()->ReadBasicSample().Value;

		return Sample;
	}

	void AnalogIn::ResetImpl(dispatch_tag<InputPort>)
	{
		ResetImpl(dispatch_tag<AnalogIn>());
	}
}