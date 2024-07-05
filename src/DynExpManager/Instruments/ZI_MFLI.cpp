// This file is part of DynExp.

#include "stdafx.h"
#include "ZI_MFLI.h"

namespace DynExpInstr
{
	void ZI_MFLITasks::InitTask::InitFuncImpl(dispatch_tag<LockinAmplifierTasks::InitTask>, DynExp::InstrumentInstance& Instance)
	{
		auto InstrParams = DynExp::dynamic_Params_cast<ZI_MFLI>(Instance.ParamsGetter());
		auto InstrData = DynExp::dynamic_InstrumentData_cast<ZI_MFLI>(Instance.InstrumentDataGetter());

		Instance.LockObject(InstrParams->HardwareAdapter, InstrData->HardwareAdapter);

		InstrData->HardwareAdapter->ConfigureInput(GetUsedSignalInput(), GetUsedDemodulator());

		InitFuncImpl(dispatch_tag<InitTask>(), Instance);
	}

	void ZI_MFLITasks::ExitTask::ExitFuncImpl(dispatch_tag<LockinAmplifierTasks::ExitTask>, DynExp::InstrumentInstance& Instance)
	{
		ExitFuncImpl(dispatch_tag<ExitTask>(), Instance);

		auto InstrData = DynExp::dynamic_InstrumentData_cast<ZI_MFLI>(Instance.InstrumentDataGetter());

		try
		{
			InstrData->HardwareAdapter->StopAcquisition();
			InstrData->HardwareAdapter->SetEnabled(GetUsedDemodulator(), false);
		}
		catch (...)
		{
			// Swallow any exception which might arise from instrument shutdown since a failure
			// of this function is not considered a severe error.
		}

		Instance.UnlockObject(InstrData->HardwareAdapter);
	}

	void ZI_MFLITasks::UpdateTask::UpdateFuncImpl(dispatch_tag<LockinAmplifierTasks::UpdateTask>, DynExp::InstrumentInstance& Instance)
	{
		try
		{
			auto InstrData = DynExp::dynamic_InstrumentData_cast<ZI_MFLI>(Instance.InstrumentDataGetter());
			bool UpdateError = false;

			try
			{
				InstrData->SetSensitivity(InstrData->HardwareAdapter->GetInputRange(GetUsedSignalInput()));
				InstrData->SetPhase(InstrData->HardwareAdapter->GetDemodPhase(GetUsedDemodulator()));
				InstrData->SetTimeConstant(InstrData->HardwareAdapter->GetDemodTimeConstant(GetUsedDemodulator()));
				InstrData->SetFilterOrder(InstrData->HardwareAdapter->GetDemodFilterOrder(GetUsedDemodulator()));
				InstrData->SetTriggerMode(InstrData->HardwareAdapter->GetTriggerMode());
				InstrData->SetTriggerEdge(InstrData->HardwareAdapter->GetTriggerEdge());
				InstrData->SetSamplingRate(InstrData->HardwareAdapter->GetDemodSamplingRate(GetUsedDemodulator()));
				InstrData->SetEnable(InstrData->HardwareAdapter->GetEnabled(GetUsedDemodulator()));

				InstrData->NegInputLoad = InstrData->HardwareAdapter->GetNegInputLoad(GetUsedSignalInput());
				InstrData->PosInputLoad = InstrData->HardwareAdapter->GetPosInputLoad(GetUsedSignalInput());
				InstrData->Overload = DynExpHardware::ZILabOneHardwareAdapter::DetermineOverload(InstrData->PosInputLoad, InstrData->NegInputLoad);
				InstrData->OscillatorFrequency = InstrData->HardwareAdapter->GetOscillatorFrequency(0);
				InstrData->AcquisitionProgress = InstrData->HardwareAdapter->GetAcquisitionProgress();
			}
			catch ([[maybe_unused]] const DynExpHardware::ZILabOneException& e)
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
		// Issued if reading data from ZI_MFLI controller failed.
		catch (const DynExpHardware::ZILabOneException& e)
		{
			Instance.GetOwner().SetWarning(e);

			return;
		}

		UpdateFuncImpl(dispatch_tag<UpdateTask>(), Instance);
	}

	DynExp::TaskResultType ZI_MFLITasks::ReadTask::RunChild(DynExp::InstrumentInstance& Instance)
	{
		auto InstrData = DynExp::dynamic_InstrumentData_cast<ZI_MFLI>(Instance.InstrumentDataGetter());

		auto SampleStream = InstrData->GetCastSampleStream<LockinAmplifierData::SampleStreamType>();
		auto LockinSamples = InstrData->HardwareAdapter->GetAcquiredData();

		for (const auto& Sample : LockinSamples)
			SampleStream->WriteSample({ Sample.GetDisambiguatedValue(InstrData->GetSignalType()), Sample.Time });

		return {};
	}

	DynExp::TaskResultType ZI_MFLITasks::ClearDataTask::RunChild(DynExp::InstrumentInstance& Instance)
	{
		auto InstrData = DynExp::dynamic_InstrumentData_cast<ZI_MFLI>(Instance.InstrumentDataGetter());

		InstrData->HardwareAdapter->ClearAcquiredData();

		return {};
	}

	DynExp::TaskResultType ZI_MFLITasks::StartTask::RunChild(DynExp::InstrumentInstance& Instance)
	{
		auto InstrData = DynExp::dynamic_InstrumentData_cast<ZI_MFLI>(Instance.InstrumentDataGetter());

		InstrData->HardwareAdapter->StartAcquisition(GetUsedDemodulator(), InstrData->GetSampleStream()->GetStreamSizeWrite());

		return {};
	}

	DynExp::TaskResultType ZI_MFLITasks::StopTask::RunChild(DynExp::InstrumentInstance& Instance)
	{
		auto InstrData = DynExp::dynamic_InstrumentData_cast<ZI_MFLI>(Instance.InstrumentDataGetter());

		InstrData->HardwareAdapter->StopAcquisition();

		return {};
	}

	DynExp::TaskResultType ZI_MFLITasks::SetSensitivityTask::RunChild(DynExp::InstrumentInstance& Instance)
	{
		auto InstrData = DynExp::dynamic_InstrumentData_cast<ZI_MFLI>(Instance.InstrumentDataGetter());

		InstrData->HardwareAdapter->SetInputRange(GetUsedSignalInput(), Sensitivity);

		return {};
	}

	DynExp::TaskResultType ZI_MFLITasks::AutoAdjustSensitivityTask::RunChild(DynExp::InstrumentInstance& Instance)
	{
		auto InstrData = DynExp::dynamic_InstrumentData_cast<ZI_MFLI>(Instance.InstrumentDataGetter());

		InstrData->HardwareAdapter->AutoAdjustInputRange(GetUsedSignalInput());

		return {};
	}

	DynExp::TaskResultType ZI_MFLITasks::SetPhaseTask::RunChild(DynExp::InstrumentInstance& Instance)
	{
		auto InstrData = DynExp::dynamic_InstrumentData_cast<ZI_MFLI>(Instance.InstrumentDataGetter());

		InstrData->HardwareAdapter->SetDemodPhase(GetUsedDemodulator(), Phase);

		return {};
	}

	DynExp::TaskResultType ZI_MFLITasks::AutoAdjustPhaseTask::RunChild(DynExp::InstrumentInstance& Instance)
	{
		auto InstrData = DynExp::dynamic_InstrumentData_cast<ZI_MFLI>(Instance.InstrumentDataGetter());

		InstrData->HardwareAdapter->AutoAdjustDemodPhase(GetUsedDemodulator());

		return {};
	}

	DynExp::TaskResultType ZI_MFLITasks::SetTimeConstantTask::RunChild(DynExp::InstrumentInstance& Instance)
	{
		auto InstrData = DynExp::dynamic_InstrumentData_cast<ZI_MFLI>(Instance.InstrumentDataGetter());

		InstrData->HardwareAdapter->SetDemodTimeConstant(GetUsedDemodulator(), TimeConstant);

		return {};
	}

	DynExp::TaskResultType ZI_MFLITasks::SetFilterOrderTask::RunChild(DynExp::InstrumentInstance& Instance)
	{
		auto InstrData = DynExp::dynamic_InstrumentData_cast<ZI_MFLI>(Instance.InstrumentDataGetter());

		InstrData->HardwareAdapter->SetDemodFilterOrder(GetUsedDemodulator(), FilterOrder);

		return {};
	}

	DynExp::TaskResultType ZI_MFLITasks::SetTriggerModeTask::RunChild(DynExp::InstrumentInstance& Instance)
	{
		auto InstrData = DynExp::dynamic_InstrumentData_cast<ZI_MFLI>(Instance.InstrumentDataGetter());

		InstrData->HardwareAdapter->SetTriggerMode(TriggerMode, GetUsedDemodulator(), TriggerChannel);

		return {};
	}

	DynExp::TaskResultType ZI_MFLITasks::SetTriggerEdgeTask::RunChild(DynExp::InstrumentInstance& Instance)
	{
		auto InstrData = DynExp::dynamic_InstrumentData_cast<ZI_MFLI>(Instance.InstrumentDataGetter());

		InstrData->HardwareAdapter->SetTriggerEdge(TriggerEdge);

		return {};
	}

	DynExp::TaskResultType ZI_MFLITasks::SetSignalTypeTask::RunChild(DynExp::InstrumentInstance& Instance)
	{
		auto InstrData = DynExp::dynamic_InstrumentData_cast<ZI_MFLI>(Instance.InstrumentDataGetter());

		InstrData->SetSignalType(SignalType);

		return {};
	}

	DynExp::TaskResultType ZI_MFLITasks::SetSamplingRateTask::RunChild(DynExp::InstrumentInstance& Instance)
	{
		auto InstrData = DynExp::dynamic_InstrumentData_cast<ZI_MFLI>(Instance.InstrumentDataGetter());

		InstrData->HardwareAdapter->SetDemodSamplingRate(GetUsedDemodulator(), SamplingRate);

		return {};
	}

	DynExp::TaskResultType ZI_MFLITasks::SetEnableTask::RunChild(DynExp::InstrumentInstance& Instance)
	{
		auto InstrData = DynExp::dynamic_InstrumentData_cast<ZI_MFLI>(Instance.InstrumentDataGetter());

		InstrData->HardwareAdapter->SetEnabled(GetUsedDemodulator(), Enable);

		return {};
	}

	DynExp::TaskResultType ZI_MFLITasks::ForceTriggerTask::RunChild(DynExp::InstrumentInstance& Instance)
	{
		auto InstrData = DynExp::dynamic_InstrumentData_cast<ZI_MFLI>(Instance.InstrumentDataGetter());

		InstrData->HardwareAdapter->ForceTrigger();

		return {};
	}

	void ZI_MFLIData::ResetImpl(dispatch_tag<LockinAmplifierData>)
	{
		Sensitivity = 1;
		Phase = 0;
		TimeConstant = 1e-3;
		FilterOrder = 1;
		TriggerMode = LockinAmplifierDefs::TriggerModeType::ExternSingle;
		TriggerEdge = LockinAmplifierDefs::TriggerEdgeType::Fall;
		Signal = LockinAmplifierDefs::SignalType::R;
		SamplingRate = 1000;
		Enable = false;

		Overload = false;
		NegInputLoad = 0;
		PosInputLoad = 0;
		OscillatorFrequency = 0;
		AcquisitionProgress = -1;

		NumFailedStatusUpdateAttempts = 0;

		ResetImpl(dispatch_tag<ZI_MFLIData>());
	}

	Util::TextValueListType<DynExpHardware::ZILabOneHardwareAdapter::SignalInputType> ZI_MFLIParams::SignalInputTypeStrList()
	{
		Util::TextValueListType<DynExpHardware::ZILabOneHardwareAdapter::SignalInputType> List = {
			{ "Voltage", DynExpHardware::ZILabOneHardwareAdapter::SignalInputType::Voltage },
			{ "Differential voltage", DynExpHardware::ZILabOneHardwareAdapter::SignalInputType::DifferentialVoltage },
			{ "Current", DynExpHardware::ZILabOneHardwareAdapter::SignalInputType::Current },
		};

		return List;
	}

	ZI_MFLI::ZI_MFLI(const std::thread::id OwnerThreadID, DynExp::ParamsBasePtrType&& Params)
		: LockinAmplifier(OwnerThreadID, std::move(Params)),
		SignalInput(DynExpHardware::ZILabOneHardwareAdapter::SignalInputType::Voltage), UsedDemodulator(0), TriggerChannel(1)
	{
	}

	Util::OptionalBool ZI_MFLI::HasFinished() const
	{
		auto InstrData = DynExp::dynamic_InstrumentData_cast<ZI_MFLI>(GetInstrumentData());

		return InstrData->HardwareAdapter->HasFinishedAcquisition();
	}

	Util::OptionalBool ZI_MFLI::IsRunning() const
	{
		return !HasFinished();
	}

	void ZI_MFLI::ResetImpl(dispatch_tag<LockinAmplifier>)
	{
		{
			auto InstrParams = DynExp::dynamic_Params_cast<ZI_MFLI>(GetParams());

			SignalInput = InstrParams->SignalInput;
			UsedDemodulator = InstrParams->Demodulator;
			TriggerChannel = InstrParams->TriggerChannel;
		} // InstrParams unlocked here.

		ResetImpl(dispatch_tag<ZI_MFLI>());
	}

	void ZI_MFLI::ApplyFromParamsImpl(dispatch_tag<LockinAmplifier>) const
	{
		auto InstrParams = DynExp::dynamic_Params_cast<ZI_MFLI>(GetParams());

		if (InstrParams->AutoApplyParams == LockinAmplifierParams::AutoApplyParamsType::AutoApply)
		{
			SetSensitivity(InstrParams->Sensitivity);
			SetPhase(InstrParams->Phase);
			SetTimeConstant(InstrParams->TimeConstant);
			SetFilterOrder(Util::NumToT<uint8_t>(InstrParams->FilterOrder));
			SetTriggerMode(InstrParams->TriggerMode);
			SetTriggerEdge(InstrParams->TriggerEdge);
			SetSignalType(InstrParams->Signal);
			SetSamplingRate(InstrParams->SamplingRate);
			SetEnable(InstrParams->Enable.Get());
		}
	}
}