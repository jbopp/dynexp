// This file is part of DynExp.

#include "stdafx.h"
#include "Stage.h"

namespace DynExpInstr
{
	void PositionerStageData::ResetImpl(dispatch_tag<InstrumentDataBase>)
	{
		Position = 0;
		Velocity = 0;

		ResetImpl(dispatch_tag<PositionerStageData>());
	}

	PositionerStageParams::~PositionerStageParams()
	{
	}

	PositionerStageConfigurator::~PositionerStageConfigurator()
	{
	}

	PositionerStage::~PositionerStage()
	{
	}

	void PositionerStage::SetHome() const
	{
		throw Util::NotImplementedException();
	}

	void PositionerStage::Reference(DirectionType Direction, DynExp::TaskBase::CallbackType CallbackFunc) const
	{
		throw Util::NotImplementedException();
	}

	void PositionerStage::Calibrate(DynExp::TaskBase::CallbackType CallbackFunc) const
	{
		throw Util::NotImplementedException();
	}

	void PositionerStage::MoveToHome(DynExp::TaskBase::CallbackType CallbackFunc) const
	{
		throw Util::NotImplementedException();
	}

	void PositionerStage::ResetImpl(dispatch_tag<InstrumentBase>)
	{
		ResetImpl(dispatch_tag<PositionerStage>());
	}
}