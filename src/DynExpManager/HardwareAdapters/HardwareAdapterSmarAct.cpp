// This file is part of DynExp.

#include "stdafx.h"
#include "HardwareAdapterSmarAct.h"

namespace DynExpHardware
{
	auto SmarActHardwareAdapter::Enumerate()
	{
		// Since C++17, writing to std::string's internal buffer is allowed.
		std::string DeviceList;
		DeviceList.resize(256);
		size_t RequiredSize = DeviceList.size();
		auto Result = SmarActSyms::SA_CTL_FindDevices("", DeviceList.data(), &RequiredSize);

		// In case buffer of DeviceList is to small, RequiredSize now contains the required size. So, try again.
		if (DeviceList.size() < RequiredSize)
		{
			DeviceList.resize(RequiredSize + 1);
			RequiredSize = DeviceList.size();
			Result = SmarActSyms::SA_CTL_FindDevices("", DeviceList.data(), &RequiredSize);
		}

		// Adjust length of string to length of data written by SA_CTL_FindDevices()
		DeviceList.resize(RequiredSize);

		if (Result != SA_CTL_ERROR_NONE)
			throw SmarActException("Error enumerating SmarAct controllers.", Result);

		// All descriptors are separated by '\n'. Replace that by ' ' and use std::istringstream to split by ' '.
		std::replace(DeviceList.begin(), DeviceList.end(), '\n', ' ');
		std::istringstream ss(DeviceList);
		std::vector<std::string> DeviceDescriptors{ std::istream_iterator<std::string>(ss), std::istream_iterator<std::string>() };

		return DeviceDescriptors;
	}

	void SmarActHardwareAdapterParams::ConfigureParamsImpl(dispatch_tag<HardwareAdapterParamsBase>)
	{
		auto SmarActDevices = SmarActHardwareAdapter::Enumerate();
		if (!DeviceDescriptor.Get().empty() &&
			std::find(SmarActDevices.cbegin(), SmarActDevices.cend(), DeviceDescriptor) == std::cend(SmarActDevices))
			SmarActDevices.push_back(DeviceDescriptor);
		if (SmarActDevices.empty())
			throw Util::EmptyException("There is not any available SmarAct controller.");
		DeviceDescriptor.SetTextList(std::move(SmarActDevices));

		ConfigureParamsImpl(dispatch_tag<SmarActHardwareAdapterParams>());
	}

	SmarActHardwareAdapter::SmarActHardwareAdapter(const std::thread::id OwnerThreadID, DynExp::ParamsBasePtrType&& Params)
		: HardwareAdapterBase(OwnerThreadID, std::move(Params))
	{
		Init();
	}

	SmarActHardwareAdapter::~SmarActHardwareAdapter()
	{
		// Not locking, since the object is being destroyed. This should be inherently thread-safe.
		CloseUnsafe();
	}

	void SmarActHardwareAdapter::Init()
	{
		auto DerivedParams = dynamic_Params_cast<SmarActHardwareAdapter>(GetParams());

		DeviceDescriptor = DerivedParams->DeviceDescriptor.Get();

		SmarActHandleValid = false;
		SmarActHandle = SmarActSyms::SA_CTL_DeviceHandle_t();
	}

	void SmarActHardwareAdapter::ResetImpl(dispatch_tag<HardwareAdapterBase>)
	{
		// auto lock = AcquireLock(); not necessary here, since DynExp ensures that Object::Reset() can only
		// be called if respective object is not in use.

		CloseUnsafe();
		Init();

		ResetImpl(dispatch_tag<SmarActHardwareAdapter>());
	}

	void SmarActHardwareAdapter::EnsureReadyStateChild()
	{
		auto lock = AcquireLock(HardwareOperationTimeout);

		OpenUnsafe();
	}

	bool SmarActHardwareAdapter::IsReadyChild() const
	{
		auto lock = AcquireLock(HardwareOperationTimeout);

		auto Exception = GetExceptionUnsafe();
		Util::ForwardException(Exception);

		return IsOpened();
	}

	bool SmarActHardwareAdapter::IsConnectedChild() const noexcept
	{
		return IsOpened();
	}

	void SmarActHardwareAdapter::CheckError(const SmarActSyms::SA_CTL_Result_t Result, const std::source_location Location) const
	{
		if (Result == SA_CTL_ERROR_NONE)
			return;

		std::string ErrorString(SmarActSyms::SA_CTL_GetResultInfo(Result));

		// AcquireLock() has already been called by an (in)direct caller of this function.
		ThrowExceptionUnsafe(std::make_exception_ptr(SmarActException(ErrorString, Result, Location)));
	}

	void SmarActHardwareAdapter::OpenUnsafe()
	{
		if (IsOpened())
			return;

		auto Result = SmarActSyms::SA_CTL_Open(&SmarActHandle, DeviceDescriptor.c_str(), "");
		CheckError(Result);

		SmarActHandleValid = true;
	}

	void SmarActHardwareAdapter::CloseUnsafe()
	{
		if (IsOpened())
		{
			// Handle now considered invalid, even if SA_CTL_Close() fails.
			SmarActHandleValid = false;

			auto Result = SmarActSyms::SA_CTL_Close(SmarActHandle);
			CheckError(Result);
		}
	}

	SmarActHardwareAdapter::PositionType SmarActHardwareAdapter::GetCurrentPosition(const ChannelType Channel) const
	{
		auto lock = AcquireLock(HardwareOperationTimeout);

		PositionType Position;
		auto Result = SmarActSyms::SA_CTL_GetProperty_i64(SmarActHandle, Channel, SA_CTL_PKEY_POSITION, &Position, nullptr);
		CheckError(Result);

		return Position;
	}

	SmarActHardwareAdapter::PositionType SmarActHardwareAdapter::GetTargetPosition(const ChannelType Channel) const
	{
		auto lock = AcquireLock(HardwareOperationTimeout);

		PositionType TargetPosition;
		auto Result = SmarActSyms::SA_CTL_GetProperty_i64(SmarActHandle, Channel, SA_CTL_PKEY_TARGET_POSITION, &TargetPosition, nullptr);
		CheckError(Result);

		return TargetPosition;
	}

	SmarActHardwareAdapter::PositionType SmarActHardwareAdapter::GetVelocity(const ChannelType Channel) const
	{
		auto lock = AcquireLock(HardwareOperationTimeout);

		PositionType Velocity;
		auto Result = SmarActSyms::SA_CTL_GetProperty_i64(SmarActHandle, Channel, SA_CTL_PKEY_MOVE_VELOCITY, &Velocity, nullptr);
		CheckError(Result);

		return Velocity;
	}

	int32_t SmarActHardwareAdapter::GetChannelState(const ChannelType Channel) const
	{
		auto lock = AcquireLock(HardwareOperationTimeout);

		int32_t State;
		auto Result = SmarActSyms::SA_CTL_GetProperty_i32(SmarActHandle, Channel, SA_CTL_PKEY_CHANNEL_STATE, &State, 0);
		CheckError(Result);

		return State;
	}

	void SmarActHardwareAdapter::Calibrate(const ChannelType Channel) const
	{
		auto lock = AcquireLock(HardwareOperationTimeout);

		auto Result = SmarActSyms::SA_CTL_Calibrate(SmarActHandle, Channel, 0);
		CheckError(Result);
	}

	void SmarActHardwareAdapter::Reference(const ChannelType Channel) const
	{
		auto lock = AcquireLock(HardwareOperationTimeout);

		// Using direction configured as 'Safe Direction' on the SmarAct controller. Not allwing
		// to change this setting, because this requires recalibration.
		auto Result = SmarActSyms::SA_CTL_Reference(SmarActHandle, Channel, 0);
		CheckError(Result);
	}

	void SmarActHardwareAdapter::SetVelocity(const ChannelType Channel, PositionType Velocity) const
	{
		auto lock = AcquireLock(HardwareOperationTimeout);

		auto Result = SmarActSyms::SA_CTL_SetProperty_i64(SmarActHandle, Channel, SA_CTL_PKEY_MOVE_VELOCITY, Velocity);
		CheckError(Result);
	}

	void SmarActHardwareAdapter::MoveAbsolute(const ChannelType Channel, PositionType Position) const
	{
		auto lock = AcquireLock(HardwareOperationTimeout);

		auto Result = SmarActSyms::SA_CTL_SetProperty_i32(SmarActHandle, Channel, SA_CTL_PKEY_MOVE_MODE, SA_CTL_MOVE_MODE_CL_ABSOLUTE);
		CheckError(Result);

		Result = SmarActSyms::SA_CTL_Move(SmarActHandle, Channel, Position, 0);
		CheckError(Result);
	}

	void SmarActHardwareAdapter::MoveRelative(const ChannelType Channel, PositionType Position) const
	{
		auto lock = AcquireLock(HardwareOperationTimeout);

		auto Result = SmarActSyms::SA_CTL_SetProperty_i32(SmarActHandle, Channel, SA_CTL_PKEY_MOVE_MODE, SA_CTL_MOVE_MODE_CL_RELATIVE);
		CheckError(Result);

		Result = SmarActSyms::SA_CTL_Move(SmarActHandle, Channel, Position, 0);
		CheckError(Result);
	}

	void SmarActHardwareAdapter::StopMotion(const ChannelType Channel) const
	{
		auto lock = AcquireLock(HardwareOperationTimeout);

		if (!IsOpened())
			return;

		auto Result = SmarActSyms::SA_CTL_Stop(SmarActHandle, Channel, 0);
		CheckError(Result);
	}
}