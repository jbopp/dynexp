// This file is part of DynExp.

/**
 * @file ImageViewerEvents.h
 * @brief Events for inter-module communication handled by the DynExpModule::ImageViewer::ImageViewer
 * module.
*/

#pragma once

#include "stdafx.h"
#include "Module.h"

namespace DynExpModule::ImageViewer
{
	class PauseImageCapturingEvent : public DynExp::InterModuleEvent<PauseImageCapturingEvent, bool>
	{
	public:
		PauseImageCapturingEvent(bool ResetImageTransformation = false) : ResetImageTransformation(ResetImageTransformation) {}
		PauseImageCapturingEvent(const PauseImageCapturingEvent& Other, DynExp::ItemIDType CommunicatorID) : InterModuleEvent(Other, CommunicatorID), ResetImageTransformation(Other.ResetImageTransformation) {}
		virtual ~PauseImageCapturingEvent() {}

	private:
		virtual void InvokeWithParamsChild(DynExp::ModuleInstance& Instance, EventListenersType::EventFunctionType EventFunc) const override;
		
		const bool ResetImageTransformation;
	};

	class ImageCapturingPausedEvent : public DynExp::InterModuleEvent<ImageCapturingPausedEvent>
	{
	public:
		ImageCapturingPausedEvent() = default;
		ImageCapturingPausedEvent(const ImageCapturingPausedEvent& Other, DynExp::ItemIDType CommunicatorID) : InterModuleEvent(Other, CommunicatorID) {}
		virtual ~ImageCapturingPausedEvent() {}

	private:
		virtual void InvokeWithParamsChild(DynExp::ModuleInstance& Instance, EventListenersType::EventFunctionType EventFunc) const override;
	};

	class ResumeImageCapturingEvent : public DynExp::InterModuleEvent<ResumeImageCapturingEvent>
	{
	public:
		ResumeImageCapturingEvent() = default;
		ResumeImageCapturingEvent(const ResumeImageCapturingEvent& Other, DynExp::ItemIDType CommunicatorID) : InterModuleEvent(Other, CommunicatorID) {}
		virtual ~ResumeImageCapturingEvent() {}

	private:
		virtual void InvokeWithParamsChild(DynExp::ModuleInstance& Instance, EventListenersType::EventFunctionType EventFunc) const override;
	};

	class ImageCapturingResumedEvent : public DynExp::InterModuleEvent<ImageCapturingResumedEvent>
	{
	public:
		ImageCapturingResumedEvent() = default;
		ImageCapturingResumedEvent(const ImageCapturingResumedEvent& Other, DynExp::ItemIDType CommunicatorID) : InterModuleEvent(Other, CommunicatorID) {}
		virtual ~ImageCapturingResumedEvent() {}

	private:
		virtual void InvokeWithParamsChild(DynExp::ModuleInstance& Instance, EventListenersType::EventFunctionType EventFunc) const override;
	};

	class AutofocusEvent : public DynExp::InterModuleEvent<AutofocusEvent, bool>
	{
	public:
		AutofocusEvent(bool ResetImageTransformation = false) : ResetImageTransformation(ResetImageTransformation) {}
		AutofocusEvent(const AutofocusEvent& Other, DynExp::ItemIDType CommunicatorID) : InterModuleEvent(Other, CommunicatorID), ResetImageTransformation(Other.ResetImageTransformation) {}
		virtual ~AutofocusEvent() {}

	private:
		virtual void InvokeWithParamsChild(DynExp::ModuleInstance& Instance, EventListenersType::EventFunctionType EventFunc) const override;

		const bool ResetImageTransformation;
	};

	class FinishedAutofocusEvent : public DynExp::InterModuleEvent<FinishedAutofocusEvent, bool, double>
	{
	public:
		FinishedAutofocusEvent(bool Success = true, double Voltage = .0) : Success(Success), Voltage(Voltage) {}
		FinishedAutofocusEvent(const FinishedAutofocusEvent& Other, DynExp::ItemIDType CommunicatorID) : InterModuleEvent(Other, CommunicatorID), Success(Other.Success), Voltage(Other.Voltage) {}
		virtual ~FinishedAutofocusEvent() {}

	private:
		virtual void InvokeWithParamsChild(DynExp::ModuleInstance& Instance, EventListenersType::EventFunctionType EventFunc) const override;

		const bool Success;
		const double Voltage;
	};
}