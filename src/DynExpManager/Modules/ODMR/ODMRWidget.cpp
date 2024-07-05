// This file is part of DynExp.

#include "stdafx.h"
#include "ODMR.h"
#include "moc_ODMRWidget.cpp"
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
		: QModuleWidget(Owner, parent), StatusBar(this),
		ODMRDataSeries(nullptr), ODMRFitSeries(nullptr), ODMRDataChart(nullptr), ODMRXAxis(new QValueAxis(this)), ODMRYAxis(new QValueAxis(this)),
		SensitivityDataSeries(nullptr), SensitivityDataChart(nullptr), SensitivityXAxis(new QLogValueAxis(this)), SensitivityYAxis(new QLogValueAxis(this))
	{
		ui.setupUi(this);

		// Status bar
		ui.MainStatusBar->addWidget(StatusBar.StateLabel, 5);
		ui.MainStatusBar->addWidget(StatusBar.SweepStateLabel, 2);
		ui.MainStatusBar->addWidget(StatusBar.AcquisitionTimeLabel, 3);

		// Graph to display a single ODMR trace
		ODMRDataChart = new QChart();
		ui.ODMRChartView->setChart(ODMRDataChart);						// Takes ownership of ODMRDataChart.
		ui.ODMRChartView->setRenderHint(QPainter::Antialiasing);
		ODMRDataChart->setTheme(DynExpUI::DefaultQChartTheme);
		ODMRDataChart->legend()->setVisible(false);
		ODMRXAxis->setTitleText("frequency in GHz");

		// Graph to display a single sensitivity measurement
		SensitivityDataChart = new QChart();
		ui.SensitivityChartView->setChart(SensitivityDataChart);		// Takes ownership of SensitivityDataChart.
		ui.SensitivityChartView->setRenderHint(QPainter::Antialiasing);
		SensitivityDataChart->setTheme(DynExpUI::DefaultQChartTheme);
		SensitivityDataChart->legend()->setVisible(false);
		SensitivityXAxis->setBase(10);
		SensitivityXAxis->setTitleText("frequency in Hz");
		SensitivityYAxis->setBase(10);
		SensitivityYAxis->setTitleText("sensitivity ASD in T/sqrt(Hz)");

		// Chart takes ownership of axes.
		ODMRDataChart->addAxis(ODMRXAxis, Qt::AlignBottom);
		ODMRDataChart->addAxis(ODMRYAxis, Qt::AlignLeft);
		SensitivityDataChart->addAxis(SensitivityXAxis, Qt::AlignBottom);
		SensitivityDataChart->addAxis(SensitivityYAxis, Qt::AlignLeft);

		connect(ui.CBParamSweepType, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &ODMRWidget::OnSweepSeriesParamChanged);
	}

	void ODMRWidget::InitializeUI(Util::SynchronizedPointer<ODMRData>& ModuleData)
	{
		if (!GetUIInitialized())
		{
			ui.SBRFPower->setValue(ModuleData->RFPower);
			ui.SBRFCenter->setValue(ModuleData->RFCenterFreq / 1e6);
			ui.SBRFSpan->setValue(ModuleData->RFFreqSpan / 1e6);
			ui.SBRFFreqSpacing->setValue(ModuleData->RFFreqSpacing / 1e3);
			ui.SBRFDwellTime->setValue(ModuleData->RFDwellTime / 1e-3);
			ui.RBRFModulationTypeNone->setChecked(true);	// Emits signal to update module data accordingly.
			ui.SBRFModulationFreq->setValue(ModuleData->RFModulationFreq / 1e3);
			ui.SBRFModulationDepth->setValue(ModuleData->RFModulationDepth / 1e3);
			ui.SBDataAcquisitionODMRSamplingRate->setValue(ModuleData->ODMRSamplingRate);
			ui.LESaveDataPath->setText(QString::fromStdString(ModuleData->SaveDataPath));
			ui.SBSaveDataCurrentIndex->setValue(ModuleData->CurrentSaveIndex);
			ui.CBSaveDataEnable->setChecked(ModuleData->AutosaveEnabled);
			ui.CBSensitivityEnable->setChecked(ModuleData->SensitivityEnabled);
			ui.CBSensitivityOncePerSweep->setChecked(ModuleData->SensitivityOncePerSweep);
			ui.CBSensitivityOffResEnable->setChecked(ModuleData->SensitivityOffResonanceEnabled);
			ui.SBSensitivityFreq->setValue(ModuleData->SensitivityResonanceFreq / 1e6);
			ui.SBSensitivityOffResFreq->setValue(ModuleData->SensitivityOffResonanceFreq / 1e6);
			ui.SBSensitivitySpan->setValue(ModuleData->SensitivityResonanceSpan / 1e6);
			ui.SBSensitivitySamplingRate->setValue(ModuleData->SensitivitySamplingRate);
			ui.SBSensitivityDuration->setValue(ModuleData->SensitivityDuration);
			ui.GBSensitivityAnalysis->setChecked(ModuleData->SensitivityAnalysisEnabled);
			ui.SBGyromagneticRatio->setValue(ModuleData->GyromagneticRatio / 1e6);
			ui.CBParamSweepEnable->setChecked(ModuleData->SweepSeriesEnabled);
			if (ModuleData->TestFeature(ODMRData::FeatureType::AuxAnalogOut))
				ui.CBParamSweepType->insertItem(ui.CBParamSweepType->count(), "Auxiliary analog out");
			ui.CBParamSweepType->setCurrentIndex(0);		// Emits signal to update module data accordingly, but see below.
			ui.SBParamSweepStart->setValue(ModuleData->SweepSeriesStart);
			ui.SBParamSweepStop->setValue(ModuleData->SweepSeriesStop);
			ui.SBParamSweepStep->setValue(ModuleData->SweepSeriesStep);
			ui.CBParamSweepRetrace->setChecked(ModuleData->SweepSeriesRetrace);
			ui.CBParamSweepAdvanceLastValue->setChecked(ModuleData->SweepSeriesAdvanceLastValue);

			ui.SBRFPower->setMinimum(ModuleData->RFGeneratorMinFuncDesc.Amplitude);
			ui.SBRFPower->setMaximum(ModuleData->RFGeneratorMaxFuncDesc.Amplitude);
			ui.SBRFPower->setValue(ModuleData->RFGeneratorDefaultFuncDesc.Amplitude);
			ui.SBRFPower->setSuffix(QString(" ") + ModuleData->GetRFGenerator()->GetValueUnitStr());
			ui.SBRFCenter->setMinimum(ModuleData->RFGeneratorMinFuncDesc.FrequencyInHz / 1e6);
			ui.SBRFCenter->setMaximum(ModuleData->RFGeneratorMaxFuncDesc.FrequencyInHz / 1e6);
			ui.SBRFCenter->setValue(ModuleData->RFGeneratorDefaultFuncDesc.FrequencyInHz / 1e6);
			ui.SBSensitivityFreq->setMinimum(ModuleData->RFGeneratorMinFuncDesc.FrequencyInHz / 1e6);
			ui.SBSensitivityFreq->setMaximum(ModuleData->RFGeneratorMaxFuncDesc.FrequencyInHz / 1e6);
			ui.SBSensitivityFreq->setValue(ModuleData->RFGeneratorDefaultFuncDesc.FrequencyInHz / 1e6);

			ui.SBDataAcquisitionODMRSamplingRate->setEnabled(ModuleData->TestFeature(ODMRData::FeatureType::LockinDetection));
			ui.SBSensitivitySamplingRate->setEnabled(ModuleData->TestFeature(ODMRData::FeatureType::LockinDetection));
			ui.SBSensitivityDuration->setEnabled(ModuleData->TestFeature(ODMRData::FeatureType::LockinDetection));

			ODMRYAxis->setTitleText(QString("ODMR signal in ") + ModuleData->GetSignalDetector()->GetValueUnitStr());

			// This is not emitted if setCurrentIndex() does not change the index (because it already is the desired value).
			// So, do it manually.
			OnSweepSeriesParamChanged(ui.CBParamSweepType->currentIndex());

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

		ui.GBRFSweep->setEnabled(IsReady);
		ui.GBRFModulation->setEnabled(IsReady);
		ui.GBDataAcquisition->setEnabled(IsReady);
		ui.GBSaveData->setEnabled(IsReady);
		ui.GBSensitivity->setEnabled(IsReady);
		ui.GBSensitivityAnalysis->setEnabled(IsReady);
		ui.GBParamSweep->setEnabled(IsReady);
		ui.BStart->setEnabled(IsReady);
		ui.BStartSensitivity->setEnabled(IsReady);
		ui.BStop->setEnabled(!IsReady);
	}

	void ODMRWidget::UpdateUIData(Util::SynchronizedPointer<ODMRData>& ModuleData)
	{
		ui.LERFNumSamples->setText(QString::number(ModuleData->GetNumSamples()));
		ui.LODMRCurrentSelection->setText(ModuleData->ODMRPlot.SelectedPoint.isNull() ? QString() : (QString::number(ModuleData->ODMRPlot.SelectedPoint.x() * 1e3, 'f', 3) + " MHz"));
		ui.LEODMRFitSlope->setText(QString::number(std::get<1>(ModuleData->ODMRPlot.FitParams) * 1e6) + " [y]/MHz");
		ui.LEODMRFitOffset->setText(QString::number(std::get<0>(ModuleData->ODMRPlot.FitParams)) + " [y]");

		if (!ui.SBSaveDataCurrentIndex->hasFocus())
			ui.SBSaveDataCurrentIndex->setValue(ModuleData->CurrentSaveIndex);

		StatusBar.Update();
	}

	void ODMRWidget::UpdateODMRPlot(const ODMRPlotType& ODMRPlot)
	{
		ODMRDataChart->removeAllSeries();
		ODMRDataSeries = new QLineSeries(this);
		ODMRDataSeries->append(ODMRPlot.DataPoints);
		ODMRDataSeries->setPointsVisible(false);
		ODMRFitSeries = new QLineSeries(this);
		ODMRFitSeries->append(ODMRPlot.FitPoints);
		ODMRFitSeries->setPointsVisible(false);

		ODMRXAxis->setRange(ODMRPlot.DataPointsMinValues.x(), ODMRPlot.DataPointsMaxValues.x());
		ODMRYAxis->setRange(ODMRPlot.DataPointsMinValues.y(), ODMRPlot.DataPointsMaxValues.y());

		ODMRDataChart->addSeries(ODMRDataSeries);
		ODMRDataChart->addSeries(ODMRFitSeries);
		ODMRDataSeries->attachAxis(ODMRDataChart->axes()[0]);
		ODMRDataSeries->attachAxis(ODMRDataChart->axes()[1]);
		ODMRFitSeries->attachAxis(ODMRDataChart->axes()[0]);
		ODMRFitSeries->attachAxis(ODMRDataChart->axes()[1]);
	}

	void ODMRWidget::UpdateSensitivityPlot(const SensitivityPlotType& SensitivityPlot)
	{
		SensitivityDataChart->removeAllSeries();
		SensitivityDataSeries = new QLineSeries(this);
		SensitivityDataSeries->append(SensitivityPlot.DataPoints);
		SensitivityDataSeries->setPointsVisible(false);

		SensitivityYAxis->setRange(SensitivityPlot.DataPointsMinValues.y(), SensitivityPlot.DataPointsMaxValues.y());
		SensitivityXAxis->setRange(SensitivityPlot.DataPointsMinValues.x(), SensitivityPlot.DataPointsMaxValues.x());

		SensitivityDataChart->addSeries(SensitivityDataSeries);
		SensitivityDataSeries->attachAxis(SensitivityDataChart->axes()[0]);
		SensitivityDataSeries->attachAxis(SensitivityDataChart->axes()[1]);
	}

	void ODMRWidget::OnBrowseSavePathClicked()
	{
		auto Filename = Util::PromptSaveFilePathModule(this, "Select file for saving ODMR data",
			".csv", " Comma-separated values file (*.csv)");
		if (Filename.isEmpty())
			return;

		// Emits signal to update module data accordingly.
		ui.LESaveDataPath->setText(Filename);
	}

	void ODMRWidget::OnSweepSeriesParamChanged(int Index)
	{
		QDoubleSpinBox* Destiny = nullptr;

		if (Index != 2)	// != ODMRData::SweepSeriesType::AnalogOut
		{
			switch (Index)
			{
			case 0:	// ODMRData::SweepSeriesType::RFModulationDepth
				Destiny = ui.SBRFModulationDepth;
				break;
			case 1:	// ODMRData::SweepSeriesType::RFPower
				Destiny = ui.SBRFPower;
				break;
			}

			if (Destiny)
			{
				ui.SBParamSweepStart->setMinimum(Destiny->minimum());
				ui.SBParamSweepStart->setMaximum(Destiny->maximum());
				ui.SBParamSweepStart->setSuffix(Destiny->suffix());
				ui.SBParamSweepStop->setMinimum(Destiny->minimum());
				ui.SBParamSweepStop->setMaximum(Destiny->maximum());
				ui.SBParamSweepStop->setSuffix(Destiny->suffix());
				ui.SBParamSweepStep->setMinimum(Destiny->minimum());
				ui.SBParamSweepStep->setMaximum(Destiny->maximum());
				ui.SBParamSweepStep->setSuffix(Destiny->suffix());
			}
		}
		else
		{
			ui.SBParamSweepStart->setMinimum(AuxAnalogOutMinValue);
			ui.SBParamSweepStart->setMaximum(AuxAnalogOutMaxValue);
			ui.SBParamSweepStart->setSuffix(QString(" ") + DynExpInstr::DataStreamInstrumentData::UnitTypeToStr(AuxAnalogOutValueUnit));
			ui.SBParamSweepStop->setMinimum(AuxAnalogOutMinValue);
			ui.SBParamSweepStop->setMaximum(AuxAnalogOutMaxValue);
			ui.SBParamSweepStop->setSuffix(ui.SBParamSweepStart->suffix());
			ui.SBParamSweepStep->setMinimum(AuxAnalogOutMinValue);
			ui.SBParamSweepStep->setMaximum(AuxAnalogOutMaxValue);
			ui.SBParamSweepStep->setSuffix(ui.SBParamSweepStart->suffix());
		}
	}
}