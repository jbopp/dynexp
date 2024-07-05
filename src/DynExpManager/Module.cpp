// This file is part of DynExp.

#include "stdafx.h"
#include "moc_Module.cpp"
#include "Module.h"
#include "DynExpManager.h"

namespace DynExp
{
	int ModuleThreadMain(ModuleInstance Instance, ModuleBase* const Module)
	{
		bool IsExiting = false;
		std::chrono::time_point<std::chrono::system_clock> LastMainLoopExecution;	// LastUpdate.time_since_epoch() == 0 now.
		auto ReturnCode = Util::DynExpErrorCodes::NoError;

		try
		{
			// Throw if Module->TreatModuleExceptionsAsWarnings() is true, but MaxAllowedSubsequentExceptions have been occurred
			// in a row to avoid infinite loops due to an exception occurring in each pass.
			unsigned int NumSubsequentExceptions = 0;
			constexpr unsigned int MaxAllowedSubsequentExceptions = 10;

			auto& NewEventNotifier = Module->GetModuleData()->ModuleThreadOnly.GetNewEventNotifier();

			while (!IsExiting)
			{
				try
				{
					if (Module->IsExiting() || Instance.IsExiting())
						IsExiting = true;

					if (!IsExiting)
					{
						if (!Instance.CareAboutWrappers())
						{
							if (!Module->IsPaused())
							{
								Module->ModuleThreadOnly.OnPause(Instance);
								Module->SetPaused(true, Instance.GetNotReadyObjectNamesString());
							}
							else
								Module->ModuleThreadOnly.SetReasonWhyPaused(Instance.GetNotReadyObjectNamesString());

							std::this_thread::sleep_for(std::chrono::milliseconds(100));
							continue;
						}
						else if (Module->IsPaused())
						{
							Module->SetPaused(false);
							Module->ModuleThreadOnly.OnResume(Instance);
						}
					}

					while (Module->GetModuleData()->GetNumEnqueuedEvents())
						Module->ModuleThreadOnly.HandleEvent(Instance);

					if (!IsExiting)
					{
						auto MainLoopDelay = Module->GetMainLoopDelay();
						auto Now = std::chrono::system_clock::now();

						if (Now - LastMainLoopExecution >= MainLoopDelay || MainLoopDelay == decltype(MainLoopDelay)::max())
						{
							ReturnCode = Module->ModuleThreadOnly.ModuleMainLoop(Instance);

							if (ReturnCode != Util::DynExpErrorCodes::NoError)
								IsExiting = true;

							LastMainLoopExecution = Now;
						}

						if (MainLoopDelay == decltype(MainLoopDelay)::max())
							NewEventNotifier.Wait();
						else if (MainLoopDelay.count() == 0)
							std::this_thread::yield();
						else
							NewEventNotifier.Wait(MainLoopDelay);
					}

					NumSubsequentExceptions = 0;
				}
				catch ([[maybe_unused]] const Util::LinkedObjectNotLockedException& e)
				{
					// In this case, terminate the module since this error cannot be fixed while the module is running.
					throw;
				}
				catch ([[maybe_unused]] const Util::InvalidObjectLinkException& e)
				{
					// In this case, terminate the module since this error cannot be fixed while the module is running.
					throw;
				}
				catch (const Util::Exception& e)
				{
					++NumSubsequentExceptions;

					if (Module->TreatModuleExceptionsAsWarnings() && NumSubsequentExceptions < MaxAllowedSubsequentExceptions)
						Module->SetWarning(e);
					else
						throw;
				}
				catch (const std::exception& e)
				{
					++NumSubsequentExceptions;

					if (Module->TreatModuleExceptionsAsWarnings() && NumSubsequentExceptions < MaxAllowedSubsequentExceptions)
						Module->SetWarning(e.what(), Util::DynExpErrorCodes::GeneralError);
					else
						throw;
				}
				// No catch (...) to terminate module in that case regardless of TreatExceptionsAsWarnings
				// since we do not know at all what has happened.
			}
		}
		catch (const Util::Exception& e)
		{
			Util::EventLog().Log("A module has been terminated because of the error reported below.", Util::ErrorType::Error);
			Util::EventLog().Log(e);

			// std::abort() is called when (e.g. timeout) exception occurrs while setting the caught exception.
			Module->GetModuleData()->ModuleThreadOnly.SetException(std::current_exception());
			Module->ModuleThreadOnly.OnError(Instance);

			return e.ErrorCode;
		}
		catch (const std::exception& e)
		{
			Util::EventLog().Log("A module has been terminated because of the following error: " + std::string(e.what()), Util::ErrorType::Error);

			// std::abort() is called when (e.g. timeout) exception occurrs while setting the caught exception.
			Module->GetModuleData()->ModuleThreadOnly.SetException(std::current_exception());
			Module->ModuleThreadOnly.OnError(Instance);

			return Util::DynExpErrorCodes::GeneralError;
		}
		catch (...)
		{
			Util::EventLog().Log("A module has been terminated because of an unknown error.", Util::ErrorType::Error);

			// std::abort() is called when (e.g. timeout) exception occurrs while setting the caught exception.
			Module->GetModuleData()->ModuleThreadOnly.SetException(std::current_exception());
			Module->ModuleThreadOnly.OnError(Instance);

			return Util::DynExpErrorCodes::GeneralError;
		}

		return ReturnCode;
	}

	EventBase::~EventBase()
	{
	}

	void ModuleDataBase::EnqueueEvent(EventPtrType&& Event)
	{
		if (!Event)
			throw Util::InvalidArgException("Event cannot be nullptr.");

		EventQueue.push(std::move(Event));
		NewEventNotifier.Notify();
	}

	ModuleDataBase::EventPtrType ModuleDataBase::PopEvent()
	{
		if (EventQueue.empty())
			return nullptr;

		auto Event = std::move(EventQueue.front());
		EventQueue.pop();

		return Event;
	}

	void ModuleDataBase::Reset()
	{
		ModuleException = nullptr;
		EventQueueType().swap(EventQueue);	// clear EventQueue

		ResetImpl(dispatch_tag<ModuleDataBase>());
	}

	ModuleParamsBase::~ModuleParamsBase()
	{
	}

	ModuleConfiguratorBase::~ModuleConfiguratorBase()
	{
	}

	ModuleBase::ModuleBase(const std::thread::id OwnerThreadID, ParamsBasePtrType&& Params)
		: RunnableObject(OwnerThreadID, std::move(Params)),
		ModuleThreadOnly(*this), EventListenersOnly(*this),
		ModuleData(std::move(dynamic_Params_cast<ModuleBase>(GetParams())->ModuleData))
	{
		if (!ModuleData)
			throw Util::InvalidArgException("ModuleData cannot be nullptr.");
	}

	ModuleBase::~ModuleBase()
	{
	}

	ModuleBase::ModuleDataTypeSyncPtrType ModuleBase::GetModuleData(const std::chrono::milliseconds Timeout)
	{
		return ModuleDataTypeSyncPtrType(ModuleData.get(), Timeout);
	}

	ModuleBase::ModuleDataTypeSyncPtrConstType ModuleBase::GetModuleData(const std::chrono::milliseconds Timeout) const
	{
		return ModuleDataTypeSyncPtrConstType(ModuleData.get(), Timeout);
	}

	void ModuleBase::EnqueueEvent(ModuleDataBase::EventPtrType&& Event) const
	{
		GetNonConstModuleData()->EnqueueEvent(std::move(Event));
	}

	ModuleBase::ModuleDataTypeSyncPtrType ModuleBase::GetNonConstModuleData(const std::chrono::milliseconds Timeout) const
	{
		return ModuleDataTypeSyncPtrType(ModuleData.get(), Timeout);
	}

	void ModuleBase::HandleEvent(ModuleInstance& Instance)
	{
		EnsureCallFromRunnableThread();

		ModuleDataBase::EventPtrType EventPtr;

		{
			// Locks ModuleData
			auto ModuleDataPtr = GetModuleData();

			if (!ModuleDataPtr->GetNumEnqueuedEvents())
				return;

			EventPtr = ModuleDataPtr->PopEvent();
		} // ModuleData unlocked here

		if (EventPtr)
			EventPtr->Invoke(Instance);
	}

	void ModuleBase::AddRegisteredEvent(EventListenersBase& EventListeners)
	{
		EnsureCallFromRunnableThread();

		auto RegisteredEvent = std::find(RegisteredEvents.cbegin(), RegisteredEvents.cend(), &EventListeners);
		if (RegisteredEvent != RegisteredEvents.cend())
			return;

		RegisteredEvents.push_back(&EventListeners);
	}

	void ModuleBase::RemoveRegisteredEvent(EventListenersBase& EventListeners)
	{
		EnsureCallFromRunnableThread();

		auto RegisteredEvent = std::find(RegisteredEvents.cbegin(), RegisteredEvents.cend(), &EventListeners);
		if (RegisteredEvent == RegisteredEvents.cend())
			return;

		RegisteredEvents.erase(RegisteredEvent);
	}

	Util::DynExpErrorCodes::DynExpErrorCodes ModuleBase::ExecModuleMainLoop(ModuleInstance& Instance)
	{
		EnsureCallFromRunnableThread();

		return ModuleMainLoop(Instance);
	}

	void ModuleBase::OnPause(ModuleInstance& Instance)
	{
		EnsureCallFromRunnableThread();

		OnPauseChild(Instance);
	}

	void ModuleBase::OnResume(ModuleInstance& Instance)
	{
		EnsureCallFromRunnableThread();

		OnResumeChild(Instance);
	}

	void ModuleBase::OnError(ModuleInstance& Instance)
	{
		EnsureCallFromRunnableThread();

		try
		{
			OnErrorChild(Instance);
		}
		catch (const Util::Exception& e)
		{
			Util::EventLog().Log("Calling a module's error handler, the error listed below occurred.", Util::ErrorType::Error);
			Util::EventLog().Log(e);
		}
		catch (const std::exception& e)
		{
			Util::EventLog().Log("Calling a module's error handler, the error listed below occurred.", Util::ErrorType::Error);
			Util::EventLog().Log(e.what());
		}
		catch (...)
		{
			Util::EventLog().Log("Calling a module's error handler, an unknown error occurred.", Util::ErrorType::Error);
		}
	}

	void ModuleBase::ResetImpl(dispatch_tag<RunnableObject>)
	{
		ModuleData->ModuleBaseOnly.Reset();
		RegisteredEvents.clear();

		ResetImpl(dispatch_tag<ModuleBase>());
	}

	void ModuleBase::RunChild()
	{
		MakeAndEnqueueEvent(this, &ModuleBase::OnInit);

		StoreThread(std::thread(ModuleThreadMain, ModuleInstance(
			*this,
			MakeThreadExitedPromise(),
			{ *this, &ModuleBase::GetModuleData, { ModuleBase::GetModuleDataTimeoutDefault } }
		), this));
	}

	void ModuleBase::NotifyChild()
	{
		GetModuleData()->ModuleBaseOnly.GetNewEventNotifier().Notify();
	}

	void ModuleBase::TerminateChild(const std::chrono::milliseconds Timeout)
	{
		try
		{
			MakeAndEnqueueEvent(this, &ModuleBase::OnExit);
			MakeAndEnqueueEvent(this, &ModuleBase::OnDeregisterEvents);
		}
		catch (const Util::Exception& e)
		{
			// Exception occurring while trying to issue an OnExit event is swallowed and logged. It is tried to terminate
			// the module anyway.
			Util::EventLog().Log("A module has been terminated whithout cleaning up since the error reported below has occurred while issuing an exit event.",
				Util::ErrorType::Error);
			Util::EventLog().Log(e);
		}
	}

	std::unique_ptr<BusyDialog> ModuleBase::MakeStartupBusyDialogChild(QWidget* ParentWidget) const
	{
		auto StartupDialog = std::make_unique<BusyDialog>(ParentWidget);
		StartupDialog->SetDescriptionText("Starting module's dependencies...");

		return StartupDialog;
	}

	std::exception_ptr ModuleBase::GetExceptionChild(const std::chrono::milliseconds Timeout) const
	{
		using namespace std::chrono_literals;

		// If ModuleData cannot be locked because it is continuously locked by main/module thread, dead lock
		// is avoided since GetModuleData throws Util::TimeoutException after timeout in this case.
		// Short timeout only since main thread should not block.
		return GetExceptionUnsafe(GetModuleData(ShortTimeoutDefault));
	}

	bool ModuleBase::IsReadyChild() const
	{
		using namespace std::chrono_literals;
		auto LockedModuleData = GetModuleData(ShortTimeoutDefault);

		auto Exception = GetExceptionUnsafe(LockedModuleData);
		Util::ForwardException(Exception);

		return IsRunning() && !IsExiting();
	}

	void ModuleBase::OnDeregisterEvents(ModuleInstance* Instance) const
	{
		// Copy vector since EventListenersBase::Deregister() removes entry from RegisteredEvents
		// via call to ModuleBase::RemoveRegisteredEvent().
		auto Events = RegisteredEvents;

		for (auto Event : Events)
			Event->Deregister(*this);
	}

	ModuleInstance::ModuleInstance(ModuleBase& Owner, std::promise<void>&& ThreadExitedPromise,
		const ModuleBase::ModuleDataGetterType ModuleDataGetter)
		: RunnableInstance(Owner, std::move(ThreadExitedPromise)),
		ModuleDataGetter(ModuleDataGetter)
	{
	}

	ModuleInstance::ModuleInstance(ModuleInstance&& Other)
		: RunnableInstance(std::move(Other)), ModuleDataGetter(Other.ModuleDataGetter)
	{
	}

	InterModuleEventBase::~InterModuleEventBase()
	{
	}

	constexpr Qt::WindowFlags QModuleWidget::GetQtWindowFlagsResizable()
	{
		return Qt::CustomizeWindowHint | Qt::WindowTitleHint |
			Qt::WindowMinMaxButtonsHint | Qt::WindowCloseButtonHint;
	}

	constexpr Qt::WindowFlags QModuleWidget::GetQtWindowFlagsNonResizable()
	{
		return Qt::CustomizeWindowHint | Qt::WindowTitleHint |
			Qt::WindowCloseButtonHint;
	}

	QModuleWidget::QModuleWidget(QModuleBase& Owner, QWidget* Parent)
		: QWidget(Parent), Owner(Owner), DynExpMgr(nullptr),
		DockWindowShortcut(new QShortcut(QKeySequence(Qt::CTRL + Qt::Key_D), this)),
		FocusMainWindowShortcut(new QShortcut(QKeySequence(Qt::CTRL + Qt::Key_0), this))
	{
		connect(DockWindowShortcut, &QShortcut::activated, this, &QModuleWidget::OnDockWindow);
		connect(FocusMainWindowShortcut, &QShortcut::activated, this, &QModuleWidget::OnFocusMainWindow);
	}

	Qt::WindowFlags QModuleWidget::GetQtWindowFlags() const noexcept
	{
		return AllowResize() ? GetQtWindowFlagsResizable() : GetQtWindowFlagsNonResizable();
	}

	std::string QModuleWidget::GetDataSaveDirectory() const
	{
		return DynExpMgr ? DynExpMgr->GetDataSaveDirectory() : "";
	}

	void QModuleWidget::SetDataSaveDirectory(std::string_view Directory) const
	{
		DynExpMgr->SetDataSaveDirectory(Directory);
	}

	void QModuleWidget::EnableDockWindowShortcut(bool Enable) noexcept
	{
		DockWindowShortcut->setEnabled(Enable);
	}

	void QModuleWidget::closeEvent(QCloseEvent* Event)
	{
		if (DynExpMgr)
			if (QMessageBox::question(this, "DynExp - Stop module?",
				QString::fromStdString("Do you want to stop this module?"),
				QMessageBox::StandardButton::Yes | QMessageBox::StandardButton::No, QMessageBox::StandardButton::No)
				== QMessageBox::StandardButton::Yes)
				if (DynExpMgr->StopItem(&Owner))
				{
					Event->accept();
					return;
				}

		Event->ignore();
	}

	void QModuleWidget::OnDockWindow()
	{
		Owner.DockUndockWindow();
	}

	void QModuleWidget::OnFocusWindow()
	{
		Owner.SetFocus();
	}

	void QModuleWidget::OnFocusMainWindow()
	{
		Owner.FocusMainWindow();
	}

	QModuleDockWidget::QModuleDockWidget(QModuleBase& Owner, QWidget* Parent, Qt::WindowFlags Flags)
		: QDockWidget(Parent, Flags), Owner(Owner)
	{
	}

	void QModuleDockWidget::closeEvent(QCloseEvent* Event)
	{
		Event->ignore();
		Owner.DockUndockWindow();
	}

	// Delegate Ctrl+1 - Ctrl+9 to main window
	void QModuleDockWidget::keyPressEvent(QKeyEvent* Event)
	{
		if (Event->modifiers().testFlag(Qt::ControlModifier))
			if (Event->key() >= Qt::Key_1 && Event->key() <= Qt::Key_1 + 8)
			{
				Owner.FocusMainWindow();
				Owner.SendKeyPressEventToMainWindow(Event);

				return;
			}
		
		QDockWidget::keyPressEvent(Event);
	}

	void QModuleDataBase::ResetImpl(dispatch_tag<ModuleDataBase>)
	{
		ResetImpl(dispatch_tag<QModuleDataBase>());
	}

	void WindowStyleParamsExtension::ApplyTo(QWidget& Widget, bool Show) const
	{
		if (WindowState == WindowStateType::Minimized)
			Widget.showMinimized();
		else
		{
			QPoint WindowPos(WindowPosX, WindowPosY);
			QSize WindowSize(Util::NumToT<int>(WindowWidth.Get()), Util::NumToT<int>(WindowHeight.Get()));
			if (!WindowSize.isEmpty())
			{
				Widget.move(WindowPos);
				Widget.resize(WindowSize);
			}

			if (Show)
			{
				if (WindowState == WindowStateType::Maximized)
					Widget.showMaximized();
				else
					Widget.showNormal();
			}
		}
	}

	void WindowStyleParamsExtension::FromWidget(const QWidget& Widget)
	{
		if (Widget.isMinimized())
			WindowState = WindowStateType::Minimized;
		else
		{
			WindowState = Widget.isMaximized() ? WindowStateType::Maximized : WindowStateType::Normal;

			auto WindowPos = Widget.pos();
			auto WindowSize = Widget.size();
			WindowPosX = WindowPos.x();
			WindowPosY = WindowPos.y();
			WindowWidth = WindowSize.width();
			WindowHeight = WindowSize.height();
		}
	}

	QModuleParamsBase::~QModuleParamsBase()
	{
	}

	QModuleConfiguratorBase::~QModuleConfiguratorBase()
	{
	}

	QModuleBase::QModuleBase(const std::thread::id OwnerThreadID, DynExp::ParamsBasePtrType&& Params)
		: ModuleBase(OwnerThreadID, std::move(Params)), Widget(nullptr), MdiArea(nullptr)
	{
	}

	QModuleBase::~QModuleBase()
	{
	}

	QAction& QModuleBase::InitUI(DynExpManager& DynExpMgr, QMdiArea* const MdiArea)
	{
		if (!MdiArea)
			throw Util::InvalidArgException("MdiArea cannot be nullptr.");

		// Make UI
		auto WidgetPtr = MakeUIWidget();
		if (!WidgetPtr)
			throw Util::InvalidDataException("Overridden QModuleBase::MakeUIWidget() must not return nullptr.");
		Widget = WidgetPtr.get();
		Widget->DynExpMgr = &DynExpMgr;

		// Initialize container widgets
		this->MdiArea = MdiArea;
		MdiSubWindow = std::make_unique<QMdiSubWindow>();												// Gets a parent when inserting into MdiArea
		DockWidget = std::make_unique<QModuleDockWidget>(*this, nullptr, Widget->GetQtWindowFlags());	// nullptr parent to make it floating.
		DockWidget->setAllowedAreas(Qt::DockWidgetArea::NoDockWidgetArea);
		DockWidget->setFeatures(QDockWidget::DockWidgetFeature::DockWidgetClosable | QDockWidget::DockWidgetFeature::DockWidgetFloatable);
		DockWidget->setLocale(QLocale(QLocale::Language::English, QLocale::Country::UnitedStates));		// For number format.

		// Insert new Widget into main MDI area. MdiSubWindow becomes Widget's parent.
		MdiSubWindow->setWidget(WidgetPtr.release());
		MdiArea->addSubWindow(MdiSubWindow.get(), Widget->GetQtWindowFlags());
		Widget->EnableDockWindowShortcut(false);

		SetWidgetProperties(MdiSubWindow.get());
		SetWidgetProperties(DockWidget.get());

		// Create action to push module's window into focus.
		// Longer timeout while fetching object name since it could take longer if the entire project is started (empiric value).
		ModuleWindowFocusAction = std::make_unique<QAction>(QString::fromStdString(GetObjectName(std::chrono::seconds(1))));
		QObject::connect(ModuleWindowFocusAction.get(), &QAction::triggered, Widget, &QModuleWidget::OnFocusWindow);

		MdiSubWindow->show();

		return *ModuleWindowFocusAction;
	}

	void QModuleBase::HideUI()
	{
		// Widget is deleted by its respective parent's destructor.
		MdiSubWindow.reset();
		DockWidget.reset();
		ModuleWindowFocusAction.reset();
	}

	void QModuleBase::DisableUI()
	{
		if (!Widget)
			return;

		Widget->setEnabled(false);
	}

	void QModuleBase::UpdateUI()
	{
		if (!Widget)
			return;

		Widget->setEnabled(true);
		UpdateUIChild({ *this, &ModuleBase::GetModuleData, { ModuleBase::GetModuleDataTimeoutDefault } });
	}

	void QModuleBase::UpdateModuleWindowFocusAction()
	{
		if (!Widget || !MdiSubWindow || !ModuleWindowFocusAction)
			throw Util::InvalidStateException("UI widget has not been created yet.");

		if (Widget->parent() != MdiSubWindow.get())
			ModuleWindowFocusAction->setIcon(QIcon(DynExpUI::Icons::UndockedWindow));
		else
			ModuleWindowFocusAction->setIcon(QIcon());
	}

	void QModuleBase::DockWindow() noexcept
	{
		if (!Widget || !MdiArea || !MdiSubWindow || !DockWidget)
			return;

		DockWidget->hide();
		MdiSubWindow->setWidget(Widget);	// Sets Widget parent.
		MdiArea->addSubWindow(MdiSubWindow.get(), Widget->GetQtWindowFlags());
		MdiSubWindow->show();
		Widget->EnableDockWindowShortcut(false);
	}

	void QModuleBase::UndockWindow() noexcept
	{
		if (!Widget || !MdiArea || !MdiSubWindow || !DockWidget)
			return;

		MdiArea->removeSubWindow(MdiSubWindow.get());
		MdiSubWindow->setWidget(nullptr);
		Widget->setParent(DockWidget.get());
		DockWidget->setWidget(Widget);
		DockWidget->showNormal();
		Widget->EnableDockWindowShortcut(true);
	}

	void QModuleBase::DockUndockWindow() noexcept
	{
		if (!Widget || !MdiArea || !MdiSubWindow || !DockWidget)
			return;

		if (Widget->parent() == MdiSubWindow.get())
			UndockWindow();
		else
			DockWindow();

		SetFocus();
	}

	void QModuleBase::SetFocus() noexcept
	{
		if (!Widget || !MdiArea || !MdiSubWindow || !DockWidget)
			return;

		if (Widget->parent() != MdiSubWindow.get())
		{
			Widget->activateWindow();
			Widget->raise();
		}
		else
		{
			FocusMainWindow();
			MdiArea->setActiveSubWindow(MdiSubWindow.get());
		}
	}

	void QModuleBase::FocusMainWindow() noexcept
	{
		if (!Widget)
			return;

		Widget->DynExpMgr->FocusMainWindow();
	}

	void QModuleBase::SendKeyPressEventToMainWindow(QKeyEvent* Event) noexcept
	{
		if (!Widget)
			return;

		Widget->DynExpMgr->PostKeyPressEvent(Event);
	}

	bool QModuleBase::IsWindowDocked() noexcept
	{
		// Just because this is the default and we don't want to throw here.
		if (!Widget || !MdiSubWindow)
			return true;

		return Widget->parent() == MdiSubWindow.get();
	}

	bool QModuleBase::IsActiveWindow()
	{
		if (!Widget || !MdiSubWindow)
			throw Util::InvalidStateException("UI widget has not been created yet.");

		if (Widget->parent() != MdiSubWindow.get())
			return false;

		return MdiArea->currentSubWindow() == MdiSubWindow.get();
	}

	void QModuleBase::RestoreWindowStatesFromParamsChild()
	{
		if (!Widget || !MdiArea || !MdiSubWindow || !DockWidget)
			return;

		auto ModuleParams = DynExp::dynamic_Params_cast<QModuleBase>(GetParams());
		if (ModuleParams->WindowStyleParams.WindowDockingState == WindowStyleParamsExtension::WindowDockingStateType::Docked)
			DockWindow();
		else
			UndockWindow();
		ModuleParams->WindowStyleParams.ApplyTo(IsWindowDocked() ? static_cast<QWidget&>(*MdiSubWindow) : static_cast<QWidget&>(*DockWidget));
	}

	void QModuleBase::UpdateParamsFromWindowStatesChild()
	{
		if (!Widget || !MdiArea || !MdiSubWindow || !DockWidget)
			return;

		auto ModuleParams = DynExp::dynamic_Params_cast<QModuleBase>(GetParams());
		ModuleParams->WindowStyleParams.FromWidget(IsWindowDocked() ? static_cast<QWidget&>(*MdiSubWindow) : static_cast<QWidget&>(*DockWidget));
		ModuleParams->WindowStyleParams.WindowDockingState = IsWindowDocked() ?
			WindowStyleParamsExtension::WindowDockingStateType::Docked : WindowStyleParamsExtension::WindowDockingStateType::Undocked;
	}

	void QModuleBase::SetWidgetProperties(QWidget* const WindowWidget) const
	{
		if (!WindowWidget)
			throw Util::InvalidArgException("WindowWidget cannot be nullptr.");

		// Longer timeout while fetching object name since it could take longer if the entire project is started (empiric value).
		WindowWidget->setWindowTitle(QString::fromStdString(GetObjectName(std::chrono::seconds(1))));
		WindowWidget->setWindowIcon(QIcon(DynExpUI::Icons::Module));

		if (!Widget->AllowResize())
			WindowWidget->layout()->setSizeConstraint(QLayout::SetFixedSize);
	}

	void QModuleBase::ResetImpl(dispatch_tag<ModuleBase>)
	{
		ResetImpl(dispatch_tag<QModuleBase>());
	}
}