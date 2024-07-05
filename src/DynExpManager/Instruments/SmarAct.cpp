// This file is part of DynExp.

#include "stdafx.h"
#include "SmarAct.h"

namespace DynExpInstr
{
	void SmarActTasks::InitTask::InitFuncImpl(dispatch_tag<PositionerStageTasks::InitTask>, DynExp::InstrumentInstance& Instance)
	{
		{
			auto InstrParams = DynExp::dynamic_Params_cast<SmarAct>(Instance.ParamsGetter());
			auto InstrData = DynExp::dynamic_InstrumentData_cast<SmarAct>(Instance.InstrumentDataGetter());

			InstrData->Channel = InstrParams->Channel;
			Instance.LockObject(InstrParams->HardwareAdapter, InstrData->HardwareAdapter);
		} // InstrParams and InstrData unlocked here.

		InitFuncImpl(dispatch_tag<InitTask>(), Instance);
	}

	void SmarActTasks::ExitTask::ExitFuncImpl(dispatch_tag<PositionerStageTasks::ExitTask>, DynExp::InstrumentInstance& Instance)
	{
		ExitFuncImpl(dispatch_tag<ExitTask>(), Instance);

		auto InstrData = DynExp::dynamic_InstrumentData_cast<SmarAct>(Instance.InstrumentDataGetter());

		try
		{
			// Abort motion.
			InstrData->HardwareAdapter->StopMotion(InstrData->GetChannel());
		}
		catch (...)
		{
			// Swallow any exception which might arise from HardwareAdapter->StopMotion() since a failure
			// of this function is not considered a severe error.
		}

		Instance.UnlockObject(InstrData->HardwareAdapter);
	}

	void SmarActTasks::UpdateTask::UpdateFuncImpl(dispatch_tag<PositionerStageTasks::UpdateTask>, DynExp::InstrumentInstance& Instance)
	{
		try
		{
			auto InstrData = DynExp::dynamic_InstrumentData_cast<SmarAct>(Instance.InstrumentDataGetter());
			bool UpdateError = false;

			try
			{
				InstrData->SetCurrentPosition(InstrData->HardwareAdapter->GetCurrentPosition(InstrData->GetChannel()));
				InstrData->SetVelocity(InstrData->HardwareAdapter->GetVelocity(InstrData->GetChannel()));
				InstrData->SmarActChannelStatus.Set(InstrData->HardwareAdapter->GetChannelState(InstrData->GetChannel()));
			}
			catch ([[maybe_unused]] const DynExpHardware::SmarActException& e)
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

		UpdateFuncImpl(dispatch_tag<UpdateTask>(), Instance);
	}

	DynExp::TaskResultType SmarActTasks::SetHomeTask::RunChild(DynExp::InstrumentInstance& Instance)
	{
		auto InstrData = DynExp::dynamic_InstrumentData_cast<SmarAct>(Instance.InstrumentDataGetter());

		InstrData->SetHomePosition(InstrData->GetCurrentPosition());

		return {};
	}

	DynExp::TaskResultType SmarActTasks::ReferenceTask::RunChild(DynExp::InstrumentInstance& Instance)
	{
		auto InstrData = DynExp::dynamic_InstrumentData_cast<SmarAct>(Instance.InstrumentDataGetter());

		InstrData->HardwareAdapter->Reference(InstrData->GetChannel());
		InstrData->GetSmarActChannelStatus().Set(InstrData->HardwareAdapter->GetChannelState(InstrData->GetChannel()));

		return {};
	}

	DynExp::TaskResultType SmarActTasks::CalibrateTask::RunChild(DynExp::InstrumentInstance& Instance)
	{
		auto InstrData = DynExp::dynamic_InstrumentData_cast<SmarAct>(Instance.InstrumentDataGetter());

		InstrData->HardwareAdapter->Calibrate(InstrData->GetChannel());
		InstrData->GetSmarActChannelStatus().Set(InstrData->HardwareAdapter->GetChannelState(InstrData->GetChannel()));

		return {};
	}

	DynExp::TaskResultType SmarActTasks::SetVelocityTask::RunChild(DynExp::InstrumentInstance& Instance)
	{
		auto InstrData = DynExp::dynamic_InstrumentData_cast<SmarAct>(Instance.InstrumentDataGetter());

		InstrData->HardwareAdapter->SetVelocity(InstrData->GetChannel(), Velocity);

		return {};
	}

	DynExp::TaskResultType SmarActTasks::MoveToHomeTask::RunChild(DynExp::InstrumentInstance& Instance)
	{
		auto InstrData = DynExp::dynamic_InstrumentData_cast<SmarAct>(Instance.InstrumentDataGetter());

		InstrData->HardwareAdapter->MoveAbsolute(InstrData->GetChannel(), InstrData->GetHomePosition());
		InstrData->GetSmarActChannelStatus().Set(InstrData->HardwareAdapter->GetChannelState(InstrData->GetChannel()));

		return {};
	}

	DynExp::TaskResultType SmarActTasks::MoveAbsoluteTask::RunChild(DynExp::InstrumentInstance& Instance)
	{
		auto InstrData = DynExp::dynamic_InstrumentData_cast<SmarAct>(Instance.InstrumentDataGetter());

		InstrData->HardwareAdapter->MoveAbsolute(InstrData->GetChannel(), Position);
		InstrData->GetSmarActChannelStatus().Set(InstrData->HardwareAdapter->GetChannelState(InstrData->GetChannel()));

		return {};
	}

	DynExp::TaskResultType SmarActTasks::MoveRelativeTask::RunChild(DynExp::InstrumentInstance& Instance)
	{
		auto InstrData = DynExp::dynamic_InstrumentData_cast<SmarAct>(Instance.InstrumentDataGetter());

		InstrData->HardwareAdapter->MoveRelative(InstrData->GetChannel(), Position);
		InstrData->GetSmarActChannelStatus().Set(InstrData->HardwareAdapter->GetChannelState(InstrData->GetChannel()));

		return {};
	}

	DynExp::TaskResultType SmarActTasks::StopMotionTask::RunChild(DynExp::InstrumentInstance& Instance)
	{
		auto InstrData = DynExp::dynamic_InstrumentData_cast<SmarAct>(Instance.InstrumentDataGetter());

		InstrData->HardwareAdapter->StopMotion(InstrData->GetChannel());

		return DynExp::TaskResultType();
	}

	void SmarActData::ResetImpl(dispatch_tag<PositionerStageData>)
	{
		Channel = 0;
		SmarActChannelStatus.Set(0);
		NumFailedStatusUpdateAttempts = 0;
		HomePosition = 0;

		ResetImpl(dispatch_tag<SmarActData>());
	}

	SmarAct::SmarAct(const std::thread::id OwnerThreadID, DynExp::ParamsBasePtrType&& Params)
		: PositionerStage(OwnerThreadID, std::move(Params))
	{
	}

	void SmarAct::OnErrorChild() const
	{
		auto InstrData = DynExp::dynamic_InstrumentData_cast<SmarAct>(GetInstrumentData());

		// Abort motion.
		InstrData->HardwareAdapter->StopMotion(InstrData->GetChannel());
	}

	void SmarAct::ResetImpl(dispatch_tag<PositionerStage>)
	{
		ResetImpl(dispatch_tag<SmarAct>());
	}
}