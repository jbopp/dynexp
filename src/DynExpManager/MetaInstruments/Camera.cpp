// This file is part of DynExp.

#include "stdafx.h"
#include "Camera.h"

namespace DynExpInstr
{
	void CameraTasks::UpdateTask::UpdateFuncImpl(dispatch_tag<UpdateTaskBase>, DynExp::InstrumentInstance& Instance)
	{
		auto Image = ObtainImage(Instance);

		if (Image.isNull())
			return;

		CameraData::ComputeHistogramType ComputeHistogram;
		Util::ImageRGBHistogramType RGBHistogram;
		Util::ImageHistogramType IntensityHistogram;

		{
			auto InstrData = DynExp::dynamic_InstrumentData_cast<Camera>(Instance.InstrumentDataGetter());

			ComputeHistogram = InstrData->GetComputeHistogram();
		} // Unlock InstrData for probably expensive calculation.

		if (ComputeHistogram != CameraData::ComputeHistogramType::NoHistogram)
		{
			if (ComputeHistogram == CameraData::ComputeHistogramType::IntensityHistogram)
				IntensityHistogram = Util::ComputeIntensityHistogram(Image);
			else
			{
				RGBHistogram = Util::ComputeRGBHistogram(Image);

				if (ComputeHistogram == CameraData::ComputeHistogramType::IntensityHistogram ||
					ComputeHistogram == CameraData::ComputeHistogramType::IntensityAndRGBHistogram)
					IntensityHistogram = Util::ConvertRGBToIntensityHistogram(RGBHistogram);
			}
		}

		{
			auto InstrData = DynExp::dynamic_InstrumentData_cast<Camera>(Instance.InstrumentDataGetter());

			InstrData->SetImage(std::move(Image));

			if (ComputeHistogram == CameraData::ComputeHistogramType::IntensityHistogram ||
				ComputeHistogram == CameraData::ComputeHistogramType::IntensityAndRGBHistogram)
				InstrData->SetIntensityHistogram(std::move(IntensityHistogram));
			if (ComputeHistogram == CameraData::ComputeHistogramType::RGBHistogram ||
				ComputeHistogram == CameraData::ComputeHistogramType::IntensityAndRGBHistogram)
				InstrData->SetRGBHistogram(std::move(RGBHistogram));
		} // InstrData unlocked here.

		UpdateFuncImpl(dispatch_tag<UpdateTask>(), Instance);
	}

	QImage CameraData::GetImage() const
	{
		if (!IsImageAvailbale())
			throw Util::EmptyException("There is currently no image.");

		// Move-constructs new object by stealing from internal data.
		return std::move(CurrentImage);
	}

	QImage CameraData::GetImageCopy(const QRect& RegionOfInterest) const
	{
		if (!IsImageAvailbale())
			throw Util::EmptyException("There is currently no image.");

		return CurrentImage.copy(RegionOfInterest);
	}

	void CameraData::SetImage(QImage&& Other)
	{
		CurrentImage = std::move(Other);
	}

	void CameraData::ClearImage() const
	{
		CurrentImage = QImage();

		IntensityHistogram = {};
		RGBHistogram = {};
	}

	void CameraData::ResetImpl(dispatch_tag<InstrumentDataBase>)
	{
		ImageWidth = 0;
		ImageHeight = 0;

		CameraModes.clear();

		MinExposureTime = TimeType();
		MaxExposureTime = TimeType();
		CurrentExposureTime = TimeType();
		CurrentFPS = 0.f;

		ComputeHistogram = ComputeHistogramType::NoHistogram;
		ImageTransformation = {};

		ClearImage();

		ResetImpl(dispatch_tag<CameraData>());
	}

	CameraParams::~CameraParams()
	{
	}

	CameraConfigurator::~CameraConfigurator()
	{
	}

	Camera::~Camera()
	{
	}

	void Camera::SetCameraMode(size_t ID, DynExp::TaskBase::CallbackType CallbackFunc) const
	{
		throw Util::NotImplementedException();
	}

	void Camera::SetExposureTime(const CameraData::TimeType ExposureTime, DynExp::TaskBase::CallbackType CallbackFunc) const
	{
		throw Util::NotImplementedException();
	}

	void Camera::SetExposureTimeSync(const CameraData::TimeType ExposureTime) const
	{
		AsSyncTask(&Camera::SetExposureTime, ExposureTime);
	}

	void Camera::StopCapturingSync() const
	{
		AsSyncTask(&Camera::StopCapturing);
	}

	void Camera::ResetImpl(dispatch_tag<InstrumentBase>)
	{
		ResetImpl(dispatch_tag<Camera>());
	}
}