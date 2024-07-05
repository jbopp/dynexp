// This file is part of DynExp.

/**
 * @file HardwareAdapterPVCam.h
 * @brief Implementation of a hardware adapter to control Teledyne Photometrics PVCam
 * hardware. Windows only. Requires PVCam from photometrics.com to be installed.
*/

#pragma once

#include "stdafx.h"
#include "HardwareAdapter.h"
#include "../MetaInstruments/Camera.h"

#undef DEPRECATED
namespace DynExpHardware::PVCamSyms
{
	#include "../include/PVCam/master.h"
	#include "../include/PVCam/pvcam.h"
}

namespace DynExpHardware
{
	class PVCamHardwareAdapter;

	class PVCamException : public Util::Exception
	{
	public:
		PVCamException(std::string Description, const int ErrorCode, const Util::ErrorType Type = Util::ErrorType::Error,
			const std::source_location Location = std::source_location::current()) noexcept
			: Exception(std::move(Description), Type, ErrorCode, Location)
		{}
	};

	class PVCamHardwareAdapterParams : public DynExp::HardwareAdapterParamsBase
	{
	public:
		PVCamHardwareAdapterParams(DynExp::ItemIDType ID, const DynExp::DynExpCore& Core) : HardwareAdapterParamsBase(ID, Core) {}
		virtual ~PVCamHardwareAdapterParams() = default;

		virtual const char* GetParamClassTag() const noexcept override { return "PVCamHardwareAdapterParams"; }

		Param<TextList> CameraName = { *this, {}, "CameraName", "Camera name",
			"Name of the PVCam camera to connect with" };

	private:
		void ConfigureParamsImpl(dispatch_tag<HardwareAdapterParamsBase>) override final;
		virtual void ConfigureParamsImpl(dispatch_tag<PVCamHardwareAdapterParams>) {}
	};

	class PVCamHardwareAdapterConfigurator : public DynExp::HardwareAdapterConfiguratorBase
	{
	public:
		using ObjectType = PVCamHardwareAdapter;
		using ParamsType = PVCamHardwareAdapterParams;

		PVCamHardwareAdapterConfigurator() = default;
		virtual ~PVCamHardwareAdapterConfigurator() = default;

	private:
		virtual DynExp::ParamsBasePtrType MakeParams(DynExp::ItemIDType ID, const DynExp::DynExpCore& Core) const override { return DynExp::MakeParams<PVCamHardwareAdapterConfigurator>(ID, Core); }
	};

	class PVCamHardwareAdapter : public DynExp::HardwareAdapterBase
	{
		/**
		 * @brief Only one instance of this class is allowed for synchronizing calls to
		 * the PVCam library from any @p PVCamHardwareAdapter instance.
		 * @warning Always lock the mutex of @p PVCamHardwareAdapter (by a call to
		 * Util::ILockable::AcquireLock()) before the @p PVCamInitializer (by a call to
		 * PVCamInitializer::Lock()).
		*/
		class PVCamInitializer : public Util::ILockable
		{
		private:
			PVCamInitializer() = default;
			~PVCamInitializer();

		public:
			static void Init() { GetInstance(true); }
			static auto GetIsInitialized() noexcept { return GetInstance().IsInitialized.load(); }

			// Synchronizes every call to PVCam library from anywhere.
			static [[nodiscard]] LockType Lock(const std::chrono::milliseconds Timeout = std::chrono::milliseconds(100));

		private:
			// Does not throw if MayInit = false.
			static PVCamInitializer& GetInstance(bool MayInit = false);

			std::atomic<bool> IsInitialized = false;
		};

	public:
		using ParamsType = PVCamHardwareAdapterParams;
		using ConfigType = PVCamHardwareAdapterConfigurator;

		using TimeType = std::chrono::milliseconds;

		constexpr static auto Name() noexcept { return "PVCam"; }
		constexpr static auto Category() noexcept { return "Image Capturing"; }
		constexpr static auto BytesPerPixel() noexcept { return 2; }
		static auto Enumerate();

		PVCamHardwareAdapter(const std::thread::id OwnerThreadID, DynExp::ParamsBasePtrType&& Params);
		virtual ~PVCamHardwareAdapter();

		virtual std::string GetName() const override { return Name(); }
		virtual std::string GetCategory() const override { return Category(); }

		bool IsOpened() const noexcept { return PVCamHandleValid; }
		auto GetCameraState() const noexcept { return CameraState.load(); }
		decltype(PVCamSyms::rgn_type::s2) GetImageWidth() const;
		decltype(PVCamSyms::rgn_type::p2) GetImageHeight() const;
		DynExpInstr::CameraData::CameraModesType GetCameraModes() const;
		auto GetBitDepth() const noexcept { return BitDepth.load(); }
		auto GetColorMode() const noexcept { return ColorMode.load(); }

		TimeType GetMinExposureTime() const;
		TimeType GetMaxExposureTime() const;
		TimeType GetExposureTime() const;
		float GetFPS() const;

		// GetCurrentImage() returns an object move-constructed from the internal data. Subsequent calls to
		// GetCurrentImage() will return an empty image until a new one has been captured. GetCurrentImageCopy()
		// instead copies the image first and returns the copy which is more expensive.
		Util::BlobDataType GetCurrentImage() const;
		Util::BlobDataType GetCurrentImageCopy() const;

		void SetCameraMode(size_t ID) const;
		void SetExposureTime(TimeType Time) const;

		void CaptureSingle() const;
		void StartCapturing() const;
		void StopCapturing() const;

	private:
		using CameraStateType = DynExpInstr::CameraData::CapturingStateType;
		using PVCamEnumType = std::pair<PVCamSyms::int32, std::string>;
		using PVCamEnumVectorType = std::vector<PVCamEnumType>;

		struct PVCamReadoutOptionType
		{
			struct GainOptionType
			{
				GainOptionType(PVCamSyms::int16 GainIndex, PVCamSyms::int16 BitDepth)
					: GainIndex(GainIndex), BitDepth(BitDepth) {}

				PVCamSyms::int16 GainIndex;
				PVCamSyms::int16 BitDepth;
			};

			PVCamReadoutOptionType(PVCamEnumType Port, PVCamSyms::int16 SpeedIndex, float ReadoutFrequency)
				: Port(std::move(Port)), SpeedIndex(SpeedIndex), ReadoutFrequency(ReadoutFrequency) {}

			PVCamEnumType Port;
			PVCamSyms::int16 SpeedIndex;
			float ReadoutFrequency;	//!< in MHz
			std::vector<GainOptionType> Gains;
		};

		static void PV_DECL NewFrameCallback(PVCamSyms::FRAME_INFO* FrameInfo, void* PVCamHardwareAdapterPtr);

		void Init();

		void ResetImpl(dispatch_tag<HardwareAdapterBase>) override final;
		virtual void ResetImpl(dispatch_tag<PVCamHardwareAdapter>) {}

		void EnsureReadyStateChild() override final;
		bool IsReadyChild() const override final;
		bool IsConnectedChild() const noexcept override final;

		// Not thread-safe, must be called from function calling AcquireLock().
		void CheckError(const PVCamSyms::rs_bool Result, bool OnlyLog = false, const std::source_location Location = std::source_location::current()) const;

		void OpenUnsafe();
		void CloseUnsafe();

		void ReserveMemory(const PVCamSyms::uns32 BytesRequired) const;
		PVCamEnumVectorType ReadPVCamEnumUnsafe(PVCamSyms::uns32 ParamID, std::string ParamName);
		void SetupSpeedTableUnsafe();
		void SetCameraModeUnsafe(size_t ID) const;
		void StartCapturingUnsafe() const;
		void StopCapturingUnsafe() const;
		void ReadMinMaxExposureTimeUnsafe() const;
		void ReadExposureTimeUnsafe() const;
		void NewFrame(PVCamSyms::FRAME_INFO* FrameInfo) noexcept;

		static constexpr unsigned int NumFramesInBuffer = 2;

		mutable std::atomic<CameraStateType> CameraState;

		std::string CameraName;
		std::vector<PVCamReadoutOptionType> CameraSpeedTable;
		mutable TimeType MinExpTime;
		mutable TimeType MaxExpTime;
		mutable TimeType ExposureTime;
		float CurrentFPS;
		PVCamSyms::rgn_type Region;
		mutable std::atomic<PVCamSyms::int16> BitDepth;
		std::atomic<PVCamSyms::PL_COLOR_MODES> ColorMode;

		// PVCam library might write to ImageData at any time while capturing except when it calls
		// callback function NewFrameCallback(). So copy ImageData to CopiedImageData in that function.
		mutable Util::BlobDataType ImageData;
		mutable Util::BlobDataType CopiedImageData;

		std::atomic<bool> PVCamHandleValid;
		PVCamSyms::int16 PVCamHandle;
	};
}