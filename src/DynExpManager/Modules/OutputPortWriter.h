// This file is part of DynExp.

/**
 * @file OutputPortWriter.h
 * @brief Implementation of a module to write single values to output port instruments.
*/

#pragma once

#include "stdafx.h"
#include "DynExpCore.h"
#include "../MetaInstruments/OutputPort.h"

#include <QWidget>
#include "ui_OutputPortWriter.h"

namespace DynExpModule
{
	class OutputPortWriter;

	class OutputPortWriterWidget : public DynExp::QModuleWidget
	{
		Q_OBJECT

	public:
		OutputPortWriterWidget(OutputPortWriter& Owner, QModuleWidget* parent = nullptr);
		~OutputPortWriterWidget() = default;

		Ui::OutputPortWriter ui;
	};

	class OutputPortWriterData : public DynExp::QModuleDataBase
	{
	public:
		OutputPortWriterData() { Init(); }
		virtual ~OutputPortWriterData() = default;

		DynExp::LinkedObjectWrapperContainer<DynExpInstr::OutputPort> OutputPort;

		bool UIInitialized;
		bool IsDigitalPort;
		bool ValueChanged;
		double MinAllowedValue;
		double MaxAllowedValue;
		double Value;

	private:
		void ResetImpl(dispatch_tag<QModuleDataBase>) override final;
		virtual void ResetImpl(dispatch_tag<OutputPortWriterData>) {};

		void Init();
	};

	class OutputPortWriterParams : public DynExp::QModuleParamsBase
	{
	public:
		OutputPortWriterParams(DynExp::ItemIDType ID, const DynExp::DynExpCore& Core) : QModuleParamsBase(ID, Core) {}
		virtual ~OutputPortWriterParams() = default;

		virtual const char* GetParamClassTag() const noexcept override { return "OutputPortWriterParams"; }

		Param<DynExp::ObjectLink<DynExpInstr::OutputPort>> OutputPort = { *this, GetCore().GetInstrumentManager(),
			"OutputPort", "Output port", "Underlying output port to be controlled by this module", DynExpUI::Icons::Instrument };

	private:
		void ConfigureParamsImpl(dispatch_tag<QModuleParamsBase>) override final {}
	};

	class OutputPortWriterConfigurator : public DynExp::QModuleConfiguratorBase
	{
	public:
		using ObjectType = OutputPortWriter;
		using ParamsType = OutputPortWriterParams;

		OutputPortWriterConfigurator() = default;
		virtual ~OutputPortWriterConfigurator() = default;

	private:
		virtual DynExp::ParamsBasePtrType MakeParams(DynExp::ItemIDType ID, const DynExp::DynExpCore& Core) const override final { return DynExp::MakeParams<OutputPortWriterConfigurator>(ID, Core); }
	};

	class OutputPortWriter : public DynExp::QModuleBase
	{
	public:
		using ParamsType = OutputPortWriterParams;
		using ConfigType = OutputPortWriterConfigurator;
		using ModuleDataType = OutputPortWriterData;

		constexpr static auto Name() noexcept { return "Output Port Writer"; }
		constexpr static auto Category() noexcept { return "I/O"; }

		OutputPortWriter(const std::thread::id OwnerThreadID, DynExp::ParamsBasePtrType&& Params)
			: QModuleBase(OwnerThreadID, std::move(Params)) {}
		virtual ~OutputPortWriter() = default;

		virtual std::string GetName() const override { return Name(); }
		virtual std::string GetCategory() const override { return Category(); }

		std::chrono::milliseconds GetMainLoopDelay() const override final { return std::chrono::milliseconds(50); }

	private:
		static constexpr auto DialControlValueDivider = 100;

		Util::DynExpErrorCodes::DynExpErrorCodes ModuleMainLoop(DynExp::ModuleInstance& Instance) override final;

		void ResetImpl(dispatch_tag<QModuleBase>) override final;

		std::unique_ptr<DynExp::QModuleWidget> MakeUIWidget() override final;
		void UpdateUIChild(const ModuleBase::ModuleDataGetterType& ModuleDataGetter) override final;

		// Events, run in module thread
		void OnInit(DynExp::ModuleInstance* Instance) const override final;
		void OnExit(DynExp::ModuleInstance* Instance) const override final;
		void OnValueChanged(DynExp::ModuleInstance* Instance, int Value) const;
		void OnStateButtonClicked(DynExp::ModuleInstance* Instance, bool Checked) const;
	};
}