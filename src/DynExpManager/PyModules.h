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
		 * @brief Default-constructs a PyDataStreamInstrument object.
		*/
		PyDataStreamInstrument() noexcept = default;

		/** @name Not mapped to Python
		 * These functions cannot be accessed through the Python interface.
		*/
		///@{
		/**
		 * @brief Constructs a PyDataStreamInstrument object.
		 * @param IsTimeUsed @copydoc #IsTimeUsed
		 * @param ValueUnit @copydoc #ValueUnit
		*/
		PyDataStreamInstrument(bool IsTimeUsed, DynExp::Units::UnitType ValueUnit) noexcept
			: IsTimeUsed(IsTimeUsed), ValueUnit(ValueUnit) {}

		/**
		 * @brief Resets #ShouldClearFlag to false.
		*/
		void Cleared() { ShouldClearFlag = false; }

		/**
		 * @brief Getter for #ShouldClearFlag.
		*/
		auto ShouldCLear() const noexcept { return ShouldClearFlag; }
		///@}

		/**
		 * @brief Allows to calculate the ID of the last consumed sample based on the number of samples consumed from the stream.
		 * @param NumConsumedSamples Number of samples consumed from @p Samples 
		 * @return ID of the last consumed sample
		*/
		size_t CalcLastConsumedSampleID(size_t NumConsumedSamples) const;

		/**
		 * @brief Allows to calculate the ID of the last consumed sample for the situation that no samples was consumed.
		 * @return ID of the last consumed sample
		*/
		size_t ConsumeNone() const { return CalcLastConsumedSampleID(0); }

		/**
		 * @brief Allows to calculate the ID of the last consumed sample for the situation that all samples were consumed.
		 * @return ID of the last consumed sample
		*/
		size_t ConsumeAll() const;

		/**
		 * @brief Requests a call to DataStreamBase::Clear() on the related data stream instrument.
		 * Sets #ShouldClearFlag to true.
		*/
		void Clear() { ShouldClearFlag = true; }

		/**
		 * @brief Contains the result of DynExpInstr::DataStreamBase::IsBasicSampleTimeUsed().
		*/
		bool IsTimeUsed{ false };
		
		/**
		 * @brief Refer to DynExp::Units::UnitType.
		*/
		DynExp::Units::UnitType ValueUnit{ DynExp::Units::UnitType::Arbitrary };

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

	private:
		/**
		 * @brief Indicates whether DataStreamBase::Clear() should be called on the related data stream instrument.
		*/
		bool ShouldClearFlag{ false };
	};
}