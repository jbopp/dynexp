// This file is part of DynExp.

/**
 * @file Camera.h
 * @brief Defines a meta instrument for an image capturing device.
*/

#pragma once

#include "stdafx.h"
#include "Instrument.h"

namespace DynExpInstr
{
	class Camera;

	/**
	 * @brief Tasks for @p Camera
	*/
	namespace CameraTasks
	{
		/**
		 * @copydoc DynExp::InitTaskBase
		*/
		class InitTask : public DynExp::InitTaskBase
		{
			void InitFuncImpl(dispatch_tag<InitTaskBase>, DynExp::InstrumentInstance& Instance) override final { InitFuncImpl(dispatch_tag<InitTask>(), Instance); }

			/**
			 * @copydoc InitFuncImpl(dispatch_tag<DynExp::InitTaskBase>, DynExp::InstrumentInstance&)
			*/
			virtual void InitFuncImpl(dispatch_tag<InitTask>, DynExp::InstrumentInstance& Instance) {}
		};

		/**
		 * @copydoc DynExp::ExitTaskBase
		*/
		class ExitTask : public DynExp::ExitTaskBase
		{
			void ExitFuncImpl(dispatch_tag<ExitTaskBase>, DynExp::InstrumentInstance& Instance) override final { ExitFuncImpl(dispatch_tag<ExitTask>(), Instance); }

			/**
			 * @copydoc ExitFuncImpl(dispatch_tag<DynExp::ExitTaskBase>, DynExp::InstrumentInstance&)
			*/
			virtual void ExitFuncImpl(dispatch_tag<ExitTask>, DynExp::InstrumentInstance& Instance) {}
		};

		/**
		 * @copydoc DynExp::UpdateTaskBase
		*/
		class UpdateTask : public DynExp::UpdateTaskBase
		{
			void UpdateFuncImpl(dispatch_tag<UpdateTaskBase>, DynExp::InstrumentInstance& Instance) override final;

			/**
			 * @copydoc UpdateFuncImpl(dispatch_tag<DynExp::UpdateTaskBase>, DynExp::InstrumentInstance&)
			*/
			virtual void UpdateFuncImpl(dispatch_tag<UpdateTask>, DynExp::InstrumentInstance& Instance) {}

			/**
			 * @brief Retrieves the current image from the underlying hardware device,
			 * applies image transformations (e.g. brightness/contrast corrections), and
			 * returns the image.
			 * @param Instance Handle to the instrument thread's data
			 * @return Transformed current image. Use e.g. Util::QImageFromBlobData() to
			 * create the image to be returned.
			*/
			virtual QImage ObtainImage(DynExp::InstrumentInstance& Instance) = 0;
		};
	}

	/**
	 * @brief Data class for @p Camera
	*/
	class CameraData : public DynExp::InstrumentDataBase
	{
	public:
		/**
		 * @brief Type describing an image transformation.
		*/
		struct ImageTransformationType
		{
			/**
			 * @brief Factor to enhance the image brightness (between -1 and 1).
			*/
			float BrightnessFactor = 0.f;

			/**
			 * @brief Factor to enhance the image contrast. Valid interval is (0, Inf).
			*/
			float ContrastFactor = 1.f;

			/**
			 * @brief Determines whether the image transformation is to be applied (enabled).
			*/
			bool IsEnabled = false;
		};

		using ImageDimensionType = unsigned int;			//!< Type describing image dimensions such as width and height
		using CameraModesType = std::vector<std::string>;	//!< List type containing strings of modes the camera can operate in
		using TimeType = std::chrono::milliseconds;			//!< Time type describing the camera's times like its exposure time

		/**
		 * @brief Type indicating whether the camera is currently capturing images.
		*/
		enum class CapturingStateType {
			Stopped,					//!< The camera is not capturing.
			CapturingSingle,			//!< The camera is caturing a single image and will stop afterwards.
			CapturingContinuously		//!< The camera is capturing one image after the other.
		};

		/**
		 * @brief Type indicating whether histograms should be computed for newly captured images.
		*/
		enum class ComputeHistogramType {
			NoHistogram,				//!< Histogram computation is disabled.
			IntensityHistogram,			//!< Compute an intensity histogram by converting the image to grayscale.
			RGBHistogram,				//!< Compute separate histograms for each of the image's color channels (RGB).
			IntensityAndRGBHistogram	//!< Combination of @p IntensityHistogram and @p RGBHistogram
		};

		CameraData() = default;
		virtual ~CameraData() = default;

		auto GetImageWidth() const noexcept { return ImageWidth; }												//!< Getter for #ImageWidth
		void SetImageWidth(ImageDimensionType ImageWidth) noexcept { this->ImageWidth = ImageWidth; }			//!< Setter for #ImageWidth
		auto GetImageHeight() const noexcept { return ImageHeight; }											//!< Getter for #ImageHeight
		void SetImageHeight(ImageDimensionType ImageHeight) noexcept { this->ImageHeight = ImageHeight; }		//!< Setter for #ImageHeight

		auto GetCameraModes() const { return CameraModes; }														//!< Getter for #CameraModes
		void SetCameraModes(CameraModesType CameraModes) { this->CameraModes = CameraModes; }					//!< Setter for #CameraModes

		TimeType GetMinExposureTime() const { return MinExposureTime; }											//!< Getter for #MinExposureTime
		TimeType GetMaxExposureTime() const { return MaxExposureTime; }											//!< Getter for #MaxExposureTime
		TimeType GetExposureTime() const { return CurrentExposureTime; }										//!< Getter for #CurrentExposureTime
		void SetMinExposureTime(TimeType MinExposureTime) { this->MinExposureTime = MinExposureTime; }			//!< Setter for #MinExposureTime
		void SetMaxExposureTime(TimeType MaxExposureTime) { this->MaxExposureTime = MaxExposureTime; }			//!< Setter for #MaxExposureTime
		void SetExposureTime(TimeType CurrentExposureTime) { this->CurrentExposureTime = CurrentExposureTime; }	//!< Setter for #CurrentExposureTime

		auto GetCurrentFPS() const noexcept { return CurrentFPS; }												//!< Getter for #CurrentFPS
		void SetCurrentFPS(float CurrentFPS) noexcept { this->CurrentFPS = CurrentFPS; }						//!< Setter for #CurrentFPS

		auto GetComputeHistogram() const noexcept { return ComputeHistogram; }																				//!< Getter for #ComputeHistogram
		void SetComputeHistogram(ComputeHistogramType ComputeHistogram) const noexcept { this->ComputeHistogram = ComputeHistogram; }						//!< Setter for #ComputeHistogram. Adjustable by modules.
		auto GetIntensityHistogram() const noexcept { return IntensityHistogram; }																			//!< Getter for #IntensityHistogram
		void SetIntensityHistogram(Util::ImageHistogramType&& IntensityHistogram) noexcept { this->IntensityHistogram = std::move(IntensityHistogram); }	//!< Setter for #IntensityHistogram. Moves from argument.
		auto GetRGBHistogram() const noexcept { return RGBHistogram; }																						//!< Getter for #RGBHistogram
		void SetRGBHistogram(Util::ImageRGBHistogramType&& RGBHistogram) noexcept { this->RGBHistogram = std::move(RGBHistogram); }							//!< Setter for #RGBHistogram. Moves from argument.

		const auto& GetImageTransformation() const noexcept { return ImageTransformation; }														//!< Getter for #ImageTransformation
		void SetImageTransformation(const ImageTransformationType& Transformation) const noexcept { ImageTransformation = Transformation; }		//!< Setter for #ImageTransformation. Adjustable by modules.

		/**
		 * @brief Moving getter for #CurrentImage.
		 * @return Returns an image move-constructed from #CurrentImage. Subsequent calls to
		 * @p GetImage() or @p GetImageCopy() will fail until a new image has been captured.
		 * @throws Util::EmptyException is thrown if #CurrentImage is empty.
		*/
		QImage GetImage() const;

		/**
		 * @brief Copying getter for #CurrentImage. This function is more expensive than
		 * @p GetImage().
		 * @param RegionOfInterest Region of interest to copy and return.
		 * Refer to Qt documentation of @p QImage::copy().
		 * @return Returns an image copy-constructed from #CurrentImage.
		 * @throws Util::EmptyException is thrown if #CurrentImage is empty.
		*/
		QImage GetImageCopy(const QRect& RegionOfInterest = QRect()) const;

		/**
		 * @brief Determines whether an image is currently available.
		 * @return Returns true if #CurrentImage is not empty (not null), false otherwise.
		 */
		bool IsImageAvailbale() const noexcept { return !CurrentImage.isNull(); }

		/**
		 * @brief Setter for #CurrentImage.
		 * @param Other Image to set #CurrentImage to by moving from @p Other.
		*/
		void SetImage(QImage&& Other);

		/**
		 * @brief Resets #CurrentImage to a default-constructed empty image and
		 * #IntensityHistogram as well as #RGBHistogram to default-constructed empty histograms.
		*/
		void ClearImage() const;

		/**
		 * @brief Returns the camera's current capturing state.
		 * @return Capturing state of type CameraData::CapturingStateType
		*/
		auto GetCapturingState() const noexcept { return GetCapturingStateChild(); }

		/**
		 * @brief Determines whether the camera is currently capturing an image.
		 * @return Returns true if @p GetCapturingState() returns anything else than
		 * CapturingStateType::Stopped, false otherwise.
		*/
		bool IsCapturing() const noexcept { return GetCapturingStateChild() != CapturingStateType::Stopped; }

		/**
		 * @brief Determines whether the camera is currently capturing a single image.
		 * @return Returns true if @p GetCapturingState() returns
		 * CapturingStateType::CapturingSingle, false otherwise.
		*/
		bool IsCapturingSingle() const noexcept { return GetCapturingStateChild() == CapturingStateType::CapturingSingle; }

		/**
		 * @brief Determines whether the camera is currently capturing images consecutively.
		 * @return Returns true if @p GetCapturingState() returns
		 * CapturingStateType::CapturingContinuously, false otherwise.
		*/
		bool IsCapturingContinuously() const noexcept { return GetCapturingStateChild() == CapturingStateType::CapturingContinuously; }

	private:
		void ResetImpl(dispatch_tag<InstrumentDataBase>) override final;
		virtual void ResetImpl(dispatch_tag<CameraData>) {};						//!< @copydoc ResetImpl(dispatch_tag<DynExp::InstrumentDataBase>)

		/** @name Override
		 * Override by derived classes.
		*/
		///@{
		virtual CapturingStateType GetCapturingStateChild() const noexcept = 0;		//!< @copydoc GetCapturingState
		///@}

		ImageDimensionType ImageWidth = 0;		//!< Width of the images the camera captures
		ImageDimensionType ImageHeight = 0;		//!< Height of the images the camera captures

		CameraModesType CameraModes;			//!< Image capturing modes the camera can work in

		TimeType MinExposureTime;				//!< Minimal exposure time the camera supports
		TimeType MaxExposureTime;				//!< Maximal exposure time the camera supports
		TimeType CurrentExposureTime;			//!< Current exposure time of the camera
		float CurrentFPS = 0.f;					//!< Current frames per second when the camera is capturing images continuously

		/**
		 * @brief Determines the histogram types to be computed for each captured image.
		 * Logical const-ness: allow modules to communicate to this instrument by
		 * adjusting #ComputeHistogram.
		*/
		mutable ComputeHistogramType ComputeHistogram = ComputeHistogramType::NoHistogram;

		/**
		 * @brief Intensity (grayscale) histogram belonging to #CurrentImage.
		 * The histogram is computed if #ComputeHistogram is set accordingly.
		 * Logical const-ness: allow const member function @p ClearImage() to clear the histogram.
		*/
		mutable Util::ImageHistogramType IntensityHistogram = {};

		/**
		 * @brief Color (RGB) histograms belonging to #CurrentImage.
		 * The histograms are computed if #ComputeHistogram is set accordingly.
		 * Logical const-ness: allow const member function @p ClearImage() to clear the histograms.
		*/
		mutable Util::ImageRGBHistogramType RGBHistogram = {};

		/**
		 * @brief Image transformation to be applied to each captured image.
		 * Logical const-ness: allow modules to communicate to this instrument by
		 * adjusting #ImageTransformation.
		*/
		mutable ImageTransformationType ImageTransformation;

		/**
		 * @brief Current image captured by the camera.
		 * Logical const-ness: allow const member function @p GetImage() to move from
		 * #CurrentImage and @p ClearImage() to clear it.
		*/
		mutable QImage CurrentImage;
	};

	/**
	 * @brief Parameter class for @p Camera.
	*/
	class CameraParams : public DynExp::InstrumentParamsBase
	{
	public:
		/**
		 * @brief Constructs the parameters for a @p Camera instance.
		 * @copydetails DynExp::ParamsBase::ParamsBase
		*/
		CameraParams(DynExp::ItemIDType ID, const DynExp::DynExpCore& Core) : InstrumentParamsBase(ID, Core) {}

		virtual ~CameraParams() = 0;

		virtual const char* GetParamClassTag() const noexcept override { return "CameraParams"; }

	private:
		void ConfigureParamsImpl(dispatch_tag<InstrumentParamsBase>) override final { ConfigureParamsImpl(dispatch_tag<CameraParams>()); }
		virtual void ConfigureParamsImpl(dispatch_tag<CameraParams>) {}		//!< @copydoc ConfigureParamsImpl(dispatch_tag<DynExp::InstrumentParamsBase>)

		DummyParam Dummy = { *this };										//!< @copydoc DynExp::ParamsBase::DummyParam
	};

	/**
	 * @brief Configurator class for @p Camera
	*/
	class CameraConfigurator : public DynExp::InstrumentConfiguratorBase
	{
	public:
		using ObjectType = Camera;
		using ParamsType = CameraParams;

		CameraConfigurator() = default;
		virtual ~CameraConfigurator() = 0;
	};

	/**
	 * @brief Meta instrument for an image capturing (camera) device.
	*/
	class Camera : public DynExp::InstrumentBase
	{
	public:
		using ParamsType = CameraParams;										//!< @copydoc DynExp::Object::ParamsType
		using ConfigType = CameraConfigurator;									//!< @copydoc DynExp::Object::ConfigType
		using InstrumentDataType = CameraData;									//!< @copydoc DynExp::InstrumentBase::InstrumentDataType

		/** @name gRPC aliases
		 * Redefined to use this instrument with DynExpInstr::gRPCInstrument.
		*/
		///@{
		using InitTaskType = CameraTasks::InitTask;								//!< @copydoc DynExp::InitTaskBase
		using ExitTaskType = CameraTasks::ExitTask;								//!< @copydoc DynExp::ExitTaskBase
		using UpdateTaskType = CameraTasks::UpdateTask;							//!< @copydoc DynExp::UpdateTaskBase
		///@}

		constexpr static auto Name() noexcept { return "Camera"; }				//!< @copydoc DynExp::SerialCommunicationHardwareAdapter::Name
		constexpr static auto Category() noexcept { return "Image Capturing"; }	//!< @copydoc DynExp::InstrumentBase::Category

		/**
		 * @copydoc DynExp::InstrumentBase::InstrumentBase
		*/
		Camera(const std::thread::id OwnerThreadID, DynExp::ParamsBasePtrType&& Params)
			: InstrumentBase(OwnerThreadID, std::move(Params)) {}

		virtual ~Camera() = 0;

		virtual std::string GetName() const override { return Name(); }
		virtual std::string GetCategory() const override { return Category(); }

		virtual std::chrono::milliseconds GetTaskQueueDelay() const override { return std::chrono::milliseconds(16); /* approx. 63 fps */ }

		/** @name Override (instrument information)
		 * Override by derived classes to provide information about the instrument.
		*/
		///@{
		/**
		 * @brief Determines whether the derived camera's exposure time can be set by software.
		 * @return Return true if the camera's exposure time can be adjusted, false otherwise.
		*/
		virtual bool CanSetExposureTime() const noexcept { return false; }

		/**
		 * @brief Determines the camera's physical pixel size assuming square pixels.
		 * @return Return the camera pixel size in micrometers.
		*/
		virtual double GetPixelSizeInMicrons() const noexcept = 0;
		///@}

		/** @name Override (instrument tasks)
		 * Override by derived classes to insert tasks into the instrument's task queue.
		 * Logical const-ness: const member functions to allow modules inserting tasks into
		 * the instrument's task queue.
		*/
		///@{
		/**
		 * @brief Sets the image capturing modes the camera should work in.
		 * @param ID Index of the capturing mode as listed in CameraData::CameraModes.
		 * @param CallbackFunc @copybrief DynExp::TaskBase::CallbackFunc
		 * @throws Util::NotImplementedException is thrown by the default implementation, which
		 * does not do anything more.
		*/
		virtual void SetCameraMode(size_t ID, DynExp::TaskBase::CallbackType CallbackFunc = nullptr) const;

		/**
		 * @brief Sets the camera's exposure time.
		 * @param ExposureTime Exposure time to be used for image capturing
		 * @param CallbackFunc @copybrief DynExp::TaskBase::CallbackFunc
		 * @throws Util::NotImplementedException is thrown by the default implementation, which
		 * does not do anything more.
		*/
		virtual void SetExposureTime(const CameraData::TimeType ExposureTime, DynExp::TaskBase::CallbackType CallbackFunc = nullptr) const;

		/**
		 * @brief Synchronized version of @p SetExposureTime(), which blocks until a
		 * set exposure time task issued by an overridden @p SetExposureTime() has finished.
		 * @param ExposureTime Exposure time to be used for image capturing
		*/
		virtual void SetExposureTimeSync(const CameraData::TimeType ExposureTime) const;

		/**
		 * @brief Makes the camera capture a single image.
		 * @param CallbackFunc @copybrief DynExp::TaskBase::CallbackFunc
		*/
		virtual void CaptureSingle(DynExp::TaskBase::CallbackType CallbackFunc = nullptr) const = 0;

		/**
		 * @brief Makes the camera capture images continuously.
		 * @param CallbackFunc @copybrief DynExp::TaskBase::CallbackFunc
		*/
		virtual void StartCapturing(DynExp::TaskBase::CallbackType CallbackFunc = nullptr) const = 0;

		/**
		 * @brief Stops image capturing.
		 * @param CallbackFunc @copybrief DynExp::TaskBase::CallbackFunc
		*/
		virtual void StopCapturing(DynExp::TaskBase::CallbackType CallbackFunc = nullptr) const = 0;

		/**
		 * @brief Synchronized version of @p StopCapturing(), which blocks until a
		 * stop capturing task issued by an overridden @p StopCapturing() has finished.
		*/
		virtual void StopCapturingSync() const;
		///@}

	private:
		void ResetImpl(dispatch_tag<InstrumentBase>) override final;
		virtual void ResetImpl(dispatch_tag<Camera>) = 0;				//!< @copydoc ResetImpl(dispatch_tag<DynExp::InstrumentBase>)
	};

	/**
	 * @brief Applies an image transformation to a single pixel.
	 * @tparam T Type of the pixel to transform
	 * @param Pixel Pixel to transform
	 * @param ImageTransformation Image transformation to apply. Refer to CameraData::ImageTransformationType.
	 * The CameraData::ImageTransformationType::IsEnabled property is ignored here and assumed to be true.
	 * @return Returns the transformed pixel.
	*/
	template <typename T>
	inline T TransformPixel(T Pixel, const CameraData::ImageTransformationType& ImageTransformation)
	{
		// Apply user transformations
		float PixelF = ImageTransformation.ContrastFactor * Pixel + ImageTransformation.BrightnessFactor * std::numeric_limits<T>::max();

		// Round and apply limits.
		PixelF = std::min(static_cast<float>(std::numeric_limits<T>::max()), std::round(PixelF));
		PixelF = std::max(static_cast<float>(std::numeric_limits<T>::lowest()), PixelF);

		return static_cast<T>(PixelF);
	}
}