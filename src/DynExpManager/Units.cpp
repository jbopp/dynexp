// This file is part of DynExp.

#include "Units.h"

namespace DynExp
{
	bool IsIntegerUnit(UnitType Unit)
	{
		switch (Unit)
		{
		case UnitType::LogicLevel:
		case UnitType::Counts:
		case UnitType::Index: return true;
		default: return false;
		}
	}

	bool IsIntensityUnit(UnitType Unit)
	{
		switch (Unit)
		{
		case UnitType::Arbitrary:
		case UnitType::LogicLevel:
		case UnitType::Counts:
		case UnitType::Volt:
		case UnitType::Ampere:
		case UnitType::Power_W:
		case UnitType::Power_dBm: return true;
		default: return false;
		}
	}

	bool IsTimeUnit(UnitType Unit)
	{
		switch (Unit)
		{
		case UnitType::Index:
		case UnitType::Time_s:
		case UnitType::Time_ms:
		case UnitType::Time_us:
		case UnitType::Time_ns:
		case UnitType::Time_ps: return true;
		default: return false;
		}
	}

	bool IsTimeUnitStrict(UnitType Unit)
	{
		return IsTimeUnit(Unit) && Unit != UnitType::Index;
	}

	bool IsFrequencyUnit(UnitType Unit)
	{
		switch (Unit)
		{
		case UnitType::Freq_Hz:
		case UnitType::Inv_cm:
		case UnitType::Wavelength_nm: return true;
		default: return false;
		}
	}

	const char* UnitCategoryToStr(UnitType Unit)
	{
		switch (Unit)
		{
		case UnitType::Arbitrary:
		case UnitType::LogicLevel:
		case UnitType::Counts:
		case UnitType::Volt:
		case UnitType::Ampere:
		case UnitType::Power_W:
		case UnitType::Power_dBm: return "intensity";
		case UnitType::Index: return "sample";
		case UnitType::Time_s:
		case UnitType::Time_ms:
		case UnitType::Time_us:
		case UnitType::Time_ns:
		case UnitType::Time_ps: return "time";
		case UnitType::Freq_Hz:
		case UnitType::Inv_cm: return "frequency";
		case UnitType::Wavelength_nm: return "wavelength";
		default: return "<unknown unit category>";
		}
	}

	const char* UnitTypeToStr(UnitType Unit)
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
		case UnitType::Index: return "i";
		case UnitType::Time_s: return "s";
		case UnitType::Time_ms: return "ms";
		case UnitType::Time_us: return "us";
		case UnitType::Time_ns: return "ns";
		case UnitType::Time_ps: return "ps";
		case UnitType::Freq_Hz: return "Hz";
		case UnitType::Inv_cm: return "1/cm";
		case UnitType::Wavelength_nm: return "nm";
		default: return "<unknown unit>";
		}
	}
}