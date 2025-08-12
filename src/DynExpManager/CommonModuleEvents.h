// This file is part of DynExp.

/**
 * @file CommonModuleEvents.h
 * @brief Provides common events for inter-module communication.
 * Also refer to DynExp::InterModuleEventBase.
*/

#pragma once

#include "stdafx.h"
#include "Module.h"

namespace DynExpModule
{
	/**
	 * @brief This event signals that an action (like a measurement) started
	 * by a @p TriggerEvent has been completed.
	*/
	class FinishedEvent : public DynExp::InterModuleEvent<FinishedEvent>
	{
	public:
		/**
		 * @copydoc DynExp::InterModuleEventBase::InterModuleEventBase
		*/
		FinishedEvent() = default;

		/**
		 * @copydoc DynExp::InterModuleEventBase::InterModuleEventBase(const InterModuleEventBase&, ItemIDType)
		*/
		FinishedEvent(const FinishedEvent& Other, DynExp::ItemIDType CommunicatorID)
			: InterModuleEvent(Other, CommunicatorID) {}

		virtual ~FinishedEvent() {}

		virtual std::string GetName() const override { return "Finished"; }

	private:
		/**
		 * @copydoc DynExp::InterModuleEvent::InvokeWithParamsChild
		*/
		virtual void InvokeWithParamsChild(DynExp::ModuleInstance& Instance, EventListenersType::EventFunctionType EventFunc) const override;
	};

	/**
	 * @brief This event tells the receiver where to store e.g. acquired data.
	*/
	class SetFilenameEvent : public DynExp::InterModuleEvent<SetFilenameEvent, std::string>
	{
	public:
		/**
		 * @brief Constructs a @p SetFilenameEvent event.
		 * @param Filename @copybrief #Filename
		*/
		SetFilenameEvent(const std::string& Filename = "Unknown.dat") : Filename(Filename) {}

		/**
		 * @copydoc DynExp::InterModuleEventBase::InterModuleEventBase(const InterModuleEventBase&, ItemIDType)
		*/
		SetFilenameEvent(const SetFilenameEvent& Other, DynExp::ItemIDType CommunicatorID)
			: InterModuleEvent(Other, CommunicatorID), Filename(Other.Filename) {}

		virtual ~SetFilenameEvent() {}

		virtual std::string GetName() const override { return "Set filename to Unknown.dat"; }

	private:
		/**
		 * @copydoc DynExp::InterModuleEvent::InvokeWithParamsChild
		*/
		virtual void InvokeWithParamsChild(DynExp::ModuleInstance& Instance, EventListenersType::EventFunctionType EventFunc) const override;

		const std::string Filename;		//!< Filename where to store data.
	};

	/**
	 * @brief This event is intended to make the receiver prepare an action
	 * (like a measurement) that is started when the receiver receives a
	 * subsequent trigger event (@p TriggerEvent).
	*/
	class StartEvent : public DynExp::InterModuleEvent<StartEvent>
	{
	public:
		/**
		 * @copydoc DynExp::InterModuleEventBase::InterModuleEventBase
		*/
		StartEvent() = default;

		/**
		 * @copydoc DynExp::InterModuleEventBase::InterModuleEventBase(const InterModuleEventBase&, ItemIDType)
		*/
		StartEvent(const StartEvent& Other, DynExp::ItemIDType CommunicatorID)
			: InterModuleEvent(Other, CommunicatorID) {}

		virtual ~StartEvent() {}

		virtual std::string GetName() const override { return "Start"; }

	private:
		/**
		 * @copydoc DynExp::InterModuleEvent::InvokeWithParamsChild
		*/
		virtual void InvokeWithParamsChild(DynExp::ModuleInstance& Instance, EventListenersType::EventFunctionType EventFunc) const override;
	};

	/**
	 * @brief This event is intended to make the receiver stop an action
	 * (like a measurement).
	*/
	class StopEvent : public DynExp::InterModuleEvent<StopEvent>
	{
	public:
		/**
		 * @copydoc DynExp::InterModuleEventBase::InterModuleEventBase
		*/
		StopEvent() = default;

		/**
		 * @copydoc DynExp::InterModuleEventBase::InterModuleEventBase(const InterModuleEventBase&, ItemIDType)
		*/
		StopEvent(const StopEvent& Other, DynExp::ItemIDType CommunicatorID)
			: InterModuleEvent(Other, CommunicatorID) {}

		virtual ~StopEvent() {}

		virtual std::string GetName() const override { return "Stop"; }

	private:
		/**
		 * @copydoc DynExp::InterModuleEvent::InvokeWithParamsChild
		*/
		virtual void InvokeWithParamsChild(DynExp::ModuleInstance& Instance, EventListenersType::EventFunctionType EventFunc) const override;
	};

	/**
	 * @brief This event is intended to make the receiver start an action
	 * (like a measurement) after it received a start event (@p StartEvent).
	*/
	class TriggerEvent : public DynExp::InterModuleEvent<TriggerEvent>
	{
	public:
		/**
		 * @copydoc DynExp::InterModuleEventBase::InterModuleEventBase
		*/
		TriggerEvent() = default;

		/**
		 * @copydoc DynExp::InterModuleEventBase::InterModuleEventBase(const InterModuleEventBase&, ItemIDType)
		*/
		TriggerEvent(const TriggerEvent& Other, DynExp::ItemIDType CommunicatorID)
			: InterModuleEvent(Other, CommunicatorID) {}

		virtual ~TriggerEvent() {}

		virtual std::string GetName() const override { return "Trigger"; }

	private:
		/**
		 * @copydoc DynExp::InterModuleEvent::InvokeWithParamsChild
		*/
		virtual void InvokeWithParamsChild(DynExp::ModuleInstance& Instance, EventListenersType::EventFunctionType EventFunc) const override;
	};
}