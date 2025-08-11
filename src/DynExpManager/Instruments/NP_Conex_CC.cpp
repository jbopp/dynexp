// This file is part of DynExp.

#include "stdafx.h"
#include "NP_Conex_CC.h"

namespace DynExpInstr
{
	void NP_Conex_CC_Tasks::InitTask::InitFuncImpl(dispatch_tag<PositionerStageTasks::InitTask>, DynExp::InstrumentInstance& Instance)
	{
		{
			auto InstrParams = DynExp::dynamic_Params_cast<NP_Conex_CC>(Instance.ParamsGetter());
			auto InstrData = DynExp::dynamic_InstrumentData_cast<NP_Conex_CC>(Instance.InstrumentDataGetter());

			InstrData->Channel = InstrParams->Conex_CC_Address;
			Instance.LockObject(InstrParams->HardwareAdapter, InstrData->HardwareAdapter);
			InstrData->HardwareAdapter->Clear();

			// Define and go to home (this includes resetting the controller):
			// To set the current position as home position, the stage has to be in the CONFIGURATION state. This state can only be reached from the NOT REFERENCED state. 
			*InstrData->HardwareAdapter << Util::ToStr(InstrData->GetChannel()) + "RS"; // Go to NOT REFERENCED state.
			std::this_thread::sleep_for(std::chrono::milliseconds(500)); // This takes 500 ms.
			*InstrData->HardwareAdapter << Util::ToStr(InstrData->GetChannel()) + "PW1"; // Go to CONFIGURATION state.
			*InstrData->HardwareAdapter << Util::ToStr(InstrData->GetChannel()) + "HT1"; // Set current position to home position.
			*InstrData->HardwareAdapter << Util::ToStr(InstrData->GetChannel()) + "PW0"; // Go to NOT REFERENCED state.
			std::this_thread::sleep_for(std::chrono::seconds(3)); // This takes 3 s.
			*InstrData->HardwareAdapter << Util::ToStr(InstrData->GetChannel()) + "OR"; // Go to READY state.
		}

		InitFuncImpl(dispatch_tag<InitTask>(), Instance);
	}

	void NP_Conex_CC_Tasks::ExitTask::ExitFuncImpl(dispatch_tag<PositionerStageTasks::ExitTask>, DynExp::InstrumentInstance& Instance)
	{
		ExitFuncImpl(dispatch_tag<ExitTask>(), Instance);

		auto InstrData = DynExp::dynamic_InstrumentData_cast<NP_Conex_CC>(Instance.InstrumentDataGetter());

		try
		{
			// Abort motion.
			*InstrData->HardwareAdapter << Util::ToStr(InstrData->GetChannel()) + "ST";
		}
		catch (...)
		{
			// Swallow any exception which might arise from HardwareAdapter->operator<<() since a failure
			// of this function is not considered a severe error.
		}

		Instance.UnlockObject(InstrData->HardwareAdapter);
	}

	void NP_Conex_CC_Tasks::UpdateTask::UpdateFuncImpl(dispatch_tag<PositionerStageTasks::UpdateTask>, DynExp::InstrumentInstance& Instance)
	{
		try
		{
			auto InstrData = DynExp::dynamic_InstrumentData_cast<NP_Conex_CC>(Instance.InstrumentDataGetter());
			auto Owner = DynExp::dynamic_Object_cast<NP_Conex_CC>(&Instance.GetOwner());
			bool UpdateError = false;

			try
			{
				// Tell status:
				*InstrData->HardwareAdapter << Util::ToStr(InstrData->GetChannel()) + "TS";
				std::stringstream StatusStream(NP_Conex_CC::AnswerToNumberString(InstrData->HardwareAdapter->WaitForLine(1, std::chrono::milliseconds(25)), "TS"));
				StatusStream.exceptions(std::ofstream::failbit | std::ofstream::badbit);

				uint16_t ErrorMap; // Variable to hold the error map (16 bits)
				char ErrorMapBuffer[5]{ 0 };  // 4 characters + 1 for null terminator
				StatusStream.read(ErrorMapBuffer, 4);  // Read the first 4 characters
				std::istringstream ErrorMapStream(ErrorMapBuffer);

				ErrorMapStream >> std::hex >> ErrorMap;
				InstrData->ErrorCode = static_cast<NP_Conex_CC_StageData::ErrorCodeType>(ErrorMap); // Only 0 is no error

				uint16_t State; // Variable to hold the state (8 bits). However, uint16_t is used, since on many systems, uint8_t is treated like a char instead of an int.
				char StateBuffer[3]{ 0 };  // 2 characters + 1 for null terminator
				StatusStream.read(StateBuffer, 2);  // Read the next 2 characters
				std::istringstream StateStream(StateBuffer);

				StateStream >> std::hex >> State;
				InstrData->Conex_CCStatus.Set(Util::NumToT<uint8_t>(State)); // Type-conversion to uint8_t. The controller state is exactly two hex characters (8 bit), so it should never exceed 8 bits.
	
				auto Conex_CCStatus = InstrData->GetConex_CCStatus();
				// Check if stage is in READY state. If not, set it to READY:
				if (Conex_CCStatus.NotReferencedFromReset() || Conex_CCStatus.NotReferencedFromHoming() || Conex_CCStatus.NotReferencedFromConfiguration()
					|| Conex_CCStatus.NotReferencedFromDisable() || Conex_CCStatus.NotReferencedFromReady() || Conex_CCStatus.NotReferencedFromMoving()
					|| Conex_CCStatus.NotReferencedNoParams() || Conex_CCStatus.Configuration() || Conex_CCStatus.DisableFromReady()
					|| Conex_CCStatus.DisableFromMoving() || Conex_CCStatus.DisableFromTracking() || Conex_CCStatus.DisableFromReadyT()
					|| Conex_CCStatus.TrackingFromReadyT() || Conex_CCStatus.TrackingFromTracking())
				{
					*InstrData->HardwareAdapter << Util::ToStr(InstrData->GetChannel()) + "VA?";
					// 2. Set default velocity.
					InstrData->EnqueuePriorityTask(DynExp::MakeTask<NP_Conex_CC_Tasks::SetVelocityTask>(Owner->GetDefaultVelocity()));
					// 1. The stage is set to READY state, before the velocity is set to the default velocity:
					InstrData->EnqueuePriorityTask(DynExp::MakeTask<NP_Conex_CC_Tasks::SetReadyTask>());
				}

				// Tell position:
				*InstrData->HardwareAdapter << Util::ToStr(InstrData->GetChannel()) + "TP";
				auto PositionalAnswer = NP_Conex_CC::AnswerToNumberString(InstrData->HardwareAdapter->WaitForLine(1, std::chrono::milliseconds(25)), "TP");
				InstrData->SetCurrentPosition(Util::NumToT<PositionerStageData::PositionType>(Util::StrToT<double>(PositionalAnswer) * Owner->GetInputValuePositionTypeRatio()));

				// Tell velocity:
				*InstrData->HardwareAdapter << Util::ToStr(InstrData->GetChannel()) + "VA?";
				auto VelocityAnswer = NP_Conex_CC::AnswerToNumberString(InstrData->HardwareAdapter->WaitForLine(1, std::chrono::milliseconds(25)), "VA");
				InstrData->SetVelocity(Util::NumToT<PositionerStageData::PositionType>(Util::StrToT<double>(VelocityAnswer) * Owner->GetInputValuePositionTypeRatio()));
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
		// Issued by NP_Conex_CC::AnswerToNumberString() or StrToT() if unexpected or no data has been received.
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

	DynExp::TaskResultType NP_Conex_CC_Tasks::ResetTask::RunChild(DynExp::InstrumentInstance& Instance)
	{
		auto InstrData = DynExp::dynamic_InstrumentData_cast<NP_Conex_CC>(Instance.InstrumentDataGetter());

		*InstrData->HardwareAdapter << Util::ToStr(InstrData->GetChannel()) + "RS"; // This takes 500 ms.

		return {};
	}

	DynExp::TaskResultType NP_Conex_CC_Tasks::SetReadyTask::RunChild(DynExp::InstrumentInstance& Instance)
	{
		auto InstrData = DynExp::dynamic_InstrumentData_cast<NP_Conex_CC>(Instance.InstrumentDataGetter());

		*InstrData->HardwareAdapter << Util::ToStr(InstrData->GetChannel()) + "OR";

		return {};
	}

	DynExp::TaskResultType NP_Conex_CC_Tasks::SetHomeTask::RunChild(DynExp::InstrumentInstance& Instance)
	{
		auto InstrData = DynExp::dynamic_InstrumentData_cast<NP_Conex_CC>(Instance.InstrumentDataGetter());

		// 3. After homing, the stage is set to READY state again:
		InstrData->EnqueuePriorityTask(DynExp::MakeTask<NP_Conex_CC_Tasks::SetReadyTask>(nullptr,
			std::chrono::system_clock::now() + std::chrono::milliseconds(3000)));
		// 2. In the NOT REFERENCED state, the homing commands are applied:
		InstrData->EnqueuePriorityTask(DynExp::MakeTask<NP_Conex_CC_Tasks::SetHomeExecutionTask>(nullptr,
			std::chrono::system_clock::now() + std::chrono::milliseconds(500)));
		// 1. Go to NOT REFERENCED state:
		InstrData->EnqueuePriorityTask(DynExp::MakeTask<NP_Conex_CC_Tasks::ResetTask>());

		return {};
	}

	DynExp::TaskResultType NP_Conex_CC_Tasks::SetHomeExecutionTask::RunChild(DynExp::InstrumentInstance& Instance)
	{
		auto InstrData = DynExp::dynamic_InstrumentData_cast<NP_Conex_CC>(Instance.InstrumentDataGetter());
		auto Owner = DynExp::dynamic_Object_cast<NP_Conex_CC>(&Instance.GetOwner());

		*InstrData->HardwareAdapter << Util::ToStr(InstrData->GetChannel()) + "PW1";
		*InstrData->HardwareAdapter << Util::ToStr(InstrData->GetChannel()) + "VA" + Util::ToStr(InstrData->GetVelocity() / Owner->GetInputValuePositionTypeRatio());
		*InstrData->HardwareAdapter << Util::ToStr(InstrData->GetChannel()) + "HT1"; // use current position as HOME
		*InstrData->HardwareAdapter << Util::ToStr(InstrData->GetChannel()) + "PW0"; // this takes 3 s.

		return {};
	}

	DynExp::TaskResultType NP_Conex_CC_Tasks::ReferenceTask::RunChild(DynExp::InstrumentInstance& Instance)
	{
		auto InstrData = DynExp::dynamic_InstrumentData_cast<NP_Conex_CC>(Instance.InstrumentDataGetter());

		// 3. After homing, the stage is set to READY state again:
		InstrData->EnqueuePriorityTask(DynExp::MakeTask<NP_Conex_CC_Tasks::SetReadyTask>(nullptr,
			std::chrono::system_clock::now() + std::chrono::milliseconds(3000)));
		// 2. In the NOT REFERENCED state, the referencing commands are applied:
		InstrData->EnqueuePriorityTask(DynExp::MakeTask<NP_Conex_CC_Tasks::ReferenceExecutionTask>(nullptr,
			std::chrono::system_clock::now() + std::chrono::milliseconds(500)));
		// 1. Go to NOT REFERENCED state:
		InstrData->EnqueuePriorityTask(DynExp::MakeTask<NP_Conex_CC_Tasks::ResetTask>());

		return {};
	}

	DynExp::TaskResultType NP_Conex_CC_Tasks::ReferenceExecutionTask::RunChild(DynExp::InstrumentInstance& Instance)
	{
		auto InstrData = DynExp::dynamic_InstrumentData_cast<NP_Conex_CC>(Instance.InstrumentDataGetter());
		auto Owner = DynExp::dynamic_Object_cast<NP_Conex_CC>(&Instance.GetOwner());

		*InstrData->HardwareAdapter << Util::ToStr(InstrData->GetChannel()) + "PW1";
		*InstrData->HardwareAdapter << Util::ToStr(InstrData->GetChannel()) + "VA" + Util::ToStr(InstrData->GetVelocity() / Owner->GetInputValuePositionTypeRatio());
		*InstrData->HardwareAdapter << Util::ToStr(InstrData->GetChannel()) + "HT2"; // use mechanical zero as HOME
		*InstrData->HardwareAdapter << Util::ToStr(InstrData->GetChannel()) + "PW0"; // this takes 3 s.

		return {};
	}

	DynExp::TaskResultType NP_Conex_CC_Tasks::SetVelocityTask::RunChild(DynExp::InstrumentInstance& Instance)
	{
		auto InstrData = DynExp::dynamic_InstrumentData_cast<NP_Conex_CC>(Instance.InstrumentDataGetter());
		auto Owner = DynExp::dynamic_Object_cast<NP_Conex_CC>(&Instance.GetOwner());

		*InstrData->HardwareAdapter << Util::ToStr(InstrData->GetChannel()) + "VA" + Util::ToStr(Velocity / Owner->GetInputValuePositionTypeRatio());

		return {};
	}

	DynExp::TaskResultType NP_Conex_CC_Tasks::MoveToHomeTask::RunChild(DynExp::InstrumentInstance& Instance)
	{
		auto InstrData = DynExp::dynamic_InstrumentData_cast<NP_Conex_CC>(Instance.InstrumentDataGetter());

		// 2. Move to home position:
		InstrData->EnqueuePriorityTask(DynExp::MakeTask<NP_Conex_CC_Tasks::MoveToHomeExecutionTask>(nullptr,
			std::chrono::system_clock::now() + std::chrono::milliseconds(300)));
		// 1. Stop the motion:
		InstrData->EnqueuePriorityTask(DynExp::MakeTask<NP_Conex_CC_Tasks::StopMotionTask>());

		return {};
	}

	DynExp::TaskResultType NP_Conex_CC_Tasks::MoveToHomeExecutionTask::RunChild(DynExp::InstrumentInstance& Instance)
	{
		auto InstrData = DynExp::dynamic_InstrumentData_cast<NP_Conex_CC>(Instance.InstrumentDataGetter());

		*InstrData->HardwareAdapter << Util::ToStr(InstrData->GetChannel()) + "PA0";

		return {};
	}

	DynExp::TaskResultType NP_Conex_CC_Tasks::MoveAbsoluteTask::RunChild(DynExp::InstrumentInstance& Instance)
	{
		auto InstrData = DynExp::dynamic_InstrumentData_cast<NP_Conex_CC>(Instance.InstrumentDataGetter());

		// 2. Move to absolute position:
		InstrData->EnqueuePriorityTask(DynExp::MakeTask<NP_Conex_CC_Tasks::MoveAbsoluteExecutionTask>(Position, nullptr,
			std::chrono::system_clock::now() + std::chrono::milliseconds(300)));
		// 1. Stop the motion:
		InstrData->EnqueuePriorityTask(DynExp::MakeTask<NP_Conex_CC_Tasks::StopMotionTask>());

		return {};
	}

	DynExp::TaskResultType NP_Conex_CC_Tasks::MoveAbsoluteExecutionTask::RunChild(DynExp::InstrumentInstance& Instance)
	{
		auto InstrData = DynExp::dynamic_InstrumentData_cast<NP_Conex_CC>(Instance.InstrumentDataGetter());
		auto Owner = DynExp::dynamic_Object_cast<NP_Conex_CC>(&Instance.GetOwner());

		*InstrData->HardwareAdapter << Util::ToStr(InstrData->GetChannel()) + "PA" + Util::ToStr(Position / Owner->GetInputValuePositionTypeRatio());

		return {};
	}

	DynExp::TaskResultType NP_Conex_CC_Tasks::MoveRelativeTask::RunChild(DynExp::InstrumentInstance& Instance)
	{
		auto InstrData = DynExp::dynamic_InstrumentData_cast<NP_Conex_CC>(Instance.InstrumentDataGetter());

		// 2. Move to relative position:
		InstrData->EnqueuePriorityTask(DynExp::MakeTask<NP_Conex_CC_Tasks::MoveRelativeExecutionTask>(Position, nullptr,
			std::chrono::system_clock::now() + std::chrono::milliseconds(300)));
		// 1. Stop the motion:
		InstrData->EnqueuePriorityTask(DynExp::MakeTask<NP_Conex_CC_Tasks::StopMotionTask>());

		return {};
	}

	DynExp::TaskResultType NP_Conex_CC_Tasks::MoveRelativeExecutionTask::RunChild(DynExp::InstrumentInstance& Instance)
	{
		auto InstrData = DynExp::dynamic_InstrumentData_cast<NP_Conex_CC>(Instance.InstrumentDataGetter());
		auto Owner = DynExp::dynamic_Object_cast<NP_Conex_CC>(&Instance.GetOwner());

		*InstrData->HardwareAdapter << Util::ToStr(InstrData->GetChannel()) + "PR" + Util::ToStr(Position / Owner->GetInputValuePositionTypeRatio());

		return {};
	}

	DynExp::TaskResultType NP_Conex_CC_Tasks::StopMotionTask::RunChild(DynExp::InstrumentInstance& Instance)
	{
		auto InstrData = DynExp::dynamic_InstrumentData_cast<NP_Conex_CC>(Instance.InstrumentDataGetter());

		// It is not possible to use an if-condition that checks if the stage is currently moving,
		// since the update of the Conex_CCStatus is too slow.
		*InstrData->HardwareAdapter << Util::ToStr(InstrData->GetChannel()) + "ST";

		return DynExp::TaskResultType();
	}

	void NP_Conex_CC_StageData::ResetImpl(dispatch_tag<PositionerStageData>)
	{
		Conex_CCStatus.Set(0);
		ErrorCode = NoError;
		NumFailedStatusUpdateAttempts = 0;

		ResetImpl(dispatch_tag<NP_Conex_CC_StageData>());
	}

	bool NP_Conex_CC_StageData::IsMovingChild() const noexcept
	{
		return Conex_CCStatus.Moving() || Conex_CCStatus.Homing();
	}

	bool NP_Conex_CC_StageData::HasArrivedChild() const noexcept
	{
		return Conex_CCStatus.ReadyFromHoming() || Conex_CCStatus.ReadyFromMoving() || Conex_CCStatus.ReadyFromDisable()
			|| Conex_CCStatus.ReadyTFromReady() || Conex_CCStatus.ReadyTFromTracking() || Conex_CCStatus.ReadyTFromDisableT();
	}

	bool NP_Conex_CC_StageData::HasFailedChild() const noexcept
	{
		return ErrorCode != 0
			|| Conex_CCStatus.NotReferencedFromReset() || Conex_CCStatus.NotReferencedFromHoming() || Conex_CCStatus.NotReferencedFromConfiguration()
			|| Conex_CCStatus.NotReferencedFromDisable() || Conex_CCStatus.NotReferencedFromReady() || Conex_CCStatus.NotReferencedFromMoving()
			|| Conex_CCStatus.NotReferencedNoParams() || Conex_CCStatus.Configuration() || Conex_CCStatus.DisableFromReady()
			|| Conex_CCStatus.DisableFromMoving() || Conex_CCStatus.DisableFromTracking() || Conex_CCStatus.DisableFromReadyT()
			|| Conex_CCStatus.TrackingFromReadyT() || Conex_CCStatus.TrackingFromTracking();
	}

	std::string NP_Conex_CC::AnswerToNumberString(std::string&& Answer, const char* StartCode)
	{
		auto Pos = Answer.find(StartCode);

		if (Answer.empty())
			throw Util::InvalidDataException("Received an empty answer.");
		else if (Pos == std::string::npos)
			throw Util::InvalidDataException("Received an unexpected answer.");

		return Answer.substr(Pos + 2);
	}

	NP_Conex_CC::NP_Conex_CC(const std::thread::id OwnerThreadID, DynExp::ParamsBasePtrType&& Params)
		: PositionerStage(OwnerThreadID, std::move(Params))
	{
	}

	void NP_Conex_CC::OnErrorChild() const
	{
		auto InstrData = DynExp::dynamic_InstrumentData_cast<NP_Conex_CC>(GetInstrumentData());
		
		// Abort motion.
		*InstrData->HardwareAdapter << Util::ToStr(InstrData->GetChannel()) + "ST";
	}

	void NP_Conex_CC::ResetImpl(dispatch_tag<PositionerStage>)
	{
		ResetImpl(dispatch_tag<NP_Conex_CC>());
	}
}