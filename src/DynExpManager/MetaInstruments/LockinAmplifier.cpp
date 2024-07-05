// This file is part of DynExp.

#include "stdafx.h"
#include "LockinAmplifier.h"

namespace DynExpInstr
{
	LockinAmplifierDefs::LockinSample::LockinSample(uint8_t Channel, DataType Time, LockinResultCartesian CartesianResult) noexcept
		: Channel(Channel), CartesianResult(CartesianResult), Time(Time)
	{
		UpdatePolar();
	}

	LockinAmplifierDefs::LockinSample::LockinSample(uint8_t Channel, DataType Time, LockinResultPolar PolarResult) noexcept
		: Channel(Channel), PolarResult(PolarResult), Time(Time)
	{
		UpdateCartesian();
	}

	constexpr LockinAmplifierDefs::LockinSample::LockinSample(uint8_t Channel, DataType Time, LockinResultCartesian CartesianResult, LockinResultPolar PolarResult) noexcept
		: Channel(Channel), CartesianResult(CartesianResult), PolarResult(PolarResult), Time(Time)
	{
	}

	void LockinAmplifierDefs::LockinSample::UpdatePolar() noexcept
	{
		PolarResult.R = std::sqrt(CartesianResult.X * CartesianResult.X + CartesianResult.Y * CartesianResult.Y);
		PolarResult.Theta = std::atan(CartesianResult.Y / CartesianResult.X);
	}

	void LockinAmplifierDefs::LockinSample::UpdateCartesian() noexcept
	{
		CartesianResult.X = PolarResult.R * std::cos(PolarResult.Theta);
		CartesianResult.Y = PolarResult.R * std::sin(PolarResult.Theta);
	}

	LockinAmplifierDefs::LockinSample::DataType LockinAmplifierDefs::LockinSample::GetDisambiguatedValue(SignalType Signal) const noexcept
	{
		switch (Signal)
		{
		case SignalType::R: return PolarResult.R;
		case SignalType::Theta: return PolarResult.Theta;
		case SignalType::X: return CartesianResult.X;
		case SignalType::Y: return CartesianResult.Y;
		default: return 0;
		}
	}

	void LockinAmplifierTasks::InitTask::InitFuncImpl(dispatch_tag<DataStreamInstrumentTasks::InitTask>, DynExp::InstrumentInstance& Instance)
	{
		auto Owner = DynExp::dynamic_Object_cast<LockinAmplifier>(&Instance.GetOwner());

		Owner->ApplyFromParams();

		InitFuncImpl(dispatch_tag<InitTask>(), Instance);
	}

	double LockinAmplifierData::GetAcquisitionTime() const
	{
		auto SamplingRate = GetSamplingRate();

		// Avoid division by zero.
		if (SamplingRate > 0)
			return GetSampleStream()->GetStreamSizeWrite() / SamplingRate;
		else
			return 0;
	}

	void LockinAmplifierData::ResetImpl(dispatch_tag<DataStreamInstrumentData>)
	{
		ResetImpl(dispatch_tag<LockinAmplifierData>());
	}

	Util::TextValueListType<LockinAmplifierParams::AutoApplyParamsType> LockinAmplifierParams::AutoApplyParamsTypeStrList()
	{
		Util::TextValueListType<AutoApplyParamsType> List = {
			{ "Do not apply automatically", AutoApplyParamsType::DoNotApply },
			{ "Automatically apply parameters", AutoApplyParamsType::AutoApply },
		};

		return List;
	}

	LockinAmplifierParams::~LockinAmplifierParams()
	{
	}

	LockinAmplifierConfigurator::~LockinAmplifierConfigurator()
	{
	}

	LockinAmplifier::~LockinAmplifier()
	{
	}

	void LockinAmplifier::PersistDataToParams() const
	{
		auto InstrParams = DynExp::dynamic_Params_cast<LockinAmplifier>(GetNonConstParams());
		auto InstrData = DynExp::dynamic_InstrumentData_cast<LockinAmplifier>(GetInstrumentData());

		InstrParams->Sensitivity = InstrData->GetSensitivity();
		InstrParams->Phase = InstrData->GetPhase();
		InstrParams->TimeConstant = InstrData->GetTimeConstant();
		InstrParams->FilterOrder = InstrData->GetFilterOrder();
		InstrParams->TriggerMode = InstrData->GetTriggerMode();
		InstrParams->TriggerEdge = InstrData->GetTriggerEdge();
		InstrParams->Signal = InstrData->GetSignalType();
		InstrParams->SamplingRate = InstrData->GetSamplingRate();
		InstrParams->Enable = InstrData->IsEnabled();

		PersistDataToParamsImpl(dispatch_tag<LockinAmplifier>());
	}

	void LockinAmplifier::ApplyFromParams() const
	{
		ApplyFromParamsImpl(dispatch_tag<LockinAmplifier>());
	}

	void LockinAmplifier::AutoAdjustSensitivity(DynExp::TaskBase::CallbackType CallbackFunc) const
	{
		throw Util::NotImplementedException();
	}

	void LockinAmplifier::AutoAdjustPhase(DynExp::TaskBase::CallbackType CallbackFunc) const
	{
		throw Util::NotImplementedException();
	}

	void LockinAmplifier::SetFilterOrder(uint8_t FilterOrder, DynExp::TaskBase::CallbackType CallbackFunc) const
	{
		throw Util::NotImplementedException();
	}

	void LockinAmplifier::SetTriggerMode(LockinAmplifierDefs::TriggerModeType TriggerMode, DynExp::TaskBase::CallbackType CallbackFunc) const
	{
		throw Util::NotImplementedException();
	}

	void LockinAmplifier::SetTriggerEdge(LockinAmplifierDefs::TriggerEdgeType TriggerEdge, DynExp::TaskBase::CallbackType CallbackFunc) const
	{
		throw Util::NotImplementedException();
	}

	void LockinAmplifier::SetSamplingRate(double SamplingRate, DynExp::TaskBase::CallbackType CallbackFunc) const
	{
		throw Util::NotImplementedException();
	}

	void LockinAmplifier::SetEnable(bool Enable, DynExp::TaskBase::CallbackType CallbackFunc) const
	{
		throw Util::NotImplementedException();
	}

	void LockinAmplifier::ForceTrigger(DynExp::TaskBase::CallbackType CallbackFunc) const
	{
		throw Util::NotImplementedException();
	}

	void LockinAmplifier::ResetImpl(dispatch_tag<DataStreamInstrument>)
	{
		ResetImpl(dispatch_tag<LockinAmplifier>());
	}
}