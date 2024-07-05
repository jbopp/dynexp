// This file is part of DynExp.

#include "stdafx.h"
#include "WidefieldLocalization.h"

namespace DynExpInstr
{
	std::strong_ordering operator<=>(const WidefieldLocalizationCellIDType& lhs, const WidefieldLocalizationCellIDType& rhs)
	{
		if (lhs.X_id == rhs.X_id && lhs.Y_id == rhs.Y_id)
			return std::strong_ordering::equal;
		else if (lhs.Y_id > rhs.Y_id || (lhs.Y_id == rhs.Y_id && lhs.X_id > rhs.X_id))
			return std::strong_ordering::greater;
		else
			return std::strong_ordering::less;
	}

	std::ostream& operator<<(std::ostream& stream, const WidefieldLocalizationCellIDType& CellID)
	{
		stream << "CellIDString = " << CellID.IDString << "\n";
		stream << "CellIDX = " << CellID.X_id << "\n";
		stream << "CellIDY = " << CellID.Y_id << "\n";
		stream << "CellIDValid = " << (CellID.Valid ? "yes" : "no") << "\n";

		return stream;
	}

	void WidefieldLocalizationTasks::InitTask::InitFuncImpl(dispatch_tag<gRPCInstrumentTasks::InitTask<DynExp::InstrumentBase, 0, DynExpProto::WidefieldLocalization::WidefieldLocalization>>, DynExp::InstrumentInstance& Instance)
	{
		InitFuncImpl(dispatch_tag<InitTask>(), Instance);
	}

	void WidefieldLocalizationTasks::ExitTask::ExitFuncImpl(dispatch_tag<gRPCInstrumentTasks::ExitTask<DynExp::InstrumentBase, 0, DynExpProto::WidefieldLocalization::WidefieldLocalization>>, DynExp::InstrumentInstance& Instance)
	{
		ExitFuncImpl(dispatch_tag<ExitTask>(), Instance);
	}

	void WidefieldLocalizationTasks::UpdateTask::UpdateFuncImpl(dispatch_tag<gRPCInstrumentTasks::UpdateTask<DynExp::InstrumentBase, 0, DynExpProto::WidefieldLocalization::WidefieldLocalization>>, DynExp::InstrumentInstance& Instance)
	{
		UpdateFuncImpl(dispatch_tag<UpdateTask>(), Instance);
	}

	WidefieldLocalizationTasks::ImageProcessingTaskBase::ImageProcessingTaskBase(const QImage& Image) noexcept
		: ImageWidth(Util::NumToT<google::protobuf::uint32>(Image.width())), ImageHeight(Util::NumToT<google::protobuf::uint32>(Image.height())),
		ImageFormat(Image.format())
	{
		ImageData.resize(Image.sizeInBytes());

		// No deep copy in QImage::bits(), since Image is const (see https://doc.qt.io/qt-5/qimage.html#bits-1).
		std::memcpy(ImageData.data(), Image.bits(), ImageData.size());
	}

	DynExpProto::WidefieldLocalization::ImageMessage WidefieldLocalizationTasks::ImageProcessingTaskBase::MakeImageMessage()
	{
		DynExpProto::WidefieldLocalization::ImageMessage ImageMsg;

		ImageMsg.set_width(ImageWidth);
		ImageMsg.set_height(ImageHeight);

		switch (ImageFormat)
		{
		case QImage::Format::Format_Grayscale8: ImageMsg.set_imageformat(DynExpProto::WidefieldLocalization::ImageFormatType::Mono8); break;
		case QImage::Format::Format_Grayscale16: ImageMsg.set_imageformat(DynExpProto::WidefieldLocalization::ImageFormatType::Mono16); break;
		default: throw Util::NotImplementedException("The format of the given image is not supported.");
		}

		ImageMsg.set_image(std::move(ImageData));

		return ImageMsg;
	}

	WidefieldLocalizationTasks::ReadCellIDTask::ReadCellIDTask(const QImage& Image, CallbackType CallbackFunc) noexcept
		: TaskBase(CallbackFunc), ImageProcessingTaskBase(Image)
	{
	}

	DynExp::TaskResultType WidefieldLocalizationTasks::ReadCellIDTask::RunChild(DynExp::InstrumentInstance& Instance)
	{
		grpc::ClientContext Context;
		DynExpProto::WidefieldLocalization::ImageMessage ImageMsg = MakeImageMessage();
		DynExpProto::WidefieldLocalization::CellIDMessage CellIDMsg;

		StubPtrType<DynExpProto::WidefieldLocalization::WidefieldLocalization> StubPtr;
		{
			auto InstrData = DynExp::dynamic_InstrumentData_cast<WidefieldLocalization>(Instance.InstrumentDataGetter());
			StubPtr = InstrData->GetStub<0>();
		} // InstrData unlocked here.

		auto Result = StubPtr->ReadCellID(&Context, ImageMsg, &CellIDMsg);
		if (!Result.ok())
			throw DynExpHardware::gRPCException(Result);
		if (CellIDMsg.resultmsg().result() != DynExpProto::WidefieldLocalization::ResultType::OK &&
			CellIDMsg.resultmsg().result() != DynExpProto::WidefieldLocalization::ResultType::LocalizationFailed)
			throw Util::ServiceFailedException("Reading the cell id failed on the remote service site.");

		{
			auto InstrData = DynExp::dynamic_InstrumentData_cast<WidefieldLocalization>(Instance.InstrumentDataGetter());

			if (CellIDMsg.resultmsg().result() == DynExpProto::WidefieldLocalization::ResultType::OK)
			{
				// Reading the cell id worked.
				InstrData->CellID.IDString = CellIDMsg.idstring();
				InstrData->CellID.X_id = CellIDMsg.id().x();
				InstrData->CellID.Y_id = CellIDMsg.id().y();
				InstrData->CellID.Valid = true;
				InstrData->CellID.CellShift_px_x = CellIDMsg.has_cellshift_px() ? CellIDMsg.cellshift_px().x() : 0;
				InstrData->CellID.CellShift_px_y = CellIDMsg.has_cellshift_px() ? CellIDMsg.cellshift_px().y() : 0;
			}
			else
			{
				// Extracting the cell id did not work since it was not readable from the image.
				// However, the extraction routines themselves worked. Do not throw in case of that 'soft' error.
				InstrData->CellID = {};
			}
		} // InstrData unlocked here.

		return {};
	}

	WidefieldLocalizationTasks::AnalyzeWidefieldTask::AnalyzeWidefieldTask(const QImage& Image, CallbackType CallbackFunc) noexcept
		: TaskBase(CallbackFunc), ImageProcessingTaskBase(Image)
	{
	}

	DynExp::TaskResultType WidefieldLocalizationTasks::AnalyzeWidefieldTask::RunChild(DynExp::InstrumentInstance& Instance)
	{
		grpc::ClientContext Context;
		DynExpProto::WidefieldLocalization::ImageMessage ImageMsg = MakeImageMessage();
		DynExpProto::WidefieldLocalization::PositionsMessage PositionsMsg;

		StubPtrType<DynExpProto::WidefieldLocalization::WidefieldLocalization> StubPtr;
		{
			auto InstrData = DynExp::dynamic_InstrumentData_cast<WidefieldLocalization>(Instance.InstrumentDataGetter());
			StubPtr = InstrData->GetStub<0>();
		} // InstrData unlocked here.

		auto Result = StubPtr->AnalyzeWidefield(&Context, ImageMsg, &PositionsMsg);
		if (!Result.ok())
			throw DynExpHardware::gRPCException(Result);

		{
			auto InstrData = DynExp::dynamic_InstrumentData_cast<WidefieldLocalization>(Instance.InstrumentDataGetter());
			InstrData->SetLocalizedPositions(PositionsMsg);
		} // InstrData unlocked here.

		return {};
	}

	WidefieldLocalizationTasks::AnalyzeDistortionTask::AnalyzeDistortionTask(const QImage& Image, CallbackType CallbackFunc) noexcept
		: TaskBase(CallbackFunc), ImageProcessingTaskBase(Image)
	{
	}

	DynExp::TaskResultType WidefieldLocalizationTasks::AnalyzeDistortionTask::RunChild(DynExp::InstrumentInstance& Instance)
	{
		grpc::ClientContext Context;
		DynExpProto::WidefieldLocalization::ImageMessage ImageMsg = MakeImageMessage();
		DynExpProto::WidefieldLocalization::VoidMessage VoidMsg;

		StubPtrType<DynExpProto::WidefieldLocalization::WidefieldLocalization> StubPtr;
		{
			auto InstrData = DynExp::dynamic_InstrumentData_cast<WidefieldLocalization>(Instance.InstrumentDataGetter());
			StubPtr = InstrData->GetStub<0>();
		} // InstrData unlocked here.

		auto Result = StubPtr->AnalyzeTipTilt(&Context, ImageMsg, &VoidMsg);
		if (!Result.ok())
			throw DynExpHardware::gRPCException(Result);

		return {};
	}

	WidefieldLocalizationTasks::RecallPositionsTask::RecallPositionsTask(const QImage& Image, const WidefieldLocalizationCellIDType& CellID,
		std::string_view MeasureSavePath, CallbackType CallbackFunc) noexcept
		: TaskBase(CallbackFunc), ImageProcessingTaskBase(Image), CellID(CellID), MeasureSavePath(MeasureSavePath)
	{
	}

	DynExp::TaskResultType DynExpInstr::WidefieldLocalizationTasks::RecallPositionsTask::RunChild(DynExp::InstrumentInstance& Instance)
	{
		grpc::ClientContext Context;
		DynExpProto::WidefieldLocalization::RecallPositionsMessage RecallPositionsMsg;
		DynExpProto::WidefieldLocalization::PositionsMessage PositionsMsg;

		auto ImageMsg = std::make_unique<DynExpProto::WidefieldLocalization::ImageMessage>(MakeImageMessage());
		RecallPositionsMsg.set_allocated_image(ImageMsg.release());

		auto CellIDResultsMsg = std::make_unique<DynExpProto::WidefieldLocalization::ResultMessage>();
		CellIDResultsMsg->set_result(CellID.Valid ? DynExpProto::WidefieldLocalization::ResultType::OK : DynExpProto::WidefieldLocalization::ResultType::GeneralError);
		auto IDPointMsg = std::make_unique<DynExpProto::WidefieldLocalization::PointMessage>();
		IDPointMsg->set_x(CellID.X_id);
		IDPointMsg->set_y(CellID.Y_id);
		auto CellIDMsg = std::make_unique<DynExpProto::WidefieldLocalization::CellIDMessage>();
		CellIDMsg->set_allocated_resultmsg(CellIDResultsMsg.release());
		CellIDMsg->set_idstring(CellID.IDString);
		CellIDMsg->set_allocated_id(IDPointMsg.release());
		RecallPositionsMsg.set_allocated_cellid(CellIDMsg.release());

		RecallPositionsMsg.set_measuresavepath(MeasureSavePath);

		StubPtrType<DynExpProto::WidefieldLocalization::WidefieldLocalization> StubPtr;
		{
			auto InstrData = DynExp::dynamic_InstrumentData_cast<WidefieldLocalization>(Instance.InstrumentDataGetter());
			StubPtr = InstrData->GetStub<0>();
		} // InstrData unlocked here.

		auto Result = StubPtr->RecallPositions(&Context, RecallPositionsMsg, &PositionsMsg);
		if (!Result.ok())
			throw DynExpHardware::gRPCException(Result);

		{
			auto InstrData = DynExp::dynamic_InstrumentData_cast<WidefieldLocalization>(Instance.InstrumentDataGetter());
			InstrData->SetLocalizedPositions(PositionsMsg);
		} // InstrData unlocked here.

		return {};
	}

	void WidefieldLocalizationData::ResetImpl(dispatch_tag<gRPCInstrumentData>)
	{
		CellID = {};
		LocalizedPositions.clear();

		ResetImpl(dispatch_tag<WidefieldLocalizationData>());
	}

	void WidefieldLocalizationData::SetLocalizedPositions(const DynExpProto::WidefieldLocalization::PositionsMessage& PositionsMsg)
	{
		if (PositionsMsg.resultmsg().result() != DynExpProto::WidefieldLocalization::ResultType::OK)
			throw Util::ServiceFailedException("Widefield localization failed on the remote service site.");

		const auto NumPositions = PositionsMsg.entries_size();
		LocalizedPositions.clear();
		for (std::remove_const_t<decltype(NumPositions)> i = 0; i < NumPositions; ++i)
		{
			QPoint Position{ Util::NumToT<int>(PositionsMsg.entries(i).pos_px().x()), Util::NumToT<int>(PositionsMsg.entries(i).pos_px().y()) };
			LocalizedPositions.emplace(PositionsMsg.entries(i).id(), Position);
		}
	}

	WidefieldLocalization::WidefieldLocalization(const std::thread::id OwnerThreadID, DynExp::ParamsBasePtrType&& Params)
		: gRPCInstrument(OwnerThreadID, std::move(Params))
	{
	}

	void WidefieldLocalization::ResetImpl(dispatch_tag<gRPCInstrument>)
	{
		ResetImpl(dispatch_tag<WidefieldLocalization>());
	}
}