// This file is part of DynExp.

#include "stdafx.h"
#include "TimeTagger.h"

namespace DynExpInstr
{
	void TimeTaggerTasks::InitTask::InitFuncImpl(dispatch_tag<DataStreamInstrumentTasks::InitTask>, DynExp::InstrumentInstance& Instance)
	{
		auto InstrParams = DynExp::dynamic_Params_cast<TimeTagger>(Instance.ParamsGetter());
		auto InstrData = DynExp::dynamic_InstrumentData_cast<TimeTagger>(Instance.InstrumentDataGetter());

		if (ApplyDataStreamSizeFromParams())
			InstrData->GetSampleStream()->SetStreamSize(InstrParams->StreamSizeParams.StreamSize);

		InstrData->SetValueUnit(DataStreamInstrumentData::UnitType::Counts);
		InstrData->SetStreamMode(InstrParams->StreamMode);

		InitFuncImpl(dispatch_tag<InitTask>(), Instance);
	}

	DynExp::TaskResultType DynExpInstr::TimeTaggerTasks::ResetBufferSizeTask::RunChild(DynExp::InstrumentInstance& Instance)
	{
		auto InstrParams = DynExp::dynamic_Params_cast<TimeTagger>(Instance.ParamsGetter());
		auto InstrData = DynExp::dynamic_InstrumentData_cast<TimeTagger>(Instance.InstrumentDataGetter());

		InstrData->GetSampleStream()->SetStreamSize(InstrParams->StreamSizeParams.StreamSize);

		return {};
	}

	void TimeTaggerData::HBTResultsType::Reset()
	{
		Enabled = false;
		EventCounts = 0;
		IntegrationTime = std::chrono::microseconds(0);

		ResultVector.clear();
	}

	void TimeTaggerData::SetStreamMode(StreamModeType StreamMode) const noexcept
	{
		this->StreamMode = StreamMode;
		StreamModeChanged = true;
	}

	void TimeTaggerData::ResetImpl(dispatch_tag<DataStreamInstrumentData>)
	{
		StreamMode = StreamModeType::Counts;
		HBTResults.Reset();

		ResetImpl(dispatch_tag<TimeTaggerData>());
	}

	Util::TextValueListType<TimeTaggerData::StreamModeType> TimeTaggerParams::StreamModeTypeStrList()
	{
		Util::TextValueListType<TimeTaggerData::StreamModeType> List = {
			{ "Sample accumulated counts detected within an exposure interval.", TimeTaggerData::StreamModeType::Counts },
			{ "Sample time tags denoting the occurrence of single counts.", TimeTaggerData::StreamModeType::Events }
		};

		return List;
	}

	TimeTaggerParams::~TimeTaggerParams()
	{
	}

	TimeTaggerConfigurator::~TimeTaggerConfigurator()
	{
	}

	TimeTagger::~TimeTagger()
	{
	}

	void TimeTagger::ConfigureInput(bool UseRisingEdge, double ThresholdInVolts, DynExp::TaskBase::CallbackType CallbackFunc) const
	{
		throw Util::NotImplementedException();
	}

	void TimeTagger::SetExposureTime(Util::picoseconds ExposureTime, DynExp::TaskBase::CallbackType CallbackFunc) const
	{
		throw Util::NotImplementedException();
	}

	void TimeTagger::SetCoincidenceWindow(Util::picoseconds CoincidenceWindow, DynExp::TaskBase::CallbackType CallbackFunc) const
	{
		throw Util::NotImplementedException();
	}

	void TimeTagger::SetDelay(Util::picoseconds Delay, DynExp::TaskBase::CallbackType CallbackFunc) const
	{
		throw Util::NotImplementedException();
	}

	void TimeTagger::SetHBTActive(bool Enable, DynExp::TaskBase::CallbackType CallbackFunc) const
	{
		throw Util::NotImplementedException();
	}

	void TimeTagger::ConfigureHBT(Util::picoseconds BinWidth, size_t BinCount, DynExp::TaskBase::CallbackType CallbackFunc) const
	{
		throw Util::NotImplementedException();
	}

	void TimeTagger::ResetHBT(DynExp::TaskBase::CallbackType CallbackFunc) const
	{
		throw Util::NotImplementedException();
	}

	void TimeTagger::ResetImpl(dispatch_tag<DataStreamInstrument>)
	{
		ResetImpl(dispatch_tag<TimeTagger>());
	}
}