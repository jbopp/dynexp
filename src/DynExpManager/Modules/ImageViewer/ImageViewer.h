// This file is part of DynExp.

/**
 * @file ImageViewer.h
 * @brief Implementation of a module to display images recorded by camera instruments.
*/

#pragma once

#include "stdafx.h"
#include "DynExpCore.h"
#include "../../MetaInstruments/Camera.h"
#include "../../MetaInstruments/AnalogOut.h"
#include "../../Instruments/InterModuleCommunicator.h"

#include "ImageViewerEvents.h"

#include <QWidget>
#include "ui_ImageViewer.h"

namespace DynExpModule::ImageViewer
{
	class ImageViewer;

	enum class StateType {
		Ready,
		AutofocusInit,
		AutofocusGotoSample,
		AutofocusWaitBeforeCapture,
		AutofocusWaitForImage,
		AutofocusStep,
		AutofocusFinished
	};

	using StateMachineStateType = Util::StateMachineState<StateType(ImageViewer::*)(DynExp::ModuleInstance&)>;

	class ImageViewerWidget : public DynExp::QModuleWidget
	{
		Q_OBJECT

	public:
		ImageViewerWidget(ImageViewer& Owner, QModuleWidget* parent = nullptr);
		~ImageViewerWidget() = default;

		bool AllowResize() const noexcept override final { return true; }

		void SetImage(const QImage& NewImage) noexcept;
		void SetImageViewEnabled(bool Enable);
		void SetIntensityHistogram(Util::ImageHistogramType&& NewIntensityHistogram) noexcept;
		void SetRGBHistogram(Util::ImageRGBHistogramType&& NewRGBHistogram) noexcept;
		auto GetComputeHistogram() const noexcept;
		void UpdateScene();
		auto GetSaveImageFilename() const { return SaveImageFilename; }
		void ResetSaveImageFilename() { SaveImageFilename.clear(); }

		Ui::ImageViewer ui;

	private:
		bool eventFilter(QObject* obj, QEvent* event) override;
		virtual void resizeEvent(QResizeEvent* event) override;

		void UpdateHistogram();

		QMenu* HistogramContextMenu;
		QActionGroup* HistogramLinLogActionGroup;
		QAction* HistogramLinAction;
		QAction* HistogramLogAction;
		QAction* HistogramBWAction;
		QAction* HistogramColorAction;

		QBarSet* HistogramBarSetI;
		QBarSet* HistogramBarSetR;
		QBarSet* HistogramBarSetG;
		QBarSet* HistogramBarSetB;
		QBarSeries* HistogramBarSeries;
		QChart* HistogramChart;
		QValueAxis* HistogramXAxis;
		QValueAxis* HistogramYAxis;

		Util::MarkerGraphicsView* GraphicsView;
		QPixmap Pixmap;
		QGraphicsPixmapItem* GraphicsPixmapItem;
		QGraphicsScene* GraphicsScene;

		Util::ImageHistogramType IntensityHistogram = {};
		Util::ImageRGBHistogramType RGBHistogram = {};

		QString SaveImageFilename;

	private slots:
		void OnHistogramContextMenuRequested(const QPoint& Position);
		void OnSaveImageClicked();
		void OnZoomResetClicked();
		void OnZoomInClicked();
		void OnZoomOutClicked();
		void OnZoomFitClicked(bool Checked);
		void OnImageMouseMove(QMouseEvent* Event);
	};

	class ImageViewerData : public DynExp::QModuleDataBase
	{
	public:
		using TimeType = DynExpInstr::CameraData::TimeType;

		ImageViewerData() { Init(); }
		virtual ~ImageViewerData() = default;

		double CalcBrennerGradientFromImage() const;

		DynExp::LinkedObjectWrapperContainer<DynExpInstr::Camera> Camera;
		DynExp::LinkedObjectWrapperContainer<DynExpInstr::AnalogOut> Focus;
		DynExp::LinkedObjectWrapperContainer<DynExpInstr::InterModuleCommunicator> Communicator;

		bool UIInitialized = false;

		DynExpInstr::CameraData::CameraModesType CameraModes;
		DynExpInstr::CameraData::CapturingStateType CapturingState = DynExpInstr::CameraData::CapturingStateType::Stopped;
		TimeType MinExposureTime;
		TimeType MaxExposureTime;
		TimeType CurrentExposureTime;
		float CurrentFPS = 0.f;
		DynExpInstr::CameraData::ComputeHistogramType ComputeHistogram = DynExpInstr::CameraData::ComputeHistogramType::NoHistogram;

		QImage CurrentImage;
		bool HasImageChanged = false;
		bool ImageCapturingPaused = false;
		bool CaptureAfterPause = false;

		Util::ImageHistogramType IntensityHistogram = {};
		Util::ImageRGBHistogramType RGBHistogram = {};

	private:
		void ResetImpl(dispatch_tag<QModuleDataBase>) override final;
		virtual void ResetImpl(dispatch_tag<ImageViewerData>) {};

		void Init();
	};

	class ImageViewerParams : public DynExp::QModuleParamsBase
	{
	public:
		ImageViewerParams(DynExp::ItemIDType ID, const DynExp::DynExpCore& Core) : QModuleParamsBase(ID, Core) {}
		virtual ~ImageViewerParams() = default;

		virtual const char* GetParamClassTag() const noexcept override { return "ImageViewerParams"; }

		Param<DynExp::ObjectLink<DynExpInstr::Camera>> Camera = { *this, GetCore().GetInstrumentManager(),
			"Camera", "Camera", "Underlying camera to be used as an image source", DynExpUI::Icons::Instrument };
		Param<DynExp::ObjectLink<DynExpInstr::AnalogOut>> Focus = { *this, GetCore().GetInstrumentManager(),
			"Focus", "Focusing voltage (AO)", "Analog output (voltage) to adjust the focus", DynExpUI::Icons::Instrument, true };
		Param<ParamsConfigDialog::NumberType> AutofocusNumSteps = { *this, "AutofocusNumSteps", "Autofocus step count",
			"Specifies how fine the focus is scanned during the autofocus algorithm. Higher values lead to higher precision and longer execution times.",
			false, 10, 3, 10000, 10, 0};
		Param<ParamsConfigDialog::NumberType> AutofocusFocusChangeTime = { *this, "AutofocusFocusChangeTime",
			"Focus change time (ms)", "Time it takes to change the focus after applying a new focus voltage.",
			false, 500, 0, 10000, 10, 0 };
		Param<DynExp::ObjectLink<DynExpInstr::InterModuleCommunicator>> Communicator = { *this, GetCore().GetInstrumentManager(),
			"InterModuleCommunicator", "Inter-module communicator", "Inter-module communicator to control this module with", DynExpUI::Icons::Instrument, true };

	private:
		void ConfigureParamsImpl(dispatch_tag<QModuleParamsBase>) override final {}
	};

	class ImageViewerConfigurator : public DynExp::QModuleConfiguratorBase
	{
	public:
		using ObjectType = ImageViewer;
		using ParamsType = ImageViewerParams;

		ImageViewerConfigurator() = default;
		virtual ~ImageViewerConfigurator() = default;

	private:
		virtual DynExp::ParamsBasePtrType MakeParams(DynExp::ItemIDType ID, const DynExp::DynExpCore& Core) const override final { return DynExp::MakeParams<ImageViewerConfigurator>(ID, Core); }
	};

	class ImageViewer : public DynExp::QModuleBase
	{
	public:
		using ParamsType = ImageViewerParams;
		using ConfigType = ImageViewerConfigurator;
		using ModuleDataType = ImageViewerData;

		constexpr static auto Name() noexcept { return "Image Viewer"; }
		constexpr static auto Category() noexcept { return "Image Capturing"; }

		ImageViewer(const std::thread::id OwnerThreadID, DynExp::ParamsBasePtrType&& Params);
		virtual ~ImageViewer();

		virtual std::string GetName() const override { return Name(); }
		virtual std::string GetCategory() const override { return Category(); }

		// approx. 63 fps
		std::chrono::milliseconds GetMainLoopDelay() const override final { return std::chrono::milliseconds(16); }

		// Events which the UI thread might enqueue.
		void OnSaveImage(DynExp::ModuleInstance* Instance, QString Filename) const;

	private:
		Util::DynExpErrorCodes::DynExpErrorCodes ModuleMainLoop(DynExp::ModuleInstance& Instance) override final;

		void ResetImpl(dispatch_tag<QModuleBase>) override final;

		std::unique_ptr<DynExp::QModuleWidget> MakeUIWidget() override final;
		void UpdateUIChild(const ModuleBase::ModuleDataGetterType& ModuleDataGetter) override final;

		// Helper functions
		bool IsReadyState() const;
		bool IsAutofocusingState() const;
		void FinishAutofocus(Util::SynchronizedPointer<ModuleDataType>& ModuleData, const FinishedAutofocusEvent& Event) const;

		// Function and types for autofocusing
		// ->
		struct AutofocusParamsType
		{
			DynExpInstr::AnalogOutData::SampleStreamType::SampleType MinVoltage{};
			DynExpInstr::AnalogOutData::SampleStreamType::SampleType MaxVoltage{};
			ParamsConfigDialog::NumberType NumSteps{};
			std::chrono::milliseconds WaitTimeBeforeCapture{};

			constexpr auto GetVoltageDiff() const noexcept { return MaxVoltage - MinVoltage; }
			constexpr auto GetVoltageIncrement(bool Fine = false) const noexcept { return GetVoltageDiff() / NumSteps / (Fine ? NumSteps : 1); }
		};

		struct AutofocusResultsType
		{
			bool Success = false;
			double FocusVoltage = std::numeric_limits<double>::quiet_NaN();
		};

		struct AutofocusSampleType
		{
			double Voltage{};
			double Result{};
		};
		// <-

		// Events, run in module thread
		void OnInit(DynExp::ModuleInstance* Instance) const override final;
		void OnExit(DynExp::ModuleInstance* Instance) const override final;
		void OnCameraModeChanged(DynExp::ModuleInstance* Instance, int Index) const;
		void OnExposureTimeChanged(DynExp::ModuleInstance* Instance, int Value) const;
		void OnCaptureSingle(DynExp::ModuleInstance* Instance, bool) const;
		void OnCaptureContinuously(DynExp::ModuleInstance* Instance, bool Checked) const;
		void OnPauseImageCapturing(DynExp::ModuleInstance* Instance, bool ResetImageTransformation = false) const;
		void OnResumeImageCapturing(DynExp::ModuleInstance* Instance) const;
		void OnAutofocusClicked(DynExp::ModuleInstance* Instance, bool Checked) const;
		void OnAutofocus(DynExp::ModuleInstance* Instance, bool ResetImageTransformation = false) const;

		// State functions for state machine
		StateType ReadyStateFunc(DynExp::ModuleInstance& Instance);
		StateType AutofocusInitStateFunc(DynExp::ModuleInstance& Instance);
		StateType AutofocusGotoSampleStateFunc(DynExp::ModuleInstance& Instance);
		StateType AutofocusWaitBeforeCaptureStateFunc(DynExp::ModuleInstance& Instance);
		StateType AutofocusWaitForImageStateFunc(DynExp::ModuleInstance& Instance);
		StateType AutofocusStepStateFunc(DynExp::ModuleInstance& Instance);
		StateType AutofocusFinishedStateFunc(DynExp::ModuleInstance& Instance);

		// States for state machine
		static constexpr auto ReadyState = Util::StateMachineState(StateType::Ready,
			&ImageViewer::ReadyStateFunc, "Ready");
		static constexpr auto AutofocusInitState = Util::StateMachineState(StateType::AutofocusInit,
			&ImageViewer::AutofocusInitStateFunc, "Autofocusing...");
		static constexpr auto AutofocusGotoSampleState = Util::StateMachineState(StateType::AutofocusGotoSample,
			&ImageViewer::AutofocusGotoSampleStateFunc, "Autofocusing...");
		static constexpr auto AutofocusWaitBeforeCaptureState = Util::StateMachineState(StateType::AutofocusWaitBeforeCapture,
			&ImageViewer::AutofocusWaitBeforeCaptureStateFunc, "Autofocusing...");
		static constexpr auto AutofocusWaitForImageState = Util::StateMachineState(StateType::AutofocusWaitForImage,
			&ImageViewer::AutofocusWaitForImageStateFunc, "Autofocusing...");
		static constexpr auto AutofocusStepState = Util::StateMachineState(StateType::AutofocusStep,
			&ImageViewer::AutofocusStepStateFunc, "Autofocusing...");
		static constexpr auto AutofocusFinishedState = Util::StateMachineState(StateType::AutofocusFinished,
			&ImageViewer::AutofocusFinishedStateFunc, "Autofocusing...");

		// Logical const-ness: allow events to set the state machine's current state.
		mutable Util::StateMachine<StateMachineStateType> StateMachine;

		// Variables for autofocusing
		mutable AutofocusParamsType AutofocusParams;
		AutofocusResultsType AutofocusResults;
		bool AutofocusIsPerformingFineSweep = false;
		std::vector<AutofocusSampleType> AutofocusSamples;
		std::vector<AutofocusSampleType>::iterator AutofocusCurrentSample;
		std::chrono::system_clock::time_point AutofocusWaitingEndTimePoint;

		const std::shared_ptr<std::atomic<bool>> PauseUpdatingUI;
		size_t NumFailedUpdateAttempts = 0;
	};
}