// This file is part of DynExp.

/**
 * @file WidefieldMicroscopeWidget.h
 * @brief User interface belonging to the DynExpModule::Widefield::WidefieldMicroscope module.
*/

#pragma once

#include "stdafx.h"
#include "DynExpCore.h"

#include <QWidget>
#include "ui_WidefieldMicroscope.h"

namespace DynExpModule::Widefield
{
	class WidefieldMicroscope;
	class WidefieldMicroscopeData;

	enum class StateType {
		DummyState,			// To be temporarily replaced by a StataMachine's context.
		Initializing,
		SetupTransitionBegin,
		SetupTransitioning,
		SetupTransitionEnd,
		SetupTransitionFinished,
		Ready,
		AutofocusBegin,
		AutofocusWaiting,
		AutofocusFinished,
		LEDImageAcquisitionBegin,
		WidefieldImageAcquisitionBegin,
		WaitingForLEDImageReadyToCapture,
		WaitingForLEDImage,
		WaitingForLEDImageFinished,
		WaitingForWidefieldImageReadyToCapture,
		WaitingForWidefieldImage,
		WaitingForWidefieldImageFinished,
		WaitingForWidefieldCellID,
		WidefieldCellWaitUntilCentered,
		WidefieldCellIDReadFinished,
		WaitingForWidefieldLocalization,
		WidefieldLocalizationFinished,
		FindingConfocalSpotBegin,
		FindingConfocalSpotAfterTransitioningToConfocalMode,
		FindingConfocalSpotAfterRecordingWidefieldImage,
		ConfocalScanStep,
		ConfocalScanWaitUntilMoved,
		ConfocalScanCapture,
		ConfocalScanWaitUntilCaptured,
		ConfocalOptimizationInit,
		ConfocalOptimizationInitSubStep,
		ConfocalOptimizationWait,
		ConfocalOptimizationStep,
		ConfocalOptimizationFinished,
		HBTAcquiring,
		HBTFinished,
		Waiting,
		WaitingFinished,	// To be temporarily replaced by a StataMachine's context.
		SpectrumAcquisitionWaiting,
		SpectrumAcquisitionFinished,
		AutoMeasureLocalizationStep,
		AutoMeasureLocalizationSaveLEDImage,
		AutoMeasureLocalizationSaveWidefieldImage,
		AutoMeasureLocalizationMoving,
		AutoMeasureLocalizationFinished,
		AutoMeasureCharacterizationStep,
		AutoMeasureCharacterizationGotoEmitter,
		AutoMeasureCharacterizationOptimizationFinished,
		AutoMeasureCharacterizationSpectrumBegin,
		AutoMeasureCharacterizationSpectrumFinished,
		AutoMeasureCharacterizationHBTBegin,
		AutoMeasureCharacterizationHBTWaitForInit,
		AutoMeasureCharacterizationHBTFinished,
		AutoMeasureCharacterizationFinished,
		AutoMeasureSampleStep,
		AutoMeasureSampleReadCellID,
		AutoMeasureSampleReadCellIDFinished,
		AutoMeasureSampleLocalize,
		AutoMeasureSampleFindEmitters,
		AutoMeasureSampleCharacterize,
		AutoMeasureSampleAdvanceCell,
		AutoMeasureSampleFinished
	};

	using StateMachineStateType = Util::StateMachineState<StateType(WidefieldMicroscope::*)(DynExp::ModuleInstance&)>;

	class WidefieldMicroscopeWidget : public DynExp::QModuleWidget
	{
		Q_OBJECT

	private:
		struct StatusBarType
		{
			StatusBarType(WidefieldMicroscopeWidget* Owner);

			void Update();

			const StateMachineStateType* CurrentState;
			const Util::StateMachineContext<StateMachineStateType>* CurrentContext;

			QLabel* StateLabel;
			QLabel* CellIDLabel;
			QMenu* CellIDContextMenu;
			QLabel* HomePosLabel;
			QLabel* PumpPowerLabel;
			QWidget* ImageCoordinatesGroup;
			QHBoxLayout* ImageCoordinatesLayout;
			QLabel* XCoordLabel;
			QLabel* YCoordLabel;
		};

		enum EmitterListColumnType : int { EmitterID, EmitterName, Image_x, Image_y, Sample_x, Sample_y, EmitterState };

	public:
		enum LocalizationType : int { LocalizeEmittersFromImage, RecallEmitterPositions };

		WidefieldMicroscopeWidget(WidefieldMicroscope& Owner, QModuleWidget* parent = nullptr);
		~WidefieldMicroscopeWidget() = default;

		bool AllowResize() const noexcept override final { return true; }

		const WidefieldMicroscope& GetCastOwner() const noexcept;

		void InitializeUI(Util::SynchronizedPointer<WidefieldMicroscopeData>& ModuleData);
		void SetUIState(const StateMachineStateType* State, const Util::StateMachineContext<StateMachineStateType>* Context,
			Util::SynchronizedPointer<WidefieldMicroscopeData>& ModuleData);
		void UpdateUIData(Util::SynchronizedPointer<WidefieldMicroscopeData>& ModuleData);
		void UpdateWidefieldUIData(Util::SynchronizedPointer<WidefieldMicroscopeData>& ModuleData);
		void UpdateConfocalUIData(Util::SynchronizedPointer<WidefieldMicroscopeData>& ModuleData);
		void UpdateHBTUIData(Util::SynchronizedPointer<WidefieldMicroscopeData>& ModuleData);
		void UpdateAutoMeasureUIData(Util::SynchronizedPointer<WidefieldMicroscopeData>& ModuleData, bool IsCharacterizingSample);

		const auto& GetUI() const noexcept { return ui; }
		const auto& GetWidefieldConfocalModeActionGroup() const noexcept { return WidefieldConfocalModeActionGroup; }
		const auto& GetMainGraphicsView() const noexcept { return MainGraphicsView; }
		const auto& GetConfocalSurface3DSeries() const noexcept { return ConfocalSurface3DSeries; }
		bool GetUIInitialized() const noexcept { return UIInitialized; }

		void SetMainGraphicsImage(const QImage& Image) noexcept;
		void UpdateScene();

	private:
		enum class EmitterListTaskType { None, GoToSamplePos, MarkerToConfocalSpot, RunCharacterization };

		bool eventFilter(QObject* obj, QEvent* event) override;
		virtual void resizeEvent(QResizeEvent* event) override;

		/**
		 * @brief Stores the values of the respective cells of ui.TWEmitterList's currently selected row in MarkerPos and SamplePos.
		 * @return Returns true if exactly one row has been selected, so that storing was successful. False otherwise.
		*/
		bool StoreTWEmitterListSelection();

		Ui::WidefieldMicroscope ui;
		StatusBarType StatusBar;
		
		QActionGroup* WidefieldConfocalModeActionGroup;

		Util::MarkerGraphicsView* MainGraphicsView;
		QPixmap MainGraphicsPixmap;
		QGraphicsPixmapItem* MainGraphicsPixmapItem;
		QGraphicsScene* MainGraphicsScene;
		QPoint CurrentConfocalSpotPosition;
		QString SaveWidefieldImageFilename;

		QMenu* EmitterListContextMenu;

		QMenu* ConfocalMapContextMenu;
		Q3DSurface* ConfocalGraph;
		QWidget* ConfocalGraphContainer;
		QSurfaceDataProxy* ConfocalSurfaceDataProxy;
		QSurfaceDataArray* ConfocalSurfaceDataArray;
		QSurface3DSeries* ConfocalSurface3DSeries;
		size_t NumItemsInArray;
		double ConfocalSurfaceMinCounts;
		double ConfocalSurfaceMaxCounts;

		QXYSeries* HBTDataSeries;
		QChart* HBTDataChart;
		QValueAxis* HBTXAxis;
		QValueAxis* HBTYAxis;

		bool UIInitialized = false;

		QPoint MarkerPos;
		QPointF SamplePos;
		Util::MarkerGraphicsView::MarkerType::IDType MarkerID;
		EmitterListTaskType EmitterListTask = EmitterListTaskType::None;

	private slots:
		void OnSaveImageClicked();
		void OnZoomResetClicked();
		void OnZoomInClicked();
		void OnZoomOutClicked();
		void OnZoomFitClicked(bool Checked);
		void OnImageMouseMove(QMouseEvent* Event);
		void OnCellIDContextMenuRequested(const QPoint& Position);
		void OnEmitterListCellClicked(int Row, int Column);
		void OnEmitterListCellDoubleClicked(int Row, int Column);
		void OnEmitterListContextMenuRequested(const QPoint& Position);
		void OnEmitterListDeleteEntryClicked();
		void OnEmitterListEditEntryClicked();
		void OnEmitterListGoToSamplePositionClicked();
		void OnEmitterListBringMarkerToConfocalSpotClicked();
		void OnEmitterListRunCharacterizationFromID();
		void OnConfocalMapResetClicked();
		void OnConfocalMapSaveRawDataClicked();
		void OnHBTSaveRawDataClicked();
		void OnAutoMeasureSavePathBrowseClicked();
	};
}