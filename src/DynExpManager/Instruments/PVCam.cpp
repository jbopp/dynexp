// This file is part of DynExp.

#include "stdafx.h"
#include "PVCam.h"

namespace DynExpInstr
{
	void PVCamTasks::InitTask::InitFuncImpl(dispatch_tag<CameraTasks::InitTask>, DynExp::InstrumentInstance& Instance)
	{
		{
			auto InstrParams = DynExp::dynamic_Params_cast<PVCam>(Instance.ParamsGetter());
			auto InstrData = DynExp::dynamic_InstrumentData_cast<PVCam>(Instance.InstrumentDataGetter());

			Instance.LockObject(InstrParams->HardwareAdapter, InstrData->HardwareAdapter);

			auto ColorMode = InstrData->HardwareAdapter->GetColorMode();
			if (ColorMode != DynExpHardware::PVCamSyms::PL_COLOR_MODES::COLOR_NONE)
				throw Util::NotImplementedException("Only monochrome PVCam cameras are supported.");
			static_assert(DynExpHardware::PVCamHardwareAdapter::BytesPerPixel() == 2,
				"Only monochrome PVCam cameras with an image format of 2 bytes per pixel are supported.");

			InstrData->SetImageWidth(InstrData->HardwareAdapter->GetImageWidth());
			InstrData->SetImageHeight(InstrData->HardwareAdapter->GetImageHeight());
			InstrData->SetCameraModes(InstrData->HardwareAdapter->GetCameraModes());
		} // InstrParams and InstrData unlocked here.

		InitFuncImpl(dispatch_tag<InitTask>(), Instance);
	}

	void PVCamTasks::ExitTask::ExitFuncImpl(dispatch_tag<CameraTasks::ExitTask>, DynExp::InstrumentInstance& Instance)
	{
		ExitFuncImpl(dispatch_tag<ExitTask>(), Instance);

		auto InstrData = DynExp::dynamic_InstrumentData_cast<PVCam>(Instance.InstrumentDataGetter());

		try
		{
			// Stop image capturing if this is the only or last instrument using the camera.
			if (!InstrData->HardwareAdapter->IsSharedUsageEnabled() || InstrData->HardwareAdapter->GetUseCount() == 1)
				InstrData->HardwareAdapter->StopCapturing();
		}
		catch (...)
		{
			// Swallow any exception which might arise from HardwareAdapter->StopCapturing() since a failure
			// of this function is not considered a severe error.
		}

		Instance.UnlockObject(InstrData->HardwareAdapter);
	}

	QImage PVCamTasks::UpdateTask::ObtainImage(DynExp::InstrumentInstance& Instance)
	{
		try
		{
			Util::BlobDataType ImageBlob;
			CameraData::ImageDimensionType ImageWidth = 0, ImageHeight = 0;
			CameraData::ImageTransformationType ImageTransformation;
			unsigned int BitDepth = 16;

			{
				auto InstrData = DynExp::dynamic_InstrumentData_cast<PVCam>(Instance.InstrumentDataGetter());
				bool UpdateError = false;

				try
				{
					InstrData->SetMinExposureTime(InstrData->HardwareAdapter->GetMinExposureTime());
					InstrData->SetMaxExposureTime(InstrData->HardwareAdapter->GetMaxExposureTime());
					InstrData->SetExposureTime(InstrData->HardwareAdapter->GetExposureTime());
					InstrData->SetCurrentFPS(InstrData->HardwareAdapter->GetFPS());
					InstrData->SetCapturingState(InstrData->HardwareAdapter->GetCameraState());

					BitDepth = InstrData->HardwareAdapter->GetBitDepth();
					ImageWidth = InstrData->GetImageWidth();
					ImageHeight = InstrData->GetImageHeight();
					ImageTransformation = InstrData->GetImageTransformation();
					ImageBlob = InstrData->HardwareAdapter->GetCurrentImage();
				}
				catch ([[maybe_unused]] const DynExpHardware::PVCamException& e)
				{
					UpdateError = true;

					// Swallow if just one or two subsequent updates failed.
					if (InstrData->NumFailedStatusUpdateAttempts++ >= 3)
						throw;
				}

				if (!UpdateError)
					InstrData->NumFailedStatusUpdateAttempts = 0;
			}	// InstrData unlocked here.

			// Compute histogram stretch by bit depth of camera
			auto ImageBlobPtr = ImageBlob.GetPtr();
			for (auto i = 0; i < ImageBlob.Size(); i += 2)
			{
				// Convert pixel value by stretching the histogram according to the camera's bit depth.
				DynExpHardware::PVCamSyms::uns16 Pixel = (*(ImageBlobPtr) | (*(ImageBlobPtr + 1) << 8)) * std::exp2(16) / std::exp2(BitDepth);

				// Apply user transformations
				if (ImageTransformation.IsEnabled)
					Pixel = TransformPixel(Pixel, ImageTransformation);

				// Write back
				*(ImageBlobPtr) = Pixel & 0xff;
				*(ImageBlobPtr + 1) = (Pixel & 0xff00) >> 8;

				ImageBlobPtr += 2;
			}

			// Image moved by copy elision.
			return Util::QImageFromBlobData(std::move(ImageBlob), ImageWidth, ImageHeight,
				ImageWidth * DynExpHardware::PVCamHardwareAdapter::BytesPerPixel(), QImage::Format_Grayscale16);
		}
		// Issued if a mutex is blocked by another operation.
		catch (const Util::TimeoutException& e)
		{
			Instance.GetOwner().SetWarning(e);

			return {};
		}
		// Issued if reading data from PVCam failed.
		catch (const DynExpHardware::PVCamException& e)
		{
			Instance.GetOwner().SetWarning(e);

			return {};
		}
	}

	DynExp::TaskResultType PVCamTasks::SetCameraMode::RunChild(DynExp::InstrumentInstance& Instance)
	{
		auto InstrData = DynExp::dynamic_InstrumentData_cast<PVCam>(Instance.InstrumentDataGetter());

		InstrData->HardwareAdapter->SetCameraMode(ID);

		return {};
	}

	DynExp::TaskResultType PVCamTasks::SetExposureTimeTask::RunChild(DynExp::InstrumentInstance& Instance)
	{
		auto InstrData = DynExp::dynamic_InstrumentData_cast<PVCam>(Instance.InstrumentDataGetter());

		InstrData->HardwareAdapter->SetExposureTime(ExposureTime);

		return {};
	}

	DynExp::TaskResultType PVCamTasks::CaptureSingleTask::RunChild(DynExp::InstrumentInstance& Instance)
	{
		auto InstrData = DynExp::dynamic_InstrumentData_cast<PVCam>(Instance.InstrumentDataGetter());

		InstrData->HardwareAdapter->CaptureSingle();

		return {};
	}

	DynExp::TaskResultType PVCamTasks::StartCapturingTask::RunChild(DynExp::InstrumentInstance& Instance)
	{
		auto InstrData = DynExp::dynamic_InstrumentData_cast<PVCam>(Instance.InstrumentDataGetter());

		InstrData->HardwareAdapter->StartCapturing();

		return {};
	}

	DynExp::TaskResultType PVCamTasks::StopCapturingTask::RunChild(DynExp::InstrumentInstance& Instance)
	{
		auto InstrData = DynExp::dynamic_InstrumentData_cast<PVCam>(Instance.InstrumentDataGetter());

		InstrData->HardwareAdapter->StopCapturing();

		return {};
	}

	void PVCamData::ResetImpl(dispatch_tag<CameraData>)
	{
		CapturingState = CapturingStateType::Stopped;
		NumFailedStatusUpdateAttempts = 0;

		ResetImpl(dispatch_tag<PVCamData>());
	}

	PVCam::PVCam(const std::thread::id OwnerThreadID, DynExp::ParamsBasePtrType&& Params)
		: Camera(OwnerThreadID, std::move(Params))
	{
	}

	void PVCam::ResetImpl(dispatch_tag<Camera>)
	{
		ResetImpl(dispatch_tag<PVCam>());
	}
}