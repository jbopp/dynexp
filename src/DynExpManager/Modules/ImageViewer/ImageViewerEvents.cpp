// This file is part of DynExp.

#include "stdafx.h"
#include "ImageViewerEvents.h"

namespace DynExpModule::ImageViewer
{
	void PauseImageCapturingEvent::InvokeWithParamsChild(DynExp::ModuleInstance& Instance, EventListenersType::EventFunctionType EventFunc) const
	{
		EventFunc(&Instance, ResetImageTransformation);
	}

	void ImageCapturingPausedEvent::InvokeWithParamsChild(DynExp::ModuleInstance& Instance, EventListenersType::EventFunctionType EventFunc) const
	{
		EventFunc(&Instance);
	}

	void ResumeImageCapturingEvent::InvokeWithParamsChild(DynExp::ModuleInstance& Instance, EventListenersType::EventFunctionType EventFunc) const
	{
		EventFunc(&Instance);
	}

	void ImageCapturingResumedEvent::InvokeWithParamsChild(DynExp::ModuleInstance& Instance, EventListenersType::EventFunctionType EventFunc) const
	{
		EventFunc(&Instance);
	}

	void AutofocusEvent::InvokeWithParamsChild(DynExp::ModuleInstance& Instance, EventListenersType::EventFunctionType EventFunc) const
	{
		EventFunc(&Instance, ResetImageTransformation);
	}

	void FinishedAutofocusEvent::InvokeWithParamsChild(DynExp::ModuleInstance& Instance, EventListenersType::EventFunctionType EventFunc) const
	{
		EventFunc(&Instance, Success, Voltage);
	}
}