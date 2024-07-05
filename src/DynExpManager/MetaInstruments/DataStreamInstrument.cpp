// This file is part of DynExp.

#include "stdafx.h"
#include "DataStreamInstrument.h"

namespace DynExpInstr
{
	DynExp::TaskResultType DataStreamInstrumentTasks::SetStreamSizeTask::RunChild(DynExp::InstrumentInstance& Instance)
	{
		auto InstrData = DynExp::dynamic_InstrumentData_cast<DataStreamInstrument>(Instance.InstrumentDataGetter());

		InstrData->GetSampleStream()->SetStreamSize(BufferSizeInSamples);

		return {};
	}

	void DataStreamBase::WriteBasicSamples(const BasicSampleListType& Samples)
	{
		for (const auto& Sample : Samples)
			WriteBasicSampleChild(Sample);
	}

	DataStreamBase::BasicSampleListType DataStreamBase::ReadBasicSamples(size_t Count)
	{
		if (!CanRead())
			return {};

		BasicSampleListType Samples;

		for (decltype(Count) i = 0; i < Count; ++i)
			Samples.push_back(ReadBasicSampleChild());

		return Samples;
	}

	void DataStreamBase::WriteBasicSampleChild(const BasicSample& Sample)
	{
		throw Util::NotImplementedException("This data stream does not support writing samples of type BasicSample.");
	}

	BasicSample DataStreamBase::ReadBasicSampleChild()
	{
		throw Util::NotImplementedException("This data stream does not support reading samples of type BasicSample.");

		return {};
	}

	size_t CircularDataStreamBase::GetNumRecentBasicSamples(size_t Count) const
	{
		return std::min(GetNumSamplesWritten() - Count, GetStreamSizeRead());
	}

	DataStreamBase::BasicSampleListType CircularDataStreamBase::ReadRecentBasicSamples(size_t Count)
	{
		auto OldReadPos = GetReadPosition();
		auto ReadLength = Util::NumToT<signed long long>(GetNumRecentBasicSamples(Count));
		SeekEqual(std::ios_base::in);
		SeekRel(-ReadLength, std::ios_base::cur, std::ios_base::in);
		auto Samples = ReadBasicSamples(ReadLength);
		SeekAbs(OldReadPos, std::ios_base::in);

		return Samples;
	}

	void StreamSizeParamsExtension::DisableUserEditable()
	{
		DynExp::ParamsBase::DisableUserEditable(StreamSize);
	}

	Util::TextValueListType<NumericSampleStreamParamsExtension::SamplingModeType> NumericSampleStreamParamsExtension::SamplingModeTypeStrList()
	{
		Util::TextValueListType<SamplingModeType> List = {
			{ "Perform reading/writing only once.", SamplingModeType::Single },
			{ "Perform reading/writing continuously.", SamplingModeType::Continuous }
		};

		return List;
	}

	void NumericSampleStreamParamsExtension::DisableUserEditable()
	{
		DynExp::ParamsBase::DisableUserEditable(SamplingRate);
		DynExp::ParamsBase::DisableUserEditable(SamplingMode);
	}

	const char* DataStreamInstrumentData::UnitTypeToStr(const UnitType& Unit)
	{
		switch (Unit)
		{
		case UnitType::Arbitrary: return "a.u.";
		case UnitType::LogicLevel: return "TTL";
		case UnitType::Counts: return "#";
		case UnitType::Volt: return "V";
		case UnitType::Ampere: return "A";
		case UnitType::Power_W: return "W";
		case UnitType::Power_dBm: return "dBm";
		default: return "<unknown unit>";
		}
	}

	DataStreamInstrumentData::DataStreamInstrumentData(DataStreamBasePtrType&& SampleStream)
		: SampleStream(std::move(SampleStream)), HardwareMinValue(0), HardwareMaxValue(0), ValueUnit(UnitType::Arbitrary)
	{
		if (!this->SampleStream)
			throw Util::InvalidArgException("SampleStream cannot be nullptr.");
	}

	void DataStreamInstrumentData::ResetImpl(dispatch_tag<InstrumentDataBase>)
	{
		SampleStream->Clear();

		HardwareMinValue = 0;
		HardwareMaxValue = 0;
		ValueUnit = UnitType::Arbitrary;

		ResetImpl(dispatch_tag<DataStreamInstrumentData>());
	}

	DataStreamInstrumentParams::~DataStreamInstrumentParams()
	{
	}

	DataStreamInstrumentConfigurator::~DataStreamInstrumentConfigurator()
	{
	}

	DataStreamInstrument::~DataStreamInstrument()
	{
	}

	void DataStreamInstrument::ReadData(DynExp::TaskBase::CallbackType CallbackFunc) const
	{
		// Do not throw a Util::NotImplementedException here, since it is fine to read data
		// which has been written to the buffer of e.g. a derived function generator instrument.
	}

	void DataStreamInstrument::WriteData(DynExp::TaskBase::CallbackType CallbackFunc) const
	{
		throw Util::NotImplementedException("This data stream instrument does not support write operations.");
	}

	void DataStreamInstrument::Restart(DynExp::TaskBase::CallbackType CallbackFunc) const
	{
		Stop();
		Start(CallbackFunc);
	}

	void DataStreamInstrument::SetStreamSize(size_t BufferSizeInSamples, DynExp::TaskBase::CallbackType CallbackFunc) const
	{
		MakeAndEnqueueTask<DataStreamInstrumentTasks::SetStreamSizeTask>(BufferSizeInSamples, CallbackFunc);
	}

	bool DataStreamInstrument::CanRead(const std::chrono::milliseconds Timeout) const
	{
		// Issue a read task, so it is allowed to poll CanRead() to await at least one sample becoming available.
		ReadData();

		return dynamic_InstrumentData_cast<DataStreamInstrument>(GetInstrumentData(Timeout))->GetSampleStream()->CanRead();
	}

	void DataStreamInstrument::Clear(const std::chrono::milliseconds Timeout) const
	{
		DynExp::dynamic_InstrumentData_cast<DataStreamInstrument>(GetInstrumentData(Timeout))->GetSampleStream()->Clear();

		ClearData();
	}

	void DataStreamInstrument::ResetImpl(dispatch_tag<InstrumentBase>)
	{
		ResetImpl(dispatch_tag<DataStreamInstrument>());
	}
}