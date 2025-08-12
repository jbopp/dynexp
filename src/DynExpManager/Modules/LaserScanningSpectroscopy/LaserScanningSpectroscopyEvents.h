// This file is part of DynExp.

/**
 * @file PLEEvents.h
 * @brief Events for inter-module communication handled by the DynExpModule::PLE::PLE
 * module.
*/

#pragma once

#include "stdafx.h"
#include "Module.h"
/*
namespace DynExpModule::PLE
{
	class StartCapturingEvent : public DynExp::InterModuleEvent<StartCapturingEvent, std::filesystem::path>
	{
	public:
		StartCapturingEvent(std::filesystem::path Filename) : Filename(Filename) {}
		virtual ~StartCapturingEvent() {}

	private:
		virtual void InvokeWithParamsChild(DynExp::ModuleInstance& Instance, EventListenersType::EventFunctionType EventFunc) const override;

		std::filesystem::path Filename;
	};

	class SetFilenameEvent : public DynExp::InterModuleEvent<SetFilenameEvent>
	{
	public:
		SetFilenameEvent() {}
		virtual ~SetFilenameEvent() {}

	private:
		virtual void InvokeWithParamsChild(DynExp::ModuleInstance& Instance, EventListenersType::EventFunctionType EventFunc) const override;
	};

	class FinishedCapturingEvent : public DynExp::InterModuleEvent<FinishedCapturingEvent>
	{
	public:
		FinishedCapturingEvent() {}
		virtual ~FinishedCapturingEvent() {}

	private:
		virtual void InvokeWithParamsChild(DynExp::ModuleInstance& Instance, EventListenersType::EventFunctionType EventFunc) const override;
	};

	class FinishedPLEEvent : public DynExp::InterModuleEvent<FinishedPLEEvent>
	{
	public:
		FinishedPLEEvent() {}
		virtual ~FinishedPLEEvent() {}

	private:
		virtual void InvokeWithParamsChild(DynExp::ModuleInstance& Instance, EventListenersType::EventFunctionType EventFunc) const override;
	};
}*/