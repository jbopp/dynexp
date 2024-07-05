// This file is part of DynExp.

/**
 * @file SignalDesigner.h
 * @brief Implementation of a module to design waveforms and to store them in data streams of
 * function generator instruments.
*/

#pragma once

#include "stdafx.h"
#include "DynExpCore.h"
#include "../MetaInstruments/FunctionGenerator.h"

#include <QWidget>
#include "ui_SignalDesigner.h"

namespace DynExpModule
{
	class SignalDesigner;
	class SignalDesignerData;

	class SignalDesignerWidget : public DynExp::QModuleWidget
	{
		Q_OBJECT

	public:
		SignalDesignerWidget(SignalDesigner& Owner, QModuleWidget* parent = nullptr);
		~SignalDesignerWidget() = default;

		bool AllowResize() const noexcept override final { return true; }

		const auto& GetUI() const noexcept { return ui; }
		bool HavePulsesChanged() noexcept;
		auto GetPulses() const { return Pulses; }

		void InitializeUI(Util::SynchronizedPointer<SignalDesignerData>& ModuleData);

	private:
		Ui::SignalDesigner ui;

		QMenu* PulsesContextMenu;
		QAction* AddPulseAction;
		QAction* RemovePulseAction;
		QAction* ClearPulsesAction;

		DynExpInstr::FunctionGeneratorDefs::PulsesDescType Pulses;
		bool PulsesChanged = false;

	private slots:
		void OnPulsesContextMenuRequested(const QPoint& Position);
		void OnAddPulse(bool);
		void OnRemovePulse(bool);
		void OnClearPulses(bool);
		void OnPulsesChanged(QTableWidgetItem*);
	};

	class SignalDesignerData : public DynExp::QModuleDataBase
	{
	public:
		SignalDesignerData() { Init(); }
		virtual ~SignalDesignerData() = default;

		void LockFunctionGenerators(DynExp::ModuleInstance* Instance, const DynExp::ParamsBase::ListParam<DynExp::ObjectLink<DynExpInstr::FunctionGenerator>>& FuncGenParam) { Instance->LockObject(FuncGenParam, FunctionGenerator); }
		void UnlockFunctionGenerators(DynExp::ModuleInstance* Instance) { Instance->UnlockObject(FunctionGenerator); }

		void SetCurrentFuncGenIndex(int Index);
		auto& GetFuncGen() { return FunctionGenerator[CurrentFuncGenIndex]; }
		const auto& GetFuncGenLabels() const noexcept { return FunctionGenerator.GetLabels(); }
		std::string_view GetFuncGenIconPath() const { return FunctionGenerator.GetIconPath(); }

		bool IsUIInitialized() const noexcept { return UIInitialized; }
		void SetUIInitialized() noexcept { UIInitialized = true; }

		DynExpInstr::FunctionGeneratorDefs::FunctionDescType MinFuncDesc;
		DynExpInstr::FunctionGeneratorDefs::FunctionDescType MaxFuncDesc;
		DynExpInstr::FunctionGeneratorDefs::FunctionDescType DefaultFuncDesc;
		Util::FeatureTester<DynExpInstr::FunctionGenerator::WaveformCapsType> WaveformCaps;
		Util::FeatureTester<DynExpInstr::FunctionGenerator::TriggerCapsType> TriggerCaps;

		DynExpInstr::FunctionGeneratorDefs::WaveformTypes CurrentWaveform;
		double CurrentFrequencyInHz;
		double CurrentAmplitude;
		double CurrentOffset;
		double CurrentPhaseInRad;
		double CurrentDutyCycle;
		DynExpInstr::FunctionGeneratorDefs::PulsesDescType CurrentPulses;
		DynExpInstr::FunctionGeneratorDefs::TriggerDescType::TriggerModeType CurrentTriggerMode;
		DynExpInstr::FunctionGeneratorDefs::TriggerDescType::TriggerEdgeType CurrentTriggerEdge;
		bool CurrentAutostart;
		bool CurrentPersistParameters;

		size_t CurrentStreamSize;

	private:
		void ResetImpl(dispatch_tag<QModuleDataBase>) override final;
		virtual void ResetImpl(dispatch_tag<SignalDesignerData>) {};

		void Init();

		bool UIInitialized;
		int CurrentFuncGenIndex;

		DynExp::LinkedObjectWrapperContainerList<DynExpInstr::FunctionGenerator> FunctionGenerator;
	};

	class SignalDesignerParams : public DynExp::QModuleParamsBase
	{
	public:
		SignalDesignerParams(DynExp::ItemIDType ID, const DynExp::DynExpCore& Core) : QModuleParamsBase(ID, Core) {}
		virtual ~SignalDesignerParams() = default;

		virtual const char* GetParamClassTag() const noexcept override { return "SignalDesignerParams"; }

		ListParam<DynExp::ObjectLink<DynExpInstr::FunctionGenerator>> FunctionGenerator = { *this, GetCore().GetInstrumentManager(),
			"FunctionGenerator", "Function generator(s)", "Underlying function generator instrument(s) to be used to generate the designed waveform(s)", DynExpUI::Icons::Instrument };

	private:
		void ConfigureParamsImpl(dispatch_tag<QModuleParamsBase>) override final {}
	};

	class SignalDesignerConfigurator : public DynExp::QModuleConfiguratorBase
	{
	public:
		using ObjectType = SignalDesigner;
		using ParamsType = SignalDesignerParams;

		SignalDesignerConfigurator() = default;
		virtual ~SignalDesignerConfigurator() = default;

	private:
		virtual DynExp::ParamsBasePtrType MakeParams(DynExp::ItemIDType ID, const DynExp::DynExpCore& Core) const override final { return DynExp::MakeParams<SignalDesignerConfigurator>(ID, Core); }
	};

	class SignalDesigner : public DynExp::QModuleBase
	{
	public:
		using ParamsType = SignalDesignerParams;
		using ConfigType = SignalDesignerConfigurator;
		using ModuleDataType = SignalDesignerData;

		constexpr static auto Name() noexcept { return "Signal Designer"; }
		constexpr static auto Category() noexcept { return "I/O"; }

		SignalDesigner(const std::thread::id OwnerThreadID, DynExp::ParamsBasePtrType&& Params)
			: QModuleBase(OwnerThreadID, std::move(Params)) {}
		virtual ~SignalDesigner() = default;

		virtual std::string GetName() const override { return Name(); }
		virtual std::string GetCategory() const override { return Category(); }

	private:
		Util::DynExpErrorCodes::DynExpErrorCodes ModuleMainLoop(DynExp::ModuleInstance& Instance) override final;

		void ResetImpl(dispatch_tag<QModuleBase>) override final;

		std::unique_ptr<DynExp::QModuleWidget> MakeUIWidget() override final;
		void UpdateUIChild(const ModuleBase::ModuleDataGetterType& ModuleDataGetter) override final;

		void UpdateWaveform(Util::SynchronizedPointer<SignalDesignerData>& ModuleData, bool Autostart = false) const;

		// Events, run in module thread
		void OnInit(DynExp::ModuleInstance* Instance) const override final;
		void OnExit(DynExp::ModuleInstance* Instance) const override final;
		void OnSourceChanged(DynExp::ModuleInstance* Instance, int Index) const;
		void OnSignalTypeChanged(DynExp::ModuleInstance* Instance, QString Text) const;
		void OnStreamSizeChanged(DynExp::ModuleInstance* Instance, int Value) const;
		void OnResetStreamSize(DynExp::ModuleInstance* Instance, bool) const;
		void OnFrequencyChanged(DynExp::ModuleInstance* Instance, double Value) const;
		void OnPhaseChanged(DynExp::ModuleInstance* Instance, double Value) const;
		void OnAmplitudeChanged(DynExp::ModuleInstance* Instance, double Value) const;
		void OnOffsetChanged(DynExp::ModuleInstance* Instance, double Value) const;
		void OnDutyCycleChanged(DynExp::ModuleInstance* Instance, double Value) const;
		void OnPulsesChanged(DynExp::ModuleInstance* Instance) const;
		void OnTriggerModeChanged(DynExp::ModuleInstance* Instance, QString Text) const;
		void OnTriggerEdgeChanged(DynExp::ModuleInstance* Instance, QString Text) const;
		void OnAutostartChanged(DynExp::ModuleInstance* Instance, int Value) const;
		void OnPersistParametersClicked(DynExp::ModuleInstance* Instance, bool Value) const;
		void OnStart(DynExp::ModuleInstance* Instance, bool) const;
		void OnStop(DynExp::ModuleInstance* Instance, bool) const;
		void OnForceTrigger(DynExp::ModuleInstance* Instance, bool) const;
	};
}