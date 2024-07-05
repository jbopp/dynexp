// This file is part of DynExp.

#include "stdafx.h"
#include "FunctionGenerator.h"

namespace DynExpInstr
{
	FunctionGeneratorDefs::PulsesDescType::PulsesDescType(PulsesDescType&& Other)
		: Pulses(std::move(Other.Pulses)), Offset(Other.Offset)
	{
		Other.Reset();
	}

	FunctionGeneratorDefs::PulsesDescType::PulsesDescType(PulsesType&& Pulses)
		: Pulses(std::move(Pulses)), Offset(0.0)
	{
		Pulses.clear();
	}

	FunctionGeneratorDefs::PulsesDescType::PulsesDescType(const std::vector<double>& PulseStarts, const std::vector<double>& PulseAmplitudes, double Offset)
		: Offset(Offset)
	{
		if (PulseStarts.size() != PulseAmplitudes.size())
			throw Util::InvalidArgException("The given vectors of pulse start times and pulse lengths do not have the sime size.");

		std::transform(PulseStarts.cbegin(), PulseStarts.cend(), PulseAmplitudes.cbegin(),
			std::inserter(Pulses, Pulses.begin()),
			[](typename PulsesType::key_type key, typename PulsesType::mapped_type value) { return std::make_pair(key, value); });
	}

	FunctionGeneratorDefs::PulsesDescType& FunctionGeneratorDefs::PulsesDescType::operator=(PulsesDescType&& Other)
	{
		Pulses = std::move(Other.Pulses);
		Offset = Other.Offset;

		Other.Reset();
		
		return *this;
	}

	constexpr Util::seconds FunctionGeneratorDefs::PeriodFromFunctionDesc(const FunctionDescType& FunctionDesc)
	{
		return Util::seconds(1.0 / FunctionDesc.FrequencyInHz);
	}

	// returns Heaviside(-(Phase mod 2*pi) + 2*pi * DutyCycle)
	double FunctionGeneratorDefs::RectFunc(double DutyCycle, double Phase)
	{
		return std::fmod(Phase, 2.0 * std::numbers::pi) <= 2.0 * std::numbers::pi * DutyCycle;
	}

	// returns Heaviside((Phase mod 2*pi) - 2*pi * DutyCycle)
	double FunctionGeneratorDefs::InvRectFunc(double DutyCycle, double Phase)
	{
		return std::fmod(Phase, 2.0 * std::numbers::pi) > 2.0 * std::numbers::pi * DutyCycle;
	}

	// returns RectFunc(Phase, RiseFallRatio) * (Phase mod(2*pi)) / (pi * RiseFallRatio)
	//			+ InvRectFunc(Phase, RiseFallRatio) * ((-Phase) mod(2*pi)) / (pi * (1 - RiseFallRatio))
	//			- 1
	double FunctionGeneratorDefs::RampFunc(double RiseFallRatio, double Phase)
	{
		// RiseFallRatio = 0 or RiseFallRatio = 1 lead to division by 0.
		constexpr double MaxPrecision = 1e-6;
		RiseFallRatio = std::min(1.0 - MaxPrecision, RiseFallRatio);
		RiseFallRatio = std::max(MaxPrecision, RiseFallRatio);

		return RectFunc(RiseFallRatio, Phase) * std::fmod(Phase, 2.0 * std::numbers::pi) / (std::numbers::pi * RiseFallRatio)
			+ InvRectFunc(RiseFallRatio, Phase) * (std::fmod(-Phase, 2.0 * std::numbers::pi) + 2.0 * std::numbers::pi) / (std::numbers::pi * (1 - RiseFallRatio))
			- 1;
	}

	void FunctionGeneratorTasks::InitTask::InitFuncImpl(dispatch_tag<DataStreamInstrumentTasks::InitTask>, DynExp::InstrumentInstance& Instance)
	{
		// Initialize this FunctionGenerator meta instrument first.
		// -> Nothing to be done.
		
		// Initialize derived classes next since only tasks generating certain waveform and setting up the trigger
		// are enqueued afterwards. These tasks may rely on the derived classes being fully initialized.
		InitFuncImpl(dispatch_tag<InitTask>(), Instance);

		auto InstrParams = DynExp::dynamic_Params_cast<FunctionGenerator>(Instance.ParamsGetter());
		auto InstrData = DynExp::dynamic_InstrumentData_cast<FunctionGenerator>(Instance.InstrumentDataGetter());
		auto Owner = DynExp::dynamic_Object_cast<FunctionGenerator>(&Instance.GetOwner());

		if (InstrParams->WaveformType == FunctionGeneratorDefs::WaveformTypes::Sine)
			Owner->SetSineFunction(FunctionGeneratorDefs::SineFunctionDescType(
				InstrParams->FrequencyInHz, InstrParams->Amplitude, InstrParams->Offset, InstrParams->PhaseInRad),
				false, InstrParams->Autostart.Get());
		if (InstrParams->WaveformType == FunctionGeneratorDefs::WaveformTypes::Rect)
			Owner->SetRectFunction(FunctionGeneratorDefs::RectFunctionDescType(
				InstrParams->FrequencyInHz, InstrParams->Amplitude, InstrParams->Offset, InstrParams->PhaseInRad, InstrParams->DutyCycle),
				false, InstrParams->Autostart.Get());
		if (InstrParams->WaveformType == FunctionGeneratorDefs::WaveformTypes::Ramp)
			Owner->SetRampFunction(FunctionGeneratorDefs::RampFunctionDescType(
				InstrParams->FrequencyInHz, InstrParams->Amplitude, InstrParams->Offset, InstrParams->PhaseInRad, InstrParams->DutyCycle),
				false, InstrParams->Autostart.Get());
		if (InstrParams->WaveformType == FunctionGeneratorDefs::WaveformTypes::Pulse)
			Owner->SetPulseFunction(FunctionGeneratorDefs::PulsesDescType(
				InstrParams->PulseStarts.Get(), InstrParams->PulseAmplitudes.Get(), InstrParams->Offset),
				false, InstrParams->Autostart.Get());

		if (Owner->GetTriggerCaps().Test(FunctionGenerator::TriggerCapsType::CanConfigure))
			Owner->SetTrigger(FunctionGeneratorDefs::TriggerDescType(
				InstrParams->TriggerMode, InstrParams->TriggerEdge),
				false);
	}

	void FunctionGeneratorTasks::UpdateTask::UpdateFuncImpl(dispatch_tag<DataStreamInstrumentTasks::UpdateTask>, DynExp::InstrumentInstance& Instance)
	{
		{
			auto InstrData = DynExp::dynamic_InstrumentData_cast<FunctionGenerator>(Instance.InstrumentDataGetter());
			if (InstrData->DataHasBeenUpdated)
				if (InstrData->GetSampleStream()->GetNumSamplesWritten() >= InstrData->GetSampleStream()->GetStreamSizeWrite())
				{
					DynExp::dynamic_Object_cast<FunctionGenerator>(&Instance.GetOwner())->WriteData();
					InstrData->DataHasBeenUpdated = false;

					if (InstrData->ShouldAutostart)
						DynExp::dynamic_Object_cast<FunctionGenerator>(&Instance.GetOwner())->Start();
				}
		} // InstrData unlocked here.

		UpdateFuncImpl(dispatch_tag<UpdateTask>(), Instance);
	}

	DynExp::TaskResultType FunctionGeneratorTasks::SetSineFunctionTask::RunChild(DynExp::InstrumentInstance& Instance)
	{
		auto InstrData = DynExp::dynamic_InstrumentData_cast<FunctionGenerator>(Instance.InstrumentDataGetter());

		auto SampleStream = InstrData->GetSampleStream();
		SampleStream->Clear();

		auto NumSamples = SampleStream->GetStreamSizeWrite();
		for (decltype(NumSamples) i = 0; i < NumSamples; ++i)
		{
			auto t = FunctionGeneratorDefs::PeriodFromFunctionDesc(FunctionDesc) / NumSamples * i;
			double Value = FunctionDesc.Amplitude * std::sin(FunctionDesc.FrequencyInHz * 2 * std::numbers::pi
				* t.count() + FunctionDesc.PhaseInRad) + FunctionDesc.Offset;

			SampleStream->WriteBasicSample({ Value, t.count() });
		}

		InstrData->DataHasBeenUpdated = true;
		InstrData->CurrentWaveformType = FunctionGeneratorDefs::WaveformTypes::Sine;
		InstrData->CurrentFrequencyInHz = FunctionDesc.FrequencyInHz;
		InstrData->CurrentAmplitude = FunctionDesc.Amplitude;
		InstrData->CurrentOffset = FunctionDesc.Offset;
		InstrData->CurrentPhaseInRad = FunctionDesc.PhaseInRad;
		InstrData->ShouldAutostart = Autostart;

		return {};
	}

	DynExp::TaskResultType FunctionGeneratorTasks::SetRectFunctionTask::RunChild(DynExp::InstrumentInstance& Instance)
	{
		auto InstrData = DynExp::dynamic_InstrumentData_cast<FunctionGenerator>(Instance.InstrumentDataGetter());

		auto SampleStream = InstrData->GetSampleStream();
		SampleStream->Clear();

		auto NumSamples = SampleStream->GetStreamSizeWrite();
		for (decltype(NumSamples) i = 0; i < NumSamples; ++i)
		{
			auto t = FunctionGeneratorDefs::PeriodFromFunctionDesc(FunctionDesc) / NumSamples * i;
			double Value = FunctionDesc.Amplitude * FunctionGeneratorDefs::RectFunc(FunctionDesc.DutyCycle,
				FunctionDesc.FrequencyInHz * 2 * std::numbers::pi * t.count() + FunctionDesc.PhaseInRad) + FunctionDesc.Offset;

			SampleStream->WriteBasicSample({ Value, t.count() });
		}

		InstrData->DataHasBeenUpdated = true;
		InstrData->CurrentWaveformType = FunctionGeneratorDefs::WaveformTypes::Rect;
		InstrData->CurrentFrequencyInHz = FunctionDesc.FrequencyInHz;
		InstrData->CurrentAmplitude = FunctionDesc.Amplitude;
		InstrData->CurrentOffset = FunctionDesc.Offset;
		InstrData->CurrentPhaseInRad = FunctionDesc.PhaseInRad;
		InstrData->CurrentDutyCycle = FunctionDesc.DutyCycle;
		InstrData->ShouldAutostart = Autostart;

		return {};
	}

	DynExp::TaskResultType FunctionGeneratorTasks::SetRampFunctionTask::RunChild(DynExp::InstrumentInstance& Instance)
	{
		auto InstrData = DynExp::dynamic_InstrumentData_cast<FunctionGenerator>(Instance.InstrumentDataGetter());

		auto SampleStream = InstrData->GetSampleStream();
		SampleStream->Clear();

		auto NumSamples = SampleStream->GetStreamSizeWrite();
		for (decltype(NumSamples) i = 0; i < NumSamples; ++i)
		{
			auto t = FunctionGeneratorDefs::PeriodFromFunctionDesc(FunctionDesc) / NumSamples * i;
			double Value = FunctionDesc.Amplitude * FunctionGeneratorDefs::RampFunc(FunctionDesc.RiseFallRatio,
				FunctionDesc.FrequencyInHz * 2 * std::numbers::pi * t.count() + FunctionDesc.PhaseInRad) + FunctionDesc.Offset;

			SampleStream->WriteBasicSample({ Value, t.count() });
		}

		InstrData->DataHasBeenUpdated = true;
		InstrData->CurrentWaveformType = FunctionGeneratorDefs::WaveformTypes::Ramp;
		InstrData->CurrentFrequencyInHz = FunctionDesc.FrequencyInHz;
		InstrData->CurrentAmplitude = FunctionDesc.Amplitude;
		InstrData->CurrentOffset = FunctionDesc.Offset;
		InstrData->CurrentPhaseInRad = FunctionDesc.PhaseInRad;
		InstrData->CurrentDutyCycle = FunctionDesc.RiseFallRatio;
		InstrData->ShouldAutostart = Autostart;

		return {};
	}

	DynExp::TaskResultType FunctionGeneratorTasks::SetPulseFunctionTask::RunChild(DynExp::InstrumentInstance& Instance)
	{
		auto InstrData = DynExp::dynamic_InstrumentData_cast<FunctionGenerator>(Instance.InstrumentDataGetter());

		if (FunctionDesc.Pulses.size() > 1)
		{
			auto SampleStream = InstrData->GetSampleStream();
			SampleStream->Clear();
		
			auto NumSamples = SampleStream->GetStreamSizeWrite();
			auto StartTime = FunctionDesc.Pulses.cbegin()->first;
			auto TimePerSample = (std::prev(FunctionDesc.Pulses.cend())->first - StartTime) / (NumSamples - 1);
			auto PulseIt = FunctionDesc.Pulses.cbegin();
			for (decltype(NumSamples) i = 0; i < NumSamples; ++i)
			{
				auto t = StartTime + i * TimePerSample;
				while (std::next(PulseIt) != FunctionDesc.Pulses.cend() && t >= std::next(PulseIt)->first)
					++PulseIt;

				SampleStream->WriteBasicSample({ PulseIt->second + FunctionDesc.Offset, t });
			}
		}

		InstrData->DataHasBeenUpdated = true;
		InstrData->CurrentWaveformType = FunctionGeneratorDefs::WaveformTypes::Pulse;
		InstrData->CurrentPulses = FunctionDesc.Pulses;
		InstrData->ShouldAutostart = Autostart;

		return {};
	}

	DynExp::TaskResultType FunctionGeneratorTasks::SetArbitraryFunctionTask::RunChild(DynExp::InstrumentInstance& Instance)
	{
		auto InstrData = DynExp::dynamic_InstrumentData_cast<FunctionGenerator>(Instance.InstrumentDataGetter());

		auto SampleStream = InstrData->GetSampleStream();
		SampleStream->Clear();
		SampleStream->SetStreamSize(Samples.size());
		SampleStream->WriteBasicSamples(Samples);

		InstrData->DataHasBeenUpdated = true;
		InstrData->CurrentWaveformType = FunctionGeneratorDefs::WaveformTypes::None;
		InstrData->ShouldAutostart = Autostart;

		return {};
	}

	DynExp::TaskResultType FunctionGeneratorTasks::SetTriggerTask::RunChild(DynExp::InstrumentInstance& Instance)
	{
		auto InstrData = DynExp::dynamic_InstrumentData_cast<FunctionGenerator>(Instance.InstrumentDataGetter());

		InstrData->CurrentTriggerMode = TriggerDesc.TriggerMode;
		InstrData->CurrentTriggerEdge = TriggerDesc.TriggerEdge;

		return {};
	}

	void FunctionGeneratorData::ResetImpl(dispatch_tag<DataStreamInstrumentData>)
	{
		DataHasBeenUpdated = false;

		CurrentWaveformType = FunctionGeneratorDefs::WaveformTypes::None;
		CurrentFrequencyInHz = FunctionGeneratorDefs::GetDefaultDefaultFunctionDesc().FrequencyInHz;
		CurrentAmplitude = FunctionGeneratorDefs::GetDefaultDefaultFunctionDesc().Amplitude;
		CurrentOffset = FunctionGeneratorDefs::GetDefaultDefaultFunctionDesc().Offset;
		CurrentPhaseInRad = 0;
		CurrentDutyCycle = .5;
		CurrentTriggerMode = FunctionGeneratorDefs::TriggerDescType::TriggerModeType::Continuous;
		CurrentTriggerEdge = FunctionGeneratorDefs::TriggerDescType::TriggerEdgeType::Rise;
		ShouldAutostart = false;

		CurrentPulses.Reset();

		ResetImpl(dispatch_tag<FunctionGeneratorData>());
	}

	FunctionGeneratorParams::~FunctionGeneratorParams()
	{
	}

	FunctionGeneratorConfigurator::~FunctionGeneratorConfigurator()
	{
	}

	FunctionGenerator::~FunctionGenerator()
	{
	}

	void FunctionGenerator::SetSineFunction(const FunctionGeneratorDefs::SineFunctionDescType& FunctionDesc,
		bool PersistParams, bool Autostart, DynExp::TaskBase::CallbackType CallbackFunc) const
	{
		if (!GetWaveformCaps().Test(WaveformCapsType::UserDefined))
			throw Util::NotAvailableException(
				"The function generator of type " + GetCategoryAndName() + " is not capable to generate a user-defined function.", Util::ErrorType::Error);

		if (PersistParams)
		{
			auto InstrParams = DynExp::dynamic_Params_cast<FunctionGenerator>(GetNonConstParams());
			InstrParams->WaveformType = FunctionGeneratorDefs::WaveformTypes::Sine;
			InstrParams->FrequencyInHz = FunctionDesc.FrequencyInHz;
			InstrParams->Amplitude = FunctionDesc.Amplitude;
			InstrParams->Offset = FunctionDesc.Offset;
			InstrParams->PhaseInRad = IsPhaseAdjustable() ? FunctionDesc.PhaseInRad : 0;
			InstrParams->DutyCycle = .5;
			InstrParams->Autostart = Autostart;
		} // InstrParams unlocked here.

		MakeAndEnqueueTask<FunctionGeneratorTasks::SetSineFunctionTask>(FunctionDesc, Autostart, CallbackFunc);
		UpdateData();
	}

	void FunctionGenerator::SetRectFunction(const FunctionGeneratorDefs::RectFunctionDescType& FunctionDesc,
		bool PersistParams, bool Autostart, DynExp::TaskBase::CallbackType CallbackFunc) const
	{
		if (!GetWaveformCaps().Test(WaveformCapsType::UserDefined))
			throw Util::NotAvailableException(
				"The function generator of type " + GetCategoryAndName() + " is not capable to generate a user-defined function.", Util::ErrorType::Error);

		if (PersistParams)
		{
			auto InstrParams = DynExp::dynamic_Params_cast<FunctionGenerator>(GetNonConstParams());
			InstrParams->WaveformType = FunctionGeneratorDefs::WaveformTypes::Rect;
			InstrParams->FrequencyInHz = FunctionDesc.FrequencyInHz;
			InstrParams->Amplitude = FunctionDesc.Amplitude;
			InstrParams->Offset = FunctionDesc.Offset;
			InstrParams->PhaseInRad = IsPhaseAdjustable() ? FunctionDesc.PhaseInRad : 0;
			InstrParams->DutyCycle = FunctionDesc.DutyCycle;
			InstrParams->Autostart = Autostart;
		} // InstrParams unlocked here.

		MakeAndEnqueueTask<FunctionGeneratorTasks::SetRectFunctionTask>(FunctionDesc, Autostart, CallbackFunc);
		UpdateData();
	}

	void FunctionGenerator::SetRampFunction(const FunctionGeneratorDefs::RampFunctionDescType& FunctionDesc,
		bool PersistParams, bool Autostart, DynExp::TaskBase::CallbackType CallbackFunc) const
	{
		if (!GetWaveformCaps().Test(WaveformCapsType::UserDefined))
			throw Util::NotAvailableException(
				"The function generator of type " + GetCategoryAndName() + " is not capable to generate a user-defined function.", Util::ErrorType::Error);

		if (PersistParams)
		{
			auto InstrParams = DynExp::dynamic_Params_cast<FunctionGenerator>(GetNonConstParams());
			InstrParams->WaveformType = FunctionGeneratorDefs::WaveformTypes::Ramp;
			InstrParams->FrequencyInHz = FunctionDesc.FrequencyInHz;
			InstrParams->Amplitude = FunctionDesc.Amplitude;
			InstrParams->Offset = FunctionDesc.Offset;
			InstrParams->PhaseInRad = IsPhaseAdjustable() ? FunctionDesc.PhaseInRad : 0;
			InstrParams->DutyCycle = FunctionDesc.RiseFallRatio;
			InstrParams->Autostart = Autostart;
		} // InstrParams unlocked here.

		MakeAndEnqueueTask<FunctionGeneratorTasks::SetRampFunctionTask>(FunctionDesc, Autostart, CallbackFunc);
		UpdateData();
	}

	void FunctionGenerator::SetPulseFunction(const FunctionGeneratorDefs::PulsesDescType& FunctionDesc,
		bool PersistParams, bool Autostart, DynExp::TaskBase::CallbackType CallbackFunc) const
	{
		if (!GetWaveformCaps().Test(WaveformCapsType::UserDefined))
			throw Util::NotAvailableException(
				"The function generator of type " + GetCategoryAndName() + " is not capable to generate a user-defined function.", Util::ErrorType::Error);

		// Valid pulse sequency consists of at least two edges.
		if (FunctionDesc.Pulses.size() < 1)
			return;

		if (PersistParams)
		{
			auto InstrParams = DynExp::dynamic_Params_cast<FunctionGenerator>(GetNonConstParams());
			InstrParams->WaveformType = FunctionGeneratorDefs::WaveformTypes::Pulse;
			InstrParams->Offset = FunctionDesc.Offset;

			auto PulsesKeys = std::views::keys(FunctionDesc.Pulses);
			auto PulsesValues = std::views::values(FunctionDesc.Pulses);
			InstrParams->PulseStarts = { PulsesKeys.begin(), PulsesKeys.end() };
			InstrParams->PulseAmplitudes = { PulsesValues.begin(), PulsesValues.end() };

			InstrParams->Autostart = Autostart;
		} // InstrParams unlocked here.

		MakeAndEnqueueTask<FunctionGeneratorTasks::SetPulseFunctionTask>(FunctionDesc, Autostart, CallbackFunc);
		UpdateData();
	}

	void FunctionGenerator::SetArbitraryFunction(DataStreamBase::BasicSampleListType&& Samples,
		bool Autostart, DynExp::TaskBase::CallbackType CallbackFunc) const
	{
		if (!GetWaveformCaps().Test(WaveformCapsType::UserDefined))
			throw Util::NotAvailableException(
				"The function generator of type " + GetCategoryAndName() + " is not capable to generate a user-defined function.", Util::ErrorType::Error);

		MakeAndEnqueueTask<FunctionGeneratorTasks::SetArbitraryFunctionTask>(std::move(Samples), Autostart, CallbackFunc);
		UpdateData();
	}

	void FunctionGenerator::SetModulation(const FunctionGeneratorDefs::ModulationDescType& ModulationDesc,
		bool PersistParams, DynExp::TaskBase::CallbackType CallbackFunc) const
	{
		throw Util::NotImplementedException();
	}

	void FunctionGenerator::SetSweep(const FunctionGeneratorDefs::SweepDescType& SweepDesc,
		bool PersistParams, DynExp::TaskBase::CallbackType CallbackFunc) const
	{
		throw Util::NotImplementedException();
	}

	void FunctionGenerator::SetTrigger(const FunctionGeneratorDefs::TriggerDescType& TriggerDesc,
		bool PersistParams, DynExp::TaskBase::CallbackType CallbackFunc) const
	{
		if (PersistParams)
		{
			auto InstrParams = DynExp::dynamic_Params_cast<FunctionGenerator>(GetNonConstParams());
			InstrParams->TriggerMode = TriggerDesc.TriggerMode;
			InstrParams->TriggerEdge = TriggerDesc.TriggerEdge;
		} // InstrParams unlocked here.

		// CallbackFunc is to be called by task issued by overridden SetTriggerChild().
		MakeAndEnqueueTask<FunctionGeneratorTasks::SetTriggerTask>(TriggerDesc);

		SetTriggerChild(TriggerDesc, PersistParams, CallbackFunc);
	}

	void FunctionGenerator::ForceTrigger(DynExp::TaskBase::CallbackType CallbackFunc) const
	{
		throw Util::NotImplementedException();
	}

	void FunctionGenerator::ForceTriggerSync() const
	{
		AsSyncTask(&FunctionGenerator::ForceTrigger);
	}

	void FunctionGenerator::ResetImpl(dispatch_tag<DataStreamInstrument>)
	{
		ResetImpl(dispatch_tag<FunctionGenerator>());
	}

	void FunctionGenerator::SetTriggerChild(const FunctionGeneratorDefs::TriggerDescType& TriggerDesc,
		bool PersistParams, DynExp::TaskBase::CallbackType CallbackFunc) const
	{
		throw Util::NotImplementedException();
	}
}