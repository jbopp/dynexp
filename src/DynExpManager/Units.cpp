// This file is part of DynExp.

#include "Units.h"

namespace DynExp
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
		case UnitType::Time_s: return "s";
		case UnitType::Time_ms: return "ms";
		case UnitType::Time_us: return "us";
		case UnitType::Time_ns: return "ns";
		case UnitType::Time_ps: return "ps";
		case UnitType::Freq_Hz: return "Hz";
		case UnitType::Length_nm: return "nm";
		case UnitType::Inv_cm: return "1/cm";
		default: return "<unknown unit>";
		}
	}
}