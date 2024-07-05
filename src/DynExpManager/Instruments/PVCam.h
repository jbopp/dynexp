// This file is part of DynExp.

/**
 * @file PVCam.h
 * @brief Implementation of an instrument to control Teledyne Photometrics PVCam
 * cameras.
*/

#pragma once

#include "stdafx.h"
#include "DynExpCore.h"
#include "HardwareAdapters/HardwareAdapterPVCam.h"
#include "MetaInstruments/Camera.h"

namespace DynExpInstr
{
	class PVCam;

	namespace PVCamTasks
	{
		class InitTask : public CameraTasks::InitTask
		{
			void InitFuncImpl(dispatch_tag<CameraTasks::InitTask>, DynExp::InstrumentInstance& Instance) override final;

			virtual void InitFuncImpl(dispatch_tag<InitTask>, DynExp::InstrumentInstance& Instance) {}
		};

		class ExitTask : public CameraTasks::ExitTask
		{
			void ExitFuncImpl(dispatch_tag<CameraTasks::ExitTask>, DynExp::InstrumentInstance& Instance) override final;

			virtual void ExitFuncImpl(dispatch_tag<ExitTask>, DynExp::InstrumentInstance& Instance) {}
		};

		class UpdateTask : public CameraTasks::UpdateTask
		{
			void UpdateFuncImpl(dispatch_tag<CameraTasks::UpdateTask>, DynExp::InstrumentInstance& Instance) override final { UpdateFuncImpl(dispatch_tag<UpdateTask>(), Instance); }

			virtual void UpdateFuncImpl(dispatch_tag<UpdateTask>, DynExp::InstrumentInstance& Instance) {}

			virtual QImage ObtainImage(DynExp::InstrumentInstance& Instance) override;
		};

		class SetCameraMode final : public DynExp::TaskBase
		{
		public:
			SetCameraMode(const size_t ID, CallbackType CallbackFunc) noexcept : TaskBase(CallbackFunc), ID(ID) {}

		private:
			virtual DynExp::TaskResultType RunChild(DynExp::InstrumentInstance& Instance) override;

			const size_t ID;
		};

		class SetExposureTimeTask final : public DynExp::TaskBase
		{
		public:
			SetExposureTimeTask(const CameraData::TimeType ExposureTime, CallbackType CallbackFunc) noexcept
				: TaskBase(CallbackFunc), ExposureTime(ExposureTime) {}

		private:
			virtual DynExp::TaskResultType RunChild(DynExp::InstrumentInstance& Instance) override;

			const CameraData::TimeType ExposureTime;
		};

		class CaptureSingleTask final : public DynExp::TaskBase
		{
		public:
			CaptureSingleTask(CallbackType CallbackFunc) noexcept : TaskBase(CallbackFunc) {}

		private:
			virtual DynExp::TaskResultType RunChild(DynExp::InstrumentInstance& Instance) override;
		};

		class StartCapturingTask final : public DynExp::TaskBase
		{
		public:
			StartCapturingTask(CallbackType CallbackFunc) noexcept : TaskBase(CallbackFunc) {}

		private:
			virtual DynExp::TaskResultType RunChild(DynExp::InstrumentInstance& Instance) override;
		};

		class StopCapturingTask final : public DynExp::TaskBase
		{
		public:
			StopCapturingTask(CallbackType CallbackFunc) noexcept : TaskBase(CallbackFunc) {}

		private:
			virtual DynExp::TaskResultType RunChild(DynExp::InstrumentInstance& Instance) override;
		};
	}

	class PVCamData : public CameraData
	{
		friend class PVCamTasks::InitTask;
		friend class PVCamTasks::UpdateTask;

	public:
		PVCamData() = default;
		virtual ~PVCamData() = default;

		void SetCapturingState(CapturingStateType CapturingState) noexcept { this->CapturingState = CapturingState; }

		DynExp::LinkedObjectWrapperContainer<DynExpHardware::PVCamHardwareAdapter> HardwareAdapter;

	private:
		void ResetImpl(dispatch_tag<CameraData>) override final;
		virtual void ResetImpl(dispatch_tag<PVCamData>) {};

		virtual CapturingStateType GetCapturingStateChild() const noexcept override { return CapturingState; }

		CapturingStateType CapturingState = CapturingStateType::Stopped;
		size_t NumFailedStatusUpdateAttempts = 0;
	};

	class PVCamParams : public CameraParams
	{
	public:
		PVCamParams(DynExp::ItemIDType ID, const DynExp::DynExpCore& Core) : CameraParams(ID, Core) {}
		virtual ~PVCamParams() = default;

		virtual const char* GetParamClassTag() const noexcept override { return "PVCamParams"; }

		Param<DynExp::ObjectLink<DynExpHardware::PVCamHardwareAdapter>> HardwareAdapter = { *this, GetCore().GetHardwareAdapterManager(),
			"HardwareAdapter", "PVCam camera", "Underlying hardware adapter of this instrument", DynExpUI::Icons::HardwareAdapter };

	private:
		void ConfigureParamsImpl(dispatch_tag<CameraParams>) override final { ConfigureParamsImpl(dispatch_tag<PVCamParams>()); }
		virtual void ConfigureParamsImpl(dispatch_tag<PVCamParams>) {}
	};

	class PVCamConfigurator : public CameraConfigurator
	{
	public:
		using ObjectType = PVCam;
		using ParamsType = PVCamParams;

		PVCamConfigurator() = default;
		virtual ~PVCamConfigurator() = default;

	private:
		virtual DynExp::ParamsBasePtrType MakeParams(DynExp::ItemIDType ID, const DynExp::DynExpCore& Core) const override { return DynExp::MakeParams<PVCamConfigurator>(ID, Core); }
	};

	class PVCam : public Camera
	{
	public:
		using ParamsType = PVCamParams;
		using ConfigType = PVCamConfigurator;
		using InstrumentDataType = PVCamData;

		constexpr static auto Name() noexcept { return "PVCam"; }

		PVCam(const std::thread::id OwnerThreadID, DynExp::ParamsBasePtrType&& Params);
		virtual ~PVCam() {}

		virtual std::string GetName() const override { return Name(); }

		virtual bool CanSetExposureTime() const noexcept override { return true; }
		virtual double GetPixelSizeInMicrons() const noexcept override { return 6.5; }

		virtual void SetCameraMode(size_t ID, DynExp::TaskBase::CallbackType CallbackFunc = nullptr) const override { MakeAndEnqueueTask<PVCamTasks::SetCameraMode>(ID, CallbackFunc); }
		virtual void SetExposureTime(const CameraData::TimeType ExposureTime, DynExp::TaskBase::CallbackType CallbackFunc = nullptr) const override { MakeAndEnqueueTask<PVCamTasks::SetExposureTimeTask>(ExposureTime, CallbackFunc); }
		virtual void CaptureSingle(DynExp::TaskBase::CallbackType CallbackFunc = nullptr) const override { MakeAndEnqueueTask<PVCamTasks::CaptureSingleTask>(CallbackFunc); }
		virtual void StartCapturing(DynExp::TaskBase::CallbackType CallbackFunc = nullptr) const override { MakeAndEnqueueTask<PVCamTasks::StartCapturingTask>(CallbackFunc); }
		virtual void StopCapturing(DynExp::TaskBase::CallbackType CallbackFunc = nullptr) const override { MakeAndEnqueueTask<PVCamTasks::StopCapturingTask>(CallbackFunc); }

	private:
		void ResetImpl(dispatch_tag<Camera>) override final;
		virtual void ResetImpl(dispatch_tag<PVCam>) {}

		virtual std::unique_ptr<DynExp::InitTaskBase> MakeInitTask() const override { return DynExp::MakeTask<PVCamTasks::InitTask>(); }
		virtual std::unique_ptr<DynExp::ExitTaskBase> MakeExitTask() const override { return DynExp::MakeTask<PVCamTasks::ExitTask>(); }
		virtual std::unique_ptr<DynExp::UpdateTaskBase> MakeUpdateTask() const override { return DynExp::MakeTask<PVCamTasks::UpdateTask>(); }
	};
}