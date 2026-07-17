// This file is part of DynExp.

#include "stdafx.h"
#include "Laser.h"

namespace DynExpInstr
{
	void LaserData::ResetImpl(dispatch_tag<InstrumentDataBase>)
	{
		Frequency = 0.0;		
		Intensity = 0.0;		
		ScanRange = 0.0;				
		ScanRate = 0.0;				

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