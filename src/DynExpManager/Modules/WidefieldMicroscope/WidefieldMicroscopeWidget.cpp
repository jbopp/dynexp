// This file is part of DynExp.

#include "stdafx.h"
#include "WidefieldMicroscope.h"
#include "moc_WidefieldMicroscopeWidget.cpp"
#include "WidefieldMicroscopeWidget.h"

namespace DynExpModule::Widefield
{
	WidefieldMicroscopeWidget::StatusBarType::StatusBarType(WidefieldMicroscopeWidget* Owner)
		: CurrentState(nullptr), CurrentContext(nullptr),
		StateLabel(new QLabel(Owner)),
		CellIDLabel(new QLabel(Owner)), CellIDContextMenu(new QMenu(Owner)),
		HomePosLabel(new QLabel(Owner)),
		PumpPowerLabel(new QLabel(Owner)),
		ImageCoordinatesGroup(new QWidget(Owner)), ImageCoordinatesLayout(new QHBoxLayout),
		XCoordLabel(new QLabel(Owner)), YCoordLabel(new QLabel(Owner))
	{
		CellIDLabel->setContextMenuPolicy(Qt::ContextMenuPolicy::CustomContextMenu);
		PumpPowerLabel->setMinimumWidth(140);
		XCoordLabel->setAlignment(Qt::AlignmentFlag::AlignRight);
		YCoordLabel->setAlignment(Qt::AlignmentFlag::AlignRight);
		ImageCoordinatesLayout->addWidget(XCoordLabel);
		ImageCoordinatesLayout->addWidget(YCoordLabel);
		ImageCoordinatesGroup->setLayout(ImageCoordinatesLayout);
	}

	void WidefieldMicroscopeWidget::StatusBarType::Update()
	{
		if (CurrentContext && !std::string(CurrentContext->GetDescription()).empty())
			StateLabel->setText(QString(" ") + CurrentContext->GetDescription());
		else
			StateLabel->setText(CurrentState ? (QString(" ") + CurrentState->GetDescription()) : "< Unknown >");

		if (CurrentState && CurrentState->GetState() != StateType::Ready)
			StateLabel->setStyleSheet(DynExpUI::StatusBarBusyStyleSheet);
		else
			StateLabel->setStyleSheet("");
	}

	WidefieldMicroscopeWidget::WidefieldMicroscopeWidget(WidefieldMicroscope& Owner, QModuleWidget* parent)
		: QModuleWidget(Owner, parent), StatusBar(this),
		WidefieldConfocalModeActionGroup(new QActionGroup(this)), MainGraphicsView(nullptr),
		MainGraphicsScene(new QGraphicsScene(this)), CurrentConfocalSpotPosition(0, 0),
		EmitterListContextMenu(new QMenu(this)),
		ConfocalMapContextMenu(new QMenu(this)), ConfocalGraph(nullptr), ConfocalGraphContainer(nullptr),
		ConfocalSurfaceDataProxy(new QSurfaceDataProxy(this)), ConfocalSurfaceDataArray(nullptr),
		ConfocalSurface3DSeries(new QSurface3DSeries(ConfocalSurfaceDataProxy)),
		NumItemsInArray(0),
		ConfocalSurfaceMinCounts(std::numeric_limits<decltype(ConfocalSurfaceMinCounts)>::max()), ConfocalSurfaceMaxCounts(0),
		HBTDataSeries(nullptr), HBTDataChart(nullptr), HBTXAxis(new QValueAxis(this)), HBTYAxis(new QValueAxis(this))
	{
		ui.setupUi(this);

		// For shortcuts
		this->addAction(ui.action_Stop_current_action);
		this->addAction(ui.action_Optimize_positions);
		this->addAction(ui.action_Save_Image);
		this->addAction(ui.action_Save_Image);
		this->addAction(ui.action_Zoom_in);
		this->addAction(ui.action_Zoom_out);
		this->addAction(ui.action_Zoom_reset);
		this->addAction(ui.action_Zoom_fit);
		this->addAction(ui.action_EmitterList_Edit_name);
		this->addAction(ui.action_EmitterList_Remove_entry);

		ui.action_Zoom_fit->setChecked(true);

		// Status bar
		ui.MainStatusBar->addWidget(StatusBar.StateLabel, 4);
		ui.MainStatusBar->addWidget(StatusBar.CellIDLabel, 1);
		ui.MainStatusBar->addWidget(StatusBar.HomePosLabel, 2);
		ui.MainStatusBar->addWidget(StatusBar.PumpPowerLabel, 1);
		ui.MainStatusBar->addPermanentWidget(StatusBar.ImageCoordinatesGroup, 2);

		StatusBar.CellIDContextMenu->addAction(ui.action_Reset_CellID);
		connect(StatusBar.CellIDLabel, &QWidget::customContextMenuRequested, this, &WidefieldMicroscopeWidget::OnCellIDContextMenuRequested);

		// Tool bar
		WidefieldConfocalModeActionGroup->addAction(ui.action_Widefield_mode);
		WidefieldConfocalModeActionGroup->addAction(ui.action_Widefield_mode);
		WidefieldConfocalModeActionGroup->addAction(ui.action_Confocal_mode);

		// Main image
		MainGraphicsView = new Util::MarkerGraphicsView(ui.MainHSplitter);
		MainGraphicsView->setObjectName(QString::fromUtf8("MainGraphicsView"));
		MainGraphicsView->setMinimumSize(QSize(300, 200));
		ui.MainHSplitter->addWidget(MainGraphicsView);

		MainGraphicsPixmapItem = MainGraphicsScene->addPixmap(MainGraphicsPixmap);
		MainGraphicsView->setScene(MainGraphicsScene);
		MainGraphicsView->setAlignment(Qt::AlignLeft | Qt::AlignTop);
		MainGraphicsView->viewport()->installEventFilter(this);
		MainGraphicsView->viewport()->setMouseTracking(true);

		MainGraphicsView->contextMenu()->addSeparator();
		MainGraphicsView->contextMenu()->addAction(ui.action_Zoom_in);
		MainGraphicsView->contextMenu()->addAction(ui.action_Zoom_out);
		MainGraphicsView->contextMenu()->addAction(ui.action_Zoom_reset);
		MainGraphicsView->contextMenu()->addAction(ui.action_Zoom_fit);
		MainGraphicsView->contextMenu()->addSeparator();
		MainGraphicsView->contextMenu()->addAction(ui.action_Save_Image);

		// Emitter list
		EmitterListContextMenu->addAction(ui.action_EmitterList_Edit_name);
		EmitterListContextMenu->addAction(ui.action_EmitterList_Remove_entry);
		EmitterListContextMenu->addSeparator();
		EmitterListContextMenu->addAction(ui.action_EmitterList_Go_to_sample_position);
		EmitterListContextMenu->addAction(ui.action_EmitterList_Bring_marker_to_confocal_spot);
		EmitterListContextMenu->addSeparator();
		EmitterListContextMenu->addAction(ui.action_EmitterList_Run_characterization_from_ID);

		// Graph to display confocal scans
		ConfocalMapContextMenu->addAction(ui.action_confocal_map_save_raw_data);
		ConfocalMapContextMenu->addAction(ui.action_confocal_map_reset);
		ui.BConfocalGraphTools->setMenu(ConfocalMapContextMenu);
		ConfocalGraph = new Q3DSurface(); // Ownership transferred to ConfocalGraphContainer below.
		ConfocalGraphContainer = QWidget::createWindowContainer(ConfocalGraph, ui.WidgetConfocalGraphContainer);
		ConfocalGraphContainer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);		
		ui.WidgetConfocalGraphContainer->layout()->addWidget(ConfocalGraphContainer);
		ConfocalGraph->activeTheme()->setType(DynExpUI::DefaultQ3DTheme);
		ConfocalGraph->activeTheme()->setLabelBorderEnabled(false);
		ConfocalGraph->scene()->activeCamera()->setCameraPreset(Q3DCamera::CameraPreset::CameraPresetDirectlyAbove);
		ConfocalGraph->setShadowQuality(QAbstract3DGraph::ShadowQuality::ShadowQualityNone);
		ConfocalGraph->setOrthoProjection(true);
		ConfocalGraph->setSelectionMode(QAbstract3DGraph::SelectionFlag::SelectionItem);
		
		ConfocalGraph->axisX()->setLabelFormat("%.0f");
		ConfocalGraph->axisX()->setAutoAdjustRange(true);
		ConfocalGraph->axisX()->setLabelAutoRotation(90);
		ConfocalGraph->axisX()->setTitle("X in nm");
		ConfocalGraph->axisX()->setTitleVisible(true);
		ConfocalGraph->axisY()->setLabelFormat("%.0f");
		ConfocalGraph->axisY()->setAutoAdjustRange(false);
		ConfocalGraph->axisY()->setTitle("Count rate in Hz");
		ConfocalGraph->axisY()->setTitleVisible(true);
		ConfocalGraph->axisZ()->setLabelFormat("%.0f");
		ConfocalGraph->axisZ()->setAutoAdjustRange(true);
		ConfocalGraph->axisZ()->setTitle("Y in nm");
		ConfocalGraph->axisZ()->setTitleVisible(true);

		ConfocalSurface3DSeries->setDrawMode(QSurface3DSeries::DrawSurface);
		ConfocalSurface3DSeries->setFlatShadingEnabled(true);
		ConfocalSurface3DSeries->setBaseGradient(DynExpUI::GetDefaultLinearGradient());
		ConfocalSurface3DSeries->setColorStyle(Q3DTheme::ColorStyleRangeGradient);
		ConfocalSurface3DSeries->setItemLabelFormat("(@xLabel nm, @zLabel nm): @yLabel Hz");
		ConfocalGraph->addSeries(ConfocalSurface3DSeries);

		// Graph to display HBT result
		HBTDataChart = new QChart();
		ui.HBTChart->setChart(HBTDataChart);		// Takes ownership of HBTDataChart.
		ui.HBTChart->setRenderHint(QPainter::Antialiasing);
		HBTDataChart->setTheme(DynExpUI::DefaultQChartTheme);
		HBTDataChart->legend()->setVisible(false);
		HBTXAxis->setTitleText("time in ps");
		HBTYAxis->setTitleText("g(2)");

		// Chart takes ownership of axes.
		HBTDataChart->addAxis(HBTXAxis, Qt::AlignBottom);
		HBTDataChart->addAxis(HBTYAxis, Qt::AlignLeft);
	}

	const WidefieldMicroscope& WidefieldMicroscopeWidget::GetCastOwner() const noexcept
	{
		return static_cast<const WidefieldMicroscope&>(GetOwner());
	}

	void WidefieldMicroscopeWidget::InitializeUI(Util::SynchronizedPointer<WidefieldMicroscopeData>& ModuleData)
	{
		if (!GetUIInitialized())
		{
			ui.action_Toogle_LED_light_source->setEnabled(ModuleData->TestFeature(WidefieldMicroscopeData::FeatureType::LEDLightToggle));
			ui.action_Toogle_pump_light_source->setEnabled(ModuleData->TestFeature(WidefieldMicroscopeData::FeatureType::PumpLightToggle));
			ui.GBGeneralLaserPowers->setEnabled(ModuleData->TestFeature(WidefieldMicroscopeData::FeatureType::SetPumpPower));

			auto WidefieldTabIndex = ui.TabWidget->indexOf(ui.TabWidefield);
			auto ConfocalTabIndex = ui.TabWidget->indexOf(ui.TabConfocal);
			auto HBTTabIndex = ui.TabWidget->indexOf(ui.TabHBT);
			ui.TabWidget->setTabVisible(WidefieldTabIndex, ModuleData->TestFeature(WidefieldMicroscopeData::FeatureType::Widefield));
			ui.TabWidget->setTabVisible(ConfocalTabIndex, ModuleData->TestFeature(WidefieldMicroscopeData::FeatureType::Confocal));
			ui.TabWidget->setTabVisible(HBTTabIndex, ModuleData->TestFeature(WidefieldMicroscopeData::FeatureType::HBT));

			ui.SBGeneralWidefieldPower->setMinimum(ModuleData->GetMinPumpPower());
			ui.SBGeneralWidefieldPower->setMaximum(ModuleData->GetMaxPumpPower());
			ui.SBGeneralWidefieldPower->setSingleStep(std::abs(ModuleData->GetMaxPumpPower() - ModuleData->GetMinPumpPower()) / 100.0);
			ui.SBGeneralConfocalPower->setMinimum(ModuleData->GetMinPumpPower());
			ui.SBGeneralConfocalPower->setMaximum(ModuleData->GetMaxPumpPower());
			ui.SBGeneralConfocalPower->setSingleStep(std::abs(ModuleData->GetMaxPumpPower() - ModuleData->GetMinPumpPower()) / 100.0);
			if (ModuleData->TestFeature(WidefieldMicroscopeData::FeatureType::FocusAdjustment))
			{
				ui.SBGeneralFocusCurrentVoltage->setMinimum(ModuleData->GetMinFocusVoltage());
				ui.SBGeneralFocusCurrentVoltage->setMaximum(ModuleData->GetMaxFocusVoltage());
				ui.SBGeneralFocusCurrentVoltage->setSingleStep(std::abs(ModuleData->GetMaxFocusVoltage() - ModuleData->GetMinFocusVoltage()) / 100.0);
				ui.SBGeneralFocusZeroVoltage->setMinimum(ModuleData->GetMinFocusVoltage());
				ui.SBGeneralFocusZeroVoltage->setMaximum(ModuleData->GetMaxFocusVoltage());
				ui.SBGeneralFocusZeroVoltage->setSingleStep(std::abs(ModuleData->GetMaxFocusVoltage() - ModuleData->GetMinFocusVoltage()) / 100.0);
				ui.SBGeneralFocusConfocalOffsetVoltage->setMinimum(-std::abs(ModuleData->GetMinFocusVoltage()) - std::abs(ModuleData->GetMaxFocusVoltage()));
				ui.SBGeneralFocusConfocalOffsetVoltage->setMaximum(std::abs(ModuleData->GetMinFocusVoltage()) + std::abs(ModuleData->GetMaxFocusVoltage()));
				ui.SBGeneralFocusConfocalOffsetVoltage->setSingleStep(std::abs(ModuleData->GetMaxFocusVoltage() - ModuleData->GetMinFocusVoltage()) / 100.0);
			}
			else
				ui.GBGeneralFocus->setVisible(false);
			if (!ModuleData->TestFeature(WidefieldMicroscopeData::FeatureType::WidefieldConfocalSwitch))
			{
				ui.LGeneralWidefieldPower->setText("Power in any mode");
				ui.LGeneralConfocalPower->setVisible(false);
				ui.SBGeneralConfocalPower->setVisible(false);
				ui.LGeneralFocusConfocalOffsetVoltage->setVisible(false);
				ui.SBGeneralFocusConfocalOffsetVoltage->setVisible(false);
			}

			ui.SBWidefieldLEDExposureTime->setMinimum(ModuleData->GetMinCameraExposureTime().count());
			ui.SBWidefieldLEDExposureTime->setMaximum(ModuleData->GetMaxCameraExposureTime().count());
			ui.SBWidefieldPumpExposureTime->setMinimum(ModuleData->GetMinCameraExposureTime().count());
			ui.SBWidefieldPumpExposureTime->setMaximum(ModuleData->GetMaxCameraExposureTime().count());

			ui.TWEmitterList->setColumnWidth(EmitterListColumnType::EmitterName, 96);
			ui.TWEmitterList->setColumnWidth(EmitterListColumnType::Sample_x, 134);
			ui.TWEmitterList->setColumnWidth(EmitterListColumnType::Sample_y, 134);
			ui.TWEmitterList->setColumnWidth(EmitterListColumnType::EmitterState, 134);

			UIInitialized = true;
		}
	}

	void WidefieldMicroscopeWidget::SetUIState(const StateMachineStateType* State, const Util::StateMachineContext<StateMachineStateType>* Context,
		Util::SynchronizedPointer<WidefieldMicroscopeData>& ModuleData)
	{
		bool IsReady = State->GetState() == StateType::Ready;
		StatusBar.CurrentState = State;
		StatusBar.CurrentContext = Context;

		StatusBar.CellIDLabel->setText(ModuleData->HasCellID() ? QString::fromStdString(ModuleData->GetCellID().IDString) : "< Unknown cell ID. >");
		StatusBar.CellIDLabel->setToolTip(ModuleData->HasCellID() ?
			("(" + QString::number(ModuleData->GetCellID().X_id) + ", " + QString::number(ModuleData->GetCellID().Y_id) + ")") : "n/a");

		QString HomePosString = "home: (" + QString::number(ModuleData->GetSampleHomePosition().x) + " nm, " + QString::number(ModuleData->GetSampleHomePosition().y) + " nm)";
		StatusBar.HomePosLabel->setText(ModuleData->GetSampleHomePosition().IsEmpty() ? "< No home position set. >" : HomePosString);

		StatusBar.PumpPowerLabel->setText(ModuleData->TestFeature(WidefieldMicroscopeData::FeatureType::MeasurePumpPower) ?
			(" Pump power: " + QString::number(ModuleData->GetMeasuredPumpPower(), 'f', 2) + " V") : "");
		if (ModuleData->GetPumpLightTurnedOn())
			StatusBar.PumpPowerLabel->setStyleSheet(DynExpUI::StatusBarRunningStyleSheet);
		else
			StatusBar.PumpPowerLabel->setStyleSheet("");

		MainGraphicsView->EnableActions(IsReady);
		ui.action_Save_Image->setEnabled(IsReady);
		ui.action_Stop_current_action->setEnabled(!IsReady);
		ui.action_Set_home_position->setEnabled(IsReady && ModuleData->TestFeature(WidefieldMicroscopeData::FeatureType::Confocal));
		ui.action_Go_home_position->setEnabled(IsReady && ModuleData->TestFeature(WidefieldMicroscopeData::FeatureType::Confocal));
		ui.action_Widefield_mode->setEnabled(IsReady && ModuleData->TestFeature(WidefieldMicroscopeData::FeatureType::WidefieldConfocalSwitch));
		ui.action_Confocal_mode->setEnabled(IsReady && ModuleData->TestFeature(WidefieldMicroscopeData::FeatureType::WidefieldConfocalSwitch));
		ui.action_Autofocus->setEnabled(IsReady && ModuleData->TestFeature(WidefieldMicroscopeData::FeatureType::FocusAdjustment));
		ui.action_Optimize_positions->setEnabled(IsReady && ModuleData->TestFeature(WidefieldMicroscopeData::FeatureType::ConfocalOptimization)
			&& ModuleData->GetSetupMode() == WidefieldMicroscopeData::SetupModeType::Confocal);
		ui.action_Toggle_HBT_mirror->setEnabled(IsReady && ModuleData->TestFeature(WidefieldMicroscopeData::FeatureType::HBTSwitch));
		ui.action_Reset_CellID->setEnabled(IsReady);
		ui.GBGeneralLaserPowers->setEnabled(IsReady && ModuleData->TestFeature(WidefieldMicroscopeData::FeatureType::SetPumpPower));
		ui.GBGeneralFocus->setEnabled(IsReady && ModuleData->TestFeature(WidefieldMicroscopeData::FeatureType::FocusAdjustment));
		ui.SBWidefieldLEDExposureTime->setEnabled(IsReady &&
			ModuleData->TestFeature(WidefieldMicroscopeData::FeatureType::Widefield) && ModuleData->GetWidefieldCamera()->CanSetExposureTime());
		ui.BWidefieldApplyLEDExposureTime->setEnabled(IsReady &&
			ModuleData->TestFeature(WidefieldMicroscopeData::FeatureType::Widefield) && ModuleData->GetWidefieldCamera()->CanSetExposureTime());
		ui.SBWidefieldPumpExposureTime->setEnabled(IsReady &&
			ModuleData->TestFeature(WidefieldMicroscopeData::FeatureType::Widefield) && ModuleData->GetWidefieldCamera()->CanSetExposureTime());
		ui.BWidefieldApplyPumpExposureTime->setEnabled(IsReady &&
			ModuleData->TestFeature(WidefieldMicroscopeData::FeatureType::Widefield) && ModuleData->GetWidefieldCamera()->CanSetExposureTime());
		ui.BWidefieldFindConfocalSpot->setEnabled(IsReady && ModuleData->TestFeature(WidefieldMicroscopeData::FeatureType::WidefieldConfocalSwitch));
		ui.BWidefieldLEDCapture->setEnabled(IsReady);
		ui.BWidefieldCapture->setEnabled(IsReady);
		ui.BReadCellID->setEnabled(IsReady && ModuleData->TestFeature(WidefieldMicroscopeData::FeatureType::WidefieldLocalization));
		ui.BAnalyzeImageDistortion->setEnabled(IsReady && ModuleData->TestFeature(WidefieldMicroscopeData::FeatureType::WidefieldLocalization));
		ui.BLocalizeEmitters->setEnabled(IsReady && ModuleData->TestFeature(WidefieldMicroscopeData::FeatureType::WidefieldLocalization));
		ui.action_EmitterList_Edit_name->setEnabled(IsReady);
		ui.action_EmitterList_Remove_entry->setEnabled(IsReady);
		ui.action_EmitterList_Go_to_sample_position->setEnabled(IsReady);
		ui.action_EmitterList_Bring_marker_to_confocal_spot->setEnabled(IsReady);
		ui.action_EmitterList_Run_characterization_from_ID->setEnabled(IsReady);
		ui.SBConfocalWidth->setEnabled(IsReady);
		ui.SBConfocalHeight->setEnabled(IsReady);
		ui.SBConfocalDistPerPixel->setEnabled(IsReady);
		ui.SBConfocalSPDExposureTime->setEnabled(IsReady);
		ui.SBConfocalOptimizationInitXYStepSize->setEnabled(IsReady && ModuleData->TestFeature(WidefieldMicroscopeData::FeatureType::ConfocalOptimization));
		ui.SBConfocalOptimizationInitZStepSize->setEnabled(IsReady && ModuleData->TestFeature(WidefieldMicroscopeData::FeatureType::ConfocalOptimization));
		ui.SBConfocalOptimizationTolerance->setEnabled(IsReady && ModuleData->TestFeature(WidefieldMicroscopeData::FeatureType::ConfocalOptimization));
		ui.BConfocalScan->setEnabled(IsReady);
		ui.action_confocal_map_save_raw_data->setEnabled(IsReady);
		ui.SBHBTBinWidth->setEnabled(IsReady);
		ui.SBHBTBinCount->setEnabled(IsReady);
		ui.BHBTSaveData->setEnabled(IsReady);
		ui.BHBT->setEnabled(IsReady);
		ui.GBAutoMeasureGeneralSettings->setEnabled(IsReady);
		ui.GBAutoMeasureLocalizationSettings->setEnabled(IsReady);
		ui.GBAutoMeasureCharacterizationSettings->setEnabled(IsReady);
		ui.GBAutoMeasureCellSettings->setEnabled(IsReady);
		ui.BAutoMeasureRunLocalization->setEnabled(IsReady && ModuleData->TestFeature(WidefieldMicroscopeData::FeatureType::Widefield));
		ui.BAutoMeasureRunCharacterization->setEnabled(IsReady);
		ui.BAutoMeasureRunSampleCharacterization->setEnabled(IsReady && ModuleData->TestFeature(WidefieldMicroscopeData::FeatureType::Widefield) &&
			ModuleData->TestFeature(WidefieldMicroscopeData::FeatureType::Confocal) &&
			ModuleData->TestFeature(WidefieldMicroscopeData::FeatureType::WidefieldLocalization));
	}

	void WidefieldMicroscopeWidget::UpdateUIData(Util::SynchronizedPointer<WidefieldMicroscopeData>& ModuleData)
	{
		// Toolbar
		ui.action_Toogle_LED_light_source->setChecked(ModuleData->GetLEDLightTurnedOn());
		ui.action_Toogle_pump_light_source->setChecked(ModuleData->GetPumpLightTurnedOn());
		ui.action_Widefield_mode->setChecked(ModuleData->GetSetupMode() == WidefieldMicroscopeData::SetupModeType::Widefield);
		ui.action_Confocal_mode->setChecked(ModuleData->GetSetupMode() == WidefieldMicroscopeData::SetupModeType::Confocal);

		// General
		// QDoubleSpinBox::setValue() emits QDoubleSpinBox::valueChanged().
		if (!ui.SBGeneralWidefieldPower->hasFocus())
			ui.SBGeneralWidefieldPower->setValue(ModuleData->GetWidefieldPumpPower());
		if (!ui.SBGeneralConfocalPower->hasFocus())
			ui.SBGeneralConfocalPower->setValue(ModuleData->GetConfocalPumpPower());
		if (!ui.SBGeneralFocusCurrentVoltage->hasFocus())
			ui.SBGeneralFocusCurrentVoltage->setValue(ModuleData->GetFocusCurrentVoltage());
		if (!ui.SBGeneralFocusZeroVoltage->hasFocus())
			ui.SBGeneralFocusZeroVoltage->setValue(ModuleData->GetFocusZeroVoltage());
		if (!ui.SBGeneralFocusConfocalOffsetVoltage->hasFocus())
			ui.SBGeneralFocusConfocalOffsetVoltage->setValue(ModuleData->GetFocusConfocalOffsetVoltage());

		// MainGraphicsView
		if (ModuleData->TestFeature(WidefieldMicroscopeData::FeatureType::SampleXYPositioning))
		{
			auto SamplePosition = ModuleData->GetSamplePosition();
			MainGraphicsView->SetCurrentImagePos({ static_cast<qreal>(SamplePosition.x), static_cast<qreal>(SamplePosition.y) });
		}
		else
			MainGraphicsView->SetCurrentImagePos({ });
	}

	void WidefieldMicroscopeWidget::UpdateWidefieldUIData(Util::SynchronizedPointer<WidefieldMicroscopeData>& ModuleData)
	{
		if (!ui.SBWidefieldLEDExposureTime->hasFocus())
			ui.SBWidefieldLEDExposureTime->setValue(ModuleData->GetLEDCameraExposureTime().count());

		if (!ui.SBWidefieldPumpExposureTime->hasFocus())
			ui.SBWidefieldPumpExposureTime->setValue(ModuleData->GetWidefieldCameraExposureTime().count());

		if (ModuleData->IsCurrentImageAvlbl())
		{
			// SetMainGraphicsImage creates a deep copy of the current image.
			SetMainGraphicsImage(ModuleData->GetCurrentImage());
			ModuleData->ResetCurrentImageAvlbl();
		}

		if (!SaveWidefieldImageFilename.isEmpty())
		{
			ModuleData->EnqueueEvent(DynExp::MakeEvent(&GetCastOwner(), &WidefieldMicroscope::OnSaveCurrentImage, SaveWidefieldImageFilename));

			SaveWidefieldImageFilename.clear();
		}

		if (ModuleData->HaveLocalizedPositionsChanged())
		{
			MainGraphicsView->RemoveMarkers(true);
			for (const auto& Position : ModuleData->GetLocalizedPositions())
				MainGraphicsView->AddMarker(Position.second.Position, QColorConstants::DarkYellow, true, Position.first, "Emitter " + Util::ToStr(Position.first));

			ModuleData->ResetLocalizedPositionsChanged();
		}

		if (CurrentConfocalSpotPosition != ModuleData->GetConfocalSpotImagePosition())
		{
			MainGraphicsView->RemoveMarker(CurrentConfocalSpotPosition);
			MainGraphicsView->AddMarker(ModuleData->GetConfocalSpotImagePosition(), QColorConstants::Red, false, -1, "Confocal spot");

			CurrentConfocalSpotPosition = ModuleData->GetConfocalSpotImagePosition();
		}

		if (MainGraphicsView->HaveMarkersChanged())
		{
			// Copy intended.
			const auto LocalizedPositions = ModuleData->GetLocalizedPositions();
			ModuleData->ClearLocalizedPositions();

			bool SortingEnabled = ui.TWEmitterList->isSortingEnabled();
			ui.TWEmitterList->setSortingEnabled(false);

			ui.TWEmitterList->clearContents();
			ui.TWEmitterList->setRowCount(0);

			auto& Markers = MainGraphicsView->GetMarkers();
			for (const auto& Marker : Markers)
			{
				const auto Row = ui.TWEmitterList->rowCount();
				ui.TWEmitterList->insertRow(Row);

				QTableWidgetItem* IDItem = new QTableWidgetItem;
				WidefieldMicroscopeData::LocalizedEmitterStateType LocalizedEmitterState = WidefieldMicroscopeData::LocalizedEmitterStateType::NotSet;
				if (Marker.GetID() >= 0)
				{
					IDItem->setData(Qt::EditRole, Marker.GetID());
					IDItem->setData(Qt::UserRole, Marker.GetID());

					auto OldLocalizedPosition = LocalizedPositions.find(Marker.GetID());
					if (OldLocalizedPosition != LocalizedPositions.cend())
						LocalizedEmitterState = OldLocalizedPosition->second.State;
					ModuleData->AppendLocalizedPosition({ Marker.GetID(), { Marker.GetMarkerPos(), LocalizedEmitterState } });
				}
				else
				{
					IDItem->setData(Qt::UserRole, Util::MarkerGraphicsView::MarkerType::IDType(-1));
					IDItem->setText("n/a");
				}

				ui.TWEmitterList->setItem(Row, EmitterListColumnType::EmitterID, IDItem);
				ui.TWEmitterList->setItem(Row, EmitterListColumnType::EmitterName, new QTableWidgetItem(Marker.GetName().data()));
				ui.TWEmitterList->setItem(Row, EmitterListColumnType::Image_x, new QTableWidgetItem(QString::number(Marker.GetMarkerPos().x())));
				ui.TWEmitterList->item(Row, EmitterListColumnType::Image_x)->setData(Qt::ItemDataRole::UserRole, Marker.GetMarkerPos().x());
				ui.TWEmitterList->setItem(Row, EmitterListColumnType::Image_y, new QTableWidgetItem(QString::number(Marker.GetMarkerPos().y())));
				ui.TWEmitterList->item(Row, EmitterListColumnType::Image_y)->setData(Qt::ItemDataRole::UserRole, Marker.GetMarkerPos().y());
				ui.TWEmitterList->setItem(Row, EmitterListColumnType::Sample_x, new QTableWidgetItem(Marker.GetImagePos().isNull() ? "n/a" : QString::number(Marker.GetImagePos().x(), 'f', 0)));
				ui.TWEmitterList->item(Row, EmitterListColumnType::Sample_x)->setData(Qt::ItemDataRole::UserRole, Marker.GetImagePos().x());
				ui.TWEmitterList->setItem(Row, EmitterListColumnType::Sample_y, new QTableWidgetItem(Marker.GetImagePos().isNull() ? "n/a" : QString::number(Marker.GetImagePos().y(), 'f', 0)));
				ui.TWEmitterList->item(Row, EmitterListColumnType::Sample_y)->setData(Qt::ItemDataRole::UserRole, Marker.GetImagePos().y());
				ui.TWEmitterList->setItem(Row, EmitterListColumnType::EmitterState, new QTableWidgetItem(WidefieldMicroscopeData::GetLocalizedEmitterStateString(LocalizedEmitterState)));
				ui.TWEmitterList->item(Row, EmitterListColumnType::EmitterState)->setForeground(WidefieldMicroscopeData::GetLocalizedEmitterColor(LocalizedEmitterState));
			}

			ui.TWEmitterList->setSortingEnabled(SortingEnabled);

			ModuleData->ResetLocalizedPositionsChanged();
			ModuleData->ResetAutoMeasureCurrentEmitter();
		}

		if (ModuleData->HaveLocalizedPositionsStateChanged())
		{
			for (int i = 0; i < ui.TWEmitterList->rowCount(); ++i)
			{
				auto ID = ui.TWEmitterList->item(i, EmitterListColumnType::EmitterID)->data(Qt::ItemDataRole::UserRole).value<Util::MarkerGraphicsView::MarkerType::IDType>();
				auto Emitter = ModuleData->GetLocalizedPositions().find(ID);
				if (Emitter == ModuleData->GetLocalizedPositions().cend())
					continue;

				ui.TWEmitterList->item(i, EmitterListColumnType::EmitterState)->setText(WidefieldMicroscopeData::GetLocalizedEmitterStateString(Emitter->second.State));
				ui.TWEmitterList->item(i, EmitterListColumnType::EmitterState)->setForeground(WidefieldMicroscopeData::GetLocalizedEmitterColor(Emitter->second.State));
			}

			ModuleData->ClearLocalizedPositionsStateChanged();
		}

		switch (EmitterListTask)
		{
		case EmitterListTaskType::GoToSamplePos:
			ModuleData->EnqueueEvent(DynExp::MakeEvent(&GetCastOwner(), &WidefieldMicroscope::OnGoToSamplePos, SamplePos));
			EmitterListTask = EmitterListTaskType::None;
			break;
		case EmitterListTaskType::MarkerToConfocalSpot:
			ModuleData->EnqueueEvent(DynExp::MakeEvent(&GetCastOwner(), &WidefieldMicroscope::OnBringMarkerToConfocalSpot, MarkerPos, SamplePos));
			EmitterListTask = EmitterListTaskType::None;
			break;
		case EmitterListTaskType::RunCharacterization:
			ModuleData->EnqueueEvent(DynExp::MakeEvent(&GetCastOwner(), &WidefieldMicroscope::OnRunCharacterizationFromID, MarkerID));
			EmitterListTask = EmitterListTaskType::None;
			break;
		default: break;
		}
	}

	void WidefieldMicroscopeWidget::UpdateConfocalUIData(Util::SynchronizedPointer<WidefieldMicroscopeData>& ModuleData)
	{
		if (!ui.SBConfocalWidth->hasFocus())
			ui.SBConfocalWidth->setValue(ModuleData->GetConfocalScanWidth());
		if (!ui.SBConfocalHeight->hasFocus())
			ui.SBConfocalHeight->setValue(ModuleData->GetConfocalScanHeight());
		if (!ui.SBConfocalDistPerPixel->hasFocus())
			ui.SBConfocalDistPerPixel->setValue(ModuleData->GetConfocalScanDistPerPixel());
		if (!ui.SBConfocalOptimizationInitXYStepSize->hasFocus())
			ui.SBConfocalOptimizationInitXYStepSize->setValue(ModuleData->GetConfocalOptimizationInitXYStepSize());
		if (!ui.SBConfocalOptimizationInitZStepSize->hasFocus())
			ui.SBConfocalOptimizationInitZStepSize->setValue(ModuleData->GetConfocalOptimizationInitZStepSize());
		if (!ui.SBConfocalOptimizationTolerance->hasFocus())
			ui.SBConfocalOptimizationTolerance->setValue(ModuleData->GetConfocalOptimizationTolerance());

		if (ModuleData->HasConfocalScanSurfacePlotRows())
		{
			auto Rows = ModuleData->GetConfocalScanSurfacePlotRows();
			NumItemsInArray = 0;
			ConfocalSurfaceMinCounts = std::numeric_limits<decltype(ConfocalSurfaceMinCounts)>::max();
			ConfocalSurfaceMaxCounts = 0;

			// Deletes old array.
			ConfocalSurfaceDataProxy->resetArray(nullptr);
			// Ownership transferred to ConfocalSurfaceDataProxy below with resetArray().
			ConfocalSurfaceDataArray = new QSurfaceDataArray;
			ConfocalSurfaceDataArray->reserve(Util::NumToT<int>(Rows.size()));

			for (auto& Row : Rows)
				*ConfocalSurfaceDataArray << Row.release();

			ConfocalSurfaceDataProxy->resetArray(ConfocalSurfaceDataArray);
		}

		if (ModuleData->GetConfocalScanResults().size() > NumItemsInArray)
		{
			for (auto i = NumItemsInArray; i < ModuleData->GetConfocalScanResults().size(); ++i)
			{
				const auto& ResultItem = ModuleData->GetConfocalScanResults()[i];
				if (ResultItem.first.RowIndex < 0 || ResultItem.first.ColumnIndex < 0)
					continue;

				auto& SurfaceItem = ConfocalSurfaceDataArray->operator[](ResultItem.first.RowIndex)->operator[](ResultItem.first.ColumnIndex);

				SurfaceItem.setY(ResultItem.second);
				ConfocalSurfaceDataProxy->setItem(ResultItem.first.RowIndex, ResultItem.first.ColumnIndex, SurfaceItem);

				ConfocalSurfaceMinCounts = std::min(ConfocalSurfaceMinCounts, ResultItem.second);
				ConfocalSurfaceMaxCounts = std::max(ConfocalSurfaceMaxCounts, ResultItem.second);

				++NumItemsInArray;
			}

			if (ConfocalSurfaceMinCounts < std::numeric_limits<decltype(ConfocalSurfaceMinCounts)>::max() && ConfocalSurfaceMaxCounts > 0)
				ConfocalGraph->axisY()->setRange(ConfocalSurfaceMinCounts, ConfocalSurfaceMaxCounts);
		}
	}

	void WidefieldMicroscopeWidget::UpdateHBTUIData(Util::SynchronizedPointer<WidefieldMicroscopeData>& ModuleData)
	{
		if (!ui.SBHBTBinWidth->hasFocus())
			ui.SBHBTBinWidth->setValue(ModuleData->GetHBTBinWidth().count());
		if (!ui.SBHBTBinCount->hasFocus())
			ui.SBHBTBinCount->setValue(Util::NumToT<int>(ModuleData->GetHBTBinCount()));
		if (!ui.SBHBTAcquisitionTime->hasFocus())
			ui.SBHBTAcquisitionTime->setValue(static_cast<double>(ModuleData->GetHBTMaxIntegrationTime().count()) / std::chrono::microseconds::period::den);
		ui.LEHBTTotalIntegrationTime->setText(QString::number(ModuleData->GetHBTTotalIntegrationTime().count() / std::chrono::microseconds::period::den) + " s");

		if (ui.HBTChart->isVisible())
		{
			HBTDataChart->removeAllSeries();
			HBTDataSeries = new QLineSeries(this);
			HBTDataSeries->append(ModuleData->GetHBTDataPoints());
			HBTDataSeries->setPointsVisible(false);

			HBTXAxis->setRange(ModuleData->GetHBTDataPointsMinValues().x(), ModuleData->GetHBTDataPointsMaxValues().x());
			HBTYAxis->setRange(ModuleData->GetHBTDataPointsMinValues().y(), ModuleData->GetHBTDataPointsMaxValues().y());
			ui.LEHBTMinValue->setText(QString::number(ModuleData->GetHBTDataPointsMinValues().y()));

			HBTDataChart->addSeries(HBTDataSeries);
			HBTDataSeries->attachAxis(HBTDataChart->axes()[0]);
			HBTDataSeries->attachAxis(HBTDataChart->axes()[1]);
		}
	}

	void WidefieldMicroscopeWidget::UpdateAutoMeasureUIData(Util::SynchronizedPointer<WidefieldMicroscopeData>& ModuleData, bool IsCharacterizingSample)
	{
		{
			const QSignalBlocker Blocker(ui.LEAutoMeasureSavePath);
			ui.LEAutoMeasureSavePath->setText(QString::fromUtf16(ModuleData->GetAutoMeasureSavePath().u16string().c_str()));
		} // Blocker unlocked here.
		
		if (!ui.SBAutoMeasureNumberImageSets->hasFocus())
			ui.SBAutoMeasureNumberImageSets->setValue(ModuleData->GetAutoMeasureNumberImageSets());
		if (!ui.SBAutoMeasureInitialImageSetWaitTime->hasFocus())
			ui.SBAutoMeasureInitialImageSetWaitTime->setValue(ModuleData->GetAutoMeasureInitialImageSetWaitTime().count());
		if (!ui.SBAutoMeasureImagePositionScatterRadius->hasFocus())
			ui.SBAutoMeasureImagePositionScatterRadius->setValue(ModuleData->GetAutoMeasureImagePositionScatterRadius());
		if (!ui.CBAutoMeasureLocalize->hasFocus())
			ui.CBAutoMeasureLocalize->setCurrentIndex(ModuleData->GetAutoMeasureLocalizationType());
		if (!ui.CBAutoMeasureOptimize->hasFocus())
			ui.CBAutoMeasureOptimize->setChecked(ModuleData->GetAutoMeasureOptimizeEnabled());
		if (!ui.CBAutoMeasureEnableSpectrum->hasFocus())
			ui.CBAutoMeasureEnableSpectrum->setChecked(ModuleData->GetAutoMeasureSpectrumEnabled());
		if (!ui.CBAutoMeasureEnableHBT->hasFocus())
			ui.CBAutoMeasureEnableHBT->setChecked(ModuleData->GetAutoMeasureHBTEnabled());
		if (!ui.SBAutoMeasureOptimizationAttempts->hasFocus())
			ui.SBAutoMeasureOptimizationAttempts->setValue(ModuleData->GetAutoMeasureNumOptimizationAttempts());
		if (!ui.SBAutoMeasureOptimizationReruns->hasFocus())
			ui.SBAutoMeasureOptimizationReruns->setValue(ModuleData->GetAutoMeasureMaxOptimizationReruns());
		if (!ui.SBAutoMeasureOptimizationMaxDistance->hasFocus())
			ui.SBAutoMeasureOptimizationMaxDistance->setValue(ModuleData->GetAutoMeasureOptimizationMaxDistance());
		if (!ui.SBAutoMeasureCountRateThreshold->hasFocus())
			ui.SBAutoMeasureCountRateThreshold->setValue(ModuleData->GetAutoMeasureCountRateThreshold());
		if (!ui.SBAutoMeasureCellRangeFromX->hasFocus())
			ui.SBAutoMeasureCellRangeFromX->setValue(ModuleData->GetAutoMeasureCellRangeFrom().x());
		if (!ui.SBAutoMeasureCellRangeFromY->hasFocus())
			ui.SBAutoMeasureCellRangeFromY->setValue(ModuleData->GetAutoMeasureCellRangeFrom().y());
		if (!ui.SBAutoMeasureCellRangeToX->hasFocus())
			ui.SBAutoMeasureCellRangeToX->setValue(ModuleData->GetAutoMeasureCellRangeTo().x());
		if (!ui.SBAutoMeasureCellRangeToY->hasFocus())
			ui.SBAutoMeasureCellRangeToY->setValue(ModuleData->GetAutoMeasureCellRangeTo().y());
		if (!ui.SBAutoMeasureCellSkipX->hasFocus())
			ui.SBAutoMeasureCellSkipX->setValue(ModuleData->GetAutoMeasureCellSkip().x());
		if (!ui.SBAutoMeasureCellSkipY->hasFocus())
			ui.SBAutoMeasureCellSkipY->setValue(ModuleData->GetAutoMeasureCellSkip().y());

		if (ModuleData->GetAutoMeasureCurrentImageSet() >= 0 && ModuleData->GetAutoMeasureCurrentImageSet() < ModuleData->GetAutoMeasureNumberImageSets())
		{
			ui.LAutoMeasureCellProgress->setText("Recording image set " + QString::number(ModuleData->GetAutoMeasureCurrentImageSet() + 1) +
				" / " + QString::number(ModuleData->GetAutoMeasureNumberImageSets()));
			ui.PBAutoMeasureCellProgress->setValue(static_cast<int>(100.0 * ModuleData->GetAutoMeasureCurrentImageSet() / ModuleData->GetAutoMeasureNumberImageSets()));
			ui.PBAutoMeasureCellProgress->setVisible(true);
		}
		else if (ModuleData->IsAutoMeasureRunning())
		{
			const auto EmitterCount = std::distance(ModuleData->GetAutoMeasureFirstEmitter(), ModuleData->GetLocalizedPositions().end());
			const auto CurrentEmitter = EmitterCount - std::distance(ModuleData->GetAutoMeasureCurrentEmitter(), ModuleData->GetLocalizedPositions().end());

			ui.LAutoMeasureCellProgress->setText("Emitter " + QString::number(CurrentEmitter + 1) +
				" / " + QString::number(EmitterCount) + ", " +
				QString::number(ModuleData->GetNumFinishedLocalizedPositions()) + " successful, " +
				QString::number(ModuleData->GetNumFailedLocalizedPositions()) + " failed");
			ui.PBAutoMeasureCellProgress->setValue(static_cast<int>(100.0 * CurrentEmitter / EmitterCount));
			ui.PBAutoMeasureCellProgress->setVisible(true);
		}
		else
		{
			ui.LAutoMeasureCellProgress->setText("Ready");
			ui.PBAutoMeasureCellProgress->setVisible(false);
		}

		ui.GBAutoMeasureSampleProgress->setVisible(IsCharacterizingSample && ModuleData->HasCellID());
		if (IsCharacterizingSample && ModuleData->HasCellID())
		{
			const auto CurrentCell = ModuleData->GetAutoMeasureCurrentCellIndex();
			const auto CellCount = ModuleData->GetAutoMeasureCellCount();

			ui.LAutoMeasureSampleProgress->setText("Cell " + QString::number(CurrentCell + 1) +
				" / " + QString::number(CellCount));
			ui.PBAutoMeasureSampleProgress->setValue(static_cast<int>(100.0 * CurrentCell / CellCount));
		}
	}

	void WidefieldMicroscopeWidget::SetMainGraphicsImage(const QImage& Image) noexcept
	{
		MainGraphicsPixmap = QPixmap::fromImage(Image);		// deep copy
		MainGraphicsPixmap.detach();						// just to be sure...
	}

	void WidefieldMicroscopeWidget::UpdateScene()
	{
		if (MainGraphicsView->isVisible() && !MainGraphicsView->visibleRegion().isEmpty())
		{
			MainGraphicsPixmapItem->setPixmap(MainGraphicsPixmap);
			MainGraphicsScene->update();

			OnZoomFitClicked(ui.action_Zoom_fit->isChecked());
		}

		StatusBar.Update();
	}

	bool WidefieldMicroscopeWidget::eventFilter(QObject* obj, QEvent* event)
	{
		if (MainGraphicsView->viewport())
			if (event->type() == QEvent::MouseMove)
			{
				OnImageMouseMove(static_cast<QMouseEvent*>(event));

				return true;
			}

		return QObject::eventFilter(obj, event);
	}

	void WidefieldMicroscopeWidget::resizeEvent(QResizeEvent* event)
	{
		OnZoomFitClicked(ui.action_Zoom_fit->isChecked());
	}

	bool WidefieldMicroscopeWidget::StoreTWEmitterListSelection()
	{
		if (ui.TWEmitterList->selectedItems().empty())
			return false;

		const auto Row = ui.TWEmitterList->selectedItems()[0]->row();

		MarkerPos = { ui.TWEmitterList->item(Row, EmitterListColumnType::Image_x)->data(Qt::ItemDataRole::UserRole).toInt(),
			ui.TWEmitterList->item(Row, EmitterListColumnType::Image_y)->data(Qt::ItemDataRole::UserRole).toInt() };
		SamplePos = { ui.TWEmitterList->item(Row, EmitterListColumnType::Sample_x)->data(Qt::ItemDataRole::UserRole).toReal(),
			ui.TWEmitterList->item(Row, EmitterListColumnType::Sample_y)->data(Qt::ItemDataRole::UserRole).toReal() };
		MarkerID = ui.TWEmitterList->item(Row, EmitterListColumnType::EmitterID)->data(Qt::ItemDataRole::UserRole).value<decltype(MarkerID)>();

		return true;
	}

	void WidefieldMicroscopeWidget::OnSaveImageClicked()
	{
		auto Filename = Util::PromptSaveFilePathModule(this, "Save image", ".png", "Portable Network Graphics image (*.png)");
		if (Filename.isEmpty())
			return;

		SaveWidefieldImageFilename = Filename;
	}

	void WidefieldMicroscopeWidget::OnZoomResetClicked()
	{
		ui.action_Zoom_fit->setChecked(false);
		MainGraphicsView->ZoomReset();
	}

	void WidefieldMicroscopeWidget::OnZoomInClicked()
	{
		ui.action_Zoom_fit->setChecked(false);
		MainGraphicsView->ZoomIn();
	}

	void WidefieldMicroscopeWidget::OnZoomOutClicked()
	{
		ui.action_Zoom_fit->setChecked(false);
		MainGraphicsView->ZoomOut();
	}

	void WidefieldMicroscopeWidget::OnZoomFitClicked(bool Checked)
	{
		if (Checked)
			MainGraphicsView->fitInView(MainGraphicsPixmapItem, Qt::AspectRatioMode::KeepAspectRatio);
	}

	void WidefieldMicroscopeWidget::OnImageMouseMove(QMouseEvent* Event)
	{
		auto LocalPoint = MainGraphicsView->mapFromGlobal(Event->globalPos());

		if (!MainGraphicsView->items(LocalPoint).empty())
		{
			auto Point = MainGraphicsView->mapToScene(LocalPoint).toPoint();

			StatusBar.XCoordLabel->setText("X:" + QString::number(Point.x()));
			StatusBar.YCoordLabel->setText("Y:" + QString::number(Point.y()));
		}
	}

	void WidefieldMicroscopeWidget::OnCellIDContextMenuRequested(const QPoint& Position)
	{
		StatusBar.CellIDContextMenu->exec(StatusBar.CellIDLabel->mapToGlobal(Position));
	}

	void WidefieldMicroscopeWidget::OnEmitterListCellClicked(int Row, int Column)
	{
		if (!StoreTWEmitterListSelection())
			MainGraphicsView->DeselectMarkers();
		else
			MainGraphicsView->SelectMarker(MarkerPos);
	}

	void WidefieldMicroscopeWidget::OnEmitterListCellDoubleClicked(int Row, int Column)
	{
		if (StoreTWEmitterListSelection())
			EmitterListTask = EmitterListTaskType::MarkerToConfocalSpot;
	}

	void WidefieldMicroscopeWidget::OnEmitterListContextMenuRequested(const QPoint& Position)
	{
		EmitterListContextMenu->exec(ui.TWEmitterList->mapToGlobal(Position));
	}

	void WidefieldMicroscopeWidget::OnEmitterListDeleteEntryClicked()
	{
		for (int i = 0; i < ui.TWEmitterList->selectedItems().size(); ++i)
		{
			const auto Row = ui.TWEmitterList->selectedItems()[i]->row();
			const QPoint Pos = { ui.TWEmitterList->item(Row, EmitterListColumnType::Image_x)->data(Qt::ItemDataRole::UserRole).toInt(),
				ui.TWEmitterList->item(Row, EmitterListColumnType::Image_y)->data(Qt::ItemDataRole::UserRole).toInt() };

			MainGraphicsView->RemoveMarker(Pos, true);
		}
	}

	void WidefieldMicroscopeWidget::OnEmitterListEditEntryClicked()
	{
		if (ui.TWEmitterList->selectedItems().empty() || !StoreTWEmitterListSelection())
			return;

		const auto Row = ui.TWEmitterList->selectedItems()[0]->row();
		const auto OldName = ui.TWEmitterList->item(Row, EmitterListColumnType::EmitterName)->text();

		bool OKClicked = false;
		QString NewName = QInputDialog::getText(this, "Edit name",
			QString("Enter new name for marker \"") + OldName + "\":", QLineEdit::Normal, OldName, &OKClicked);

		if (OKClicked)
			MainGraphicsView->RenameMarker(MarkerPos, NewName.toStdString());
	}

	void WidefieldMicroscopeWidget::OnEmitterListGoToSamplePositionClicked()
	{
		if (StoreTWEmitterListSelection())
			EmitterListTask = EmitterListTaskType::GoToSamplePos;
	}

	void WidefieldMicroscopeWidget::OnEmitterListBringMarkerToConfocalSpotClicked()
	{
		if (StoreTWEmitterListSelection())
			EmitterListTask = EmitterListTaskType::MarkerToConfocalSpot;
	}

	void WidefieldMicroscopeWidget::OnEmitterListRunCharacterizationFromID()
	{
		if (StoreTWEmitterListSelection())
			EmitterListTask = EmitterListTaskType::RunCharacterization;
	}

	void WidefieldMicroscopeWidget::OnConfocalMapResetClicked()
	{
		ConfocalGraph->scene()->activeCamera()->setCameraPreset(Q3DCamera::CameraPreset::CameraPresetDirectlyAbove);
	}

	void WidefieldMicroscopeWidget::OnConfocalMapSaveRawDataClicked()
	{
		if (!ConfocalSurfaceDataArray)
			return;

		auto Filename = Util::PromptSaveFilePathModule(this, "Save data", ".csv", " Comma-separated values file (*.csv)");
		if (Filename.isEmpty())
			return;

		std::stringstream CSVData;
		{
			auto ModuleData = DynExp::dynamic_ModuleData_cast<WidefieldMicroscope>(GetOwner().GetModuleData());

			CSVData = ModuleData->AssembleCSVHeader(true, false, false);
			ModuleData->WriteConfocalScanResults(CSVData);
		} // ModuleData unlocked here.

		if (!Util::SaveToFile(Filename, CSVData.str()))
			QMessageBox::warning(this, "DynExp - Error", "Error writing data to file.");
	}

	void WidefieldMicroscopeWidget::OnHBTSaveRawDataClicked()
	{
		auto Filename = Util::PromptSaveFilePathModule(this, "Save data", ".csv", " Comma-separated values file (*.csv)");
		if (Filename.isEmpty())
			return;

		std::stringstream CSVData;
		{
			auto ModuleData = DynExp::dynamic_ModuleData_cast<WidefieldMicroscope>(GetOwner().GetModuleData());

			CSVData = ModuleData->AssembleCSVHeader(false, true, false);
			ModuleData->WriteHBTResults(CSVData);
		} // ModuleData unlocked here.

		if (!Util::SaveToFile(Filename, CSVData.str()))
			QMessageBox::warning(this, "DynExp - Error", "Error writing data to file.");
	}

	void WidefieldMicroscopeWidget::OnAutoMeasureSavePathBrowseClicked()
	{
		auto Filename = Util::PromptSaveFilePathModule(this, "Select directory and filename prefix for saving data in auto-measure mode",
			".csv", " Comma-separated values file (*.csv)");
		if (Filename.isEmpty())
			return;

		// Emits signal to update module data accordingly.
		ui.LEAutoMeasureSavePath->setText(Filename);
	}
}