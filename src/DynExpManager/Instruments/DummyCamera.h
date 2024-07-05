// This file is part of DynExp.

/**
 * @file DummyCamera.h
 * @brief Implementation of a camera instrument without any related physical hardware loading image
 * files from disk.
*/

#pragma once

#include "stdafx.h"
#include "DynExpCore.h"
#include "MetaInstruments/Camera.h"

namespace DynExpInstr
{
	class DummyCamera;

	namespace DummyCameraTasks
	{
		class InitTask : public CameraTasks::InitTask
		{
			void InitFuncImpl(dispatch_tag<CameraTasks::InitTask>, DynExp::InstrumentInstance& Instance) override final;

			virtual void InitFuncImpl(dispatch_tag<InitTask>, DynExp::InstrumentInstance& Instance) {}
		};

		class ExitTask : public CameraTasks::ExitTask
		{
			void ExitFuncImpl(dispatch_tag<CameraTasks::ExitTask>, DynExp::InstrumentInstance& Instance) override final { ExitFuncImpl(dispatch_tag<ExitTask>(), Instance); }

			virtual void ExitFuncImpl(dispatch_tag<ExitTask>, DynExp::InstrumentInstance& Instance) {}
		};

		class UpdateTask : public CameraTasks::UpdateTask
		{
			void UpdateFuncImpl(dispatch_tag<CameraTasks::UpdateTask>, DynExp::InstrumentInstance& Instance) override final { UpdateFuncImpl(dispatch_tag<UpdateTask>(), Instance); }

			virtual void UpdateFuncImpl(dispatch_tag<UpdateTask>, DynExp::InstrumentInstance& Instance) {}

			virtual QImage ObtainImage(DynExp::InstrumentInstance& Instance) override;
		};
	}

	class DummyCameraData : public CameraData
	{
		friend class DummyCameraTasks::InitTask;
		friend class DummyCameraTasks::UpdateTask;

	public:
		DummyCameraData() = default;
		virtual ~DummyCameraData() = default;

		QImage GetDummyImage() { return DummyImage; }

	private:
		void ResetImpl(dispatch_tag<CameraData>) override final;
		virtual void ResetImpl(dispatch_tag<DummyCameraData>) {};

		virtual CapturingStateType GetCapturingStateChild() const noexcept override { return CapturingStateType::CapturingContinuously; }

		QImage DummyImage;
	};

	class DummyCameraParams : public CameraParams
	{
	public:
		DummyCameraParams(DynExp::ItemIDType ID, const DynExp::DynExpCore& Core) : CameraParams(ID, Core) {}
		virtual ~DummyCameraParams() = default;

		virtual const char* GetParamClassTag() const noexcept override { return "DummyCameraParams"; }

		Param<ParamsConfigDialog::TextType> ImagePath = { *this, "ImagePath", "Image path",
			"Path to an image file to use as a dummy camera image", true, "", DynExp::TextUsageType::Path };

	private:
		void ConfigureParamsImpl(dispatch_tag<CameraParams>) override final { ConfigureParamsImpl(dispatch_tag<DummyCameraParams>()); }
		virtual void ConfigureParamsImpl(dispatch_tag<DummyCameraParams>) {}
	};

	class DummyCameraConfigurator : public CameraConfigurator
	{
	public:
		using ObjectType = DummyCamera;
		using ParamsType = DummyCameraParams;

		DummyCameraConfigurator() = default;
		virtual ~DummyCameraConfigurator() = default;

	private:
		virtual DynExp::ParamsBasePtrType MakeParams(DynExp::ItemIDType ID, const DynExp::DynExpCore& Core) const override { return DynExp::MakeParams<DummyCameraConfigurator>(ID, Core); }
	};

	class DummyCamera : public Camera
	{
	public:
		using ParamsType = DummyCameraParams;
		using ConfigType = DummyCameraConfigurator;
		using InstrumentDataType = DummyCameraData;

		constexpr static auto Name() noexcept { return "Dummy Camera"; }

		DummyCamera(const std::thread::id OwnerThreadID, DynExp::ParamsBasePtrType&& Params);
		virtual ~DummyCamera() {}

		virtual std::string GetName() const override { return Name(); }

		virtual double GetPixelSizeInMicrons() const noexcept override { return 0; }

		virtual void CaptureSingle(DynExp::TaskBase::CallbackType CallbackFunc = nullptr) const noexcept override { MakeAndEnqueueTask<DynExp::DefaultTask>(CallbackFunc); }
		virtual void StartCapturing(DynExp::TaskBase::CallbackType CallbackFunc = nullptr) const noexcept override { MakeAndEnqueueTask<DynExp::DefaultTask>(CallbackFunc); }
		virtual void StopCapturing(DynExp::TaskBase::CallbackType CallbackFunc = nullptr) const noexcept override { MakeAndEnqueueTask<DynExp::DefaultTask>(CallbackFunc); }

	private:
		void ResetImpl(dispatch_tag<Camera>) override final;
		virtual void ResetImpl(dispatch_tag<DummyCamera>) {}

		virtual std::unique_ptr<DynExp::InitTaskBase> MakeInitTask() const override { return DynExp::MakeTask<DummyCameraTasks::InitTask>(); }
		virtual std::unique_ptr<DynExp::UpdateTaskBase> MakeUpdateTask() const override { return DynExp::MakeTask<DummyCameraTasks::UpdateTask>(); }
	};
}