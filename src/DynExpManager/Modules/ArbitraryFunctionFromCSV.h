// This file is part of DynExp.

/**
 * @file ArbitraryFunctionFromCSV.h
 * @brief Implementation of a module to read a waveform from a CSV file and to store it in a
 * data stream of a function generator instrument.
*/

#pragma once

#include "stdafx.h"
#include "DynExpCore.h"
#include "../MetaInstruments/FunctionGenerator.h"

namespace DynExpModule
{
	class ArbitraryFunctionFromCSV;

	class ArbitraryFunctionFromCSVData : public DynExp::ModuleDataBase
	{
	public:
		ArbitraryFunctionFromCSVData() { Init(); }
		virtual ~ArbitraryFunctionFromCSVData() = default;

		DynExp::LinkedObjectWrapperContainer<DynExpInstr::FunctionGenerator> FunctionGenerator;

	private:
		void ResetImpl(dispatch_tag<ModuleDataBase>) override final;
		virtual void ResetImpl(dispatch_tag<ArbitraryFunctionFromCSVData>) {};

		void Init();
	};

	class ArbitraryFunctionFromCSVParams : public DynExp::ModuleParamsBase
	{
	public:
		ArbitraryFunctionFromCSVParams(DynExp::ItemIDType ID, const DynExp::DynExpCore& Core) : ModuleParamsBase(ID, Core) {}
		virtual ~ArbitraryFunctionFromCSVParams() = default;

		virtual const char* GetParamClassTag() const noexcept override { return "ArbitraryFunctionFromCSVParams"; }

		Param<DynExp::ObjectLink<DynExpInstr::FunctionGenerator>> FunctionGenerator = { *this, GetCore().GetInstrumentManager(),
			"FunctionGenerator", "Function generator", "Underlying data stream instrument to be used as a function generator", DynExpUI::Icons::Instrument };
		Param<ParamsConfigDialog::TextType> CSVDataPath = { *this, "CSVDataPath", "CSV data path",
			"Path to a CSV file containing the arbitrary function data as a single column (values) or as two columns (times;values)", true, "", DynExp::TextUsageType::Code };
		Param<ParamsConfigDialog::NumberType> SkipLines = { *this, "SkipLines", "Skip lines",
			"Number of (header) lines to skip at the beginning of the file", true,
			0, 0, std::numeric_limits<ParamsConfigDialog::NumberType>::max(), 1, 0 };
		Param<ParamsConfigDialog::NumberType> TimeStretch = { *this, "TimeStretch", "Time stretch",
			"Factor to multiply all times with", true,
			1, std::numeric_limits<ParamsConfigDialog::NumberType>::lowest(), std::numeric_limits<ParamsConfigDialog::NumberType>::max(), .1, 3 };
		Param<ParamsConfigDialog::NumberType> TimeOffset = { *this, "TimeOffset", "Time offset",
			"Offset to add to all times", true,
			0, std::numeric_limits<ParamsConfigDialog::NumberType>::lowest(), std::numeric_limits<ParamsConfigDialog::NumberType>::max(), .1, 3 };
		Param<ParamsConfigDialog::NumberType> ValueStretch = { *this, "ValueStretch", "Value stretch",
			"Factor to multiply all values with", true,
			1, std::numeric_limits<ParamsConfigDialog::NumberType>::lowest(), std::numeric_limits<ParamsConfigDialog::NumberType>::max(), .1, 3 };
		Param<ParamsConfigDialog::NumberType> ValueOffset = { *this, "ValueOffset", "Value offset",
			"Offset to add to all values", true,
			0, std::numeric_limits<ParamsConfigDialog::NumberType>::lowest(), std::numeric_limits<ParamsConfigDialog::NumberType>::max(), .1, 3 };

	private:
		void ConfigureParamsImpl(dispatch_tag<ModuleParamsBase>) override final {}
	};

	class ArbitraryFunctionFromCSVConfigurator : public DynExp::ModuleConfiguratorBase
	{
	public:
		using ObjectType = ArbitraryFunctionFromCSV;
		using ParamsType = ArbitraryFunctionFromCSVParams;

		ArbitraryFunctionFromCSVConfigurator() = default;
		virtual ~ArbitraryFunctionFromCSVConfigurator() = default;

	private:
		virtual DynExp::ParamsBasePtrType MakeParams(DynExp::ItemIDType ID, const DynExp::DynExpCore& Core) const override final { return DynExp::MakeParams<ArbitraryFunctionFromCSVConfigurator>(ID, Core); }
	};

	class ArbitraryFunctionFromCSV : public DynExp::ModuleBase
	{
	public:
		using ParamsType = ArbitraryFunctionFromCSVParams;
		using ConfigType = ArbitraryFunctionFromCSVConfigurator;
		using ModuleDataType = ArbitraryFunctionFromCSVData;

		constexpr static auto Name() noexcept { return "Arbitrary Function from CSV"; }
		constexpr static auto Category() noexcept { return "I/O"; }

		ArbitraryFunctionFromCSV(const std::thread::id OwnerThreadID, DynExp::ParamsBasePtrType&& Params)
			: ModuleBase(OwnerThreadID, std::move(Params)) {}
		virtual ~ArbitraryFunctionFromCSV() = default;

		virtual std::string GetName() const override { return Name(); }
		virtual std::string GetCategory() const override { return Category(); }

		bool TreatModuleExceptionsAsWarnings() const override { return false; }

		// Only run main loop in case of an event.
		std::chrono::milliseconds GetMainLoopDelay() const override final { return decltype(ModuleBase::GetMainLoopDelay())::max(); }

	private:
		Util::DynExpErrorCodes::DynExpErrorCodes ModuleMainLoop(DynExp::ModuleInstance& Instance) override final;

		void ResetImpl(dispatch_tag<ModuleBase>) override final;

		// Events, run in module thread
		void OnInit(DynExp::ModuleInstance* Instance) const override final;
		void OnExit(DynExp::ModuleInstance* Instance) const override final;
	};
}