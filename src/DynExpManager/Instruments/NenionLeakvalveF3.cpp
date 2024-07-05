// This file is part of DynExp.

#include "stdafx.h"
#include "NenionLeakvalveF3.h"

namespace DynExpInstr
{
	void NenionLeakvalveF3Tasks::InitTask::InitFuncImpl(dispatch_tag<PositionerStageTasks::InitTask>, DynExp::InstrumentInstance& Instance)
	{
		{
			auto InstrParams = DynExp::dynamic_Params_cast<NenionLeakvalveF3>(Instance.ParamsGetter());
			auto InstrData = DynExp::dynamic_InstrumentData_cast<NenionLeakvalveF3>(Instance.InstrumentDataGetter());

			Instance.LockObject(InstrParams->HardwareAdapter, InstrData->HardwareAdapter);
			InstrData->HardwareAdapter->Clear();

			// Enable motor current.
			*InstrData->HardwareAdapter << "E";
		} // InstrParams and InstrData unlocked here.

		InitFuncImpl(dispatch_tag<InitTask>(), Instance);
	}

	void NenionLeakvalveF3Tasks::ExitTask::ExitFuncImpl(dispatch_tag<PositionerStageTasks::ExitTask>, DynExp::InstrumentInstance& Instance)
	{
		ExitFuncImpl(dispatch_tag<ExitTask>(), Instance);

		auto InstrData = DynExp::dynamic_InstrumentData_cast<NenionLeakvalveF3>(Instance.InstrumentDataGetter());

		try
		{
			// Abort motion.
			*InstrData->HardwareAdapter << "H";
		}
		catch (...)
		{
			// Swallow any exception which might arise from HardwareAdapter->StopMotion() since a failure
			// of this function is not considered a severe error.
		}

		Instance.UnlockObject(InstrData->HardwareAdapter);
	}

	void NenionLeakvalveF3Tasks::UpdateTask::UpdateFuncImpl(dispatch_tag<PositionerStageTasks::UpdateTask>, DynExp::InstrumentInstance& Instance)
	{
		try
		{
			auto InstrData = DynExp::dynamic_InstrumentData_cast<NenionLeakvalveF3>(Instance.InstrumentDataGetter());
			bool UpdateError = false;

			try
			{
				std::string StateString;

				// Empty read buffer. The valve behaves strangly if this is not done.
				do
				{
					StateString = InstrData->HardwareAdapter->WaitForLine(5);
				} while (!StateString.empty());

				*InstrData->HardwareAdapter << "S";

				// Sometimes, the valve echos a sent goto command back. Repeat reading data in this case.
				do
				{
					StateString = InstrData->HardwareAdapter->WaitForLine(5);
				} while (!StateString.empty() && StateString[0] == 'G');
				if (StateString.size() < 3 || StateString[1] != 'P')
					throw Util::InvalidDataException("Received an unexpected answer.");

				switch (StateString[0])
				{
				case 'R':
					InstrData->NenionLeakvalveF3Status = NenionLeakvalveF3Data::NenionLeakvalveF3StatusType::Moving;
					break;
				case 'I':
					InstrData->NenionLeakvalveF3Status = NenionLeakvalveF3Data::NenionLeakvalveF3StatusType::Arrived;
					break;
				case 'S':
					InstrData->NenionLeakvalveF3Status = NenionLeakvalveF3Data::NenionLeakvalveF3StatusType::Ready;
					break;
				case 'L':
					InstrData->NenionLeakvalveF3Status = NenionLeakvalveF3Data::NenionLeakvalveF3StatusType::LocalMode;
					break;
				case 'A':
					InstrData->NenionLeakvalveF3Status = NenionLeakvalveF3Data::NenionLeakvalveF3StatusType::AnalogMode;
					break;
				case 'D':
					InstrData->NenionLeakvalveF3Status = NenionLeakvalveF3Data::NenionLeakvalveF3StatusType::Disabled;
					break;
				default:
					throw Util::InvalidDataException("Received an invalid state descriptor.");
				}

				InstrData->SetCurrentPosition(Util::StrToT<PositionerStageData::PositionType>(StateString.substr(2)));
			}
			catch ([[maybe_unused]] const Util::InvalidDataException& e)
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
		// Issued here or by StrToT() if unexpected or no data has been received.
		catch (const Util::InvalidDataException& e)
		{
			Instance.GetOwner().SetWarning(e);

			return;
		}

		UpdateFuncImpl(dispatch_tag<UpdateTask>(), Instance);
	}

	DynExp::TaskResultType NenionLeakvalveF3Tasks::SetHomeTask::RunChild(DynExp::InstrumentInstance& Instance)
	{
		auto InstrData = DynExp::dynamic_InstrumentData_cast<NenionLeakvalveF3>(Instance.InstrumentDataGetter());

		InstrData->SetHomePosition(InstrData->GetCurrentPosition());

		return {};
	}

	DynExp::TaskResultType NenionLeakvalveF3Tasks::ReferenceTask::RunChild(DynExp::InstrumentInstance& Instance)
	{
		auto InstrData = DynExp::dynamic_InstrumentData_cast<NenionLeakvalveF3>(Instance.InstrumentDataGetter());

		*InstrData->HardwareAdapter << "N";

		return {};
	}

	DynExp::TaskResultType NenionLeakvalveF3Tasks::SetVelocityTask::RunChild(DynExp::InstrumentInstance& Instance)
	{
		auto InstrData = DynExp::dynamic_InstrumentData_cast<NenionLeakvalveF3>(Instance.InstrumentDataGetter());

		InstrData->SetVelocity(Velocity);

		return {};
	}

	DynExp::TaskResultType NenionLeakvalveF3Tasks::MoveToHomeTask::RunChild(DynExp::InstrumentInstance& Instance)
	{
		auto InstrData = DynExp::dynamic_InstrumentData_cast<NenionLeakvalveF3>(Instance.InstrumentDataGetter());

		*InstrData->HardwareAdapter << "G" + Util::ToStr(InstrData->GetHomePosition());

		return {};
	}

	DynExp::TaskResultType NenionLeakvalveF3Tasks::MoveAbsoluteTask::RunChild(DynExp::InstrumentInstance& Instance)
	{
		auto InstrData = DynExp::dynamic_InstrumentData_cast<NenionLeakvalveF3>(Instance.InstrumentDataGetter());

		const auto NewPos = static_cast<const NenionLeakvalveF3&>(Instance.GetOwner()).EnforcePositionLimits(Position);
		*InstrData->HardwareAdapter << "G" + Util::ToStr(NewPos);

		return {};
	}

	DynExp::TaskResultType NenionLeakvalveF3Tasks::MoveRelativeTask::RunChild(DynExp::InstrumentInstance& Instance)
	{
		auto InstrData = DynExp::dynamic_InstrumentData_cast<NenionLeakvalveF3>(Instance.InstrumentDataGetter());

		const auto NewPos = static_cast<const NenionLeakvalveF3&>(Instance.GetOwner()).EnforcePositionLimits(InstrData->GetCurrentPosition() + Position);
		*InstrData->HardwareAdapter << "G" + Util::ToStr(NewPos);

		return {};
	}

	DynExp::TaskResultType NenionLeakvalveF3Tasks::StopMotionTask::RunChild(DynExp::InstrumentInstance& Instance)
	{
		auto InstrData = DynExp::dynamic_InstrumentData_cast<NenionLeakvalveF3>(Instance.InstrumentDataGetter());

		// Abort motion.
		*InstrData->HardwareAdapter << "H";

		return DynExp::TaskResultType();
	}

	void NenionLeakvalveF3Data::ResetImpl(dispatch_tag<PositionerStageData>)
	{
		NenionLeakvalveF3Status = NenionLeakvalveF3StatusType::Ready;
		NumFailedStatusUpdateAttempts = 0;
		HomePosition = 0;

		ResetImpl(dispatch_tag<NenionLeakvalveF3Data>());
	}

	bool NenionLeakvalveF3Data::HasFailedChild() const noexcept
	{
		return !(NenionLeakvalveF3Status == NenionLeakvalveF3StatusType::Ready ||
			NenionLeakvalveF3Status == NenionLeakvalveF3StatusType::Moving ||
			NenionLeakvalveF3Status == NenionLeakvalveF3StatusType::Arrived);
	}

	NenionLeakvalveF3::NenionLeakvalveF3(const std::thread::id OwnerThreadID, DynExp::ParamsBasePtrType&& Params)
		: PositionerStage(OwnerThreadID, std::move(Params))
	{
	}

	PositionerStageData::PositionType NenionLeakvalveF3::EnforcePositionLimits(PositionerStageData::PositionType Position) const
	{
		return std::min(GetMaxPosition(), std::max(GetMinPosition(), Position));
	}

	void NenionLeakvalveF3::OnErrorChild() const
	{
		auto InstrData = DynExp::dynamic_InstrumentData_cast<NenionLeakvalveF3>(GetInstrumentData());

		// Abort motion.
		*InstrData->HardwareAdapter << "H";
	}

	void NenionLeakvalveF3::ResetImpl(dispatch_tag<PositionerStage>)
	{
		ResetImpl(dispatch_tag<NenionLeakvalveF3>());
	}
}