// This file is part of DynExp.

#include "stdafx.h"
#include "moc_ImageViewer.cpp"
#include "ImageViewer.h"

namespace DynExpModule::ImageViewer
{
	ImageViewerWidget::ImageViewerWidget(ImageViewer& Owner, QModuleWidget* parent)
		: QModuleWidget(Owner, parent),
		HistogramContextMenu(new QMenu(this)), HistogramLinLogActionGroup(new QActionGroup(this)),
		HistogramBarSetI(nullptr), HistogramBarSetR(nullptr), HistogramBarSetG(nullptr), HistogramBarSetB(nullptr),
		HistogramBarSeries(new QBarSeries(this)), HistogramChart(nullptr), HistogramXAxis(new QValueAxis(this)), HistogramYAxis(new QValueAxis(this)),
		GraphicsView(nullptr), GraphicsPixmapItem(nullptr), GraphicsScene(new QGraphicsScene(this))
	{
		ui.setupUi(this);

		ui.action_Zoom_fit->setChecked(true);

		HistogramLinAction = HistogramLinLogActionGroup->addAction("&Linear");
		HistogramLinAction->setCheckable(true);
		HistogramLinAction->setChecked(true);
		HistogramLogAction = HistogramLinLogActionGroup->addAction("Lo&garithmic");
		HistogramLogAction->setCheckable(true);
		HistogramContextMenu->addActions(HistogramLinLogActionGroup->actions());
		HistogramContextMenu->addSeparator();
		HistogramBWAction = HistogramContextMenu->addAction("Show &intensity");
		HistogramBWAction->setCheckable(true);
		HistogramBWAction->setChecked(true);
		HistogramColorAction = HistogramContextMenu->addAction("Show &colors");
		HistogramColorAction->setCheckable(true);

		HistogramChart = new QChart();
		ui.Histogram->setChart(HistogramChart);				// Takes ownership of HistogramChart.
		ui.Histogram->setRenderHint(QPainter::Antialiasing);
		HistogramChart->addSeries(HistogramBarSeries);
		HistogramChart->setTheme(DynExpUI::DefaultQChartTheme);
		HistogramChart->legend()->setVisible(false);
		HistogramXAxis->setTitleText("pixel value");
		HistogramXAxis->setLabelFormat("%d");
		HistogramXAxis->setRange(0, 255);
		HistogramXAxis->setTickCount(255 / 85 + 1);
		HistogramChart->addAxis(HistogramXAxis, Qt::AlignBottom);
		HistogramYAxis->setTitleText("counts");
		HistogramYAxis->setLabelFormat("%.0e");
		HistogramYAxis->setRange(0, 1);
		HistogramChart->addAxis(HistogramYAxis, Qt::AlignLeft);

		GraphicsView = new Util::MarkerGraphicsView(ui.MainSplitter);
		GraphicsView->setObjectName(QString::fromUtf8("Image"));
		QSizePolicy ImageSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
		ImageSizePolicy.setHorizontalStretch(1);
		ImageSizePolicy.setVerticalStretch(0);
		ImageSizePolicy.setHeightForWidth(GraphicsView->sizePolicy().hasHeightForWidth());
		GraphicsView->setSizePolicy(ImageSizePolicy);
		GraphicsView->setMinimumSize(QSize(400, 300));
		GraphicsView->viewport()->setProperty("cursor", QVariant(QCursor(Qt::CrossCursor)));
		GraphicsView->setSizeAdjustPolicy(QAbstractScrollArea::AdjustToContentsOnFirstShow);

		GraphicsPixmapItem = GraphicsScene->addPixmap(Pixmap);
		GraphicsView->setScene(GraphicsScene);
		GraphicsView->setAlignment(Qt::AlignLeft | Qt::AlignTop);
		GraphicsView->viewport()->installEventFilter(this);
		GraphicsView->viewport()->setMouseTracking(true);
	}

	void ImageViewerWidget::SetImage(const QImage& NewImage) noexcept
	{
		Pixmap = QPixmap::fromImage(NewImage);	// deep copy
		Pixmap.detach();						// just to be sure...
	}

	void ImageViewerWidget::SetImageViewEnabled(bool Enable)
	{
		if (GraphicsView)
			GraphicsView->setEnabled(Enable);
	}

	void ImageViewerWidget::SetIntensityHistogram(Util::ImageHistogramType&& NewIntensityHistogram) noexcept
	{
		IntensityHistogram = std::move(NewIntensityHistogram);
	}

	void ImageViewerWidget::SetRGBHistogram(Util::ImageRGBHistogramType&& NewRGBHistogram) noexcept
	{
		RGBHistogram = std::move(NewRGBHistogram);
	}

	auto ImageViewerWidget::GetComputeHistogram() const noexcept
	{
		using CHT = DynExpInstr::CameraData::ComputeHistogramType;

		if (!ui.ExposureTimeGroupBox->isVisible() || ui.ExposureTimeGroupBox->visibleRegion().isEmpty())
			return CHT::NoHistogram;

		if (HistogramBWAction->isChecked())
			return HistogramColorAction->isChecked() ? CHT::IntensityAndRGBHistogram : CHT::IntensityHistogram;
		if (HistogramColorAction->isChecked())
			return CHT::RGBHistogram;

		return CHT::NoHistogram;
	}

	void ImageViewerWidget::UpdateScene()
	{
		if (GraphicsView->isVisible() && !GraphicsView->visibleRegion().isEmpty() && !Pixmap.isNull())
		{
			GraphicsPixmapItem->setPixmap(Pixmap);
			GraphicsScene->update();

			OnZoomFitClicked(ui.action_Zoom_fit->isChecked());
		}

		if (ui.ExposureTimeGroupBox->isVisible() && !ui.ExposureTimeGroupBox->visibleRegion().isEmpty())
			UpdateHistogram();

		ui.ImageGeometry->setText(QString::number(Pixmap.width()) + " x " + QString::number(Pixmap.height()));
	}

	bool ImageViewerWidget::eventFilter(QObject* obj, QEvent* event)
	{
		if (GraphicsView->viewport())
			if (event->type() == QEvent::MouseMove)
			{
				OnImageMouseMove(static_cast<QMouseEvent*>(event));

				return true;
			}
			
		return QObject::eventFilter(obj, event);
	}

	void ImageViewerWidget::resizeEvent(QResizeEvent* event)
	{
		OnZoomFitClicked(ui.action_Zoom_fit->isChecked());
	}

	void ImageViewerWidget::UpdateHistogram()
	{
		using CHT = DynExpInstr::CameraData::ComputeHistogramType;

		auto ComputeHistogram = GetComputeHistogram();
		if (ComputeHistogram == CHT::NoHistogram || IntensityHistogram.size() >= std::numeric_limits<int>::max())
			return;

		// Remove all existing data series.
		if (HistogramBarSetI)
			if (HistogramBarSeries->remove(HistogramBarSetI))
				HistogramBarSetI = nullptr;
		if (HistogramBarSetR)
			if (HistogramBarSeries->remove(HistogramBarSetR))
				HistogramBarSetR = nullptr;
		if (HistogramBarSetG)
			if (HistogramBarSeries->remove(HistogramBarSetG))
				HistogramBarSetG = nullptr;
		if (HistogramBarSetB)
			if (HistogramBarSeries->remove(HistogramBarSetB))
				HistogramBarSetB = nullptr;

		// Some removals failed. Do not continue in order to avoid memory leaks.
		if (HistogramBarSetI || HistogramBarSetR || HistogramBarSetG || HistogramBarSetB)
			return;

		HistogramChart->removeSeries(HistogramBarSeries);

		// Create new data series.
		if (ComputeHistogram == CHT::IntensityHistogram ||
			ComputeHistogram == CHT::IntensityAndRGBHistogram)
		{
			HistogramBarSetI = new QBarSet("I", this);
			HistogramBarSetI->setColor(Qt::white);
			HistogramBarSetI->setBorderColor(Qt::white);
		}
		
		if (ComputeHistogram == CHT::RGBHistogram ||
			ComputeHistogram == CHT::IntensityAndRGBHistogram)
		{
			HistogramBarSetR = new QBarSet("R", this);
			HistogramBarSetG = new QBarSet("G", this);
			HistogramBarSetB = new QBarSet("B", this);
			HistogramBarSetR->setColor(Qt::red);
			HistogramBarSetR->setBorderColor(Qt::red);
			HistogramBarSetG->setColor(Qt::green);
			HistogramBarSetG->setBorderColor(Qt::green);
			HistogramBarSetB->setColor(Qt::blue);
			HistogramBarSetB->setBorderColor(Qt::blue);
		}

		bool LogPlot = HistogramLogAction->isChecked();
		qreal MaxValue = 1;
		for (int i = 0; i < static_cast<int>(IntensityHistogram.size()); ++i)
		{
			if (HistogramBarSetI)
			{
				// Add .1 in log case to avoid NaN if value is 0.
				*HistogramBarSetI << (LogPlot ? std::log10(IntensityHistogram[i] + .1) : IntensityHistogram[i]);

				MaxValue = std::max(MaxValue, HistogramBarSetI->at(i));
			}

			if (HistogramBarSetR && HistogramBarSetG && HistogramBarSetB)
			{
				// Add .1 in log case to avoid NaN if value is 0.
				*HistogramBarSetR << (LogPlot ? std::log10(std::get<0>(RGBHistogram)[i] + .1) : std::get<0>(RGBHistogram)[i]);
				*HistogramBarSetG << (LogPlot ? std::log10(std::get<1>(RGBHistogram)[i] + .1) : std::get<1>(RGBHistogram)[i]);
				*HistogramBarSetB << (LogPlot ? std::log10(std::get<2>(RGBHistogram)[i] + .1) : std::get<2>(RGBHistogram)[i]);

				MaxValue = std::max({ MaxValue, HistogramBarSetR->at(i), HistogramBarSetG->at(i), HistogramBarSetB->at(i) });
			}
		}

		if (HistogramBarSetI)
			HistogramBarSeries->append(HistogramBarSetI);
		if (HistogramBarSetR && HistogramBarSetG && HistogramBarSetB)
			HistogramBarSeries->append({ HistogramBarSetR, HistogramBarSetG, HistogramBarSetB });
		
		HistogramChart->addSeries(HistogramBarSeries);
		HistogramYAxis->setMax(MaxValue);
		HistogramBarSeries->attachAxis(HistogramXAxis);
		HistogramBarSeries->attachAxis(HistogramYAxis);
	}

	void ImageViewerWidget::OnHistogramContextMenuRequested(const QPoint& Position)
	{
		HistogramContextMenu->exec(ui.Histogram->mapToGlobal(Position));
	}

	void ImageViewerWidget::OnSaveImageClicked()
	{
		auto Filename = Util::PromptSaveFilePathModule(this, "Save image", ".png", "Portable Network Graphics image (*.png)");
		if (Filename.isEmpty())
			return;

		SaveImageFilename = Filename;
	}

	void ImageViewerWidget::OnZoomResetClicked()
	{
		ui.action_Zoom_fit->setChecked(false);
		GraphicsView->ZoomReset();
	}

	void ImageViewerWidget::OnZoomInClicked()
	{
		ui.action_Zoom_fit->setChecked(false);
		GraphicsView->ZoomIn();
	}

	void ImageViewerWidget::OnZoomOutClicked()
	{
		ui.action_Zoom_fit->setChecked(false);
		GraphicsView->ZoomOut();
	}

	void ImageViewerWidget::OnZoomFitClicked(bool Checked)
	{
		if (Checked)
			GraphicsView->fitInView(GraphicsPixmapItem, Qt::AspectRatioMode::KeepAspectRatio);
	}

	void ImageViewerWidget::OnImageMouseMove(QMouseEvent* Event)
	{
		auto LocalPoint = GraphicsView->mapFromGlobal(Event->globalPos());

		if (!GraphicsView->items(LocalPoint).empty())
		{
			auto Point = GraphicsView->mapToScene(LocalPoint).toPoint();

			ui.CursorPosition->setText("X:" + QString::number(Point.x()) + ", Y:" + QString::number(Point.y()));
		}
	}

	void ImageViewerData::ResetImpl(dispatch_tag<QModuleDataBase>)
	{
		Init();
	}

	/**
	 * @brief Calculates the Brenner gradient of @p CurrentImage.
	 * Refer to J. F. Brenner et al. J. Histochem. Cytochem. 24 (1), 100-111 (1976).
	 * @return Brenner gradient or NaN if @p CurrentImage is empty.
	*/
	double ImageViewerData::CalcBrennerGradientFromImage() const
	{
		if (CurrentImage.isNull())
			return std::numeric_limits<double>::quiet_NaN();

		const auto GrayImage = CurrentImage.convertToFormat(QImage::Format::Format_Grayscale8);
		auto RawImage = GrayImage.constBits();
		
		double BrennerGradient = .0;
		for (int y = 0; y < GrayImage.height(); ++y)
		{
			for (int x = 0; x < GrayImage.width() - 2; ++x)
			{
				BrennerGradient += std::pow((static_cast<double>(*RawImage) - static_cast<double>(*(RawImage + 2))) / 255.0, 2);

				++RawImage;
			}

			RawImage += 2;
		}

		// Multiply by 1000 for convenience since we are normally dealing with very small gradients.
		return BrennerGradient / GrayImage.width() / GrayImage.height() * 1e3;
	}

	void ImageViewerData::Init()
	{
		UIInitialized = false;

		CameraModes.clear();
		CapturingState = DynExpInstr::CameraData::CapturingStateType::Stopped;
		TimeType MinExposureTime = TimeType();
		TimeType MaxExposureTime = TimeType();
		TimeType CurrentExposureTime = TimeType();
		CurrentFPS = 0.f;
		ComputeHistogram = DynExpInstr::CameraData::ComputeHistogramType::NoHistogram;

		CurrentImage = QImage();
		HasImageChanged = false;
		ImageCapturingPaused = false;
		CaptureAfterPause = false;

		IntensityHistogram = {};
		RGBHistogram = {};
	}

	ImageViewer::ImageViewer(const std::thread::id OwnerThreadID, DynExp::ParamsBasePtrType&& Params)
		: QModuleBase(OwnerThreadID, std::move(Params)),
		StateMachine(ReadyState,
			AutofocusInitState, AutofocusGotoSampleState, AutofocusWaitBeforeCaptureState,
			AutofocusWaitForImageState, AutofocusStepState, AutofocusFinishedState),
		PauseUpdatingUI(std::make_shared<std::atomic<bool>>(false))
	{
	}

	ImageViewer::~ImageViewer()
	{
	}

	void ImageViewer::OnSaveImage(DynExp::ModuleInstance* Instance, QString Filename) const
	{
		QImage Image;
		{
			auto ModuleData = DynExp::dynamic_ModuleData_cast<ImageViewer>(Instance->ModuleDataGetter());
			Image = ModuleData->CurrentImage.copy();
		} // ModuleData unlocked here for heavy save operation.

		if (!Image.save(Filename))
			Util::EventLog().Log("Image Viewer: Saving the current image failed.", Util::ErrorType::Error);
	}

	Util::DynExpErrorCodes::DynExpErrorCodes ImageViewer::ModuleMainLoop(DynExp::ModuleInstance& Instance)
	{
		try
		{
			StateMachine.Invoke(*this, Instance);

			NumFailedUpdateAttempts = 0;
		} // ModuleData and CameraData unlocked here.
		catch (const Util::TimeoutException& e)
		{
			if (NumFailedUpdateAttempts++ >= 3)
				Instance.GetOwner().SetWarning(e);
		}

		return Util::DynExpErrorCodes::NoError;
	}

	void ImageViewer::ResetImpl(dispatch_tag<QModuleBase>)
	{
		StateMachine.SetCurrentState(StateType::Ready);

		AutofocusParams = {};
		AutofocusResults = {};
		AutofocusIsPerformingFineSweep = false;
		AutofocusSamples.clear();
		AutofocusCurrentSample = AutofocusSamples.end();

		*PauseUpdatingUI = false;
		NumFailedUpdateAttempts = 0;
	}

	std::unique_ptr<DynExp::QModuleWidget> ImageViewer::MakeUIWidget()
	{
		auto Widget = std::make_unique<ImageViewerWidget>(*this);

		Connect(Widget->ui.CBCameraMode, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &ImageViewer::OnCameraModeChanged);
		Connect(Widget->ui.ExposureTime, QOverload<int>::of(&QSpinBox::valueChanged), this, &ImageViewer::OnExposureTimeChanged);
		Connect(Widget->ui.action_Capture_Frame, &QAction::triggered, this, &ImageViewer::OnCaptureSingle);
		Connect(Widget->ui.action_Capture_continuously, &QAction::triggered, this, &ImageViewer::OnCaptureContinuously);
		Connect(Widget->ui.action_Autofocus, &QAction::triggered, this, &ImageViewer::OnAutofocusClicked);

		return Widget;
	}

	void ImageViewer::UpdateUIChild(const ModuleBase::ModuleDataGetterType& ModuleDataGetter)
	{
		using CHT = DynExpInstr::CameraData::ComputeHistogramType;

		if (*PauseUpdatingUI)
			return;

		auto Widget = GetWidget<ImageViewerWidget>();
		auto ModuleData = DynExp::dynamic_ModuleData_cast<ImageViewer>(ModuleDataGetter());

		// ModuleData is locked now. This syncs writing CameraData's settings with OnAutofocus().
		const bool Ready = IsReadyState();
		const bool Autofocusing = IsAutofocusingState();

		if (!ModuleData->UIInitialized)
		{
			Widget->ui.action_Autofocus->setEnabled(ModuleData->Focus.valid());

			if (ModuleData->CameraModes.empty())
				Widget->ui.CameraModeGroupBox->setVisible(false);
			else
			{
				for (const auto& Mode : ModuleData->CameraModes)
				{
					Widget->ui.CBCameraMode->insertItem(Widget->ui.CBCameraMode->count(), QString::fromStdString(Mode));
					Widget->ui.CBCameraMode->setItemData(Widget->ui.CBCameraMode->count() - 1, QString::fromStdString(Mode), Qt::ToolTipRole);
				}

				// Triggers QComboBox::currentIndexChanged().
				Widget->ui.CBCameraMode->setCurrentIndex(0);
			}

			ModuleData->UIInitialized = true;
		}

		Widget->ui.action_Save_Image->setEnabled(Ready);
		Widget->ui.action_Capture_Frame->setEnabled(Ready);
		Widget->ui.action_Capture_continuously->setEnabled(Ready);
		Widget->ui.CameraModeGroupBox->setEnabled(Ready);
		Widget->ui.ImageModifiersGroupBox->setEnabled(Ready);
		Widget->ui.Histogram->setEnabled(Ready);
		Widget->SetImageViewEnabled(Ready);

		Widget->ui.ExposureTimeGroupBox->setEnabled(Ready && ModuleData->Camera->CanSetExposureTime());
		Widget->ui.ExposureTimeMinValue->setText("min. " + QString::number(ModuleData->MinExposureTime.count()) + " ms");
		Widget->ui.ExposureTimeMaxValue->setText("max. " + QString::number(ModuleData->MaxExposureTime.count()) + " ms");
		Widget->ui.ExposureTime->setMinimum(ModuleData->MinExposureTime.count());
		Widget->ui.ExposureTime->setMaximum(ModuleData->MaxExposureTime.count());

		if (!Widget->ui.ExposureTime->hasFocus())
		{
			const QSignalBlocker Blocker(Widget->ui.ExposureTime);
			Widget->ui.ExposureTime->setValue(Util::NumToT<int>(ModuleData->CurrentExposureTime.count()));
		}

		if (!Autofocusing && !ModuleData->ImageCapturingPaused)
		{
			DynExpInstr::CameraData::ImageTransformationType ImageTransformation;
			if (Widget->ui.ImageModifiersGroupBox->isChecked())
			{
				// Values from controls ranging from -10 to 10.
				ImageTransformation.BrightnessFactor = Widget->ui.ImageModifiersBrightness->value() / 10.f;
				ImageTransformation.ContrastFactor = std::pow(10, Widget->ui.ImageModifiersContrast->value() / 10.f);
				ImageTransformation.IsEnabled = true;
			}

			{
				auto CameraData = DynExp::dynamic_InstrumentData_cast<DynExpInstr::Camera>(ModuleData->Camera->GetInstrumentData());
				CameraData->SetImageTransformation(ImageTransformation);
			} // CameraData unlocked here.
		}

		ModuleData->ComputeHistogram = Widget->GetComputeHistogram();

		if (!ModuleData->CurrentImage.isNull() && ModuleData->HasImageChanged)
		{
			// SetImage creates a deep copy of CurrentImage.
			Widget->SetImage(ModuleData->CurrentImage);
			ModuleData->HasImageChanged = false;

			if (ModuleData->ComputeHistogram == CHT::IntensityHistogram ||
				ModuleData->ComputeHistogram == CHT::IntensityAndRGBHistogram)
				Widget->SetIntensityHistogram(std::move(ModuleData->IntensityHistogram));
			if (ModuleData->ComputeHistogram == CHT::RGBHistogram ||
				ModuleData->ComputeHistogram == CHT::IntensityAndRGBHistogram)
				Widget->SetRGBHistogram(std::move(ModuleData->RGBHistogram));

			Widget->UpdateScene();
		}

		if (Ready && !Widget->GetSaveImageFilename().isEmpty())
		{
			ModuleData->Camera->StopCapturing();
			MakeAndEnqueueEvent(this, &ImageViewer::OnSaveImage, Widget->GetSaveImageFilename());

			Widget->ResetSaveImageFilename();
		}

		Widget->ui.action_Capture_continuously->setChecked(ModuleData->CapturingState == DynExpInstr::CameraData::CapturingStateType::CapturingContinuously);
		Widget->ui.action_Autofocus->setChecked(Autofocusing);

		if (Autofocusing)
			Widget->ui.CurrentFPS->setText("Autofocusing...");
		else if (ModuleData->CapturingState == DynExpInstr::CameraData::CapturingStateType::CapturingSingle)
			Widget->ui.CurrentFPS->setText("Capturing frame...");
		else if (ModuleData->CapturingState == DynExpInstr::CameraData::CapturingStateType::CapturingContinuously)
		{
#ifdef DYNEXP_DEBUG
			Widget->ui.CurrentFPS->setText("Capturing... (FPS: " + QString::number(ModuleData->CurrentFPS, 'f', 1) +
				+ ", Brenner gradient: " + QString::number(ModuleData->CalcBrennerGradientFromImage(), 'f', 3) + ")");
#else
			Widget->ui.CurrentFPS->setText("Capturing... (FPS: " + QString::number(ModuleData->CurrentFPS, 'f', 1) + ")");
#endif // DYNEXP_DEBUG
		}
		else
			Widget->ui.CurrentFPS->setText("Stopped");
	}

	bool ImageViewer::IsReadyState() const
	{
		const auto CurrentState = StateMachine.GetCurrentState()->GetState();

		return CurrentState == StateType::Ready;
	}

	bool ImageViewer::IsAutofocusingState() const
	{
		const auto CurrentState = StateMachine.GetCurrentState()->GetState();

		return CurrentState == StateType::AutofocusInit ||
			CurrentState == StateType::AutofocusGotoSample ||
			CurrentState == StateType::AutofocusWaitBeforeCapture ||
			CurrentState == StateType::AutofocusWaitForImage ||
			CurrentState == StateType::AutofocusStep ||
			CurrentState == StateType::AutofocusFinished;
	}

	void ImageViewer::FinishAutofocus(Util::SynchronizedPointer<ModuleDataType>& ModuleData, const FinishedAutofocusEvent& Event) const
	{
		if (ModuleData->Communicator.valid())
			ModuleData->Communicator->PostEvent(*this, Event);

		if (ModuleData->CaptureAfterPause)
			ModuleData->Camera->StartCapturing();
	}

	void ImageViewer::OnInit(DynExp::ModuleInstance* Instance) const
	{
		PauseImageCapturingEvent::Register(*this, &ImageViewer::OnPauseImageCapturing);
		ResumeImageCapturingEvent::Register(*this, &ImageViewer::OnResumeImageCapturing);
		AutofocusEvent::Register(*this, &ImageViewer::OnAutofocus);

		auto ModuleParams = DynExp::dynamic_Params_cast<ImageViewer>(Instance->ParamsGetter());
		auto ModuleData = DynExp::dynamic_ModuleData_cast<ImageViewer>(Instance->ModuleDataGetter());

		Instance->LockObject(ModuleParams->Camera, ModuleData->Camera);
		if (ModuleParams->Focus.ContainsID())
		{
			Instance->LockObject(ModuleParams->Focus, ModuleData->Focus);

			AutofocusParams.MinVoltage = ModuleData->Focus->GetUserMinValue();
			AutofocusParams.MaxVoltage = ModuleData->Focus->GetUserMaxValue();
		}
		if (ModuleParams->Communicator.ContainsID())
			Instance->LockObject(ModuleParams->Communicator, ModuleData->Communicator);

		auto CameraData = DynExp::dynamic_InstrumentData_cast<DynExpInstr::Camera>(ModuleData->Camera->GetInstrumentData());
		ModuleData->CameraModes = CameraData->GetCameraModes();
	}

	void ImageViewer::OnExit(DynExp::ModuleInstance* Instance) const
	{
		auto ModuleData = DynExp::dynamic_ModuleData_cast<ImageViewer>(Instance->ModuleDataGetter());

		Instance->UnlockObject(ModuleData->Camera);
		Instance->UnlockObject(ModuleData->Focus);
		Instance->UnlockObject(ModuleData->Communicator);

		PauseImageCapturingEvent::Deregister(*this);
		ResumeImageCapturingEvent::Deregister(*this);
		AutofocusEvent::Deregister(*this);
	}

	void ImageViewer::OnCameraModeChanged(DynExp::ModuleInstance* Instance, int Index) const
	{
		if (Index < 0)
			return;

		auto ModuleData = DynExp::dynamic_ModuleData_cast<ImageViewer>(Instance->ModuleDataGetter());

		*PauseUpdatingUI = true;
		ModuleData->Camera->SetCameraMode(Util::NumToT<size_t>(Index),
			[Pause = PauseUpdatingUI](const DynExp::TaskBase&, DynExp::ExceptionContainer&) {
				*Pause = false;
			}
		);
	}

	void ImageViewer::OnExposureTimeChanged(DynExp::ModuleInstance* Instance, int Value) const
	{
		try
		{
			auto ModuleData = DynExp::dynamic_ModuleData_cast<ImageViewer>(Instance->ModuleDataGetter());

			if (ModuleData->Camera->CanSetExposureTime())
			{
				*PauseUpdatingUI = true;
				ModuleData->Camera->SetExposureTime(std::chrono::milliseconds(Value),
					[Pause = PauseUpdatingUI](const DynExp::TaskBase&, DynExp::ExceptionContainer&) {
						*Pause = false;
					}
				);
			}
		}
		catch ([[maybe_unused]] const Util::TimeoutException& e)
		{
			// Swallow since it is likely that this exception occurs here when the user
			// very quickly scrolls through the range of exposure times. It may take
			// time to transfer the new exposure time to the camera.
		}
	}

	void ImageViewer::OnCaptureSingle(DynExp::ModuleInstance* Instance, bool) const
	{
		auto ModuleData = DynExp::dynamic_ModuleData_cast<ImageViewer>(Instance->ModuleDataGetter());
		ModuleData->ImageCapturingPaused = false;
		ModuleData->CaptureAfterPause = false;

		ModuleData->Camera->CaptureSingle();
	}

	void ImageViewer::OnCaptureContinuously(DynExp::ModuleInstance* Instance, bool Checked) const
	{
		auto ModuleData = DynExp::dynamic_ModuleData_cast<ImageViewer>(Instance->ModuleDataGetter());
		ModuleData->ImageCapturingPaused = false;
		ModuleData->CaptureAfterPause = false;

		if (Checked)
			ModuleData->Camera->StartCapturing();
		else
			ModuleData->Camera->StopCapturing();
	}

	void ImageViewer::OnPauseImageCapturing(DynExp::ModuleInstance* Instance, bool ResetImageTransformation) const
	{
		auto ModuleData = DynExp::dynamic_ModuleData_cast<ImageViewer>(Instance->ModuleDataGetter());

		{
			auto CameraData = DynExp::dynamic_InstrumentData_cast<DynExpInstr::Camera>(ModuleData->Camera->GetInstrumentData());

			ModuleData->ImageCapturingPaused = true;
			ModuleData->CaptureAfterPause = CameraData->IsCapturingContinuously();

			if (ResetImageTransformation)
				CameraData->SetImageTransformation({});
		} // CameraData unlocked here.

		if (ModuleData->Communicator.valid())
			ModuleData->Communicator->PostEvent(*this, ImageCapturingPausedEvent{});
	}

	void ImageViewer::OnResumeImageCapturing(DynExp::ModuleInstance* Instance) const
	{
		auto ModuleData = DynExp::dynamic_ModuleData_cast<ImageViewer>(Instance->ModuleDataGetter());

		if (!ModuleData->ImageCapturingPaused)
			return;
		ModuleData->ImageCapturingPaused = false;

		if (ModuleData->CaptureAfterPause)
			ModuleData->Camera->StartCapturing();

		if (ModuleData->Communicator.valid())
			ModuleData->Communicator->PostEvent(*this, ImageCapturingResumedEvent{});
	}

	void ImageViewer::OnAutofocusClicked(DynExp::ModuleInstance* Instance, bool Checked) const
	{
		auto ModuleData = DynExp::dynamic_ModuleData_cast<ImageViewer>(Instance->ModuleDataGetter());

		if (!ModuleData->Focus.valid())
			return;

		if (Checked)
			OnAutofocus(Instance);
		else
		{
			if (ModuleData->Communicator.valid())
				ModuleData->Communicator->PostEvent(*this, FinishedAutofocusEvent{ false });

			StateMachine.SetCurrentState(StateType::Ready);
		}
	}

	void ImageViewer::OnAutofocus(DynExp::ModuleInstance* Instance, bool ResetImageTransformation) const
	{
		using CHT = DynExpInstr::CameraData::ComputeHistogramType;

		auto ModuleData = DynExp::dynamic_ModuleData_cast<ImageViewer>(Instance->ModuleDataGetter());

		if (!ModuleData->Focus.valid())
		{
			if (ModuleData->Communicator.valid())
				ModuleData->Communicator->PostEvent(*this, FinishedAutofocusEvent{ false });

			return;
		}

		{
			auto CameraData = DynExp::dynamic_InstrumentData_cast<DynExpInstr::Camera>(ModuleData->Camera->GetInstrumentData());

			if (ResetImageTransformation)
				CameraData->SetImageTransformation({});

			ModuleData->CaptureAfterPause = CameraData->IsCapturingContinuously();
		} // CameraData unlocked here.

		ModuleData->Camera->StopCapturingSync();

		{
			auto CameraData = DynExp::dynamic_InstrumentData_cast<DynExpInstr::Camera>(ModuleData->Camera->GetInstrumentData());
			CameraData->SetComputeHistogram(CHT::NoHistogram);
		} // CameraData unlocked here.

		StateMachine.SetCurrentState(StateType::AutofocusInit);
	}

	StateType ImageViewer::ReadyStateFunc(DynExp::ModuleInstance& Instance)
	{
		using CHT = DynExpInstr::CameraData::ComputeHistogramType;

		auto ModuleData = DynExp::dynamic_ModuleData_cast<ImageViewer>(Instance.ModuleDataGetter());
		auto CameraData = DynExp::dynamic_InstrumentData_cast<DynExpInstr::Camera>(ModuleData->Camera->GetInstrumentData());

		ModuleData->CapturingState = CameraData->GetCapturingState();
		ModuleData->MinExposureTime = CameraData->GetMinExposureTime();
		ModuleData->MaxExposureTime = CameraData->GetMaxExposureTime();
		ModuleData->CurrentExposureTime = CameraData->GetExposureTime();
		ModuleData->CurrentFPS = CameraData->GetCurrentFPS();

		CameraData->SetComputeHistogram(ModuleData->ComputeHistogram);

		if (CameraData->IsImageAvailbale() && !ModuleData->ImageCapturingPaused)
		{
			ModuleData->CurrentImage = CameraData->GetImage();
			ModuleData->HasImageChanged = true;

			if (ModuleData->ComputeHistogram == CHT::IntensityHistogram ||
				ModuleData->ComputeHistogram == CHT::IntensityAndRGBHistogram)
				ModuleData->IntensityHistogram = CameraData->GetIntensityHistogram();
			if (ModuleData->ComputeHistogram == CHT::RGBHistogram ||
				ModuleData->ComputeHistogram == CHT::IntensityAndRGBHistogram)
				ModuleData->RGBHistogram = CameraData->GetRGBHistogram();
		}

		return StateType::Ready;
	}

	StateType ImageViewer::AutofocusInitStateFunc(DynExp::ModuleInstance& Instance)
	{
		{
			auto ModuleParams = DynExp::dynamic_Params_cast<ImageViewer>(Instance.ParamsGetter());

			AutofocusParams.NumSteps = ModuleParams->AutofocusNumSteps;
			AutofocusParams.WaitTimeBeforeCapture = std::chrono::milliseconds(ModuleParams->AutofocusFocusChangeTime);
		} // ModuleParams unlocked here.

		AutofocusResults = {};
		AutofocusIsPerformingFineSweep = false;

		AutofocusSamples.clear();
		for (double Voltage = AutofocusParams.MinVoltage; Voltage <= AutofocusParams.MaxVoltage; Voltage += AutofocusParams.GetVoltageIncrement())
			AutofocusSamples.emplace_back(Voltage, std::numeric_limits<double>::quiet_NaN());
		AutofocusCurrentSample = AutofocusSamples.begin();

		return StateType::AutofocusGotoSample;
	}

	StateType ImageViewer::AutofocusGotoSampleStateFunc(DynExp::ModuleInstance& Instance)
	{
		if (AutofocusCurrentSample == AutofocusSamples.end())
			return StateType::AutofocusStep;
		else
		{
			auto ModuleData = DynExp::dynamic_ModuleData_cast<ImageViewer>(Instance.ModuleDataGetter());
			ModuleData->Focus->SetSync(AutofocusCurrentSample->Voltage);

			AutofocusWaitingEndTimePoint = std::chrono::system_clock::now() + AutofocusParams.WaitTimeBeforeCapture;

			return StateType::AutofocusWaitBeforeCapture;
		}
	}

	StateType ImageViewer::AutofocusWaitBeforeCaptureStateFunc(DynExp::ModuleInstance& Instance)
	{
		if (std::chrono::system_clock::now() >= AutofocusWaitingEndTimePoint)
		{
			auto ModuleData = DynExp::dynamic_ModuleData_cast<ImageViewer>(Instance.ModuleDataGetter());
			auto CameraData = DynExp::dynamic_InstrumentData_cast<DynExpInstr::Camera>(ModuleData->Camera->GetInstrumentData());

			CameraData->ClearImage();
			ModuleData->Camera->CaptureSingle();

			return StateType::AutofocusWaitForImage;
		}

		return StateType::AutofocusWaitBeforeCapture;
	}

	StateType ImageViewer::AutofocusWaitForImageStateFunc(DynExp::ModuleInstance& Instance)
	{
		auto ModuleData = DynExp::dynamic_ModuleData_cast<ImageViewer>(Instance.ModuleDataGetter());
		auto CameraData = DynExp::dynamic_InstrumentData_cast<DynExpInstr::Camera>(ModuleData->Camera->GetInstrumentData());

		if (CameraData->IsImageAvailbale())
		{
			ModuleData->CurrentExposureTime = CameraData->GetExposureTime();
			ModuleData->CurrentImage = CameraData->GetImage();
			ModuleData->HasImageChanged = true;

			// Employing Brenner gradient as the figure of merit to maximize for autofocusing
			// in the fine mode and the image's variance in the coarse mode.
			// (S. Yazdanfar et al. Opt. Expres 16 (12), 8670-8677 (2008)).
			AutofocusCurrentSample->Result = ModuleData->CalcBrennerGradientFromImage();
			AutofocusCurrentSample++;

			return StateType::AutofocusGotoSample;
		}

		return StateType::AutofocusWaitForImage;
	}

	StateType ImageViewer::AutofocusStepStateFunc(DynExp::ModuleInstance& Instance)
	{
		double OptimalVoltage = std::max_element(AutofocusSamples.cbegin(), AutofocusSamples.cend(), [](const auto& a, const auto& b) {
			return a.Result < b.Result;
		})->Voltage;

		if (!AutofocusIsPerformingFineSweep)
		{
			AutofocusIsPerformingFineSweep = true;
			
			AutofocusSamples.clear();
			for (double Voltage = std::max(AutofocusParams.MinVoltage, OptimalVoltage - AutofocusParams.GetVoltageIncrement());
				Voltage <= std::min(AutofocusParams.MaxVoltage, OptimalVoltage + AutofocusParams.GetVoltageIncrement());
				Voltage += AutofocusParams.GetVoltageIncrement(true))
				AutofocusSamples.emplace_back(Voltage, std::numeric_limits<double>::quiet_NaN());
			AutofocusCurrentSample = AutofocusSamples.begin();

			return StateType::AutofocusGotoSample;
		}
		else
		{
			AutofocusResults.Success = OptimalVoltage > AutofocusSamples.front().Voltage + AutofocusParams.GetVoltageIncrement(true) / 2.0 &&
				OptimalVoltage < AutofocusSamples.back().Voltage - AutofocusParams.GetVoltageIncrement(true) / 2.0;
			if (AutofocusResults.Success)
				AutofocusResults.FocusVoltage = OptimalVoltage;
		}

		return StateType::AutofocusFinished;
	}

	StateType ImageViewer::AutofocusFinishedStateFunc(DynExp::ModuleInstance& Instance)
	{
		auto ModuleData = DynExp::dynamic_ModuleData_cast<ImageViewer>(Instance.ModuleDataGetter());

		if (AutofocusResults.Success)
			ModuleData->Focus->SetSync(AutofocusResults.FocusVoltage);

		FinishAutofocus(ModuleData, FinishedAutofocusEvent{ AutofocusResults.Success, AutofocusResults.FocusVoltage });

		return StateType::Ready;
	}
}