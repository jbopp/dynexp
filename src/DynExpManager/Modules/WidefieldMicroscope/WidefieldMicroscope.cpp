// This file is part of DynExp.

#include "stdafx.h"
#include "WidefieldMicroscope.h"

namespace DynExpModule::Widefield
{
	constexpr void WidefieldMicroscopeData::PositionPoint::Reset() noexcept
	{
		x = 0;
		y = 0;
		z = 0;

		MeasuredX = 0;
		MeasuredY = 0;
		MeasuredZ = 0;

		UsingX = false;
		UsingY = false;
		UsingZ = false;

		RowIndex = -1;
		ColumnIndex = -1;
	}

	double WidefieldMicroscopeData::PositionPoint::DistTo(const PositionPoint& Other) const
	{
		double dist = 0;

		if (UsingX && Other.UsingX)
			dist += (x - Other.x) * (x - Other.x);
		if (UsingY && Other.UsingY)
			dist += (y - Other.y) * (y - Other.y);
		if (UsingZ && Other.UsingZ)
			dist += (z - Other.z) * (z - Other.z);

		return std::sqrt(dist);
	}

	std::string WidefieldMicroscopeData::PositionPoint::ToStr(std::string_view Prefix) const
	{
		std::stringstream stream;
		stream << std::setprecision(9);

		stream << Prefix << "Valid = " << (!IsEmpty() ? "yes" : "no") << "\n";
		if (UsingX)
		{
			stream << Prefix << "X = " << x << " nm\n";
			stream << Prefix << "MeasuredX = " << MeasuredX << " nm\n";
		}
		if (UsingY)
		{
			stream << Prefix << "Y = " << y << " nm\n";
			stream << Prefix << "MeasuredY = " << MeasuredY << " nm\n";
		}
		if (UsingZ)
		{
			stream << Prefix << "Z = " << z << " nm\n";
			stream << Prefix << "MeasuredZ = " << MeasuredZ << " nm\n";
		}

		return stream.str();
	}

	const char* WidefieldMicroscopeData::GetLocalizedEmitterStateString(LocalizedEmitterStateType State)
	{
		switch (State)
		{
		case LocalizedEmitterStateType::Characterizing: return "Characterizing...";
		case LocalizedEmitterStateType::Finished: return "Finished";
		case LocalizedEmitterStateType::Failed: return "Failed";
		default: return "";
		}
	}

	QColor WidefieldMicroscopeData::GetLocalizedEmitterColor(LocalizedEmitterStateType State)
	{
		switch (State)
		{
		case LocalizedEmitterStateType::Characterizing: return DynExpUI::DarkPalette::blue;
		case LocalizedEmitterStateType::Finished: return "green";
		case LocalizedEmitterStateType::Failed: return "red";
		default: return QApplication::palette().text().color();
		}
	}

	/**
	 * @brief Returns current sample position.
	 * @return Sample position in nm
	*/
	WidefieldMicroscopeData::PositionPoint WidefieldMicroscopeData::GetSamplePosition() const
	{
		if (!TestFeature(WidefieldMicroscopeData::FeatureType::SampleXYPositioning))
			return {};

		auto Ratio = GetSampleStageX()->GetStepNanoMeterRatio();
		auto X = DynExp::dynamic_InstrumentData_cast<DynExpInstr::PositionerStage>(GetSampleStageX()->GetInstrumentData())->GetCurrentPosition();
		auto Y = DynExp::dynamic_InstrumentData_cast<DynExpInstr::PositionerStage>(GetSampleStageY()->GetInstrumentData())->GetCurrentPosition();

		return { static_cast<decltype(X)>(X * Ratio) , static_cast<decltype(Y)>(Y * Ratio) };
	}

	std::stringstream WidefieldMicroscopeData::AssembleCSVHeader(bool IncludeConfocalScan, bool IncludeHBT, bool IncludeAutoMeasure) const
	{
		std::stringstream CSVData;
		CSVData << std::setprecision(9);

		CSVData << "WidefieldPumpPower = " << WidefieldPumpPower << " V\n";
		CSVData << "ConfocalPumpPower = " << ConfocalPumpPower << " V\n";
		CSVData << "FocusCurrentVoltage = " << FocusCurrentVoltage << " V\n";
		CSVData << "FocusZeroVoltage = " << FocusZeroVoltage << " V\n";
		CSVData << "FocusConfocalOffsetVoltage = " << FocusConfocalOffsetVoltage << " V\n";
		
		CSVData << "LEDCameraExposureTime = " << LEDCameraExposureTime.count() << " ms\n";
		CSVData << "WidefieldCameraExposureTime = " << WidefieldCameraExposureTime.count() << " ms\n";
		CSVData << GetSamplePosition().ToStr("CurrentPosition");
		CSVData << WidefieldPosition.ToStr("WidefieldPosition");
		
		CSVData << CurrentCellID;

		if (IncludeConfocalScan)
		{
			CSVData << SampleHomePosition.ToStr("SampleHomePosition");
			CSVData << "ConfocalScanWidth = " << ConfocalScanWidth << "\n";
			CSVData << "ConfocalScanHeight = " << ConfocalScanHeight << "\n";
			CSVData << "ConfocalScanDistPerPixel = " << ConfocalScanDistPerPixel << "\n";
			CSVData << "SPDExposureTime = " << SPDExposureTime.count() << " ms\n";
		}

		CSVData << "ConfocalOptimizationInitXYStepSize = " << ConfocalOptimizationInitXYStepSize << "\n";
		CSVData << "ConfocalOptimizationInitZStepSize = " << ConfocalOptimizationInitZStepSize << "\n";
		CSVData << "ConfocalOptimizationTolerance = " << ConfocalOptimizationTolerance << "\n";

		if (IncludeHBT)
		{
			CSVData << "HBTBinWidth = " << HBTBinWidth.count() << " ps\n";
			CSVData << "HBTBinCount = " << HBTBinCount << "\n";
			CSVData << "HBTMaxIntegrationTime = " << HBTMaxIntegrationTime.count() << " us\n";
			CSVData << HBTSamplePosition.ToStr("HBTSamplePosition");
			CSVData << "HBTNumEventCounts = " << HBTNumEventCounts << "\n";
			CSVData << "HBTTotalIntegrationTime = " << HBTTotalIntegrationTime.count() << " us\n";
		}

		if (IncludeAutoMeasure)
		{
			CSVData << AutoMeasureCurrentCellPosition.ToStr("AutoMeasureCurrentCellPosition");
			CSVData << "AutoMeasureNumberImageSets = " << AutoMeasureNumberImageSets << "\n";
			CSVData << "AutoMeasureCurrentImageSet = " << AutoMeasureCurrentImageSet << "\n";
			CSVData << "AutoMeasureInitialImageSetWaitTime = " << AutoMeasureInitialImageSetWaitTime.count() << " s\n";
			CSVData << "AutoMeasureImagePositionScatterRadius = " << AutoMeasureImagePositionScatterRadius << "\n";
			CSVData << "AutoMeasureLocalizationType = " << AutoMeasureLocalizationType << "\n";
			CSVData << "AutoMeasureOptimizeEnabled = " << (AutoMeasureOptimizeEnabled ? "yes" : "no") << "\n";
			CSVData << "AutoMeasureSpectrumEnabled = " << (AutoMeasureSpectrumEnabled ? "yes" : "no") << "\n";
			CSVData << "AutoMeasureHBTEnabled = " << (AutoMeasureHBTEnabled ? "yes" : "no") << "\n";
			CSVData << "AutoMeasureNumOptimizationAttempts = " << AutoMeasureNumOptimizationAttempts << "\n";
			CSVData << "AutoMeasureCurrentOptimizationAttempt = " << AutoMeasureCurrentOptimizationAttempt << "\n";
			CSVData << "AutoMeasureMaxOptimizationReruns = " << AutoMeasureMaxOptimizationReruns << "\n";
			CSVData << "AutoMeasureCurrentOptimizationRerun = " << AutoMeasureCurrentOptimizationRerun << "\n";
			CSVData << "AutoMeasureOptimizationMaxDistance = " << AutoMeasureOptimizationMaxDistance << "\n";
			CSVData << "AutoMeasureCountRateThreshold = " << AutoMeasureCountRateThreshold << "\n";
			CSVData << "AutoMeasureCellRangeFromX = " << AutoMeasureCellRangeFrom.x() << "\n";
			CSVData << "AutoMeasureCellRangeFromY = " << AutoMeasureCellRangeFrom.y() << "\n";
			CSVData << "AutoMeasureCellRangeToX = " << AutoMeasureCellRangeTo.x() << "\n";
			CSVData << "AutoMeasureCellRangeToY = " << AutoMeasureCellRangeTo.y() << "\n";
			CSVData << "AutoMeasureCellSkipX = " << AutoMeasureCellSkip.x() << "\n";
			CSVData << "AutoMeasureCellSkipY = " << AutoMeasureCellSkip.y() << "\n";
			if (AutoMeasureFirstEmitter != LocalizedPositions.cend())
				CSVData << "AutoMeasureFirstEmitter = " << AutoMeasureFirstEmitter->first << "\n";
			if (AutoMeasureCurrentEmitter != LocalizedPositions.cend())
				CSVData << "AutoMeasureCurrentEmitter = " << AutoMeasureCurrentEmitter->first << "\n";
		}

		CSVData << "HEADER_END\n";

		return CSVData;
	}

	void WidefieldMicroscopeData::WriteConfocalScanResults(std::stringstream& Stream) const
	{
		Stream << "X_measured(nm);Y_measured(nm);X_destiny(nm);Y_destiny(nm);C(Hz)\n";

		auto& ResultItems = GetConfocalScanResults();
		for (const auto& ResultItem : ResultItems)
			Stream << ResultItem.first.MeasuredX << ";" << ResultItem.first.MeasuredY << ";"
				<< ResultItem.first.x << ";" << ResultItem.first.y << ";"
				<< ResultItem.second << "\n";
	}

	void WidefieldMicroscopeData::WriteHBTResults(std::stringstream& Stream) const
	{
		Stream << "t(ps);g2\n";

		for (const auto& ResultItem : GetHBTDataPoints())
			Stream << ResultItem.x() << ";" << ResultItem.y() << "\n";
	}

	void WidefieldMicroscopeData::SetLEDLightTurnedOn(bool State)
	{
		LEDLightTurnedOn = State;
		LEDSwitch->SetSync(State);
	}

	void WidefieldMicroscopeData::SetPumpLightTurnedOn(bool State)
	{
		 PumpLightTurnedOn = State;
		 PumpSwitch->SetSync(State);
	}

	void WidefieldMicroscopeData::IncrementCellID()
	{
		if (Util::NumToT<int>(++CurrentCellID.X_id) > AutoMeasureCellRangeTo.x())
		{
			CurrentCellID.X_id = AutoMeasureCellRangeFrom.x();
			++CurrentCellID.Y_id;
		}

		CurrentCellID.IDString = "x" + Util::ToStr(CurrentCellID.X_id) + "y" + Util::ToStr(CurrentCellID.Y_id);
	}

	size_t WidefieldMicroscopeData::GetNumFinishedLocalizedPositions() const
	{
		return std::count_if(LocalizedPositions.cbegin(), LocalizedPositions.cend(), [](const auto& Pos) {
			return Pos.second.State == LocalizedEmitterStateType::Finished;
		});
	}

	size_t WidefieldMicroscopeData::GetNumFailedLocalizedPositions() const
	{
		return std::count_if(LocalizedPositions.cbegin(), LocalizedPositions.cend(), [](const auto& Pos) {
			return Pos.second.State == LocalizedEmitterStateType::Failed;
		});
	}

	void WidefieldMicroscopeData::AppendLocalizedPosition(LocalizedPositionsMapType::value_type&& Position)
	{
		LocalizedPositions.insert(std::move(Position));
		LocalizedPositionsChanged = true;
	}

	void WidefieldMicroscopeData::SetLocalizedPositions(LocalizedPositionsMapType&& Positions)
	{
		LocalizedPositions = std::move(Positions);
		LocalizedPositionsChanged = true;

		ResetAutoMeasureCurrentEmitter();
	}

	void WidefieldMicroscopeData::ClearLocalizedPositions()
	{
		LocalizedPositions.clear();
		LocalizedPositionsChanged = true;

		ResetAutoMeasureCurrentEmitter();
	}

	std::filesystem::path WidefieldMicroscopeData::GetAutoMeasureSavePath() const
	{
		return CurrentCellID.Valid ?
			(AutoMeasureSavePath.parent_path() / CurrentCellID.IDString / AutoMeasureSavePath.filename()) : AutoMeasureSavePath;
	}

	void WidefieldMicroscopeData::ResetImpl(dispatch_tag<QModuleDataBase>)
	{
		Init();
	}

	bool WidefieldMicroscopeData::IsCellRangeValid() const noexcept
	{
		return AutoMeasureCellRangeTo.x() - AutoMeasureCellRangeFrom.x() >= 0 &&
			AutoMeasureCellRangeTo.y() - AutoMeasureCellRangeFrom.y() >= 0;
	}

	int WidefieldMicroscopeData::GetAutoMeasureCellLineLength() const noexcept
	{
		return AutoMeasureCellRangeTo.x() - AutoMeasureCellRangeFrom.x() + 1;
	}

	int WidefieldMicroscopeData::GetAutoMeasureCellColumnLength() const noexcept
	{
		return AutoMeasureCellRangeTo.y() - AutoMeasureCellRangeFrom.y() + 1;
	}

	int WidefieldMicroscopeData::GetAutoMeasureCellCount() const noexcept
	{
		return GetAutoMeasureCellLineLength() * GetAutoMeasureCellColumnLength();
	}

	int WidefieldMicroscopeData::GetAutoMeasureCurrentCellIndex() const
	{
		if (!CurrentCellID.Valid)
			throw Util::InvalidDataException("The current cell ID is invalid.");

		return (Util::NumToT<int>(CurrentCellID.Y_id) - AutoMeasureCellRangeFrom.y()) * GetAutoMeasureCellLineLength() +
			Util::NumToT<int>(CurrentCellID.X_id) - AutoMeasureCellRangeFrom.x();
	}

	bool WidefieldMicroscopeData::SetAutoMeasureFirstEmitter(Util::MarkerGraphicsView::MarkerType::IDType FirstEmitterID) noexcept
	{
		if (FirstEmitterID < 0)
			return false;

		auto FirstEmitter = LocalizedPositions.find(FirstEmitterID);
		if (FirstEmitter == LocalizedPositions.cend())
			return false;
		
		AutoMeasureFirstEmitter = FirstEmitter;
		AutoMeasureCurrentEmitter = FirstEmitter;

		return true;
	}

	void WidefieldMicroscopeData::ResetAutoMeasureCurrentEmitter() noexcept
	{
		AutoMeasureFirstEmitter = LocalizedPositions.begin();
		AutoMeasureCurrentEmitter = LocalizedPositions.begin();
	}

	void WidefieldMicroscopeData::Init()
	{
		Features = {};
		ClearUIMessage();

		SetupMode = SetupModeType::Unknown;
		LEDLightTurnedOn = false;
		PumpLightTurnedOn = false;
		MinPumpPower = .0;
		MaxPumpPower = .0;
		WidefieldPumpPower = .0;
		ConfocalPumpPower = .0;
		MeasuredPumpPower = .0;
		MinFocusVoltage = .0;
		MaxFocusVoltage = .0;
		FocusCurrentVoltage = .0;
		FocusZeroVoltage = .0;
		FocusConfocalOffsetVoltage = .0;
		AutofocusFinished = false;

		MinCameraExposureTime = CameraTimeType();
		MaxCameraExposureTime = CameraTimeType();
		LEDCameraExposureTime = std::chrono::milliseconds(30);
		WidefieldCameraExposureTime = std::chrono::milliseconds(3500);
		ConfocalSpotImagePosition = { 0, 0 };
		WidefieldPosition.Reset();
		CurrentImage = QImage();
		CurrentImageChanged = false;
		CurrentCellID = {};
		LastCellID = {};
		LocalizedPositions.clear();
		LocalizedPositionsChanged = false;
		LocalizedPositionsStateChanged = false;

		SampleHomePosition.Reset();
		ConfocalScanWidth = 20;
		ConfocalScanHeight = 20;
		ConfocalScanDistPerPixel = 50;
		SPDExposureTime = std::chrono::milliseconds(100);
		SPD1State.Reset();
		SPD2State.Reset();
		ClearConfocalScanResults();
		ClearConfocalScanSurfacePlotRows();

		ConfocalOptimizationInitXYStepSize = 20.0;
		ConfocalOptimizationInitZStepSize = 20.0;
		ConfocalOptimizationTolerance = 1.0;
		LastCountRate = .0;

		HBTBinWidth = Util::picoseconds(500);
		HBTBinCount = 256;
		HBTMaxIntegrationTime = std::chrono::microseconds(0);
		HBTSamplePosition.Reset();
		HBTNumEventCounts = 0;
		HBTTotalIntegrationTime = std::chrono::microseconds(0);
		HBTDataPoints.clear();
		HBTDataPointsMinValues = {};
		HBTDataPointsMaxValues = {};

		AutoMeasureRunning = false;
		AutoMeasureSavePath.clear();
		AutoMeasureCurrentCellPosition.Reset();
		AutoMeasureNumberImageSets = 10;
		AutoMeasureCurrentImageSet = -1;
		AutoMeasureInitialImageSetWaitTime = std::chrono::seconds(30);
		AutoMeasureImagePositionScatterRadius = 0;
		AutoMeasureLocalizationType = WidefieldMicroscopeWidget::LocalizationType::LocalizeEmittersFromImage;
		AutoMeasureOptimizeEnabled = true;
		AutoMeasureSpectrumEnabled = true;
		AutoMeasureHBTEnabled = true;
		AutoMeasureNumOptimizationAttempts = 2;
		AutoMeasureCurrentOptimizationAttempt = 0;
		AutoMeasureMaxOptimizationReruns = 0;
		AutoMeasureCurrentOptimizationRerun = 0;
		AutoMeasureOptimizationMaxDistance = 1000;
		AutoMeasureCountRateThreshold = 40000;
		AutoMeasureCellRangeFrom = { 0, 0 };
		AutoMeasureCellRangeTo = { 10, 10 };
		AutoMeasureCellSkip = { 100000, 100000 };

		ResetAutoMeasureCurrentEmitter();
	}

	WidefieldMicroscopeData::PositionPoint operator+(const WidefieldMicroscopeData::PositionPoint& lhs, const WidefieldMicroscopeData::PositionPoint& rhs)
	{
		WidefieldMicroscopeData::PositionPoint Point;

		if (lhs.UsingX && rhs.UsingX)
		{
			Point.x = lhs.x + rhs.x;
			Point.MeasuredX = lhs.MeasuredX + rhs.MeasuredX;
			Point.UsingX = true;
		}
		else if (lhs.UsingX && !rhs.UsingX)
		{
			Point.x = lhs.x;
			Point.MeasuredX = lhs.MeasuredX;
			Point.UsingX = true;
		}
		else if (!lhs.UsingX && rhs.UsingX)
		{
			Point.x = rhs.x;
			Point.MeasuredX = rhs.MeasuredX;
			Point.UsingX = true;
		}

		if (lhs.UsingY && rhs.UsingY)
		{
			Point.y = lhs.y + rhs.y;
			Point.MeasuredY = lhs.MeasuredY + rhs.MeasuredY;
			Point.UsingY = true;
		}
		else if (lhs.UsingY && !rhs.UsingY)
		{
			Point.y = lhs.y;
			Point.MeasuredY = lhs.MeasuredY;
			Point.UsingY = true;
		}
		else if (!lhs.UsingY && rhs.UsingY)
		{
			Point.y = rhs.y;
			Point.MeasuredY = rhs.MeasuredY;
			Point.UsingY = true;
		}

		if (lhs.UsingZ && rhs.UsingZ)
		{
			Point.z = lhs.z + rhs.z;
			Point.MeasuredZ = lhs.MeasuredZ + rhs.MeasuredZ;
			Point.UsingZ = true;
		}
		else if (lhs.UsingZ && !rhs.UsingZ)
		{
			Point.z = lhs.z;
			Point.MeasuredZ = lhs.MeasuredZ;
			Point.UsingZ = true;
		}
		else if (!lhs.UsingZ && rhs.UsingZ)
		{
			Point.z = rhs.z;
			Point.MeasuredZ = rhs.MeasuredZ;
			Point.UsingZ = true;
		}

		return Point;
	}

	WidefieldMicroscopeData::PositionPoint operator-(const WidefieldMicroscopeData::PositionPoint& lhs, const WidefieldMicroscopeData::PositionPoint& rhs)
	{
		WidefieldMicroscopeData::PositionPoint Point;

		if (lhs.UsingX && rhs.UsingX)
		{
			Point.x = lhs.x - rhs.x;
			Point.MeasuredX = lhs.MeasuredX - rhs.MeasuredX;
			Point.UsingX = true;
		}
		else if (lhs.UsingX && !rhs.UsingX)
		{
			Point.x = lhs.x;
			Point.MeasuredX = lhs.MeasuredX;
			Point.UsingX = true;
		}
		else if (!lhs.UsingX && rhs.UsingX)
		{
			Point.x = -rhs.x;
			Point.MeasuredX = -rhs.MeasuredX;
			Point.UsingX = true;
		}

		if (lhs.UsingY && rhs.UsingY)
		{
			Point.y = lhs.y - rhs.y;
			Point.MeasuredY = lhs.MeasuredY - rhs.MeasuredY;
			Point.UsingY = true;
		}
		else if (lhs.UsingY && !rhs.UsingY)
		{
			Point.y = lhs.y;
			Point.MeasuredY = lhs.MeasuredY;
			Point.UsingY = true;
		}
		else if (!lhs.UsingY && rhs.UsingY)
		{
			Point.y = -rhs.y;
			Point.MeasuredY = -rhs.MeasuredY;
			Point.UsingY = true;
		}

		if (lhs.UsingZ && rhs.UsingZ)
		{
			Point.z = lhs.z - rhs.z;
			Point.MeasuredZ = lhs.MeasuredZ - rhs.MeasuredZ;
			Point.UsingZ = true;
		}
		else if (lhs.UsingZ && !rhs.UsingZ)
		{
			Point.z = lhs.z;
			Point.MeasuredZ = lhs.MeasuredZ;
			Point.UsingZ = true;
		}
		else if (!lhs.UsingZ && rhs.UsingZ)
		{
			Point.z = -rhs.z;
			Point.MeasuredZ = -rhs.MeasuredZ;
			Point.UsingZ = true;
		}

		return Point;
	}

	WidefieldMicroscope::WidefieldMicroscope(const std::thread::id OwnerThreadID, DynExp::ParamsBasePtrType&& Params)
		: QModuleBase(OwnerThreadID, std::move(Params)),
		StateMachine(InitializingState,
			SetupTransitionBeginState, SetupTransitioningState, SetupTransitionEndState, SetupTransitionFinishedState,
			ReadyState,
			AutofocusBeginState, AutofocusWaitingState, AutofocusFinishedState,
			LEDImageAcquisitionBeginState, WidefieldImageAcquisitionBeginState,
			WaitingForLEDImageReadyToCaptureState, WaitingForLEDImageState, WaitingForLEDImageFinishedState,
			WaitingForWidefieldImageReadyToCaptureState, WaitingForWidefieldImageState, WaitingForWidefieldImageFinishedState,
			WaitingForWidefieldCellIDState, WidefieldCellWaitUntilCenteredState, WidefieldCellIDReadFinishedState,
			WaitingForWidefieldLocalizationState, WidefieldLocalizationFinishedState,
			FindingConfocalSpotBeginState, FindingConfocalSpotAfterTransitioningToConfocalModeState, FindingConfocalSpotAfterRecordingWidefieldImageState,
			ConfocalScanStepState, ConfocalScanWaitUntilMovedState, ConfocalScanCaptureState, ConfocalScanWaitUntilCapturedState,
			ConfocalOptimizationInitState, ConfocalOptimizationInitSubStepState, ConfocalOptimizationWaitState, ConfocalOptimizationStepState, ConfocalOptimizationFinishedState,
			HBTAcquiringState, HBTFinishedState,
			WaitingState, WaitingFinishedState,
			SpectrumAcquisitionWaitingState, SpectrumAcquisitionFinishedState,
			AutoMeasureLocalizationStepState, AutoMeasureLocalizationSaveLEDImageState, AutoMeasureLocalizationSaveWidefieldImageState,
			AutoMeasureLocalizationMovingState, AutoMeasureLocalizationFinishedState,
			AutoMeasureCharacterizationStepState, AutoMeasureCharacterizationGotoEmitterState, AutoMeasureCharacterizationOptimizationFinishedState,
			AutoMeasureCharacterizationSpectrumBeginState, AutoMeasureCharacterizationSpectrumFinishedState,
			AutoMeasureCharacterizationHBTBeginState, AutoMeasureCharacterizationHBTWaitForInitState, AutoMeasureCharacterizationHBTFinishedState,
			AutoMeasureCharacterizationFinishedState,
			AutoMeasureSampleStepState, AutoMeasureSampleReadCellIDState, AutoMeasureSampleReadCellIDFinishedState,
			AutoMeasureSampleLocalizeState, AutoMeasureSampleFindEmittersState,
			AutoMeasureSampleCharacterizeState, AutoMeasureSampleAdvanceCellState, AutoMeasureSampleFinishedState),
		ConfocalScanPositionerStateX(std::make_shared<AtomicPositionerStateType>()),
		ConfocalScanPositionerStateY(std::make_shared<AtomicPositionerStateType>()),
		ConfocalScanPositionerStateZ(std::make_shared<AtomicPositionerStateType>()),
		WidefieldCellIDState(std::make_shared<AtomicWidefieldImageProcessingStateType>()),
		WidefieldLocalizationState(std::make_shared<AtomicWidefieldImageProcessingStateType>()),
		GSLConfocalOptimizationState(gsl_multimin_fminimizer_alloc(GSLConfocalOptimizationMinimizer, GSLConfocalOptimizationNumDimensions)),
		GSLConfocalOptimizationStepSize(gsl_vector_alloc(GSLConfocalOptimizationNumDimensions)),
		GSLConfocalOptimizationInitialPoint(gsl_vector_alloc(GSLConfocalOptimizationNumDimensions))
	{
		if (!GSLConfocalOptimizationState || !GSLConfocalOptimizationStepSize || !GSLConfocalOptimizationInitialPoint)
			throw Util::NotAvailableException("Could not initialize GSL multimin library components.", Util::ErrorType::Error);
	}

	WidefieldMicroscope::~WidefieldMicroscope()
	{
		if (GSLConfocalOptimizationInitialPoint)
			gsl_vector_free(GSLConfocalOptimizationInitialPoint);
		if (GSLConfocalOptimizationStepSize)
			gsl_vector_free(GSLConfocalOptimizationStepSize);
		if (GSLConfocalOptimizationState)
			gsl_multimin_fminimizer_free(GSLConfocalOptimizationState);
	}

	std::chrono::milliseconds WidefieldMicroscope::GetMainLoopDelay() const
	{
		auto CurrentState = StateMachine.GetCurrentState()->GetState();

		// If doing some measurement, run much faster.
		if (CurrentState == StateType::Ready)
			return std::chrono::milliseconds(30);
		else
			return std::chrono::milliseconds(2);
	}

	void WidefieldMicroscope::OnSaveCurrentImage(DynExp::ModuleInstance* Instance, QString Filename) const
	{
		static constexpr const char* SaveErrorMsg = "Saving an image failed.";

		QImage Image;
		{
			auto ModuleData = DynExp::dynamic_ModuleData_cast<WidefieldMicroscope>(Instance->ModuleDataGetter());
			Image = ModuleData->GetCurrentImage().copy();
		} // ModuleData unlocked here for heavy save operation.

		if (!Image.save(Filename))
		{
			if (LogUIMessagesOnly)
				Util::EventLogger().Log(SaveErrorMsg, Util::ErrorType::Error);
			else
			{
				auto ModuleData = DynExp::dynamic_ModuleData_cast<WidefieldMicroscope>(Instance->ModuleDataGetter());
				ModuleData->SetUIMessage(SaveErrorMsg);
			}
		}
	}

	void WidefieldMicroscope::OnGoToSamplePos(DynExp::ModuleInstance* Instance, QPointF SamplePos) const
	{
		auto ModuleData = DynExp::dynamic_ModuleData_cast<WidefieldMicroscope>(Instance->ModuleDataGetter());

		if (!IsReadyState() || !ModuleData->TestFeature(WidefieldMicroscopeData::FeatureType::Widefield) || SamplePos.isNull())
			return;

		MoveSampleTo(
			{ Util::NumToT<WidefieldMicroscopeData::PositionType>(SamplePos.x()), Util::NumToT<WidefieldMicroscopeData::PositionType>(SamplePos.y()) },
			ModuleData
		);
	}

	void WidefieldMicroscope::OnBringMarkerToConfocalSpot(DynExp::ModuleInstance* Instance, QPoint MarkerPos, QPointF SamplePos) const
	{
		auto ModuleParams = DynExp::dynamic_Params_cast<WidefieldMicroscope>(Instance->ParamsGetter());
		auto ModuleData = DynExp::dynamic_ModuleData_cast<WidefieldMicroscope>(Instance->ModuleDataGetter());

		if (!IsReadyState() || !ModuleData->TestFeature(WidefieldMicroscopeData::FeatureType::Widefield) || SamplePos.isNull())
			return;

		BringMarkerToConfocalSpot(ModuleParams, ModuleData, MarkerPos, SamplePos);
	}

	void WidefieldMicroscope::OnRunCharacterizationFromID(DynExp::ModuleInstance* Instance, Util::MarkerGraphicsView::MarkerType::IDType ID) const
	{
		auto ModuleData = DynExp::dynamic_ModuleData_cast<WidefieldMicroscope>(Instance->ModuleDataGetter());

		StateMachine.SetCurrentState(StartAutoMeasureCharacterization(ModuleData, ID));
	}

	Util::DynExpErrorCodes::DynExpErrorCodes WidefieldMicroscope::ModuleMainLoop(DynExp::ModuleInstance& Instance)
	{
		try
		{
			auto ModuleData = DynExp::dynamic_ModuleData_cast<WidefieldMicroscope>(Instance.ModuleDataGetter());

			if (ModuleData->TestFeature(WidefieldMicroscopeData::FeatureType::MeasurePumpPower))
				if (ModuleData->GetPumpPowerIndicator()->CanRead())
					ModuleData->SetMeasuredPumpPower(ModuleData->GetPumpPowerIndicator()->Get());

			StateMachine.Invoke(*this, Instance);

			NumFailedUpdateAttempts = 0;
		}
		catch (const Util::TimeoutException& e)
		{
			if (NumFailedUpdateAttempts++ >= 3)
				Instance.GetOwner().SetWarning(e);
		}

		return Util::DynExpErrorCodes::NoError;
	}

	void WidefieldMicroscope::ResetImpl(dispatch_tag<QModuleBase>)
	{
		StateMachine.SetCurrentState(StateType::Initializing);

		*ConfocalScanPositionerStateX = PositionerStateType::Arrived;
		*ConfocalScanPositionerStateY = PositionerStateType::Arrived;
		*ConfocalScanPositionerStateZ = PositionerStateType::Arrived;
		*WidefieldCellIDState = WidefieldImageProcessingStateType::Finished;
		*WidefieldLocalizationState = WidefieldImageProcessingStateType::Finished;

		ConfocalScanPositions.clear();

		ConfocalOptimizationNumStepsPerformed = 0;
		ConfocalOptimizationPromisesRenewed = false;

		TurnOnPumpSourceAfterTransitioning = false;
		ImageCapturingPaused = false;

		HBTIntegrationTimeBeforeReset = {};

		NumFailedUpdateAttempts = 0;
		LogUIMessagesOnly = false;
	}

	std::unique_ptr<DynExp::QModuleWidget> WidefieldMicroscope::MakeUIWidget()
	{
		auto Widget = std::make_unique<WidefieldMicroscopeWidget>(*this);

		Connect(Widget->GetUI().action_Terminate, &QAction::triggered, this, &WidefieldMicroscope::OnTerminate);
		Connect(Widget->GetUI().action_Stop_current_action, &QAction::triggered, this, &WidefieldMicroscope::OnStopAction);
		Connect(Widget->GetUI().action_Set_home_position, &QAction::triggered, this, &WidefieldMicroscope::OnSetHomePosition);
		Connect(Widget->GetUI().action_Go_home_position, &QAction::triggered, this, &WidefieldMicroscope::OnGoToHomePosition);
		Connect(Widget->GetUI().action_Toogle_LED_light_source, &QAction::triggered, this, &WidefieldMicroscope::OnToggleLEDLightSource);
		Connect(Widget->GetUI().action_Toogle_pump_light_source, &QAction::triggered, this, &WidefieldMicroscope::OnTogglePumpLightSource);
		Connect(Widget->GetWidefieldConfocalModeActionGroup(), &QActionGroup::triggered, this, &WidefieldMicroscope::OnSetupModeChanged);
		Connect(Widget->GetUI().action_Autofocus, &QAction::triggered, this, &WidefieldMicroscope::OnAutofocus);
		Connect(Widget->GetUI().action_Optimize_positions, &QAction::triggered, this, &WidefieldMicroscope::OnOptimizePositions);
		Connect(Widget->GetUI().action_Toggle_HBT_mirror, &QAction::triggered, this, &WidefieldMicroscope::OnToggleHBTMirror);
		Connect(Widget->GetUI().action_Reset_CellID, &QAction::triggered, this, &WidefieldMicroscope::OnResetCellID);
		Connect(Widget->GetUI().SBGeneralWidefieldPower, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &WidefieldMicroscope::OnGeneralWidefieldPowerChanged);
		Connect(Widget->GetUI().SBGeneralConfocalPower, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &WidefieldMicroscope::OnGeneralConfocalPowerChanged);
		Connect(Widget->GetUI().SBGeneralFocusCurrentVoltage, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &WidefieldMicroscope::OnGeneralFocusCurrentVoltageChanged);
		Connect(Widget->GetUI().SBGeneralFocusZeroVoltage, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &WidefieldMicroscope::OnGeneralFocusZeroVoltageChanged);
		Connect(Widget->GetUI().SBGeneralFocusConfocalOffsetVoltage, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &WidefieldMicroscope::OnGeneralFocusConfocalOffsetVoltageChanged);
		Connect(Widget->GetUI().BGeneralFocusSetZeroVoltage, &QPushButton::clicked, this, &WidefieldMicroscope::OnGeneralSetZeroFocus);
		Connect(Widget->GetUI().BGeneralFocusApplyZeroVoltage, &QPushButton::clicked, this, &WidefieldMicroscope::OnGeneralApplyZeroFocus);
		Connect(Widget->GetUI().SBWidefieldLEDExposureTime, QOverload<int>::of(&QSpinBox::valueChanged), this, &WidefieldMicroscope::OnWidefieldLEDExposureTimeChanged);
		Connect(Widget->GetUI().BWidefieldApplyLEDExposureTime, &QPushButton::clicked, this, &WidefieldMicroscope::OnWidefieldApplyLEDExposureTime);
		Connect(Widget->GetUI().SBWidefieldPumpExposureTime, QOverload<int>::of(&QSpinBox::valueChanged), this, &WidefieldMicroscope::OnWidefieldPumpExposureTimeChanged);
		Connect(Widget->GetUI().BWidefieldApplyPumpExposureTime, &QPushButton::clicked, this, &WidefieldMicroscope::OnWidefieldApplyPumpExposureTime);
		Connect(Widget->GetUI().BWidefieldFindConfocalSpot, &QPushButton::clicked, this, &WidefieldMicroscope::OnWidefieldFindConfocalSpot);
		Connect(Widget->GetUI().BWidefieldLEDCapture, &QPushButton::clicked, this, &WidefieldMicroscope::OnCaptureLEDImage);
		Connect(Widget->GetUI().BWidefieldCapture, &QPushButton::clicked, this, &WidefieldMicroscope::OnCaptureWidefieldImage);
		Connect(Widget->GetUI().BReadCellID, &QPushButton::clicked, this, &WidefieldMicroscope::OnWidefieldReadCellID);
		Connect(Widget->GetUI().BAnalyzeImageDistortion, &QPushButton::clicked, this, &WidefieldMicroscope::OnWidefieldAnalyzeImageDistortion);
		Connect(Widget->GetUI().BLocalizeEmitters, &QPushButton::clicked, this, &WidefieldMicroscope::OnWidefieldLocalizeEmitters);
		Connect(Widget->GetMainGraphicsView(), &Util::MarkerGraphicsView::mouseClickEvent, this, &WidefieldMicroscope::OnWidefieldImageClicked);
		Connect(Widget->GetUI().SBConfocalWidth, QOverload<int>::of(&QSpinBox::valueChanged), this, &WidefieldMicroscope::OnConfocalConfocalWidthChanged);
		Connect(Widget->GetUI().SBConfocalHeight, QOverload<int>::of(&QSpinBox::valueChanged), this, &WidefieldMicroscope::OnConfocalConfocalHeightChanged);
		Connect(Widget->GetUI().SBConfocalDistPerPixel, QOverload<int>::of(&QSpinBox::valueChanged), this, &WidefieldMicroscope::OnConfocalConfocalDistPerPixelChanged);
		Connect(Widget->GetUI().SBConfocalSPDExposureTime, QOverload<int>::of(&QSpinBox::valueChanged), this, &WidefieldMicroscope::OnConfocalSPDExposureTimeChanged);
		Connect(Widget->GetUI().SBConfocalOptimizationInitXYStepSize, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &WidefieldMicroscope::OnConfocalOptimizationInitXYStepSizeChanged);
		Connect(Widget->GetUI().SBConfocalOptimizationInitZStepSize, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &WidefieldMicroscope::OnConfocalOptimizationInitZStepSizeChanged);
		Connect(Widget->GetUI().SBConfocalOptimizationTolerance, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &WidefieldMicroscope::OnConfocalOptimizationToleranceChanged);
		Connect(Widget->GetUI().BConfocalScan, &QPushButton::clicked, this, &WidefieldMicroscope::OnPerformConfocalScan);
		Connect(Widget->GetConfocalSurface3DSeries(), &QSurface3DSeries::selectedPointChanged, this, &WidefieldMicroscope::ConfocalSurfaceSelectedPointChanged);
		Connect(Widget->GetUI().SBHBTBinWidth, QOverload<int>::of(&QSpinBox::valueChanged), this, &WidefieldMicroscope::OnHBTBinWidthChanged);
		Connect(Widget->GetUI().SBHBTBinCount, QOverload<int>::of(&QSpinBox::valueChanged), this, &WidefieldMicroscope::OnHBTBinCountChanged);
		Connect(Widget->GetUI().SBHBTAcquisitionTime, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &WidefieldMicroscope::OnHHBTMaxIntegrationTimeChanged);
		Connect(Widget->GetUI().BHBT, &QPushButton::clicked, this, &WidefieldMicroscope::OnMeasureHBT);
		Connect(Widget->GetUI().LEAutoMeasureSavePath, &QLineEdit::textChanged, this, &WidefieldMicroscope::OnAutoMeasureSavePathChanged);
		Connect(Widget->GetUI().SBAutoMeasureNumberImageSets, QOverload<int>::of(&QSpinBox::valueChanged), this, &WidefieldMicroscope::OnAutoMeasureNumberImageSetsChanged);
		Connect(Widget->GetUI().SBAutoMeasureInitialImageSetWaitTime, QOverload<int>::of(&QSpinBox::valueChanged), this, &WidefieldMicroscope::OnAutoMeasureInitialImageSetWaitTimeChanged);
		Connect(Widget->GetUI().SBAutoMeasureImagePositionScatterRadius, QOverload<int>::of(&QSpinBox::valueChanged), this, &WidefieldMicroscope::OnAutoMeasureImagePositionScatterRadius);
		Connect(Widget->GetUI().CBAutoMeasureLocalize, &QComboBox::currentIndexChanged, this, &WidefieldMicroscope::OnAutoMeasureLocalizationTypeChanged);
		Connect(Widget->GetUI().CBAutoMeasureOptimize, &QCheckBox::stateChanged, this, &WidefieldMicroscope::OnToggleAutoMeasureOptimizeEnabled);
		Connect(Widget->GetUI().CBAutoMeasureEnableSpectrum, &QCheckBox::stateChanged, this, &WidefieldMicroscope::OnToggleAutoMeasureSpectrumEnabled);
		Connect(Widget->GetUI().CBAutoMeasureEnableHBT, &QCheckBox::stateChanged, this, &WidefieldMicroscope::OnToggleAutoMeasureHBTEnabled);
		Connect(Widget->GetUI().SBAutoMeasureOptimizationAttempts, QOverload<int>::of(&QSpinBox::valueChanged), this, &WidefieldMicroscope::OnAutoMeasureNumOptimizationAttemptsChanged);
		Connect(Widget->GetUI().SBAutoMeasureOptimizationReruns, QOverload<int>::of(&QSpinBox::valueChanged), this, &WidefieldMicroscope::OnAutoMeasureMaxOptimizationRerunsChanged);
		Connect(Widget->GetUI().SBAutoMeasureOptimizationMaxDistance, QOverload<int>::of(&QSpinBox::valueChanged), this, &WidefieldMicroscope::OnAutoMeasureOptimizationMaxDistanceChanged);
		Connect(Widget->GetUI().SBAutoMeasureCountRateThreshold, QOverload<int>::of(&QSpinBox::valueChanged), this, &WidefieldMicroscope::OnAutoMeasureCountRateThresholdChanged);
		Connect(Widget->GetUI().SBAutoMeasureCellRangeFromX, QOverload<int>::of(&QSpinBox::valueChanged), this, &WidefieldMicroscope::OnAutoMeasureCellRangeFromXChanged);
		Connect(Widget->GetUI().SBAutoMeasureCellRangeFromY, QOverload<int>::of(&QSpinBox::valueChanged), this, &WidefieldMicroscope::OnAutoMeasureCellRangeFromYChanged);
		Connect(Widget->GetUI().SBAutoMeasureCellRangeToX, QOverload<int>::of(&QSpinBox::valueChanged), this, &WidefieldMicroscope::OnAutoMeasureCellRangeToXChanged);
		Connect(Widget->GetUI().SBAutoMeasureCellRangeToY, QOverload<int>::of(&QSpinBox::valueChanged), this, &WidefieldMicroscope::OnAutoMeasureCellRangeToYChanged);
		Connect(Widget->GetUI().SBAutoMeasureCellSkipX, QOverload<int>::of(&QSpinBox::valueChanged), this, &WidefieldMicroscope::OnAutoMeasureCellSkipXChanged);
		Connect(Widget->GetUI().SBAutoMeasureCellSkipY, QOverload<int>::of(&QSpinBox::valueChanged), this, &WidefieldMicroscope::OnAutoMeasureCellSkipYChanged);
		Connect(Widget->GetUI().BAutoMeasureRunLocalization, &QPushButton::clicked, this, &WidefieldMicroscope::OnAutoMeasureRunLocalization);
		Connect(Widget->GetUI().BAutoMeasureRunCharacterization, &QPushButton::clicked, this, &WidefieldMicroscope::OnAutoMeasureRunCharacterization);
		Connect(Widget->GetUI().BAutoMeasureRunSampleCharacterization, &QPushButton::clicked, this, &WidefieldMicroscope::OnAutoMeasureRunSampleCharacterization);

		auto ModuleData = DynExp::dynamic_ModuleData_cast<WidefieldMicroscope>(GetModuleData());
		ModuleData->SetSPDExposureTime(std::chrono::milliseconds(Widget->GetUI().SBConfocalSPDExposureTime->value()));

		return Widget;
	}

	void WidefieldMicroscope::UpdateUIChild(const ModuleBase::ModuleDataGetterType& ModuleDataGetter)
	{
		auto Widget = GetWidget<WidefieldMicroscopeWidget>();
		std::string UIMessage;

		{
			auto ModuleData = DynExp::dynamic_ModuleData_cast<WidefieldMicroscope>(ModuleDataGetter());

			Widget->InitializeUI(ModuleData);
			Widget->SetUIState(StateMachine.GetCurrentState(), StateMachine.GetContext(), ModuleData);
			Widget->UpdateUIData(ModuleData);
			Widget->UpdateWidefieldUIData(ModuleData);
			Widget->UpdateConfocalUIData(ModuleData);
			Widget->UpdateHBTUIData(ModuleData);
			Widget->UpdateAutoMeasureUIData(ModuleData, IsCharacterizingSample());
			Widget->UpdateScene();

			if (!ModuleData->GetUIMessage().empty())
			{
				UIMessage = ModuleData->GetUIMessage();
				ModuleData->ClearUIMessage();
			}
		} // ModuleData unlocked here.

		if (!UIMessage.empty())
			QMessageBox::warning(Widget, "Widefield Microscope", UIMessage.data());
	}

	/**
	 * @brief Aborts a measurement or experimental sequence and continues with the ready state.
	 * @param ModuleData Reference to WidefieldMicroscope's module data.
	 * @return Always returns StateType::Ready.
	*/
	StateType WidefieldMicroscope::ResetState(Util::SynchronizedPointer<ModuleDataType>& ModuleData) const
	{
		if (ModuleData->TestFeature(WidefieldMicroscopeData::FeatureType::InterModuleCommunicator))
			ModuleData->GetAcqCommunicator()->PostEvent(*this, SpectrumViewer::SetSilentModeEvent{ false });

		ModuleData->ResetAutoMeasureCurrentImageSet();
		ModuleData->SetAutoMeasureRunning(false);

		LogUIMessagesOnly = false;

		StateMachine.ResetContext();

		// Must always return StateType::Ready.
		return StateType::Ready;
	}

	/**
	 * @brief Returns whether the microscope is characterizing an entire sample moving from cell to cell.
	 * This function is thread-safe since @p StateMachine::GetContext() reads the current context atomically.
	 * @return True if currently characterizing an entire sample, false otherwise.
	*/
	bool WidefieldMicroscope::IsCharacterizingSample() const noexcept
	{
		auto CurrentContext = StateMachine.GetContext();

		return CurrentContext == &AutoMeasureSampleContext ||
			CurrentContext == &AutoMeasureSampleRecenterCellContext ||
			CurrentContext == &AutoMeasureSampleLocalizationContext ||
			CurrentContext == &AutoMeasureSampleCharacterizationContext ||
			CurrentContext == &AutoMeasureSampleCharacterizationOptimizationContext ||
			CurrentContext == &AutoMeasureSampleCharacterizationSpectrumContext ||
			CurrentContext == &AutoMeasureSampleCharacterizationHBTContext;
	}

	// Point in nm
	void WidefieldMicroscope::MoveSampleTo(const ModuleDataType::PositionPoint& Point, Util::SynchronizedPointer<ModuleDataType>& ModuleData) const
	{
		if (Point.UsingX && ModuleData->TestFeature(WidefieldMicroscopeData::FeatureType::SampleXYPositioning))
		{
			*ConfocalScanPositionerStateX = PositionerStateType::WaitingForMovement;
			ModuleData->GetSampleStageX()->MoveAbsolute(Point.x / ModuleData->GetSampleStageX()->GetStepNanoMeterRatio(),
				[PositionerState = ConfocalScanPositionerStateX](const DynExp::TaskBase&, DynExp::ExceptionContainer&) {
					*PositionerState = PositionerStateType::Moving;
				}
			);
		}

		if (Point.UsingY && ModuleData->TestFeature(WidefieldMicroscopeData::FeatureType::SampleXYPositioning))
		{
			*ConfocalScanPositionerStateY = PositionerStateType::WaitingForMovement;
			ModuleData->GetSampleStageY()->MoveAbsolute(Point.y / ModuleData->GetSampleStageX()->GetStepNanoMeterRatio(),
				[PositionerState = ConfocalScanPositionerStateY](const DynExp::TaskBase&, DynExp::ExceptionContainer&) {
					*PositionerState = PositionerStateType::Moving;
				}
			);
		}

		if (Point.UsingZ && ModuleData->TestFeature(WidefieldMicroscopeData::FeatureType::SampleZPositioning))
		{
			*ConfocalScanPositionerStateZ = PositionerStateType::WaitingForMovement;
			ModuleData->GetSampleStageZ()->MoveAbsolute(Point.z / ModuleData->GetSampleStageX()->GetStepNanoMeterRatio(),
				[PositionerState = ConfocalScanPositionerStateZ](const DynExp::TaskBase&, DynExp::ExceptionContainer&) {
					*PositionerState = PositionerStateType::Moving;
				}
			);
		}
	}

	/**
	 * @brief Checks whether the sample is moving in x- or y-direction.
	 * The z-direction is ignored currently since this module doesn't currently make use of it.
	 * @param ModuleData WidefieldMicroscope's locked module data
	 * @return true if the sample is moving, false otherwise
	*/
	bool WidefieldMicroscope::IsSampleMoving(Util::SynchronizedPointer<ModuleDataType>& ModuleData) const
	{
		if (*ConfocalScanPositionerStateX == PositionerStateType::Arrived && *ConfocalScanPositionerStateY == PositionerStateType::Arrived)
			return false;
		if (*ConfocalScanPositionerStateX != PositionerStateType::Moving || *ConfocalScanPositionerStateY != PositionerStateType::Moving)
			return true;	// Movement has not started yet.

		auto SampleStageXData = DynExp::dynamic_InstrumentData_cast<DynExpInstr::PositionerStage>(ModuleData->GetSampleStageX()->GetInstrumentData());
		auto SampleStageYData = DynExp::dynamic_InstrumentData_cast<DynExpInstr::PositionerStage>(ModuleData->GetSampleStageY()->GetInstrumentData());
		bool IsMoving = !SampleStageXData->HasArrived() || !SampleStageYData->HasArrived();

		if (!IsMoving)
		{
			*ConfocalScanPositionerStateX = PositionerStateType::Arrived;
			*ConfocalScanPositionerStateY = PositionerStateType::Arrived;
		}

		return IsMoving;
	}

	void WidefieldMicroscope::SetFocus(Util::SynchronizedPointer<ModuleDataType>& ModuleData, double Voltage, bool IgnoreOffset) const
	{
		if (!ModuleData->TestFeature(WidefieldMicroscopeData::FeatureType::FocusAdjustment))
			return;

		ModuleData->GetSampleFocusPiezoZ()->SetSync(std::max(ModuleData->GetMinFocusVoltage(),
			std::min(ModuleData->GetMaxFocusVoltage(), Voltage +
				((ModuleData->GetSetupMode() == WidefieldMicroscopeData::SetupModeType::Confocal && !IgnoreOffset) ? ModuleData->GetFocusConfocalOffsetVoltage() : .0))));
		ModuleData->SetFocusCurrentVoltage(Voltage);
	}

	WidefieldMicroscopeData::PositionPoint WidefieldMicroscope::CalcMarkerToConfocalSpotDestiny(Util::SynchronizedPointer<const ParamsType>& ModuleParams, Util::SynchronizedPointer<ModuleDataType>& ModuleData,
		QPoint MarkerPos, QPointF SamplePos) const
	{
		const auto DiffPosInPx = ModuleData->GetConfocalSpotImagePosition() - MarkerPos;
		const WidefieldMicroscopeData::PositionPoint SamplePosPoint(Util::NumToT<WidefieldMicroscopeData::PositionType>(SamplePos.x()), Util::NumToT<WidefieldMicroscopeData::PositionType>(SamplePos.y()));

		return SamplePosPoint + PositionPointFromPixelDist(ModuleParams, ModuleData, DiffPosInPx.x(), DiffPosInPx.y());
	}

	void WidefieldMicroscope::BringMarkerToConfocalSpot(Util::SynchronizedPointer<const ParamsType>& ModuleParams, Util::SynchronizedPointer<ModuleDataType>& ModuleData,
		QPoint MarkerPos, QPointF SamplePos) const
	{
		MoveSampleTo(CalcMarkerToConfocalSpotDestiny(ModuleParams, ModuleData, MarkerPos, SamplePos), ModuleData);
	}

	WidefieldMicroscope::ModuleDataType::QSurfaceDataRowsType WidefieldMicroscope::CalculateConfocalScanPositions(const int Width, const int Height,
		const int DistPerPixel, const WidefieldMicroscopeData::PositionPoint CenterPosition) const
	{
		ModuleDataType::QSurfaceDataRowsType QSurfaceDataRows;
		ConfocalScanPositions.clear();

		for (int i = 1; i <= Height; ++i)
		{
			auto y = CenterPosition.y + (-static_cast<double>(Height) / 2 + i) * DistPerPixel;
			auto Row = std::make_unique<QSurfaceDataRow>(Width);

			for (auto j = Width; j > 0; --j)
			{
				double x{};

				// Create a 'wiggly' line.
				if (i % 2)
					x = CenterPosition.x + (-static_cast<double>(Width) / 2 + j) * DistPerPixel;
				else
					x = CenterPosition.x + (static_cast<double>(Width) / 2 - j + 1) * DistPerPixel;

				auto ColIndex = i % 2 ? j - 1 : Width - j;
				(*Row)[ColIndex].setPosition(QVector3D(std::round(x), 0, std::round(y)));
				ConfocalScanPositions.emplace_back(static_cast<WidefieldMicroscopeData::PositionType>(x), static_cast<WidefieldMicroscopeData::PositionType>(y));
				ConfocalScanPositions.back().RowIndex = Util::NumToT<int>(QSurfaceDataRows.size());
				ConfocalScanPositions.back().ColumnIndex = ColIndex;
			}

			QSurfaceDataRows.push_back(std::move(Row));
		}

		return QSurfaceDataRows;
	}

	void WidefieldMicroscope::UpdatePumpPower(Util::SynchronizedPointer<ModuleDataType>& ModuleData) const
	{
		if (!ModuleData->TestFeature(WidefieldMicroscopeData::FeatureType::SetPumpPower))
			return;

		if (ModuleData->GetSetupMode() == WidefieldMicroscopeData::SetupModeType::Widefield || ModuleData->GetSetupMode() == WidefieldMicroscopeData::SetupModeType::Unknown)
			ModuleData->GetPumpPower()->SetSync(ModuleData->GetWidefieldPumpPower());
		else
			ModuleData->GetPumpPower()->SetSync(ModuleData->GetConfocalPumpPower());
	}

	void WidefieldMicroscope::PrepareImageRecording(Util::SynchronizedPointer<ModuleDataType>& ModuleData) const
	{
		ImageCapturingPaused = false;
		if (ModuleData->TestFeature(WidefieldMicroscopeData::FeatureType::InterModuleCommunicator))
			ModuleData->GetAcqCommunicator()->PostEvent(*this, ImageViewer::PauseImageCapturingEvent{ true });

		ModuleData->GetWidefieldCamera()->StopCapturingSync();

		{
			auto CameraData = DynExp::dynamic_InstrumentData_cast<DynExpInstr::Camera>(ModuleData->GetWidefieldCamera()->GetInstrumentData());
			CameraData->ClearImage();
		} // CameraData unlocked here.
	}

	void WidefieldMicroscope::RecordImage(Util::SynchronizedPointer<ModuleDataType>& ModuleData) const
	{
		ModuleData->GetWidefieldCamera()->CaptureSingle();
	}

	StateType WidefieldMicroscope::InitiateReadCellIDFromImage(Util::SynchronizedPointer<ModuleDataType>& ModuleData) const
	{
		if (ModuleData->GetCurrentImage().isNull())
			*WidefieldCellIDState = WidefieldImageProcessingStateType::Failed;
		else
		{
			*WidefieldCellIDState = WidefieldImageProcessingStateType::Waiting;
			ModuleData->GetWidefieldLocalizer()->ReadCellID(ModuleData->GetCurrentImage(),
				[CellIDState = WidefieldCellIDState](const DynExp::TaskBase&, DynExp::ExceptionContainer& Exception) {
					try
					{
						Exception.Throw();
					}
					catch (const Util::ServiceFailedException& e)
					{
						*CellIDState = WidefieldImageProcessingStateType::Failed;
						Exception.ClearError();

						Util::EventLog().Log("Reading the cell ID from the current image, the error listed below occurred.", Util::ErrorType::Error);
						Util::EventLog().Log(e.what());

						return;
					}

					*CellIDState = WidefieldImageProcessingStateType::Finished;
				}
			);
		}

		return StateType::WaitingForWidefieldCellID;
	}

	StateType WidefieldMicroscope::InitiateLocalizationFromImage(Util::SynchronizedPointer<ModuleDataType>& ModuleData) const
	{
		if (ModuleData->GetCurrentImage().isNull())
			*WidefieldLocalizationState = WidefieldImageProcessingStateType::Failed;
		else
		{
			const auto CallbackFunc = [LocalizerState = WidefieldLocalizationState](const DynExp::TaskBase&, DynExp::ExceptionContainer& Exception) {
				try
				{
					Exception.Throw();
				}
				catch (const Util::ServiceFailedException& e)
				{
					*LocalizerState = WidefieldImageProcessingStateType::Failed;
					Exception.ClearError();

					Util::EventLog().Log("Localizing emitters in the widefield image, the error listed below occurred.", Util::ErrorType::Error);
					Util::EventLog().Log(e.what());

					return;
				}

				*LocalizerState = WidefieldImageProcessingStateType::Finished;
			};

			*WidefieldLocalizationState = WidefieldImageProcessingStateType::Waiting;

			if (ModuleData->GetAutoMeasureLocalizationType() == WidefieldMicroscopeWidget::LocalizationType::LocalizeEmittersFromImage)
				ModuleData->GetWidefieldLocalizer()->AnalyzeWidefield(ModuleData->GetCurrentImage(), CallbackFunc);
			else
				ModuleData->GetWidefieldLocalizer()->RecallPositions(ModuleData->GetCurrentImage(),
					ModuleData->GetCellID(), ModuleData->GetAutoMeasureSavePath().string(), CallbackFunc);
		}

		return StateType::WaitingForWidefieldLocalization;
	}

	void WidefieldMicroscope::PrepareAPDsForConfocalMode(Util::SynchronizedPointer<ModuleDataType>& ModuleData) const
	{
		ModuleData->GetSPD1()->SetStreamSize(1);
		ModuleData->GetSPD1()->SetExposureTime(ModuleData->GetSPDExposureTime());
		ModuleData->GetSPD1()->SetCoincidenceWindow(ModuleData->GetSPD1()->GetResolution());

		if (ModuleData->TestFeature(WidefieldMicroscopeData::FeatureType::HBT))
		{
			ModuleData->GetSPD2()->SetStreamSize(1);
			ModuleData->GetSPD2()->SetExposureTime(ModuleData->GetSPDExposureTime());
			ModuleData->GetSPD2()->SetCoincidenceWindow(ModuleData->GetSPD2()->GetResolution());
		}
	}

	void WidefieldMicroscope::InitializeConfocalOptimizer(Util::SynchronizedPointer<ModuleDataType>& ModuleData) const
	{
		ConfocalOptimizationNumStepsPerformed = 0;

		PrepareAPDsForConfocalMode(ModuleData);
		SetFocus(ModuleData, ModuleData->GetFocusZeroVoltage());
		auto SamplePosition = ModuleData->GetSamplePosition();

		gsl_vector_set(GSLConfocalOptimizationStepSize, 0,
			ModuleData->GetSampleStageX()->GetResolution() * ModuleData->GetConfocalOptimizationInitXYStepSize());
		gsl_vector_set(GSLConfocalOptimizationStepSize, 1,
			ModuleData->GetSampleStageY()->GetResolution() * ModuleData->GetConfocalOptimizationInitXYStepSize());
		gsl_vector_set(GSLConfocalOptimizationStepSize, 2,
			ModuleData->GetSampleFocusPiezoZ()->GetHardwareResolution() * ModuleData->GetConfocalOptimizationInitZStepSize());
		gsl_vector_set(GSLConfocalOptimizationInitialPoint, 0, SamplePosition.x);
		gsl_vector_set(GSLConfocalOptimizationInitialPoint, 1, SamplePosition.y);
		gsl_vector_set(GSLConfocalOptimizationInitialPoint, 2, ModuleData->GetFocusZeroVoltage());
	}

	void WidefieldMicroscope::SetHBTSwitch(Util::SynchronizedPointer<const ParamsType>& ModuleParams,
		Util::SynchronizedPointer<ModuleDataType>& ModuleData, bool IsHBTMode) const
	{
		ModuleData->GetWidefieldHBTSwitch()->Clear();
		ModuleData->GetWidefieldHBTSwitch()->SetRectFunction({ 1, 1, 0, 0,
			IsHBTMode ? ModuleParams->WidefieldHBTSwitchHighDutyCycle : ModuleParams->WidefieldHBTSwitchLowDutyCycle },
			false, true);
	}

	void WidefieldMicroscope::InitializeHBT(Util::SynchronizedPointer<ModuleDataType>& ModuleData) const
	{
		ModuleData->ClearHBTDataPoints();
		ModuleData->SetHBTSamplePosition(ModuleData->GetSamplePosition());

		auto TimeTagger = DynExp::dynamic_Object_cast<DynExpInstr::TimeTagger>(ModuleData->GetSPD1().get());
		TimeTagger->SetHBTActive(true);
		TimeTagger->ConfigureHBT(ModuleData->GetHBTBinWidth(), ModuleData->GetHBTBinCount());
		TimeTagger->ResetHBT();
		HBTIntegrationTimeBeforeReset = ModuleData->GetHBTTotalIntegrationTime();
	}

	void WidefieldMicroscope::StopHBT(Util::SynchronizedPointer<ModuleDataType>& ModuleData) const
	{
		if (ModuleData->TestFeature(WidefieldMicroscopeData::FeatureType::HBT))
		{
			auto SPD1 = DynExp::dynamic_Object_cast<DynExpInstr::TimeTagger>(ModuleData->GetSPD1().get());
			auto SPD1Data = DynExp::dynamic_InstrumentData_cast<DynExpInstr::TimeTagger>(SPD1->GetInstrumentData());
			auto SPD2Data = DynExp::dynamic_InstrumentData_cast<DynExpInstr::TimeTagger>(ModuleData->GetSPD2()->GetInstrumentData());
			SPD1->SetHBTActive(false);
			SPD1Data->ResetStreamMode();
			SPD2Data->ResetStreamMode();
		}
	}

	StateType WidefieldMicroscope::StartAutofocus(Util::SynchronizedPointer<ModuleDataType>& ModuleData) const
	{
		if (ModuleData->TestFeature(WidefieldMicroscopeData::FeatureType::WidefieldConfocalSwitch))
		{
			ModuleData->SetSetupMode(WidefieldMicroscopeData::SetupModeType::Widefield);

			if (!IsCharacterizingSample())
				StateMachine.SetContext(&AutofocusSetupTransitioningContext);

			return StateType::SetupTransitionBegin;
		}
		else
			return StateType::AutofocusBegin;
	}

	StateType WidefieldMicroscope::StartAutoMeasureLocalization(Util::SynchronizedPointer<ModuleDataType>& ModuleData) const
	{
		if (ModuleData->TestFeature(WidefieldMicroscopeData::FeatureType::InterModuleCommunicator))
			ModuleData->GetAcqCommunicator()->PostEvent(*this, SpectrumViewer::SetSilentModeEvent{ true });

		ModuleData->SetAutoMeasureCurrentCellPosition(ModuleData->GetSamplePosition());
		ModuleData->ResetAutoMeasureCurrentImageSet();
		WaitingEndTimePoint = std::chrono::system_clock::now() + ModuleData->GetAutoMeasureInitialImageSetWaitTime();

		LogUIMessagesOnly = true;

		StateMachine.SetContext(IsCharacterizingSample() ? &AutoMeasureSampleLocalizationContext : &AutoMeasureLocalizationContext);
		if (ModuleData->TestFeature(WidefieldMicroscopeData::FeatureType::WidefieldConfocalSwitch))
		{
			ModuleData->SetSetupMode(WidefieldMicroscopeData::SetupModeType::Widefield);

			return StateType::SetupTransitionBegin;
		}
		else
			return StateType::Waiting;
	}

	StateType WidefieldMicroscope::StartAutoMeasureCharacterization(Util::SynchronizedPointer<ModuleDataType>& ModuleData, Util::MarkerGraphicsView::MarkerType::IDType FirstEmitterID) const
	{
		if (FirstEmitterID < 0)
			ModuleData->ResetAutoMeasureCurrentEmitter();
		else
		{
			bool Success = ModuleData->SetAutoMeasureFirstEmitter(FirstEmitterID);
			if (!Success)
				return ResetState(ModuleData);
		}

		std::ranges::for_each(ModuleData->GetLocalizedPositions(), [](auto& Item) { Item.second.State = WidefieldMicroscopeData::LocalizedEmitterStateType::NotSet; });
		ModuleData->SetLocalizedPositionsStateChanged();
		ModuleData->SetAutoMeasureRunning(true);
		ModuleData->SetAutoMeasureCurrentCellPosition(ModuleData->GetSamplePosition());

		LogUIMessagesOnly = true;

		StateMachine.SetContext(IsCharacterizingSample() ? &AutoMeasureSampleCharacterizationContext : &AutoMeasureCharacterizationContext);
		if (ModuleData->TestFeature(WidefieldMicroscopeData::FeatureType::WidefieldConfocalSwitch))
		{
			ModuleData->SetSetupMode(WidefieldMicroscopeData::SetupModeType::Confocal);
			
			return StateType::SetupTransitionBegin;
		}
		else
			return StateType::AutoMeasureCharacterizationStep;
	}

	StateType WidefieldMicroscope::StartAutoMeasureSampleCharacterization(Util::SynchronizedPointer<ModuleDataType>& ModuleData) const
	{
		LogUIMessagesOnly = true;

		StateMachine.SetContext(&AutoMeasureSampleContext);

		return StateType::AutoMeasureSampleStep;
	}

	std::filesystem::path WidefieldMicroscope::BuildFilename(Util::SynchronizedPointer<ModuleDataType>& ModuleData, std::string_view FilenameSuffix) const
	{
		auto SavePath = ModuleData->GetAutoMeasureSavePath();
		SavePath.replace_filename(SavePath.filename().stem().concat(FilenameSuffix));
		std::filesystem::create_directories(SavePath.parent_path());

		return SavePath;
	}

	WidefieldMicroscope::ModuleDataType::PositionPoint WidefieldMicroscope::RandomPointInCircle(ModuleDataType::PositionType Radius) const
	{
		static std::random_device random_devce;
		static std::mt19937 random_generator(random_devce());
		static std::uniform_real_distribution<double> angle_random_distribution(0., 2. * std::numbers::pi);
		std::uniform_real_distribution<double> radius_random_distribution(0., Radius);

		const auto angle = angle_random_distribution(random_generator);
		const auto r = radius_random_distribution(random_generator);
		const auto x = Util::NumToT<ModuleDataType::PositionType>(r * std::cos(angle));
		const auto y = Util::NumToT<ModuleDataType::PositionType>(r * std::sin(angle));
		
		return { x, y };
	}

	void WidefieldMicroscope::ConfocalOptimizationInitPromises()
	{
		ConfocalOptimizationStatePromise = decltype(ConfocalOptimizationStatePromise)();
		ConfocalOptimizationStateFuture = ConfocalOptimizationStatePromise.get_future();
		ConfocalOptimizationFeedbackPromise = decltype(ConfocalOptimizationFeedbackPromise)();
		ConfocalOptimizationFeedbackFuture = ConfocalOptimizationFeedbackPromise.get_future();

		ConfocalOptimizationPromisesRenewed = true;
	}

	// Run gsl minimizer in its own thread since it calls a callback function to evaluate the result of the samples.
	// This can only be done in an async way interacting with the module thread since stages have to be moved etc.
	WidefieldMicroscope::ConfocalOptimizationThreadReturnType WidefieldMicroscope::ConfocalOptimizationThread(WidefieldMicroscope* Owner)
	{
		try
		{
			if (Owner->ConfocalOptimizationNumStepsPerformed == 0)
			{
				auto Status = gsl_multimin_fminimizer_set(Owner->GSLConfocalOptimizationState, &Owner->GSLConfocalOptimizationFuncDesc,
					Owner->GSLConfocalOptimizationInitialPoint, Owner->GSLConfocalOptimizationStepSize);

				return Status ? ConfocalOptimizationThreadReturnType::Failed : ConfocalOptimizationThreadReturnType::NextStep;
			}
			else
			{
				auto Status = gsl_multimin_fminimizer_iterate(Owner->GSLConfocalOptimizationState);
				if (Status)
					return ConfocalOptimizationThreadReturnType::Failed;

				auto Size = gsl_multimin_fminimizer_size(Owner->GSLConfocalOptimizationState);

				{
					auto ModuleData = DynExp::dynamic_ModuleData_cast<WidefieldMicroscope>(Owner->GetModuleData());

					Status = gsl_multimin_test_size(Size, ModuleData->GetConfocalOptimizationTolerance());
				} // ModuleData unlocked here.

				return Status == GSL_SUCCESS ? ConfocalOptimizationThreadReturnType::Finished :
					(Status == GSL_CONTINUE ? ConfocalOptimizationThreadReturnType::NextStep : ConfocalOptimizationThreadReturnType::Failed);
			}
		}
		catch (...)
		{
			return ConfocalOptimizationThreadReturnType::Failed;
		}
	}

	// This callback function can be evaluated by gsl minimizer multiple times per iteration!
	double WidefieldMicroscope::ConfocalOptimizationFuncForwarder(const gsl_vector* vector, void* params)
	{
		return static_cast<WidefieldMicroscope*>(params)->ConfocalOptimizationFunc(vector);
	}

	double WidefieldMicroscope::ConfocalOptimizationFunc(const gsl_vector* vector) noexcept
	{
		try
		{
			// ConfocalOptimizationPromisesRenewed must also be set to true if minimizer should abort...
			while (!ConfocalOptimizationPromisesRenewed)
				std::this_thread::yield();

			auto X = static_cast<decltype(ConfocalOptimizationStateType::X)>(gsl_vector_get(vector, 0));
			auto Y = static_cast<decltype(ConfocalOptimizationStateType::Y)>(gsl_vector_get(vector, 1));
			auto Z = static_cast<decltype(ConfocalOptimizationStateType::Z)>(gsl_vector_get(vector, 2));

			// ...in that case, prmoise might already contain a value. This will trigger a std::future_error
			// being caught (and swallowed) below.
			ConfocalOptimizationStatePromise.set_value({ X, Y, Z });

			if (!ConfocalOptimizationFeedbackFuture.valid())
				return GSL_NAN;

			// If ConfocalOptimizationFeedbackPromise is destroyed while waiting, std::future_error is thrown here.
			return ConfocalOptimizationFeedbackFuture.get();
		}
		catch (...)
		{
			return GSL_NAN;
		}
	}

	void WidefieldMicroscope::OnInit(DynExp::ModuleInstance* Instance) const
	{
		ImageViewer::ImageCapturingPausedEvent::Register(*this, &WidefieldMicroscope::OnImageCapturingPaused);
		ImageViewer::FinishedAutofocusEvent::Register(*this, &WidefieldMicroscope::OnFinishedAutofocus);
		SpectrumViewer::SpectrumFinishedRecordingEvent::Register(*this, &WidefieldMicroscope::OnSpectrumFinishedRecording);

		auto ModuleParams = DynExp::dynamic_Params_cast<WidefieldMicroscope>(Instance->ParamsGetter());
		auto ModuleData = DynExp::dynamic_ModuleData_cast<WidefieldMicroscope>(Instance->ModuleDataGetter());

		if (ModuleParams->AcqCommunicator.ContainsID())
		{
			Instance->LockObject(ModuleParams->AcqCommunicator, ModuleData->GetAcqCommunicator());
			ModuleData->SetFeature(WidefieldMicroscopeData::FeatureType::InterModuleCommunicator);
		}

		if (ModuleParams->WidefieldCamera.ContainsID())
		{
			Instance->LockObject(ModuleParams->WidefieldCamera, ModuleData->GetWidefieldCamera());
			ModuleData->SetFeature(WidefieldMicroscopeData::FeatureType::Widefield);

			{
				auto CameraData = DynExp::dynamic_InstrumentData_cast<DynExpInstr::Camera>(ModuleData->GetWidefieldCamera()->GetInstrumentData());
				ModuleData->SetMinCameraExposureTime(CameraData->GetMinExposureTime());
				ModuleData->SetMaxCameraExposureTime(CameraData->GetMaxExposureTime());
			} // CameraData unlocked here.

			if (ModuleParams->WidefieldLocalizer.ContainsID())
			{
				Instance->LockObject(ModuleParams->WidefieldLocalizer, ModuleData->GetWidefieldLocalizer());
				ModuleData->SetFeature(WidefieldMicroscopeData::FeatureType::WidefieldLocalization);
			}
		}

		if (ModuleParams->SampleStageX.ContainsID() && ModuleParams->SampleStageY.ContainsID())
		{
			Instance->LockObject(ModuleParams->SampleStageX, ModuleData->GetSampleStageX());
			Instance->LockObject(ModuleParams->SampleStageY, ModuleData->GetSampleStageY());

			ModuleData->SetFeature(WidefieldMicroscopeData::FeatureType::SampleXYPositioning);
			
			if (ModuleParams->SPD1.ContainsID())
			{
				Instance->LockObject(ModuleParams->SPD1, ModuleData->GetSPD1());
				ModuleData->SetFeature(WidefieldMicroscopeData::FeatureType::Confocal);
			}
		}

		if (ModuleParams->SampleStageZ.ContainsID())
		{
			Instance->LockObject(ModuleParams->SampleStageZ, ModuleData->GetSampleStageZ());
			ModuleData->SetFeature(WidefieldMicroscopeData::FeatureType::SampleZPositioning);
		}

		if (ModuleParams->FocusPiezoZ.ContainsID())
		{
			Instance->LockObject(ModuleParams->FocusPiezoZ, ModuleData->GetSampleFocusPiezoZ());

			if (ModuleData->TestFeature(WidefieldMicroscopeData::FeatureType::InterModuleCommunicator))
				ModuleData->SetFeature(WidefieldMicroscopeData::FeatureType::FocusAdjustment);
			if (ModuleData->TestFeature(WidefieldMicroscopeData::FeatureType::SampleXYPositioning))
				ModuleData->SetFeature(WidefieldMicroscopeData::FeatureType::ConfocalOptimization);

			ModuleData->SetMinFocusVoltage(ModuleData->GetSampleFocusPiezoZ()->GetUserMinValue());
			ModuleData->SetMaxFocusVoltage(ModuleData->GetSampleFocusPiezoZ()->GetUserMaxValue());
		}

		if (ModuleParams->SPD1.ContainsID() && ModuleParams->SPD2.ContainsID())
		{
			if (!ModuleData->TestFeature(WidefieldMicroscopeData::FeatureType::Confocal))
				Instance->LockObject(ModuleParams->SPD1, ModuleData->GetSPD1());
			Instance->LockObject(ModuleParams->SPD2, ModuleData->GetSPD2());

			ModuleData->SetFeature(WidefieldMicroscopeData::FeatureType::HBT);
		}

		if (ModuleParams->LEDSwitch.ContainsID())
		{
			Instance->LockObject(ModuleParams->LEDSwitch, ModuleData->GetLEDSwitch());
			ModuleData->SetFeature(WidefieldMicroscopeData::FeatureType::LEDLightToggle);

			ModuleData->SetLEDLightTurnedOn(false);
		}

		if (ModuleParams->PumpSwitch.ContainsID())
		{
			Instance->LockObject(ModuleParams->PumpSwitch, ModuleData->GetPumpSwitch());
			ModuleData->SetFeature(WidefieldMicroscopeData::FeatureType::PumpLightToggle);

			ModuleData->SetPumpLightTurnedOn(false);
		}

		if (ModuleParams->PumpPower.ContainsID())
		{
			Instance->LockObject(ModuleParams->PumpPower, ModuleData->GetPumpPower());
			ModuleData->SetFeature(WidefieldMicroscopeData::FeatureType::SetPumpPower);
			
			ModuleData->SetMinPumpPower(ModuleData->GetPumpPower()->GetUserMinValue());
			ModuleData->SetMaxPumpPower(ModuleData->GetPumpPower()->GetUserMaxValue());
			ModuleData->SetWidefieldPumpPower(ModuleParams->DefaultPowerWidefieldMode);
			ModuleData->SetConfocalPumpPower(ModuleParams->DefaultPowerConfocalMode);
		}

		if (ModuleParams->PumpPowerIndicator.ContainsID())
		{
			Instance->LockObject(ModuleParams->PumpPowerIndicator, ModuleData->GetPumpPowerIndicator());
			ModuleData->SetFeature(WidefieldMicroscopeData::FeatureType::MeasurePumpPower);
		}

		if (ModuleParams->WidefieldConfocalSwitch.ContainsID())
		{
			Instance->LockObject(ModuleParams->WidefieldConfocalSwitch, ModuleData->GetWidefieldConfocalSwitch());
			ModuleData->SetFeature(WidefieldMicroscopeData::FeatureType::WidefieldConfocalSwitch);

			if (ModuleParams->WidefieldConfocalIndicator.ContainsID())
			{
				Instance->LockObject(ModuleParams->WidefieldConfocalIndicator, ModuleData->GetWidefieldConfocalIndicator());
				ModuleData->SetFeature(WidefieldMicroscopeData::FeatureType::WidefieldConfocalIndicator);

				ModuleData->SetSetupMode(ModuleData->GetWidefieldConfocalIndicator()->GetSync() ? WidefieldMicroscopeData::SetupModeType::Confocal : WidefieldMicroscopeData::SetupModeType::Widefield);
			}
			else
				ModuleData->SetSetupMode(WidefieldMicroscopeData::SetupModeType::Widefield);
		}

		if (ModuleParams->WidefieldHBTSwitch.ContainsID())
		{
			Instance->LockObject(ModuleParams->WidefieldHBTSwitch, ModuleData->GetWidefieldHBTSwitch());
			ModuleData->SetFeature(WidefieldMicroscopeData::FeatureType::HBTSwitch);

			MakeAndEnqueueEvent(this, &WidefieldMicroscope::OnToggleHBTMirror, false);
		}
	}

	void WidefieldMicroscope::OnExit(DynExp::ModuleInstance* Instance) const
	{
		auto ModuleData = DynExp::dynamic_ModuleData_cast<WidefieldMicroscope>(Instance->ModuleDataGetter());

		OnStopAction(Instance, false);

		Instance->UnlockObject(ModuleData->GetAcqCommunicator());
		Instance->UnlockObject(ModuleData->GetSampleStageX());
		Instance->UnlockObject(ModuleData->GetSampleStageY());
		Instance->UnlockObject(ModuleData->GetSampleStageZ());
		Instance->UnlockObject(ModuleData->GetSampleFocusPiezoZ());
		Instance->UnlockObject(ModuleData->GetLEDSwitch());
		Instance->UnlockObject(ModuleData->GetPumpSwitch());
		Instance->UnlockObject(ModuleData->GetWidefieldConfocalSwitch());
		Instance->UnlockObject(ModuleData->GetWidefieldConfocalIndicator());
		Instance->UnlockObject(ModuleData->GetWidefieldHBTSwitch());
		Instance->UnlockObject(ModuleData->GetPumpPower());
		Instance->UnlockObject(ModuleData->GetPumpPowerIndicator());
		Instance->UnlockObject(ModuleData->GetWidefieldCamera());
		Instance->UnlockObject(ModuleData->GetWidefieldLocalizer());
		Instance->UnlockObject(ModuleData->GetSPD1());
		Instance->UnlockObject(ModuleData->GetSPD2());

		ImageViewer::ImageCapturingPausedEvent::Deregister(*this);
		ImageViewer::FinishedAutofocusEvent::Deregister(*this);
		SpectrumViewer::SpectrumFinishedRecordingEvent::Deregister(*this);
	}

	void WidefieldMicroscope::OnTerminate(DynExp::ModuleInstance* Instance, bool) const
	{
		Instance->Exit();
	}

	void WidefieldMicroscope::OnStopAction(DynExp::ModuleInstance* Instance, bool) const
	{
		auto ModuleData = DynExp::dynamic_ModuleData_cast<WidefieldMicroscope>(Instance->ModuleDataGetter());

		// Firstly, stop moving parts.
		if (ModuleData->TestFeature(WidefieldMicroscopeData::FeatureType::Confocal))
		{
			ModuleData->GetSampleStageX()->StopMotion();
			ModuleData->GetSampleStageY()->StopMotion();
		}

		if (ModuleData->TestFeature(WidefieldMicroscopeData::FeatureType::Widefield))
			ModuleData->GetWidefieldCamera()->StopCapturing();

		StopHBT(ModuleData);

		if (ConfocalOptimizationThreadReturnFuture.valid())
		{
			// Unblock ConfocalOptimizationThread (see above). Set ConfocalOptimizationPromisesRenewed to true
			// last to avoid ConfocalOptimizationFunc access ConfocalOptimizationFeedbackFuture while
			// ConfocalOptimizationFeedbackPromise is destroyed.
			ConfocalOptimizationFeedbackPromise = decltype(ConfocalOptimizationFeedbackPromise)();
			ConfocalOptimizationPromisesRenewed = true;

			// ...and wait for ConfocalOptimizationThread to terminate. This is important not to use involved
			// WidefieldMicroscope member variables by multiple running gsl optimization threads if a new
			// optimization is started directly after.
			ConfocalOptimizationThreadReturnFuture.wait();
		}

		if (ModuleData->IsAutoMeasureRunning())
		{
			if (ModuleData->GetAutoMeasureCurrentEmitter() != ModuleData->GetLocalizedPositions().cend())
			{
				ModuleData->GetAutoMeasureCurrentEmitter()->second.State = WidefieldMicroscopeData::LocalizedEmitterStateType::NotSet;
				ModuleData->SetLocalizedPositionsStateChanged();
			}

			ModuleData->SetAutoMeasureRunning(false);
		}

		StateMachine.SetCurrentState(ResetState(ModuleData));
	}

	void WidefieldMicroscope::OnSetHomePosition(DynExp::ModuleInstance* Instance, bool) const
	{
		auto ModuleData = DynExp::dynamic_ModuleData_cast<WidefieldMicroscope>(Instance->ModuleDataGetter());

		if (!IsReadyState() || !ModuleData->TestFeature(WidefieldMicroscopeData::FeatureType::Confocal))
			return;
		
		ModuleData->SetSampleHomePosition(ModuleData->GetSamplePosition());
	}

	void WidefieldMicroscope::OnGoToHomePosition(DynExp::ModuleInstance* Instance, bool) const
	{
		auto ModuleData = DynExp::dynamic_ModuleData_cast<WidefieldMicroscope>(Instance->ModuleDataGetter());

		if (!IsReadyState() || !ModuleData->TestFeature(WidefieldMicroscopeData::FeatureType::Confocal))
			return;

		MoveSampleTo(ModuleData->GetSampleHomePosition(), ModuleData);
	}

	void WidefieldMicroscope::OnToggleLEDLightSource(DynExp::ModuleInstance* Instance, bool State) const
	{
		auto ModuleData = DynExp::dynamic_ModuleData_cast<WidefieldMicroscope>(Instance->ModuleDataGetter());

		if (!ModuleData->TestFeature(WidefieldMicroscopeData::FeatureType::LEDLightToggle))
			return;

		ModuleData->SetLEDLightTurnedOn(State);
	}

	void WidefieldMicroscope::OnTogglePumpLightSource(DynExp::ModuleInstance* Instance, bool State) const
	{
		auto ModuleData = DynExp::dynamic_ModuleData_cast<WidefieldMicroscope>(Instance->ModuleDataGetter());

		if (!ModuleData->TestFeature(WidefieldMicroscopeData::FeatureType::PumpLightToggle))
			return;

		ModuleData->SetPumpLightTurnedOn(State);
	}

	void WidefieldMicroscope::OnSetupModeChanged(DynExp::ModuleInstance* Instance, QAction* Action) const
	{
		auto ModuleData = DynExp::dynamic_ModuleData_cast<WidefieldMicroscope>(Instance->ModuleDataGetter());

		if (!IsReadyState() || !ModuleData->TestFeature(WidefieldMicroscopeData::FeatureType::WidefieldConfocalSwitch))
			return;

		if (Action == GetWidget<WidefieldMicroscopeWidget>()->GetUI().action_Widefield_mode)
			ModuleData->SetSetupMode(WidefieldMicroscopeData::SetupModeType::Widefield);
		else if (Action == GetWidget<WidefieldMicroscopeWidget>()->GetUI().action_Confocal_mode)
			ModuleData->SetSetupMode(WidefieldMicroscopeData::SetupModeType::Confocal);
		else
			return;

		StateMachine.SetCurrentState(StateType::SetupTransitionBegin);
	}

	void WidefieldMicroscope::OnAutofocus(DynExp::ModuleInstance* Instance, bool) const
	{
		auto ModuleData = DynExp::dynamic_ModuleData_cast<WidefieldMicroscope>(Instance->ModuleDataGetter());

		if (!IsReadyState() || !ModuleData->TestFeature(WidefieldMicroscopeData::FeatureType::FocusAdjustment))
			return;

		StateMachine.SetCurrentState(StartAutofocus(ModuleData));
	}

	void WidefieldMicroscope::OnOptimizePositions(DynExp::ModuleInstance* Instance, bool) const
	{
		auto ModuleData = DynExp::dynamic_ModuleData_cast<WidefieldMicroscope>(Instance->ModuleDataGetter());

		if (!IsReadyState() || !ModuleData->TestFeature(WidefieldMicroscopeData::FeatureType::ConfocalOptimization)
			|| ModuleData->GetSetupMode() != WidefieldMicroscopeData::SetupModeType::Confocal)
			return;

		InitializeConfocalOptimizer(ModuleData);

		// To move the positioners and to record the count rate, state functions from confocal scanning are used.
		StateMachine.SetContext(&ConfocalOptimizationContext);
		StateMachine.SetCurrentState(StateType::ConfocalOptimizationInit);
	}

	void WidefieldMicroscope::OnToggleHBTMirror(DynExp::ModuleInstance* Instance, bool Checked) const
	{
		auto ModuleParams = DynExp::dynamic_Params_cast<WidefieldMicroscope>(Instance->ParamsGetter());
		auto ModuleData = DynExp::dynamic_ModuleData_cast<WidefieldMicroscope>(Instance->ModuleDataGetter());

		if (!IsReadyState() || !ModuleData->TestFeature(WidefieldMicroscopeData::FeatureType::HBTSwitch))
			return;

		SetHBTSwitch(ModuleParams, ModuleData, Checked);
	}

	void WidefieldMicroscope::OnResetCellID(DynExp::ModuleInstance* Instance, bool) const
	{
		if (!IsReadyState())
			return;

		auto ModuleData = DynExp::dynamic_ModuleData_cast<WidefieldMicroscope>(Instance->ModuleDataGetter());
		ModuleData->ResetCellID();
	}

	void WidefieldMicroscope::OnGeneralWidefieldPowerChanged(DynExp::ModuleInstance* Instance, double Value) const
	{
		auto ModuleData = DynExp::dynamic_ModuleData_cast<WidefieldMicroscope>(Instance->ModuleDataGetter());
		ModuleData->SetWidefieldPumpPower(Value);

		UpdatePumpPower(ModuleData);
	}

	void WidefieldMicroscope::OnGeneralConfocalPowerChanged(DynExp::ModuleInstance* Instance, double Value) const
	{
		auto ModuleData = DynExp::dynamic_ModuleData_cast<WidefieldMicroscope>(Instance->ModuleDataGetter());
		ModuleData->SetConfocalPumpPower(Value);

		UpdatePumpPower(ModuleData);
	}

	void WidefieldMicroscope::OnGeneralFocusCurrentVoltageChanged(DynExp::ModuleInstance* Instance, double Value) const
	{
		if (!IsReadyState())
			return;

		auto ModuleData = DynExp::dynamic_ModuleData_cast<WidefieldMicroscope>(Instance->ModuleDataGetter());
		SetFocus(ModuleData, Value);
	}

	void WidefieldMicroscope::OnGeneralFocusZeroVoltageChanged(DynExp::ModuleInstance* Instance, double Value) const
	{
		auto ModuleData = DynExp::dynamic_ModuleData_cast<WidefieldMicroscope>(Instance->ModuleDataGetter());
		ModuleData->SetFocusZeroVoltage(Value);
	}

	void WidefieldMicroscope::OnGeneralFocusConfocalOffsetVoltageChanged(DynExp::ModuleInstance* Instance, double Value) const
	{
		auto ModuleData = DynExp::dynamic_ModuleData_cast<WidefieldMicroscope>(Instance->ModuleDataGetter());
		ModuleData->SetFocusConfocalOffsetVoltage(Value);

		if (IsReadyState() && ModuleData->GetSetupMode() == WidefieldMicroscopeData::SetupModeType::Confocal)
			SetFocus(ModuleData, ModuleData->GetFocusCurrentVoltage());
	}

	void WidefieldMicroscope::OnGeneralSetZeroFocus(DynExp::ModuleInstance* Instance, bool) const
	{
		auto ModuleData = DynExp::dynamic_ModuleData_cast<WidefieldMicroscope>(Instance->ModuleDataGetter());
		ModuleData->SetFocusZeroVoltage(ModuleData->GetFocusCurrentVoltage());
	}

	void WidefieldMicroscope::OnGeneralApplyZeroFocus(DynExp::ModuleInstance* Instance, bool) const
	{
		if (!IsReadyState())
			return;

		auto ModuleData = DynExp::dynamic_ModuleData_cast<WidefieldMicroscope>(Instance->ModuleDataGetter());
		SetFocus(ModuleData, ModuleData->GetFocusZeroVoltage());
	}

	void WidefieldMicroscope::OnWidefieldLEDExposureTimeChanged(DynExp::ModuleInstance* Instance, int Value) const
	{
		auto ModuleData = DynExp::dynamic_ModuleData_cast<WidefieldMicroscope>(Instance->ModuleDataGetter());
		ModuleData->SetLEDCameraExposureTime(std::chrono::milliseconds(static_cast<std::chrono::milliseconds::rep>(Value)));
	}

	void WidefieldMicroscope::OnWidefieldApplyLEDExposureTime(DynExp::ModuleInstance* Instance, bool) const
	{
		auto ModuleData = DynExp::dynamic_ModuleData_cast<WidefieldMicroscope>(Instance->ModuleDataGetter());

		if (!IsReadyState() || !ModuleData->TestFeature(WidefieldMicroscopeData::FeatureType::Widefield))
			return;

		if (ModuleData->GetWidefieldCamera()->CanSetExposureTime())
			ModuleData->GetWidefieldCamera()->SetExposureTime(ModuleData->GetLEDCameraExposureTime());
	}

	void WidefieldMicroscope::OnWidefieldPumpExposureTimeChanged(DynExp::ModuleInstance* Instance, int Value) const
	{
		auto ModuleData = DynExp::dynamic_ModuleData_cast<WidefieldMicroscope>(Instance->ModuleDataGetter());
		ModuleData->SetWidefieldCameraExposureTime(std::chrono::milliseconds(static_cast<std::chrono::milliseconds::rep>(Value)));
	}

	void WidefieldMicroscope::OnWidefieldApplyPumpExposureTime(DynExp::ModuleInstance* Instance, bool) const
	{
		auto ModuleData = DynExp::dynamic_ModuleData_cast<WidefieldMicroscope>(Instance->ModuleDataGetter());

		if (!IsReadyState() || !ModuleData->TestFeature(WidefieldMicroscopeData::FeatureType::Widefield))
			return;

		if (ModuleData->GetWidefieldCamera()->CanSetExposureTime())
			ModuleData->GetWidefieldCamera()->SetExposureTime(ModuleData->GetWidefieldCameraExposureTime());
	}

	void WidefieldMicroscope::OnWidefieldFindConfocalSpot(DynExp::ModuleInstance* Instance, bool) const
	{
		auto ModuleData = DynExp::dynamic_ModuleData_cast<WidefieldMicroscope>(Instance->ModuleDataGetter());

		if (!IsReadyState() || !ModuleData->TestFeature(WidefieldMicroscopeData::FeatureType::Widefield)
			|| !ModuleData->TestFeature(WidefieldMicroscopeData::FeatureType::WidefieldConfocalSwitch))
			return;

		StateMachine.SetCurrentState(StateType::FindingConfocalSpotBegin);
	}

	void WidefieldMicroscope::OnCaptureLEDImage(DynExp::ModuleInstance* Instance, bool) const
	{
		auto ModuleData = DynExp::dynamic_ModuleData_cast<WidefieldMicroscope>(Instance->ModuleDataGetter());

		if (!IsReadyState() || !ModuleData->TestFeature(WidefieldMicroscopeData::FeatureType::Widefield))
			return;

		if (ModuleData->TestFeature(WidefieldMicroscopeData::FeatureType::WidefieldConfocalSwitch))
		{
			ModuleData->SetSetupMode(WidefieldMicroscopeData::SetupModeType::Widefield);

			StateMachine.SetContext(&LEDImageAcquisitionSetupTransitioningContext);
			StateMachine.SetCurrentState(StateType::SetupTransitionBegin);
		}
		else
			StateMachine.SetCurrentState(StateType::LEDImageAcquisitionBegin);
	}

	void WidefieldMicroscope::OnCaptureWidefieldImage(DynExp::ModuleInstance* Instance, bool) const
	{
		auto ModuleData = DynExp::dynamic_ModuleData_cast<WidefieldMicroscope>(Instance->ModuleDataGetter());

		if (!IsReadyState() || !ModuleData->TestFeature(WidefieldMicroscopeData::FeatureType::Widefield))
			return;

		if (ModuleData->TestFeature(WidefieldMicroscopeData::FeatureType::WidefieldConfocalSwitch))
		{
			ModuleData->SetSetupMode(WidefieldMicroscopeData::SetupModeType::Widefield);

			StateMachine.SetContext(&WidefieldImageAcquisitionSetupTransitioningContext);
			StateMachine.SetCurrentState(StateType::SetupTransitionBegin);
		}
		else
			StateMachine.SetCurrentState(StateType::WidefieldImageAcquisitionBegin);
	}

	void WidefieldMicroscope::OnWidefieldReadCellID(DynExp::ModuleInstance* Instance, bool) const
	{
		auto ModuleData = DynExp::dynamic_ModuleData_cast<WidefieldMicroscope>(Instance->ModuleDataGetter());

		if (!IsReadyState() || !ModuleData->TestFeature(WidefieldMicroscopeData::FeatureType::WidefieldLocalization))
			return;

		StateMachine.SetCurrentState(InitiateReadCellIDFromImage(ModuleData));
	}

	void WidefieldMicroscope::OnWidefieldAnalyzeImageDistortion(DynExp::ModuleInstance* Instance, bool) const
	{
		auto ModuleData = DynExp::dynamic_ModuleData_cast<WidefieldMicroscope>(Instance->ModuleDataGetter());

		if (!IsReadyState() || !ModuleData->TestFeature(WidefieldMicroscopeData::FeatureType::WidefieldLocalization) || ModuleData->GetCurrentImage().isNull())
			return;

		ModuleData->GetWidefieldLocalizer()->AnalyzeDistortion(ModuleData->GetCurrentImage());
	}

	void WidefieldMicroscope::OnWidefieldLocalizeEmitters(DynExp::ModuleInstance* Instance, bool) const
	{
		auto ModuleData = DynExp::dynamic_ModuleData_cast<WidefieldMicroscope>(Instance->ModuleDataGetter());

		if (!IsReadyState() || !ModuleData->TestFeature(WidefieldMicroscopeData::FeatureType::WidefieldLocalization))
			return;

		StateMachine.SetCurrentState(InitiateLocalizationFromImage(ModuleData));
	}

	void WidefieldMicroscope::OnWidefieldImageClicked(DynExp::ModuleInstance* Instance, QPoint Position) const
	{
		auto ModuleData = DynExp::dynamic_ModuleData_cast<WidefieldMicroscope>(Instance->ModuleDataGetter());
		
		// The rest is tested in OnBringMarkerToConfocalSpot().
		if (ModuleData->GetWidefieldPosition().IsEmpty())
			return;
		
		OnBringMarkerToConfocalSpot(Instance, Position, { static_cast<qreal>(ModuleData->GetWidefieldPosition().x), static_cast<qreal>(ModuleData->GetWidefieldPosition().y) });
	}

	void WidefieldMicroscope::OnConfocalConfocalWidthChanged(DynExp::ModuleInstance* Instance, int Value) const
	{
		auto ModuleData = DynExp::dynamic_ModuleData_cast<WidefieldMicroscope>(Instance->ModuleDataGetter());
		ModuleData->SetConfocalScanWidth(Value);
	}

	void WidefieldMicroscope::OnConfocalConfocalHeightChanged(DynExp::ModuleInstance* Instance, int Value) const
	{
		auto ModuleData = DynExp::dynamic_ModuleData_cast<WidefieldMicroscope>(Instance->ModuleDataGetter());
		ModuleData->SetConfocalScanHeight(Value);
	}

	void WidefieldMicroscope::OnConfocalConfocalDistPerPixelChanged(DynExp::ModuleInstance* Instance, int Value) const
	{
		auto ModuleData = DynExp::dynamic_ModuleData_cast<WidefieldMicroscope>(Instance->ModuleDataGetter());
		ModuleData->SetConfocalScanDistPerPixel(Value);
	}

	void WidefieldMicroscope::OnConfocalSPDExposureTimeChanged(DynExp::ModuleInstance* Instance, int Value) const
	{
		auto ModuleData = DynExp::dynamic_ModuleData_cast<WidefieldMicroscope>(Instance->ModuleDataGetter());
		ModuleData->SetSPDExposureTime(std::chrono::milliseconds(Value));
	}

	void WidefieldMicroscope::OnConfocalOptimizationInitXYStepSizeChanged(DynExp::ModuleInstance* Instance, double Value) const
	{
		auto ModuleData = DynExp::dynamic_ModuleData_cast<WidefieldMicroscope>(Instance->ModuleDataGetter());
		ModuleData->SetConfocalOptimizationInitXYStepSize(Value);
	}

	void WidefieldMicroscope::OnConfocalOptimizationInitZStepSizeChanged(DynExp::ModuleInstance* Instance, double Value) const
	{
		auto ModuleData = DynExp::dynamic_ModuleData_cast<WidefieldMicroscope>(Instance->ModuleDataGetter());
		ModuleData->SetConfocalOptimizationInitZStepSize(Value);
	}

	void WidefieldMicroscope::OnConfocalOptimizationToleranceChanged(DynExp::ModuleInstance* Instance, double Value) const
	{
		auto ModuleData = DynExp::dynamic_ModuleData_cast<WidefieldMicroscope>(Instance->ModuleDataGetter());
		ModuleData->SetConfocalOptimizationTolerance(Value);
	}

	void WidefieldMicroscope::OnPerformConfocalScan(DynExp::ModuleInstance* Instance, bool) const
	{
		int Width{}, Height{}, DistPerPixel{};
		WidefieldMicroscopeData::PositionPoint CenterPosition{ 0, 0 };

		OnGoToHomePosition(Instance, false);

		{
			auto ModuleData = DynExp::dynamic_ModuleData_cast<WidefieldMicroscope>(Instance->ModuleDataGetter());

			if (!IsReadyState() || !ModuleData->TestFeature(WidefieldMicroscopeData::FeatureType::Confocal))
				return;

			CenterPosition = ModuleData->GetSamplePosition();

			Width = ModuleData->GetConfocalScanWidth();
			Height = ModuleData->GetConfocalScanHeight();
			DistPerPixel = ModuleData->GetConfocalScanDistPerPixel();

			PrepareAPDsForConfocalMode(ModuleData);

			ModuleData->ClearConfocalScanResults();

		} // ModuleData unlocked here.

		// ModuleData unlocked for this call, because it is potentially heavy.
		auto SurfaceDataRows = CalculateConfocalScanPositions(Width, Height, DistPerPixel, CenterPosition);

		auto ModuleData = DynExp::dynamic_ModuleData_cast<WidefieldMicroscope>(Instance->ModuleDataGetter());
		ModuleData->SetConfocalScanSurfacePlotRows(std::move(SurfaceDataRows));

		StateMachine.SetCurrentState(StateType::ConfocalScanStep);
	}

	void WidefieldMicroscope::ConfocalSurfaceSelectedPointChanged(DynExp::ModuleInstance* Instance, QPoint Position) const
	{
		auto ModuleData = DynExp::dynamic_ModuleData_cast<WidefieldMicroscope>(Instance->ModuleDataGetter());

		if (!IsReadyState() || !ModuleData->TestFeature(WidefieldMicroscopeData::FeatureType::Confocal))
			return;

		const auto& Results = ModuleData->GetConfocalScanResults();
		auto ResultItemIt = std::find_if(Results.cbegin(), Results.cend(), [&Position](const std::pair<WidefieldMicroscopeData::PositionPoint, double>& ResultItem) {
			return ResultItem.first.RowIndex == Position.x() && ResultItem.first.ColumnIndex == Position.y();
		});

		if (ResultItemIt != Results.cend())
			MoveSampleTo({ ResultItemIt->first.MeasuredX, ResultItemIt->first.MeasuredY }, ModuleData);
	}

	void WidefieldMicroscope::OnHBTBinWidthChanged(DynExp::ModuleInstance* Instance, int Value) const
	{
		auto ModuleData = DynExp::dynamic_ModuleData_cast<WidefieldMicroscope>(Instance->ModuleDataGetter());
		ModuleData->SetHBTBinWidth(Util::picoseconds(static_cast<Util::picoseconds::rep>(Value)));
	}

	void WidefieldMicroscope::OnHBTBinCountChanged(DynExp::ModuleInstance* Instance, int Value) const
	{
		auto ModuleData = DynExp::dynamic_ModuleData_cast<WidefieldMicroscope>(Instance->ModuleDataGetter());
		ModuleData->SetHBTBinCount(Value);
	}

	void WidefieldMicroscope::OnHHBTMaxIntegrationTimeChanged(DynExp::ModuleInstance* Instance, double Value) const
	{
		auto ModuleData = DynExp::dynamic_ModuleData_cast<WidefieldMicroscope>(Instance->ModuleDataGetter());
		ModuleData->SetHBTMaxIntegrationTime(std::chrono::microseconds(Util::NumToT<std::chrono::microseconds::rep>(Value * std::chrono::microseconds::period::den)));
	}

	void WidefieldMicroscope::OnMeasureHBT(DynExp::ModuleInstance* Instance, bool) const
	{
		auto ModuleData = DynExp::dynamic_ModuleData_cast<WidefieldMicroscope>(Instance->ModuleDataGetter());

		if (!IsReadyState() || !ModuleData->TestFeature(WidefieldMicroscopeData::FeatureType::HBT))
			return;

		InitializeHBT(ModuleData);

		StateMachine.SetCurrentState(StateType::HBTAcquiring);
	}

	void WidefieldMicroscope::OnImageCapturingPaused(DynExp::ModuleInstance* Instance) const
	{
		ImageCapturingPaused = true;
	}

	void WidefieldMicroscope::OnFinishedAutofocus(DynExp::ModuleInstance* Instance, bool Success, double Voltage) const
	{
		static constexpr const char* AutofocusFailedErrorMsg = "Autofocusing failed!";

		if (StateMachine.GetCurrentState()->GetState() != StateType::AutofocusWaiting)
			return;

		auto ModuleData = DynExp::dynamic_ModuleData_cast<WidefieldMicroscope>(Instance->ModuleDataGetter());
		ModuleData->SetAutofocusFinished();

		if (Success)
		{
			Voltage -= ModuleData->GetSetupMode() == WidefieldMicroscopeData::SetupModeType::Confocal ? ModuleData->GetFocusConfocalOffsetVoltage() : .0;

			SetFocus(ModuleData, Voltage);
			ModuleData->SetFocusZeroVoltage(Voltage);
		}
		else
		{
			if (LogUIMessagesOnly)
				Util::EventLogger().Log(AutofocusFailedErrorMsg, Util::ErrorType::Error);
			else
			{
				auto ModuleData = DynExp::dynamic_ModuleData_cast<WidefieldMicroscope>(Instance->ModuleDataGetter());
				ModuleData->SetUIMessage(AutofocusFailedErrorMsg);
			}
		}

		if (ModuleData->TestFeature(WidefieldMicroscopeData::FeatureType::LEDLightToggle))
			ModuleData->SetLEDLightTurnedOn(false);
	}

	void WidefieldMicroscope::OnSpectrumFinishedRecording(DynExp::ModuleInstance* Instance) const
	{
		StateMachine.SetCurrentState(StateType::SpectrumAcquisitionFinished);
	}

	void WidefieldMicroscope::OnAutoMeasureSavePathChanged(DynExp::ModuleInstance* Instance, QString Path) const
	{
		auto ModuleData = DynExp::dynamic_ModuleData_cast<WidefieldMicroscope>(Instance->ModuleDataGetter());
		ModuleData->SetAutoMeasureSavePath(Path.toStdString());
	}

	void WidefieldMicroscope::OnAutoMeasureNumberImageSetsChanged(DynExp::ModuleInstance* Instance, int Value) const
	{
		auto ModuleData = DynExp::dynamic_ModuleData_cast<WidefieldMicroscope>(Instance->ModuleDataGetter());
		ModuleData->SetAutoMeasureNumberImageSets(Value);
	}

	void WidefieldMicroscope::OnAutoMeasureInitialImageSetWaitTimeChanged(DynExp::ModuleInstance* Instance, int Value) const
	{
		auto ModuleData = DynExp::dynamic_ModuleData_cast<WidefieldMicroscope>(Instance->ModuleDataGetter());
		ModuleData->SetAutoMeasureInitialImageSetWaitTime(std::chrono::seconds(static_cast<std::chrono::seconds::rep>(Value)));
	}

	void WidefieldMicroscope::OnAutoMeasureImagePositionScatterRadius(DynExp::ModuleInstance* Instance, int Value) const
	{
		auto ModuleData = DynExp::dynamic_ModuleData_cast<WidefieldMicroscope>(Instance->ModuleDataGetter());
		ModuleData->SetAutoMeasureImagePositionScatterRadius(Value);
	}

	void WidefieldMicroscope::OnAutoMeasureLocalizationTypeChanged(DynExp::ModuleInstance* Instance, int Value) const
	{
		auto ModuleData = DynExp::dynamic_ModuleData_cast<WidefieldMicroscope>(Instance->ModuleDataGetter());
		ModuleData->SetAutoMeasureLocalizationType(static_cast<WidefieldMicroscopeWidget::LocalizationType>(Value));
	}

	void WidefieldMicroscope::OnToggleAutoMeasureOptimizeEnabled(DynExp::ModuleInstance* Instance, int State) const
	{
		auto ModuleData = DynExp::dynamic_ModuleData_cast<WidefieldMicroscope>(Instance->ModuleDataGetter());
		ModuleData->SetAutoMeasureOptimizeEnabled(State);
	}

	void WidefieldMicroscope::OnToggleAutoMeasureSpectrumEnabled(DynExp::ModuleInstance* Instance, int State) const
	{
		auto ModuleData = DynExp::dynamic_ModuleData_cast<WidefieldMicroscope>(Instance->ModuleDataGetter());
		ModuleData->SetAutoMeasureSpectrumEnabled(State);
	}

	void WidefieldMicroscope::OnToggleAutoMeasureHBTEnabled(DynExp::ModuleInstance* Instance, int State) const
	{
		auto ModuleData = DynExp::dynamic_ModuleData_cast<WidefieldMicroscope>(Instance->ModuleDataGetter());
		ModuleData->SetAutoMeasureHBTEnabled(State);
	}

	void WidefieldMicroscope::OnAutoMeasureNumOptimizationAttemptsChanged(DynExp::ModuleInstance* Instance, int Value) const
	{
		auto ModuleData = DynExp::dynamic_ModuleData_cast<WidefieldMicroscope>(Instance->ModuleDataGetter());
		ModuleData->SetAutoMeasureNumOptimizationAttempts(Value);
	}

	void WidefieldMicroscope::OnAutoMeasureMaxOptimizationRerunsChanged(DynExp::ModuleInstance* Instance, int Value) const
	{
		auto ModuleData = DynExp::dynamic_ModuleData_cast<WidefieldMicroscope>(Instance->ModuleDataGetter());
		ModuleData->SetAutoMeasureMaxOptimizationReruns(Value);
	}

	void WidefieldMicroscope::OnAutoMeasureOptimizationMaxDistanceChanged(DynExp::ModuleInstance* Instance, int Value) const
	{
		auto ModuleData = DynExp::dynamic_ModuleData_cast<WidefieldMicroscope>(Instance->ModuleDataGetter());
		ModuleData->SetAutoMeasureOptimizationMaxDistance(Value);
	}

	void WidefieldMicroscope::OnAutoMeasureCountRateThresholdChanged(DynExp::ModuleInstance* Instance, int Value) const
	{
		auto ModuleData = DynExp::dynamic_ModuleData_cast<WidefieldMicroscope>(Instance->ModuleDataGetter());
		ModuleData->SetAutoMeasureCountRateThreshold(Value);
	}

	void WidefieldMicroscope::OnAutoMeasureCellRangeFromXChanged(DynExp::ModuleInstance* Instance, int Value) const
	{
		auto ModuleData = DynExp::dynamic_ModuleData_cast<WidefieldMicroscope>(Instance->ModuleDataGetter());
		ModuleData->GetAutoMeasureCellRangeFrom().setX(Value);
	}

	void WidefieldMicroscope::OnAutoMeasureCellRangeFromYChanged(DynExp::ModuleInstance* Instance, int Value) const
	{
		auto ModuleData = DynExp::dynamic_ModuleData_cast<WidefieldMicroscope>(Instance->ModuleDataGetter());
		ModuleData->GetAutoMeasureCellRangeFrom().setY(Value);
	}

	void WidefieldMicroscope::OnAutoMeasureCellRangeToXChanged(DynExp::ModuleInstance* Instance, int Value) const
	{
		auto ModuleData = DynExp::dynamic_ModuleData_cast<WidefieldMicroscope>(Instance->ModuleDataGetter());
		ModuleData->GetAutoMeasureCellRangeTo().setX(Value);
	}

	void WidefieldMicroscope::OnAutoMeasureCellRangeToYChanged(DynExp::ModuleInstance* Instance, int Value) const
	{
		auto ModuleData = DynExp::dynamic_ModuleData_cast<WidefieldMicroscope>(Instance->ModuleDataGetter());
		ModuleData->GetAutoMeasureCellRangeTo().setY(Value);
	}

	void WidefieldMicroscope::OnAutoMeasureCellSkipXChanged(DynExp::ModuleInstance* Instance, int Value) const
	{
		auto ModuleData = DynExp::dynamic_ModuleData_cast<WidefieldMicroscope>(Instance->ModuleDataGetter());
		ModuleData->GetAutoMeasureCellSkip().setX(Value);
	}

	void WidefieldMicroscope::OnAutoMeasureCellSkipYChanged(DynExp::ModuleInstance* Instance, int Value) const
	{
		auto ModuleData = DynExp::dynamic_ModuleData_cast<WidefieldMicroscope>(Instance->ModuleDataGetter());
		ModuleData->GetAutoMeasureCellSkip().setY(Value);
	}

	void WidefieldMicroscope::OnAutoMeasureRunLocalization(DynExp::ModuleInstance* Instance, bool) const
	{
		auto ModuleData = DynExp::dynamic_ModuleData_cast<WidefieldMicroscope>(Instance->ModuleDataGetter());

		if (!IsReadyState() || !ModuleData->TestFeature(WidefieldMicroscopeData::FeatureType::Widefield))
			return;

		StateMachine.SetCurrentState(StartAutoMeasureLocalization(ModuleData));
	}

	void WidefieldMicroscope::OnAutoMeasureRunCharacterization(DynExp::ModuleInstance* Instance, bool) const
	{
		auto ModuleData = DynExp::dynamic_ModuleData_cast<WidefieldMicroscope>(Instance->ModuleDataGetter());

		if (!IsReadyState() || !ModuleData->TestFeature(WidefieldMicroscopeData::FeatureType::Confocal) ||
			ModuleData->GetLocalizedPositions().empty() || ModuleData->GetWidefieldPosition().IsEmpty())
			return;

		StateMachine.SetCurrentState(StartAutoMeasureCharacterization(ModuleData));
	}

	void WidefieldMicroscope::OnAutoMeasureRunSampleCharacterization(DynExp::ModuleInstance* Instance, bool) const
	{
		auto ModuleData = DynExp::dynamic_ModuleData_cast<WidefieldMicroscope>(Instance->ModuleDataGetter());

		if (!IsReadyState() || !ModuleData->TestFeature(WidefieldMicroscopeData::FeatureType::Widefield) ||
			!ModuleData->TestFeature(WidefieldMicroscopeData::FeatureType::Confocal) ||
			!ModuleData->TestFeature(WidefieldMicroscopeData::FeatureType::WidefieldLocalization))
			return;

		if (!ModuleData->IsCellRangeValid())
		{
			ModuleData->SetUIMessage("Please specify a valid cell range.");

			return;
		}

		// Required to especially reset WidefieldMicroscopeData::LastCellID, which is needed to determine
		// the direction of advancing from cell to cell.
		ModuleData->ResetCellID();

		StateMachine.SetCurrentState(StartAutoMeasureSampleCharacterization(ModuleData));
	}

	StateType WidefieldMicroscope::ReturnToReadyStateFunc(DynExp::ModuleInstance& Instance)
	{
		auto ModuleData = DynExp::dynamic_ModuleData_cast<WidefieldMicroscope>(Instance.ModuleDataGetter());

		// This state may be replaced by a state machine context.
		// Do not perform any complex task here, just transition back to the ready state.
		return ResetState(ModuleData);
	}

	StateType WidefieldMicroscope::InitializingStateFunc(DynExp::ModuleInstance& Instance)
	{
		auto ModuleData = DynExp::dynamic_ModuleData_cast<WidefieldMicroscope>(Instance.ModuleDataGetter());

		ResetState(ModuleData);

		if (ModuleData->TestFeature(WidefieldMicroscopeData::FeatureType::WidefieldConfocalSwitch))
			return StateType::SetupTransitionBegin;

		return StateType::Ready;
	}

	StateType WidefieldMicroscope::SetupTransitionBeginStateFunc(DynExp::ModuleInstance& Instance)
	{
		{
			auto ModuleParams = DynExp::dynamic_Params_cast<WidefieldMicroscope>(Instance.ParamsGetter());
			SetupTransitionFinishedTimePoint = std::chrono::system_clock::now() + std::chrono::milliseconds(ModuleParams->WidefieldConfocalTransitionTime);
		} // ModuleParams unlocked here.

		auto ModuleData = DynExp::dynamic_ModuleData_cast<WidefieldMicroscope>(Instance.ModuleDataGetter());

		if (ModuleData->GetSetupMode() == WidefieldMicroscopeData::SetupModeType::Unknown)
		{
			ModuleData->SetUIMessage("Transitioning failed due to unknown destiny setup mode!");

			return ResetState(ModuleData);
		}

		if (ModuleData->TestFeature(WidefieldMicroscopeData::FeatureType::PumpLightToggle))
		{
			TurnOnPumpSourceAfterTransitioning = ModuleData->GetPumpLightTurnedOn();
			ModuleData->SetPumpLightTurnedOn(false);
		}
		else
			TurnOnPumpSourceAfterTransitioning = false;

		if (ModuleData->GetSetupMode() == WidefieldMicroscopeData::SetupModeType::Widefield)
			ModuleData->GetWidefieldConfocalSwitch()->Set(false);
		else if (ModuleData->GetSetupMode() == WidefieldMicroscopeData::SetupModeType::Confocal)
			ModuleData->GetWidefieldConfocalSwitch()->Set(true);

		return StateType::SetupTransitioning;
	}

	StateType WidefieldMicroscope::SetupTransitioningStateFunc(DynExp::ModuleInstance& Instance)
	{
		return std::chrono::system_clock::now() >= SetupTransitionFinishedTimePoint ? StateType::SetupTransitionEnd : StateType::SetupTransitioning;
	}

	StateType WidefieldMicroscope::SetupTransitionEndStateFunc(DynExp::ModuleInstance& Instance)
	{
		auto ModuleData = DynExp::dynamic_ModuleData_cast<WidefieldMicroscope>(Instance.ModuleDataGetter());

		if (ModuleData->TestFeature(WidefieldMicroscopeData::FeatureType::WidefieldConfocalIndicator)
			&& ModuleData->GetWidefieldConfocalIndicator()->GetSync() != (ModuleData->GetSetupMode() == WidefieldMicroscopeData::SetupModeType::Confocal))
		{
			ModuleData->SetUIMessage("Transitioning into " + std::string(ModuleData->GetSetupMode() == WidefieldMicroscopeData::SetupModeType::Confocal ? "confocal" : "widefield") + " mode failed!");
			ModuleData->SetSetupMode(ModuleData->GetSetupMode() == WidefieldMicroscopeData::SetupModeType::Confocal ? WidefieldMicroscopeData::SetupModeType::Widefield : WidefieldMicroscopeData::SetupModeType::Confocal);

			return ResetState(ModuleData);
		}

		// Update focus to account for a focus deviation in between confocal/widefield mode.
		SetFocus(ModuleData, ModuleData->GetFocusCurrentVoltage());

		UpdatePumpPower(ModuleData);
		if (TurnOnPumpSourceAfterTransitioning)
			ModuleData->SetPumpLightTurnedOn(true);

		return StateType::SetupTransitionFinished;
	}

	StateType WidefieldMicroscope::ReadyStateFunc(DynExp::ModuleInstance& Instance)
	{
		return StateType::Ready;
	}

	StateType WidefieldMicroscope::AutofocusBeginStateFunc(DynExp::ModuleInstance& Instance)
	{
		auto ModuleData = DynExp::dynamic_ModuleData_cast<WidefieldMicroscope>(Instance.ModuleDataGetter());

		// This is required here since AutoMeasureSampleStepStateFunc() uses the autofocusing states to switch to the
		// widefield mode - even if the FocusAdjustment feature is not set.
		if (!ModuleData->TestFeature(WidefieldMicroscopeData::FeatureType::FocusAdjustment))
			return StateType::AutofocusFinished;

		if (ModuleData->GetWidefieldCamera()->CanSetExposureTime())
			ModuleData->GetWidefieldCamera()->SetExposureTimeSync(ModuleData->GetLEDCameraExposureTime());

		if (ModuleData->TestFeature(WidefieldMicroscopeData::FeatureType::LEDLightToggle))
			ModuleData->SetLEDLightTurnedOn(true);
		if (ModuleData->TestFeature(WidefieldMicroscopeData::FeatureType::PumpLightToggle))
			ModuleData->SetPumpLightTurnedOn(false);

		ModuleData->ResetAutofocusFinished();
		ModuleData->GetAcqCommunicator()->PostEvent(*this, ImageViewer::AutofocusEvent{ true });

		return StateType::AutofocusWaiting;
	}

	StateType WidefieldMicroscope::AutofocusWaitingStateFunc(DynExp::ModuleInstance& Instance)
	{
		auto ModuleData = DynExp::dynamic_ModuleData_cast<WidefieldMicroscope>(Instance.ModuleDataGetter());

		// If directly continuing with capturing a LED image in the context of the automatic sample
		// characterization, wait for a small amount of time until the focus is set to the correct value.
		if (StateMachine.GetContext() == &AutoMeasureSampleContext)
			WaitingEndTimePoint = std::chrono::system_clock::now() + std::chrono::seconds(2);

		return ModuleData->IsAutofocusFinished() ? StateType::AutofocusFinished : StateType::AutofocusWaiting;
	}

	StateType WidefieldMicroscope::LEDImageAcquisitionBeginStateFunc(DynExp::ModuleInstance& Instance)
	{
		auto ModuleData = DynExp::dynamic_ModuleData_cast<WidefieldMicroscope>(Instance.ModuleDataGetter());

		if (ModuleData->GetWidefieldCamera()->CanSetExposureTime())
			ModuleData->GetWidefieldCamera()->SetExposureTimeSync(ModuleData->GetLEDCameraExposureTime());

		if (ModuleData->TestFeature(WidefieldMicroscopeData::FeatureType::LEDLightToggle))
			ModuleData->SetLEDLightTurnedOn(true);
		if (ModuleData->TestFeature(WidefieldMicroscopeData::FeatureType::PumpLightToggle))
			ModuleData->SetPumpLightTurnedOn(false);

		// Always pretend to be in widefield mode if capturing a LED image.
		SetFocus(ModuleData, ModuleData->GetFocusCurrentVoltage(), true);

		PrepareImageRecording(ModuleData);

		return StateType::WaitingForLEDImageReadyToCapture;
	}

	StateType WidefieldMicroscope::WidefieldImageAcquisitionBeginStateFunc(DynExp::ModuleInstance& Instance)
	{
		auto ModuleData = DynExp::dynamic_ModuleData_cast<WidefieldMicroscope>(Instance.ModuleDataGetter());

		if (ModuleData->GetWidefieldCamera()->CanSetExposureTime())
			ModuleData->GetWidefieldCamera()->SetExposureTimeSync(ModuleData->GetWidefieldCameraExposureTime());

		if (ModuleData->TestFeature(WidefieldMicroscopeData::FeatureType::LEDLightToggle))
			ModuleData->SetLEDLightTurnedOn(false);
		if (ModuleData->TestFeature(WidefieldMicroscopeData::FeatureType::PumpLightToggle))
			ModuleData->SetPumpLightTurnedOn(true);

		PrepareImageRecording(ModuleData);

		return StateType::WaitingForWidefieldImageReadyToCapture;
	}

	StateType WidefieldMicroscope::WaitingForImageReadyToCaptureStateFunc(DynExp::ModuleInstance& Instance)
	{
		auto ModuleData = DynExp::dynamic_ModuleData_cast<WidefieldMicroscope>(Instance.ModuleDataGetter());

		if (ModuleData->TestFeature(WidefieldMicroscopeData::FeatureType::InterModuleCommunicator) && !ImageCapturingPaused)
			return StateMachine.GetCurrentState()->GetState();

		RecordImage(ModuleData);

		return StateMachine.GetCurrentState()->GetState() == StateType::WaitingForLEDImageReadyToCapture ?
			StateType::WaitingForLEDImage : StateType::WaitingForWidefieldImage;
	}

	StateType WidefieldMicroscope::WaitingForImageStateFunc(DynExp::ModuleInstance& Instance)
	{
		auto ModuleData = DynExp::dynamic_ModuleData_cast<WidefieldMicroscope>(Instance.ModuleDataGetter());
		auto CameraData = DynExp::dynamic_InstrumentData_cast<DynExpInstr::Camera>(ModuleData->GetWidefieldCamera()->GetInstrumentData());

		if (CameraData->IsImageAvailbale())
		{
			if (ModuleData->TestFeature(WidefieldMicroscopeData::FeatureType::LEDLightToggle))
				ModuleData->SetLEDLightTurnedOn(false);
			if (ModuleData->TestFeature(WidefieldMicroscopeData::FeatureType::PumpLightToggle))
				ModuleData->SetPumpLightTurnedOn(false);

			// Reset focus (refer to WidefieldMicroscope::LEDImageAcquisitionBeginStateFunc()).
			SetFocus(ModuleData, ModuleData->GetFocusCurrentVoltage());

			ModuleData->SetCurrentImage(CameraData->GetImage());

			if (StateMachine.GetCurrentState()->GetState() == StateType::WaitingForWidefieldImage)
				ModuleData->SetWidefieldPosition(ModuleData->GetSamplePosition());

			if (ModuleData->TestFeature(WidefieldMicroscopeData::FeatureType::InterModuleCommunicator))
				ModuleData->GetAcqCommunicator()->PostEvent(*this, ImageViewer::ResumeImageCapturingEvent{});

			return StateMachine.GetCurrentState()->GetState() == StateType::WaitingForLEDImage ?
				StateType::WaitingForLEDImageFinished : StateType::WaitingForWidefieldImageFinished;
		}

		return StateMachine.GetCurrentState()->GetState();
	}

	StateType WidefieldMicroscope::WaitingForWidefieldCellIDStateFunc(DynExp::ModuleInstance& Instance)
	{
		static constexpr const char* ReadCellIDErrorMsg = "Reading cell ID from current image failed. See log for further information.";

		if (*WidefieldCellIDState == WidefieldImageProcessingStateType::Waiting)
			return StateType::WaitingForWidefieldCellID;

		auto ModuleParams = DynExp::dynamic_Params_cast<WidefieldMicroscope>(Instance.ParamsGetter());
		auto ModuleData = DynExp::dynamic_ModuleData_cast<WidefieldMicroscope>(Instance.ModuleDataGetter());

		if (*WidefieldCellIDState == WidefieldImageProcessingStateType::Finished)
		{
			auto WidefieldLocalizerData = DynExp::dynamic_InstrumentData_cast<DynExpInstr::WidefieldLocalization>(ModuleData->GetWidefieldLocalizer()->GetInstrumentData());
			ModuleData->SetCellID(WidefieldLocalizerData->GetCellID());

			if (WidefieldLocalizerData->GetCellID().HasCellShift())
			{
				MoveSampleTo(ModuleData->GetSamplePosition() +
					PositionPointFromPixelDist(ModuleParams, ModuleData, WidefieldLocalizerData->GetCellID().CellShift_px_x, WidefieldLocalizerData->GetCellID().CellShift_px_y),
					ModuleData);

				return StateType::WidefieldCellWaitUntilCentered;
			}
		}
		else
		{
			ModuleData->ResetCellID();

			if (LogUIMessagesOnly)
				Util::EventLogger().Log(ReadCellIDErrorMsg, Util::ErrorType::Error);
			else
				ModuleData->SetUIMessage(ReadCellIDErrorMsg);
		}

		return StateType::WidefieldCellIDReadFinished;
	}

	StateType WidefieldMicroscope::WidefieldCellWaitUntilCenteredStateFunc(DynExp::ModuleInstance& Instance)
	{
		auto ModuleData = DynExp::dynamic_ModuleData_cast<WidefieldMicroscope>(Instance.ModuleDataGetter());
		if (IsSampleMoving(ModuleData))
			return StateType::WidefieldCellWaitUntilCentered;

		if (IsCharacterizingSample())
		{
			StateMachine.SetContext(&AutoMeasureSampleRecenterCellContext);

			// Record new, now centered LED image.
			return StateType::LEDImageAcquisitionBegin;
		}

		return StateType::WidefieldCellIDReadFinished;
	}

	StateType WidefieldMicroscope::WaitingForWidefieldLocalizationStateFunc(DynExp::ModuleInstance& Instance)
	{
		static constexpr const char* LocalizationFailedErrorMsg = "Localization of emitters in widefield image failed. See log for further information.";

		if (*WidefieldLocalizationState == WidefieldImageProcessingStateType::Waiting)
			return StateType::WaitingForWidefieldLocalization;

		auto ModuleData = DynExp::dynamic_ModuleData_cast<WidefieldMicroscope>(Instance.ModuleDataGetter());

		if (*WidefieldLocalizationState == WidefieldImageProcessingStateType::Finished)
		{
			auto WidefieldLocalizerData = DynExp::dynamic_InstrumentData_cast<DynExpInstr::WidefieldLocalization>(ModuleData->GetWidefieldLocalizer()->GetInstrumentData());
			auto& LocalizedPositionsRaw = WidefieldLocalizerData->GetLocalizedPositions();

			WidefieldMicroscopeData::LocalizedPositionsMapType LocalizedPositions;
			for (const auto& Position : LocalizedPositionsRaw)
				LocalizedPositions[Util::NumToT<WidefieldMicroscopeData::LocalizedPositionsMapType::key_type>(Position.first)] = { Position.second };

			ModuleData->SetLocalizedPositions(std::move(LocalizedPositions));
		}
		else
		{
			ModuleData->ClearLocalizedPositions();
			
			if (LogUIMessagesOnly)
				Util::EventLogger().Log(LocalizationFailedErrorMsg, Util::ErrorType::Error);
			else
				ModuleData->SetUIMessage(LocalizationFailedErrorMsg);
		}

		return StateType::WidefieldLocalizationFinished;
	}

	StateType WidefieldMicroscope::FindingConfocalSpotBeginStateFunc(DynExp::ModuleInstance& Instance)
	{
		auto ModuleData = DynExp::dynamic_ModuleData_cast<WidefieldMicroscope>(Instance.ModuleDataGetter());

		// If other modules reading the camera's images are running, stop capturing, so that those modules do not take
		// away a single frame which is to be processed here.
		ModuleData->GetWidefieldCamera()->StopCapturing();

		if (ModuleData->GetWidefieldCamera()->CanSetExposureTime())
			ModuleData->GetWidefieldCamera()->SetExposureTimeSync(ModuleData->GetWidefieldCameraExposureTime());

		ModuleData->SetSetupMode(WidefieldMicroscopeData::SetupModeType::Confocal);

		StateMachine.SetContext(&FindingConfocalSpotBeginContext);
		return StateType::SetupTransitionBegin;
	}

	StateType WidefieldMicroscope::FindingConfocalSpotAfterTransitioningToConfocalModeStateFunc(DynExp::ModuleInstance& Instance)
	{
		auto ModuleData = DynExp::dynamic_ModuleData_cast<WidefieldMicroscope>(Instance.ModuleDataGetter());

		if (ModuleData->TestFeature(WidefieldMicroscopeData::FeatureType::LEDLightToggle))
			ModuleData->SetLEDLightTurnedOn(false);
		if (ModuleData->TestFeature(WidefieldMicroscopeData::FeatureType::PumpLightToggle))
			ModuleData->SetPumpLightTurnedOn(true);

		PrepareImageRecording(ModuleData);

		StateMachine.SetContext(&FindingConfocalSpotRecordingWidefieldImageContext);
		return StateType::WaitingForWidefieldImageReadyToCapture;
	}

	StateType WidefieldMicroscope::FindingConfocalSpotAfterRecordingWidefieldImageStateFunc(DynExp::ModuleInstance& Instance)
	{
		auto ModuleData = DynExp::dynamic_ModuleData_cast<WidefieldMicroscope>(Instance.ModuleDataGetter());

		if (ModuleData->TestFeature(WidefieldMicroscopeData::FeatureType::PumpLightToggle))
			ModuleData->SetPumpLightTurnedOn(false);

		auto IntensityImage = ModuleData->GetCurrentImage().convertToFormat(QImage::Format_Grayscale8);
		const unsigned char* DataPtr = IntensityImage.constBits();

		QPoint MaxCoord(0, 0);
		unsigned char MaxValue = 0;
		for (int y = 0; y < IntensityImage.width(); ++y)
			for (int x = 0; x < IntensityImage.height(); ++x)
				if (*DataPtr++ > MaxValue)
				{
					MaxCoord = { x, y };
					MaxValue = *(DataPtr - 1);
				}

		ModuleData->SetConfocalSpotImagePosition(MaxCoord);
		ModuleData->SetSetupMode(WidefieldMicroscopeData::SetupModeType::Widefield);

		return StateType::SetupTransitionBegin;
	}

	StateType WidefieldMicroscope::ConfocalScanStepStateFunc(DynExp::ModuleInstance& Instance)
	{
		if (ConfocalScanPositions.empty())
			return StateType::Ready;

		const auto& Position = ConfocalScanPositions.front();
		
		{
			auto ModuleData = DynExp::dynamic_ModuleData_cast<WidefieldMicroscope>(Instance.ModuleDataGetter());
			ModuleData->GetSampleStageX()->UpdateData();
			ModuleData->GetSampleStageY()->UpdateData();
			MoveSampleTo(Position, ModuleData);
		} // ModuleData unlocked here.

		return StateType::ConfocalScanWaitUntilMoved;
	}

	StateType WidefieldMicroscope::ConfocalScanWaitUntilMovedStateFunc(DynExp::ModuleInstance& Instance)
	{
		auto ModuleData = DynExp::dynamic_ModuleData_cast<WidefieldMicroscope>(Instance.ModuleDataGetter());
		if (IsSampleMoving(ModuleData))
			return StateType::ConfocalScanWaitUntilMoved;

		auto MeasuredPos = ModuleData->GetSamplePosition();
		ConfocalScanPositions.front().MeasuredX = MeasuredPos.x;
		ConfocalScanPositions.front().MeasuredY = MeasuredPos.y;

		return StateType::ConfocalScanCapture;
	}

	StateType WidefieldMicroscope::ConfocalScanCaptureStateFunc(DynExp::ModuleInstance& Instance)
	{
		auto ModuleData = DynExp::dynamic_ModuleData_cast<WidefieldMicroscope>(Instance.ModuleDataGetter());

		ModuleData->ResetSPD1State();
		ModuleData->ResetSPD2State();

		{
			auto SPD1Data = DynExp::dynamic_InstrumentData_cast<DynExpInstr::TimeTagger>(ModuleData->GetSPD1()->GetInstrumentData());
			ModuleData->SetSPD1SamplesWritten(SPD1Data->GetCastSampleStream<DynExpInstr::TimeTaggerData::SampleStreamType>()->GetNumSamplesWritten());
		} // SPD1Data unlocked here.
		ModuleData->GetSPD1()->ReadData();

		if (ModuleData->TestFeature(WidefieldMicroscopeData::FeatureType::HBT))
		{
			{
				auto SPD2Data = DynExp::dynamic_InstrumentData_cast<DynExpInstr::TimeTagger>(ModuleData->GetSPD2()->GetInstrumentData());
				ModuleData->SetSPD2SamplesWritten(SPD2Data->GetCastSampleStream<DynExpInstr::TimeTaggerData::SampleStreamType>()->GetNumSamplesWritten());
			} // SPD2Data unlocked here.
			ModuleData->GetSPD2()->ReadData();
		}

		return StateType::ConfocalScanWaitUntilCaptured;
	}

	// Skips first sample to be sure that the sample read has entirely been captured at the most recent sample stages' positions.
	StateType WidefieldMicroscope::ConfocalScanWaitUntilCapturedStateFunc(DynExp::ModuleInstance& Instance)
	{
		auto ModuleData = DynExp::dynamic_ModuleData_cast<WidefieldMicroscope>(Instance.ModuleDataGetter());
		bool UsingSPD2 = ModuleData->TestFeature(WidefieldMicroscopeData::FeatureType::HBT);

		{
			auto SPD1Data = DynExp::dynamic_InstrumentData_cast<DynExpInstr::TimeTagger>(ModuleData->GetSPD1()->GetInstrumentData());
			auto SPD1DataSampleStream = SPD1Data->GetCastSampleStream<DynExpInstr::TimeTaggerData::SampleStreamType>();
			if (SPD1DataSampleStream->GetNumSamplesWritten() >= ModuleData->GetSPD1SamplesWritten() + 2)
				ModuleData->SetSPD1Ready(SPD1DataSampleStream->ReadSample().Value);
		} // SPD1Data unlocked here.
		ModuleData->GetSPD1()->ReadData();

		if (UsingSPD2)
		{
			{
				auto SPD2Data = DynExp::dynamic_InstrumentData_cast<DynExpInstr::TimeTagger>(ModuleData->GetSPD2()->GetInstrumentData());
				auto SPD2DataSampleStream = SPD2Data->GetCastSampleStream<DynExpInstr::TimeTaggerData::SampleStreamType>();
				if (SPD2DataSampleStream->GetNumSamplesWritten() >= ModuleData->GetSPD2SamplesWritten() + 2)
					ModuleData->SetSPD2Ready(SPD2DataSampleStream->ReadSample().Value);
			} // SPD2Data unlocked here.
			ModuleData->GetSPD2()->ReadData();
		}

		if (ModuleData->GetSPD1Ready() && (!UsingSPD2 || ModuleData->GetSPD2Ready()))
		{
			auto ExposureTime = ModuleData->GetSPDExposureTime().count();
			double CountRate = (ModuleData->GetSPD1Value() + ModuleData->GetSPD2Value())
				/ ExposureTime * WidefieldMicroscope::ModuleDataType::SPDTimeType::period::den;

			if (StateMachine.GetContext() != &ConfocalOptimizationContext &&
				StateMachine.GetContext() != &AutoMeasureCharacterizationContext &&
				StateMachine.GetContext() != &AutoMeasureCharacterizationOptimizationContext &&
				StateMachine.GetContext() != &AutoMeasureCharacterizationSpectrumContext &&
				StateMachine.GetContext() != &AutoMeasureCharacterizationHBTContext &&
				!IsCharacterizingSample())
				ModuleData->GetConfocalScanResults().emplace_back(ConfocalScanPositions.front(), CountRate);
			ModuleData->SetLastCountRate(CountRate);

			ConfocalScanPositions.pop_front();

			return StateType::ConfocalScanStep;
		}

		return StateType::ConfocalScanWaitUntilCaptured;
	}

	StateType WidefieldMicroscope::ConfocalOptimizationInitStateFunc(DynExp::ModuleInstance& Instance)
	{
		ConfocalOptimizationInitPromises();

		ConfocalOptimizationThreadReturnFuture = std::async(std::launch::async, &WidefieldMicroscope::ConfocalOptimizationThread, this);

		return StateType::ConfocalOptimizationWait;
	}

	StateType WidefieldMicroscope::ConfocalOptimizationInitSubStepStateFunc(DynExp::ModuleInstance& Instance)
	{
		ConfocalOptimizationInitPromises();

		return StateType::ConfocalOptimizationWait;
	}

	StateType WidefieldMicroscope::ConfocalOptimizationWaitStateFunc(DynExp::ModuleInstance& Instance)
	{
		// New sample available?
		if (ConfocalOptimizationStateFuture.wait_for(std::chrono::microseconds(0)) == std::future_status::ready)
		{
			const auto State = ConfocalOptimizationStateFuture.get();

			auto ModuleData = DynExp::dynamic_ModuleData_cast<WidefieldMicroscope>(Instance.ModuleDataGetter());

			SetFocus(ModuleData, State.Z);
			ConfocalScanPositions.clear();
			ConfocalScanPositions.emplace_back(State.X, State.Y);

			// StateType::DummyState is replaced by context.
			return StateType::DummyState;
		}

		// Optimizer iteration finished?
		if (ConfocalOptimizationThreadReturnFuture.wait_for(std::chrono::microseconds(0)) == std::future_status::ready)
			return StateType::ConfocalOptimizationStep;
		else
			return StateType::ConfocalOptimizationWait;
	}

	StateType WidefieldMicroscope::ConfocalOptimizationStepStateFunc(DynExp::ModuleInstance& Instance)
	{
		static constexpr const char* OptimizationMaxIterReachedErrorMsg = "Optimizing confocal count rate failed - maximal number of iterations reached!";
		static constexpr const char* OptimizationFailedErrorMsg = "Optimizing confocal count rate failed!";

		{
			auto ModuleData = DynExp::dynamic_ModuleData_cast<WidefieldMicroscope>(Instance.ModuleDataGetter());

			ConfocalOptimizationPromisesRenewed = false;

			// Negative value to find maximum instead of minimum.
			ConfocalOptimizationFeedbackPromise.set_value(-ModuleData->GetLastCountRate());
		} // ModuleData unlocked here.

		// Optimizer iteration finished? Next iteration...
		if (ConfocalOptimizationThreadReturnFuture.wait_for(std::chrono::microseconds(0)) == std::future_status::ready)
		{
			auto Result = ConfocalOptimizationThreadReturnFuture.get();

			if (++ConfocalOptimizationNumStepsPerformed > 100)
			{
				auto ModuleData = DynExp::dynamic_ModuleData_cast<WidefieldMicroscope>(Instance.ModuleDataGetter());
				if (LogUIMessagesOnly)
					Util::EventLogger().Log(OptimizationMaxIterReachedErrorMsg, Util::ErrorType::Error);
				else
					ModuleData->SetUIMessage(OptimizationMaxIterReachedErrorMsg);
			}

			if (Result == ConfocalOptimizationThreadReturnType::NextStep)
				return StateType::ConfocalOptimizationInit;
			if (Result == ConfocalOptimizationThreadReturnType::Failed)
			{
				auto ModuleData = DynExp::dynamic_ModuleData_cast<WidefieldMicroscope>(Instance.ModuleDataGetter());
				if (LogUIMessagesOnly)
					Util::EventLogger().Log(OptimizationFailedErrorMsg, Util::ErrorType::Error);
				else
					ModuleData->SetUIMessage(OptimizationFailedErrorMsg);
			}

			return StateType::ConfocalOptimizationFinished;
		}
		
		// Iteration not finished. Prepare for next sample within the same iteration.
		return StateType::ConfocalOptimizationInitSubStep;
	}

	StateType WidefieldMicroscope::HBTAcquiringStateFunc(DynExp::ModuleInstance& Instance)
	{
		auto ModuleData = DynExp::dynamic_ModuleData_cast<WidefieldMicroscope>(Instance.ModuleDataGetter());
		auto SPD1Data = DynExp::dynamic_InstrumentData_cast<DynExpInstr::TimeTagger>(ModuleData->GetSPD1()->GetInstrumentData());

		// If the number of already enqueued tasks is not checked and if the SPD's
		// read data task is slow, the queue might grow large.
		if (SPD1Data->GetNumEnqueuedTasks() < 10)
			ModuleData->GetSPD1()->ReadData();

		ModuleData->SetHBTTotalIntegrationTime(SPD1Data->GetHBTResults().IntegrationTime);
		ModuleData->SetHBTNumEventCounts(SPD1Data->GetHBTResults().EventCounts);

		double XMin(std::numeric_limits<double>::max()), XMax(std::numeric_limits<double>::lowest());
		double YMin(std::numeric_limits<double>::max()), YMax(std::numeric_limits<double>::lowest());
		ModuleData->GetHBTDataPoints().clear();
		for (const auto& ResultItem : SPD1Data->GetHBTResults().ResultVector)
		{
			auto X = ResultItem.Time * std::pico::den;
			auto Y = ResultItem.Value;
			ModuleData->GetHBTDataPoints().append(QPointF(X, Y));

			XMin = std::min(XMin, X);
			XMax = std::max(XMax, X);
			YMin = std::min(YMin, Y);
			YMax = std::max(YMax, Y);
			ModuleData->SetHBTDataPointsMaxValues({ XMax, YMax });
			ModuleData->SetHBTDataPointsMinValues({ XMin, YMin });
		}

		if (ModuleData->GetHBTMaxIntegrationTime() > std::chrono::microseconds(0) && ModuleData->GetHBTTotalIntegrationTime() >= ModuleData->GetHBTMaxIntegrationTime())
		{
			StopHBT(ModuleData);

			return StateType::HBTFinished;
		}
		else
			return StateType::HBTAcquiring;
	}

	StateType WidefieldMicroscope::WaitingStateFunc(DynExp::ModuleInstance& Instance)
	{
		return std::chrono::system_clock::now() >= WaitingEndTimePoint ? StateType::WaitingFinished : StateType::Waiting;
	}

	StateType WidefieldMicroscope::SpectrumAcquisitionWaitingStateFunc(DynExp::ModuleInstance& Instance)
	{
		// Nothing to do here. We await SpectrumFinishedRecordingEvent.
		return StateType::SpectrumAcquisitionWaiting;
	}

	StateType WidefieldMicroscope::AutoMeasureLocalizationStepStateFunc(DynExp::ModuleInstance& Instance)
	{
		auto ModuleData = DynExp::dynamic_ModuleData_cast<WidefieldMicroscope>(Instance.ModuleDataGetter());

		if (ModuleData->IncrementAutoMeasureCurrentImageSet() >= ModuleData->GetAutoMeasureNumberImageSets())
			return StateType::AutoMeasureLocalizationFinished;

		return StateType::LEDImageAcquisitionBegin;
	}

	StateType WidefieldMicroscope::AutoMeasureLocalizationSaveLEDImageStateFunc(DynExp::ModuleInstance& Instance)
	{
		int Index;
		std::filesystem::path Filename;

		{
			auto ModuleData = DynExp::dynamic_ModuleData_cast<WidefieldMicroscope>(Instance.ModuleDataGetter());

			Index = ModuleData->GetAutoMeasureCurrentImageSet();
			Filename = BuildFilename(ModuleData, "_LED_" + Util::ToStr(Index) + ".png");
		} // ModuleData unlocked here.

		OnSaveCurrentImage(&Instance, QString::fromUtf16(Filename.u16string().c_str()));

		return StateType::WidefieldImageAcquisitionBegin;
	}

	StateType WidefieldMicroscope::AutoMeasureLocalizationSaveWidefieldImageStateFunc(DynExp::ModuleInstance& Instance)
	{
		int Index, ImagePositionScatterRadius;
		std::filesystem::path Filename;

		{
			auto ModuleData = DynExp::dynamic_ModuleData_cast<WidefieldMicroscope>(Instance.ModuleDataGetter());

			Index = ModuleData->GetAutoMeasureCurrentImageSet();
			ImagePositionScatterRadius = ModuleData->GetAutoMeasureImagePositionScatterRadius();
			Filename = BuildFilename(ModuleData, "_WF_" + Util::ToStr(Index) + ".png");
		} // ModuleData unlocked here.

		OnSaveCurrentImage(&Instance, QString::fromUtf16(Filename.u16string().c_str()));

		// Move to next image capturing position.
		if (ImagePositionScatterRadius > 0)
		{
			auto ModuleData = DynExp::dynamic_ModuleData_cast<WidefieldMicroscope>(Instance.ModuleDataGetter());

			// Record first and last image sets at the cell's center position.
			// Subtract 2 since index of current image set starts at 0 and this function runs before
			// advancing to the next image set whose position is determined here.
			if (ModuleData->GetAutoMeasureNumberImageSets() > 2 &&
				ModuleData->GetAutoMeasureCurrentImageSet() != ModuleData->GetAutoMeasureNumberImageSets() - 2)
				MoveSampleTo(ModuleData->GetAutoMeasureCurrentCellPosition() + RandomPointInCircle(ImagePositionScatterRadius), ModuleData);
			else
				MoveSampleTo(ModuleData->GetAutoMeasureCurrentCellPosition(), ModuleData);

			return StateType::AutoMeasureLocalizationMoving;
		}
		else
			return StateType::AutoMeasureLocalizationStep;
	}

	StateType WidefieldMicroscope::AutoMeasureLocalizationMovingStateFunc(DynExp::ModuleInstance& Instance)
	{
		auto ModuleData = DynExp::dynamic_ModuleData_cast<WidefieldMicroscope>(Instance.ModuleDataGetter());

		if (IsSampleMoving(ModuleData))
			return StateType::AutoMeasureLocalizationMoving;

		return StateType::AutoMeasureLocalizationStep;
	}

	StateType WidefieldMicroscope::AutoMeasureCharacterizationStepStateFunc(DynExp::ModuleInstance& Instance)
	{
		auto ModuleParams = DynExp::dynamic_Params_cast<WidefieldMicroscope>(Instance.ParamsGetter());
		auto ModuleData = DynExp::dynamic_ModuleData_cast<WidefieldMicroscope>(Instance.ModuleDataGetter());

		// Save the emitter list after characterizing each emitter.
		std::stringstream CSVData;
		CSVData = ModuleData->AssembleCSVHeader(false, false, true);
		CSVData << "ID;X(px);Y(px);State\n";

		auto& Positions = ModuleData->GetLocalizedPositions();
		for (const auto& Position : Positions)
			CSVData << Position.first << ";" << Position.second.Position.x() << ";" << Position.second.Position.y() << ";"
			<< WidefieldMicroscopeData::GetLocalizedEmitterStateString(Position.second.State) << "\n";

		if (!Util::SaveToFile(QString::fromUtf16(BuildFilename(ModuleData, "_Emitters.csv").u16string().c_str()), CSVData.str()))
			Util::EventLogger().Log("Saving the emitter list failed.", Util::ErrorType::Error);

		if (ModuleData->GetAutoMeasureCurrentEmitter() == ModuleData->GetLocalizedPositions().cend())
		{
			if (ModuleData->TestFeature(WidefieldMicroscopeData::FeatureType::PumpLightToggle))
				ModuleData->SetPumpLightTurnedOn(false);

			ModuleData->SetAutoMeasureRunning(false);

			return StateType::AutoMeasureCharacterizationFinished;
		}

		if (ModuleData->TestFeature(WidefieldMicroscopeData::FeatureType::LEDLightToggle))
			ModuleData->SetLEDLightTurnedOn(false);
		if (ModuleData->TestFeature(WidefieldMicroscopeData::FeatureType::PumpLightToggle))
			ModuleData->SetPumpLightTurnedOn(true);

		ModuleData->GetAutoMeasureCurrentEmitter()->second.State = WidefieldMicroscopeData::LocalizedEmitterStateType::Characterizing;
		ModuleData->SetLocalizedPositionsStateChanged();
		BringMarkerToConfocalSpot(ModuleParams, ModuleData, ModuleData->GetAutoMeasureCurrentEmitter()->second.Position,
			{ static_cast<qreal>(ModuleData->GetWidefieldPosition().x), static_cast<qreal>(ModuleData->GetWidefieldPosition().y) });

		ModuleData->ResetAutoMeasureCurrentOptimizationAttempt();

		return StateType::AutoMeasureCharacterizationGotoEmitter;
	}

	StateType WidefieldMicroscope::AutoMeasureCharacterizationGotoEmitterStateFunc(DynExp::ModuleInstance& Instance)
	{
		auto ModuleParams = DynExp::dynamic_Params_cast<WidefieldMicroscope>(Instance.ParamsGetter());
		auto ModuleData = DynExp::dynamic_ModuleData_cast<WidefieldMicroscope>(Instance.ModuleDataGetter());

		if (IsSampleMoving(ModuleData))
			return StateType::AutoMeasureCharacterizationGotoEmitter;

		if (ModuleData->GetAutoMeasureOptimizeEnabled() && ModuleData->TestFeature(WidefieldMicroscopeData::FeatureType::ConfocalOptimization) &&
			ModuleData->GetSetupMode() == WidefieldMicroscopeData::SetupModeType::Confocal)
		{
			ModuleData->ResetAutoMeasureCurrentOptimizationRerun();
			InitializeConfocalOptimizer(ModuleData);

			if (ModuleData->TestFeature(WidefieldMicroscopeData::FeatureType::HBTSwitch))
			{
				SetHBTSwitch(ModuleParams, ModuleData, true);

				WaitingEndTimePoint = std::chrono::system_clock::now() + std::chrono::milliseconds(ModuleParams->WidefieldHBTTransitionTime);

				StateMachine.SetContext(IsCharacterizingSample() ? &AutoMeasureSampleCharacterizationOptimizationContext : &AutoMeasureCharacterizationOptimizationContext);
				return StateType::Waiting;
			}
			else
				return StateType::ConfocalOptimizationInit;
		}
		else
			return StateType::AutoMeasureCharacterizationOptimizationFinished;
	}

	StateType WidefieldMicroscope::AutoMeasureCharacterizationOptimizationFinishedStateFunc(DynExp::ModuleInstance& Instance)
	{
		auto ModuleParams = DynExp::dynamic_Params_cast<WidefieldMicroscope>(Instance.ParamsGetter());
		auto ModuleData = DynExp::dynamic_ModuleData_cast<WidefieldMicroscope>(Instance.ModuleDataGetter());

		const auto EmitterDestiny = CalcMarkerToConfocalSpotDestiny(ModuleParams, ModuleData, ModuleData->GetAutoMeasureCurrentEmitter()->second.Position,
			{ static_cast<qreal>(ModuleData->GetWidefieldPosition().x), static_cast<qreal>(ModuleData->GetWidefieldPosition().y) });
			
		if (ModuleData->GetLastCountRate() >= ModuleData->GetAutoMeasureCountRateThreshold() &&
			EmitterDestiny.DistTo(ModuleData->GetSamplePosition()) <= ModuleData->GetAutoMeasureOptimizationMaxDistance())
		{
			// Optimization succeeded.
			if (ModuleData->GetAutoMeasureSpectrumEnabled() && ModuleData->TestFeature(WidefieldMicroscopeData::FeatureType::InterModuleCommunicator))
			{
				if (ModuleData->TestFeature(WidefieldMicroscopeData::FeatureType::HBTSwitch))
				{
					SetHBTSwitch(ModuleParams, ModuleData, false);

					WaitingEndTimePoint = std::chrono::system_clock::now() + std::chrono::milliseconds(ModuleParams->WidefieldHBTTransitionTime);

					StateMachine.SetContext(IsCharacterizingSample() ? &AutoMeasureSampleCharacterizationSpectrumContext : &AutoMeasureCharacterizationSpectrumContext);
					return StateType::Waiting;
				}
				else
					return StateType::AutoMeasureCharacterizationSpectrumBegin;
			}
			else
				return StateType::AutoMeasureCharacterizationSpectrumFinished;
		}

		if (ModuleData->GetAutoMeasureOptimizeEnabled() && ModuleData->TestFeature(WidefieldMicroscopeData::FeatureType::ConfocalOptimization) &&
			ModuleData->GetSetupMode() == WidefieldMicroscopeData::SetupModeType::Confocal)
		{
			if (ModuleData->IncrementAutoMeasureCurrentOptimizationRerun() >= ModuleData->GetAutoMeasureMaxOptimizationReruns())
			{
				if (ModuleData->IncrementAutoMeasureCurrentOptimizationAttempt() <= ModuleData->GetAutoMeasureNumOptimizationAttempts())
				{
					BringMarkerToConfocalSpot(ModuleParams, ModuleData, ModuleData->GetAutoMeasureCurrentEmitter()->second.Position,
						{ static_cast<qreal>(ModuleData->GetWidefieldPosition().x), static_cast<qreal>(ModuleData->GetWidefieldPosition().y) });

					return StateType::AutoMeasureCharacterizationGotoEmitter;
				}
			}
			else
			{
				InitializeConfocalOptimizer(ModuleData);

				return StateType::ConfocalOptimizationInit;
			}
		}

		ModuleData->GetAutoMeasureCurrentEmitter()->second.State = WidefieldMicroscopeData::LocalizedEmitterStateType::Failed;
		ModuleData->SetLocalizedPositionsStateChanged();

		ModuleData->IncrementAutoMeasureCurrentEmitter();
		return StateType::AutoMeasureCharacterizationStep;
	}

	StateType WidefieldMicroscope::AutoMeasureCharacterizationSpectrumBeginStateFunc(DynExp::ModuleInstance& Instance)
	{
		auto ModuleData = DynExp::dynamic_ModuleData_cast<WidefieldMicroscope>(Instance.ModuleDataGetter());

		ModuleData->GetAcqCommunicator()->PostEvent(*this, SpectrumViewer::RecordSpectrumEvent {
			BuildFilename(ModuleData, "_Emitter" + Util::ToStr(ModuleData->GetAutoMeasureCurrentEmitter()->first) + "_Spectrum.csv").string() });

		return StateType::SpectrumAcquisitionWaiting;
	}

	StateType WidefieldMicroscope::AutoMeasureCharacterizationSpectrumFinishedStateFunc(DynExp::ModuleInstance& Instance)
	{
		auto ModuleParams = DynExp::dynamic_Params_cast<WidefieldMicroscope>(Instance.ParamsGetter());
		auto ModuleData = DynExp::dynamic_ModuleData_cast<WidefieldMicroscope>(Instance.ModuleDataGetter());

		if (ModuleData->GetAutoMeasureHBTEnabled() && ModuleData->TestFeature(WidefieldMicroscopeData::FeatureType::HBT))
		{
			if (ModuleData->TestFeature(WidefieldMicroscopeData::FeatureType::HBTSwitch))
			{
				SetHBTSwitch(ModuleParams, ModuleData, true);

				WaitingEndTimePoint = std::chrono::system_clock::now() + std::chrono::milliseconds(ModuleParams->WidefieldHBTTransitionTime);

				StateMachine.SetContext(IsCharacterizingSample() ? &AutoMeasureSampleCharacterizationHBTContext : &AutoMeasureCharacterizationHBTContext);
				return StateType::Waiting;
			}
			else
				return StateType::AutoMeasureCharacterizationHBTBegin;
		}

		ModuleData->GetAutoMeasureCurrentEmitter()->second.State = WidefieldMicroscopeData::LocalizedEmitterStateType::Finished;
		ModuleData->SetLocalizedPositionsStateChanged();

		ModuleData->IncrementAutoMeasureCurrentEmitter();
		return StateType::AutoMeasureCharacterizationStep;
	}

	StateType WidefieldMicroscope::AutoMeasureCharacterizationHBTBeginStateFunc(DynExp::ModuleInstance& Instance)
	{
		auto ModuleData = DynExp::dynamic_ModuleData_cast<WidefieldMicroscope>(Instance.ModuleDataGetter());

		InitializeHBT(ModuleData);

		return StateType::AutoMeasureCharacterizationHBTWaitForInit;
	}

	StateType WidefieldMicroscope::AutoMeasureCharacterizationHBTWaitForInitStateFunc(DynExp::ModuleInstance& Instance)
	{
		auto ModuleData = DynExp::dynamic_ModuleData_cast<WidefieldMicroscope>(Instance.ModuleDataGetter());
		auto SPD1Data = DynExp::dynamic_InstrumentData_cast<DynExpInstr::TimeTagger>(ModuleData->GetSPD1()->GetInstrumentData());

		// If the number of already enqueued tasks is not checked and if the SPD's
		// read data task is slow, the queue might grow large.
		if (SPD1Data->GetNumEnqueuedTasks() < 10)
			ModuleData->GetSPD1()->ReadData();

		ModuleData->SetHBTTotalIntegrationTime(SPD1Data->GetHBTResults().IntegrationTime);

		if (HBTIntegrationTimeBeforeReset <= std::chrono::milliseconds(10) || ModuleData->GetHBTTotalIntegrationTime() < HBTIntegrationTimeBeforeReset)
			return StateType::HBTAcquiring;

		return StateType::AutoMeasureCharacterizationHBTWaitForInit;
	}

	StateType WidefieldMicroscope::AutoMeasureCharacterizationHBTFinishedStateFunc(DynExp::ModuleInstance& Instance)
	{
		auto ModuleData = DynExp::dynamic_ModuleData_cast<WidefieldMicroscope>(Instance.ModuleDataGetter());

		auto Filename = BuildFilename(ModuleData, "_Emitter" + Util::ToStr(ModuleData->GetAutoMeasureCurrentEmitter()->first) + "_g2.csv");
		std::stringstream CSVData;

		CSVData = ModuleData->AssembleCSVHeader(false, true, false);
		ModuleData->WriteHBTResults(CSVData);

		ModuleData->GetAutoMeasureCurrentEmitter()->second.State = WidefieldMicroscopeData::LocalizedEmitterStateType::Finished;
		ModuleData->SetLocalizedPositionsStateChanged();

		if (!Util::SaveToFile(QString::fromUtf16(Filename.u16string().c_str()), CSVData.str()))
			Util::EventLogger().Log("Saving the g2 result failed.", Util::ErrorType::Error);

		ModuleData->IncrementAutoMeasureCurrentEmitter();
		return StateType::AutoMeasureCharacterizationStep;
	}

	StateType WidefieldMicroscope::AutoMeasureSampleStepStateFunc(DynExp::ModuleInstance& Instance)
	{
		auto ModuleData = DynExp::dynamic_ModuleData_cast<WidefieldMicroscope>(Instance.ModuleDataGetter());

		if (IsSampleMoving(ModuleData))
			return StateType::AutoMeasureSampleStep;

		// AutofocusBeginStateFunc() checks whether the FeatureType::FocusAdjustment feature is set.
		return StartAutofocus(ModuleData);
	}

	StateType WidefieldMicroscope::AutoMeasureSampleReadCellIDStateFunc(DynExp::ModuleInstance& Instance)
	{
		auto ModuleData = DynExp::dynamic_ModuleData_cast<WidefieldMicroscope>(Instance.ModuleDataGetter());

		return InitiateReadCellIDFromImage(ModuleData);
	}

	StateType WidefieldMicroscope::AutoMeasureSampleReadCellIDFinishedStateFunc(DynExp::ModuleInstance& Instance)
	{
		auto ModuleData = DynExp::dynamic_ModuleData_cast<WidefieldMicroscope>(Instance.ModuleDataGetter());

		// It was not possible to extract the cell ID from the image, so just increment the previous id
		// (hoping that we are still at the right place on the sample).
		if (!ModuleData->GetCellID().Valid)
		{
			ModuleData->SetCellIDToLastCellID();
			ModuleData->IncrementCellID();

			Util::EventLogger().Log("Reading a cell ID failed. Estimating it to " +
				ModuleData->GetCellID().IDString + ".", Util::ErrorType::Warning);
		}

		// Cannot continue if current cell's ID does not comply with certain criteria.
		// In that case, we wouldn't know where we are on the sample. Then, better abort the
		// measurement instead of damaging anything by moving the sample around blindly.
		if (Util::NumToT<int>(ModuleData->GetCellID().X_id) < ModuleData->GetAutoMeasureCellRangeFrom().x() ||
			Util::NumToT<int>(ModuleData->GetCellID().Y_id) < ModuleData->GetAutoMeasureCellRangeFrom().y() ||
			Util::NumToT<int>(ModuleData->GetCellID().X_id) > ModuleData->GetAutoMeasureCellRangeTo().x() ||
			Util::NumToT<int>(ModuleData->GetCellID().Y_id) > ModuleData->GetAutoMeasureCellRangeTo().y())
		{
			ModuleData->SetUIMessage("The current cell's ID is outside the specified cell range. Probably moved in wrong direction? Characterizing the sample cannot continue.");

			return ResetState(ModuleData);
		}

		if (ModuleData->GetLastCellID().Valid && ModuleData->GetLastCellID() >= ModuleData->GetCellID())
		{
			ModuleData->SetUIMessage("The current cell's ID is not larger than the previous cell's ID. Probably moved in wrong direction? Characterizing the sample cannot continue.");

			return ResetState(ModuleData);
		}

		if (ModuleData->GetAutoMeasureLocalizationType() == WidefieldMicroscopeWidget::LocalizationType::LocalizeEmittersFromImage)
			return StartAutoMeasureLocalization(ModuleData);
		else
			return InitiateLocalizationFromImage(ModuleData);
	}

	StateType WidefieldMicroscope::AutoMeasureSampleLocalizeStateFunc(DynExp::ModuleInstance& Instance)
	{
		auto ModuleData = DynExp::dynamic_ModuleData_cast<WidefieldMicroscope>(Instance.ModuleDataGetter());

		// Do not record LED/Widefield images in case no emitters could be recalled from cell.
		if (ModuleData->GetLocalizedPositions().empty())
		{
			// This is required since the position is not updated because image recording is skipped.
			// Though, AutoMeasureSampleAdvanceCellStateFunc() expects the widefield position to be
			// set to the current sample position. So, set the position here manually.
			ModuleData->SetWidefieldPosition(ModuleData->GetSamplePosition());

			return StateType::AutoMeasureSampleAdvanceCell;
		}

		return StartAutoMeasureLocalization(ModuleData);
	}

	StateType WidefieldMicroscope::AutoMeasureSampleFindEmittersStateFunc(DynExp::ModuleInstance& Instance)
	{
		auto ModuleData = DynExp::dynamic_ModuleData_cast<WidefieldMicroscope>(Instance.ModuleDataGetter());

		if (ModuleData->TestFeature(WidefieldMicroscopeData::FeatureType::InterModuleCommunicator))
			ModuleData->GetAcqCommunicator()->PostEvent(*this, SpectrumViewer::SetSilentModeEvent{ false });

		if (ModuleData->GetAutoMeasureLocalizationType() == WidefieldMicroscopeWidget::LocalizationType::LocalizeEmittersFromImage)
			return InitiateLocalizationFromImage(ModuleData);
		else
			return StateType::WidefieldLocalizationFinished;
	}

	StateType WidefieldMicroscope::AutoMeasureSampleCharacterizeStateFunc(DynExp::ModuleInstance& Instance)
	{
		auto ModuleData = DynExp::dynamic_ModuleData_cast<WidefieldMicroscope>(Instance.ModuleDataGetter());

		return StartAutoMeasureCharacterization(ModuleData);
	}

	/**
	 * @brief Checks whether the last cell within the sample characterization range (specified as a square region by
	 * @p WidefieldMicroscopeData::AutoMeasureCellRangeFrom and @p WidefieldMicroscopeData::AutoMeasureCellRangeTo)
	 * has been reached. If this is not the case, the sample is moved to advance to the next cell.
	 * To advance, the x-coordinate is increased. After reaching the end of the current line
	 * (given by WidefieldMicroscopeData::AutoMeasureCellRangeTo::x), a 'line break' is performed by increasing the
	 * y-coordinate and resetting the x-coordinate to WidefieldMicroscopeData::AutoMeasureCellRangeFrom::x.
	 * @param Instance Handle to current WidefieldMicroscope instance
	 * @return @p StateType::AutoMeasureSampleFinished when finished, @p StateType::AutoMeasureSampleStep otherwise
	*/
	StateType WidefieldMicroscope::AutoMeasureSampleAdvanceCellStateFunc(DynExp::ModuleInstance& Instance)
	{
		auto ModuleData = DynExp::dynamic_ModuleData_cast<WidefieldMicroscope>(Instance.ModuleDataGetter());

		if (ModuleData->GetWidefieldPosition().IsEmpty())
		{
			ModuleData->SetUIMessage("The current widefield image's position is invalid. Characterizing the sample cannot continue.");

			return ResetState(ModuleData);
		}

		// Has finished?
		if (Util::NumToT<int>(ModuleData->GetCellID().X_id) == ModuleData->GetAutoMeasureCellRangeTo().x() &&
			Util::NumToT<int>(ModuleData->GetCellID().Y_id) == ModuleData->GetAutoMeasureCellRangeTo().y())
			return StateType::AutoMeasureSampleFinished;

		// Not finished yet, so advance to next cell.
		if (Util::NumToT<int>(ModuleData->GetCellID().X_id) == ModuleData->GetAutoMeasureCellRangeTo().x())
			MoveSampleTo({
				ModuleData->GetWidefieldPosition().x - Util::NumToT<WidefieldMicroscopeData::PositionType>(ModuleData->GetAutoMeasureCellSkip().x()) * (ModuleData->GetAutoMeasureCellLineLength() - 1),
				ModuleData->GetWidefieldPosition().y + Util::NumToT<WidefieldMicroscopeData::PositionType>(ModuleData->GetAutoMeasureCellSkip().y())
			}, ModuleData);
		else
			MoveSampleTo({
				ModuleData->GetWidefieldPosition().x + Util::NumToT<WidefieldMicroscopeData::PositionType>(ModuleData->GetAutoMeasureCellSkip().x()),
				ModuleData->GetWidefieldPosition().y
			}, ModuleData);

		return StartAutoMeasureSampleCharacterization(ModuleData);
	}
}