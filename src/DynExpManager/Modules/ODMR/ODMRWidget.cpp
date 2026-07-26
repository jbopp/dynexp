// This file is part of DynExp.

#include "stdafx.h"
#include "ODMR.h"
#include "moc_ODMRWidget.cpp"
#include "ui_ODMR.h"
#include "ODMRWidget.h"

namespace DynExpModule::ODMR
{
	ODMRWidget::StatusBarType::StatusBarType(ODMRWidget* Owner)
		: CurrentState(nullptr),
		StateLabel(new QLabel(Owner)), SweepStateLabel(new QLabel(Owner)), AcquisitionTimeLabel(new QLabel(Owner))
	{
	}

	void ODMRWidget::StatusBarType::Update()
	{
		StateLabel->setText(CurrentState ? (QString(" ") + CurrentState->GetDescription()) : "< Unknown >");
		if (CurrentState && CurrentState->GetState() != StateType::Ready)
			StateLabel->setStyleSheet(DynExpUI::StatusBarBusyStyleSheet);
		else
			StateLabel->setStyleSheet("");
	}

	ODMRWidget::ODMRWidget(ODMR& Owner, QModuleWidget* parent)
		: QModuleWidget(Owner, parent),
		ui(std::make_unique<Ui::ODMR>()), StatusBar(this),
		ODMRGraph(nullptr), SensitivityGraph(nullptr)
	{
		ui->setupUi(this);

		// Status bar
		ui->MainStatusBar->addWidget(StatusBar.StateLabel, 5);
		ui->MainStatusBar->addWidget(StatusBar.SweepStateLabel, 2);
		ui->MainStatusBar->addWidget(StatusBar.AcquisitionTimeLabel, 3);

		// Graph to display a single ODMR trace
		connect(ui->ODMRGraph, &QQuickWidget::statusChanged, [this](QQuickWidget::Status Status) {
			if (Status == QQuickWidget::Status::Ready && ui->ODMRGraph->rootObject())
			{
				auto BackendVariant = ui->ODMRGraph->rootObject()->property("backend");
				ODMRGraph = BackendVariant.isValid() ? BackendVariant.value<DynExpQuick::DynExpLineGraphBackend*>() : nullptr;

				if (ODMRGraph)
				{
					ODMRGraph->InsertSeries("meas");
					ODMRGraph->InsertSeries("fit");
				}
			}
		});
		ui->ODMRGraph->loadFromModule("Modules.DynExpQuick", "QDynExpLineGraph");

		// Graph to display a single sensitivity measurement
		connect(ui->SensitivityGraph, &QQuickWidget::statusChanged, [this](QQuickWidget::Status Status) {
			if (Status == QQuickWidget::Status::Ready && ui->SensitivityGraph->rootObject())
			{
				auto BackendVariant = ui->SensitivityGraph->rootObject()->property("backend");
				SensitivityGraph = BackendVariant.isValid() ? BackendVariant.value<DynExpQuick::DynExpLineGraphBackend*>() : nullptr;

				if (SensitivityGraph)
					SensitivityGraph->InsertSeries("noise");
			}
		});
		ui->SensitivityGraph->loadFromModule("Modules.DynExpQuick", "QDynExpLineGraph");

		connect(ui->CBParamSweepType, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &ODMRWidget::OnSweepSeriesParamChanged);
	}

	void ODMRWidget::InitializeUI(Util::SynchronizedPointer<ODMRData>& ModuleData)
	{
		if (!GetUIInitialized())
		{
			ui->SBRFPower->setValue(ModuleData->RFPower);
			ui->SBRFCenter->setValue(ModuleData->RFCenterFreq / 1e6);
			ui->SBRFSpan->setValue(ModuleData->RFFreqSpan / 1e6);
			ui->SBRFFreqSpacing->setValue(ModuleData->RFFreqSpacing / 1e3);
			ui->SBRFDwellTime->setValue(ModuleData->RFDwellTime / 1e-3);
			ui->RBRFModulationTypeNone->setChecked(true);	// Emits signal to update module data accordingly.
			ui->SBRFModulationFreq->setValue(ModuleData->RFModulationFreq / 1e3);
			ui->SBRFModulationDepth->setValue(ModuleData->RFModulationDepth / 1e3);
			ui->SBDataAcquisitionODMRSamplingRate->setValue(ModuleData->ODMRSamplingRate);
			ui->LESaveDataPath->setText(QString::fromStdString(ModuleData->SaveDataPath));
			ui->SBSaveDataCurrentIndex->setValue(ModuleData->CurrentSaveIndex);
			ui->CBSaveDataEnable->setChecked(ModuleData->AutosaveEnabled);
			ui->CBSensitivityEnable->setChecked(ModuleData->SensitivityEnabled);
			ui->CBSensitivityOncePerSweep->setChecked(ModuleData->SensitivityOncePerSweep);
			ui->CBSensitivityOffResEnable->setChecked(ModuleData->SensitivityOffResonanceEnabled);
			ui->SBSensitivityFreq->setValue(ModuleData->SensitivityResonanceFreq / 1e6);
			ui->SBSensitivityOffResFreq->setValue(ModuleData->SensitivityOffResonanceFreq / 1e6);
			ui->SBSensitivitySpan->setValue(ModuleData->SensitivityResonanceSpan / 1e6);
			ui->SBSensitivitySamplingRate->setValue(ModuleData->SensitivitySamplingRate);
			ui->SBSensitivityDuration->setValue(ModuleData->SensitivityDuration);
			ui->GBSensitivityAnalysis->setChecked(ModuleData->SensitivityAnalysisEnabled);
			ui->SBGyromagneticRatio->setValue(ModuleData->GyromagneticRatio / 1e6);
			ui->CBParamSweepEnable->setChecked(ModuleData->SweepSeriesEnabled);
			if (ModuleData->TestFeature(ODMRData::FeatureType::AuxAnalogOut))
				ui->CBParamSweepType->insertItem(ui->CBParamSweepType->count(), "Auxiliary analog out");
			ui->CBParamSweepType->setCurrentIndex(0);		// Emits signal to update module data accordingly, but see below.
			ui->SBParamSweepStart->setValue(ModuleData->SweepSeriesStart);
			ui->SBParamSweepStop->setValue(ModuleData->SweepSeriesStop);
			ui->SBParamSweepStep->setValue(ModuleData->SweepSeriesStep);
			ui->CBParamSweepRetrace->setChecked(ModuleData->SweepSeriesRetrace);
			ui->CBParamSweepAdvanceLastValue->setChecked(ModuleData->SweepSeriesAdvanceLastValue);

			ui->SBRFPower->setMinimum(ModuleData->RFGeneratorMinFuncDesc.Amplitude);
			ui->SBRFPower->setMaximum(ModuleData->RFGeneratorMaxFuncDesc.Amplitude);
			ui->SBRFPower->setValue(ModuleData->RFGeneratorDefaultFuncDesc.Amplitude);
			ui->SBRFPower->setSuffix(QString(" ") + ModuleData->GetRFGenerator()->GetValueUnitStr());
			ui->SBRFCenter->setMinimum(ModuleData->RFGeneratorMinFuncDesc.FrequencyInHz / 1e6);
			ui->SBRFCenter->setMaximum(ModuleData->RFGeneratorMaxFuncDesc.FrequencyInHz / 1e6);
			ui->SBRFCenter->setValue(ModuleData->RFGeneratorDefaultFuncDesc.FrequencyInHz / 1e6);
			ui->SBSensitivityFreq->setMinimum(ModuleData->RFGeneratorMinFuncDesc.FrequencyInHz / 1e6);
			ui->SBSensitivityFreq->setMaximum(ModuleData->RFGeneratorMaxFuncDesc.FrequencyInHz / 1e6);
			ui->SBSensitivityFreq->setValue(ModuleData->RFGeneratorDefaultFuncDesc.FrequencyInHz / 1e6);

			ui->SBDataAcquisitionODMRSamplingRate->setEnabled(ModuleData->TestFeature(ODMRData::FeatureType::LockinDetection));
			ui->SBSensitivitySamplingRate->setEnabled(ModuleData->TestFeature(ODMRData::FeatureType::LockinDetection));
			ui->SBSensitivityDuration->setEnabled(ModuleData->TestFeature(ODMRData::FeatureType::LockinDetection));

			// This is not emitted if setCurrentIndex() does not change the index (because it already is the desired value).
			// So, do it manually.
			OnSweepSeriesParamChanged(ui->CBParamSweepType->currentIndex());

			AuxAnalogOutValueUnit = ModuleData->AuxAnalogOutValueUnit;
			AuxAnalogOutMinValue = ModuleData->AuxAnalogOutMinValue;
			AuxAnalogOutMaxValue = ModuleData->AuxAnalogOutMaxValue;

			UIInitialized = true;
		}
	}

	void ODMRWidget::SetUIState(const StateMachineStateType* State, Util::SynchronizedPointer<ODMRData>& ModuleData)
	{
		bool IsReady = State->GetState() == StateType::Ready;
		StatusBar.CurrentState = State;

		StatusBar.AcquisitionTimeLabel->setText((IsReady || ModuleData->AcquisitionTime <= 0) ? "" : QString::fromStdString("Acquisition time: " + Util::ToStr(ModuleData->AcquisitionTime, 2) + " s"));
		StatusBar.SweepStateLabel->setText(IsReady ? "" : (!ModuleData->GetSweepNumberSteps() ? "single run" :
			(QString("Sweep ") + QString::number(ModuleData->CurrentSweepIndex + 1) + " / " + QString::number(ModuleData->GetSweepNumberSteps()))));

		ui->GBRFSweep->setEnabled(IsReady);
		ui->GBRFModulation->setEnabled(IsReady);
		ui->GBDataAcquisition->setEnabled(IsReady);
		ui->GBSaveData->setEnabled(IsReady);
		ui->GBSensitivity->setEnabled(IsReady);
		ui->GBSensitivityAnalysis->setEnabled(IsReady);
		ui->GBParamSweep->setEnabled(IsReady);
		ui->BStart->setEnabled(IsReady);
		ui->BStartSensitivity->setEnabled(IsReady);
		ui->BStop->setEnabled(!IsReady);
	}

	void ODMRWidget::UpdateUIData(Util::SynchronizedPointer<ODMRData>& ModuleData)
	{
		ui->LERFNumSamples->setText(QString::number(ModuleData->GetNumSamples()));
		ui->LEODMRFitSlope->setText(QString::number(std::get<1>(ModuleData->ODMRPlot.FitParams) * 1e6) + " [y]/MHz");
		ui->LEODMRFitOffset->setText(QString::number(std::get<0>(ModuleData->ODMRPlot.FitParams)) + " [y]");

		if (!ui->SBSaveDataCurrentIndex->hasFocus())
			ui->SBSaveDataCurrentIndex->setValue(ModuleData->CurrentSaveIndex);

		StatusBar.Update();
	}

	void ODMRWidget::UpdateODMRPlot(ODMRPlotType& ODMRPlot)
	{
		if (ODMRPlot.HasChanged)
			ODMRPlot.HasChanged = false;
		else
		{
			ODMRPlot.PlotInfo.ResetHoveredSample();
			ODMRPlot.PlotInfo.ReprocessSamples(ODMRPlot.DataPoints, 0);
			ODMRPlot.PlotInfo.ReprocessSamples(ODMRPlot.FitPoints, 0);
		}

		ODMRGraph->UpdateSeries(0, ODMRPlot.DataPoints, ODMRPlot.PlotInfo);
		ODMRGraph->UpdateSeries(1, ODMRPlot.FitPoints, ODMRPlot.PlotInfo);
		ODMRGraph->UpdateData(ODMRPlot.PlotInfo);
	}

	void ODMRWidget::UpdateSensitivityPlot(SensitivityPlotType& SensitivityPlot)
	{
		if (SensitivityPlot.HasChanged)
			SensitivityPlot.HasChanged = false;
		else
		{
			SensitivityPlot.PlotInfo.ResetHoveredSample();
			SensitivityPlot.PlotInfo.ReprocessSamples(SensitivityPlot.DataPoints, 0);
		}

		SensitivityGraph->UpdateSeries(0, SensitivityPlot.DataPoints, SensitivityPlot.PlotInfo);
		SensitivityGraph->UpdateData(SensitivityPlot.PlotInfo);
	}

	void ODMRWidget::OnBrowseSavePathClicked()
	{
		auto Filename = Util::PromptSaveFilePathModule(this, "Select file for saving ODMR data",
			".csv", " Comma-separated values file (*.csv)");
		if (Filename.isEmpty())
			return;

		// Emits signal to update module data accordingly.
		ui->LESaveDataPath->setText(Filename);
	}

	void ODMRWidget::OnSweepSeriesParamChanged(int Index)
	{
		QDoubleSpinBox* Destiny = nullptr;

		if (Index != 2)	// != ODMRData::SweepSeriesType::AnalogOut
		{
			switch (Index)
			{
			case 0:	// ODMRData::SweepSeriesType::RFModulationDepth
				Destiny = ui->SBRFModulationDepth;
				break;
			case 1:	// ODMRData::SweepSeriesType::RFPower
				Destiny = ui->SBRFPower;
				break;
			}

			if (Destiny)
			{
				ui->SBParamSweepStart->setMinimum(Destiny->minimum());
				ui->SBParamSweepStart->setMaximum(Destiny->maximum());
				ui->SBParamSweepStart->setSuffix(Destiny->suffix());
				ui->SBParamSweepStop->setMinimum(Destiny->minimum());
				ui->SBParamSweepStop->setMaximum(Destiny->maximum());
				ui->SBParamSweepStop->setSuffix(Destiny->suffix());
				ui->SBParamSweepStep->setMinimum(Destiny->minimum());
				ui->SBParamSweepStep->setMaximum(Destiny->maximum());
				ui->SBParamSweepStep->setSuffix(Destiny->suffix());
			}
		}
		else
		{
			ui->SBParamSweepStart->setMinimum(AuxAnalogOutMinValue);
			ui->SBParamSweepStart->setMaximum(AuxAnalogOutMaxValue);
			ui->SBParamSweepStart->setSuffix(QString(" ") + DynExp::Units::UnitTypeToStr(AuxAnalogOutValueUnit));
			ui->SBParamSweepStop->setMinimum(AuxAnalogOutMinValue);
			ui->SBParamSweepStop->setMaximum(AuxAnalogOutMaxValue);
			ui->SBParamSweepStop->setSuffix(ui->SBParamSweepStart->suffix());
			ui->SBParamSweepStep->setMinimum(AuxAnalogOutMinValue);
			ui->SBParamSweepStep->setMaximum(AuxAnalogOutMaxValue);
			ui->SBParamSweepStep->setSuffix(ui->SBParamSweepStart->suffix());
		}
	}
}