// This file is part of DynExp.

#include "stdafx.h"
#include "DummyCamera.h"

namespace DynExpInstr
{
	void DummyCameraTasks::InitTask::InitFuncImpl(dispatch_tag<CameraTasks::InitTask>, DynExp::InstrumentInstance& Instance)
	{
		{
			auto InstrParams = DynExp::dynamic_Params_cast<DummyCamera>(Instance.ParamsGetter());
			auto InstrData = DynExp::dynamic_InstrumentData_cast<DummyCamera>(Instance.InstrumentDataGetter());

			InstrData->DummyImage = QImage(QString::fromStdString(InstrParams->ImagePath.GetPath().string()));
			if (InstrData->DummyImage.isNull())
				throw Util::InvalidDataException("Could not load image from \"" + InstrParams->ImagePath.Get() + "\"");
		} // InstrParams and InstrData unlocked here.

		InitFuncImpl(dispatch_tag<InitTask>(), Instance);
	}

	QImage DummyCameraTasks::UpdateTask::ObtainImage(DynExp::InstrumentInstance& Instance)
	{
		QImage Image;
		CameraData::ImageTransformationType ImageTransformation;
		{
			auto InstrData = DynExp::dynamic_InstrumentData_cast<DummyCamera>(Instance.InstrumentDataGetter());

			Image = InstrData->DummyImage;
			ImageTransformation = InstrData->GetImageTransformation();
		} // InstrData unlocked here.

		if (ImageTransformation.IsEnabled)
		{
			auto ImageBlobPtr = Image.bits();

			long long MaxIndex = 0;
			switch (Image.format())
			{
			case QImage::Format::Format_ARGB32:
			case QImage::Format::Format_RGB32:
				MaxIndex = Image.width() * Image.height() * 4;
				break;
			case QImage::Format::Format_Grayscale8:
			case QImage::Format::Format_Grayscale16:
				MaxIndex = Image.width() * Image.height();
				break;
			default:
				throw Util::NotImplementedException("An image format with the given format cannot be manipulated.");
			}

			for (long long i = 0; i < MaxIndex; ++i)
				if (Image.format() == QImage::Format::Format_Grayscale16)
				{
					uint16_t Pixel = *(ImageBlobPtr) | (*(ImageBlobPtr + 1) << 8);
					Pixel = TransformPixel(Pixel, ImageTransformation);

					// Write back
					*(ImageBlobPtr) = Pixel & 0xff;
					*(ImageBlobPtr + 1) = (Pixel & 0xff00) >> 8;

					ImageBlobPtr += 2;
				}
				else
				{
					*ImageBlobPtr = TransformPixel(*ImageBlobPtr, ImageTransformation);
					ImageBlobPtr++;
				}
		}

		return Image;
	}

	void DummyCameraData::ResetImpl(dispatch_tag<CameraData>)
	{
		DummyImage = QImage();

		ResetImpl(dispatch_tag<DummyCameraData>());
	}

	DummyCamera::DummyCamera(const std::thread::id OwnerThreadID, DynExp::ParamsBasePtrType&& Params)
		: Camera(OwnerThreadID, std::move(Params))
	{
	}

	void DummyCamera::ResetImpl(dispatch_tag<Camera>)
	{
		ResetImpl(dispatch_tag<DummyCamera>());
	}
}