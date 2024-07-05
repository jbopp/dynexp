// This file is part of DynExp.

/**
 * @file SpectrumViewer.h
 * @brief Implementation of a module to plot the spectrum stored in a spectrometer instrument.
*/

#pragma once

#include "stdafx.h"
#include "DynExpCore.h"
#include "../../MetaInstruments/Spectrometer.h"
#include "../../Instruments/InterModuleCommunicator.h"

#include "SpectrumViewerEvents.h"

#include <QWidget>
#include "ui_SpectrumViewer.h"

namespace DynExpModule::SpectrumViewer
{
	class SpectrumViewer;
	class SpectrumViewerData;

	class SpectrumViewerWidget : public DynExp::QModuleWidget
	{
		Q_OBJECT

	public:
		struct SampleDataType
		{
			SampleDataType() { Reset(); }

			void Reset();
			std::string ToStr(DynExpInstr::SpectrometerData::TimeType ExposureTime) const;

			QList<QPointF> Points;
			QPointF MinValues;
			QPointF MaxValues;

			DynExpInstr::SpectrometerData::FrequencyUnitType FrequencyUnit;
			DynExpInstr::SpectrometerData::IntensityUnitType IntensityUnit;
		};

		SpectrumViewerWidget(SpectrumViewer& Owner, QModuleWidget* parent = nullptr);
		~SpectrumViewerWidget() = default;

		bool AllowResize() const noexcept override final { return true; }

		const auto& GetUI() const noexcept { return ui; }

		void InitializeUI(Util::SynchronizedPointer<SpectrumViewerData>& ModuleData);
		void UpdateUI(Util::SynchronizedPointer<SpectrumViewerData>& ModuleData);
		void SetData(SampleDataType&& SampleData, DynExpInstr::SpectrometerData::TimeType ExposureTime);

	private:
		void FinishedSavingData() noexcept { IsSavingData = false; }
		using FinishedSavingDataGuardType = Util::OnDestruction<SpectrumViewerWidget, decltype(&SpectrumViewerWidget::FinishedSavingData)>;

		QXYSeries* DataSeries;
		QChart* DataChart;
		QValueAxis* XAxis;
		QValueAxis* YAxis;

		Ui::SpectrumViewer ui;

		SampleDataType CurrentSpectrum;
		DynExpInstr::SpectrometerData::TimeType CurrentExposureTime{};

		// If data is currently being saved to file, do not update internal data.
		std::atomic_bool IsSavingData = false;

	private slots:
		void OnSaveCSVClicked();
	};

	class SpectrumViewerData : public DynExp::QModuleDataBase
	{
	public:
		SpectrumViewerData() { Init(); }
		virtual ~SpectrumViewerData() = default;

		bool IsUIInitialized() const noexcept { return UIInitialized; }
		void SetUIInitialized() noexcept { UIInitialized = true; }
		auto& GetSpectrometer() { return Spectrometer; }
		auto& GetCommunicator() { return Communicator; }

		DynExpInstr::SpectrometerData::FrequencyUnitType FrequencyUnit;
		DynExpInstr::SpectrometerData::IntensityUnitType IntensityUnit;
		double MinFrequency;
		double MaxFrequency;
		DynExpInstr::SpectrometerData::TimeType MinExposureTime;
		DynExpInstr::SpectrometerData::TimeType MaxExposureTime;
		DynExpInstr::SpectrometerData::TimeType CurrentExposureTime;
		DynExpInstr::SpectrometerData::TimeType AcquisitionExposureTime;
		double CurrentLowerFrequency;
		double CurrentUpperFrequency;
		bool SilentModeEnabled;
		DynExpInstr::SpectrometerData::CapturingStateType CapturingState;
		double CapturingProgress;
		std::string AutoSaveFilename;

		SpectrumViewerWidget::SampleDataType CurrentSpectrum;
		bool SpectrumRecordingPaused;

	private:
		void ResetImpl(dispatch_tag<QModuleDataBase>) override final;
		virtual void ResetImpl(dispatch_tag<SpectrumViewerData>) {};

		void Init();

		bool UIInitialized;

		DynExp::LinkedObjectWrapperContainer<DynExpInstr::Spectrometer> Spectrometer;
		DynExp::LinkedObjectWrapperContainer<DynExpInstr::InterModuleCommunicator> Communicator;
	};

	class SpectrumViewerParams : public DynExp::QModuleParamsBase
	{
	public:
		SpectrumViewerParams(DynExp::ItemIDType ID, const DynExp::DynExpCore& Core) : QModuleParamsBase(ID, Core) {}
		virtual ~SpectrumViewerParams() = default;

		virtual const char* GetParamClassTag() const noexcept override { return "SpectrumViewerParams"; }

		Param<DynExp::ObjectLink<DynExpInstr::Spectrometer>> Spectrometer = { *this, GetCore().GetInstrumentManager(),
			"Spectrometer", "Spectrometer", "Underlying spectrometer instrument to be used as a data source", DynExpUI::Icons::Instrument };
		Param<DynExp::ObjectLink<DynExpInstr::InterModuleCommunicator>> Communicator = { *this, GetCore().GetInstrumentManager(),
			"InterModuleCommunicator", "Inter-module communicator", "Inter-module communicator to control this module with", DynExpUI::Icons::Instrument, true };

	private:
		void ConfigureParamsImpl(dispatch_tag<QModuleParamsBase>) override final {}
	};

	class SpectrumViewerConfigurator : public DynExp::QModuleConfiguratorBase
	{
	public:
		using ObjectType = SpectrumViewer;
		using ParamsType = SpectrumViewerParams;

		SpectrumViewerConfigurator() = default;
		virtual ~SpectrumViewerConfigurator() = default;

	private:
		virtual DynExp::ParamsBasePtrType MakeParams(DynExp::ItemIDType ID, const DynExp::DynExpCore& Core) const override final { return DynExp::MakeParams<SpectrumViewerConfigurator>(ID, Core); }
	};

	class SpectrumViewer : public DynExp::QModuleBase
	{
	public:
		using ParamsType = SpectrumViewerParams;
		using ConfigType = SpectrumViewerConfigurator;
		using ModuleDataType = SpectrumViewerData;

		constexpr static auto Name() noexcept { return "Spectrum Viewer"; }
		constexpr static auto Category() noexcept { return "Image Capturing"; }

		SpectrumViewer(const std::thread::id OwnerThreadID, DynExp::ParamsBasePtrType&& Params)
			: QModuleBase(OwnerThreadID, std::move(Params)) {}
		virtual ~SpectrumViewer() = default;

		virtual std::string GetName() const override { return Name(); }
		virtual std::string GetCategory() const override { return Category(); }

		std::chrono::milliseconds GetMainLoopDelay() const override final { return std::chrono::milliseconds(50); }

	private:
		Util::DynExpErrorCodes::DynExpErrorCodes ModuleMainLoop(DynExp::ModuleInstance& Instance) override final;

		void ResetImpl(dispatch_tag<QModuleBase>) override final;

		std::unique_ptr<DynExp::QModuleWidget> MakeUIWidget() override final;
		void UpdateUIChild(const ModuleBase::ModuleDataGetterType& ModuleDataGetter) override final;

		SpectrumViewerWidget::SampleDataType ProcessSpectrum(DynExpInstr::SpectrometerData::SpectrumType&& Spectrum,
			Util::SynchronizedPointer<SpectrumViewerData>& ModuleData);
		void SaveSpectrum(const SpectrumViewerWidget::SampleDataType& Spectrum,
			Util::SynchronizedPointer<SpectrumViewerData>& ModuleData);

		// Events, run in module thread
		void OnInit(DynExp::ModuleInstance* Instance) const override final;
		void OnExit(DynExp::ModuleInstance* Instance) const override final;

		void OnRunClicked(DynExp::ModuleInstance* Instance, bool) const;
		void OnStopClicked(DynExp::ModuleInstance* Instance, bool) const;
		void OnSilentModeToggled(DynExp::ModuleInstance* Instance, bool Checked) const;
		void OnExposureTimeChanged(DynExp::ModuleInstance* Instance, int Value) const;
		void OnLowerLimitChanged(DynExp::ModuleInstance* Instance, double Value) const;
		void OnUpperLimitChanged(DynExp::ModuleInstance* Instance, double Value) const;
		void OnRecordAndSaveSpectrum(DynExp::ModuleInstance* Instance, std::string SaveDataFilename) const;
		void OnPauseSpectrumRecording(DynExp::ModuleInstance* Instance) const;
		void OnResumeSpectrumRecording(DynExp::ModuleInstance* Instance) const;

		size_t NumFailedUpdateAttempts = 0;
	};
}