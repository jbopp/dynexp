// This file is part of DynExp.

#include "stdafx.h"
#include "moc_DynExpManager.cpp"
#include "DynExpManager.h"

DynExpManager::DynExpManager(DynExp::DynExpCore& DynExpCore, QWidget* parent)
	: QMainWindow(parent), DynExpCore(DynExpCore),
	UpdateUITimer(new QTimer(this)),
	AboutDialog(new DynExpAbout(this)), CircuitDiagramDlg(std::make_unique<CircuitDiagram>(nullptr)),
	ModuleWindowsActionGroup(new QActionGroup(this)),
	UIThemeActionGroup(new QActionGroup(this)), UIBrightThemeAction(nullptr), UIDarkThemeAction(nullptr),
	LogContextMenu(new QMenu(this)),
	ItemTreeContextMenu(new QMenu(this)), ClearWarningAction(nullptr),
	ItemTreeHardwareAdapters(nullptr), ItemTreeInstruments(nullptr), ItemTreeModules(nullptr),
	StatusBar(this), IsResetting(false), ShouldRedrawCircuitDiagram(true), ShouldUpdateCircuitDiagram(false)
{
	qApp->setStyle(QStyleFactory::create("Fusion"));

	ui.setupUi(this);

	// Item Libraries
	RegisterItemsFromLibrary(DynExpCore.GetHardwareAdapterLib(), ui.menu_Add_Hardware_Adapter,
		DynExpUI::Icons::HardwareAdapter, &DynExpManager::OnAddHardwareAdapter);
	RegisterItemsFromLibrary(DynExpCore.GetInstrumentLib(), ui.menu_Add_Instrument,
		DynExpUI::Icons::Instrument, &DynExpManager::OnAddInstrument);
	RegisterItemsFromLibrary(DynExpCore.GetModuleLib(), ui.menu_Add_Module,
		DynExpUI::Icons::Module, &DynExpManager::OnAddModule);

	// Window menu
	connect(ui.menu_Window, &QMenu::aboutToShow, this, &DynExpManager::OnWindowMenuOpened);
	connect(ui.menu_Window, &QMenu::aboutToHide, this, &DynExpManager::OnWindowMenuClosed);

	// Theme menu
	UIBrightThemeAction = UIThemeActionGroup->addAction("&Bright");
	UIBrightThemeAction->setCheckable(true);
	UIBrightThemeAction->setChecked(true);
	UIDarkThemeAction = UIThemeActionGroup->addAction("&Dark");
	UIDarkThemeAction->setCheckable(true);
	ui.menu_UI_Theme->addActions(UIThemeActionGroup->actions());
	connect(UIThemeActionGroup, &QActionGroup::triggered, this, &DynExpManager::OnUIThemeChanged);

	// Log table
	ui.tableLog->horizontalHeader()->setDefaultAlignment(Qt::AlignLeft);
	ui.tableLog->verticalHeader()->setMinimumSectionSize(18);
	ui.tableLog->verticalHeader()->setDefaultSectionSize(ui.tableLog->verticalHeader()->minimumSectionSize());
	ui.tableLog->setColumnWidth(0, 140);

	// Log table context menu
	LogContextMenu->addAction("&Clear Log", this, &DynExpManager::OnClearLog);

	// Status bar
	ui.statusBarMain->addWidget(StatusBar.State, 8);
	ui.statusBarMain->addPermanentWidget(StatusBar.NumRunningInstrGroup, 1);
	ui.statusBarMain->addPermanentWidget(StatusBar.NumRunningModuleGroup, 1);
	connect(StatusBar.State, &QPushButton::clicked, this, &DynExpManager::OnStatusBarStateClicked);

	// Error list dialog
	ErrorListDlg = new ErrorListDialog(this, StatusBar.State);

	// Item tree
	ItemTreeHardwareAdapters = new QTreeWidgetItem(ui.treeItems, { "Hardware Adapters", "", "" });
	ItemTreeHardwareAdapters->setIcon(0, QIcon(DynExpUI::Icons::HardwareAdapter));
	ItemTreeHardwareAdapters->setFirstColumnSpanned(true);
	ItemTreeInstruments = new QTreeWidgetItem(ui.treeItems, { "Instruments", "", "" });
	ItemTreeInstruments->setIcon(0, QIcon(DynExpUI::Icons::Instrument));
	ItemTreeInstruments->setFirstColumnSpanned(true);
	ItemTreeModules = new QTreeWidgetItem(ui.treeItems, { "Modules", "", "" });
	ItemTreeModules->setIcon(0, QIcon(DynExpUI::Icons::Module));
	ItemTreeModules->setFirstColumnSpanned(true);
	ui.treeItems->header()->resizeSection(0, 120);
	ui.treeItems->header()->resizeSection(1, 180);
	ui.treeItems->header()->resizeSection(2, 100);
	ui.splitterInstrListMain->setStretchFactor(0, 16);
	ui.splitterInstrListMain->setStretchFactor(1, 5);

	// Item tree context menu
	ClearWarningAction = ItemTreeContextMenu->addAction("Clear &Warning", this, &DynExpManager::OnClearWarning);

	// MDI area
	connect(ui.mdiMain, &QMdiArea::subWindowActivated, this, &DynExpManager::OnModuleWindowActivated);

	DisableAllActions();

	// If a project has been loaded due to a command line option, run it now.
	if (DynExpCore.IsProjectOpened())
		OnRunProject();

	// Start UI callback timer (63 fps UI update frequency should enable fluent motion)
	connect(UpdateUITimer, &QTimer::timeout, this, &DynExpManager::OnUpdateUI);
	UpdateUITimer->setInterval(std::chrono::milliseconds(16));
	UpdateUITimer->start();
}

DynExpManager::StatusBarType::StatusBarType(DynExpManager* Owner)
	: NumItemsInWarningState(0), NumItemsInErrorState(0),
	State(new QPushButton(Owner)),
	NumRunningInstrGroup(new QWidget(Owner)), NumRunningInstrLayout(new QHBoxLayout),
	NumRunningInstrImage(new QLabel(Owner)), NumRunningInstr(new QLabel(Owner)),
	NumRunningModuleGroup(new QWidget(Owner)), NumRunningModuleLayout(new QHBoxLayout),
	NumRunningModuleImage(new QLabel(Owner)), NumRunningModule(new QLabel(Owner))
{
	constexpr int Height = 16;

	State->setFlat(true);
	State->setMinimumHeight(24);
	State->setFocusPolicy(Qt::FocusPolicy::NoFocus);
	State->setStyleSheet(QString::fromStdString(DynExpUI::PushButtonReadyStyleSheetBright));

	NumRunningInstrImage->setFixedHeight(Height);
	NumRunningInstrImage->setSizePolicy({ QSizePolicy::Policy::Minimum, QSizePolicy::Policy::Minimum });
	NumRunningInstrImage->setToolTip("Running instruments");
	NumRunningInstrImage->setPixmap(QPixmap(DynExpUI::Icons::Instrument).scaledToHeight(Height));
	NumRunningInstr->setSizePolicy({ QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Expanding });
	NumRunningInstr->setToolTip("Running instruments");

	NumRunningModuleImage->setFixedHeight(Height);
	NumRunningModuleImage->setSizePolicy({ QSizePolicy::Policy::Minimum, QSizePolicy::Policy::Minimum });
	NumRunningModuleImage->setToolTip("Running modules");
	NumRunningModuleImage->setPixmap(QPixmap(DynExpUI::Icons::Module).scaledToHeight(Height));
	NumRunningModule->setSizePolicy({ QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Expanding });
	NumRunningModule->setToolTip("Running modules");

	NumRunningInstrLayout->addWidget(NumRunningInstrImage);
	NumRunningInstrLayout->addWidget(NumRunningInstr);
	NumRunningInstrGroup->setLayout(NumRunningInstrLayout);

	NumRunningModuleLayout->addWidget(NumRunningModuleImage);
	NumRunningModuleLayout->addWidget(NumRunningModule);
	NumRunningModuleGroup->setLayout(NumRunningModuleLayout);
}

std::string DynExpManager::GetObjectNameSafe(DynExp::Object* Object)
{
	std::string ObjectName = "< Unknown >";

	if (Object)
	{
		try
		{
			ObjectName = Object->GetObjectName();
		}
		catch (...)
		{
			ObjectName = "< Error while fetching name >";
		}
	}

	return ObjectName;
}

void DynExpManager::EnsureItemReadyState(DynExp::Object* Object)
{
	if (!Object)
		return;

	std::string ErrorMessage = "";
	try
	{
		Object->EnsureReadyState(false);
	}
	catch (const Util::Exception& e)
	{
		Util::EventLog().Log(e);
		ErrorMessage = e.what();
	}
	catch (const std::exception& e)
	{
		ErrorMessage = e.what();
	}
	catch (...)
	{
		ErrorMessage = "Unknown Error";
	}

	if (!ErrorMessage.empty())
	{
		QMessageBox::warning(this, "DynExp - Error",
			QString::fromStdString(
				"Starting up the item " + GetObjectNameSafe(Object) + ", the following error occurred:\n\n"
				+ ErrorMessage
			));
	}
}

void DynExpManager::ResetItem(DynExp::Object* Object)
{
	if (!Object)
		return;

	std::string ErrorMessage = "";
	try
	{
		Object->Reset();
	}
	catch (const Util::Exception& e)
	{
		Util::EventLog().Log(e);
		ErrorMessage = e.what();
	}
	catch (const std::exception& e)
	{
		ErrorMessage = e.what();
	}
	catch (...)
	{
		ErrorMessage = "Unknown Error";
	}

	if (!ErrorMessage.empty())
	{
		std::string ObjectName = GetObjectNameSafe(Object);

		QMessageBox::warning(this, "DynExp - Error",
			QString::fromStdString(
				"Resetting the item " + ObjectName + ", the following error occurred:\n\n"
				+ ErrorMessage
			));
	}
}

void DynExpManager::UpdateLog()
{
	auto EventLogSize = Util::EventLog().GetLogSize();
	if (EventLogSize >= std::numeric_limits<int>::max())
		return;
	
	if (ui.tableLog->rowCount() < static_cast<int>(EventLogSize))
	{
		auto Log = Util::EventLog().GetLog(ui.tableLog->rowCount());
		for (const auto& LogEntry : Log)
		{
			const auto Row = ui.tableLog->rowCount();

			ui.tableLog->setRowCount(Row + 1);
			ui.tableLog->setItem(Row, 0, new QTableWidgetItem(QString::fromStdString(Util::ToStr(LogEntry.TimePoint))));
			ui.tableLog->setItem(Row, 1, new QTableWidgetItem(Util::Exception::GetErrorLabel(LogEntry.Type)));
			ui.tableLog->setItem(Row, 2, new QTableWidgetItem(QString::fromStdString(LogEntry.Message)));

			for (int i = 0; i < 3; ++i)
				ui.tableLog->item(Row, i)->setForeground(HTMLColorStringToThemeColor(Util::Exception::GetErrorLabelColor(LogEntry.Type)));

			ui.tableLog->resizeColumnToContents(2);
			ui.tableLog->scrollToBottom();
		}
	}
}

void DynExpManager::ResetLogColors()
{
	for (auto i = ui.tableLog->rowCount() - 1; i >= 0; --i)
		for (int j = 0; j < 3; ++j)
			ui.tableLog->item(i, j)->setForeground(AdjustColorToThemeColor(ui.tableLog->item(i, j)->foreground().color()));
}

void DynExpManager::UpdateModulesUI() noexcept
{
	const auto& ModuleManager = DynExpCore.GetModuleManager();

	for (auto ModuleIter = ModuleManager.cbegin(); ModuleIter != ModuleManager.cend(); ++ModuleIter)
	{
		try
		{
			auto Module = ModuleIter->second.ResourcePointer.get();
			if (!Module)
				continue;
			if (!Module->HasUI())
				continue;

			// Conversion should always be fine. Throws if not.
			auto& QModule = dynamic_cast<DynExp::QModuleBase&>(*Module);

			if (!QModule.IsRunning())
				QModule.HideUI();
			else if (QModule.IsPaused())
				QModule.DisableUI();
			else
				QModule.UpdateUI();
		}
		catch (const Util::Exception& e)
		{
#ifdef DYNEXP_DEBUG
			Util::EventLog().Log("Updating a module's UI, the error listed below occurred.", Util::ErrorType::Error);
			Util::EventLog().Log(e);
#else
			if (e.Type == Util::ErrorType::Error || e.Type == Util::ErrorType::Fatal)
			{
				Util::EventLog().Log("Updating a module's UI, the error listed below occurred.", Util::ErrorType::Error);
				Util::EventLog().Log(e);
			}
#endif // DYNEXP_DEBUG
		}
		catch (const std::exception& e)
		{
			Util::EventLog().Log("Updating a module's UI, the error listed below occurred.", Util::ErrorType::Error);
			Util::EventLog().Log(e.what());
		}
		catch (...)
		{
			Util::EventLog().Log("Updating a module's UI, an unknown error occurred.", Util::ErrorType::Error);
		}
	}
}

void DynExpManager::UpdateTitleBar()
{
	if (DynExpCore.IsProjectOpened())
	{
		auto ProjectFilename = DynExpCore.GetProjectFilename();
		setWindowTitle(QString::fromStdString(std::string("DynExp Manager - ") + ProjectFilename.string()));
	}
	else
		setWindowTitle("DynExp Manager");

	ui.action_Restore_Windows_from_Settings->setEnabled(static_cast<const DynExp::DynExpCore&>(DynExpCore).GetParams()->StoreWindowStates ==
		DynExp::ProjectParams::StoreWindowStatesType::ApplyStoredWindowStates);
}

void DynExpManager::UpdateStatusBar()
{
	const auto NumRunningInstr = DynExpCore.GetInstrumentManager().GetNumRunningInstruments();
	const auto NumRunningModule = DynExpCore.GetModuleManager().GetNumRunningModules();

	StatusBar.NumRunningInstr->setText(QString::number(NumRunningInstr));
	StatusBar.NumRunningModule->setText(QString::number(NumRunningModule));

	if (StatusBar.NumItemsInErrorState > 0)
	{
		StatusBar.State->setText(QString::number(StatusBar.NumItemsInErrorState) +
			QString((StatusBar.NumItemsInErrorState != 1 ? " errors" : " error")) +
			", " + QString::number(StatusBar.NumItemsInWarningState) +
			QString((StatusBar.NumItemsInWarningState != 1 ? " warnings" : " warning")) + " occurred.");
		StatusBar.State->setStyleSheet(QString::fromStdString(DynExpUI::PushButtonErrorStyleSheet));
	}
	else
	{
		if (StatusBar.NumItemsInWarningState > 0)
		{
			StatusBar.State->setText(QString::number(StatusBar.NumItemsInWarningState) +
				QString((StatusBar.NumItemsInWarningState != 1 ? " warnings" : " warning")) + " occurred.");
			StatusBar.State->setStyleSheet(QString::fromStdString(DynExpUI::PushButtonWarningStyleSheet));
		}
		else
		{
			if (NumRunningInstr + NumRunningModule > 0)
			{
				StatusBar.State->setText("Running");
				StatusBar.State->setStyleSheet(QString::fromStdString(DynExpUI::PushButtonRunningStyleSheet));
			}
			else
			{
				StatusBar.State->setText("Ready");
				StatusBar.State->setStyleSheet(QString::fromStdString(UIBrightThemeAction->isChecked() ?
					DynExpUI::PushButtonReadyStyleSheetBright : DynExpUI::PushButtonReadyStyleSheetDark));
			}
		}
	}

	// When item in ErrorListDlg has been double-clicked, select the respective entry in ui.treeItems.
	ErrorListDlg->SetErrorEntries(ErrorEntries);
	SelectItemTreeItem(ErrorListDlg->GetSelectedEntry());
}

void DynExpManager::UpdateCircuitDiagram()
{
	if (!CircuitDiagramDlg)
		return;

	if (ShouldRedrawCircuitDiagram)
	{
		if (CircuitDiagramDlg->Redraw(DynExpCore))
		{
			ShouldRedrawCircuitDiagram = false;
			ShouldUpdateCircuitDiagram = false;
		}
		else
			return;
	}

	if (ShouldUpdateCircuitDiagram)
	{
		if (CircuitDiagramDlg->UpdateStates(DynExpCore))
			ShouldUpdateCircuitDiagram = false;
		else
			return;
	}

	// When item in CircuitDiagramDlg has been double-clicked, select the respective entry in ui.treeItems.
	SelectItemTreeItem(CircuitDiagramDlg->GetSelectedEntry());
}

void DynExpManager::UpdateItemTree()
{
	StatusBar.NumItemsInWarningState = 0;
	StatusBar.NumItemsInErrorState = 0;

	ErrorEntries.clear();

	// Update item tree and select item most recently added. This is required since editing an item
	// causes it to be removed and readded to the tree. Its selection state is restored.
	// Reverse order to show low level (hardware adapter) errors in status bar with highest priority.
	auto ItemToSelect = UpdateItemTreeSection(ItemTreeModules, DynExpCore.GetModuleManager());
	auto LastAdded = UpdateItemTreeSection(ItemTreeInstruments, DynExpCore.GetInstrumentManager());
	if (LastAdded)
		ItemToSelect = LastAdded;
	LastAdded = UpdateItemTreeSection(ItemTreeHardwareAdapters, DynExpCore.GetHardwareAdapterManager());
	if (LastAdded)
		ItemToSelect = LastAdded;

	if (ItemToSelect)
	{
		ui.treeItems->clearSelection();
		ItemToSelect->setSelected(true);
	}
}

void DynExpManager::UpdateItemTreeItem(const DynExp::HardwareAdapterManager::ResourceType& Resource)
{
	const auto ItemData = Resource.TreeWidgetItem->data(2, Qt::ItemDataRole::UserRole).value<ItemTreeItemDataType>();

	try
	{
		const auto ExceptionPtr = Resource.ResourcePointer->GetException();
		const auto Warning = Resource.ResourcePointer->GetWarning();
		const auto IsConnected = Resource.ResourcePointer->IsConnected();
		
		std::string ObjectName;
		if (ExceptionPtr || Warning)
			ObjectName = GetObjectNameSafe(Resource.ResourcePointer.get()) + " (" + Resource.ResourcePointer->GetCategoryAndName() + ")";

		if (ExceptionPtr)
		{
			if (ItemData.ItemTreeItemState != ItemTreeItemDataType::ItemTreeItemStateType::Error)
				ChangeItemTreeItemToErrorState(*Resource.TreeWidgetItem, ItemData, ExceptionPtr);

			++StatusBar.NumItemsInErrorState;
			auto ErrorMessage = Resource.TreeWidgetItem->toolTip(2);	// Contains error message.

			ErrorEntries.emplace_back(QString::fromStdString(ObjectName), ErrorMessage, Util::ErrorType::Error, Resource.TreeWidgetItem.get());
		}
		else if (Warning)
		{
			if (ItemData.ItemTreeItemState != ItemTreeItemDataType::ItemTreeItemStateType::Warning)
				ChangeItemTreeItemToWarningState(*Resource.TreeWidgetItem, ItemData, QString::fromStdString(Warning.Description));

			++StatusBar.NumItemsInWarningState;
			auto WarningMessage = Resource.TreeWidgetItem->toolTip(2);	// Contains warning message.

			ErrorEntries.emplace_back(QString::fromStdString(ObjectName), WarningMessage, Util::ErrorType::Warning, Resource.TreeWidgetItem.get());
		}
		else if (!IsConnected)
		{
			if (ItemData.ItemTreeItemState != ItemTreeItemDataType::ItemTreeItemStateType::NotConnected)
				ChangeItemTreeItemToNotConnectedState(*Resource.TreeWidgetItem, ItemData,
					"This hardware adapter is not connected yet, and thus it cannot be used now.");
		}
		else
			if (ItemData.ItemTreeItemState != ItemTreeItemDataType::ItemTreeItemStateType::Normal)
				ChangeItemTreeItemToNormalState(*Resource.TreeWidgetItem, ItemData,
					"This hardware adapter is ready to be used.", "Ready", DynExpUI::Icons::Ready);
	}
	catch ([[maybe_unused]] const Util::TimeoutException& e)
	{
		constexpr auto Msg = "This hardware adapter does not respond.";

		if (ItemData.ItemTreeItemState != ItemTreeItemDataType::ItemTreeItemStateType::NotResponding)
			ChangeItemTreeItemToNotRespondingState(*Resource.TreeWidgetItem, ItemData, Msg);

		++StatusBar.NumItemsInWarningState;
	}
}

void DynExpManager::UpdateItemTreeItem(const DynExp::InstrumentManager::ResourceType& Resource)
{
	const auto ItemData = Resource.TreeWidgetItem->data(2, Qt::ItemDataRole::UserRole).value<ItemTreeItemDataType>();

	try
	{
		const auto ExceptionPtr = Resource.ResourcePointer->GetException();
		const auto Warning = Resource.ResourcePointer->GetWarning();
		
		std::string ObjectName;
		if (ExceptionPtr || Warning)
			ObjectName = GetObjectNameSafe(Resource.ResourcePointer.get()) + " (" + Resource.ResourcePointer->GetCategoryAndName() + ")";

		if (ExceptionPtr)
		{
			if (ItemData.ItemTreeItemState != ItemTreeItemDataType::ItemTreeItemStateType::Error)
				ChangeItemTreeItemToErrorState(*Resource.TreeWidgetItem, ItemData, ExceptionPtr);

			++StatusBar.NumItemsInErrorState;
			auto ErrorMessage = Resource.TreeWidgetItem->toolTip(2);	// Contains error message.

			ErrorEntries.emplace_back(QString::fromStdString(ObjectName), ErrorMessage, Util::ErrorType::Error, Resource.TreeWidgetItem.get());
		}
		else if (Warning)
		{
			if (ItemData.ItemTreeItemState != ItemTreeItemDataType::ItemTreeItemStateType::Warning)
				ChangeItemTreeItemToWarningState(*Resource.TreeWidgetItem, ItemData, QString::fromStdString(Warning.Description));

			++StatusBar.NumItemsInWarningState;
			auto WarningMessage = Resource.TreeWidgetItem->toolTip(2);	// Contains warning message.

			ErrorEntries.emplace_back(QString::fromStdString(ObjectName), WarningMessage, Util::ErrorType::Warning, Resource.TreeWidgetItem.get());
		}
		else
		{
			if (Resource.ResourcePointer->IsRunning() &&
				ItemData.ItemTreeItemState != ItemTreeItemDataType::ItemTreeItemStateType::Running)
				ChangeItemTreeItemToRunningState(*Resource.TreeWidgetItem, ItemData, "This instrument is running.");

			if (!Resource.ResourcePointer->IsRunning() &&
				ItemData.ItemTreeItemState != ItemTreeItemDataType::ItemTreeItemStateType::Normal)
				ChangeItemTreeItemToNormalState(*Resource.TreeWidgetItem, ItemData,
					"This instrument is ready to be run.", "Stopped");
		}
	}
	catch ([[maybe_unused]] const Util::TimeoutException& e)
	{
		constexpr auto Msg = "This instrument does not respond.";

		if (ItemData.ItemTreeItemState != ItemTreeItemDataType::ItemTreeItemStateType::NotResponding)
			ChangeItemTreeItemToNotRespondingState(*Resource.TreeWidgetItem, ItemData, Msg);

		++StatusBar.NumItemsInWarningState;
	}
}

void DynExpManager::UpdateItemTreeItem(const DynExp::ModuleManager::ResourceType& Resource)
{
	const auto ItemData = Resource.TreeWidgetItem->data(2, Qt::ItemDataRole::UserRole).value<ItemTreeItemDataType>();

	static constexpr auto PausedString = "This module is paused because one or more linked instruments have been terminated. Restart these instruments in order to continue.";
	static bool DisplayWarning = false;
	static auto DisplayWarningLastSwitchTime = std::chrono::system_clock::now();

	if (DisplayWarningLastSwitchTime <= std::chrono::system_clock::now() - std::chrono::seconds(2))
	{
		DisplayWarning = !DisplayWarning;
		DisplayWarningLastSwitchTime = std::chrono::system_clock::now();
	}

	try
	{
		const auto ExceptionPtr = Resource.ResourcePointer->GetException();
		const auto Warning = Resource.ResourcePointer->GetWarning();

		std::string ObjectName;
		if (ExceptionPtr || Warning || Resource.ResourcePointer->IsPaused())
			ObjectName = GetObjectNameSafe(Resource.ResourcePointer.get()) + " (" + Resource.ResourcePointer->GetCategoryAndName() + ")";

		if (ExceptionPtr)
		{
			if (ItemData.ItemTreeItemState != ItemTreeItemDataType::ItemTreeItemStateType::Error)
				ChangeItemTreeItemToErrorState(*Resource.TreeWidgetItem, ItemData, ExceptionPtr);

			++StatusBar.NumItemsInErrorState;
			auto ErrorMessage = Resource.TreeWidgetItem->toolTip(2);	// Contains error message.

			ErrorEntries.emplace_back(QString::fromStdString(ObjectName), ErrorMessage, Util::ErrorType::Error, Resource.TreeWidgetItem.get());
		}
		else if (Warning || Resource.ResourcePointer->IsPaused())
		{
			if (Warning)
			{
				++StatusBar.NumItemsInWarningState;
				ErrorEntries.emplace_back(QString::fromStdString(ObjectName), QString::fromStdString(Warning.Description), Util::ErrorType::Warning, Resource.TreeWidgetItem.get());
			}
			if (Resource.ResourcePointer->IsPaused())
			{
				++StatusBar.NumItemsInWarningState;
				ErrorEntries.emplace_back(QString::fromStdString(ObjectName),
					QString(PausedString) + "\n\n" + QString::fromStdString(Resource.ResourcePointer->GetReasonWhyPaused().Description),
					Util::ErrorType::Warning, Resource.TreeWidgetItem.get());
			}

			if ((Warning && !Resource.ResourcePointer->IsPaused()) || (Warning && DisplayWarning))
			{
				if (ItemData.ItemTreeItemState != ItemTreeItemDataType::ItemTreeItemStateType::Warning)
					ChangeItemTreeItemToWarningState(*Resource.TreeWidgetItem, ItemData, QString::fromStdString(Warning.Description));
			}
			else
			{
				if (ItemData.ItemTreeItemState != ItemTreeItemDataType::ItemTreeItemStateType::Paused)
					ChangeItemTreeItemToPausedState(*Resource.TreeWidgetItem, ItemData, PausedString);
			}
		}
		else
		{
			if (Resource.ResourcePointer->IsRunning() &&
				ItemData.ItemTreeItemState != ItemTreeItemDataType::ItemTreeItemStateType::Running)
				ChangeItemTreeItemToRunningState(*Resource.TreeWidgetItem, ItemData, "This module is running.");

			if (!Resource.ResourcePointer->IsRunning() &&
				ItemData.ItemTreeItemState != ItemTreeItemDataType::ItemTreeItemStateType::Normal)
				ChangeItemTreeItemToNormalState(*Resource.TreeWidgetItem, ItemData,
					"This module is ready to be run.", "Stopped");
		}
	}
	catch ([[maybe_unused]] const Util::TimeoutException& e)
	{
		constexpr auto Msg = "This module does not respond.";

		if (ItemData.ItemTreeItemState != ItemTreeItemDataType::ItemTreeItemStateType::NotResponding)
			ChangeItemTreeItemToNotRespondingState(*Resource.TreeWidgetItem, ItemData, Msg);

		++StatusBar.NumItemsInWarningState;
	}
}

void DynExpManager::UpdateItemTreeItemObjectName(QTreeWidgetItem* Item, const DynExp::Object* Object)
{
	QString ObjectName;
	bool FetchingObjectNameFailed = false;

	try
	{
		ObjectName = QString::fromStdString(Object->GetObjectName());
	}
	catch (...)
	{
		ObjectName = "< Error >";
		FetchingObjectNameFailed = true;
	}

	Item->setText(0, ObjectName);
	Item->setToolTip(0, ObjectName);
	Item->setData(0, Qt::ItemDataRole::UserRole, QVariant::fromValue(FetchingObjectNameFailed));
}

void DynExpManager::SelectItemTreeItem(QTreeWidgetItem* SelectedEntry)
{
	if (!SelectedEntry)
		return;

	for (int i = 0; i < ui.treeItems->topLevelItemCount(); ++i)
	{
		QTreeWidgetItem* ParentItem = ui.treeItems->topLevelItem(i);
		for (int j = 0; j < ParentItem->childCount(); ++j)
		{
			QTreeWidgetItem* ChildItem = ParentItem->child(j);
			if (ChildItem->data(1, Qt::ItemDataRole::UserRole).value<decltype(SelectedEntry)>() == SelectedEntry)
			{
				Util::ActivateWindow(*this);

				ui.treeItems->clearSelection();
				ui.treeItems->setFocus();
				ChildItem->setSelected(true);

				return;
			}
		}
	}
}

void DynExpManager::ChangeItemTreeItemToNormalState(QTreeWidgetItem& ItemTreeItem, const ItemTreeItemDataType& ItemTreeItemData,
	const QString& Description, const QString& StateTitle, const char* IconPath)
{
	ItemTreeItem.setData(2, Qt::ItemDataRole::UserRole,
		QVariant::fromValue(ItemTreeItemDataType(ItemTreeItemData.ID, ItemTreeItemDataType::ItemTreeItemStateType::Normal)));
	ItemTreeItem.setIcon(2, QIcon(IconPath));
	ItemTreeItem.setText(2, StateTitle);
	ItemTreeItem.setForeground(2, QApplication::palette().text().color());
	ItemTreeItem.setToolTip(2, Description);

	ShouldUpdateCircuitDiagram = true;

	// Issued to update also in case an item's thread terminates regularly
	OnItemSelectionChanged();
}

void DynExpManager::ChangeItemTreeItemToRunningState(QTreeWidgetItem& ItemTreeItem, const ItemTreeItemDataType& ItemTreeItemData,
	const QString& Description)
{
	ItemTreeItem.setData(2, Qt::ItemDataRole::UserRole,
		QVariant::fromValue(ItemTreeItemDataType(ItemTreeItemData.ID, ItemTreeItemDataType::ItemTreeItemStateType::Running)));
	ItemTreeItem.setIcon(2, QIcon(DynExpUI::Icons::Running));
	ItemTreeItem.setText(2, "Running");
	ItemTreeItem.setForeground(2, HTMLColorStringToThemeColor("green"));
	ItemTreeItem.setToolTip(2, Description);

	ShouldUpdateCircuitDiagram = true;

	// Issued to update also in case an item's thread is started automatically.
	OnItemSelectionChanged();
}

void DynExpManager::ChangeItemTreeItemToPausedState(QTreeWidgetItem& ItemTreeItem, const ItemTreeItemDataType& ItemTreeItemData,
	const QString& Description)
{
	ItemTreeItem.setData(2, Qt::ItemDataRole::UserRole,
		QVariant::fromValue(ItemTreeItemDataType(ItemTreeItemData.ID, ItemTreeItemDataType::ItemTreeItemStateType::Paused)));
	ItemTreeItem.setIcon(2, QIcon(DynExpUI::Icons::Paused));
	ItemTreeItem.setText(2, "Paused");
	ItemTreeItem.setForeground(2, HTMLColorStringToThemeColor("orange"));
	ItemTreeItem.setToolTip(2, Description);

	ShouldUpdateCircuitDiagram = true;

	// Issued to update also in case an item's thread is started automatically causing a paused situation immediately.
	OnItemSelectionChanged();
}

void DynExpManager::ChangeItemTreeItemToWarningState(QTreeWidgetItem& ItemTreeItem, const ItemTreeItemDataType& ItemTreeItemData,
	const QString& Description)
{
	ItemTreeItem.setData(2, Qt::ItemDataRole::UserRole,
		QVariant::fromValue(ItemTreeItemDataType(ItemTreeItemData.ID, ItemTreeItemDataType::ItemTreeItemStateType::Warning)));
	ItemTreeItem.setIcon(2, QIcon(DynExpUI::Icons::Warning));
	ItemTreeItem.setText(2, "Warning");
	ItemTreeItem.setForeground(2, HTMLColorStringToThemeColor("orange"));
	ItemTreeItem.setToolTip(2, Description);

	ShouldUpdateCircuitDiagram = true;

	// Issued to update also in case an item's thread is started automatically causing a warning immediately.
	OnItemSelectionChanged();
}

void DynExpManager::ChangeItemTreeItemToErrorState(QTreeWidgetItem& ItemTreeItem, const ItemTreeItemDataType& ItemTreeItemData,
	const std::exception_ptr& ExceptionPtr)
{
	ItemTreeItem.setData(2, Qt::ItemDataRole::UserRole,
		QVariant::fromValue(ItemTreeItemDataType(ItemTreeItemData.ID, ItemTreeItemDataType::ItemTreeItemStateType::Error)));
	ItemTreeItem.setIcon(2, QIcon(DynExpUI::Icons::Error));
	ItemTreeItem.setText(2, "Error");
	ItemTreeItem.setForeground(2, HTMLColorStringToThemeColor("red"));
	ItemTreeItem.setToolTip(2, QString::fromStdString(Util::ExceptionToStr(ExceptionPtr)));

	ShouldUpdateCircuitDiagram = true;

	// Issued to update also in case an item's thread terminates due to an error.
	OnItemSelectionChanged();
}

void DynExpManager::ChangeItemTreeItemToNotConnectedState(QTreeWidgetItem& ItemTreeItem, const ItemTreeItemDataType& ItemTreeItemData,
	const QString& Description)
{
	ItemTreeItem.setData(2, Qt::ItemDataRole::UserRole,
		QVariant::fromValue(ItemTreeItemDataType(ItemTreeItemData.ID, ItemTreeItemDataType::ItemTreeItemStateType::NotConnected)));
	ItemTreeItem.setIcon(2, QIcon(DynExpUI::Icons::NotReady));
	ItemTreeItem.setText(2, "Connecting...");
	ItemTreeItem.setForeground(2, QApplication::palette().text().color());
	ItemTreeItem.setToolTip(2, Description);

	ShouldUpdateCircuitDiagram = true;
}

void DynExpManager::ChangeItemTreeItemToNotRespondingState(QTreeWidgetItem& ItemTreeItem, const ItemTreeItemDataType& ItemTreeItemData,
	const QString& Description)
{
	ItemTreeItem.setData(2, Qt::ItemDataRole::UserRole,
		QVariant::fromValue(ItemTreeItemDataType(ItemTreeItemData.ID, ItemTreeItemDataType::ItemTreeItemStateType::NotResponding)));
	ItemTreeItem.setIcon(2, QIcon(DynExpUI::Icons::NotResponding));
	ItemTreeItem.setText(2, "Not responding");
	ItemTreeItem.setForeground(2, HTMLColorStringToThemeColor("orange"));
	ItemTreeItem.setToolTip(2, Description);

	ShouldUpdateCircuitDiagram = true;

	// Issued to update also in case an item's thread is started automatically causing a warning immediately.
	OnItemSelectionChanged();
}

QColor DynExpManager::HTMLColorStringToThemeColor(const std::string& HTMLColor) const
{
	QString HTMLColorQ = QString::fromStdString(HTMLColor);

	if (UIThemeActionGroup->checkedAction() == UIDarkThemeAction)
	{
		if (HTMLColorQ.toLower() == "blue")
			return DynExpUI::DarkPalette::blue;
		if (HTMLColorQ.toLower() == "green")
			return DynExpUI::DarkPalette::green;
	}

	return QColor(HTMLColorQ);
}

QColor DynExpManager::AdjustColorToThemeColor(const QColor& Color) const
{
	if (UIThemeActionGroup->checkedAction() == UIBrightThemeAction)
	{
		if (Color == DynExpUI::DarkPalette::blue)
			return QColor("blue");
	}
	else if (UIThemeActionGroup->checkedAction() == UIDarkThemeAction)
	{
		if (Color == QColor("blue"))
			return DynExpUI::DarkPalette::blue;
	}

	return Color;
}

void DynExpManager::Reset(bool Force)
{
	IsResetting = true;

	try
	{
		DynExpCore.Reset(Force);
	}
	catch (...)
	{
		IsResetting = false;

		throw;
	}

	IsResetting = false;
	ShouldRedrawCircuitDiagram = true;
}

bool DynExpManager::CloseProject() noexcept
{
	if (!DynExpCore.GetHardwareAdapterManager().Empty() || !DynExpCore.GetInstrumentManager().Empty() || !DynExpCore.GetModuleManager().Empty())
		if (QMessageBox::question(this, "DynExp - Close project?",
			"Really close the current project? All running items will be stopped and unsaved changes will be lost.", 
			QMessageBox::StandardButton::Yes | QMessageBox::StandardButton::No, QMessageBox::StandardButton::No)
			!= QMessageBox::StandardButton::Yes)
			return false;

	std::string ErrorMessage = "";
	try
	{
		Reset();
	}
	catch (const Util::Exception& e)
	{
		Util::EventLog().Log(e);
		ErrorMessage = e.what();
	}
	catch (const std::exception& e)
	{
		ErrorMessage = e.what();
	}
	catch (...)
	{
		ErrorMessage = "Unknown Error";
	}

	if (!ErrorMessage.empty())
	{
		auto Result = QMessageBox::critical(this, "DynExp - Critical error",
			QString::fromStdString(
				"Closing a project and stopping all running items, the following error occurred:\n\n"
				+ ErrorMessage + "\n\n- Click 'Retry' to try again to close the project."
				+ "\n- Click 'Ignore' to force the project to be closed. This may cause DynExp to be terminated without properly shutting down."
				+ "\n- Click 'Cancel' to return to the project without closing it."
			), QMessageBox::StandardButton::Cancel | QMessageBox::StandardButton::Retry | QMessageBox::StandardButton::Ignore,
			QMessageBox::StandardButton::Cancel);
		
		switch (Result)
		{
		case QMessageBox::StandardButton::Cancel: return false;
		case QMessageBox::StandardButton::Ignore: return true;
		default: return CloseProject();
		}
	}

	OnItemSelectionChanged();

	return true;
}

bool DynExpManager::Shutdown() noexcept
{
	std::string ErrorMessage = "";
	try
	{
		DynExpCore.Shutdown();
	}
	catch (const Util::Exception& e)
	{
		Util::EventLog().Log(e);
		ErrorMessage = e.what();
	}
	catch (const std::exception& e)
	{
		ErrorMessage = e.what();
	}
	catch (...)
	{
		ErrorMessage = "Unknown Error";
	}

	if (!ErrorMessage.empty())
	{
		auto Result = QMessageBox::critical(this, "DynExp - Critical error",
			QString::fromStdString(
				"Stopping a worker thread, the following error occurred:\n\n"
				+ ErrorMessage + "\n\n- Click 'Retry' to try again to stop the thread."
				+ "\n- Click 'Ignore' to force DynExp to be terminated without properly shutting down."
				+ "\n- Click 'Cancel' to return to DynExp without closing it."
			), QMessageBox::StandardButton::Cancel | QMessageBox::StandardButton::Retry | QMessageBox::StandardButton::Ignore,
			QMessageBox::StandardButton::Cancel);

		switch (Result)
		{
		case QMessageBox::StandardButton::Cancel: return false;
		case QMessageBox::StandardButton::Ignore: std::terminate();
		default: return Shutdown();
		}
	}

	return true;
}

void DynExpManager::SaveProject(std::string_view Filename) noexcept
{
	std::string ErrorMessage = "";
	try
	{
		DynExpCore.SaveProject(Filename, *this, *CircuitDiagramDlg, *ui.splitterInstrListMain, *ui.splitterMDILog);
	}
	catch (const Util::Exception& e)
	{
		Util::EventLog().Log(e);
		ErrorMessage = e.what();
	}
	catch (const std::exception& e)
	{
		ErrorMessage = e.what();
	}
	catch (...)
	{
		ErrorMessage = "Unknown Error";
	}

	if (!ErrorMessage.empty())
	{
		QMessageBox::warning(this, "DynExp - Error",
			QString::fromStdString(
				std::string("Saving the project to file ").append(Filename)
				+ ", the following error occurred:\n\n" + ErrorMessage
			));
	}
}

void DynExpManager::DisableAllActions() noexcept
{
	ui.action_Run_Item->setEnabled(false);
	ui.action_Stop_Item->setEnabled(false);
	ui.action_Force_Stop_Item->setEnabled(false);
	ui.action_Reset_Item->setEnabled(false);
	ui.action_Configure_Item->setEnabled(false);
	ui.action_Delete_Item->setEnabled(false);

	ClearWarningAction->setEnabled(false);
}

void DynExpManager::UpdateModuleWindowsActionShortcuts() noexcept
{
	const auto NumModules = ModuleWindowsActionGroup->actions().size();

	// Shortcuts Ctrl + 1 to Ctrl + 9 (Ctrl + 0 activates main window)
	for (std::remove_const_t<decltype(NumModules)> i = 0; i < NumModules; ++i)
	{
		if (i < 9)
			ModuleWindowsActionGroup->actions()[i]->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_1 + i));
		else
			ModuleWindowsActionGroup->actions()[i]->setShortcut(QKeySequence());
	}
}

void DynExpManager::UpdateModuleWindowsActionIcons() noexcept
{
	const auto& ModuleManager = DynExpCore.GetModuleManager();

	for (auto ModuleIter = ModuleManager.cbegin(); ModuleIter != ModuleManager.cend(); ++ModuleIter)
	{
		try
		{
			auto Module = ModuleIter->second.ResourcePointer.get();
			if (!Module)
				continue;
			if (!Module->IsRunning() || !Module->HasUI())
				continue;

			// Conversion should never fail. Throws if not.
			auto& QModule = dynamic_cast<DynExp::QModuleBase&>(*Module);
			QModule.UpdateModuleWindowFocusAction();
		}
		catch (const Util::Exception& e)
		{
			Util::EventLog().Log("Enumerating active module windows, the error listed below occurred.", Util::ErrorType::Error);
			Util::EventLog().Log(e);
		}
		catch (const std::exception& e)
		{
			Util::EventLog().Log("Enumerating active module windows, the error listed below occurred.", Util::ErrorType::Error);
			Util::EventLog().Log(e.what());
		}
		catch (...)
		{
			Util::EventLog().Log("Enumerating active module windows, an unknown error occurred.", Util::ErrorType::Error);
		}
	}
}

DynExp::QModuleBase* DynExpManager::GetModuleByActiveUIWindow() noexcept
{
	const auto& ModuleManager = DynExpCore.GetModuleManager();

	for (auto ModuleIter = ModuleManager.cbegin(); ModuleIter != ModuleManager.cend(); ++ModuleIter)
	{
		try
		{
			auto Module = ModuleIter->second.ResourcePointer.get();
			if (!Module)
				continue;
			if (!Module->IsRunning() || !Module->HasUI())
				continue;

			// Conversion should never fail. Throws if not.
			auto& QModule = dynamic_cast<DynExp::QModuleBase&>(*Module);
			if (QModule.IsActiveWindow())
				return &QModule;
		}
		catch (const Util::Exception& e)
		{
			Util::EventLog().Log("Finding active module window, the error listed below occurred.", Util::ErrorType::Error);
			Util::EventLog().Log(e);
		}
		catch (const std::exception& e)
		{
			Util::EventLog().Log("Finding active module window, the error listed below occurred.", Util::ErrorType::Error);
			Util::EventLog().Log(e.what());
		}
		catch (...)
		{
			Util::EventLog().Log("Finding active module window, an unknown error occurred.", Util::ErrorType::Error);
		}
	}

	return nullptr;
}

void DynExpManager::closeEvent(QCloseEvent* event)
{
	UpdateUITimer->stop();

	if (CloseProject() && Shutdown())
	{
		CircuitDiagramDlg->hide();

		event->accept();

		return;
	}

	event->ignore();
	UpdateUITimer->start();
}

void DynExpManager::RegisterModuleUI(DynExp::Object* const Object)
{
	if (!Object)
		throw Util::InvalidArgException("Object cannot be nullptr.");

	// If Object is a module which has an UI (a QModule), init and display its UI.
	auto Module = dynamic_cast<DynExp::QModuleBase*>(Object);
	if (!Module)
		return;

	if (!Module->IsRunning())
		return;

	try
	{
		auto const Action = &Module->InitUI(*this, ui.mdiMain);
		ModuleWindowsActionGroup->addAction(Action);
		UpdateModuleWindowsActionShortcuts();
		ui.menu_Window->addAction(Action);
	}
	catch (...)
	{
		// In case of an error occurring setting up the module's UI, terminate the module.
		Module->Reset();

		throw;
	}
}

bool DynExpManager::StopItem(DynExp::RunnableObject* Object, bool Force) noexcept
{
	if (!Object)
		return false;

	std::string ErrorMessage = "";
	std::string ObjectName = "< Unknown >";
	try
	{
		ObjectName = GetObjectNameSafe(Object);
		Object->Terminate(Force);

		OnItemSelectionChanged();

		return true;
	}
	catch (const DynExp::RunnableObject::NotUnusedException& e)
	{
		if (QMessageBox::warning(this, "DynExp - Force-terminate item?",
			QString("This runnable is currently being used by at least another item. Force-terminate the item?\n\n")
			+ e.GetUserNames().data(),
			QMessageBox::StandardButton::Yes | QMessageBox::StandardButton::No, QMessageBox::StandardButton::No)
			== QMessageBox::StandardButton::Yes)
		{
			return StopItem(Object, true);
		}
	}
	catch (const Util::Exception& e)
	{
		Util::EventLog().Log(e);
		ErrorMessage = e.what();
	}
	catch (const std::exception& e)
	{
		ErrorMessage = e.what();
	}
	catch (...)
	{
		ErrorMessage = "Unknown Error";
	}

	if (!ErrorMessage.empty())
	{
		QMessageBox::warning(this, "DynExp - Error",
			QString::fromStdString(
				"Stopping the item " + ObjectName + ", the following error occurred:\n\n"
				+ ErrorMessage
			));
	}

	OnItemSelectionChanged();

	return false;
}

void DynExpManager::FocusMainWindow() noexcept
{
	activateWindow();
	raise();
}

void DynExpManager::PostKeyPressEvent(QKeyEvent* Event) noexcept
{
	// postEvent() will delete event after posting it into the event queue.
	QCoreApplication::postEvent(this, Event->clone());
}

void DynExpManager::OnUpdateUI()
{
	// Necessary to prevent calling functions of destroyed objects while stack unwinding is in progress.
	if (std::uncaught_exceptions())
		return;

	try
	{
		UpdateLog();
		UpdateTitleBar();
		UpdateItemTree();
		UpdateStatusBar();			// Relies on item tree having been updated directly before.
		UpdateCircuitDiagram();		// Relies on item tree having been updated directly before.
	}
	catch (const Util::Exception& e)
	{
#ifdef DYNEXP_DEBUG
		Util::EventLog().Log("Updating the main window UI, the error listed below occurred.", Util::ErrorType::Error);
		Util::EventLog().Log(e);
#else
		if (e.Type == Util::ErrorType::Error || e.Type == Util::ErrorType::Fatal)
		{
			Util::EventLog().Log("Updating the main window UI, the error listed below occurred.", Util::ErrorType::Error);
			Util::EventLog().Log(e);
		}
#endif // DYNEXP_DEBUG
	}
	catch (const std::exception& e)
	{
		Util::EventLog().Log("Updating the main window UI, the error listed below occurred.", Util::ErrorType::Error);
		Util::EventLog().Log(e.what());
	}
	catch (...)
	{
		Util::EventLog().Log("Updating the main window UI, an unknown error occurred.", Util::ErrorType::Error);
	}

	// The following functions do not throw.
	// ->
	UpdateModulesUI();

	if (DynExpCore.HasLoadedProjectFromCommandlineParams())
		OnRestoreWindowStatesFromParams();
	// <-
}

void DynExpManager::OnUIThemeChanged(QAction* ThemeAction)
{
	if (ThemeAction == UIBrightThemeAction)
	{
		qApp->setPalette(QPalette());
		qApp->setStyleSheet("");
	}
	else if (ThemeAction == UIDarkThemeAction)
	{
		qApp->setPalette(DynExpUI::DarkPalette::GetPalette());
		qApp->setStyleSheet(QString::fromStdString(DynExpUI::DarkPalette::GetStyleSheet()));
	}

	ResetLogColors();
	DynExpCore.GetHardwareAdapterManager().DeleteAllTreeWidgetItems();
	DynExpCore.GetInstrumentManager().DeleteAllTreeWidgetItems();
	DynExpCore.GetModuleManager().DeleteAllTreeWidgetItems();

	ShouldRedrawCircuitDiagram = true;
}

void DynExpManager::OnNewProject()
{
	if (!CloseProject())
		return;
}

void DynExpManager::OnOpenProject()
{
	if (!CloseProject())
		return;

	auto Filename = Util::PromptOpenFilePath(this, "Open project", ".dynp", "DynExp project files (*.dynp)").toStdString();
	if (Filename.empty())
		return;

	std::string ErrorMessage = "";
	try
	{
		DynExpCore.OpenProject(Filename);
	}
	catch (const Util::Exception& e)
	{
		Util::EventLog().Log(e);
		ErrorMessage = e.what();
	}
	catch (const std::exception& e)
	{
		ErrorMessage = e.what();
	}
	catch (...)
	{
		ErrorMessage = "Unknown Error";
	}

	if (!ErrorMessage.empty())
	{
		QMessageBox::warning(this, "DynExp - Error",
			QString::fromStdString(
				"Opening a project from file " + Filename + ", the following error occurred:\n\n"
				+ ErrorMessage
			));

		try
		{
			// If this throws, something went terribly wrong. Terminate application.
			Reset(true);
		}
		catch (...)
		{
			Util::EventLog().Log("Error occurred while resetting project. Execution cannot continue.",
				Util::ErrorType::Fatal);

			std::terminate();
		}
	}
	else
	{
		// First, only restore window states and sizes in order not to confuse the user
		// by windows moving away suddenly after some time of starting up the project...
		try
		{
			DynExpCore.RestoreWindowStatesFromParams(*this, *CircuitDiagramDlg, *ui.splitterInstrListMain, *ui.splitterMDILog, true);
		}
		catch (...)
		{
			// Swallow any exception here since we try again in a moment.
		}

		// ...second, run the project and restore all the module windows' states (and again the o
		OnRunProject();
		OnRestoreWindowStatesFromParams();
	}
}

void DynExpManager::OnSaveProject()
{
	if (DynExpCore.IsProjectOpened())
		SaveProject(DynExpCore.GetProjectFilename().string());
	else
		OnSaveProjectAs();
}

void DynExpManager::OnSaveProjectAs()
{
	auto Filename = Util::PromptSaveFilePath(this, "Save project as", ".dynp", "DynExp project files (*.dynp)").toStdString();
	if (Filename.empty())
		return;

	SaveProject(Filename);
}

// Firstly, start hardware adapters. Then, wait until they are connected before starting
// instruments and modules.
void DynExpManager::OnRunProject()
{
	std::string ErrorMessage = "";
	try
	{
		// Connect all hardware adapters asynchronously
		auto ConnectingHardwareAdaptersFuture = DynExpCore.ConnectHardwareAdapters();

		auto BusyDlg = std::make_unique<BusyDialog>(this);
		BusyDlg->SetDescriptionText("Connecting hardware adapters...");
		BusyDlg->SetCheckFinishedFunction(std::bind_front(&DynExp::DynExpCore::AllHardwareAdaptersConnected, &DynExpCore));

		// BusyDlg->exec() blocks and starts new event loop for the modal dialog.
		auto Result = BusyDlg->exec();
		ConnectingHardwareAdaptersFuture.wait();
		
		if (Result == QDialog::Accepted)
		{
			BusyDlg->SetDescriptionText("Starting instruments...");
			BusyDlg->SetCheckFinishedFunction(std::bind_front(&DynExp::DynExpCore::AllInstrumentsInitialized, &DynExpCore));

			DynExpCore.RunInstruments();
			Result = BusyDlg->exec();

			if (Result == QDialog::Accepted)
				DynExpCore.RunModules(std::bind_front(&DynExpManager::RegisterModuleUI, this));
			else if (BusyDlg->GetException())
				std::rethrow_exception(BusyDlg->GetException());
		}
		else if (BusyDlg->GetException())
			std::rethrow_exception(BusyDlg->GetException());
	}
	catch (const Util::Exception& e)
	{
		Util::EventLog().Log(e);
		ErrorMessage = e.what();
	}
	catch (const std::exception& e)
	{
		ErrorMessage = e.what();
	}
	catch (...)
	{
		ErrorMessage = "Unknown Error";
	}

	if (!ErrorMessage.empty())
	{
		QMessageBox::warning(this, "DynExp - Error",
			QString::fromStdString(
				"Starting up the project, the following error occurred:\n\n"
				+ ErrorMessage
			));
	}

	OnItemSelectionChanged();
}

void DynExpManager::OnStopProject()
{
	std::string ErrorMessage = "";
	try
	{
		DynExpCore.ShutdownProject();
	}
	catch (const Util::Exception& e)
	{
		Util::EventLog().Log(e);
		ErrorMessage = e.what();
	}
	catch (const std::exception& e)
	{
		ErrorMessage = e.what();
	}
	catch (...)
	{
		ErrorMessage = "Unknown Error";
	}

	if (!ErrorMessage.empty())
	{
		QMessageBox::warning(this, "DynExp - Error",
			QString::fromStdString(
				"Shutting down the project, the following error occurred:\n\n"
				+ ErrorMessage
			));
	}

	OnItemSelectionChanged();
}

void DynExpManager::OnResetFailedItems()
{
	std::string ErrorMessage = "";
	try
	{
		DynExpCore.ResetFailedItems(*this);
	}
	catch (const Util::Exception& e)
	{
		Util::EventLog().Log(e);
		ErrorMessage = e.what();
	}
	catch (const std::exception& e)
	{
		ErrorMessage = e.what();
	}
	catch (...)
	{
		ErrorMessage = "Unknown Error";
	}

	if (!ErrorMessage.empty())
	{
		QMessageBox::warning(this, "DynExp - Error",
			QString::fromStdString(
				"Resetting the project's failed items, the following error occurred:\n\n"
				+ ErrorMessage
			));
	}

	OnItemSelectionChanged();
}

void DynExpManager::OnRestoreWindowStatesFromParams()
{
	std::string ErrorMessage = "";
	try
	{
		DynExpCore.RestoreWindowStatesFromParams(*this, *CircuitDiagramDlg, *ui.splitterInstrListMain, *ui.splitterMDILog);
	}
	catch (const Util::Exception& e)
	{
		Util::EventLog().Log(e);
		ErrorMessage = e.what();
	}
	catch (const std::exception& e)
	{
		ErrorMessage = e.what();
	}
	catch (...)
	{
		ErrorMessage = "Unknown Error";
	}

	if (!ErrorMessage.empty())
	{
		QMessageBox::warning(this, "DynExp - Error",
			QString::fromStdString(
				"Restoring the main and module windows' geometries and states from the project file, the following error occurred:\n\n"
				+ ErrorMessage
			));
	}
}

void DynExpManager::OnProjectSettingsClicked()
{
	std::string ErrorMessage = "";
	try
	{
		DynExpCore.EditProjectSettings(this);
	}
	catch (const Util::Exception& e)
	{
		Util::EventLog().Log(e);
		ErrorMessage = e.what();
	}
	catch (const std::exception& e)
	{
		ErrorMessage = e.what();
	}
	catch (...)
	{
		ErrorMessage = "Unknown Error";
	}

	if (!ErrorMessage.empty())
	{
		QMessageBox::warning(this, "DynExp - Error",
			QString::fromStdString(
				"Editing the project settings, the following error occurred:\n\n"
				+ ErrorMessage
			));
	}
}

void DynExpManager::OnRunItem()
{
	if (ui.treeItems->selectedItems().length() != 1)
		return;

	auto Item = ui.treeItems->selectedItems()[0];
	const auto ItemData = Item->data(2, Qt::ItemDataRole::UserRole).value<ItemTreeItemDataType>();

	std::string ErrorMessage = "";
	std::string ObjectName = "< Unknown >";
	try
	{
		DynExp::RunnableObject* Object = nullptr;

		if (Item->parent() == ItemTreeInstruments)
			Object = DynExpCore.GetInstrumentManager().GetResource(ItemData.ID);
		if (Item->parent() == ItemTreeModules)
			Object = DynExpCore.GetModuleManager().GetResource(ItemData.ID);

		if (Object)
		{
			ObjectName = GetObjectNameSafe(Object);

			Object->Run(this);
			RegisterModuleUI(Object);
		}
	}
	catch (const Util::Exception& e)
	{
		Util::EventLog().Log(e);
		ErrorMessage = e.what();
	}
	catch (const std::exception& e)
	{
		ErrorMessage = e.what();
	}
	catch (...)
	{
		ErrorMessage = "Unknown Error";
	}

	if (!ErrorMessage.empty())
	{
		QMessageBox::warning(this, "DynExp - Error",
			QString::fromStdString(
				"Starting the item " + ObjectName + ", the following error occurred:\n\n"
				+ ErrorMessage
			));
	}

	OnItemSelectionChanged();
}

void DynExpManager::OnStopItem(bool Force)
{
	if (ui.treeItems->selectedItems().length() != 1)
		return;

	auto Item = ui.treeItems->selectedItems()[0];
	const auto ItemData = Item->data(2, Qt::ItemDataRole::UserRole).value<ItemTreeItemDataType>();

	DynExp::RunnableObject* Object = nullptr;
	std::string ErrorMessage = "";
	try
	{
		if (Item->parent() == ItemTreeInstruments)
			Object = DynExpCore.GetInstrumentManager().GetResource(ItemData.ID);
		if (Item->parent() == ItemTreeModules)
			Object = DynExpCore.GetModuleManager().GetResource(ItemData.ID);
	}
	catch (const Util::Exception& e)
	{
		Util::EventLog().Log(e);
		ErrorMessage = e.what();
	}
	catch (const std::exception& e)
	{
		ErrorMessage = e.what();
	}
	catch (...)
	{
		ErrorMessage = "Unknown Error";
	}

	if (!ErrorMessage.empty())
	{
		QMessageBox::warning(this, "DynExp - Error",
			QString::fromStdString(
				"Determining the object to be stopped, the following error occurred:\n\n"
				+ ErrorMessage
			));
	}

	StopItem(Object, Force);	// noexcept, checks whether Object is nullptr.
}

void DynExpManager::OnForceStopItem()
{
	OnStopItem(true);
}

void DynExpManager::OnResetItem()
{
	if (ui.treeItems->selectedItems().length() != 1)
		return;

	auto Item = ui.treeItems->selectedItems()[0];
	const auto ItemData = Item->data(2, Qt::ItemDataRole::UserRole).value<ItemTreeItemDataType>();
	DynExp::Object* Object = nullptr;

	std::string ErrorMessage = "";
	try
	{
		if (Item->parent() == ItemTreeHardwareAdapters)
			Object = DynExpCore.GetHardwareAdapterManager().GetResource(ItemData.ID);
		if (Item->parent() == ItemTreeInstruments)
			Object = DynExpCore.GetInstrumentManager().GetResource(ItemData.ID);
		if (Item->parent() == ItemTreeModules)
			Object = DynExpCore.GetModuleManager().GetResource(ItemData.ID);
	}
	catch (const Util::Exception& e)
	{
		Util::EventLog().Log(e);
		ErrorMessage = e.what();
	}
	catch (const std::exception& e)
	{
		ErrorMessage = e.what();
	}
	catch (...)
	{
		ErrorMessage = "Unknown Error";
	}

	if (!ErrorMessage.empty())
	{
		std::string ObjectName = GetObjectNameSafe(Object);

		QMessageBox::warning(this, "DynExp - Error",
			QString::fromStdString(
				"Resetting the item " + ObjectName + ", the following error occurred:\n\n"
				+ ErrorMessage
			));
	}
	else
		ResetItem(Object);	// ResetItem() itself checks if nullptr and handles exceptions.

	OnItemSelectionChanged();
}

void DynExpManager::OnConfigureItem()
{
	if (ui.treeItems->selectedItems().length() != 1)
		return;

	auto Item = ui.treeItems->selectedItems()[0];
	const auto ItemData = Item->data(2, Qt::ItemDataRole::UserRole).value<ItemTreeItemDataType>();
	DynExp::Object* Object = nullptr;
	DynExp::RunnableObject* RunnableObject = nullptr;
	bool ResetRequired = false;

	std::string ErrorMessage = "";
	try
	{
		if (Item->parent() == ItemTreeHardwareAdapters)
			Object = DynExpCore.GetHardwareAdapterManager().GetResource(ItemData.ID);
		if (Item->parent() == ItemTreeInstruments)
		{
			RunnableObject = DynExpCore.GetInstrumentManager().GetResource(ItemData.ID);
			Object = RunnableObject;
		}
		if (Item->parent() == ItemTreeModules)
		{
			RunnableObject = DynExpCore.GetModuleManager().GetResource(ItemData.ID);
			Object = RunnableObject;
		}

		if (!Object)
			return;

		if (Item->parent() == ItemTreeHardwareAdapters)
			ResetRequired = UpdateItemConfig(Object, DynExpCore.GetHardwareAdapterLib(), DynExpCore.GetHardwareAdapterManager());
		if (Item->parent() == ItemTreeInstruments)
			ResetRequired = UpdateItemConfig(Object, DynExpCore.GetInstrumentLib(), DynExpCore.GetInstrumentManager());
		if (Item->parent() == ItemTreeModules)
			ResetRequired = UpdateItemConfig(Object, DynExpCore.GetModuleLib(), DynExpCore.GetModuleManager());
	}
	catch (const Util::Exception& e)
	{
		Util::EventLog().Log(e);
		ErrorMessage = e.what();
	}
	catch (const std::exception& e)
	{
		ErrorMessage = e.what();
	}
	catch (...)
	{
		ErrorMessage = "Unknown Error";
	}

	std::string ObjectName = GetObjectNameSafe(Object);

	if (!ErrorMessage.empty())
		QMessageBox::warning(this, "DynExp - Error",
			QString::fromStdString(
				"Configuring the item " + ObjectName + ", the following error occurred:\n\n"
				+ ErrorMessage
			));
	else
	{
		if (ResetRequired)
		{
			// Only ask whether to reset item for hardware adapters and running instruments/modules.
			if ((Object && !RunnableObject) /* hardware adapter */ || (RunnableObject && RunnableObject->IsRunning()) /* instrument or module */)
			{
				if (QMessageBox::question(this, "DynExp - Reset item?",
					QString::fromStdString("The item " + ObjectName + " needs to be reset in order to apply the changes. Should it be reset now?"),
					QMessageBox::StandardButton::Yes | QMessageBox::StandardButton::No, QMessageBox::StandardButton::No)
					== QMessageBox::StandardButton::Yes)
					ResetItem(Object);	// See below.
			}
			else
				ResetItem(Object);		// ResetItem() itself checks if nullptr and handles exceptions.
		}
	}

	OnItemSelectionChanged();
}

void DynExpManager::OnDeleteItem()
{
	if (ui.treeItems->selectedItems().length() != 1)
		return;

	auto Item = ui.treeItems->selectedItems()[0];
	const auto ItemData = Item->data(2, Qt::ItemDataRole::UserRole).value<ItemTreeItemDataType>();

	std::string ErrorMessage = "";
	std::string ObjectName = "< Unknown >";
	try
	{
		if (Item->parent() == ItemTreeHardwareAdapters)
		{
			auto Object = DynExpCore.GetHardwareAdapterManager().GetResource(ItemData.ID);

			if (Object)
			{
				ObjectName = GetObjectNameSafe(Object);
				DynExpCore.GetHardwareAdapterManager().RemoveResource(ItemData.ID);
			}
		}
		else
		{
			DynExp::RunnableObject* Object = nullptr;

			if (Item->parent() == ItemTreeInstruments)
				Object = DynExpCore.GetInstrumentManager().GetResource(ItemData.ID);
			if (Item->parent() == ItemTreeModules)
				Object = DynExpCore.GetModuleManager().GetResource(ItemData.ID);
				
			if (Object)
			{
				ObjectName = GetObjectNameSafe(Object);

				if (Object->IsRunning())
				{
					if (QMessageBox::question(this, "DynExp - Stop item?",
						QString::fromStdString("The item " + ObjectName + " is currently running. Should it be stopped in order to be deleted?"),
						QMessageBox::StandardButton::Yes | QMessageBox::StandardButton::No, QMessageBox::StandardButton::No)
						!= QMessageBox::StandardButton::Yes)
						return;

					Object->Terminate();
				}

				if (Item->parent() == ItemTreeInstruments)
					DynExpCore.GetInstrumentManager().RemoveResource(ItemData.ID);
				if (Item->parent() == ItemTreeModules)
					DynExpCore.GetModuleManager().RemoveResource(ItemData.ID);
			}
		}
	}
	catch (const DynExp::RunnableObject::NotUnusedException& e)
	{
		ErrorMessage = e.GetErrorMessage();
	}
	catch (const Util::Exception& e)
	{
		Util::EventLog().Log(e);
		ErrorMessage = e.what();
	}
	catch (const std::exception& e)
	{
		ErrorMessage = e.what();
	}
	catch (...)
	{
		ErrorMessage = "Unknown Error";
	}

	ShouldRedrawCircuitDiagram = true;

	if (!ErrorMessage.empty())
	{
		QMessageBox::warning(this, "DynExp - Error",
			QString::fromStdString(
				"Deleting the item " + ObjectName + ", the following error occurred:\n\n"
				+ ErrorMessage
			));
	}

	OnItemSelectionChanged();
}

void DynExpManager::OnWindowMenuOpened()
{
	ui.action_Dock_Undock_Window->setEnabled(GetModuleByActiveUIWindow() != nullptr);
	ui.action_Dummy_NoWindow->setVisible(ModuleWindowsActionGroup->actions().empty());
	UpdateModuleWindowsActionIcons();
}

void DynExpManager::OnWindowMenuClosed()
{
	// To listen on shortcut.
	ui.action_Dock_Undock_Window->setEnabled(true);
}

void DynExpManager::OnDockUndockWindow()
{
	auto const QModule = GetModuleByActiveUIWindow();

	if (QModule)
		QModule->DockUndockWindow();
}

void DynExpManager::OnShowCircuitDiagram()
{
	CircuitDiagramDlg->show();
	Util::ActivateWindow(*CircuitDiagramDlg);
}

void DynExpManager::OnAboutClicked()
{
	AboutDialog->exec();
}

void DynExpManager::OnStatusBarStateClicked()
{
	// Avoid direct reopening after close (refer to ErrorListDialog::focusOutEvent()).
	if (!ErrorListDlg->HasBeenClosedByClickingOpenWidget())
	{
		ErrorListDlg->show();
		ErrorListDlg->resize(StatusBar.State->width() - 4, 250);
		ErrorListDlg->move(StatusBar.State->mapToGlobal(StatusBar.State->pos()) - QPoint(0, 6 + ErrorListDlg->height()));
		ErrorListDlg->setFocus();
	}
	else
		ErrorListDlg->ResetClosedByClickingOpenWidget();
}

void DynExpManager::OnLogContextMenuRequested(const QPoint& Position)
{
	LogContextMenu->exec(ui.tableLog->mapToGlobal(Position));
}

void DynExpManager::OnItemTreeContextMenuRequested(const QPoint& Position)
{
	ItemTreeContextMenu->exec(ui.treeItems->mapToGlobal(Position));
}

void DynExpManager::OnClearLog()
{
	std::string ErrorMessage = "";
	try
	{
		Util::EventLog().ClearLog();
		ui.tableLog->setRowCount(0);
	}
	catch (const Util::Exception& e)
	{
		Util::EventLog().Log(e);
		ErrorMessage = e.what();
	}
	catch (const std::exception& e)
	{
		ErrorMessage = e.what();
	}
	catch (...)
	{
		ErrorMessage = "Unknown Error";
	}

	if (!ErrorMessage.empty())
	{
		QMessageBox::warning(this, "DynExp - Error",
			QString::fromStdString(
				"Clearing the event log, the following error occurred:\n\n"
				+ ErrorMessage
			));
	}
}

void DynExpManager::OnClearWarning()
{
	if (ui.treeItems->selectedItems().length() != 1)
		return;

	auto Item = ui.treeItems->selectedItems()[0];
	const auto ItemData = Item->data(2, Qt::ItemDataRole::UserRole).value<ItemTreeItemDataType>();
	DynExp::Object* Object = nullptr;

	try
	{
		if (Item->parent() == ItemTreeHardwareAdapters)
			Object = DynExpCore.GetHardwareAdapterManager().GetResource(ItemData.ID);
		if (Item->parent() == ItemTreeInstruments)
			Object = DynExpCore.GetInstrumentManager().GetResource(ItemData.ID);
		if (Item->parent() == ItemTreeModules)
			Object = DynExpCore.GetModuleManager().GetResource(ItemData.ID);

		if (!Object)
			return;

		Object->ClearWarning();
	}
	catch (...)
	{
		return;
	}
}

void DynExpManager::OnAddHardwareAdapter()
{
	MakeItem(qobject_cast<QAction*>(sender()), DynExpCore.GetHardwareAdapterLib(), DynExpCore.GetHardwareAdapterManager());
}

void DynExpManager::OnAddInstrument()
{
	MakeItem(qobject_cast<QAction*>(sender()), DynExpCore.GetInstrumentLib(), DynExpCore.GetInstrumentManager());
}

void DynExpManager::OnAddModule()
{
	MakeItem(qobject_cast<QAction*>(sender()), DynExpCore.GetModuleLib(), DynExpCore.GetModuleManager());
}

void DynExpManager::OnItemSelectionChanged()
{
	if (IsResetting)
		return;

	if (ui.treeItems->selectedItems().length() != 1)
	{
		DisableAllActions();
		return;
	}

	auto Item = ui.treeItems->selectedItems()[0];

	if (Item->parent() == ItemTreeHardwareAdapters || Item->parent() == ItemTreeInstruments ||
		Item->parent() == ItemTreeModules)
	{
		ui.action_Reset_Item->setEnabled(true);
		ui.action_Configure_Item->setEnabled(true);
		ui.action_Delete_Item->setEnabled(true);

		ClearWarningAction->setEnabled(true);
	}

	if (Item->parent() == ItemTreeHardwareAdapters)
	{
		ui.action_Run_Item->setEnabled(false);
		ui.action_Stop_Item->setEnabled(false);
		ui.action_Force_Stop_Item->setEnabled(false);

		return;
	}

	if (Item->parent() == ItemTreeInstruments || Item->parent() == ItemTreeModules)
	{
		const auto ItemData = Item->data(2, Qt::ItemDataRole::UserRole).value<ItemTreeItemDataType>();

		bool IsRunning = false;
		try
		{
			if (Item->parent() == ItemTreeInstruments)
				IsRunning = DynExpCore.GetInstrumentManager().GetResource(ItemData.ID)->IsRunning();
			if (Item->parent() == ItemTreeModules)
				IsRunning = DynExpCore.GetModuleManager().GetResource(ItemData.ID)->IsRunning();
		}
		catch (...)
		{
			// Swallow especially Util::NotFoundException.
		}

		ui.action_Run_Item->setEnabled(!IsRunning);
		ui.action_Stop_Item->setEnabled(IsRunning);
		ui.action_Force_Stop_Item->setEnabled(IsRunning);

		return;
	}

	DisableAllActions();
}

void DynExpManager::OnItemDoubleClicked(QTreeWidgetItem* Item, int Column)
{
	OnConfigureItem();
}

void DynExpManager::OnModuleWindowActivated(QMdiSubWindow* Window)
{
	if (IsResetting)
		return;

	auto ActiveQModule = GetModuleByActiveUIWindow();

	if (ActiveQModule)
	{
		ui.treeItems->clearSelection();
		DynExpCore.GetModuleManager().FocusTreeWidgetItem(ActiveQModule->GetID());
	}
}