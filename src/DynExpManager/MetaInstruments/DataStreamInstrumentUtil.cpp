// This file is part of DynExp.

#include "DataStreamInstrumentUtil.h"

namespace DynExpInstr::DataStreamInstr
{
	const char* UnitTypeToStr(const UnitType& Unit)
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
}