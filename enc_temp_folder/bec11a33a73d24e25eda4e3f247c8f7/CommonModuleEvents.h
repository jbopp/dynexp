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
		FinishedEvent() = default;
		virtual ~FinishedEvent() {}

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
		 * @brief Constructs an @p SetFilenameEvent event.
		 * @param Filename @copybrief #Filename
		*/
		SetFilenameEvent(const std::string& Filename) : Filename(Filename) {}
		virtual ~SetFilenameEvent() {}

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
		StartEvent() = default;
		virtual ~StartEvent() {}

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
		StopEvent() = default;
		virtual ~StopEvent() {}

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
		TriggerEvent() = default;
		virtual ~TriggerEvent() {}

	private:
		/**
		 * @copydoc DynExp::InterModuleEvent::InvokeWithParamsChild
		*/
		virtual void InvokeWithParamsChild(DynExp::ModuleInstance& Instance, EventListenersType::EventFunctionType EventFunc) const override;
	};
}