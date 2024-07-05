// This file is part of DynExp.

/**
 * @file PyModules.h
 * @brief Implementation of mappings between %DynExp instruments and corresponding embedded
 * Python modules making the instrument data available in Python.
*/

#pragma once

#include "stdafx.h"
#include "MetaInstruments/DataStreamInstrument.h"

namespace DynExpInstr
{
	/**
	 * @brief Python mapping of a DynExpInstr::DataStreamInstrument's DynExpInstr::DataStreamBase instance.
	*/
	class PyDataStreamInstrument
	{
	public:
		/**
		 * @brief Make the Python interpreter import this module as @p PyModuleDataStreamInstrument.
		 * GIL has to be acquired before.
		*/
		static void import();

		/**
		 * @brief Allows to calculate the ID of the last consumed sample based on the amount of samples consumed from the stream.
		 * @param NumConsumedSamples Number of samples consumed from @p Samples 
		 * @return ID of the last consumed sample
		*/
		size_t CalcLastConsumedSampleID(size_t NumConsumedSamples);

		/**
		 * @brief Contains the result of DynExpInstr::DataStreamBase::IsBasicSampleTimeUsed().
		*/
		bool IsTimeUsed{};
		
		/**
		 * @brief Refer to DynExpInstr::DataStreamInstrumentData::UnitType.
		*/
		DataStreamInstrumentData::UnitType ValueUnit{DataStreamInstrumentData::UnitType::Arbitrary};

		/**
		 * @brief Contains the result of DynExpInstr::DataStreamBase::GetStreamSizeRead().
		*/
		size_t StreamSizeRead{};

		/**
		 * @brief Contains the result of DynExpInstr::DataStreamBase::GetStreamSizeWrite().
		*/
		size_t StreamSizeWrite{};

		/**
		 * @brief Contains the result of DynExpInstr::DataStreamBase::GetNumSamplesWritten().
		*/
		size_t NumSamplesWritten{};

		/**
		 * @brief Samples of the data stream instrument
		*/
		DataStreamBase::BasicSampleListType Samples;
	};
}