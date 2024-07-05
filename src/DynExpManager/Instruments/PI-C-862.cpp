// This file is part of DynExp.

#include "stdafx.h"
#include "PI-C-862.h"

namespace DynExpInstr
{
	void PI_C_862_Tasks::InitTask::InitFuncImpl(dispatch_tag<PositionerStageTasks::InitTask>, DynExp::InstrumentInstance& Instance)
	{
		{
			auto InstrParams = DynExp::dynamic_Params_cast<PI_C_862>(Instance.ParamsGetter());
			auto InstrData = DynExp::dynamic_InstrumentData_cast<PI_C_862>(Instance.InstrumentDataGetter());

			Instance.LockObject(InstrParams->HardwareAdapter, InstrData->HardwareAdapter);
			InstrData->HardwareAdapter->Clear();

			// Reset and select controller.
			*InstrData->HardwareAdapter << "RT";
			*InstrData->HardwareAdapter << std::string(1, static_cast<char>(1)) + InstrParams->MercuryAddress.Get();

			// Ignore version and factory string and await startup.
			InstrData->HardwareAdapter->WaitForLine(3, std::chrono::milliseconds(50));
		} // InstrParams and InstrData unlocked here.

		InitFuncImpl(dispatch_tag<InitTask>(), Instance);
	}

	void PI_C_862_Tasks::ExitTask::ExitFuncImpl(dispatch_tag<PositionerStageTasks::ExitTask>, DynExp::InstrumentInstance& Instance)
	{
		ExitFuncImpl(dispatch_tag<ExitTask>(), Instance);

		auto InstrData = DynExp::dynamic_InstrumentData_cast<PI_C_862>(Instance.InstrumentDataGetter());

		try
		{
			// Abort motion.
			*InstrData->HardwareAdapter << "AB";
		}
		catch (...)
		{
			// Swallow any exception which might arise from HardwareAdapter->operator<<() since a failure
			// of this function is not considered a severe error.
		}

		Instance.UnlockObject(InstrData->HardwareAdapter);
	}

	void PI_C_862_Tasks::UpdateTask::UpdateFuncImpl(dispatch_tag<PositionerStageTasks::UpdateTask>, DynExp::InstrumentInstance& Instance)
	{
		try
		{
			auto InstrData = DynExp::dynamic_InstrumentData_cast<PI_C_862>(Instance.InstrumentDataGetter());
			bool UpdateError = false;

			try
			{
				// Tell position
				*InstrData->HardwareAdapter << "TP";
				InstrData->SetCurrentPosition(Util::StrToT<PositionerStageData::PositionType>(
					PI_C_862::AnswerToNumberString(InstrData->HardwareAdapter->WaitForLine(3), "P:")));

				// Tell programmed velocity
				*InstrData->HardwareAdapter << "TY";
				InstrData->SetVelocity(Util::StrToT<PositionerStageData::PositionType>(
					PI_C_862::AnswerToNumberString(InstrData->HardwareAdapter->WaitForLine(3), "Y:")));

				// Tell status
				*InstrData->HardwareAdapter << "TS";
				std::stringstream StatusStream(PI_C_862::AnswerToNumberString(InstrData->HardwareAdapter->WaitForLine(3), "S:"));
				StatusStream.exceptions(std::ofstream::failbit | std::ofstream::badbit);
				StatusStream << std::hex;

				uint16_t Byte;			// Since stringstream handles uint8_t as character...
				StatusStream >> Byte;	// Extract LM629 status
				if (Byte > 0xFF)
					throw Util::InvalidDataException("Received an unexpected LM629 status.");
				InstrData->LM629Status.Set(static_cast<uint8_t>(Byte));

				StatusStream.seekg(15);
				StatusStream >> Byte;	// Extract error codes
				if (Byte > 0xFF)
					throw Util::InvalidDataException("Received an unexpected error code.");
				InstrData->ErrorCode = static_cast<PI_C_862StageData::ErrorCodeType>(Byte);
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
		// Issued by PI_C_862::AnswerToNumberString() or StrToT() if unexpected or no data has been received.
		catch (const Util::InvalidDataException& e)
		{
			Instance.GetOwner().SetWarning(e);

			return;
		}
		// Issued by std::stringstream if extracting data fails.
		catch (const std::stringstream::failure& e)
		{
			Instance.GetOwner().SetWarning(e.what(), Util::DynExpErrorCodes::InvalidData);

			return;
		}

		UpdateFuncImpl(dispatch_tag<UpdateTask>(), Instance);
	}

	DynExp::TaskResultType PI_C_862_Tasks::SetHomeTask::RunChild(DynExp::InstrumentInstance& Instance)
	{
		auto InstrData = DynExp::dynamic_InstrumentData_cast<PI_C_862>(Instance.InstrumentDataGetter());

		// Define home
		*InstrData->HardwareAdapter << "DH";

		return {};
	}

	DynExp::TaskResultType PI_C_862_Tasks::ReferenceTask::RunChild(DynExp::InstrumentInstance& Instance)
	{
		auto InstrData = DynExp::dynamic_InstrumentData_cast<PI_C_862>(Instance.InstrumentDataGetter());

		// Find edge
		if (Direction == PositionerStage::DirectionType::Forward)
			*InstrData->HardwareAdapter << "FE0";
		else
			*InstrData->HardwareAdapter << "FE1";

		return {};
	}

	DynExp::TaskResultType PI_C_862_Tasks::SetVelocityTask::RunChild(DynExp::InstrumentInstance& Instance)
	{
		auto InstrData = DynExp::dynamic_InstrumentData_cast<PI_C_862>(Instance.InstrumentDataGetter());

		// Set velocity
		*InstrData->HardwareAdapter << "SV" + Util::ToStr(Velocity);

		return {};
	}

	DynExp::TaskResultType PI_C_862_Tasks::MoveToHomeTask::RunChild(DynExp::InstrumentInstance& Instance)
	{
		auto InstrData = DynExp::dynamic_InstrumentData_cast<PI_C_862>(Instance.InstrumentDataGetter());

		// Go home
		*InstrData->HardwareAdapter << "GH";

		return {};
	}

	DynExp::TaskResultType PI_C_862_Tasks::MoveAbsoluteTask::RunChild(DynExp::InstrumentInstance& Instance)
	{
		auto InstrData = DynExp::dynamic_InstrumentData_cast<PI_C_862>(Instance.InstrumentDataGetter());

		// Move absolute
		*InstrData->HardwareAdapter << "MA" + Util::ToStr(Position);

		return {};
	}

	DynExp::TaskResultType PI_C_862_Tasks::MoveRelativeTask::RunChild(DynExp::InstrumentInstance& Instance)
	{
		auto InstrData = DynExp::dynamic_InstrumentData_cast<PI_C_862>(Instance.InstrumentDataGetter());

		// Move relative
		*InstrData->HardwareAdapter << "MR" + Util::ToStr(Position);

		return {};
	}

	DynExp::TaskResultType PI_C_862_Tasks::StopMotionTask::RunChild(DynExp::InstrumentInstance& Instance)
	{
		auto InstrData = DynExp::dynamic_InstrumentData_cast<PI_C_862>(Instance.InstrumentDataGetter());

		// Abort motion
		*InstrData->HardwareAdapter << "AB";

		return DynExp::TaskResultType();
	}

	void PI_C_862StageData::ResetImpl(dispatch_tag<PositionerStageData>)
	{
		LM629Status.Set(0);
		ErrorCode = NoError;
		NumFailedStatusUpdateAttempts = 0;

		ResetImpl(dispatch_tag<PI_C_862StageData>());
	}

	bool PI_C_862StageData::IsMovingChild() const noexcept
	{
		return !LM629Status.TrajectoryComplete();
	}

	bool PI_C_862StageData::HasArrivedChild() const noexcept
	{
		return LM629Status.TrajectoryComplete();
	}

	bool PI_C_862StageData::HasFailedChild() const noexcept
	{
		return ErrorCode != 0
			|| LM629Status.CommandError() || LM629Status.PositionLimitExceeded()
			|| LM629Status.ExcessivePositionError() || LM629Status.BreakpointReached();
	}

	// StartCode is something like "X:". In any case, it has a length of two characters
	std::string PI_C_862::AnswerToNumberString(std::string&& Answer, const char* StartCode)
	{
		auto Pos = Answer.find(StartCode);

		if (Pos == std::string::npos)
			throw Util::InvalidDataException("Received an unexpected answer.");

		return Answer.substr(Pos + 2);
	}

	PI_C_862::PI_C_862(const std::thread::id OwnerThreadID, DynExp::ParamsBasePtrType&& Params)
		: PositionerStage(OwnerThreadID, std::move(Params))
	{
	}

	void PI_C_862::OnErrorChild() const
	{
		auto InstrData = DynExp::dynamic_InstrumentData_cast<PI_C_862>(GetInstrumentData());

		// Abort motion.
		*InstrData->HardwareAdapter << "AB";
	}

	void PI_C_862::ResetImpl(dispatch_tag<PositionerStage>)
	{
		ResetImpl(dispatch_tag<PI_C_862>());
	}
}