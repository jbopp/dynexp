// This file is part of DynExp.

/**
 * @file WidefieldLocalization.h
 * @brief Implementation of a gRPC client instrument to access a remote service for image processing
 * (localization of photon emitters in widefield microscope images). Used by the
 * DynExpModule::Widefield::WidefieldMicroscope module.
*/

#pragma once

#include "stdafx.h"
#include "DynExpCore.h"
#include "MetaInstruments/gRPCInstrument.h"

#include "WidefieldLocalization.pb.h"
#include "WidefieldLocalization.grpc.pb.h"

namespace DynExpInstr
{
	class WidefieldLocalization;

	struct WidefieldLocalizationCellIDType
	{
		std::string IDString;
		uint32_t X_id = 0;
		uint32_t Y_id = 0;
		bool Valid = false;

		// Optional, stays zero if not returned by ReadCellID rpc.
		int32_t CellShift_px_x = 0;
		int32_t CellShift_px_y = 0;

		constexpr bool HasCellShift() const noexcept { return CellShift_px_x || CellShift_px_y; }
	};

	std::strong_ordering operator<=>(const WidefieldLocalizationCellIDType& lhs, const WidefieldLocalizationCellIDType& rhs);
	std::ostream& operator<<(std::ostream& stream, const WidefieldLocalizationCellIDType& CellID);

	namespace WidefieldLocalizationTasks
	{
		class InitTask : public gRPCInstrumentTasks::InitTask<DynExp::InstrumentBase, 0, DynExpProto::WidefieldLocalization::WidefieldLocalization>
		{
			void InitFuncImpl(dispatch_tag<gRPCInstrumentTasks::InitTask<DynExp::InstrumentBase, 0, DynExpProto::WidefieldLocalization::WidefieldLocalization>>, DynExp::InstrumentInstance& Instance) override final;
			virtual void InitFuncImpl(dispatch_tag<InitTask>, DynExp::InstrumentInstance& Instance) {}
		};

		class ExitTask : public gRPCInstrumentTasks::ExitTask<DynExp::InstrumentBase, 0, DynExpProto::WidefieldLocalization::WidefieldLocalization>
		{
			void ExitFuncImpl(dispatch_tag<gRPCInstrumentTasks::ExitTask<DynExp::InstrumentBase, 0, DynExpProto::WidefieldLocalization::WidefieldLocalization>>, DynExp::InstrumentInstance& Instance) override final;
			virtual void ExitFuncImpl(dispatch_tag<ExitTask>, DynExp::InstrumentInstance& Instance) {}
		};

		class UpdateTask : public gRPCInstrumentTasks::UpdateTask<DynExp::InstrumentBase, 0, DynExpProto::WidefieldLocalization::WidefieldLocalization>
		{
			void UpdateFuncImpl(dispatch_tag<gRPCInstrumentTasks::UpdateTask<DynExp::InstrumentBase, 0, DynExpProto::WidefieldLocalization::WidefieldLocalization>>, DynExp::InstrumentInstance& Instance) override final;
			virtual void UpdateFuncImpl(dispatch_tag<UpdateTask>, DynExp::InstrumentInstance& Instance) {}
		};

		class ImageProcessingTaskBase
		{
		public:
			ImageProcessingTaskBase(const QImage& Image) noexcept;

			// Consumes ImageData.
			DynExpProto::WidefieldLocalization::ImageMessage MakeImageMessage();

		private:
			const google::protobuf::uint32 ImageWidth;
			const google::protobuf::uint32 ImageHeight;
			const QImage::Format ImageFormat;
			std::string ImageData;
		};

		class ReadCellIDTask final : public DynExp::TaskBase, ImageProcessingTaskBase
		{
		public:
			ReadCellIDTask(const QImage& Image, CallbackType CallbackFunc) noexcept;

		private:
			virtual DynExp::TaskResultType RunChild(DynExp::InstrumentInstance& Instance) override;
		};

		class AnalyzeWidefieldTask final : public DynExp::TaskBase, ImageProcessingTaskBase
		{
		public:
			AnalyzeWidefieldTask(const QImage& Image, CallbackType CallbackFunc) noexcept;

		private:
			virtual DynExp::TaskResultType RunChild(DynExp::InstrumentInstance& Instance) override;
		};

		class AnalyzeDistortionTask final : public DynExp::TaskBase, ImageProcessingTaskBase
		{
		public:
			AnalyzeDistortionTask(const QImage& Image, CallbackType CallbackFunc) noexcept;

		private:
			virtual DynExp::TaskResultType RunChild(DynExp::InstrumentInstance& Instance) override;
		};

		class RecallPositionsTask final : public DynExp::TaskBase, ImageProcessingTaskBase
		{
		public:
			RecallPositionsTask(const QImage& Image, const WidefieldLocalizationCellIDType& CellID,
				std::string_view MeasureSavePath, CallbackType CallbackFunc) noexcept;

		private:
			virtual DynExp::TaskResultType RunChild(DynExp::InstrumentInstance& Instance) override;

			const WidefieldLocalizationCellIDType CellID;
			const std::string MeasureSavePath;
		};
	}

	class WidefieldLocalizationData : public gRPCInstrumentData<DynExp::InstrumentBase, 0, DynExpProto::WidefieldLocalization::WidefieldLocalization>
	{
		friend class WidefieldLocalizationTasks::ReadCellIDTask;
		friend class WidefieldLocalizationTasks::AnalyzeWidefieldTask;
		friend class WidefieldLocalizationTasks::RecallPositionsTask;

	public:
		WidefieldLocalizationData() = default;
		virtual ~WidefieldLocalizationData() = default;

		const auto& GetCellID() const noexcept { return CellID; }
		const auto& GetLocalizedPositions() const noexcept { return LocalizedPositions; }

	private:
		void ResetImpl(dispatch_tag<gRPCInstrumentData>) override final;
		virtual void ResetImpl(dispatch_tag<WidefieldLocalizationData>) {};

		void SetLocalizedPositions(const DynExpProto::WidefieldLocalization::PositionsMessage& PositionsMsg);

		WidefieldLocalizationCellIDType CellID;
		std::map<google::protobuf::uint32, QPoint> LocalizedPositions;
	};

	class WidefieldLocalizationParams : public gRPCInstrumentParams<DynExp::InstrumentBase, 0, DynExpProto::WidefieldLocalization::WidefieldLocalization>
	{
	public:
		WidefieldLocalizationParams(DynExp::ItemIDType ID, const DynExp::DynExpCore& Core) : gRPCInstrumentParams(ID, Core) {}
		virtual ~WidefieldLocalizationParams() = default;

		virtual const char* GetParamClassTag() const noexcept override { return "WidefieldLocalizationParams"; }

	private:
		void ConfigureParamsImpl(dispatch_tag<gRPCInstrumentParams<DynExp::InstrumentBase, 0, DynExpProto::WidefieldLocalization::WidefieldLocalization>>) override final { ConfigureParamsImpl(dispatch_tag<WidefieldLocalizationParams>()); }
		virtual void ConfigureParamsImpl(dispatch_tag<WidefieldLocalizationParams>) {}

		DummyParam Dummy = { *this };
	};

	class WidefieldLocalizationConfigurator : public gRPCInstrumentConfigurator<DynExp::InstrumentBase, 0, DynExpProto::WidefieldLocalization::WidefieldLocalization>
	{
	public:
		using ObjectType = WidefieldLocalization;
		using ParamsType = WidefieldLocalizationParams;

		WidefieldLocalizationConfigurator() = default;
		virtual ~WidefieldLocalizationConfigurator() = default;

	private:
		virtual DynExp::ParamsBasePtrType MakeParams(DynExp::ItemIDType ID, const DynExp::DynExpCore& Core) const override { return DynExp::MakeParams<WidefieldLocalizationConfigurator>(ID, Core); }
	};

	class WidefieldLocalization : public gRPCInstrument<DynExp::InstrumentBase, 0, DynExpProto::WidefieldLocalization::WidefieldLocalization>
	{
	public:
		using ParamsType = WidefieldLocalizationParams;
		using ConfigType = WidefieldLocalizationConfigurator;
		using InstrumentDataType = WidefieldLocalizationData;

		constexpr static auto Name() noexcept { return "Widefield Localization"; }

		WidefieldLocalization(const std::thread::id OwnerThreadID, DynExp::ParamsBasePtrType&& Params);
		virtual ~WidefieldLocalization() {}

		virtual std::string GetName() const override { return Name(); }

		virtual void ReadCellID(const QImage& Image, DynExp::TaskBase::CallbackType CallbackFunc = nullptr) const { MakeAndEnqueueTask<WidefieldLocalizationTasks::ReadCellIDTask>(Image, CallbackFunc); }
		virtual void AnalyzeWidefield(const QImage& Image, DynExp::TaskBase::CallbackType CallbackFunc = nullptr) const { MakeAndEnqueueTask<WidefieldLocalizationTasks::AnalyzeWidefieldTask>(Image, CallbackFunc); }
		virtual void AnalyzeDistortion(const QImage& Image, DynExp::TaskBase::CallbackType CallbackFunc = nullptr) const { MakeAndEnqueueTask<WidefieldLocalizationTasks::AnalyzeDistortionTask>(Image, CallbackFunc); }
		virtual void RecallPositions(const QImage& Image, const WidefieldLocalizationCellIDType& CellID, std::string_view MeasureSavePath, DynExp::TaskBase::CallbackType CallbackFunc = nullptr) const { MakeAndEnqueueTask<WidefieldLocalizationTasks::RecallPositionsTask>(Image, CellID, MeasureSavePath, CallbackFunc); }

	private:
		void ResetImpl(dispatch_tag<gRPCInstrument>) override final;
		virtual void ResetImpl(dispatch_tag<WidefieldLocalization>) {}

		virtual std::unique_ptr<DynExp::InitTaskBase> MakeInitTask() const override { return DynExp::MakeTask<WidefieldLocalizationTasks::InitTask>(); }
		virtual std::unique_ptr<DynExp::ExitTaskBase> MakeExitTask() const override { return DynExp::MakeTask<WidefieldLocalizationTasks::ExitTask>(); }
		virtual std::unique_ptr<DynExp::UpdateTaskBase> MakeUpdateTask() const override { return DynExp::MakeTask<WidefieldLocalizationTasks::UpdateTask>(); }
	};
}