// This file is part of DynExp.

#include "stdafx.h"
#include "Spectrometer.h"

namespace DynExpInstr
{
	const char* SpectrometerData::FrequencyUnitTypeToStr(const FrequencyUnitType& Unit)
	{
		switch (Unit)
		{
		case FrequencyUnitType::Hz: return "Hz";
		case FrequencyUnitType::nm: return "nm";
		case FrequencyUnitType::Inv_cm: return "1/cm";
		default: return "<unknown unit>";
		}
	}

	const char* SpectrometerData::IntensityUnitTypeToStr(const IntensityUnitType& Unit)
	{
		switch (Unit)
		{
		case IntensityUnitType::Counts: return "#";
		default: return "<unknown unit>";
		}
	}

	SpectrometerData::SpectrumType::SpectrumType(SpectrumType&& Other)
		: FrequencyUnit(Other.FrequencyUnit), IntensityUnit(Other.IntensityUnit), Samples(std::move(Other.Samples))
	{
		Other.Reset();
	}

	SpectrometerData::SpectrumType& SpectrometerData::SpectrumType::operator=(const SpectrumType& Other)
	{
		FrequencyUnit = Other.FrequencyUnit;
		IntensityUnit = Other.IntensityUnit;
		Samples = Other.Samples;

		return *this;
	}

	SpectrometerData::SpectrumType& SpectrometerData::SpectrumType::operator=(SpectrumType&& Other)
	{
		FrequencyUnit = Other.FrequencyUnit;
		IntensityUnit = Other.IntensityUnit;
		Samples = std::move(Other.Samples);

		Other.Reset();

		return *this;
	}

	void SpectrometerData::SpectrumType::Reset()
	{
		Samples.clear();
	}

	SpectrometerData::SpectrumType SpectrometerData::GetSpectrum() const
	{
		return std::move(CurrentSpectrum);
	}

	SpectrometerData::SpectrumType SpectrometerData::GetSpectrumCopy() const
	{
		return CurrentSpectrum;
	}

	void SpectrometerData::SetSpectrum(SpectrumType&& Other)
	{
		CurrentSpectrum = std::move(Other);
	}

	void SpectrometerData::ClearSpectrum() const
	{
		CurrentSpectrum.Reset();
	}

	void SpectrometerData::ResetImpl(dispatch_tag<InstrumentDataBase>)
	{
		MinExposureTime = TimeType();
		MaxExposureTime = TimeType();
		CurrentExposureTime = TimeType();
		CurrentLowerFrequency = 0.0;
		CurrentUpperFrequency = 0.0;
		SilentModeEnabled = false;

		CurrentSpectrum.Reset();

		ResetImpl(dispatch_tag<SpectrometerData>());
	}

	SpectrometerParams::~SpectrometerParams()
	{
	}

	SpectrometerConfigurator::~SpectrometerConfigurator()
	{
	}

	Spectrometer::~Spectrometer()
	{
	}

	void Spectrometer::SetSilentMode(bool Enable, DynExp::TaskBase::CallbackType CallbackFunc) const
	{
	}

	void Spectrometer::Abort(DynExp::TaskBase::CallbackType CallbackFunc) const
	{
		throw Util::NotImplementedException();
	}

	void Spectrometer::ResetImpl(dispatch_tag<InstrumentBase>)
	{
		ResetImpl(dispatch_tag<Spectrometer>());
	}
}