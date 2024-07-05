// This file is part of DynExp.

#include "stdafx.h"
#include "Managers.h"
#include "DynExpManager.h"

namespace DynExp
{
	std::thread::id CommonResourceManagerBase::GetOwnerThreadID(const DynExpCore& Core) noexcept
	{
		return Core.GetOwnerThreadID();
	}

	bool HardwareAdapterManager::AllConnected() const
	{
		const auto NotConnectedResource = std::find_if(cbegin(), cend(), [](const auto& i) {
			const auto ExceptionPtr = i.second.ResourcePointer->GetException();
			if (ExceptionPtr)
				std::rethrow_exception(ExceptionPtr);

			return !i.second.ResourcePointer->IsConnected();
		});

		// No not-connected resource found, std::find_if() returns iterator to end
		return NotConnectedResource == cend();
	}

	void HardwareAdapterManager::StartupChild(FunctionToCallWhenObjectStartedType FunctionToCallWhenObjectStarted) const
	{
		// Tries to connect all hardware adapters and does not stop in case of an exception.
		// The first occurring exception is instead stored and rethrown afterwards. All further
		// exceptions are swallowed here. This is not optimal, but still better than having
		// some hardware adapters being stuck in a "connecting" state due to an exception of
		// another hardware adapter.
		std::exception_ptr FirstException;

		std::for_each(cbegin(), cend(), [FunctionToCallWhenObjectStarted, &FirstException](const auto& i) {
			try
			{
				i.second.ResourcePointer->EnsureReadyState(false);

				if (FunctionToCallWhenObjectStarted)
					FunctionToCallWhenObjectStarted(i.second.ResourcePointer.get());
			}
			catch (...)
			{
				if (!FirstException)
					FirstException = std::current_exception();
			}
		});

		if (FirstException)
			std::rethrow_exception(FirstException);
	}

	QDomElement HardwareAdapterManager::MakeXMLConfigHeadNode(QDomDocument& Document) const
	{
		QDomElement ManagerNode = Document.createElement("HardwareAdapters");

		return ManagerNode;
	}

	size_t InstrumentManager::GetNumRunningInstruments() const
	{
		return std::count_if(cbegin(), cend(), [](const auto& i) { return i.second.ResourcePointer->IsRunning(); });
	}

	bool InstrumentManager::AllInitialized() const
	{
		const auto NotInitializedResource = std::find_if(cbegin(), cend(), [](const auto& i) {
			const auto ExceptionPtr = i.second.ResourcePointer->GetException();
			if (ExceptionPtr)
				std::rethrow_exception(ExceptionPtr);

			return !i.second.ResourcePointer->IsInitialized() && i.second.ResourcePointer->GetStartupType() == RunnableObjectParams::StartupType::OnCreation;
		});

		// No not-initialized resource found, std::find_if() returns iterator to end
		return NotInitializedResource == cend();
	}

	void InstrumentManager::TerminateAll() const
	{
		std::for_each(cbegin(), cend(), [](const auto& i) { i.second.ResourcePointer->Terminate(); });
	}

	void InstrumentManager::StartupChild(FunctionToCallWhenObjectStartedType FunctionToCallWhenObjectStarted) const
	{
		std::for_each(cbegin(), cend(), [FunctionToCallWhenObjectStarted](const auto& i) {
			const bool HasBeenStarted = i.second.ResourcePointer->RunIfRunOnCreation();
		
			if (HasBeenStarted && FunctionToCallWhenObjectStarted)
				FunctionToCallWhenObjectStarted(i.second.ResourcePointer.get());
		});
	}

	QDomElement InstrumentManager::MakeXMLConfigHeadNode(QDomDocument& Document) const
	{
		QDomElement ManagerNode = Document.createElement("Instruments");

		return ManagerNode;
	}

	size_t ModuleManager::GetNumRunningModules() const
	{
		return std::count_if(cbegin(), cend(), [](const auto& i) { return i.second.ResourcePointer->IsRunning(); });
	}

	void ModuleManager::TerminateAll() const
	{
		std::for_each(cbegin(), cend(), [](const auto& i) { i.second.ResourcePointer->Terminate(); });
	}

	void ModuleManager::RestoreWindowStatesFromParams() const
	{
		std::for_each(cbegin(), cend(), [](const auto& i) { i.second.ResourcePointer->RestoreWindowStatesFromParams(); });
	}

	void ModuleManager::UpdateParamsFromWindowStates() const
	{
		std::for_each(cbegin(), cend(), [](const auto& i) { i.second.ResourcePointer->UpdateParamsFromWindowStates(); });
	}

	void ModuleManager::StartupChild(FunctionToCallWhenObjectStartedType FunctionToCallWhenObjectStarted) const
	{
		std::for_each(cbegin(), cend(), [FunctionToCallWhenObjectStarted](const auto& i) {
			const bool HasBeenStarted = i.second.ResourcePointer->RunIfRunOnCreation();

			if (HasBeenStarted && FunctionToCallWhenObjectStarted)
				FunctionToCallWhenObjectStarted(i.second.ResourcePointer.get());
		});
	}

	QDomElement ModuleManager::MakeXMLConfigHeadNode(QDomDocument& Document) const
	{
		QDomElement ManagerNode = Document.createElement("Modules");

		return ManagerNode;
	}
}