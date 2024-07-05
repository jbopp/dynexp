// This file is part of DynExp.

#include "stdafx.h"
#include "ArbitraryFunctionFromCSV.h"

namespace DynExpModule
{

	void ArbitraryFunctionFromCSVData::ResetImpl(dispatch_tag<ModuleDataBase>)
	{
		Init();
	}

	void ArbitraryFunctionFromCSVData::Init()
	{
	}

	Util::DynExpErrorCodes::DynExpErrorCodes ArbitraryFunctionFromCSV::ModuleMainLoop(DynExp::ModuleInstance& Instance)
	{
		return Util::DynExpErrorCodes::NoError;
	}

	void ArbitraryFunctionFromCSV::ResetImpl(dispatch_tag<ModuleBase>)
	{
	}

	void ArbitraryFunctionFromCSV::OnInit(DynExp::ModuleInstance* Instance) const
	{
		std::string CSVDataPath;
		size_t SkipLines{};
		double TimeStretch{}, TimeOffset{}, ValueStretch{}, ValueOffset{};

		{
			auto ModuleParams = DynExp::dynamic_Params_cast<ArbitraryFunctionFromCSV>(Instance->ParamsGetter());
			auto ModuleData = DynExp::dynamic_ModuleData_cast<ArbitraryFunctionFromCSV>(Instance->ModuleDataGetter());

			Instance->LockObject(ModuleParams->FunctionGenerator, ModuleData->FunctionGenerator);

			CSVDataPath = ModuleParams->CSVDataPath.GetPath().string();
			SkipLines = ModuleParams->SkipLines;
			TimeStretch = ModuleParams->TimeStretch;
			TimeOffset = ModuleParams->TimeOffset;
			ValueStretch = ModuleParams->ValueStretch;
			ValueOffset = ModuleParams->ValueOffset;
		} // ModuleParams and ModuleData unlocked here.

		// Reading and parsing potentially very heavy. So, unlock every mutex before.
		auto CSVData = Util::ReadFromFile(CSVDataPath);

		std::vector<DynExpInstr::BasicSample> BasicSamples;
		try
		{
			try
			{
				// Try to parse format time;value...
				auto ParsedCSV = Util::ParseCSV<double, double>(CSVData, ';', SkipLines);

				for (const auto& ParsedTuple : ParsedCSV)
					BasicSamples.emplace_back(std::get<1>(ParsedTuple) * ValueStretch + ValueOffset, std::get<0>(ParsedTuple) * TimeStretch + TimeOffset);
			}
			catch ([[maybe_unused]] std::ios_base::failure& e)
			{
				// ...did not work. So, try to parse list of values. 
				auto ParsedCSV = Util::ParseCSV<double>(CSVData, ';', SkipLines);

				for (const auto& ParsedTuple : ParsedCSV)
					BasicSamples.emplace_back(std::get<0>(ParsedTuple) * ValueStretch + ValueOffset);
			}
		}
		catch ([[maybe_unused]] std::ios_base::failure& e)
		{
			throw Util::InvalidDataException("Error parsing the CSV file \"" + CSVDataPath + "\".");
		}

		auto ModuleData = DynExp::dynamic_ModuleData_cast<ArbitraryFunctionFromCSV>(Instance->ModuleDataGetter());
		ModuleData->FunctionGenerator->Clear();
		ModuleData->FunctionGenerator->SetArbitraryFunction(std::move(BasicSamples), true);
	}

	void ArbitraryFunctionFromCSV::OnExit(DynExp::ModuleInstance* Instance) const
	{
		auto ModuleData = DynExp::dynamic_ModuleData_cast<ArbitraryFunctionFromCSV>(Instance->ModuleDataGetter());

		ModuleData->FunctionGenerator->Stop();
		Instance->UnlockObject(ModuleData->FunctionGenerator);
	}
}