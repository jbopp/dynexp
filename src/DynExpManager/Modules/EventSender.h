// This file is part of DynExp.

/**
 * @file EventSender.h
 * @brief Implementation of a module to let the user manually issue inter-module events.
*/

#pragma once

#include "stdafx.h"
#include "DynExpCore.h"
#include "../Instruments/InterModuleCommunicator.h"

#include "CommonModuleEvents.h"

#include <QWidget>

namespace Ui
{
	class EventSender;
}

namespace DynExpModule
{
	class EventSender;

	class EventSenderWidget : public DynExp::QModuleWidget
	{
		Q_OBJECT

	public:
		EventSenderWidget(EventSender& Owner, QModuleWidget* parent = nullptr);
		~EventSenderWidget() = default;

		bool AllowResize() const noexcept override final { return true; }

		std::unique_ptr<Ui::EventSender> ui;

		size_t EventID;

	private slots:
		void OnEventDoubleClicked(QListWidgetItem* Item);
	};

	class EventSenderData : public DynExp::QModuleDataBase
	{
	public:
		EventSenderData() { Init(); }
		virtual ~EventSenderData() = default;

		DynExp::LinkedObjectWrapperContainer<DynExpInstr::InterModuleCommunicator> Communicator;

		bool UIInitialized;

		size_t EventID;

	private:
		void ResetImpl(dispatch_tag<QModuleDataBase>) override final;
		virtual void ResetImpl(dispatch_tag<EventSenderData>) {};

		void Init();
	};

	class EventSenderParams : public DynExp::QModuleParamsBase
	{
	public:
		EventSenderParams(DynExp::ItemIDType ID, const DynExp::DynExpCore& Core) : QModuleParamsBase(ID, Core) {}
		virtual ~EventSenderParams() = default;

		virtual const char* GetParamClassTag() const noexcept override { return "EventSenderParams"; }

		Param<DynExp::ObjectLink<DynExpInstr::InterModuleCommunicator>> Communicator = { *this, GetCore().GetInstrumentManager(),
			"InterModuleCommunicator", "Inter-module communicator", "Inter-module communicator to send inter-module events to", DynExpUI::Icons::Instrument };

	private:
		void ConfigureParamsImpl(dispatch_tag<QModuleParamsBase>) override final {}
	};

	class EventSenderConfigurator : public DynExp::QModuleConfiguratorBase
	{
	public:
		using ObjectType = EventSender;
		using ParamsType = EventSenderParams;

		EventSenderConfigurator() = default;
		virtual ~EventSenderConfigurator() = default;

	private:
		virtual DynExp::ParamsBasePtrType MakeParams(DynExp::ItemIDType ID, const DynExp::DynExpCore& Core) const override final { return DynExp::MakeParams<EventSenderConfigurator>(ID, Core); }
	};

	class EventSender : public DynExp::QModuleBase
	{
	public:
		using ParamsType = EventSenderParams;
		using ConfigType = EventSenderConfigurator;
		using ModuleDataType = EventSenderData;

		constexpr static auto Name() noexcept { return "Inter-Module Event Sender"; }
		constexpr static auto Category() noexcept { return "General"; }

		EventSender(const std::thread::id OwnerThreadID, DynExp::ParamsBasePtrType&& Params)
			: QModuleBase(OwnerThreadID, std::move(Params)) {}
		virtual ~EventSender() = default;

		virtual std::string GetName() const override { return Name(); }
		virtual std::string GetCategory() const override { return Category(); }

		bool TreatModuleExceptionsAsWarnings() const override { return false; }

		// Only run main loop in case of an event.
		std::chrono::milliseconds GetMainLoopDelay() const override final { return decltype(ModuleBase::GetMainLoopDelay())::max(); }

	private:
		Util::DynExpErrorCodes::DynExpErrorCodes ModuleMainLoop(DynExp::ModuleInstance& Instance) override final;

		void ResetImpl(dispatch_tag<QModuleBase>) override final;

		std::unique_ptr<DynExp::QModuleWidget> MakeUIWidget() override final;
		void UpdateUIChild(const ModuleBase::ModuleDataGetterType& ModuleDataGetter) override final;

		// Events, run in module thread
		void OnInit(DynExp::ModuleInstance* Instance) const override final;
		void OnExit(DynExp::ModuleInstance* Instance) const override final;
		
	};
}