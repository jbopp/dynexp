// This file is part of DynExp.

/**
 * @file Units.h
 * @brief Physical units used in DynExp. This file does not depend on further DynExp files and
 * therefore can be used independently.
*/

#pragma once

namespace DynExp
{
	/**
	 * @brief Units which can be used with DynExp instruments.
	 * @warning If this is changed, also update
	 * DynExpInstr::ToProtoFrequencyUnitType(DynExp::UnitType) in @p gRPCInstrument.h and
	 * DynExpInstr::ToProtoIntensityUnitType(DynExp::UnitType) in @p gRPCInstrument.h and
	 * DynExpInstr::ToDataStreamInstrumentUnitType(DynExpProto::Common::IntensityUnitType) in @p NetworkDataStreamInstrument.h and
	 * DynExpInstr::ToLaserUnitType(DynExpProto::Common::FrequencyUnitType) in @p NetworkLaser.h and
	 * DynExpInstr::ToLaserUnitType(DynExpProto::Common::IntensityUnitType) in @p NetworkLaser.h and
	 * DynExpInstr::ToSpectrometerUnitType(DynExpProto::Common::FrequencyUnitType) in @p NetworkSpectrometer.h and
	 * DynExpInstr::ToSpectrometerUnitType(DynExpProto::Common::IntensityUnitType) in @p NetworkSpectrometer.h and
	 * @p FrequencyUnitType enumeration in @p Common.proto and
	 * @p IntensityUnitType enumeration in @p Common.proto.
	*/
	enum class UnitType {
		// Intensity-like
		Arbitrary,		//!< Arbitrary units (a.u.)
		LogicLevel,		//!< Logic level (TTL) units (1 or 0)
		Counts,			//!< Count rate in counts per second (cps)
		Volt,			//!< Voltage in Volt (V)
		Ampere,			//!< Electric current in Ampere (A)
		Power_W,		//!< Power in Watt (W)
		Power_dBm,		//!< Power in dBm
		//
		// Time-like
		Time_s,			//!< Time in s
		Time_ms,		//!< Time in ms
		Time_us,		//!< Time in us
		Time_ns,		//!< Time in ns
		Time_ps,		//!< Time in ps
		//
		// Frequency-like
		Freq_Hz,		//!< Frequency in Hz
		Length_nm,		//!< Wavelength in nm
		Inv_cm			//!< Wavenumber in 1/cm
	};

	/**
	 * @brief Returns a descriptive string of a respective unit to be e.g. used in plots.
	 * @param Unit Unit type as used by DynExp instruments.
	 * @return Returns the human-readable unit string.
	*/
	const char* UnitTypeToStr(const UnitType& Unit);

	/**
	 * @brief Speed of light in vacuum in m/s
	*/
	static constexpr double SpeedOfLight = 2.99792458e8;
}