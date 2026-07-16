// This file is part of DynExp.

/**
 * @file SpectrumViewer.h
 * @brief Implementation of a module to plot the spectrum stored in a spectrometer instrument.
*/

#pragma once

#include "stdafx.h"
#include "DynExpCore.h"
#include "../GraphUtil.h"
#include "../../MetaInstruments/Spectrometer.h"
#include "../../Instruments/InterModuleCommunicator.h"

#include "CommonModuleEvents.h"
#include "SpectrumViewerEvents.h"

namespace DynExpModule::SpectrumViewer
{
	class SpectrumViewer;

	class SpectrumViewerData : public DynExp::QMLModuleDataBase
	{
	public:
		struct SampleDataType
		{
			std::string ToStr(DynExpInstr::SpectrometerData::TimeType ExposureTime, const DynExpModule::Graph::LineGraphPlotInfo& PlotInfo) const;

			QList<QPointF> Samples;
		};

		SpectrumViewerData() { Init(); }
		virtual ~SpectrumViewerData() = default;

		bool IsUIInitialized() const noexcept { return UIInitialized; }
		void SetUIInitialized() noexcept { UIInitialized = true; }

		auto& GetSpectrometer() { return Spectrometer; }
		auto& GetCommunicator() { return Communicator; }

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
		bool SpectrumRecordingPaused;

		Graph::LineGraphPlotInfo PlotInfo;
		SampleDataType CurrentSpectrum;

	private:
		void ResetImpl(dispatch_tag<QMLModuleDataBase>) override final;
		virtual void ResetImpl(dispatch_tag<SpectrumViewerData>) {};

		void Init();

		bool UIInitialized;

		DynExp::LinkedObjectWrapperContainer<DynExpInstr::Spectrometer> Spectrometer;
		DynExp::LinkedObjectWrapperContainer<DynExpInstr::InterModuleCommunicator> Communicator;
	};

	class SpectrumViewerParams : public DynExp::QMLModuleParamsBase
	{
	public:
		SpectrumViewerParams(DynExp::ItemIDType ID, const DynExp::DynExpCore& Core) : QMLModuleParamsBase(ID, Core) {}
		virtual ~SpectrumViewerParams() = default;

		virtual const char* GetParamClassTag() const noexcept override { return "SpectrumViewerParams"; }

		Param<DynExp::ObjectLink<DynExpInstr::Spectrometer>> Spectrometer = { *this, GetCore().GetInstrumentManager(),
			"Spectrometer", "Spectrometer", "Underlying spectrometer instrument to be used as a data source", DynExpUI::Icons::Instrument };
		Param<DynExp::ObjectLink<DynExpInstr::InterModuleCommunicator>> Communicator = { *this, GetCore().GetInstrumentManager(),
			"InterModuleCommunicator", "Inter-module communicator", "Inter-module communicator to control this module with", DynExpUI::Icons::Instrument, true };

	private:
		void ConfigureParamsImpl(dispatch_tag<QMLModuleParamsBase>) override final {}
	};

	class SpectrumViewerConfigurator : public DynExp::QMLModuleConfiguratorBase
	{
	public:
		using ObjectType = SpectrumViewer;
		using ParamsType = SpectrumViewerParams;

		SpectrumViewerConfigurator() = default;
		virtual ~SpectrumViewerConfigurator() = default;

	private:
		virtual DynExp::ParamsBasePtrType MakeParams(DynExp::ItemIDType ID, const DynExp::DynExpCore& Core) const override final { return DynExp::MakeParams<SpectrumViewerConfigurator>(ID, Core); }
	};

	class SpectrumViewer : public DynExp::QMLModuleBase
	{
	public:
		using ParamsType = SpectrumViewerParams;
		using ConfigType = SpectrumViewerConfigurator;
		using ModuleDataType = SpectrumViewerData;

		constexpr static auto Name() noexcept { return "Spectrum Viewer"; }
		constexpr static auto Category() noexcept { return "Image Capturing"; }

		SpectrumViewer(const std::thread::id OwnerThreadID, DynExp::ParamsBasePtrType&& Params)
			: QMLModuleBase(OwnerThreadID, std::move(Params)) {}
		virtual ~SpectrumViewer() = default;

		virtual std::string GetName() const override { return Name(); }
		virtual std::string GetCategory() const override { return Category(); }

		std::chrono::milliseconds GetMainLoopDelay() const override final { return std::chrono::milliseconds(50); }

	private:
		Util::DynExpErrorCodes::DynExpErrorCodes ModuleMainLoop(DynExp::ModuleInstance& Instance) override final;

		void ResetImpl(dispatch_tag<QMLModuleBase>) override final;

		void MakeConnections(QObject* Backend) override final;
		QAnyStringView GetModuleSourceUri() const noexcept override final { return "Modules.SpectrumViewer"; }
		QAnyStringView GetModuleSourceTypeName() const noexcept override final { return "SpectrumViewer"; }
		void UpdateUIChild(const ModuleBase::ModuleDataGetterType& ModuleDataGetter) override final;

		// Events, run in module thread
		void OnInit(DynExp::ModuleInstance* Instance) const override final;
		void OnExit(DynExp::ModuleInstance* Instance) const override final;

		void OnRunClicked(DynExp::ModuleInstance* Instance) const;
		void OnStopClicked(DynExp::ModuleInstance* Instance) const;
		void OnSilentModeToggled(DynExp::ModuleInstance* Instance, bool Checked) const;
		void OnExposureTimeChanged(DynExp::ModuleInstance* Instance, int Value) const;
		void OnLowerLimitChanged(DynExp::ModuleInstance* Instance, double Value) const;
		void OnUpperLimitChanged(DynExp::ModuleInstance* Instance, double Value) const;
		void OnSetFilename(DynExp::ModuleInstance* Instance, const std::string& SaveFilename) const;
		void OnTrigger(DynExp::ModuleInstance* Instance) const;
		void OnStop(DynExp::ModuleInstance* Instance) const;
		void OnPauseSpectrumRecording(DynExp::ModuleInstance* Instance) const;
		void OnResumeSpectrumRecording(DynExp::ModuleInstance* Instance) const;

		// Events, run in UI thread
		void OnSaveSpectrum() const;

		void FinishedSavingData() const noexcept { IsSavingData = false; }
		using FinishedSavingDataGuardType = Util::OnDestruction<const SpectrumViewer, decltype(&SpectrumViewer::FinishedSavingData)>;

		/**
		 * @brief If data is currently being saved to file, do not update internal data.
		*/
		mutable std::atomic_bool IsSavingData = false;

		size_t NumFailedUpdateAttempts = 0;

		/**
		 * @brief Does nothing but ensures that this module's Qt connections are disconnected when
		 * @p SignalContext is destroyed. Moreover, @p SignalContext is created in the UI thread
		 * (that creates this module instance). Hence, it ensures that Qt connections using this
		 * context are executed in the UI thread.
		*/
		QObject SignalContext;
	};
}