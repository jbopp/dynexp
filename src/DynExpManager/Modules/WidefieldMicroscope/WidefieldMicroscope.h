// This file is part of DynExp.

/**
 * @file WidefieldMicroscope.h
 * @brief Implementation of a module to control a combined widefield and confocal microscope.
*/

#pragma once

#include "stdafx.h"
#include "DynExpCore.h"
#include "../../MetaInstruments/Stage.h"
#include "../../MetaInstruments/DigitalOut.h"
#include "../../MetaInstruments/DigitalIn.h"
#include "../../MetaInstruments/AnalogOut.h"
#include "../../MetaInstruments/AnalogIn.h"
#include "../../MetaInstruments/Camera.h"
#include "../../MetaInstruments/TimeTagger.h"
#include "../../Instruments/InterModuleCommunicator.h"
#include "../../Instruments/WidefieldLocalization.h"
#include "../../Modules/ImageViewer/ImageViewerEvents.h"
#include "../../Modules/SpectrumViewer/SpectrumViewerEvents.h"

#include "WidefieldMicroscopeWidget.h"

namespace DynExpModule::Widefield
{
	class WidefieldMicroscopeData : public DynExp::QModuleDataBase
	{
	public:
		enum class FeatureType { Widefield, WidefieldLocalization,
			SampleXYPositioning, SampleZPositioning, FocusAdjustment,
			Confocal, ConfocalOptimization, HBT,
			LEDLightToggle, PumpLightToggle, SetPumpPower, MeasurePumpPower,
			WidefieldConfocalSwitch, WidefieldConfocalIndicator, HBTSwitch,
			InterModuleCommunicator, NUM_ELEMENTS};
		enum class SetupModeType { Unknown, Widefield, Confocal };
		enum class LocalizedEmitterStateType { NotSet, Characterizing, Finished, Failed };

		struct LocalizedEmitterType
		{
			QPoint Position;
			LocalizedEmitterStateType State = LocalizedEmitterStateType::NotSet;
		};

		static const char* GetLocalizedEmitterStateString(LocalizedEmitterStateType State);
		static QColor GetLocalizedEmitterColor(LocalizedEmitterStateType State);

		using PositionType = DynExpInstr::PositionerStageData::PositionType;
		using CameraTimeType = DynExpInstr::CameraData::TimeType;
		using LocalizedPositionsMapType = std::map<Util::MarkerGraphicsView::MarkerType::IDType, LocalizedEmitterType>;
		using SPDTimeType = std::chrono::milliseconds;
		using QSurfaceDataRowsType = std::vector<std::unique_ptr<QSurfaceDataRow>>;

		struct PositionPoint
		{
			constexpr PositionPoint() noexcept
				: x(0), y(0), z(0), MeasuredX(0), MeasuredY(0), MeasuredZ(0),
				UsingX(false), UsingY(false), UsingZ(false), RowIndex(-1), ColumnIndex(-1) {}
			constexpr PositionPoint(PositionType x) noexcept
				: x(x), y(0), z(0), MeasuredX(0), MeasuredY(0), MeasuredZ(0),
				UsingX(true), UsingY(false), UsingZ(false), RowIndex(-1), ColumnIndex(-1) {}
			constexpr PositionPoint(PositionType x, PositionType y)
				: x(x), y(y), z(0), MeasuredX(0), MeasuredY(0), MeasuredZ(0),
				UsingX(true), UsingY(true), UsingZ(false), RowIndex(-1), ColumnIndex(-1) {}
			constexpr PositionPoint(PositionType x, PositionType y, PositionType z)
				: x(x), y(y), z(z), MeasuredX(0), MeasuredY(0), MeasuredZ(0),
				UsingX(true), UsingY(true), UsingZ(true), RowIndex(-1), ColumnIndex(-1) {}

			constexpr void Reset() noexcept;
			constexpr bool IsEmpty() const noexcept { return !UsingX && !UsingY && !UsingZ; }
			double DistTo(const PositionPoint& Other) const;

			std::string ToStr(std::string_view Prefix = "") const;

			PositionType x;
			PositionType y;
			PositionType z;

			PositionType MeasuredX;
			PositionType MeasuredY;
			PositionType MeasuredZ;

			bool UsingX;
			bool UsingY;
			bool UsingZ;

			int RowIndex;
			int ColumnIndex;
		};

		struct SPDStateType
		{
			constexpr SPDStateType(double Value = 0, bool Ready = false, size_t StreamSamplesWritten = 0) noexcept
				: Value(Value), Ready(Ready), StreamSamplesWritten(StreamSamplesWritten) {}

			constexpr void Reset() noexcept
			{
				Value = 0;
				Ready = false;
				StreamSamplesWritten = 0;
			}

			double Value;
			bool Ready;
			size_t StreamSamplesWritten;
		};

		WidefieldMicroscopeData() { Init(); }
		virtual ~WidefieldMicroscopeData() = default;

		auto& GetSampleStageX() noexcept { return SampleStageX; }
		auto& GetSampleStageX() const noexcept { return SampleStageX; }
		auto& GetSampleStageY() noexcept { return SampleStageY; }
		auto& GetSampleStageY() const noexcept { return SampleStageY; }
		auto& GetSampleStageZ() noexcept { return SampleStageZ; }
		auto& GetSampleStageZ() const noexcept { return SampleStageZ; }
		auto& GetSampleFocusPiezoZ() noexcept { return FocusPiezoZ; }
		auto& GetSampleFocusPiezoZ() const noexcept { return FocusPiezoZ; }
		auto& GetLEDSwitch() noexcept { return LEDSwitch; }
		auto& GetLEDSwitch() const noexcept { return LEDSwitch; }
		auto& GetPumpSwitch() noexcept { return PumpSwitch; }
		auto& GetPumpSwitch() const noexcept { return PumpSwitch; }
		auto& GetWidefieldConfocalSwitch() noexcept { return WidefieldConfocalSwitch; }
		auto& GetWidefieldConfocalSwitch() const noexcept { return WidefieldConfocalSwitch; }
		auto& GetWidefieldConfocalIndicator() noexcept { return WidefieldConfocalIndicator; }
		auto& GetWidefieldConfocalIndicator() const noexcept { return WidefieldConfocalIndicator; }
		auto& GetWidefieldHBTSwitch() noexcept { return WidefieldHBTSwitch; }
		auto& GetWidefieldHBTSwitch() const noexcept { return WidefieldHBTSwitch; }
		auto& GetPumpPower() noexcept { return PumpPower; }
		auto& GetPumpPower() const noexcept { return PumpPower; }
		auto& GetPumpPowerIndicator() noexcept { return PumpPowerIndicator; }
		auto& GetPumpPowerIndicator() const noexcept { return PumpPowerIndicator; }
		auto& GetWidefieldCamera() noexcept { return WidefieldCamera; }
		auto& GetWidefieldCamera() const noexcept { return WidefieldCamera; }
		auto& GetWidefieldLocalizer() noexcept { return WidefieldLocalizer; }
		auto& GetWidefieldLocalizer() const noexcept { return WidefieldLocalizer; }
		auto& GetSPD1() noexcept { return SPD1; }
		auto& GetSPD1() const noexcept { return SPD1; }
		auto& GetSPD2() noexcept { return SPD2; }
		auto& GetSPD2() const noexcept { return SPD2; }
		auto& GetAcqCommunicator() noexcept { return AcqCommunicator; }
		auto& GetAcqCommunicator() const noexcept { return AcqCommunicator; }

		template <size_t N>
		bool TestFeature(const std::array<FeatureType, N>& Flags) const { return Features.Test(Flags); }
		
		bool TestFeature(FeatureType Flag) const { return Features.Test(Flag); }
		void SetFeature(FeatureType Flag) { Features.Set(Flag); }

		std::string_view GetUIMessage() const noexcept { return UIMessage; }
		void SetUIMessage(const std::string& Message) { UIMessage = Message; }
		void ClearUIMessage() { UIMessage = ""; }

		PositionPoint GetSamplePosition() const;
		std::stringstream AssembleCSVHeader(bool IncludeConfocalScan, bool IncludeHBT, bool IncludeAutoMeasure) const;
		void WriteConfocalScanResults(std::stringstream& Stream) const;
		void WriteHBTResults(std::stringstream& Stream) const;

		SetupModeType GetSetupMode() const noexcept { return SetupMode; }
		void SetSetupMode(SetupModeType NewMode) noexcept { SetupMode = NewMode; }
		bool GetLEDLightTurnedOn() const noexcept { return LEDLightTurnedOn; }
		void SetLEDLightTurnedOn(bool State);
		bool GetPumpLightTurnedOn() const noexcept { return PumpLightTurnedOn; }
		void SetPumpLightTurnedOn(bool State);
		double GetMinPumpPower() const noexcept { return MinPumpPower; }
		void SetMinPumpPower(double MinPower) noexcept { MinPumpPower = MinPower; }
		double GetMaxPumpPower() const noexcept { return MaxPumpPower; }
		void SetMaxPumpPower(double MaxPower) noexcept { MaxPumpPower = MaxPower; }
		double GetWidefieldPumpPower() const noexcept { return WidefieldPumpPower; }
		void SetWidefieldPumpPower(double Power) noexcept { WidefieldPumpPower = Power; }
		double GetConfocalPumpPower() const noexcept { return ConfocalPumpPower; }
		void SetConfocalPumpPower(double Power) noexcept { ConfocalPumpPower = Power; }
		double GetMeasuredPumpPower() const noexcept { return MeasuredPumpPower; }
		void SetMeasuredPumpPower(double MeasuredPower) noexcept { MeasuredPumpPower = MeasuredPower; }
		double GetMinFocusVoltage() const noexcept { return MinFocusVoltage; }
		void SetMinFocusVoltage(double MinVoltage) noexcept { MinFocusVoltage = MinVoltage; }
		double GetMaxFocusVoltage() const noexcept { return MaxFocusVoltage; }
		void SetMaxFocusVoltage(double MaxVoltage) noexcept { MaxFocusVoltage = MaxVoltage; }
		double GetFocusCurrentVoltage() const noexcept { return FocusCurrentVoltage; }
		void SetFocusCurrentVoltage(double CurrentVoltage) noexcept { FocusCurrentVoltage = CurrentVoltage; }
		double GetFocusZeroVoltage() const noexcept { return FocusZeroVoltage; }
		void SetFocusZeroVoltage(double ZeroVoltage) noexcept { FocusZeroVoltage = ZeroVoltage; }
		double GetFocusConfocalOffsetVoltage() const noexcept { return FocusConfocalOffsetVoltage; }
		void SetFocusConfocalOffsetVoltage(double ConfocalOffsetVoltage) noexcept { FocusConfocalOffsetVoltage = ConfocalOffsetVoltage; }
		bool IsAutofocusFinished() const noexcept { return AutofocusFinished; }
		void SetAutofocusFinished() noexcept { AutofocusFinished = true; }
		void ResetAutofocusFinished() noexcept { AutofocusFinished = false; }

		CameraTimeType GetMinCameraExposureTime() const noexcept { return MinCameraExposureTime; }
		void SetMinCameraExposureTime(CameraTimeType Time) noexcept { MinCameraExposureTime = Time; }
		CameraTimeType GetMaxCameraExposureTime() const noexcept { return MaxCameraExposureTime; }
		void SetMaxCameraExposureTime(CameraTimeType Time) noexcept { MaxCameraExposureTime = Time; }
		CameraTimeType GetLEDCameraExposureTime() const noexcept { return LEDCameraExposureTime; }
		void SetLEDCameraExposureTime(CameraTimeType Time) noexcept { LEDCameraExposureTime = Time; }
		CameraTimeType GetWidefieldCameraExposureTime() const noexcept { return WidefieldCameraExposureTime; }
		void SetWidefieldCameraExposureTime(CameraTimeType Time) noexcept { WidefieldCameraExposureTime = Time; }
		QPoint GetConfocalSpotImagePosition() const noexcept { return ConfocalSpotImagePosition; }
		void SetConfocalSpotImagePosition(const QPoint& Position) noexcept { ConfocalSpotImagePosition = Position; }
		const PositionPoint& GetWidefieldPosition() const noexcept { return WidefieldPosition; }
		void SetWidefieldPosition(const PositionPoint& Position) noexcept { WidefieldPosition = Position; }

		const QImage& GetCurrentImage() const noexcept { return CurrentImage; }
		void SetCurrentImage(QImage&& Image) { CurrentImage = std::move(Image); CurrentImageChanged = true; }
		bool IsCurrentImageAvlbl() const { return !CurrentImage.isNull() && CurrentImageChanged; }
		void ResetCurrentImageAvlbl() noexcept { CurrentImageChanged = false; }
		const auto& GetCellID() const noexcept { return CurrentCellID; }
		const auto& GetLastCellID() const noexcept { return LastCellID; }
		void SetCellID(const DynExpInstr::WidefieldLocalizationCellIDType& CellID) { LastCellID = CurrentCellID; CurrentCellID = CellID; }
		void SetCellIDToLastCellID() { CurrentCellID = LastCellID; }
		void IncrementCellID();
		void ResetCellID() { CurrentCellID = {}; LastCellID = {}; }
		bool HasCellID() const noexcept { return CurrentCellID.Valid; }
		auto& GetLocalizedPositions() noexcept { return LocalizedPositions; }
		const auto& GetLocalizedPositions() const noexcept { return LocalizedPositions; }
		size_t GetNumFinishedLocalizedPositions() const;
		size_t GetNumFailedLocalizedPositions() const;
		void AppendLocalizedPosition(LocalizedPositionsMapType::value_type&& Position);
		void SetLocalizedPositions(LocalizedPositionsMapType&& Positions);
		void ClearLocalizedPositions();
		bool HaveLocalizedPositionsChanged() const noexcept { return LocalizedPositionsChanged; }
		void ResetLocalizedPositionsChanged() noexcept { LocalizedPositionsChanged = false; }
		void SetLocalizedPositionsStateChanged() { LocalizedPositionsStateChanged = true; }
		void ClearLocalizedPositionsStateChanged() { LocalizedPositionsStateChanged = false; }
		bool HaveLocalizedPositionsStateChanged() const noexcept { return LocalizedPositionsStateChanged; }

		const PositionPoint& GetSampleHomePosition() const noexcept { return SampleHomePosition; }
		void SetSampleHomePosition(const PositionPoint& Position) noexcept { SampleHomePosition = Position; }
		int GetConfocalScanWidth() const noexcept { return ConfocalScanWidth; }
		void SetConfocalScanWidth(int Width) noexcept { ConfocalScanWidth = Width; }
		int GetConfocalScanHeight() const noexcept { return ConfocalScanHeight; }
		void SetConfocalScanHeight(int Length) noexcept { ConfocalScanHeight = Length; }
		int GetConfocalScanDistPerPixel() const noexcept { return ConfocalScanDistPerPixel; }
		void SetConfocalScanDistPerPixel(int DistPerPixel) noexcept { ConfocalScanDistPerPixel = DistPerPixel; }
		SPDTimeType GetSPDExposureTime() const noexcept { return SPDExposureTime; }
		void SetSPDExposureTime(SPDTimeType Time) noexcept { SPDExposureTime = Time; }
		auto GetSPD1SamplesWritten() const noexcept { return SPD1State.StreamSamplesWritten; }
		bool GetSPD1Ready() const noexcept { return SPD1State.Ready; }
		auto GetSPD1Value() const noexcept { return SPD1State.Value; }
		void SetSPD1SamplesWritten(size_t SamplesWritten) noexcept { SPD1State.StreamSamplesWritten = SamplesWritten; }
		void SetSPD1Ready(double Value) noexcept { SPD1State.Value = Value; SPD1State.Ready = true; }
		void ResetSPD1State() noexcept { SPD1State.Reset(); }
		auto GetSPD2SamplesWritten() const noexcept { return SPD2State.StreamSamplesWritten; }
		bool GetSPD2Ready() const noexcept { return SPD2State.Ready; }
		auto GetSPD2Value() const noexcept { return SPD2State.Value; }
		void SetSPD2SamplesWritten(size_t SamplesWritten) noexcept { SPD2State.StreamSamplesWritten = SamplesWritten; }
		void SetSPD2Ready(double Value) noexcept { SPD2State.Value = Value; SPD2State.Ready = true; }
		void ResetSPD2State() noexcept { SPD2State.Reset(); }

		auto& GetConfocalScanResults() noexcept { return ConfocalScanResults; }
		const auto& GetConfocalScanResults() const noexcept { return ConfocalScanResults; }
		void ClearConfocalScanResults() { ConfocalScanResults.clear(); }
		auto GetConfocalScanSurfacePlotRows() noexcept { return std::move(ConfocalScanSurfacePlotRows); }
		void SetConfocalScanSurfacePlotRows(QSurfaceDataRowsType&& QSurfaceDataRows) noexcept { ConfocalScanSurfacePlotRows = std::move(QSurfaceDataRows); }
		bool HasConfocalScanSurfacePlotRows() const noexcept { return !ConfocalScanSurfacePlotRows.empty(); }
		void ClearConfocalScanSurfacePlotRows() { ConfocalScanSurfacePlotRows.clear(); }

		double GetConfocalOptimizationInitXYStepSize() const noexcept { return ConfocalOptimizationInitXYStepSize; }
		void SetConfocalOptimizationInitXYStepSize(double StepSize) noexcept { ConfocalOptimizationInitXYStepSize = StepSize; }
		double GetConfocalOptimizationInitZStepSize() const noexcept { return ConfocalOptimizationInitZStepSize; }
		void SetConfocalOptimizationInitZStepSize(double StepSize) noexcept { ConfocalOptimizationInitZStepSize = StepSize; }
		double GetConfocalOptimizationTolerance() const noexcept { return ConfocalOptimizationTolerance; }
		void SetConfocalOptimizationTolerance(double Tolerance) noexcept { ConfocalOptimizationTolerance = Tolerance; }
		double GetLastCountRate() const noexcept { return LastCountRate; }
		void SetLastCountRate(double CountRate) noexcept { LastCountRate = CountRate; }

		auto GetHBTBinWidth() const noexcept { return HBTBinWidth; }
		void SetHBTBinWidth(Util::picoseconds BinWidth) noexcept { HBTBinWidth = BinWidth; }
		auto GetHBTBinCount() const noexcept { return HBTBinCount; }
		void SetHBTBinCount(size_t BinCount) noexcept { HBTBinCount = BinCount; }
		auto GetHBTMaxIntegrationTime() const noexcept { return HBTMaxIntegrationTime; }
		void SetHBTMaxIntegrationTime(std::chrono::microseconds MaxIntegrationTime) noexcept { HBTMaxIntegrationTime = MaxIntegrationTime; }
		const PositionPoint& GetHBTSamplePosition() const noexcept { return HBTSamplePosition; }
		void SetHBTSamplePosition(const PositionPoint& Position) noexcept { HBTSamplePosition = Position; }
		auto GetHBTNumEventCounts() const noexcept { return HBTNumEventCounts; }
		void SetHBTNumEventCounts(long long NumEventCounts) noexcept { HBTNumEventCounts = NumEventCounts; }
		auto GetHBTTotalIntegrationTime() const noexcept { return HBTTotalIntegrationTime; }
		void SetHBTTotalIntegrationTime(std::chrono::microseconds TotalIntegrationTime) noexcept { HBTTotalIntegrationTime = TotalIntegrationTime; }
		auto& GetHBTDataPoints() noexcept { return HBTDataPoints; }
		const auto& GetHBTDataPoints() const noexcept { return HBTDataPoints; }
		void ClearHBTDataPoints() { HBTDataPoints.clear(); }
		auto GetHBTDataPointsMinValues() const noexcept { return HBTDataPointsMinValues; }
		void SetHBTDataPointsMinValues(const QPointF& DataPointsMinValues) noexcept { HBTDataPointsMinValues = DataPointsMinValues; }
		auto GetHBTDataPointsMaxValues() const noexcept { return HBTDataPointsMaxValues; }
		void SetHBTDataPointsMaxValues(const QPointF& DataPointsMaxValues) noexcept { HBTDataPointsMaxValues = DataPointsMaxValues; }

		bool IsAutoMeasureRunning() const noexcept { return AutoMeasureRunning; }
		void SetAutoMeasureRunning(bool Running) noexcept { AutoMeasureRunning = Running; }
		std::filesystem::path GetAutoMeasureSavePath() const;
		void SetAutoMeasureSavePath(std::filesystem::path SavePath) noexcept { AutoMeasureSavePath = SavePath; }
		const PositionPoint& GetAutoMeasureCurrentCellPosition() const noexcept { return AutoMeasureCurrentCellPosition; }
		void SetAutoMeasureCurrentCellPosition(const PositionPoint& Position) noexcept { AutoMeasureCurrentCellPosition = Position; }
		auto GetAutoMeasureNumberImageSets() const noexcept { return AutoMeasureNumberImageSets; }
		void SetAutoMeasureNumberImageSets(int NumberImageSets) noexcept { AutoMeasureNumberImageSets = NumberImageSets; }
		auto GetAutoMeasureCurrentImageSet() const noexcept { return AutoMeasureCurrentImageSet; }
		auto IncrementAutoMeasureCurrentImageSet() noexcept { return ++AutoMeasureCurrentImageSet; }
		void ResetAutoMeasureCurrentImageSet() noexcept { AutoMeasureCurrentImageSet = -1; }
		std::chrono::seconds GetAutoMeasureInitialImageSetWaitTime() const noexcept { return AutoMeasureInitialImageSetWaitTime; }
		void SetAutoMeasureInitialImageSetWaitTime(std::chrono::seconds Time) noexcept { AutoMeasureInitialImageSetWaitTime = Time; }
		int GetAutoMeasureImagePositionScatterRadius() const noexcept { return AutoMeasureImagePositionScatterRadius; }
		void SetAutoMeasureImagePositionScatterRadius(int ScatterRadius) noexcept { AutoMeasureImagePositionScatterRadius = ScatterRadius; }
		auto GetAutoMeasureLocalizationType() const noexcept { return AutoMeasureLocalizationType; }
		void SetAutoMeasureLocalizationType(WidefieldMicroscopeWidget::LocalizationType LocalizationType) noexcept { AutoMeasureLocalizationType = LocalizationType; }
		auto GetAutoMeasureOptimizeEnabled() const noexcept { return AutoMeasureOptimizeEnabled; }
		void SetAutoMeasureOptimizeEnabled(bool Enabled) noexcept { AutoMeasureOptimizeEnabled = Enabled; }
		auto GetAutoMeasureSpectrumEnabled() const noexcept { return AutoMeasureSpectrumEnabled; }
		void SetAutoMeasureSpectrumEnabled(bool Enabled) noexcept { AutoMeasureSpectrumEnabled = Enabled; }
		auto GetAutoMeasureHBTEnabled() const noexcept { return AutoMeasureHBTEnabled; }
		void SetAutoMeasureHBTEnabled(bool Enabled) noexcept { AutoMeasureHBTEnabled = Enabled; }
		auto GetAutoMeasureNumOptimizationAttempts() const noexcept { return AutoMeasureNumOptimizationAttempts; }
		void SetAutoMeasureNumOptimizationAttempts(int NumOptimizationAttempts) noexcept { AutoMeasureNumOptimizationAttempts = NumOptimizationAttempts; }
		auto GetAutoMeasureCurrentOptimizationAttempt() const noexcept { return AutoMeasureCurrentOptimizationAttempt; }
		auto IncrementAutoMeasureCurrentOptimizationAttempt() noexcept { return ++AutoMeasureCurrentOptimizationAttempt; }
		void ResetAutoMeasureCurrentOptimizationAttempt() noexcept { AutoMeasureCurrentOptimizationAttempt = -1; }
		auto GetAutoMeasureMaxOptimizationReruns() const noexcept { return AutoMeasureMaxOptimizationReruns; }
		void SetAutoMeasureMaxOptimizationReruns(int MaxOptimizationReruns) noexcept { AutoMeasureMaxOptimizationReruns = MaxOptimizationReruns; }
		auto GetAutoMeasureCurrentOptimizationRerun() const noexcept { return AutoMeasureCurrentOptimizationRerun; }
		auto IncrementAutoMeasureCurrentOptimizationRerun() noexcept { return ++AutoMeasureCurrentOptimizationRerun; }
		void ResetAutoMeasureCurrentOptimizationRerun() noexcept { AutoMeasureCurrentOptimizationRerun = -1; }
		auto GetAutoMeasureOptimizationMaxDistance() const noexcept { return AutoMeasureOptimizationMaxDistance; }
		void SetAutoMeasureOptimizationMaxDistance(int OptimizationMaxDistance) noexcept { AutoMeasureOptimizationMaxDistance = OptimizationMaxDistance; }
		auto GetAutoMeasureCountRateThreshold() const noexcept { return AutoMeasureCountRateThreshold; }
		void SetAutoMeasureCountRateThreshold(int CountRateThreshold) noexcept { AutoMeasureCountRateThreshold = CountRateThreshold; }
		auto& GetAutoMeasureCellRangeFrom() const noexcept { return AutoMeasureCellRangeFrom; }
		auto& GetAutoMeasureCellRangeFrom() noexcept { return AutoMeasureCellRangeFrom; }
		auto& GetAutoMeasureCellRangeTo() const noexcept { return AutoMeasureCellRangeTo; }
		auto& GetAutoMeasureCellRangeTo() noexcept { return AutoMeasureCellRangeTo; }
		bool IsCellRangeValid() const noexcept;
		int GetAutoMeasureCellLineLength() const noexcept;
		int GetAutoMeasureCellColumnLength() const noexcept;
		int GetAutoMeasureCellCount() const noexcept;
		int GetAutoMeasureCurrentCellIndex() const;
		auto& GetAutoMeasureCellSkip() const noexcept { return AutoMeasureCellSkip; }
		auto& GetAutoMeasureCellSkip() noexcept { return AutoMeasureCellSkip; }
		auto GetAutoMeasureFirstEmitter() const noexcept { return AutoMeasureFirstEmitter; }
		bool SetAutoMeasureFirstEmitter(Util::MarkerGraphicsView::MarkerType::IDType FirstEmitterID) noexcept;	//!< Returns true in case of success, false otherwise.
		auto GetAutoMeasureCurrentEmitter() const noexcept { return AutoMeasureCurrentEmitter; }
		void ResetAutoMeasureCurrentEmitter() noexcept;
		auto IncrementAutoMeasureCurrentEmitter() noexcept { return ++AutoMeasureCurrentEmitter; }

	private:
		void ResetImpl(dispatch_tag<QModuleDataBase>) override final;
		virtual void ResetImpl(dispatch_tag<WidefieldMicroscopeData>) {};

		void Init();

		DynExp::LinkedObjectWrapperContainer<DynExpInstr::PositionerStage> SampleStageX;
		DynExp::LinkedObjectWrapperContainer<DynExpInstr::PositionerStage> SampleStageY;
		DynExp::LinkedObjectWrapperContainer<DynExpInstr::PositionerStage> SampleStageZ;
		DynExp::LinkedObjectWrapperContainer<DynExpInstr::AnalogOut> FocusPiezoZ;
		DynExp::LinkedObjectWrapperContainer<DynExpInstr::DigitalOut> LEDSwitch;
		DynExp::LinkedObjectWrapperContainer<DynExpInstr::DigitalOut> PumpSwitch;
		DynExp::LinkedObjectWrapperContainer<DynExpInstr::DigitalOut> WidefieldConfocalSwitch;
		DynExp::LinkedObjectWrapperContainer<DynExpInstr::DigitalIn> WidefieldConfocalIndicator;
		DynExp::LinkedObjectWrapperContainer<DynExpInstr::FunctionGenerator> WidefieldHBTSwitch;
		DynExp::LinkedObjectWrapperContainer<DynExpInstr::AnalogOut> PumpPower;
		DynExp::LinkedObjectWrapperContainer<DynExpInstr::AnalogIn> PumpPowerIndicator;
		DynExp::LinkedObjectWrapperContainer<DynExpInstr::Camera> WidefieldCamera;
		DynExp::LinkedObjectWrapperContainer<DynExpInstr::WidefieldLocalization> WidefieldLocalizer;
		DynExp::LinkedObjectWrapperContainer<DynExpInstr::TimeTagger> SPD1;
		DynExp::LinkedObjectWrapperContainer<DynExpInstr::TimeTagger> SPD2;
		DynExp::LinkedObjectWrapperContainer<DynExpInstr::InterModuleCommunicator> AcqCommunicator;

		Util::FeatureTester<FeatureType> Features;
		std::string UIMessage;

		// General
		SetupModeType SetupMode;
		bool LEDLightTurnedOn;
		bool PumpLightTurnedOn;
		double MinPumpPower;
		double MaxPumpPower;
		double WidefieldPumpPower;
		double ConfocalPumpPower;
		double MeasuredPumpPower;
		double MinFocusVoltage;
		double MaxFocusVoltage;
		double FocusCurrentVoltage;
		double FocusZeroVoltage;
		double FocusConfocalOffsetVoltage;
		bool AutofocusFinished;

		// Widefield imaging
		CameraTimeType MinCameraExposureTime;
		CameraTimeType MaxCameraExposureTime;
		CameraTimeType LEDCameraExposureTime;
		CameraTimeType WidefieldCameraExposureTime;
		QPoint ConfocalSpotImagePosition;				//!< Location of confocal spot in units of px within widefield image.
		PositionPoint WidefieldPosition;				//!< Location where the current widefield image has been taken in nm.

		QImage CurrentImage;
		bool CurrentImageChanged;
		DynExpInstr::WidefieldLocalizationCellIDType CurrentCellID;
		DynExpInstr::WidefieldLocalizationCellIDType LastCellID;
		LocalizedPositionsMapType LocalizedPositions;
		bool LocalizedPositionsChanged;
		bool LocalizedPositionsStateChanged;

		// Confocal scanning
		PositionPoint SampleHomePosition;				//!< Location set by the user as a home position in nm.
		int ConfocalScanWidth;
		int ConfocalScanHeight;
		int ConfocalScanDistPerPixel;
		SPDTimeType SPDExposureTime;
		SPDStateType SPD1State;
		SPDStateType SPD2State;

		// Confocal optimization
		double ConfocalOptimizationInitXYStepSize;
		double ConfocalOptimizationInitZStepSize;
		double ConfocalOptimizationTolerance;
		double LastCountRate;

		// vector of pairs <sample stage positions in nm, count rate in Hz>
		std::vector<std::pair<PositionPoint, double>> ConfocalScanResults;
		QSurfaceDataRowsType ConfocalScanSurfacePlotRows;

		// HBT
		Util::picoseconds HBTBinWidth;
		size_t HBTBinCount;
		std::chrono::microseconds HBTMaxIntegrationTime;
		PositionPoint HBTSamplePosition;				//!< Location in nm where the HBT measurement has been started.
		long long HBTNumEventCounts;
		std::chrono::microseconds HBTTotalIntegrationTime;

		// x is time in ps, y is g2.
		QList<QPointF> HBTDataPoints;
		QPointF HBTDataPointsMinValues;
		QPointF HBTDataPointsMaxValues;

		// Auto measure
		bool AutoMeasureRunning;
		std::filesystem::path AutoMeasureSavePath;
		PositionPoint AutoMeasureCurrentCellPosition;	//!< Center position of the current cell in nm where an automated measurement takes place.
		int AutoMeasureNumberImageSets;
		int AutoMeasureCurrentImageSet;					//!< Index of the image set being recorded if auto-measure localization is running, -1 otherwise.
		std::chrono::seconds AutoMeasureInitialImageSetWaitTime;
		int AutoMeasureImagePositionScatterRadius;
		WidefieldMicroscopeWidget::LocalizationType AutoMeasureLocalizationType;
		bool AutoMeasureOptimizeEnabled;
		bool AutoMeasureSpectrumEnabled;
		bool AutoMeasureHBTEnabled;
		int AutoMeasureNumOptimizationAttempts;
		int AutoMeasureCurrentOptimizationAttempt;
		int AutoMeasureMaxOptimizationReruns;
		int AutoMeasureCurrentOptimizationRerun;
		int AutoMeasureOptimizationMaxDistance;
		int AutoMeasureCountRateThreshold;
		QPoint AutoMeasureCellRangeFrom;
		QPoint AutoMeasureCellRangeTo;
		QPoint AutoMeasureCellSkip;
		LocalizedPositionsMapType::iterator AutoMeasureFirstEmitter;	//!< Iterator to the first emitter to be characterized.
		LocalizedPositionsMapType::iterator AutoMeasureCurrentEmitter;	//!< Iterator to the emitter being characterized.
	};

	WidefieldMicroscopeData::PositionPoint operator+(const WidefieldMicroscopeData::PositionPoint& lhs, const WidefieldMicroscopeData::PositionPoint& rhs);
	WidefieldMicroscopeData::PositionPoint operator-(const WidefieldMicroscopeData::PositionPoint& lhs, const WidefieldMicroscopeData::PositionPoint& rhs);

	class WidefieldMicroscopeParams : public DynExp::QModuleParamsBase
	{
	public:
		WidefieldMicroscopeParams(DynExp::ItemIDType ID, const DynExp::DynExpCore& Core) : QModuleParamsBase(ID, Core) {}
		virtual ~WidefieldMicroscopeParams() = default;

		virtual const char* GetParamClassTag() const noexcept override { return "WidefieldMicroscopeParams"; }

		Param<DynExp::ObjectLink<DynExpInstr::PositionerStage>> SampleStageX = { *this, GetCore().GetInstrumentManager(),
			"SampleStageX", "Sample stage X", "Positioner to move sample along x coordinate", DynExpUI::Icons::Instrument, true };
		Param<DynExp::ObjectLink<DynExpInstr::PositionerStage>> SampleStageY = { *this, GetCore().GetInstrumentManager(),
			"SampleStageY", "Sample stage Y", "Positioner to move sample along y coordinate", DynExpUI::Icons::Instrument, true };
		Param<DynExp::ObjectLink<DynExpInstr::PositionerStage>> SampleStageZ = { *this, GetCore().GetInstrumentManager(),
			"SampleStageZ", "Sample stage Z", "Positioner to move sample along z coordinate", DynExpUI::Icons::Instrument, true };
		Param<DynExp::ObjectLink<DynExpInstr::AnalogOut>> FocusPiezoZ = { *this, GetCore().GetInstrumentManager(),
			"FocusPiezoZ", "Focusing piezo (AO)", "Piezo to adjust the focus of the microscope", DynExpUI::Icons::Instrument, true };
		Param<DynExp::ObjectLink<DynExpInstr::DigitalOut>> LEDSwitch = { *this, GetCore().GetInstrumentManager(),
			"LEDSwitch", "LED switch (DO)", "Digital switch to turn the LED light source on and off", DynExpUI::Icons::Instrument, true };
		Param<DynExp::ObjectLink<DynExpInstr::DigitalOut>> PumpSwitch = { *this, GetCore().GetInstrumentManager(),
			"PumpSwitch", "Pump switch (DO)", "Digital switch to turn the pump light source on and off", DynExpUI::Icons::Instrument, true };
		Param<DynExp::ObjectLink<DynExpInstr::DigitalOut>> WidefieldConfocalSwitch = { *this, GetCore().GetInstrumentManager(),
			"WidefieldConfocalSwitch", "Widefield/confocal mode switch (DO)",
			"Digital switch to change from widefield mode (LOW) to confocal mode (HIGH)", DynExpUI::Icons::Instrument, true };
		Param<DynExp::ObjectLink<DynExpInstr::DigitalIn>> WidefieldConfocalIndicator = { *this, GetCore().GetInstrumentManager(),
			"WidefieldConfocalIndicator", "Widefield/confocal mode indicator (DI)",
			"Digital indicator which is LOW if in widefield mode and HIGH if in confocal mode", DynExpUI::Icons::Instrument, true };
		Param<ParamsConfigDialog::NumberType> WidefieldConfocalTransitionTime = { *this, "WidefieldConfocalTransitionTime",
			"Widefield/confocal transition time (ms)",
			"Time it takes to transition from widefield into confocal mode or vice versa once the widefield/confocal mode switch has been triggered",
			false, 500, 0, 10000, 10, 0 };
		Param<DynExp::ObjectLink<DynExpInstr::FunctionGenerator>> WidefieldHBTSwitch = { *this, GetCore().GetInstrumentManager(),
			"WidefieldHBTSwitch", "HBT flip mirror servo actuator (DO)", "Servo actuator to switch to HBT measurement mode", DynExpUI::Icons::Instrument, true };
		Param<ParamsConfigDialog::NumberType> WidefieldHBTSwitchLowDutyCycle = { *this, "WidefieldHBTSwitchLowDutyCycle",
			"HBT flip mirror low duty cycle",
			"Duty cycle of rectangular pulses applied to the HBT flip mirror servo actuator in order to make it flip the mirror into the low position",
			false, .2, 0, 1, .1, 2 };
		Param<ParamsConfigDialog::NumberType> WidefieldHBTSwitchHighDutyCycle = { *this, "WidefieldHBTSwitchHighDutyCycle",
			"HBT flip mirror high duty cycle",
			"Duty cycle of rectangular pulses applied to the HBT flip mirror servo actuator in order to make it flip the mirror into the high position",
			false, .8, 0, 1, .1, 2 };
		Param<ParamsConfigDialog::NumberType> WidefieldHBTTransitionTime = { *this, "WidefieldHBTTransitionTime",
			"HBT flip mirror transition time (ms)",
			"Time it takes to flip the HBT mirror once the duty cycle of the rectangular pulses applied to the flip mirror servo actuator has changed",
			false, 500, 0, 10000, 10, 0 };
		Param<DynExp::ObjectLink<DynExpInstr::AnalogOut>> PumpPower = { *this, GetCore().GetInstrumentManager(),
			"PumpPower", "Pump power (AO)", "Analog output to adjust the power of the pump light source", DynExpUI::Icons::Instrument, true };
		Param<DynExp::ObjectLink<DynExpInstr::AnalogIn>> PumpPowerIndicator = { *this, GetCore().GetInstrumentManager(),
			"PumpPowerIndicator", "Pump power indicator (AI)", "Analog input to measure the current power of the pump light source",
			DynExpUI::Icons::Instrument, true };
		Param<ParamsConfigDialog::NumberType> DefaultPowerWidefieldMode = { *this, "DefaultPowerWidefieldMode", "Default pump power (widefield mode)",
			"Defines the pump power in units of the pump power (AO) parameter which is initially used in the widefield mode",
			true, 1, 0, std::numeric_limits<ParamsConfigDialog::NumberType>::max(), .1, 3 };
		Param<ParamsConfigDialog::NumberType> DefaultPowerConfocalMode = { *this, "DefaultPowerConfocalMode", "Default pump power (confocal mode)",
			"Defines the pump power in units of the pump power (AO) parameter which is initially used in the confocal mode",
			true, .1, 0, std::numeric_limits<ParamsConfigDialog::NumberType>::max(), .1, 3 };
		Param<DynExp::ObjectLink<DynExpInstr::Camera>> WidefieldCamera = { *this, GetCore().GetInstrumentManager(),
			"WidefieldCamera", "Widefield camera", "Camera to record widefield images of the sample", DynExpUI::Icons::Instrument, true };
		Param<DynExp::ObjectLink<DynExpInstr::WidefieldLocalization>> WidefieldLocalizer = { *this, GetCore().GetInstrumentManager(),
			"WidefieldLocalizer", "Widefield localizer", "gRPC service to perform widefield localization of emitters within a recorded camera image",
			DynExpUI::Icons::Instrument, true };
		Param<ParamsConfigDialog::NumberType> WidefieldCameraMagnification = { *this, "WidefieldCameraMagnification", "Widefield camera magnification factor",
			"Magnification factor of the optical system between the sample and the widefield camera",
			false, 100, .01, 10000, 1, 2 };
		Param<DynExp::ObjectLink<DynExpInstr::TimeTagger>> SPD1 = { *this, GetCore().GetInstrumentManager(),
			"SPD1", "SPD 1", "First single photon detector for confocal light collection", DynExpUI::Icons::Instrument, true };
		Param<DynExp::ObjectLink<DynExpInstr::TimeTagger>> SPD2 = { *this, GetCore().GetInstrumentManager(),
			"SPD2", "SPD 2", "Second single photon detector for confocal light collection", DynExpUI::Icons::Instrument, true };
		Param<DynExp::ObjectLink<DynExpInstr::InterModuleCommunicator>> AcqCommunicator = { *this, GetCore().GetInstrumentManager(),
			"AcqInterModuleCommunicator", "Acq. inter-module communicator", "Inter-module communicator to control image and spectrum capturing modules with", DynExpUI::Icons::Instrument, true };

	private:
		void ConfigureParamsImpl(dispatch_tag<QModuleParamsBase>) override final {}
	};

	class WidefieldMicroscopeConfigurator : public DynExp::QModuleConfiguratorBase
	{
	public:
		using ObjectType = WidefieldMicroscope;
		using ParamsType = WidefieldMicroscopeParams;

		WidefieldMicroscopeConfigurator() = default;
		virtual ~WidefieldMicroscopeConfigurator() = default;

	private:
		virtual DynExp::ParamsBasePtrType MakeParams(DynExp::ItemIDType ID, const DynExp::DynExpCore& Core) const override final { return DynExp::MakeParams<WidefieldMicroscopeConfigurator>(ID, Core); }
	};

	class WidefieldMicroscope : public DynExp::QModuleBase
	{
		enum class PositionerStateType { WaitingForMovement, Moving, Arrived };
		enum class WidefieldImageProcessingStateType { Waiting, Finished, Failed };

		using AtomicPositionerStateType = std::atomic<PositionerStateType>;
		using AtomicWidefieldImageProcessingStateType = std::atomic<WidefieldImageProcessingStateType>;

	public:
		using ParamsType = WidefieldMicroscopeParams;
		using ConfigType = WidefieldMicroscopeConfigurator;
		using ModuleDataType = WidefieldMicroscopeData;

		constexpr static auto Name() noexcept { return "Widefield Microscope"; }
		constexpr static auto Category() noexcept { return "Experiments"; }

		WidefieldMicroscope(const std::thread::id OwnerThreadID, DynExp::ParamsBasePtrType&& Params);
		virtual ~WidefieldMicroscope();

		virtual std::string GetName() const override { return Name(); }
		virtual std::string GetCategory() const override { return Category(); }

		std::chrono::milliseconds GetMainLoopDelay() const override final;

		// Events which the UI thread might enqueue.
		void OnSaveCurrentImage(DynExp::ModuleInstance* Instance, QString Filename) const;
		void OnGoToSamplePos(DynExp::ModuleInstance* Instance, QPointF SamplePos) const;
		void OnBringMarkerToConfocalSpot(DynExp::ModuleInstance* Instance, QPoint MarkerPos, QPointF SamplePos) const;
		void OnRunCharacterizationFromID(DynExp::ModuleInstance* Instance, Util::MarkerGraphicsView::MarkerType::IDType ID) const;

	private:
		Util::DynExpErrorCodes::DynExpErrorCodes ModuleMainLoop(DynExp::ModuleInstance& Instance) override final;

		void ResetImpl(dispatch_tag<QModuleBase>) override final;

		std::unique_ptr<DynExp::QModuleWidget> MakeUIWidget() override final;
		void UpdateUIChild(const ModuleBase::ModuleDataGetterType& ModuleDataGetter) override final;

		// Helper functions
		// ->
		template <typename PosT>
		static ModuleDataType::PositionPoint PositionPointFromPixelDist(Util::SynchronizedPointer<const ParamsType>& ModuleParams, Util::SynchronizedPointer<ModuleDataType>& ModuleData,
			const PosT x, const PosT y)
		{
			return {
				Util::NumToT<WidefieldMicroscopeData::PositionType>(x * ModuleData->GetWidefieldCamera()->GetPixelSizeInMicrons() * 1000 / ModuleParams->WidefieldCameraMagnification),
				Util::NumToT<WidefieldMicroscopeData::PositionType>(y * ModuleData->GetWidefieldCamera()->GetPixelSizeInMicrons() * 1000 / ModuleParams->WidefieldCameraMagnification)
			};
		}

		bool IsReadyState() const noexcept { return StateMachine.GetCurrentState()->GetState() == StateType::Ready; }
		StateType ResetState(Util::SynchronizedPointer<ModuleDataType>& ModuleData) const;
		bool IsCharacterizingSample() const noexcept;	//!< Returns true if an entire sample (multiple cells) are processed, false if only a single cell is processed.
		void MoveSampleTo(const ModuleDataType::PositionPoint& Point, Util::SynchronizedPointer<ModuleDataType>& ModuleData) const;
		bool IsSampleMoving(Util::SynchronizedPointer<ModuleDataType>& ModuleData) const;
		void SetFocus(Util::SynchronizedPointer<ModuleDataType>& ModuleData, double Voltage, bool IgnoreOffset = false) const;
		ModuleDataType::PositionPoint CalcMarkerToConfocalSpotDestiny(Util::SynchronizedPointer<const ParamsType>& ModuleParams, Util::SynchronizedPointer<ModuleDataType>& ModuleData,
			QPoint MarkerPos, QPointF SamplePos) const;
		void BringMarkerToConfocalSpot(Util::SynchronizedPointer<const ParamsType>& ModuleParams, Util::SynchronizedPointer<ModuleDataType>& ModuleData,
			QPoint MarkerPos, QPointF SamplePos) const;
		ModuleDataType::QSurfaceDataRowsType CalculateConfocalScanPositions(const int Width, const int Height,
			const int DistPerPixel, const WidefieldMicroscopeData::PositionPoint CenterPosition) const;
		void UpdatePumpPower(Util::SynchronizedPointer<ModuleDataType>& ModuleData) const;
		void PrepareImageRecording(Util::SynchronizedPointer<ModuleDataType>& ModuleData) const;
		void RecordImage(Util::SynchronizedPointer<ModuleDataType>& ModuleData) const;
		StateType InitiateReadCellIDFromImage(Util::SynchronizedPointer<ModuleDataType>& ModuleData) const;
		StateType InitiateLocalizationFromImage(Util::SynchronizedPointer<ModuleDataType>& ModuleData) const;
		void PrepareAPDsForConfocalMode(Util::SynchronizedPointer<ModuleDataType>& ModuleData) const;
		void InitializeConfocalOptimizer(Util::SynchronizedPointer<ModuleDataType>& ModuleData) const;
		void SetHBTSwitch(Util::SynchronizedPointer<const ParamsType>& ModuleParams,
			Util::SynchronizedPointer<ModuleDataType>& ModuleData, bool IsHBTMode) const;
		void InitializeHBT(Util::SynchronizedPointer<ModuleDataType>& ModuleData) const;
		void StopHBT(Util::SynchronizedPointer<ModuleDataType>& ModuleData) const;
		StateType StartAutofocus(Util::SynchronizedPointer<ModuleDataType>& ModuleData) const;
		StateType StartAutoMeasureLocalization(Util::SynchronizedPointer<ModuleDataType>& ModuleData) const;
		StateType StartAutoMeasureCharacterization(Util::SynchronizedPointer<ModuleDataType>& ModuleData, Util::MarkerGraphicsView::MarkerType::IDType FirstEmitterID = -1) const;
		StateType StartAutoMeasureSampleCharacterization(Util::SynchronizedPointer<ModuleDataType>& ModuleData) const;
		std::filesystem::path BuildFilename(Util::SynchronizedPointer<ModuleDataType>& ModuleData, std::string_view FilenameSuffix) const;
		ModuleDataType::PositionPoint RandomPointInCircle(ModuleDataType::PositionType Radius) const;	//!< Returns a uniformly-distributed random coordinate within a circle of radius @p Radius, not thread-safe.
		// <-

		// Function and types for automatical optimization of the sample's position to maximize the count rate
		// ->
		struct ConfocalOptimizationStateType
		{
			WidefieldMicroscopeData::PositionType X{};
			WidefieldMicroscopeData::PositionType Y{};
			DynExpInstr::AnalogOutData::SampleStreamType::SampleType Z{};
		};

		enum class ConfocalOptimizationThreadReturnType { NextStep, Finished, Failed };

		using ConfocalOptimizationFeedbackType = double;

		void ConfocalOptimizationInitPromises();
		static ConfocalOptimizationThreadReturnType ConfocalOptimizationThread(WidefieldMicroscope* Owner);
		static double ConfocalOptimizationFuncForwarder(const gsl_vector* vector, void* params);
		double ConfocalOptimizationFunc(const gsl_vector* vector) noexcept;
		// <-

		// Events, run in module thread
		void OnInit(DynExp::ModuleInstance* Instance) const override final;
		void OnExit(DynExp::ModuleInstance* Instance) const override final;
		void OnTerminate(DynExp::ModuleInstance* Instance, bool) const;
		void OnStopAction(DynExp::ModuleInstance* Instance, bool) const;
		void OnSetHomePosition(DynExp::ModuleInstance* Instance, bool) const;
		void OnGoToHomePosition(DynExp::ModuleInstance* Instance, bool) const;
		void OnToggleLEDLightSource(DynExp::ModuleInstance* Instance, bool State) const;
		void OnTogglePumpLightSource(DynExp::ModuleInstance* Instance, bool State) const;
		void OnSetupModeChanged(DynExp::ModuleInstance* Instance, QAction* Action) const;
		void OnAutofocus(DynExp::ModuleInstance* Instance, bool) const;
		void OnOptimizePositions(DynExp::ModuleInstance* Instance, bool) const;
		void OnToggleHBTMirror(DynExp::ModuleInstance* Instance, bool Checked) const;
		void OnResetCellID(DynExp::ModuleInstance* Instance, bool) const;
		void OnGeneralWidefieldPowerChanged(DynExp::ModuleInstance* Instance, double Value) const;
		void OnGeneralConfocalPowerChanged(DynExp::ModuleInstance* Instance, double Value) const;
		void OnGeneralFocusCurrentVoltageChanged(DynExp::ModuleInstance* Instance, double Value) const;
		void OnGeneralFocusZeroVoltageChanged(DynExp::ModuleInstance* Instance, double Value) const;
		void OnGeneralFocusConfocalOffsetVoltageChanged(DynExp::ModuleInstance* Instance, double Value) const;
		void OnGeneralSetZeroFocus(DynExp::ModuleInstance* Instance, bool) const;
		void OnGeneralApplyZeroFocus(DynExp::ModuleInstance* Instance, bool) const;
		void OnWidefieldLEDExposureTimeChanged(DynExp::ModuleInstance* Instance, int Value) const;
		void OnWidefieldApplyLEDExposureTime(DynExp::ModuleInstance* Instance, bool) const;
		void OnWidefieldPumpExposureTimeChanged(DynExp::ModuleInstance* Instance, int Value) const;
		void OnWidefieldApplyPumpExposureTime(DynExp::ModuleInstance* Instance, bool) const;
		void OnWidefieldFindConfocalSpot(DynExp::ModuleInstance* Instance, bool) const;
		void OnCaptureLEDImage(DynExp::ModuleInstance* Instance, bool) const;
		void OnCaptureWidefieldImage(DynExp::ModuleInstance* Instance, bool) const;
		void OnWidefieldReadCellID(DynExp::ModuleInstance* Instance, bool) const;
		void OnWidefieldAnalyzeImageDistortion(DynExp::ModuleInstance* Instance, bool) const;
		void OnWidefieldLocalizeEmitters(DynExp::ModuleInstance* Instance, bool) const;
		void OnWidefieldImageClicked(DynExp::ModuleInstance* Instance, QPoint Position) const;
		void OnConfocalConfocalWidthChanged(DynExp::ModuleInstance* Instance, int Value) const;
		void OnConfocalConfocalHeightChanged(DynExp::ModuleInstance* Instance, int Value) const;
		void OnConfocalConfocalDistPerPixelChanged(DynExp::ModuleInstance* Instance, int Value) const;
		void OnConfocalSPDExposureTimeChanged(DynExp::ModuleInstance* Instance, int Value) const;
		void OnConfocalOptimizationInitXYStepSizeChanged(DynExp::ModuleInstance* Instance, double Value) const;
		void OnConfocalOptimizationInitZStepSizeChanged(DynExp::ModuleInstance* Instance, double Value) const;
		void OnConfocalOptimizationToleranceChanged(DynExp::ModuleInstance* Instance, double Value) const;
		void OnPerformConfocalScan(DynExp::ModuleInstance* Instance, bool) const;
		void ConfocalSurfaceSelectedPointChanged(DynExp::ModuleInstance* Instance, QPoint Position) const;
		void OnHBTBinWidthChanged(DynExp::ModuleInstance* Instance, int Value) const;
		void OnHBTBinCountChanged(DynExp::ModuleInstance* Instance, int Value) const;
		void OnHHBTMaxIntegrationTimeChanged(DynExp::ModuleInstance* Instance, double Value) const;
		void OnMeasureHBT(DynExp::ModuleInstance* Instance, bool) const;
		void OnImageCapturingPaused(DynExp::ModuleInstance* Instance) const;
		void OnFinishedAutofocus(DynExp::ModuleInstance* Instance, bool Success, double Voltage) const;
		void OnSpectrumFinishedRecording(DynExp::ModuleInstance* Instance) const;
		void OnAutoMeasureSavePathChanged(DynExp::ModuleInstance* Instance, QString Path) const;
		void OnAutoMeasureNumberImageSetsChanged(DynExp::ModuleInstance* Instance, int Value) const;
		void OnAutoMeasureInitialImageSetWaitTimeChanged(DynExp::ModuleInstance* Instance, int Value) const;
		void OnAutoMeasureImagePositionScatterRadius(DynExp::ModuleInstance* Instance, int Value) const;
		void OnAutoMeasureLocalizationTypeChanged(DynExp::ModuleInstance* Instance, int Value) const;
		void OnToggleAutoMeasureOptimizeEnabled(DynExp::ModuleInstance* Instance, int State) const;
		void OnToggleAutoMeasureSpectrumEnabled(DynExp::ModuleInstance* Instance, int State) const;
		void OnToggleAutoMeasureHBTEnabled(DynExp::ModuleInstance* Instance, int State) const;
		void OnAutoMeasureNumOptimizationAttemptsChanged(DynExp::ModuleInstance* Instance, int Value) const;
		void OnAutoMeasureMaxOptimizationRerunsChanged(DynExp::ModuleInstance* Instance, int Value) const;
		void OnAutoMeasureOptimizationMaxDistanceChanged(DynExp::ModuleInstance* Instance, int Value) const;
		void OnAutoMeasureCountRateThresholdChanged(DynExp::ModuleInstance* Instance, int Value) const;
		void OnAutoMeasureCellRangeFromXChanged(DynExp::ModuleInstance* Instance, int Value) const;
		void OnAutoMeasureCellRangeFromYChanged(DynExp::ModuleInstance* Instance, int Value) const;
		void OnAutoMeasureCellRangeToXChanged(DynExp::ModuleInstance* Instance, int Value) const;
		void OnAutoMeasureCellRangeToYChanged(DynExp::ModuleInstance* Instance, int Value) const;
		void OnAutoMeasureCellSkipXChanged(DynExp::ModuleInstance* Instance, int Value) const;
		void OnAutoMeasureCellSkipYChanged(DynExp::ModuleInstance* Instance, int Value) const;
		void OnAutoMeasureRunLocalization(DynExp::ModuleInstance* Instance, bool) const;
		void OnAutoMeasureRunCharacterization(DynExp::ModuleInstance* Instance, bool) const;
		void OnAutoMeasureRunSampleCharacterization(DynExp::ModuleInstance* Instance, bool) const;

		// State functions for state machine
		StateType ReturnToReadyStateFunc(DynExp::ModuleInstance& Instance);
		StateType InitializingStateFunc(DynExp::ModuleInstance& Instance);
		StateType SetupTransitionBeginStateFunc(DynExp::ModuleInstance& Instance);
		StateType SetupTransitioningStateFunc(DynExp::ModuleInstance& Instance);
		StateType SetupTransitionEndStateFunc(DynExp::ModuleInstance& Instance);
		StateType ReadyStateFunc(DynExp::ModuleInstance& Instance);
		StateType AutofocusBeginStateFunc(DynExp::ModuleInstance& Instance);
		StateType AutofocusWaitingStateFunc(DynExp::ModuleInstance& Instance);
		StateType LEDImageAcquisitionBeginStateFunc(DynExp::ModuleInstance& Instance);
		StateType WidefieldImageAcquisitionBeginStateFunc(DynExp::ModuleInstance& Instance);
		StateType WaitingForImageReadyToCaptureStateFunc(DynExp::ModuleInstance& Instance);
		StateType WaitingForImageStateFunc(DynExp::ModuleInstance& Instance);
		StateType WaitingForWidefieldCellIDStateFunc(DynExp::ModuleInstance& Instance);
		StateType WidefieldCellWaitUntilCenteredStateFunc(DynExp::ModuleInstance& Instance);
		StateType WaitingForWidefieldLocalizationStateFunc(DynExp::ModuleInstance& Instance);
		StateType FindingConfocalSpotBeginStateFunc(DynExp::ModuleInstance& Instance);
		StateType FindingConfocalSpotAfterTransitioningToConfocalModeStateFunc(DynExp::ModuleInstance& Instance);
		StateType FindingConfocalSpotAfterRecordingWidefieldImageStateFunc(DynExp::ModuleInstance& Instance);
		StateType ConfocalScanStepStateFunc(DynExp::ModuleInstance& Instance);
		StateType ConfocalScanWaitUntilMovedStateFunc(DynExp::ModuleInstance& Instance);
		StateType ConfocalScanCaptureStateFunc(DynExp::ModuleInstance& Instance);
		StateType ConfocalScanWaitUntilCapturedStateFunc(DynExp::ModuleInstance& Instance);
		StateType ConfocalOptimizationInitStateFunc(DynExp::ModuleInstance& Instance);
		StateType ConfocalOptimizationInitSubStepStateFunc(DynExp::ModuleInstance& Instance);
		StateType ConfocalOptimizationWaitStateFunc(DynExp::ModuleInstance& Instance);
		StateType ConfocalOptimizationStepStateFunc(DynExp::ModuleInstance& Instance);
		StateType HBTAcquiringStateFunc(DynExp::ModuleInstance& Instance);
		StateType WaitingStateFunc(DynExp::ModuleInstance& Instance);
		StateType SpectrumAcquisitionWaitingStateFunc(DynExp::ModuleInstance& Instance);
		StateType AutoMeasureLocalizationStepStateFunc(DynExp::ModuleInstance& Instance);
		StateType AutoMeasureLocalizationSaveLEDImageStateFunc(DynExp::ModuleInstance& Instance);
		StateType AutoMeasureLocalizationSaveWidefieldImageStateFunc(DynExp::ModuleInstance& Instance);
		StateType AutoMeasureLocalizationMovingStateFunc(DynExp::ModuleInstance& Instance);
		StateType AutoMeasureCharacterizationStepStateFunc(DynExp::ModuleInstance& Instance);
		StateType AutoMeasureCharacterizationGotoEmitterStateFunc(DynExp::ModuleInstance& Instance);
		StateType AutoMeasureCharacterizationOptimizationFinishedStateFunc(DynExp::ModuleInstance& Instance);
		StateType AutoMeasureCharacterizationSpectrumBeginStateFunc(DynExp::ModuleInstance& Instance);
		StateType AutoMeasureCharacterizationSpectrumFinishedStateFunc(DynExp::ModuleInstance& Instance);
		StateType AutoMeasureCharacterizationHBTBeginStateFunc(DynExp::ModuleInstance& Instance);
		StateType AutoMeasureCharacterizationHBTWaitForInitStateFunc(DynExp::ModuleInstance& Instance);
		StateType AutoMeasureCharacterizationHBTFinishedStateFunc(DynExp::ModuleInstance& Instance);
		StateType AutoMeasureSampleStepStateFunc(DynExp::ModuleInstance& Instance);
		StateType AutoMeasureSampleReadCellIDStateFunc(DynExp::ModuleInstance& Instance);
		StateType AutoMeasureSampleReadCellIDFinishedStateFunc(DynExp::ModuleInstance& Instance);
		StateType AutoMeasureSampleLocalizeStateFunc(DynExp::ModuleInstance& Instance);
		StateType AutoMeasureSampleFindEmittersStateFunc(DynExp::ModuleInstance& Instance);
		StateType AutoMeasureSampleCharacterizeStateFunc(DynExp::ModuleInstance& Instance);
		StateType AutoMeasureSampleAdvanceCellStateFunc(DynExp::ModuleInstance& Instance);
			
		// States for state machine
		static constexpr auto InitializingState = Util::StateMachineState(StateType::Initializing,
			&WidefieldMicroscope::InitializingStateFunc, "Initializing module...");
		static constexpr auto SetupTransitionBeginState = Util::StateMachineState(StateType::SetupTransitionBegin,
			&WidefieldMicroscope::SetupTransitionBeginStateFunc, "Transitioning...");
		static constexpr auto SetupTransitioningState = Util::StateMachineState(StateType::SetupTransitioning,
			&WidefieldMicroscope::SetupTransitioningStateFunc, "Transitioning...");
		static constexpr auto SetupTransitionEndState = Util::StateMachineState(StateType::SetupTransitionEnd,
			&WidefieldMicroscope::SetupTransitionEndStateFunc, "Transitioning...");
		static constexpr auto SetupTransitionFinishedState = Util::StateMachineState(StateType::SetupTransitionFinished,
			&WidefieldMicroscope::ReturnToReadyStateFunc);
		static constexpr auto ReadyState = Util::StateMachineState(StateType::Ready,
			&WidefieldMicroscope::ReadyStateFunc, "Ready");
		static constexpr auto AutofocusBeginState = Util::StateMachineState(StateType::AutofocusBegin,
			&WidefieldMicroscope::AutofocusBeginStateFunc);
		static constexpr auto AutofocusWaitingState = Util::StateMachineState(StateType::AutofocusWaiting,
			&WidefieldMicroscope::AutofocusWaitingStateFunc);
		static constexpr auto AutofocusFinishedState = Util::StateMachineState(StateType::AutofocusFinished,
			&WidefieldMicroscope::ReturnToReadyStateFunc);
		static constexpr auto LEDImageAcquisitionBeginState = Util::StateMachineState(StateType::LEDImageAcquisitionBegin,
			&WidefieldMicroscope::LEDImageAcquisitionBeginStateFunc, "Recording LED image...");
		static constexpr auto WidefieldImageAcquisitionBeginState = Util::StateMachineState(StateType::WidefieldImageAcquisitionBegin,
			&WidefieldMicroscope::WidefieldImageAcquisitionBeginStateFunc, "Recording widefield image...");
		static constexpr auto WaitingForLEDImageReadyToCaptureState = Util::StateMachineState(StateType::WaitingForLEDImageReadyToCapture,
			&WidefieldMicroscope::WaitingForImageReadyToCaptureStateFunc, "Recording LED image...");
		static constexpr auto WaitingForLEDImageState = Util::StateMachineState(StateType::WaitingForLEDImage,
			&WidefieldMicroscope::WaitingForImageStateFunc, "Recording LED image...");
		static constexpr auto WaitingForLEDImageFinishedState = Util::StateMachineState(StateType::WaitingForLEDImageFinished,
			&WidefieldMicroscope::ReturnToReadyStateFunc);
		static constexpr auto WaitingForWidefieldImageReadyToCaptureState = Util::StateMachineState(StateType::WaitingForWidefieldImageReadyToCapture,
			&WidefieldMicroscope::WaitingForImageReadyToCaptureStateFunc, "Recording widefield image...");
		static constexpr auto WaitingForWidefieldImageState = Util::StateMachineState(StateType::WaitingForWidefieldImage,
			&WidefieldMicroscope::WaitingForImageStateFunc, "Recording widefield image...");
		static constexpr auto WaitingForWidefieldImageFinishedState = Util::StateMachineState(StateType::WaitingForWidefieldImageFinished,
			&WidefieldMicroscope::ReturnToReadyStateFunc);
		static constexpr auto WaitingForWidefieldCellIDState = Util::StateMachineState(StateType::WaitingForWidefieldCellID,
			&WidefieldMicroscope::WaitingForWidefieldCellIDStateFunc, "Waiting for cell ID to be read from image...");
		static constexpr auto WidefieldCellWaitUntilCenteredState = Util::StateMachineState(StateType::WidefieldCellWaitUntilCentered,
			&WidefieldMicroscope::WidefieldCellWaitUntilCenteredStateFunc, "Waiting for cell to be centered in FOV...");
		static constexpr auto WidefieldCellIDReadFinishedState = Util::StateMachineState(StateType::WidefieldCellIDReadFinished,
			&WidefieldMicroscope::ReturnToReadyStateFunc);
		static constexpr auto WaitingForWidefieldLocalizationState = Util::StateMachineState(StateType::WaitingForWidefieldLocalization,
			&WidefieldMicroscope::WaitingForWidefieldLocalizationStateFunc, "Waiting for localization of emitters from image...");
		static constexpr auto WidefieldLocalizationFinishedState = Util::StateMachineState(StateType::WidefieldLocalizationFinished,
			&WidefieldMicroscope::ReturnToReadyStateFunc);
		static constexpr auto FindingConfocalSpotBeginState = Util::StateMachineState(StateType::FindingConfocalSpotBegin,
			&WidefieldMicroscope::FindingConfocalSpotBeginStateFunc);
		static constexpr auto FindingConfocalSpotAfterTransitioningToConfocalModeState = Util::StateMachineState(StateType::FindingConfocalSpotAfterTransitioningToConfocalMode,
			&WidefieldMicroscope::FindingConfocalSpotAfterTransitioningToConfocalModeStateFunc);
		static constexpr auto FindingConfocalSpotAfterRecordingWidefieldImageState = Util::StateMachineState(StateType::FindingConfocalSpotAfterRecordingWidefieldImage,
			&WidefieldMicroscope::FindingConfocalSpotAfterRecordingWidefieldImageStateFunc);
		static constexpr auto ConfocalScanStepState = Util::StateMachineState(StateType::ConfocalScanStep,
			&WidefieldMicroscope::ConfocalScanStepStateFunc, "Performing confocal scan...");
		static constexpr auto ConfocalScanWaitUntilMovedState = Util::StateMachineState(StateType::ConfocalScanWaitUntilMoved,
			&WidefieldMicroscope::ConfocalScanWaitUntilMovedStateFunc, "Performing confocal scan...");
		static constexpr auto ConfocalScanCaptureState = Util::StateMachineState(StateType::ConfocalScanCapture,
			&WidefieldMicroscope::ConfocalScanCaptureStateFunc, "Performing confocal scan...");
		static constexpr auto ConfocalScanWaitUntilCapturedState = Util::StateMachineState(StateType::ConfocalScanWaitUntilCaptured,
			&WidefieldMicroscope::ConfocalScanWaitUntilCapturedStateFunc, "Performing confocal scan...");
		static constexpr auto ConfocalOptimizationInitState = Util::StateMachineState(StateType::ConfocalOptimizationInit,
			&WidefieldMicroscope::ConfocalOptimizationInitStateFunc);
		static constexpr auto ConfocalOptimizationInitSubStepState = Util::StateMachineState(StateType::ConfocalOptimizationInitSubStep,
			&WidefieldMicroscope::ConfocalOptimizationInitSubStepStateFunc);
		static constexpr auto ConfocalOptimizationWaitState = Util::StateMachineState(StateType::ConfocalOptimizationWait,
			&WidefieldMicroscope::ConfocalOptimizationWaitStateFunc);
		static constexpr auto ConfocalOptimizationStepState = Util::StateMachineState(StateType::ConfocalOptimizationStep,
			&WidefieldMicroscope::ConfocalOptimizationStepStateFunc);
		static constexpr auto ConfocalOptimizationFinishedState = Util::StateMachineState(StateType::ConfocalOptimizationFinished,
			&WidefieldMicroscope::ReturnToReadyStateFunc);
		static constexpr auto HBTAcquiringState = Util::StateMachineState(StateType::HBTAcquiring,
			&WidefieldMicroscope::HBTAcquiringStateFunc, "HBT acquiring...");
		static constexpr auto HBTFinishedState = Util::StateMachineState(StateType::HBTFinished,
			&WidefieldMicroscope::ReturnToReadyStateFunc);
		static constexpr auto WaitingState = Util::StateMachineState(StateType::Waiting,
			&WidefieldMicroscope::WaitingStateFunc, "Waiting...");
		static constexpr auto WaitingFinishedState = Util::StateMachineState(StateType::WaitingFinished,
			&WidefieldMicroscope::ReturnToReadyStateFunc);
		static constexpr auto SpectrumAcquisitionWaitingState = Util::StateMachineState(StateType::SpectrumAcquisitionWaiting,
			&WidefieldMicroscope::SpectrumAcquisitionWaitingStateFunc, "Acquiring spectrum...");
		static constexpr auto SpectrumAcquisitionFinishedState = Util::StateMachineState(StateType::SpectrumAcquisitionFinished,
			&WidefieldMicroscope::ReturnToReadyStateFunc);
		static constexpr auto AutoMeasureLocalizationStepState = Util::StateMachineState(StateType::AutoMeasureLocalizationStep,
			&WidefieldMicroscope::AutoMeasureLocalizationStepStateFunc, "Recording image...");
		static constexpr auto AutoMeasureLocalizationSaveLEDImageState = Util::StateMachineState(StateType::AutoMeasureLocalizationSaveLEDImage,
			&WidefieldMicroscope::AutoMeasureLocalizationSaveLEDImageStateFunc, "Saving LED image...");
		static constexpr auto AutoMeasureLocalizationSaveWidefieldImageState = Util::StateMachineState(StateType::AutoMeasureLocalizationSaveWidefieldImage,
			&WidefieldMicroscope::AutoMeasureLocalizationSaveWidefieldImageStateFunc, "Saving widefield image...");
		static constexpr auto AutoMeasureLocalizationMovingState = Util::StateMachineState(StateType::AutoMeasureLocalizationMoving,
			&WidefieldMicroscope::AutoMeasureLocalizationMovingStateFunc, "Moving to next image capturing position...");
		static constexpr auto AutoMeasureLocalizationFinishedState = Util::StateMachineState(StateType::AutoMeasureLocalizationFinished,
			&WidefieldMicroscope::ReturnToReadyStateFunc);
		static constexpr auto AutoMeasureCharacterizationStepState = Util::StateMachineState(StateType::AutoMeasureCharacterizationStep,
			&WidefieldMicroscope::AutoMeasureCharacterizationStepStateFunc);
		static constexpr auto AutoMeasureCharacterizationGotoEmitterState = Util::StateMachineState(StateType::AutoMeasureCharacterizationGotoEmitter,
			&WidefieldMicroscope::AutoMeasureCharacterizationGotoEmitterStateFunc);
		static constexpr auto AutoMeasureCharacterizationOptimizationFinishedState = Util::StateMachineState(StateType::AutoMeasureCharacterizationOptimizationFinished,
			&WidefieldMicroscope::AutoMeasureCharacterizationOptimizationFinishedStateFunc);
		static constexpr auto AutoMeasureCharacterizationSpectrumFinishedState = Util::StateMachineState(StateType::AutoMeasureCharacterizationSpectrumFinished,
			&WidefieldMicroscope::AutoMeasureCharacterizationSpectrumFinishedStateFunc);
		static constexpr auto AutoMeasureCharacterizationSpectrumBeginState = Util::StateMachineState(StateType::AutoMeasureCharacterizationSpectrumBegin,
			&WidefieldMicroscope::AutoMeasureCharacterizationSpectrumBeginStateFunc);
		static constexpr auto AutoMeasureCharacterizationHBTBeginState = Util::StateMachineState(StateType::AutoMeasureCharacterizationHBTBegin,
			&WidefieldMicroscope::AutoMeasureCharacterizationHBTBeginStateFunc);
		static constexpr auto AutoMeasureCharacterizationHBTWaitForInitState = Util::StateMachineState(StateType::AutoMeasureCharacterizationHBTWaitForInit,
			&WidefieldMicroscope::AutoMeasureCharacterizationHBTWaitForInitStateFunc);
		static constexpr auto AutoMeasureCharacterizationHBTFinishedState = Util::StateMachineState(StateType::AutoMeasureCharacterizationHBTFinished,
			&WidefieldMicroscope::AutoMeasureCharacterizationHBTFinishedStateFunc);
		static constexpr auto AutoMeasureCharacterizationFinishedState = Util::StateMachineState(StateType::AutoMeasureCharacterizationFinished,
			&WidefieldMicroscope::ReturnToReadyStateFunc);
		static constexpr auto AutoMeasureSampleStepState = Util::StateMachineState(StateType::AutoMeasureSampleStep,
			&WidefieldMicroscope::AutoMeasureSampleStepStateFunc);
		static constexpr auto AutoMeasureSampleReadCellIDState = Util::StateMachineState(StateType::AutoMeasureSampleReadCellID,
			&WidefieldMicroscope::AutoMeasureSampleReadCellIDStateFunc);
		static constexpr auto AutoMeasureSampleReadCellIDFinishedState = Util::StateMachineState(StateType::AutoMeasureSampleReadCellIDFinished,
			&WidefieldMicroscope::AutoMeasureSampleReadCellIDFinishedStateFunc);
		static constexpr auto AutoMeasureSampleLocalizeState = Util::StateMachineState(StateType::AutoMeasureSampleLocalize,
			&WidefieldMicroscope::AutoMeasureSampleLocalizeStateFunc);
		static constexpr auto AutoMeasureSampleFindEmittersState = Util::StateMachineState(StateType::AutoMeasureSampleFindEmitters,
			&WidefieldMicroscope::AutoMeasureSampleFindEmittersStateFunc);
		static constexpr auto AutoMeasureSampleCharacterizeState = Util::StateMachineState(StateType::AutoMeasureSampleCharacterize,
			&WidefieldMicroscope::AutoMeasureSampleCharacterizeStateFunc);
		static constexpr auto AutoMeasureSampleAdvanceCellState = Util::StateMachineState(StateType::AutoMeasureSampleAdvanceCell,
			&WidefieldMicroscope::AutoMeasureSampleAdvanceCellStateFunc);
		static constexpr auto AutoMeasureSampleFinishedState = Util::StateMachineState(StateType::AutoMeasureSampleFinished,
			&WidefieldMicroscope::ReturnToReadyStateFunc);

		// Contexts for state machine
		const Util::StateMachineContext<StateMachineStateType> AutofocusSetupTransitioningContext = { {
			{ StateType::SetupTransitionFinished, StateType::AutofocusBegin }
		}, "Autofocusing..." };
		const Util::StateMachineContext<StateMachineStateType> FindingConfocalSpotBeginContext = { {
			{ StateType::SetupTransitionFinished, StateType::FindingConfocalSpotAfterTransitioningToConfocalMode }
		}, "Locating confocal spot..." };
		const Util::StateMachineContext<StateMachineStateType> FindingConfocalSpotRecordingWidefieldImageContext = { {
			{ StateType::WaitingForWidefieldImageFinished, StateType::FindingConfocalSpotAfterRecordingWidefieldImage }
		}, "Locating confocal spot..." };
		const Util::StateMachineContext<StateMachineStateType> ConfocalOptimizationContext = { {
			{ StateType::DummyState, StateType::ConfocalScanStep },
			{ StateType::ConfocalScanStep, StateType::ConfocalOptimizationStep }
		}, "Optimizing count rate..." };
		const Util::StateMachineContext<StateMachineStateType> LEDImageAcquisitionSetupTransitioningContext = { {
			{ StateType::SetupTransitionFinished, StateType::LEDImageAcquisitionBegin }
		} };
		const Util::StateMachineContext<StateMachineStateType> WidefieldImageAcquisitionSetupTransitioningContext = { {
			{ StateType::SetupTransitionFinished, StateType::WidefieldImageAcquisitionBegin }
		} };
		const Util::StateMachineContext<StateMachineStateType> AutoMeasureLocalizationContext = { {
			{ StateType::SetupTransitionFinished, StateType::Waiting },
			{ StateType::WaitingFinished, StateType::AutoMeasureLocalizationStep },
			{ StateType::WaitingForLEDImageFinished, StateType::AutoMeasureLocalizationSaveLEDImage },
			{ StateType::WaitingForWidefieldImageFinished, StateType::AutoMeasureLocalizationSaveWidefieldImage }
		} };
		const Util::StateMachineContext<StateMachineStateType> AutoMeasureCharacterizationContext = { {
			{ StateType::SetupTransitionFinished, StateType::AutoMeasureCharacterizationStep },
			{ StateType::ConfocalOptimizationFinished, StateType::AutoMeasureCharacterizationOptimizationFinished },
			{ StateType::SpectrumAcquisitionFinished, StateType::AutoMeasureCharacterizationSpectrumFinished },
			{ StateType::HBTFinished, StateType::AutoMeasureCharacterizationHBTFinished }
		}, "Characterizing emitters...", { &ConfocalOptimizationContext } };
		const Util::StateMachineContext<StateMachineStateType> AutoMeasureCharacterizationOptimizationContext = { {
			{ StateType::WaitingFinished, StateType::ConfocalOptimizationInit }
		}, "Characterizing emitters (optimizing count rate)...", { &AutoMeasureCharacterizationContext } };
		const Util::StateMachineContext<StateMachineStateType> AutoMeasureCharacterizationSpectrumContext = { {
			{ StateType::WaitingFinished, StateType::AutoMeasureCharacterizationSpectrumBegin }
		}, "Characterizing emitters (recording spectrum)...", { &AutoMeasureCharacterizationContext } };
		const Util::StateMachineContext<StateMachineStateType> AutoMeasureCharacterizationHBTContext = { {
			{ StateType::WaitingFinished, StateType::AutoMeasureCharacterizationHBTBegin }
		}, "Characterizing emitters (HBT acquiring)...", { &AutoMeasureCharacterizationContext } };
		const Util::StateMachineContext<StateMachineStateType> AutoMeasureSampleContext = { {
			{ StateType::AutofocusFinished, StateType::LEDImageAcquisitionBegin },
			{ StateType::WaitingForLEDImageReadyToCapture, StateType::Waiting },
			{ StateType::WaitingFinished, StateType::WaitingForLEDImageReadyToCapture },
			{ StateType::WaitingForLEDImageFinished, StateType::AutoMeasureSampleReadCellID },
			{ StateType::WidefieldCellIDReadFinished, StateType::AutoMeasureSampleReadCellIDFinished },
			{ StateType::WidefieldLocalizationFinished, StateType::AutoMeasureSampleLocalize }
		}, "Characterizing sample...", { &AutofocusSetupTransitioningContext } };
		const Util::StateMachineContext<StateMachineStateType> AutoMeasureSampleRecenterCellContext = { {
			{ StateType::WaitingForLEDImageFinished, StateType::AutoMeasureSampleReadCellIDFinished }
		}, "Characterizing sample...", { &AutoMeasureSampleContext } };
		const Util::StateMachineContext<StateMachineStateType> AutoMeasureSampleLocalizationContext = { {
			{ StateType::AutoMeasureLocalizationFinished, StateType::AutoMeasureSampleFindEmitters },
			{ StateType::WidefieldLocalizationFinished, StateType::AutoMeasureSampleCharacterize }
		}, "Characterizing sample (localizing)...", { &AutoMeasureLocalizationContext } };
		const Util::StateMachineContext<StateMachineStateType> AutoMeasureSampleCharacterizationContext = { {
			{ StateType::AutoMeasureCharacterizationFinished, StateType::AutoMeasureSampleAdvanceCell }
		}, "Characterizing sample...", { &AutoMeasureCharacterizationContext } };
		const Util::StateMachineContext<StateMachineStateType> AutoMeasureSampleCharacterizationOptimizationContext = { {
		}, "Characterizing sample (optimizing count rate)...", { &AutoMeasureSampleCharacterizationContext, &AutoMeasureCharacterizationOptimizationContext } };
		const Util::StateMachineContext<StateMachineStateType> AutoMeasureSampleCharacterizationSpectrumContext = { {
		}, "Characterizing sample (recording spectrum)...", { &AutoMeasureSampleCharacterizationContext, &AutoMeasureCharacterizationSpectrumContext } };
		const Util::StateMachineContext<StateMachineStateType> AutoMeasureSampleCharacterizationHBTContext = { {
		}, "Characterizing sample (HBT acquiring)...", { &AutoMeasureSampleCharacterizationContext, &AutoMeasureCharacterizationHBTContext } };

		// Logical const-ness: allow events to set the state machine's current state.
		mutable Util::StateMachine<StateMachineStateType> StateMachine;

		// shared_ptr since in principle PositionerStages' tasks can outlive this module instance.
		const std::shared_ptr<AtomicPositionerStateType> ConfocalScanPositionerStateX;
		const std::shared_ptr<AtomicPositionerStateType> ConfocalScanPositionerStateY;
		const std::shared_ptr<AtomicPositionerStateType> ConfocalScanPositionerStateZ;

		// shared_ptr since in principle WidefieldLocalizer's tasks can outlive this module instance.
		const std::shared_ptr<AtomicWidefieldImageProcessingStateType> WidefieldCellIDState;
		const std::shared_ptr<AtomicWidefieldImageProcessingStateType> WidefieldLocalizationState;

		mutable std::list<WidefieldMicroscopeData::PositionPoint> ConfocalScanPositions;

		// Variables for automatical optimization of the sample's position to maximize the count rate
		static constexpr size_t GSLConfocalOptimizationNumDimensions = 3;
		const gsl_multimin_fminimizer_type* const GSLConfocalOptimizationMinimizer = gsl_multimin_fminimizer_nmsimplex2;
		gsl_multimin_function GSLConfocalOptimizationFuncDesc{ &WidefieldMicroscope::ConfocalOptimizationFuncForwarder, GSLConfocalOptimizationNumDimensions, this };
		gsl_multimin_fminimizer* const GSLConfocalOptimizationState;
		gsl_vector* const GSLConfocalOptimizationStepSize;
		gsl_vector* const GSLConfocalOptimizationInitialPoint;
		mutable std::atomic<size_t> ConfocalOptimizationNumStepsPerformed = 0;						// To be accessed by ConfocalOptimizationThread and WidefieldMicroscope thread.
		mutable std::atomic_bool ConfocalOptimizationPromisesRenewed = false;						// To be accessed by ConfocalOptimizationThread and WidefieldMicroscope thread.
		std::promise<ConfocalOptimizationStateType> ConfocalOptimizationStatePromise;				// To be solely accessed by ConfocalOptimizationThread.
		std::future<ConfocalOptimizationStateType> ConfocalOptimizationStateFuture;					// To be solely accessed by WidefieldMicroscope thread.
		mutable std::promise<ConfocalOptimizationFeedbackType> ConfocalOptimizationFeedbackPromise;	// To be solely accessed by WidefieldMicroscope thread.
		std::future<ConfocalOptimizationFeedbackType> ConfocalOptimizationFeedbackFuture;			// To be solely accessed by ConfocalOptimizationThread.
		std::future<ConfocalOptimizationThreadReturnType> ConfocalOptimizationThreadReturnFuture;	// To be solely accessed by WidefieldMicroscope thread.

		// Variables for switching from a confocal to a widefield setup.
		std::chrono::system_clock::time_point SetupTransitionFinishedTimePoint;
		bool TurnOnPumpSourceAfterTransitioning = false;

		// Variables for widefield image capturing.
		mutable bool ImageCapturingPaused = false;

		// Variables for managing wait times within measurements.
		mutable std::chrono::system_clock::time_point WaitingEndTimePoint;
		mutable std::chrono::microseconds HBTIntegrationTimeBeforeReset{ 0 };

		// General
		size_t NumFailedUpdateAttempts = 0;
		mutable bool LogUIMessagesOnly = false;
	};
}