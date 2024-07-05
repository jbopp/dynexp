// This file is part of DynExp.

/**
 * @file InputPortReader.h
 * @brief Implementation of a module to read single values from input port instruments.
*/

#pragma once

#include "stdafx.h"
#include "DynExpCore.h"
#include "../MetaInstruments/InputPort.h"

#include <QWidget>
#include "ui_InputPortReader.h"

namespace DynExpModule
{
	class InputPortReader;

	class InputPortReaderWidget : public DynExp::QModuleWidget
	{
		Q_OBJECT

	public:
		InputPortReaderWidget(InputPortReader& Owner, QModuleWidget* parent = nullptr);
		~InputPortReaderWidget() = default;

		Ui::InputPortReader ui;
	};

	class InputPortReaderData : public DynExp::QModuleDataBase
	{
	public:
		InputPortReaderData() { Init(); }
		virtual ~InputPortReaderData() = default;

		DynExp::LinkedObjectWrapperContainer<DynExpInstr::InputPort> InputPort;

		bool UIInitialized;
		bool IsDigitalPort;
		double Value;

	private:
		void ResetImpl(dispatch_tag<QModuleDataBase>) override final;
		virtual void ResetImpl(dispatch_tag<InputPortReaderData>) {};

		void Init();
	};

	class InputPortReaderParams : public DynExp::QModuleParamsBase
	{
	public:
		InputPortReaderParams(DynExp::ItemIDType ID, const DynExp::DynExpCore& Core) : QModuleParamsBase(ID, Core) {}
		virtual ~InputPortReaderParams() = default;

		virtual const char* GetParamClassTag() const noexcept override { return "InputPortReaderParams"; }

		Param<DynExp::ObjectLink<DynExpInstr::InputPort>> InputPort = { *this, GetCore().GetInstrumentManager(),
			"InputPort", "Input port", "Underlying input port to be controlled by this module", DynExpUI::Icons::Instrument };

	private:
		void ConfigureParamsImpl(dispatch_tag<QModuleParamsBase>) override final {}
	};

	class InputPortReaderConfigurator : public DynExp::QModuleConfiguratorBase
	{
	public:
		using ObjectType = InputPortReader;
		using ParamsType = InputPortReaderParams;

		InputPortReaderConfigurator() = default;
		virtual ~InputPortReaderConfigurator() = default;

	private:
		virtual DynExp::ParamsBasePtrType MakeParams(DynExp::ItemIDType ID, const DynExp::DynExpCore& Core) const override final { return DynExp::MakeParams<InputPortReaderConfigurator>(ID, Core); }
	};

	class InputPortReader : public DynExp::QModuleBase
	{
	public:
		using ParamsType = InputPortReaderParams;
		using ConfigType = InputPortReaderConfigurator;
		using ModuleDataType = InputPortReaderData;

		constexpr static auto Name() noexcept { return "Input Port Reader"; }
		constexpr static auto Category() noexcept { return "I/O"; }

		InputPortReader(const std::thread::id OwnerThreadID, DynExp::ParamsBasePtrType&& Params)
			: QModuleBase(OwnerThreadID, std::move(Params)) {}
		virtual ~InputPortReader() = default;

		virtual std::string GetName() const override { return Name(); }
		virtual std::string GetCategory() const override { return Category(); }

		std::chrono::milliseconds GetMainLoopDelay() const override final { return std::chrono::milliseconds(50); }

	private:
		Util::DynExpErrorCodes::DynExpErrorCodes ModuleMainLoop(DynExp::ModuleInstance& Instance) override final;

		void ResetImpl(dispatch_tag<QModuleBase>) override final;

		std::unique_ptr<DynExp::QModuleWidget> MakeUIWidget() override final;
		void UpdateUIChild(const ModuleBase::ModuleDataGetterType& ModuleDataGetter) override final;

		// Events, run in module thread
		void OnInit(DynExp::ModuleInstance* Instance) const override final;
		void OnExit(DynExp::ModuleInstance* Instance) const override final;

		size_t NumFailedUpdateAttempts = 0;
	};
}