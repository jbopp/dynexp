// This file is part of DynExp.

#include "stdafx.h"
#include "Laser.h"

namespace DynExpInstr
{
	const char* LaserData::FrequencyUnitTypeToStr(const FrequencyUnitType& Unit)
	{
		switch (Unit)
		{
		case FrequencyUnitType::Hz: return "Hz";
		case FrequencyUnitType::nm: return "nm";
		default: return "<unknown unit>";
		}
	}

	const char* LaserData::IntensityUnitTypeToStr(const IntensityUnitType& Unit)
	{
		switch (Unit)
		{
		case IntensityUnitType::Power_W: return "W";
		default: return "<unknown unit>";
		}
	}

	void LaserData::ResetImpl(dispatch_tag<InstrumentDataBase>)
	{
		double CurrentFrequency = 0.0;		
		double CurrentIntensity = 0.0;		
		double ScanRange = 0.0;				
		double ScanRate = 0.0;				

		ResetImpl(dispatch_tag<LaserData>());
	}

	LaserParams::~LaserParams()
	{
	}

	LaserConfigurator::~LaserConfigurator()
	{
	}

	Laser::~Laser()
	{
	}

	void Laser::ScanContinuously(DynExp::TaskBase::CallbackType CallbackFunc) const
	{
		throw Util::NotImplementedException();
	}

	void Laser::DisableScan(DynExp::TaskBase::CallbackType CallbackFunc) const
	{
		throw Util::NotImplementedException();
	}

	void Laser::ResetImpl(dispatch_tag<InstrumentBase>)
	{
		ResetImpl(dispatch_tag<Laser>());
	}
}