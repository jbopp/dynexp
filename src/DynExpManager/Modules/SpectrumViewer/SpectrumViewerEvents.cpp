// This file is part of DynExp.

#include "stdafx.h"
#include "SpectrumViewerEvents.h"

namespace DynExpModule::SpectrumViewer
{
	void RecordSpectrumEvent::InvokeWithParamsChild(DynExp::ModuleInstance& Instance, EventListenersType::EventFunctionType EventFunc) const
	{
		EventFunc(&Instance, SaveDataFilename);
	}

	void SpectrumFinishedRecordingEvent::InvokeWithParamsChild(DynExp::ModuleInstance& Instance, EventListenersType::EventFunctionType EventFunc) const
	{
		EventFunc(&Instance);
	}

	void PauseSpectrumRecordingEvent::InvokeWithParamsChild(DynExp::ModuleInstance& Instance, EventListenersType::EventFunctionType EventFunc) const
	{
		EventFunc(&Instance);
	}

	void ResumeSpectrumRecordingEvent::InvokeWithParamsChild(DynExp::ModuleInstance& Instance, EventListenersType::EventFunctionType EventFunc) const
	{
		EventFunc(&Instance);
	}

	void SetSilentModeEvent::InvokeWithParamsChild(DynExp::ModuleInstance& Instance, EventListenersType::EventFunctionType EventFunc) const
	{
		EventFunc(&Instance, Enable);
	}
}