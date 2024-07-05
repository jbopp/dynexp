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
	class RecordSpectrumEvent : public DynExp::InterModuleEvent<RecordSpectrumEvent, std::string>
	{
	public:
		RecordSpectrumEvent(std::string SaveDataFilename) : SaveDataFilename(SaveDataFilename) {}
		virtual ~RecordSpectrumEvent() {}

	private:
		virtual void InvokeWithParamsChild(DynExp::ModuleInstance& Instance, EventListenersType::EventFunctionType EventFunc) const override;

		const std::string SaveDataFilename;
	};

	class SpectrumFinishedRecordingEvent : public DynExp::InterModuleEvent<SpectrumFinishedRecordingEvent>
	{
	public:
		SpectrumFinishedRecordingEvent() = default;
		virtual ~SpectrumFinishedRecordingEvent() {}

	private:
		virtual void InvokeWithParamsChild(DynExp::ModuleInstance& Instance, EventListenersType::EventFunctionType EventFunc) const override;
	};

	class PauseSpectrumRecordingEvent : public DynExp::InterModuleEvent<PauseSpectrumRecordingEvent>
	{
	public:
		PauseSpectrumRecordingEvent() = default;
		virtual ~PauseSpectrumRecordingEvent() {}

	private:
		virtual void InvokeWithParamsChild(DynExp::ModuleInstance& Instance, EventListenersType::EventFunctionType EventFunc) const override;
	};

	class ResumeSpectrumRecordingEvent : public DynExp::InterModuleEvent<ResumeSpectrumRecordingEvent>
	{
	public:
		ResumeSpectrumRecordingEvent() = default;
		virtual ~ResumeSpectrumRecordingEvent() {}

	private:
		virtual void InvokeWithParamsChild(DynExp::ModuleInstance& Instance, EventListenersType::EventFunctionType EventFunc) const override;
	};

	class SetSilentModeEvent : public DynExp::InterModuleEvent<RecordSpectrumEvent, bool>
	{
	public:
		SetSilentModeEvent(bool Enable) : Enable(Enable) {}
		virtual ~SetSilentModeEvent() {}

	private:
		virtual void InvokeWithParamsChild(DynExp::ModuleInstance& Instance, EventListenersType::EventFunctionType EventFunc) const override;

		const bool Enable;
	};
}