// This file is part of DynExp.

/**
 * @file SpectrumViewerEvents.h
 * @brief Events for inter-module communication handled by the DynExpModule::SpectrumViewer::SpectrumViewer
 * module.
*/

#pragma once

#include "stdafx.h"
#include "Module.h"

namespace DynExpModule::SpectrumViewer
{
	class PauseSpectrumRecordingEvent : public DynExp::InterModuleEvent<PauseSpectrumRecordingEvent>
	{
	public:
		PauseSpectrumRecordingEvent() = default;
		PauseSpectrumRecordingEvent(const PauseSpectrumRecordingEvent& Other, DynExp::ItemIDType CommunicatorID) : InterModuleEvent(Other, CommunicatorID) {}
		virtual ~PauseSpectrumRecordingEvent() {}

	private:
		virtual void InvokeWithParamsChild(DynExp::ModuleInstance& Instance, EventListenersType::EventFunctionType EventFunc) const override;
	};

	class ResumeSpectrumRecordingEvent : public DynExp::InterModuleEvent<ResumeSpectrumRecordingEvent>
	{
	public:
		ResumeSpectrumRecordingEvent() = default;
		ResumeSpectrumRecordingEvent(const ResumeSpectrumRecordingEvent& Other, DynExp::ItemIDType CommunicatorID) : InterModuleEvent(Other, CommunicatorID) {}
		virtual ~ResumeSpectrumRecordingEvent() {}

	private:
		virtual void InvokeWithParamsChild(DynExp::ModuleInstance& Instance, EventListenersType::EventFunctionType EventFunc) const override;
	};

	class SetSilentModeEvent : public DynExp::InterModuleEvent<SetSilentModeEvent, bool>
	{
	public:
		SetSilentModeEvent(bool Enable) : Enable(Enable) {}
		SetSilentModeEvent(const SetSilentModeEvent& Other, DynExp::ItemIDType CommunicatorID) : InterModuleEvent(Other, CommunicatorID), Enable(Other.Enable) {}
		virtual ~SetSilentModeEvent() {}

	private:
		virtual void InvokeWithParamsChild(DynExp::ModuleInstance& Instance, EventListenersType::EventFunctionType EventFunc) const override;

		const bool Enable;
	};
}