// This file is part of DynExp.

#include "stdafx.h"
#include "moc_SignalDesigner.cpp"
#include "SignalDesigner.h"

namespace DynExpModule
{
	SignalDesignerWidget::SignalDesignerWidget(SignalDesigner& Owner, QModuleWidget* parent)
		: QModuleWidget(Owner, parent), PulsesContextMenu(new QMenu(this)),
		AddPulseAction(nullptr), RemovePulseAction(nullptr), ClearPulsesAction(nullptr)
	{
		ui.setupUi(this);

		ui.TWPulses->setItemPrototype(new Util::NumericSortingTableWidgetItem());
		ui.TWPulses->setItemDelegateForColumn(0, new Util::NumericOnlyItemDelegate(this, 0));
		connect(ui.TWPulses, &QTableWidget::itemChanged, this, &SignalDesignerWidget::OnPulsesChanged);

		AddPulseAction = PulsesContextMenu->addAction("&New Pulse", this, &SignalDesignerWidget::OnAddPulse, QKeySequence(Qt::Key_N));
		addAction(AddPulseAction);		// for shortcuts
		RemovePulseAction = PulsesContextMenu->addAction("&Delete selected Pulse(s)", this, &SignalDesignerWidget::OnRemovePulse, QKeySequence(Qt::Key_Delete));
		addAction(RemovePulseAction);	// for shortcuts
		ClearPulsesAction = PulsesContextMenu->addAction(QIcon(DynExpUI::Icons::Delete), "&Clear all Pulses", this, &SignalDesignerWidget::OnClearPulses);
	}

	bool SignalDesignerWidget::HavePulsesChanged() noexcept
	{
		bool Changed = PulsesChanged;
		PulsesChanged = false;

		return Changed;
	}

	/**
	 * @brief Layout changes not involving ModuleData->CurrentWaveform come here.
	 * @param ModuleData Data instance assigned to this SignalDesigner instance.
	*/
	void SignalDesignerWidget::InitializeUI(Util::SynchronizedPointer<SignalDesignerData>& ModuleData)
	{
		if (ModuleData->IsUIInitialized())
			return;

		// Get caps from instrument.
		ModuleData->MinFuncDesc = ModuleData->GetFuncGen()->GetMinCaps();
		ModuleData->MaxFuncDesc = ModuleData->GetFuncGen()->GetMaxCaps();
		ModuleData->DefaultFuncDesc = ModuleData->GetFuncGen()->GetParamDefaults();
		ModuleData->WaveformCaps = ModuleData->GetFuncGen()->GetWaveformCaps();
		ModuleData->TriggerCaps = ModuleData->GetFuncGen()->GetTriggerCaps();

		// Read current configuration from instrument.
		auto InstrData = DynExp::dynamic_InstrumentData_cast<DynExpInstr::FunctionGenerator>(ModuleData->GetFuncGen()->GetInstrumentData());

		if (InstrData->GetCurrentWaveformType() != DynExpInstr::FunctionGeneratorDefs::WaveformTypes::None)
		{
			ModuleData->CurrentWaveform = InstrData->GetCurrentWaveformType();
			ModuleData->CurrentFrequencyInHz = InstrData->GetCurrentFrequencyInHz();
			ModuleData->CurrentAmplitude = InstrData->GetCurrentAmplitude();
			ModuleData->CurrentOffset = InstrData->GetCurrentOffset();
			ModuleData->CurrentPhaseInRad = InstrData->GetCurrentPhaseInRad();
			ModuleData->CurrentDutyCycle = InstrData->GetCurrentDutyCycle();
			ModuleData->CurrentPulses = InstrData->GetCurrentPulses();
		}
		else
		{
			if (ModuleData->WaveformCaps.Test(DynExpInstr::FunctionGenerator::WaveformCapsType::Sine) ||
				(ModuleData->WaveformCaps.Test(DynExpInstr::FunctionGenerator::WaveformCapsType::UserDefined) &&
					ModuleData->GetFuncGen()->GetValueUnit() != DynExpInstr::DataStreamInstrumentData::UnitType::LogicLevel))
				ModuleData->CurrentWaveform = DynExpInstr::FunctionGeneratorDefs::WaveformTypes::Sine;
			else if (ModuleData->WaveformCaps.Test(DynExpInstr::FunctionGenerator::WaveformCapsType::Rect) ||
				ModuleData->WaveformCaps.Test(DynExpInstr::FunctionGenerator::WaveformCapsType::UserDefined))
				ModuleData->CurrentWaveform = DynExpInstr::FunctionGeneratorDefs::WaveformTypes::Rect;
			else if (ModuleData->WaveformCaps.Test(DynExpInstr::FunctionGenerator::WaveformCapsType::Ramp))
				ModuleData->CurrentWaveform = DynExpInstr::FunctionGeneratorDefs::WaveformTypes::Ramp;
			else if (ModuleData->WaveformCaps.Test(DynExpInstr::FunctionGenerator::WaveformCapsType::Pulse))
				ModuleData->CurrentWaveform = DynExpInstr::FunctionGeneratorDefs::WaveformTypes::Pulse;

			ModuleData->CurrentFrequencyInHz = ModuleData->DefaultFuncDesc.FrequencyInHz;
			ModuleData->CurrentAmplitude = ModuleData->DefaultFuncDesc.Amplitude;
			ModuleData->CurrentOffset = ModuleData->DefaultFuncDesc.Offset;
			ModuleData->CurrentPhaseInRad = 0;
			ModuleData->CurrentDutyCycle = .5;
			ModuleData->CurrentPulses.Reset();
		}

		ModuleData->CurrentTriggerMode = InstrData->GetCurrentTriggerMode();
		ModuleData->CurrentTriggerEdge = InstrData->GetCurrentTriggerEdge();
		ModuleData->CurrentAutostart = InstrData->GetShouldAutostart();

		// Block value changed signals while initializing UI.
		const QSignalBlocker SBFrequencyInHzBlocker(ui.SBFrequencyInHz);
		const QSignalBlocker SBAmplitudeBlocker(ui.SBAmplitude);
		const QSignalBlocker SBYOffsetBlocker(ui.SBYOffset);
		const QSignalBlocker SBPhaseInDegreeBlocker(ui.SBPhaseInDegree);
		const QSignalBlocker SBDutyCycleBlocker(ui.SBDutyCycle);
		const QSignalBlocker CBAutostartBlocker(ui.CBAutostart);
		const QSignalBlocker TWPulsesBlocker(ui.TWPulses);
		const QSignalBlocker CBTriggerModeBlocker(ui.CBTriggerMode);
		const QSignalBlocker CBTriggerEdgeBlocker(ui.CBTriggerEdge);
		const QSignalBlocker BPersistBlocker(ui.BPersist);

		// Adjust available waveform types and select appropriate one.
		auto SigTypeListView = qobject_cast<QListView*>(ui.CBSignalType->view());
		{
			const QSignalBlocker CBSignalTypeBlocker(ui.CBSignalType);

			// TODO: This solution is quick & dirty...
			SigTypeListView->setRowHidden(ui.CBSignalType->findText("Sine", Qt::MatchFlag::MatchContains),
				(!ModuleData->WaveformCaps.Test(DynExpInstr::FunctionGenerator::WaveformCapsType::Sine) &&
					!ModuleData->WaveformCaps.Test(DynExpInstr::FunctionGenerator::WaveformCapsType::UserDefined))
				|| ModuleData->GetFuncGen()->GetValueUnit() == DynExpInstr::DataStreamInstrumentData::UnitType::LogicLevel);
			SigTypeListView->setRowHidden(ui.CBSignalType->findText("Rect", Qt::MatchFlag::MatchContains),
				!ModuleData->WaveformCaps.Test(DynExpInstr::FunctionGenerator::WaveformCapsType::Rect) &&
				!ModuleData->WaveformCaps.Test(DynExpInstr::FunctionGenerator::WaveformCapsType::UserDefined));
			SigTypeListView->setRowHidden(ui.CBSignalType->findText("Ramp", Qt::MatchFlag::MatchContains),
				(!ModuleData->WaveformCaps.Test(DynExpInstr::FunctionGenerator::WaveformCapsType::Ramp) &&
					!ModuleData->WaveformCaps.Test(DynExpInstr::FunctionGenerator::WaveformCapsType::UserDefined))
				|| ModuleData->GetFuncGen()->GetValueUnit() == DynExpInstr::DataStreamInstrumentData::UnitType::LogicLevel);
			SigTypeListView->setRowHidden(ui.CBSignalType->findText("Pulse", Qt::MatchFlag::MatchContains),
				!ModuleData->WaveformCaps.Test(DynExpInstr::FunctionGenerator::WaveformCapsType::Pulse) &&
				!ModuleData->WaveformCaps.Test(DynExpInstr::FunctionGenerator::WaveformCapsType::UserDefined));
		} // CBSignalTypeBlocker destroyed here.
			
		int CBSignalTypeIndex = -1;
		if (ModuleData->CurrentWaveform == DynExpInstr::FunctionGeneratorDefs::WaveformTypes::Sine &&
			!SigTypeListView->isRowHidden(ui.CBSignalType->findText("Sine", Qt::MatchFlag::MatchContains)))
			CBSignalTypeIndex = ui.CBSignalType->findText("Sine", Qt::MatchFlag::MatchContains);
		else if (ModuleData->CurrentWaveform == DynExpInstr::FunctionGeneratorDefs::WaveformTypes::Rect &&
			!SigTypeListView->isRowHidden(ui.CBSignalType->findText("Rect", Qt::MatchFlag::MatchContains)))
			CBSignalTypeIndex = ui.CBSignalType->findText("Rect", Qt::MatchFlag::MatchContains);
		else if (ModuleData->CurrentWaveform == DynExpInstr::FunctionGeneratorDefs::WaveformTypes::Ramp &&
			!SigTypeListView->isRowHidden(ui.CBSignalType->findText("Ramp", Qt::MatchFlag::MatchContains)))
			CBSignalTypeIndex = ui.CBSignalType->findText("Ramp", Qt::MatchFlag::MatchContains);
		else if (ModuleData->CurrentWaveform == DynExpInstr::FunctionGeneratorDefs::WaveformTypes::Pulse &&
			!SigTypeListView->isRowHidden(ui.CBSignalType->findText("Pulse", Qt::MatchFlag::MatchContains)))
			CBSignalTypeIndex = ui.CBSignalType->findText("Pulse", Qt::MatchFlag::MatchContains);

		const auto RowHiddenFunc = [SigTypeListView](int i) { return SigTypeListView->isRowHidden(i); };
		const auto SignalTypeIndexRange = std::views::iota(0, ui.CBSignalType->count());
		if (std::ranges::all_of(SignalTypeIndexRange, RowHiddenFunc))
			for (auto* Widget : this->findChildren<QWidget*>())
				Widget->setEnabled(false);
		else if (CBSignalTypeIndex < 0)
		{
			auto FirstAvailable = std::ranges::find_if(SignalTypeIndexRange, RowHiddenFunc);

			if (FirstAvailable != SignalTypeIndexRange.end())
			{
				// CBSignalType is intended to emit QComboBox::currentTextChanged() here.
				ui.CBSignalType->setCurrentIndex(*FirstAvailable);
				emit ui.CBSignalType->currentTextChanged(ui.CBSignalType->currentText());
			}
		}
		else
		{
			const QSignalBlocker CBSignalTypeBlocker(ui.CBSignalType);
			ui.CBSignalType->setCurrentIndex(CBSignalTypeIndex);
		}

		// Frequency
		ui.SBFrequencyInHz->setMinimum(ModuleData->MinFuncDesc.FrequencyInHz);
		ui.SBFrequencyInHz->setMaximum(ModuleData->MaxFuncDesc.FrequencyInHz);
		ui.SBFrequencyInHz->setValue(ModuleData->CurrentFrequencyInHz);
		
		// Amplitude
		if (ModuleData->GetFuncGen()->GetValueUnit() != DynExpInstr::DataStreamInstrumentData::UnitType::LogicLevel)
		{
			ui.SBAmplitude->setMinimum(ModuleData->MinFuncDesc.Amplitude);
			ui.SBAmplitude->setMaximum(ModuleData->MaxFuncDesc.Amplitude);
			ui.SBAmplitude->setValue(ModuleData->CurrentAmplitude);
			ui.SBAmplitude->setSuffix(QString(" ") + ModuleData->GetFuncGen()->GetValueUnitStr());
		}
		
		// Offset
		if ((ModuleData->MinFuncDesc.Offset == 0 && ModuleData->MaxFuncDesc.Offset == 0)
			|| ModuleData->GetFuncGen()->GetValueUnit() == DynExpInstr::DataStreamInstrumentData::UnitType::LogicLevel)
		{
			ui.LYOffset->setVisible(false);
			ui.SBYOffset->setVisible(false);
		}
		else
		{
			ui.LYOffset->setVisible(true);
			ui.SBYOffset->setVisible(true);
			ui.SBYOffset->setMinimum(ModuleData->MinFuncDesc.Offset);
			ui.SBYOffset->setMaximum(ModuleData->MaxFuncDesc.Offset);
			ui.SBYOffset->setValue(ModuleData->CurrentOffset);
			ui.SBYOffset->setSuffix(QString(" ") + ModuleData->GetFuncGen()->GetValueUnitStr());
		}

		// Phase
		if (ModuleData->GetFuncGen()->IsPhaseAdjustable())
			ui.SBPhaseInDegree->setValue(ModuleData->CurrentPhaseInRad / std::numbers::pi * 180.0);
		
		// Duty cycle
		ui.SBDutyCycle->setValue(ModuleData->CurrentDutyCycle * 100.0);

		// Pulses
		ui.TWPulses->clear();
		ui.TWPulses->setRowCount(0);
		ui.TWPulses->setHorizontalHeaderLabels({ "Time [us]", "Value [" + QString(ModuleData->GetFuncGen()->GetValueUnitStr()) + "]" });
		auto OldDelegate = ui.TWPulses->itemDelegateForColumn(1);
		ui.TWPulses->setItemDelegateForColumn(1, ModuleData->GetFuncGen()->GetValueUnit() == DynExpInstr::DataStreamInstrumentData::UnitType::LogicLevel ?
			new Util::DigitalOnlyItemDelegate(this) : new Util::NumericOnlyItemDelegate(this));
		if (OldDelegate)
			OldDelegate->deleteLater();
		for (const auto& Pulse : ModuleData->CurrentPulses.Pulses)
		{
			ui.TWPulses->insertRow(ui.TWPulses->rowCount());
			ui.TWPulses->setItem(ui.TWPulses->rowCount() - 1, 0, new Util::NumericSortingTableWidgetItem(QString::fromStdString(Util::ToStr(Pulse.first * std::micro::den))));
			ui.TWPulses->setItem(ui.TWPulses->rowCount() - 1, 1, new Util::NumericSortingTableWidgetItem(QString::fromStdString(Util::ToStr(Pulse.second))));
		}

		// Trigger
		// TODO: This solution is quick & dirty...
		int CBTriggerModeIndex = 0;
		if (ModuleData->CurrentTriggerMode == DynExpInstr::FunctionGeneratorDefs::TriggerDescType::TriggerModeType::Continuous)
			CBTriggerModeIndex = ui.CBTriggerMode->findText("Continuous", Qt::MatchFlag::MatchContains);
		else if (ModuleData->CurrentTriggerMode == DynExpInstr::FunctionGeneratorDefs::TriggerDescType::TriggerModeType::ExternSingle)
			CBTriggerModeIndex = ui.CBTriggerMode->findText("Single", Qt::MatchFlag::MatchContains);
		else if (ModuleData->CurrentTriggerMode == DynExpInstr::FunctionGeneratorDefs::TriggerDescType::TriggerModeType::ExternStep)
			CBTriggerModeIndex = ui.CBTriggerMode->findText("Step", Qt::MatchFlag::MatchContains);
		else if (ModuleData->CurrentTriggerMode == DynExpInstr::FunctionGeneratorDefs::TriggerDescType::TriggerModeType::Manual)
			CBTriggerModeIndex = ui.CBTriggerMode->findText("Manual", Qt::MatchFlag::MatchContains);
		ui.CBTriggerMode->setCurrentIndex(CBTriggerModeIndex);

		ui.CBTriggerEdge->setCurrentIndex(ModuleData->CurrentTriggerEdge == DynExpInstr::FunctionGeneratorDefs::TriggerDescType::TriggerEdgeType::Fall ?
			ui.CBTriggerEdge->findText("Fall", Qt::MatchFlag::MatchContains) : ui.CBTriggerEdge->findText("Rise", Qt::MatchFlag::MatchContains));
		ui.GBTrigger->setVisible(ModuleData->TriggerCaps.Test(DynExpInstr::FunctionGenerator::TriggerCapsType::CanConfigure));
		ui.BForce->setVisible(ModuleData->TriggerCaps.Test(DynExpInstr::FunctionGenerator::TriggerCapsType::CanForce));

		// Autostart, persist
		ui.CBAutostart->setChecked(ModuleData->CurrentAutostart);
		ui.BPersist->setChecked(ModuleData->CurrentPersistParameters);

		ModuleData->SetUIInitialized();
	}

	void SignalDesignerWidget::OnPulsesContextMenuRequested(const QPoint& Position)
	{
		PulsesContextMenu->exec(ui.TWPulses->mapToGlobal(Position));
	}

	void SignalDesignerWidget::OnAddPulse(bool)
	{
		ui.TWPulses->insertRow(ui.TWPulses->rowCount());
	}

	void SignalDesignerWidget::OnRemovePulse(bool)
	{
		auto SelectedItems = ui.TWPulses->selectedItems();
		QSet<int> SelectedRows;

		// Add selected rows.
		for (const auto Item : SelectedItems)
			SelectedRows.insert(Item->row());

		// Add empty rows.
		for (int i = 0; i < ui.TWPulses->rowCount(); ++i)
			if (!ui.TWPulses->item(i, 0) && !ui.TWPulses->item(i, 1))
				SelectedRows.insert(i);

		auto RowsToRemove = SelectedRows.values();

		// Remove bottommost rows first to not change the order.
		std::sort(RowsToRemove.begin(), RowsToRemove.end(), std::greater{});
		for (const auto Row : RowsToRemove)
			ui.TWPulses->removeRow(Row);
	}

	void SignalDesignerWidget::OnClearPulses(bool)
	{
		ui.TWPulses->clearContents();
		ui.TWPulses->setRowCount(0);
	}

	void SignalDesignerWidget::OnPulsesChanged(QTableWidgetItem*)
	{
		ui.TWPulses->sortItems(0, Qt::SortOrder::AscendingOrder);

		std::vector<double> PulseStarts, PulseAmplitudes;
		for (int i = 0; i < ui.TWPulses->rowCount(); ++i)
		{
			bool ok = true;
			if (!ui.TWPulses->item(i, 0))
				continue;
			double Start = Util::GetDefaultQtLocale().toDouble(ui.TWPulses->item(i, 0)->text(), &ok);
			if (!ok)
				continue;
			
			if (!ui.TWPulses->item(i, 1))
				continue;
			double Amplitude = Util::GetDefaultQtLocale().toDouble(ui.TWPulses->item(i, 1)->text(), &ok);
			if (!ok)
				continue;

			PulseStarts.push_back(Start / std::micro::den);
			PulseAmplitudes.push_back(Amplitude);
		}

		Pulses = { PulseStarts, PulseAmplitudes };
		PulsesChanged = true;
	}

	void SignalDesignerData::SetCurrentFuncGenIndex(int Index)
	{
		// Update CurrentSourceIndex first since GetFuncGen() depends on it.
		CurrentFuncGenIndex = Index;

		UIInitialized = false;
	}

	void SignalDesignerData::ResetImpl(dispatch_tag<QModuleDataBase>)
	{
		Init();
	}

	void SignalDesignerData::Init()
	{
		UIInitialized = false;
		CurrentFuncGenIndex = 0;

		MinFuncDesc = {};
		MaxFuncDesc = {};
		DefaultFuncDesc = {};
		WaveformCaps = {};
		TriggerCaps = {};

		CurrentWaveform = DynExpInstr::FunctionGeneratorDefs::WaveformTypes::None;
		CurrentFrequencyInHz = 0;
		CurrentAmplitude = 0;
		CurrentOffset = 0;
		CurrentPhaseInRad = 0;
		CurrentDutyCycle = .5;
		CurrentPulses.Reset();
		CurrentTriggerMode = DynExpInstr::FunctionGeneratorDefs::TriggerDescType::TriggerModeType::Continuous;
		CurrentTriggerEdge = DynExpInstr::FunctionGeneratorDefs::TriggerDescType::TriggerEdgeType::Rise;
		CurrentAutostart = false;
		CurrentPersistParameters = false;

		CurrentStreamSize = 1;
	}

	Util::DynExpErrorCodes::DynExpErrorCodes SignalDesigner::ModuleMainLoop(DynExp::ModuleInstance& Instance)
	{
		auto ModuleData = DynExp::dynamic_ModuleData_cast<SignalDesigner>(Instance.ModuleDataGetter());
		auto InstrData = DynExp::dynamic_InstrumentData_cast<DynExpInstr::FunctionGenerator>(ModuleData->GetFuncGen()->GetInstrumentData());
		ModuleData->CurrentStreamSize = InstrData->GetSampleStream()->GetStreamSizeWrite();

		return Util::DynExpErrorCodes::NoError;
	}

	void SignalDesigner::ResetImpl(dispatch_tag<QModuleBase>)
	{
	}

	std::unique_ptr<DynExp::QModuleWidget> SignalDesigner::MakeUIWidget()
	{
		auto Widget = std::make_unique<SignalDesignerWidget>(*this);

		Connect(Widget->GetUI().CBSource, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &SignalDesigner::OnSourceChanged);
		Connect(Widget->GetUI().CBSignalType, &QComboBox::currentTextChanged, this, &SignalDesigner::OnSignalTypeChanged);
		Connect(Widget->GetUI().SBStreamSize, QOverload<int>::of(&QSpinBox::valueChanged), this, &SignalDesigner::OnStreamSizeChanged);
		Connect(Widget->GetUI().PBResetStreamSize, &QPushButton::clicked, this, &SignalDesigner::OnResetStreamSize);
		Connect(Widget->GetUI().SBFrequencyInHz, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &SignalDesigner::OnFrequencyChanged);
		Connect(Widget->GetUI().SBPhaseInDegree, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &SignalDesigner::OnPhaseChanged);
		Connect(Widget->GetUI().SBAmplitude, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &SignalDesigner::OnAmplitudeChanged);
		Connect(Widget->GetUI().SBYOffset, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &SignalDesigner::OnOffsetChanged);
		Connect(Widget->GetUI().SBDutyCycle, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &SignalDesigner::OnDutyCycleChanged);
		Connect(Widget->GetUI().CBTriggerMode, &QComboBox::currentTextChanged, this, &SignalDesigner::OnTriggerModeChanged);
		Connect(Widget->GetUI().CBTriggerEdge, &QComboBox::currentTextChanged, this, &SignalDesigner::OnTriggerEdgeChanged);
		Connect(Widget->GetUI().CBAutostart, &QCheckBox::stateChanged, this, &SignalDesigner::OnAutostartChanged);
		Connect(Widget->GetUI().BPersist, &QPushButton::clicked, this, &SignalDesigner::OnPersistParametersClicked);
		Connect(Widget->GetUI().BStart, &QPushButton::clicked, this, &SignalDesigner::OnStart);
		Connect(Widget->GetUI().BStop, &QPushButton::clicked, this, &SignalDesigner::OnStop);
		Connect(Widget->GetUI().BForce, &QPushButton::clicked, this, &SignalDesigner::OnForceTrigger);

		return Widget;
	}

	void SignalDesigner::UpdateUIChild(const ModuleBase::ModuleDataGetterType& ModuleDataGetter)
	{
		auto Widget = GetWidget<SignalDesignerWidget>();
		auto ModuleData = DynExp::dynamic_ModuleData_cast<SignalDesigner>(ModuleDataGetter());

		if (!Widget->GetUI().CBSource->count())
		{
			const QSignalBlocker CBSourceBlocker(Widget->GetUI().CBSource);

			for (const auto& FuncGenLabel : ModuleData->GetFuncGenLabels())
				Widget->GetUI().CBSource->addItem(QIcon(ModuleData->GetFuncGenIconPath().data()), QString::fromStdString(FuncGenLabel));

			Widget->GetUI().CBSource->setCurrentIndex(0);
			Widget->GetUI().CBSource->setEnabled(Widget->GetUI().CBSource->count() > 1);
		}

		Widget->InitializeUI(ModuleData);

		if (!Widget->GetUI().SBStreamSize->hasFocus() && ModuleData->CurrentStreamSize <= std::numeric_limits<int>::max())
		{
			const QSignalBlocker SBStreamSizeBlocker(Widget->GetUI().SBStreamSize);
			Widget->GetUI().SBStreamSize->setValue(static_cast<int>(ModuleData->CurrentStreamSize));
		}

		// Layout changes involving CurrentWaveform come here.
		Widget->GetUI().LFrequency->setVisible(ModuleData->CurrentWaveform != DynExpInstr::FunctionGeneratorDefs::WaveformTypes::Pulse);
		Widget->GetUI().SBFrequencyInHz->setVisible(ModuleData->CurrentWaveform != DynExpInstr::FunctionGeneratorDefs::WaveformTypes::Pulse);
		Widget->GetUI().LPhaseInDegree->setVisible(ModuleData->GetFuncGen()->IsPhaseAdjustable() &&
			ModuleData->CurrentWaveform != DynExpInstr::FunctionGeneratorDefs::WaveformTypes::Pulse);
		Widget->GetUI().SBPhaseInDegree->setVisible(ModuleData->GetFuncGen()->IsPhaseAdjustable() &&
			ModuleData->CurrentWaveform != DynExpInstr::FunctionGeneratorDefs::WaveformTypes::Pulse);
		Widget->GetUI().LAmplitude->setVisible(ModuleData->GetFuncGen()->GetValueUnit() != DynExpInstr::DataStreamInstrumentData::UnitType::LogicLevel &&
			ModuleData->CurrentWaveform != DynExpInstr::FunctionGeneratorDefs::WaveformTypes::Pulse);
		Widget->GetUI().SBAmplitude->setVisible(ModuleData->GetFuncGen()->GetValueUnit() != DynExpInstr::DataStreamInstrumentData::UnitType::LogicLevel &&
			ModuleData->CurrentWaveform != DynExpInstr::FunctionGeneratorDefs::WaveformTypes::Pulse);
		Widget->GetUI().LDutyCycle->setVisible(ModuleData->CurrentWaveform == DynExpInstr::FunctionGeneratorDefs::WaveformTypes::Rect ||
			ModuleData->CurrentWaveform == DynExpInstr::FunctionGeneratorDefs::WaveformTypes::Ramp);
		Widget->GetUI().SBDutyCycle->setVisible(ModuleData->CurrentWaveform == DynExpInstr::FunctionGeneratorDefs::WaveformTypes::Rect ||
			ModuleData->CurrentWaveform == DynExpInstr::FunctionGeneratorDefs::WaveformTypes::Ramp);
		Widget->GetUI().LDutyCycle->setText(ModuleData->CurrentWaveform == DynExpInstr::FunctionGeneratorDefs::WaveformTypes::Ramp ?
			"Rise/fall ratio" : "Duty cycle");
		Widget->GetUI().TWPulses->setVisible(ModuleData->CurrentWaveform == DynExpInstr::FunctionGeneratorDefs::WaveformTypes::Pulse);

		if (Widget->HavePulsesChanged())
		{
			ModuleData->CurrentPulses = Widget->GetPulses();
			MakeAndEnqueueEvent(this, &SignalDesigner::OnPulsesChanged);
		}
	}

	void SignalDesigner::UpdateWaveform(Util::SynchronizedPointer<SignalDesignerData>& ModuleData, bool Autostart) const
	{
		ModuleData->GetFuncGen()->Clear();

		if (ModuleData->CurrentWaveform == DynExpInstr::FunctionGeneratorDefs::WaveformTypes::Sine)
			ModuleData->GetFuncGen()->SetSineFunction({ ModuleData->CurrentFrequencyInHz,
				ModuleData->CurrentAmplitude, ModuleData->CurrentOffset, ModuleData->CurrentPhaseInRad },
				ModuleData->CurrentPersistParameters, ModuleData->CurrentAutostart || Autostart);
		else if (ModuleData->CurrentWaveform == DynExpInstr::FunctionGeneratorDefs::WaveformTypes::Rect)
			ModuleData->GetFuncGen()->SetRectFunction({ ModuleData->CurrentFrequencyInHz,
				ModuleData->CurrentAmplitude, ModuleData->CurrentOffset, ModuleData->CurrentPhaseInRad, ModuleData->CurrentDutyCycle },
				ModuleData->CurrentPersistParameters, ModuleData->CurrentAutostart || Autostart);
		else if (ModuleData->CurrentWaveform == DynExpInstr::FunctionGeneratorDefs::WaveformTypes::Ramp)
			ModuleData->GetFuncGen()->SetRampFunction({ ModuleData->CurrentFrequencyInHz,
				ModuleData->CurrentAmplitude, ModuleData->CurrentOffset, ModuleData->CurrentPhaseInRad, ModuleData->CurrentDutyCycle },
				ModuleData->CurrentPersistParameters, ModuleData->CurrentAutostart || Autostart);
		else if (ModuleData->CurrentWaveform == DynExpInstr::FunctionGeneratorDefs::WaveformTypes::Pulse)
		{
			ModuleData->CurrentPulses.Offset = ModuleData->CurrentOffset;
			ModuleData->GetFuncGen()->SetPulseFunction(ModuleData->CurrentPulses,
				ModuleData->CurrentPersistParameters, ModuleData->CurrentAutostart || Autostart);
		}
	}

	void SignalDesigner::OnInit(DynExp::ModuleInstance* Instance) const
	{
		auto ModuleParams = DynExp::dynamic_Params_cast<SignalDesigner>(Instance->ParamsGetter());
		auto ModuleData = DynExp::dynamic_ModuleData_cast<SignalDesigner>(Instance->ModuleDataGetter());

		ModuleData->LockFunctionGenerators(Instance, ModuleParams->FunctionGenerator);
	}

	void SignalDesigner::OnExit(DynExp::ModuleInstance* Instance) const
	{
		auto ModuleData = DynExp::dynamic_ModuleData_cast<SignalDesigner>(Instance->ModuleDataGetter());

		ModuleData->UnlockFunctionGenerators(Instance);
	}

	void SignalDesigner::OnSourceChanged(DynExp::ModuleInstance* Instance, int Index) const
	{
		auto ModuleData = DynExp::dynamic_ModuleData_cast<SignalDesigner>(Instance->ModuleDataGetter());

		ModuleData->SetCurrentFuncGenIndex(Index);
	}

	void SignalDesigner::OnSignalTypeChanged(DynExp::ModuleInstance* Instance, QString Text) const
	{
		// TODO: This solution is quick & dirty...
		auto ModuleData = DynExp::dynamic_ModuleData_cast<SignalDesigner>(Instance->ModuleDataGetter());
		if (Text.contains("Sine", Qt::CaseSensitivity::CaseInsensitive))
			ModuleData->CurrentWaveform = DynExpInstr::FunctionGeneratorDefs::WaveformTypes::Sine;
		else if (Text.contains("Ramp", Qt::CaseSensitivity::CaseInsensitive))
			ModuleData->CurrentWaveform = DynExpInstr::FunctionGeneratorDefs::WaveformTypes::Ramp;
		else if (Text.contains("Rect", Qt::CaseSensitivity::CaseInsensitive))
			ModuleData->CurrentWaveform = DynExpInstr::FunctionGeneratorDefs::WaveformTypes::Rect;
		else if (Text.contains("Pulse", Qt::CaseSensitivity::CaseInsensitive))
			ModuleData->CurrentWaveform = DynExpInstr::FunctionGeneratorDefs::WaveformTypes::Pulse;
		else
			ModuleData->CurrentWaveform = DynExpInstr::FunctionGeneratorDefs::WaveformTypes::None;

		UpdateWaveform(ModuleData);
	}

	void SignalDesigner::OnStreamSizeChanged(DynExp::ModuleInstance* Instance, int Value) const
	{
		if (Value < 1)
			return;

		auto ModuleData = DynExp::dynamic_ModuleData_cast<SignalDesigner>(Instance->ModuleDataGetter());
		ModuleData->GetFuncGen()->SetStreamSize(Value);

		UpdateWaveform(ModuleData);
	}

	void SignalDesigner::OnResetStreamSize(DynExp::ModuleInstance* Instance, bool) const
	{
		auto ModuleData = DynExp::dynamic_ModuleData_cast<SignalDesigner>(Instance->ModuleDataGetter());
		ModuleData->GetFuncGen()->ResetStreamSize();

		UpdateWaveform(ModuleData);
	}

	void SignalDesigner::OnFrequencyChanged(DynExp::ModuleInstance* Instance, double Value) const
	{
		auto ModuleData = DynExp::dynamic_ModuleData_cast<SignalDesigner>(Instance->ModuleDataGetter());
		ModuleData->CurrentFrequencyInHz = Value;

		UpdateWaveform(ModuleData);
	}

	void SignalDesigner::OnPhaseChanged(DynExp::ModuleInstance* Instance, double Value) const
	{
		auto ModuleData = DynExp::dynamic_ModuleData_cast<SignalDesigner>(Instance->ModuleDataGetter());
		ModuleData->CurrentPhaseInRad = Value / 180.0 * std::numbers::pi;

		UpdateWaveform(ModuleData);
	}

	void SignalDesigner::OnAmplitudeChanged(DynExp::ModuleInstance* Instance, double Value) const
	{
		auto ModuleData = DynExp::dynamic_ModuleData_cast<SignalDesigner>(Instance->ModuleDataGetter());
		ModuleData->CurrentAmplitude = Value;

		UpdateWaveform(ModuleData);
	}

	void SignalDesigner::OnOffsetChanged(DynExp::ModuleInstance* Instance, double Value) const
	{
		auto ModuleData = DynExp::dynamic_ModuleData_cast<SignalDesigner>(Instance->ModuleDataGetter());
		ModuleData->CurrentOffset = Value;

		UpdateWaveform(ModuleData);
	}

	void SignalDesigner::OnDutyCycleChanged(DynExp::ModuleInstance* Instance, double Value) const
	{
		auto ModuleData = DynExp::dynamic_ModuleData_cast<SignalDesigner>(Instance->ModuleDataGetter());
		ModuleData->CurrentDutyCycle = Value / 100.0;

		UpdateWaveform(ModuleData);
	}

	void SignalDesigner::OnPulsesChanged(DynExp::ModuleInstance* Instance) const
	{
		auto ModuleData = DynExp::dynamic_ModuleData_cast<SignalDesigner>(Instance->ModuleDataGetter());

		UpdateWaveform(ModuleData);
	}

	void SignalDesigner::OnTriggerModeChanged(DynExp::ModuleInstance* Instance, QString Text) const
	{
		// TODO: This solution is quick & dirty...
		auto ModuleData = DynExp::dynamic_ModuleData_cast<SignalDesigner>(Instance->ModuleDataGetter());
		if (Text.contains("Single", Qt::CaseSensitivity::CaseInsensitive))
			ModuleData->CurrentTriggerMode = DynExpInstr::FunctionGeneratorDefs::TriggerDescType::TriggerModeType::ExternSingle;
		else if (Text.contains("Step", Qt::CaseSensitivity::CaseInsensitive))
			ModuleData->CurrentTriggerMode = DynExpInstr::FunctionGeneratorDefs::TriggerDescType::TriggerModeType::ExternStep;
		else if (Text.contains("Manual", Qt::CaseSensitivity::CaseInsensitive))
			ModuleData->CurrentTriggerMode = DynExpInstr::FunctionGeneratorDefs::TriggerDescType::TriggerModeType::Manual;
		else
			ModuleData->CurrentTriggerMode = DynExpInstr::FunctionGeneratorDefs::TriggerDescType::TriggerModeType::Continuous;

		ModuleData->GetFuncGen()->SetTrigger({ ModuleData->CurrentTriggerMode, ModuleData->CurrentTriggerEdge }, ModuleData->CurrentPersistParameters);
	}

	void SignalDesigner::OnTriggerEdgeChanged(DynExp::ModuleInstance* Instance, QString Text) const
	{
		// TODO: This solution is quick & dirty...
		auto ModuleData = DynExp::dynamic_ModuleData_cast<SignalDesigner>(Instance->ModuleDataGetter());
		if (Text.contains("Fall", Qt::CaseSensitivity::CaseInsensitive))
			ModuleData->CurrentTriggerEdge = DynExpInstr::FunctionGeneratorDefs::TriggerDescType::TriggerEdgeType::Fall;
		else
			ModuleData->CurrentTriggerEdge = DynExpInstr::FunctionGeneratorDefs::TriggerDescType::TriggerEdgeType::Rise;

		ModuleData->GetFuncGen()->SetTrigger({ ModuleData->CurrentTriggerMode, ModuleData->CurrentTriggerEdge }, ModuleData->CurrentPersistParameters);
	}

	void SignalDesigner::OnAutostartChanged(DynExp::ModuleInstance* Instance, int Value) const
	{
		auto ModuleData = DynExp::dynamic_ModuleData_cast<SignalDesigner>(Instance->ModuleDataGetter());
		ModuleData->CurrentAutostart = Value == Qt::CheckState::Checked;

		UpdateWaveform(ModuleData);
	}

	void SignalDesigner::OnPersistParametersClicked(DynExp::ModuleInstance* Instance, bool Value) const
	{
		auto ModuleData = DynExp::dynamic_ModuleData_cast<SignalDesigner>(Instance->ModuleDataGetter());
		ModuleData->CurrentPersistParameters = Value;

		UpdateWaveform(ModuleData);
	}

	void SignalDesigner::OnStart(DynExp::ModuleInstance* Instance, bool) const
	{
		auto ModuleData = DynExp::dynamic_ModuleData_cast<SignalDesigner>(Instance->ModuleDataGetter());

		UpdateWaveform(ModuleData, true);
	}

	void SignalDesigner::OnStop(DynExp::ModuleInstance* Instance, bool) const
	{
		auto ModuleData = DynExp::dynamic_ModuleData_cast<SignalDesigner>(Instance->ModuleDataGetter());

		ModuleData->GetFuncGen()->Stop();
	}

	void SignalDesigner::OnForceTrigger(DynExp::ModuleInstance* Instance, bool) const
	{
		auto ModuleData = DynExp::dynamic_ModuleData_cast<SignalDesigner>(Instance->ModuleDataGetter());

		ModuleData->GetFuncGen()->ForceTrigger();
	}
}