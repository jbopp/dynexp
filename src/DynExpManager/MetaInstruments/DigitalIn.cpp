// This file is part of DynExp.

#include "stdafx.h"
#include "DigitalIn.h"

namespace DynExpInstr
{
	void DigitalInData::ResetImpl(dispatch_tag<InputPortData>)
	{
		ResetImpl(dispatch_tag<DigitalInData>());
	}

	DigitalInData::SampleStreamType::SampleType DigitalIn::Get(DynExp::TaskBase::CallbackType CallbackFunc) const
	{
		ReadData(CallbackFunc);

		auto InstrData = dynamic_InstrumentData_cast<DigitalIn>(GetInstrumentData());
		auto Sample = InstrData->GetSampleStream()->ReadBasicSample().Value;

		return Sample;
	}

	DigitalInData::SampleStreamType::SampleType DigitalIn::GetSync() const
	{
		AsSyncTask(&DigitalIn::ReadData);

		auto InstrData = dynamic_InstrumentData_cast<DigitalIn>(GetInstrumentData());
		auto Sample = InstrData->GetSampleStream()->ReadBasicSample().Value;

		return Sample;
	}

	DigitalInParams::~DigitalInParams()
	{
	}

	void DigitalInParams::DisableUserEditable()
	{
	}

	DigitalInConfigurator::~DigitalInConfigurator()
	{
	}

	DigitalIn::~DigitalIn()
	{
	}

	void DigitalIn::ResetImpl(dispatch_tag<InputPort>)
	{
		ResetImpl(dispatch_tag<DigitalIn>());
	}
}