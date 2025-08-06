// This file is part of DynExp.

#include "stdafx.h"
#include "CommonModuleEvents.h"

namespace DynExpModule
{
	void FinishedEvent::InvokeWithParamsChild(DynExp::ModuleInstance& Instance, EventListenersType::EventFunctionType EventFunc) const
	{
		EventFunc(&Instance);
	}

	void SetFilenameEvent::InvokeWithParamsChild(DynExp::ModuleInstance& Instance, EventListenersType::EventFunctionType EventFunc) const
	{
		EventFunc(&Instance, Filename);
	}

	void StartEvent::InvokeWithParamsChild(DynExp::ModuleInstance& Instance, EventListenersType::EventFunctionType EventFunc) const
	{
		EventFunc(&Instance);
	}

	void StopEvent::InvokeWithParamsChild(DynExp::ModuleInstance& Instance, EventListenersType::EventFunctionType EventFunc) const
	{
		EventFunc(&Instance);
	}

	void TriggerEvent::InvokeWithParamsChild(DynExp::ModuleInstance& Instance, EventListenersType::EventFunctionType EventFunc) const
	{
		EventFunc(&Instance);
	}
}