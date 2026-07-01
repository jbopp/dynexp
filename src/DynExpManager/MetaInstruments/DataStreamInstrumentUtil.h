// This file is part of DynExp.

/**
 * @file DataStreamInstrumentUtil.h
 * @brief Helper classes and functions for data stream meta instrument that
 * do not depend on further DynExp files and therefore can be used independently.
*/

#pragma once

namespace DynExpInstr::DataStreamInstr
{
	/**
	 * @brief Units which can be used for data stream instruments.
	 * @warning If this is changed, also change @p ToPrototUnitType(DataStreamInstrumentData::UnitType) and
	 * @p ToDataStreamInstrumentUnitType(DynExpProto::Common::IntensityUnitType) functions in
	 * @p NetworkDataStreamInstrument.h and @p IntensityUnitType enumeration in @p Common.proto.
	*/
	enum class UnitType {
		Arbitrary,		//!< Arbitrary units (a.u.)
		LogicLevel,		//!< Logic level (TTL) units (1 or 0)
		Counts,			//!< Count rate in counts per second (cps)
		Volt,			//!< Voltage in Volt (V)
		Ampere,			//!< Electric current in Ampere (A)
		Power_W,		//!< Power in Watt (W)
		Power_dBm		//!< Power in dBm
	};

	/**
	 * @brief Returns a descriptive string of a respective unit to be e.g. used in plots.
	 * @param Unit Unit type as used by data stream instruments.
	 * @return Returns the human-readable unit string.
	*/
	const char* UnitTypeToStr(const UnitType& Unit);
}