// This file is part of DynExp.

#include "stdafx.h"
#include "HardwareAdapterPVCam.h"

namespace DynExpHardware
{
	auto PVCamHardwareAdapter::Enumerate()
	{
		PVCamInitializer::Init();
		auto PVCamLock = PVCamInitializer::Lock();

		PVCamSyms::int16 NumberOfCameras = 0;
		if (PVCamSyms::pl_cam_get_total(&NumberOfCameras) != PVCamSyms::PV_OK)
			throw PVCamException("Error enumerating PVCAM cameras.", PVCamSyms::pl_error_code());

		std::vector<std::string> DeviceDescriptors;
		for (decltype(NumberOfCameras) i = 0; i < NumberOfCameras; ++i)
		{
			std::string CameraName;
			CameraName.resize(CAM_NAME_LEN);

			if (PVCamSyms::pl_cam_get_name(i, CameraName.data()) != PVCamSyms::PV_OK)
				throw PVCamException("Error obtaining a PVCAM camera's name.", PVCamSyms::pl_error_code());
			CameraName = Util::TrimTrailingZeros(CameraName);

			DeviceDescriptors.emplace_back(std::move(CameraName));
		}

		return DeviceDescriptors;
	}

	void PVCamHardwareAdapterParams::ConfigureParamsImpl(dispatch_tag<HardwareAdapterParamsBase>)
	{
		auto PVCamDevices = PVCamHardwareAdapter::Enumerate();
		if (!CameraName.Get().empty() &&
			std::find(PVCamDevices.cbegin(), PVCamDevices.cend(), CameraName) == std::cend(PVCamDevices))
			PVCamDevices.push_back(CameraName);
		if (PVCamDevices.empty())
			throw Util::EmptyException("There is not any available PVCam camera.");
		CameraName.SetTextList(std::move(PVCamDevices));

		ConfigureParamsImpl(dispatch_tag<PVCamHardwareAdapterParams>());
	}

	PVCamHardwareAdapter::PVCamInitializer::~PVCamInitializer()
	{
		// Swallow errors from pl_pvcam_uninit() since there is no way how to handle them and throwing from destructor is even worse.
		if (IsInitialized)
			PVCamSyms::pl_pvcam_uninit();
	}

	PVCamHardwareAdapter::PVCamInitializer::LockType PVCamHardwareAdapter::PVCamInitializer::Lock(const std::chrono::milliseconds Timeout)
	{
		return GetInstance().AcquireLock(Timeout);
	}

	PVCamHardwareAdapter::PVCamInitializer& PVCamHardwareAdapter::PVCamInitializer::GetInstance(bool MayInit)
	{
		static PVCamInitializer Instance;

		if (MayInit)
		{
			auto lock = Instance.AcquireLock(std::chrono::milliseconds(100));

			// Instance.IsInitialized might have been changed in between.
			if (!Instance.IsInitialized)
			{
				if (PVCamSyms::pl_pvcam_init() != PVCamSyms::PV_OK)
					throw PVCamException("Error initializing PVCAM library.", PVCamSyms::pl_error_code());

				Instance.IsInitialized = true;
			}
		}

		return Instance;
	}

	PVCamHardwareAdapter::PVCamHardwareAdapter(const std::thread::id OwnerThreadID, DynExp::ParamsBasePtrType&& Params)
		: HardwareAdapterBase(OwnerThreadID, std::move(Params)), CameraState(CameraStateType::Stopped)
	{
		Init();
	}

	PVCamHardwareAdapter::~PVCamHardwareAdapter()
	{
		// Not locking, since the object is being destroyed. This should be inherently thread-safe.
		CloseUnsafe();
	}

	decltype(PVCamSyms::rgn_type::s2) PVCamHardwareAdapter::GetImageWidth() const
	{
		auto lock = AcquireLock(HardwareOperationTimeout);

		return Region.s2;
	}

	decltype(PVCamSyms::rgn_type::p2) PVCamHardwareAdapter::GetImageHeight() const
	{
		auto lock = AcquireLock(HardwareOperationTimeout);

		return Region.p2;
	}

	DynExpInstr::CameraData::CameraModesType PVCamHardwareAdapter::GetCameraModes() const
	{
		DynExpInstr::CameraData::CameraModesType Modes;

		for (const auto& SpeedMode : CameraSpeedTable)
			for (auto GainMode : SpeedMode.Gains)
				Modes.emplace_back(SpeedMode.Port.second + ": " + Util::ToStr(SpeedMode.ReadoutFrequency, 0) + " MHz, "
					+ Util::ToStr(GainMode.BitDepth) + " bit, gain " + Util::ToStr(GainMode.GainIndex));

		return Modes;
	}

	PVCamHardwareAdapter::TimeType PVCamHardwareAdapter::GetMinExposureTime() const
	{
		auto lock = AcquireLock(HardwareOperationTimeout);

		return MinExpTime;
	}

	PVCamHardwareAdapter::TimeType PVCamHardwareAdapter::GetMaxExposureTime() const
	{
		auto lock = AcquireLock(HardwareOperationTimeout);

		return MaxExpTime;
	}

	PVCamHardwareAdapter::TimeType PVCamHardwareAdapter::GetExposureTime() const
	{
		auto lock = AcquireLock(HardwareOperationTimeout);

		return ExposureTime;
	}

	float PVCamHardwareAdapter::GetFPS() const
	{
		auto lock = AcquireLock(HardwareOperationTimeout);

		return CurrentFPS;
	}

	Util::BlobDataType PVCamHardwareAdapter::GetCurrentImage() const
	{
		auto lock = AcquireLock(HardwareOperationTimeout);

		// Move-constructs new object by stealing from internal data.
		return std::move(CopiedImageData);
	}

	Util::BlobDataType PVCamHardwareAdapter::GetCurrentImageCopy() const
	{
		auto lock = AcquireLock(HardwareOperationTimeout);

		// Invokes copy-constructor.
		return CopiedImageData;
	}

	void PVCamHardwareAdapter::SetCameraMode(size_t ID) const
	{
		auto lock = AcquireLock(HardwareOperationTimeout);

		if (CameraState == CameraStateType::CapturingContinuously)
		{
			StopCapturingUnsafe();
			SetCameraModeUnsafe(ID);
			StartCapturingUnsafe();
		}
		else
			SetCameraModeUnsafe(ID);
	}

	void PVCamHardwareAdapter::SetExposureTime(TimeType Time) const
	{
		auto lock = AcquireLock(HardwareOperationTimeout);

		if (Time < MinExpTime || Time > MaxExpTime)
			return;

		ExposureTime = Time;

		if (CameraState == CameraStateType::CapturingContinuously)
		{
			// If running continuously, apply new exposure time
			StopCapturingUnsafe();
			StartCapturingUnsafe();
		}
	}

	void PVCamHardwareAdapter::CaptureSingle() const
	{
		using namespace PVCamSyms;

		auto lock = AcquireLock(HardwareOperationTimeout);

		if (CameraState != CameraStateType::Stopped)
			StopCapturingUnsafe();

		auto PVCamLock = PVCamInitializer::Lock();

		uns32 BytesRequired = 0;
		auto CaptureRegion = Region;
		--CaptureRegion.s2;
		--CaptureRegion.p2;
		auto Result = pl_exp_setup_seq(PVCamHandle, 1, 1, &CaptureRegion, TIMED_MODE, static_cast<uns32>(ExposureTime.count()), &BytesRequired);
		CheckError(Result);

		ReserveMemory(BytesRequired);

		ReadExposureTimeUnsafe();
		CameraState = CameraStateType::CapturingSingle;

		Result = pl_exp_start_seq(PVCamHandle, ImageData.GetPtr());
		CheckError(Result);
	}

	void PVCamHardwareAdapter::StartCapturing() const
	{
		auto lock = AcquireLock(HardwareOperationTimeout);

		if (CameraState != CameraStateType::Stopped)
			return;
		
		StartCapturingUnsafe();
	}

	void PVCamHardwareAdapter::StopCapturing() const
	{
		auto lock = AcquireLock(HardwareOperationTimeout);

		if (CameraState == CameraStateType::Stopped)
			return;

		StopCapturingUnsafe();
	}

	void PV_DECL PVCamHardwareAdapter::NewFrameCallback(PVCamSyms::FRAME_INFO* FrameInfo, void* PVCamHardwareAdapterPtr)
	{
		if (PVCamHardwareAdapterPtr)
			static_cast<PVCamHardwareAdapter*>(PVCamHardwareAdapterPtr)->NewFrame(FrameInfo);
	}

	void PVCamHardwareAdapter::Init()
	{
		auto DerivedParams = dynamic_Params_cast<PVCamHardwareAdapter>(GetParams());

		CameraName = DerivedParams->CameraName.Get();
		CameraSpeedTable.clear();
		MinExpTime = TimeType();
		MaxExpTime = TimeType();
		ExposureTime = std::chrono::milliseconds(100);
		CurrentFPS = 0.f;
		Region = { 0, 0, 1, 0, 0, 1 };
		BitDepth = 0;
		ColorMode = PVCamSyms::PL_COLOR_MODES::COLOR_NONE;

		PVCamHandleValid = false;
		PVCamHandle = -1;
	}

	void PVCamHardwareAdapter::ResetImpl(dispatch_tag<HardwareAdapterBase>)
	{
		// auto lock = AcquireLock(); not necessary here, since DynExp ensures that Object::Reset() can only
		// be called if respective object is not in use.

		CloseUnsafe();

		ImageData.Reset();
		CopiedImageData.Reset();

		Init();

		ResetImpl(dispatch_tag<PVCamHardwareAdapter>());
	}

	void PVCamHardwareAdapter::EnsureReadyStateChild()
	{
		PVCamInitializer::Init();

		auto lock = AcquireLock(HardwareOperationTimeout);

		OpenUnsafe();
	}

	bool PVCamHardwareAdapter::IsReadyChild() const
	{
		auto lock = AcquireLock(HardwareOperationTimeout);

		auto Exception = GetExceptionUnsafe();
		Util::ForwardException(Exception);

		return IsOpened();
	}

	bool PVCamHardwareAdapter::IsConnectedChild() const noexcept
	{
		return IsOpened();
	}

	void PVCamHardwareAdapter::CheckError(const PVCamSyms::rs_bool Result, bool OnlyLog, const std::source_location Location) const
	{
		if (Result == PVCamSyms::PV_OK)
			return;

		auto ErrorCode = PVCamSyms::pl_error_code();

		// Since C++17, writing to std::string's internal buffer is allowed.
		std::string ErrorString;
		ErrorString.resize(ERROR_MSG_LEN);
		PVCamSyms::pl_error_message(ErrorCode, ErrorString.data());
		ErrorString = Util::TrimTrailingZeros(ErrorString);
		PVCamException Exception(ErrorString, ErrorCode, OnlyLog ? Util::ErrorType::Warning : Util::ErrorType::Error, Location);

		if (!OnlyLog)
		{
			// AcquireLock() has already been called by an (in)direct caller of this function.
			ThrowExceptionUnsafe(std::make_exception_ptr(Exception));
		}
		else
		{
			Util::EventLog().Log(Exception);

			// Does not store exception in hardware adapter. Might be useful if the error just occurs temporarily for one frame.
			throw Exception;
		}
	}

	void PVCamHardwareAdapter::OpenUnsafe()
	{
		using namespace PVCamSyms;

		if (IsOpened())
			return;

		auto PVCamLock = PVCamInitializer::Lock();

		// Copy since pl_cam_open() unfortunately expects pointer to char* rather than const char*...
		auto TmpName = CameraName;
		auto Result = pl_cam_open(TmpName.data(), &PVCamHandle, OPEN_EXCLUSIVE);
		CheckError(Result);

		PVCamHandleValid = true;

		SetupSpeedTableUnsafe();
		ReadMinMaxExposureTimeUnsafe();

		// Additional configuration and parameter readout
		Result = pl_get_param(PVCamHandle, PARAM_SER_SIZE, ATTR_CURRENT, &Region.s2);
		CheckError(Result);
		Result = pl_get_param(PVCamHandle, PARAM_PAR_SIZE, ATTR_CURRENT, &Region.p2);
		CheckError(Result);

		int32 ColorModeAvlbl = 0;
		Result = pl_get_param(PVCamHandle, PARAM_COLOR_MODE, ATTR_AVAIL, &ColorModeAvlbl);
		CheckError(Result);
		if (ColorModeAvlbl)
		{
			Result = pl_get_param(PVCamHandle, PARAM_COLOR_MODE, ATTR_CURRENT, &ColorMode);
			CheckError(Result);
		}

		uns16 ExposueTimeMode = EXP_RES_ONE_MILLISEC;
		Result = pl_set_param(PVCamHandle, PARAM_EXP_RES_INDEX, &ExposueTimeMode);
		CheckError(Result);

		Result = pl_cam_register_callback_ex3(PVCamHandle, PL_CALLBACK_EOF, &NewFrameCallback, this);
		CheckError(Result);
	}

	void PVCamHardwareAdapter::CloseUnsafe()
	{
		if (IsOpened())
		{
			StopCapturingUnsafe();

			// Does not matter whether lock is interrupted in bewtween here and call to StopCapturing().
			auto PVCamLock = PVCamInitializer::Lock();

			// Handle now considered invalid, even if pl_cam_close() fails.
			PVCamHandleValid = false;

			auto Result = PVCamSyms::pl_cam_deregister_callback(PVCamHandle, PVCamSyms::PL_CALLBACK_EOF);
			CheckError(Result);
			Result = PVCamSyms::pl_cam_close(PVCamHandle);
			CheckError(Result);
		}
	}

	void PVCamHardwareAdapter::ReserveMemory(const PVCamSyms::uns32 BytesRequired) const
	{
		if (!BytesRequired)
			ThrowExceptionUnsafe(std::make_exception_ptr(Util::InvalidDataException("Error retrieving required memory size from PVCAM camera.")));

		ImageData.Reserve(BytesRequired);
	}

	PVCamHardwareAdapter::PVCamEnumVectorType PVCamHardwareAdapter::ReadPVCamEnumUnsafe(PVCamSyms::uns32 ParamID, std::string ParamName)
	{
		using namespace PVCamSyms;

		uns32 Count;
		auto Result = pl_get_param(PVCamHandle, ParamID, ATTR_COUNT, &Count);
		CheckError(Result);

		PVCamEnumVectorType PVCamEnumVector;

		// Query trigger/exposure configuration names.
		for (decltype(Count) i = 0; i < Count; ++i)
		{
			// Determine required string length.
			uns32 StrLength;
			Result = pl_enum_str_length(PVCamHandle, ParamID, i, &StrLength);
			CheckError(Result);

			std::string Name;
			Name.resize(StrLength);

			// Query key-value pair.
			int32 Value;
			Result = pl_get_enum_param(PVCamHandle, ParamID, i, &Value, Name.data(), StrLength);
			CheckError(Result);
 
			PVCamEnumVector.emplace_back(Value, std::move(Name));
		}

		return PVCamEnumVector;
	}

	void PVCamHardwareAdapter::SetupSpeedTableUnsafe()
	{
		using namespace PVCamSyms;

		auto Ports = ReadPVCamEnumUnsafe(PARAM_READOUT_PORT, "PARAM_READOUT_PORT");

		CameraSpeedTable.clear();

		// Determine camera speed for each available port.
		for (auto& Port : Ports)
		{
			// Select readout port
			auto Result = pl_set_param(PVCamHandle, PARAM_READOUT_PORT, &Port.first);
			CheckError(Result);

			// Query amount of available camera speed modes for the selected port.
			uns32 SpeedCount;
			Result = pl_get_param(PVCamHandle, PARAM_SPDTAB_INDEX, ATTR_COUNT, &SpeedCount);
			CheckError(Result);

			// Determine properties of each available camera speed mode.
			for (int16 i = 0; i < Util::NumToT<int16>(SpeedCount); ++i)
			{
				// First, select speed mode by index.
				// Copy since pl_set_param() unfortunately expects non-const pointer...
				auto Tmpi = i;
				Result = pl_set_param(PVCamHandle, PARAM_SPDTAB_INDEX, &Tmpi);
				CheckError(Result);

				// Second, query the time (in ns) required to read out a single pixel.
				uns16 PixelTime;
				Result = pl_get_param(PVCamHandle, PARAM_PIX_TIME, ATTR_CURRENT, &PixelTime);
				CheckError(Result);

				// Third, obtain gain information on the selected camera speed mode.
				int16 GainMin, GainMax, GainIncrement;
				Result = pl_get_param(PVCamHandle, PARAM_GAIN_INDEX, ATTR_MIN, &GainMin);
				CheckError(Result);
				Result = pl_get_param(PVCamHandle, PARAM_GAIN_INDEX, ATTR_MAX, &GainMax);
				CheckError(Result);
				Result = pl_get_param(PVCamHandle, PARAM_GAIN_INDEX, ATTR_INCREMENT, &GainIncrement);
				CheckError(Result);

				// Last, query bit depth for each gain mode.
				PVCamReadoutOptionType PVCamReadoutOption(Port, i, 1000.f / PixelTime);
				for (auto Gain = GainMin; Gain <= GainMax; Gain += GainIncrement)
				{
					// Copy since pl_set_param() unfortunately expects non-const pointer...
					auto TmpGain = Gain;
					Result = pl_set_param(PVCamHandle, PARAM_GAIN_INDEX, &TmpGain);
					CheckError(Result);

					int16 BitDepth;
					Result = pl_get_param(PVCamHandle, PARAM_BIT_DEPTH, ATTR_CURRENT, &BitDepth);
					CheckError(Result);

					PVCamReadoutOption.Gains.push_back({ Gain, BitDepth });
				}

				if (PVCamReadoutOption.Gains.empty())
					ThrowExceptionUnsafe(std::make_exception_ptr(Util::EmptyException(
						"The gain list of a speed table's entry of the selected PVCam camera is empty.")));

				CameraSpeedTable.push_back(std::move(PVCamReadoutOption));
			}
		}

		// By default, select the first available port, the first camera speed mode, and the first gain mode.
		SetCameraModeUnsafe(0);
	}

	void PVCamHardwareAdapter::SetCameraModeUnsafe(size_t ID) const
	{
		using namespace PVCamSyms;

		if (CameraSpeedTable.empty())
			ThrowExceptionUnsafe(std::make_exception_ptr(Util::EmptyException("The speed table of the selected PVCam camera is empty.")));

		decltype(ID) CurrentIndex = 0;
		for (const auto& SpeedMode : CameraSpeedTable)
		{
			for (auto GainMode : SpeedMode.Gains)
			{
				if (ID == CurrentIndex)
				{
					// Set port. Copy value since pl_set_param() unfortunately expects non-const pointer...
					auto PortID = SpeedMode.Port.first;
					auto Result = pl_set_param(PVCamHandle, PARAM_READOUT_PORT, &PortID);
					CheckError(Result);
					BitDepth = GainMode.BitDepth;

					// Set speed index.
					auto SpeedIndex = SpeedMode.SpeedIndex;
					Result = pl_set_param(PVCamHandle, PARAM_SPDTAB_INDEX, &SpeedIndex);
					CheckError(Result);

					// Set gain mode.
					auto GainIndex = GainMode.GainIndex;
					Result = pl_set_param(PVCamHandle, PARAM_GAIN_INDEX, &GainIndex);
					CheckError(Result);

					ReadMinMaxExposureTimeUnsafe();

					return;
				}

				++CurrentIndex;
			}
		}

		ThrowExceptionUnsafe(std::make_exception_ptr(Util::InvalidArgException("The given ID does not correspond to an entry in the camera's speed table.")));
	}

	void PVCamHardwareAdapter::StartCapturingUnsafe() const
	{
		auto PVCamLock = PVCamInitializer::Lock();

		PVCamSyms::uns32 BytesRequired = 0;
		auto CaptureRegion = Region;
		--CaptureRegion.s2;
		--CaptureRegion.p2;
		auto Result = pl_exp_setup_cont(PVCamHandle, 1, &CaptureRegion, PVCamSyms::TIMED_MODE,
			static_cast<PVCamSyms::uns32>(ExposureTime.count()), &BytesRequired, PVCamSyms::CIRC_OVERWRITE);
		CheckError(Result);

		ReserveMemory(BytesRequired * NumFramesInBuffer);

		ReadExposureTimeUnsafe();
		CameraState = CameraStateType::CapturingContinuously;

		Result = PVCamSyms::pl_exp_start_cont(PVCamHandle, ImageData.GetPtr(),
			Util::NumToT<PVCamSyms::uns32>(ImageData.Size() / BytesPerPixel()));
		CheckError(Result);
	}

	void PVCamHardwareAdapter::StopCapturingUnsafe() const
	{
		auto PVCamLock = PVCamInitializer::Lock();

		if (CameraState == CameraStateType::CapturingContinuously)
		{
			auto Result = PVCamSyms::pl_exp_stop_cont(PVCamHandle, PVCamSyms::CCS_CLEAR);
			CheckError(Result);
		}
		
		auto Result = PVCamSyms::pl_exp_abort(PVCamHandle, PVCamSyms::CCS_CLEAR);
		CheckError(Result);
		
		CameraState = CameraStateType::Stopped;
	}

	void PVCamHardwareAdapter::ReadMinMaxExposureTimeUnsafe() const
	{
		using namespace PVCamSyms;

		ulong64 IntMinExpTime, IntMaxExpTime;
		auto Result = pl_get_param(PVCamHandle, PARAM_EXPOSURE_TIME, ATTR_MIN, &IntMinExpTime);
		CheckError(Result);
		Result = pl_get_param(PVCamHandle, PARAM_EXPOSURE_TIME, ATTR_MAX, &IntMaxExpTime);
		CheckError(Result);

		MinExpTime = std::chrono::milliseconds(IntMinExpTime);
		MaxExpTime = std::chrono::milliseconds(IntMaxExpTime);
	}

	void PVCamHardwareAdapter::ReadExposureTimeUnsafe() const
	{
		PVCamSyms::ulong64 CurrentExpTime;
		auto Result = PVCamSyms::pl_get_param(PVCamHandle, PARAM_EXPOSURE_TIME, PVCamSyms::ATTR_CURRENT, &CurrentExpTime);
		CheckError(Result);

		ExposureTime = std::chrono::milliseconds(CurrentExpTime);
	}

	void PVCamHardwareAdapter::NewFrame(PVCamSyms::FRAME_INFO* FrameInfo) noexcept
	{
		try
		{
			auto lock = AcquireLock(HardwareOperationTimeout);

			CurrentFPS = 0.f;

			if (CameraState == CameraStateType::Stopped)
				return;

			if (CameraState == CameraStateType::CapturingSingle)
			{
				// Deep copy since PVCam library might overwrite ImageData outside its callbacks at any time.
				CopiedImageData = ImageData;

				CameraState = CameraStateType::Stopped;

				{
					auto PVCamLock = PVCamInitializer::Lock();

					auto Result = PVCamSyms::pl_exp_finish_seq(PVCamHandle, ImageData.GetPtr(), 0);
					CheckError(Result, true);
				}
			}
			else if (CameraState == CameraStateType::CapturingContinuously)
			{
				unsigned char* FrameAdr = nullptr;
				{
					auto PVCamLock = PVCamInitializer::Lock();

					auto Result = PVCamSyms::pl_exp_get_latest_frame(PVCamHandle, reinterpret_cast<void**>(&FrameAdr));
					CheckError(Result, true);
				}

				CopiedImageData.Assign(ImageData.Size() / NumFramesInBuffer, FrameAdr);

				static std::chrono::time_point<std::chrono::system_clock> LastCall;

				auto now = std::chrono::system_clock::now();
				if (LastCall.time_since_epoch().count())
					CurrentFPS = 1000.f / std::chrono::duration_cast<std::chrono::milliseconds>(now - LastCall).count();

				LastCall = now;
			}
		}
		catch (...)
		{
			// Swallow any exception since this function is called by a PVCAM thread which does not handle exceptions.
		}
	}
}