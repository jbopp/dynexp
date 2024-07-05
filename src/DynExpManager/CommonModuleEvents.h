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
	 * @brief This event is intended to make the receiver either directly start
	 * an action (like a measurement) or to prepare the receiver to await a trigger
	 * event (@p TriggerEvent) which starts the action.
	*/
	class StartEvent : public DynExp::InterModuleEvent<StartEvent>
	{
	public:
		StartEvent() = default;
		virtual ~StartEvent() {}

	private:
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
		virtual void InvokeWithParamsChild(DynExp::ModuleInstance& Instance, EventListenersType::EventFunctionType EventFunc) const override;
	};
}