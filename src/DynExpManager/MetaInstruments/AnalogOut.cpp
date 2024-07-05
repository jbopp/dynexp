// This file is part of DynExp.

#include "stdafx.h"
#include "AnalogOut.h"

namespace DynExpInstr
{
	void AnalogOutTasks::InitTask::InitFuncImpl(dispatch_tag<OutputPortTasks::InitTask>, DynExp::InstrumentInstance& Instance)
	{
		if (ApplyLimits())
		{
			auto InstrParams = DynExp::dynamic_Params_cast<AnalogOut>(Instance.ParamsGetter());
			auto InstrData = DynExp::dynamic_InstrumentData_cast<AnalogOut>(Instance.InstrumentDataGetter());

			InstrData->GetCastSampleStream<AnalogOutData::SampleStreamType>()->SetLimits(InstrParams->MinValue, InstrParams->MaxValue);
		} // InstrParams and InstrData unlocked here.

		InitFuncImpl(dispatch_tag<InitTask>(), Instance);
	}

	void AnalogOutTasks::ExitTask::ExitFuncImpl(dispatch_tag<OutputPortTasks::ExitTask>, DynExp::InstrumentInstance& Instance)
	{
		ExitFuncImpl(dispatch_tag<ExitTask>(), Instance);
	}

	void AnalogOutData::ResetImpl(dispatch_tag<OutputPortData>)
	{
		ResetImpl(dispatch_tag<AnalogOutData>());
	}

	AnalogOutParams::~AnalogOutParams()
	{
	}

	void AnalogOutParams::DisableUserEditable()
	{
		DynExp::ParamsBase::DisableUserEditable(DefaultValue);
		DynExp::ParamsBase::DisableUserEditable(MinValue);
		DynExp::ParamsBase::DisableUserEditable(MaxValue);
	}

	AnalogOutConfigurator::~AnalogOutConfigurator()
	{
	}

	AnalogOut::~AnalogOut()
	{
	}

	DataStreamInstrumentData::ValueType AnalogOut::GetUserMinValue() const
	{
		auto DerivedParams = dynamic_Params_cast<AnalogOut>(GetParams());
		return DerivedParams->MinValue;
	}

	DataStreamInstrumentData::ValueType AnalogOut::GetUserMaxValue() const
	{
		auto DerivedParams = dynamic_Params_cast<AnalogOut>(GetParams());
		return DerivedParams->MaxValue;
	}

	void AnalogOut::Set(AnalogOutData::SampleStreamType::SampleType Sample, DynExp::TaskBase::CallbackType CallbackFunc) const
	{
		{
			auto InstrData = dynamic_InstrumentData_cast<AnalogOut>(GetInstrumentData());

			InstrData->GetSampleStream()->WriteBasicSample({ static_cast<BasicSample::DataType>(Sample), 0 });
		} // InstrData unlocked here.

		WriteData(CallbackFunc);
	}

	void AnalogOut::SetSync(AnalogOutData::SampleStreamType::SampleType Sample) const
	{
		AsSyncTask(&AnalogOut::Set, Sample);
	}

	void AnalogOut::SetDefault(DynExp::TaskBase::CallbackType CallbackFunc) const
	{
		{
			auto InstrParams = DynExp::dynamic_Params_cast<AnalogOut>(GetParams());
			auto InstrData = dynamic_InstrumentData_cast<AnalogOut>(GetInstrumentData());

			AnalogOutData::SampleStreamType::SampleType DefaultValue = InstrParams->DefaultValue;
			auto SampleStream = InstrData->GetSampleStream();

			SampleStream->Clear();
			for (auto i = SampleStream->GetStreamSizeWrite(); i > 0; --i)
				SampleStream->WriteBasicSample({ static_cast<BasicSample::DataType>(DefaultValue), 0 });
		} // InstrParams and InstrData unlocked here.

		WriteData();
	}

	void AnalogOut::OnPrepareExitChild() const
	{
		SetDefault();
	}

	void AnalogOut::ResetImpl(dispatch_tag<OutputPort>)
	{
		ResetImpl(dispatch_tag<AnalogOut>());
	}	
}