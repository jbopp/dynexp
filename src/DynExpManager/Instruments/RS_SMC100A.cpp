// This file is part of DynExp.

#include "stdafx.h"
#include "RS_SMC100A.h"

namespace DynExpInstr
{
	void RS_SMC100ATasks::InitTask::InitFuncImpl(dispatch_tag<FunctionGeneratorTasks::InitTask>, DynExp::InstrumentInstance& Instance)
	{
		auto InstrParams = DynExp::dynamic_Params_cast<RS_SMC100A>(Instance.ParamsGetter());
		auto InstrData = DynExp::dynamic_InstrumentData_cast<RS_SMC100A>(Instance.InstrumentDataGetter());

		Instance.LockObject(InstrParams->HardwareAdapter, InstrData->HardwareAdapter);

		InitFuncImpl(dispatch_tag<InitTask>(), Instance);
	}

	void RS_SMC100ATasks::ExitTask::ExitFuncImpl(dispatch_tag<FunctionGeneratorTasks::ExitTask>, DynExp::InstrumentInstance& Instance)
	{
		ExitFuncImpl(dispatch_tag<ExitTask>(), Instance);

		auto InstrData = DynExp::dynamic_InstrumentData_cast<RS_SMC100A>(Instance.InstrumentDataGetter());

		Instance.UnlockObject(InstrData->HardwareAdapter);
	}

	void RS_SMC100ATasks::UpdateTask::UpdateFuncImpl(dispatch_tag<FunctionGeneratorTasks::UpdateTask>, DynExp::InstrumentInstance& Instance)
	{
		try
		{
			auto InstrData = DynExp::dynamic_InstrumentData_cast<RS_SMC100A>(Instance.InstrumentDataGetter());
			bool UpdateError = false;

			try
			{
				*InstrData->HardwareAdapter << "OUTPut1:STATe?";
				InstrData->Running = InstrData->HardwareAdapter->WaitForLine(3) == "1";
			}
			catch ([[maybe_unused]] const DynExp::SerialCommunicationException& e)
			{
				UpdateError = true;

				// Swallow if just one or two subsequent updates failed.
				if (InstrData->NumFailedStatusUpdateAttempts++ >= 3)
					throw;
			}

			if (!UpdateError)
				InstrData->NumFailedStatusUpdateAttempts = 0;
		}
		// Issued if a mutex is blocked by another operation.
		catch (const Util::TimeoutException& e)
		{
			Instance.GetOwner().SetWarning(e);

			return;
		}
		// Issued if reading data from RS_SMC100A controller failed.
		catch (const DynExp::SerialCommunicationException& e)
		{
			Instance.GetOwner().SetWarning(e);

			return;
		}

		UpdateFuncImpl(dispatch_tag<UpdateTask>(), Instance);
	}

	DynExp::TaskResultType RS_SMC100ATasks::StartTask::RunChild(DynExp::InstrumentInstance& Instance)
	{
		auto InstrData = DynExp::dynamic_InstrumentData_cast<RS_SMC100A>(Instance.InstrumentDataGetter());

		*InstrData->HardwareAdapter << "OUTPut1:STATe ON";

		return {};
	}

	DynExp::TaskResultType RS_SMC100ATasks::StopTask::RunChild(DynExp::InstrumentInstance& Instance)
	{
		auto InstrData = DynExp::dynamic_InstrumentData_cast<RS_SMC100A>(Instance.InstrumentDataGetter());

		*InstrData->HardwareAdapter << "OUTPut1:STATe OFF";

		return {};
	}

	DynExp::TaskResultType RS_SMC100ATasks::SetSineFunctionTask::RunChild(DynExp::InstrumentInstance& Instance)
	{
		auto InstrData = DynExp::dynamic_InstrumentData_cast<RS_SMC100A>(Instance.InstrumentDataGetter());

		*InstrData->HardwareAdapter << "SOURce1:FREQuency:MODE CW";
		*InstrData->HardwareAdapter << "SOURce1:FREQuency:CW " + Util::ToStr(FunctionDesc.FrequencyInHz);
		*InstrData->HardwareAdapter << "SOURce1:POWer:MODE CW";
		*InstrData->HardwareAdapter << "SOURce1:POWer:POWer " + Util::ToStr(FunctionDesc.Amplitude);

		if (Autostart)
			InstrData->EnqueueTask(DynExp::MakeTask<RS_SMC100ATasks::StartTask>(nullptr));

		return {};
	}

	DynExp::TaskResultType RS_SMC100ATasks::SetModulationTask::RunChild(DynExp::InstrumentInstance& Instance)
	{
		auto InstrData = DynExp::dynamic_InstrumentData_cast<RS_SMC100A>(Instance.InstrumentDataGetter());

		if (ModulationDesc.Type != FunctionGeneratorDefs::ModulationDescType::ModulationType::Disabled &&
			ModulationDesc.Shape == FunctionGeneratorDefs::ModulationDescType::ModulationShapeType::Sine)
		{
			*InstrData->HardwareAdapter << std::string("SOURce1:LFOutput1:FREQuency:MODE CW");
			*InstrData->HardwareAdapter << std::string("SOURce1:LFOutput1:FREQuency ")
				+ Util::ToStr(ModulationDesc.FrequencyInHz, 0) + " Hz";
		}
		else if (ModulationDesc.Type != FunctionGeneratorDefs::ModulationDescType::ModulationType::Disabled &&
			ModulationDesc.Shape == FunctionGeneratorDefs::ModulationDescType::ModulationShapeType::Pulse)
			throw Util::NotImplementedException("This device does not support LF pulse modulation.");

		if (ModulationDesc.Type == FunctionGeneratorDefs::ModulationDescType::ModulationType::Frequency)
		{
			*InstrData->HardwareAdapter << std::string("SOURce1:FM1:SOURce INT");
			*InstrData->HardwareAdapter << std::string("SOURce1:FM1:DEViation ")
				+ Util::ToStr(ModulationDesc.Depth, 0) + " Hz";

			*InstrData->HardwareAdapter << "SOURce1:AM1:STATe 0";
			*InstrData->HardwareAdapter << "SOURce1:PM1:STATe 0";
			*InstrData->HardwareAdapter << "SOURce1:FM1:STATe 1";
		}
		else if (ModulationDesc.Type == FunctionGeneratorDefs::ModulationDescType::ModulationType::Amplitude)
		{
			*InstrData->HardwareAdapter << std::string("SOURce1:AM1:SOURce INT");
			*InstrData->HardwareAdapter << std::string("SOURce1:AM1:DEPTh ")
				+ Util::ToStr(ModulationDesc.Depth, 0);

			*InstrData->HardwareAdapter << "SOURce1:FM1:STATe 0";
			*InstrData->HardwareAdapter << "SOURce1:PM1:STATe 0";
			*InstrData->HardwareAdapter << "SOURce1:AM1:STATe 1";
		}
		else if (ModulationDesc.Type == FunctionGeneratorDefs::ModulationDescType::ModulationType::Phase)
		{
			*InstrData->HardwareAdapter << std::string("SOURce1:PM1:SOURce INT");
			*InstrData->HardwareAdapter << std::string("SOURce1:PM1:DEViation ")
				+ Util::ToStr(ModulationDesc.Depth, 0);

			*InstrData->HardwareAdapter << "SOURce1:AM1:STATe 0";
			*InstrData->HardwareAdapter << "SOURce1:FM1:STATe 0";
			*InstrData->HardwareAdapter << "SOURce1:PM1:STATe 1";
		}
		else
		{
			*InstrData->HardwareAdapter << "SOURce1:AM1:STATe 0";
			*InstrData->HardwareAdapter << "SOURce1:FM1:STATe 0";
			*InstrData->HardwareAdapter << "SOURce1:PM1:STATe 0";
		}

		*InstrData->HardwareAdapter << std::string("SOURce1:LFOutput1:STATe ") +
			(ModulationDesc.Type == FunctionGeneratorDefs::ModulationDescType::ModulationType::Disabled ? "0" : "1");

		return {};
	}

	DynExp::TaskResultType RS_SMC100ATasks::SetSweepTask::RunChild(DynExp::InstrumentInstance& Instance)
	{
		auto InstrData = DynExp::dynamic_InstrumentData_cast<RS_SMC100A>(Instance.InstrumentDataGetter());

		if (SweepDesc.Type == FunctionGeneratorDefs::SweepDescType::SweepType::Frequency)
		{
			*InstrData->HardwareAdapter << std::string("SOURce1:SWEep:FREQuency:RETRace ")
				+ (SweepDesc.Retrace ? "1" : "0");
			*InstrData->HardwareAdapter << std::string("SOURce1:SWEep:FREQuency:DWELl ")
				+ Util::ToStr(SweepDesc.TimeDiffPerSample_ms, 3) + " ms";
			*InstrData->HardwareAdapter << std::string("SOURce1:FREQuency:STARt ")
				+ Util::ToStr(SweepDesc.Min, 0) + " Hz";
			*InstrData->HardwareAdapter << std::string("SOURce1:FREQuency:STOP ")
				+ Util::ToStr(SweepDesc.Max, 0) + " Hz";
			*InstrData->HardwareAdapter << std::string("SOURce1:SWEep:FREQuency:SPACing LINear");
			*InstrData->HardwareAdapter << std::string("SOURce1:SWEep:FREQuency:SHAPe SAWTooth");
			*InstrData->HardwareAdapter << std::string("SOURce1:SWEep:FREQuency:STEP:LINear ")
				+ Util::ToStr(SweepDesc.ValueDiffPerSample, 0) + " Hz";

			*InstrData->HardwareAdapter << "SOURce1:POWer:MODE CW";
			*InstrData->HardwareAdapter << "SOURce1:FREQuency:MODE SWEep";

			InstrData->CurrentSweepType = FunctionGeneratorDefs::SweepDescType::SweepType::Frequency;
		}
		else if (SweepDesc.Type == FunctionGeneratorDefs::SweepDescType::SweepType::Amplitude)
		{
			*InstrData->HardwareAdapter << std::string("SOURce1:SWEep:POWer:RETRace ")
				+ (SweepDesc.Retrace ? "1" : "0");
			*InstrData->HardwareAdapter << std::string("SOURce1:SWEep:POWer:DWELl ")
				+ Util::ToStr(SweepDesc.TimeDiffPerSample_ms, 3) + " ms";
			*InstrData->HardwareAdapter << std::string("SOURce1:POWer:STARt ")
				+ Util::ToStr(SweepDesc.Min, 2) + " dBm";
			*InstrData->HardwareAdapter << std::string("SOURce1:POWer:STOP ")
				+ Util::ToStr(SweepDesc.Max, 2) + " dBm";
			// The level sweep spacing is always linear.
			*InstrData->HardwareAdapter << std::string("SOURce1:SWEep:POWer:SHAPe SAWTooth");
			*InstrData->HardwareAdapter << std::string("SOURce1:SWEep:POWer:STEP ")
				+ Util::ToStr(SweepDesc.ValueDiffPerSample, 2) + " dB";

			*InstrData->HardwareAdapter << "SOURce1:FREQuency:MODE CW";
			*InstrData->HardwareAdapter << "SOURce1:POWer:MODE SWEep";

			InstrData->CurrentSweepType = FunctionGeneratorDefs::SweepDescType::SweepType::Amplitude;
		}
		else
		{
			*InstrData->HardwareAdapter << "SOURce1:FREQuency:MODE CW";
			*InstrData->HardwareAdapter << "SOURce1:POWer:MODE CW";

			InstrData->CurrentSweepType = FunctionGeneratorDefs::SweepDescType::SweepType::Disabled;
		}

		return {};
	}

	DynExp::TaskResultType RS_SMC100ATasks::ForceTriggerTask::RunChild(DynExp::InstrumentInstance& Instance)
	{
		auto InstrData = DynExp::dynamic_InstrumentData_cast<RS_SMC100A>(Instance.InstrumentDataGetter());

		if (InstrData->GetCurrentSweepType() == FunctionGeneratorDefs::SweepDescType::SweepType::Frequency)
		{
			*InstrData->HardwareAdapter << std::string("TRIGger1:FSWeep:SOURce ")
				+ RS_SMC100A::TriggerModeToCmdString(FunctionGeneratorDefs::TriggerDescType::TriggerModeType::Manual);
			*InstrData->HardwareAdapter << "SOURce1:SWEep:FREQuency:MODE AUTO";

			*InstrData->HardwareAdapter << "SOURce1:SWEep:FREQuency:EXECute";
		}
		else if (InstrData->GetCurrentSweepType() == FunctionGeneratorDefs::SweepDescType::SweepType::Amplitude)
		{
			*InstrData->HardwareAdapter << std::string("TRIGger1:PSWeep:SOURce ")
				+ RS_SMC100A::TriggerModeToCmdString(FunctionGeneratorDefs::TriggerDescType::TriggerModeType::Manual);
			*InstrData->HardwareAdapter << "SOURce1:SWEep:POWer:MODE AUTO";

			*InstrData->HardwareAdapter << "SOURce1:SWEep:POWer:EXECute";
		}

		return {};
	}

	DynExp::TaskResultType RS_SMC100ATasks::SetTriggerTask::RunChild(DynExp::InstrumentInstance& Instance)
	{
		auto InstrData = DynExp::dynamic_InstrumentData_cast<RS_SMC100A>(Instance.InstrumentDataGetter());

		if (InstrData->GetCurrentSweepType() == FunctionGeneratorDefs::SweepDescType::SweepType::Frequency)
		{
			*InstrData->HardwareAdapter << std::string("TRIGger1:FSWeep:SOURce ")
				+ RS_SMC100A::TriggerModeToCmdString(TriggerDesc.TriggerMode);
			if (TriggerDesc.TriggerMode == FunctionGeneratorDefs::TriggerDescType::TriggerModeType::ExternStep)
				*InstrData->HardwareAdapter << "SOURce1:SWEep:FREQuency:MODE STEP";
			else
				*InstrData->HardwareAdapter << "SOURce1:SWEep:FREQuency:MODE AUTO";
		}
		else if (InstrData->GetCurrentSweepType() == FunctionGeneratorDefs::SweepDescType::SweepType::Amplitude)
		{
			*InstrData->HardwareAdapter << std::string("TRIGger1:PSWeep:SOURce ")
				+ RS_SMC100A::TriggerModeToCmdString(TriggerDesc.TriggerMode);
			if (TriggerDesc.TriggerMode == FunctionGeneratorDefs::TriggerDescType::TriggerModeType::ExternStep)
				*InstrData->HardwareAdapter << "SOURce1:SWEep:POWer:MODE STEP";
			else
				*InstrData->HardwareAdapter << "SOURce1:SWEep:POWer:MODE AUTO";
		}

		*InstrData->HardwareAdapter << std::string("SOURce:INPut:TRIGger:SLOPe ") +
			(TriggerDesc.TriggerEdge == FunctionGeneratorDefs::TriggerDescType::TriggerEdgeType::Fall ? "NEGative" : "POSitive");

		return {};
	}

	void RS_SMC100AData::ResetImpl(dispatch_tag<FunctionGeneratorData>)
	{
		NumFailedStatusUpdateAttempts = 0;
		Running = false;
		CurrentSweepType = FunctionGeneratorDefs::SweepDescType::SweepType::Disabled;

		ResetImpl(dispatch_tag<RS_SMC100AData>());
	}

	const char* RS_SMC100A::TriggerModeToCmdString(FunctionGeneratorDefs::TriggerDescType::TriggerModeType TriggerMode) noexcept
	{
		switch (TriggerMode)
		{
		case FunctionGeneratorDefs::TriggerDescType::TriggerModeType::Continuous: return "AUTO";
		case FunctionGeneratorDefs::TriggerDescType::TriggerModeType::ExternSingle:
		case FunctionGeneratorDefs::TriggerDescType::TriggerModeType::ExternStep: return "EXTernal";
		case FunctionGeneratorDefs::TriggerDescType::TriggerModeType::Manual: return "SINGle";
		default: return "";
		}
	}

	RS_SMC100A::RS_SMC100A(const std::thread::id OwnerThreadID, DynExp::ParamsBasePtrType&& Params)
		: FunctionGenerator(OwnerThreadID, std::move(Params))
	{
	}

	void RS_SMC100A::SetSineFunction(const FunctionGeneratorDefs::SineFunctionDescType& FunctionDesc,
		bool PersistParams, bool Autostart, DynExp::TaskBase::CallbackType CallbackFunc) const
	{
		if (PersistParams)
		{
			auto InstrParams = DynExp::dynamic_Params_cast<FunctionGenerator>(GetNonConstParams());
			InstrParams->WaveformType = FunctionGeneratorDefs::WaveformTypes::Sine;
			InstrParams->FrequencyInHz = FunctionDesc.FrequencyInHz;
			InstrParams->Amplitude = FunctionDesc.Amplitude;
			InstrParams->Offset = 0;
			InstrParams->PhaseInRad = FunctionDesc.PhaseInRad;
			InstrParams->DutyCycle = 0;
			InstrParams->Autostart = Autostart;
		} // InstrParams unlocked here.

		MakeAndEnqueueTask<RS_SMC100ATasks::SetSineFunctionTask>(FunctionDesc, Autostart, CallbackFunc);
	}

	void RS_SMC100A::SetModulation(const FunctionGeneratorDefs::ModulationDescType& ModulationDesc,
		bool PersistParams, DynExp::TaskBase::CallbackType CallbackFunc) const
	{
		MakeAndEnqueueTask<RS_SMC100ATasks::SetModulationTask>(ModulationDesc, CallbackFunc);
	}

	void RS_SMC100A::SetSweep(const FunctionGeneratorDefs::SweepDescType& SweepDesc,
		bool PersistParams, DynExp::TaskBase::CallbackType CallbackFunc) const
	{
		MakeAndEnqueueTask<RS_SMC100ATasks::SetSweepTask>(SweepDesc, CallbackFunc);
	}

	void RS_SMC100A::ForceTrigger(DynExp::TaskBase::CallbackType CallbackFunc) const
	{
		MakeAndEnqueueTask<RS_SMC100ATasks::ForceTriggerTask>(CallbackFunc);
	}

	Util::OptionalBool RS_SMC100A::IsRunning() const
	{
		auto InstrData = DynExp::dynamic_InstrumentData_cast<RS_SMC100A>(GetInstrumentData());

		return InstrData->IsRunning();
	}

	void RS_SMC100A::ResetImpl(dispatch_tag<FunctionGenerator>)
	{
		ResetImpl(dispatch_tag<RS_SMC100A>());
	}

	void RS_SMC100A::SetTriggerChild(const FunctionGeneratorDefs::TriggerDescType& TriggerDesc,
		bool PersistParams, DynExp::TaskBase::CallbackType CallbackFunc) const
	{
		MakeAndEnqueueTask<RS_SMC100ATasks::SetTriggerTask>(TriggerDesc, CallbackFunc);
	}
}
