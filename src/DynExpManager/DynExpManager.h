// This file is part of DynExp.

/**
 * @file DynExpManager.h
 * @brief Implements %DynExp's main UI window called @p DynExpManager.
*/

#pragma once

#include <QtWidgets/QMainWindow>
#include "ui_DynExpManager.h"
#include "DynExpAbout.h"
#include "DynExpCore.h"
#include "CircuitDiagram.h"
#include "ErrorListDialog.h"

/**
 * @brief Implements %DynExp's main window as a Qt-based user interface (UI).
*/
class DynExpManager : public QMainWindow
{
	Q_OBJECT

public:
	/**
	 * @brief Bundles data which is assigned to items of the main @p QTreeWidget.
	 * Each item refers to a DynExp::Object instance.
	*/
	struct ItemTreeItemDataType
	{
		/**
		 * @brief States the @p QTreeWidgetItem instances can have
		*/
		enum class ItemTreeItemStateType { New, Normal, Running, Paused, Warning, Error, NotConnected, NotResponding };

		constexpr ItemTreeItemDataType() noexcept : ItemTreeItemState(ItemTreeItemStateType::New), ID(0) {}
		constexpr ItemTreeItemDataType(const DynExp::ItemIDType ID,
			const ItemTreeItemStateType ItemTreeItemState = ItemTreeItemStateType::New) noexcept
			: ItemTreeItemState(ItemTreeItemState), ID(ID) {}

		ItemTreeItemStateType ItemTreeItemState;	//!< Current state derived from the related DynExp::Object instance
		const DynExp::ItemIDType ID;				//!< ID of the DynExp::Object instance assigned to this item
	};

	/**
	 * @brief Constructs a @p DynExpManager instance.
	 * @param DynExpCore %DynExp's internal data managed by this @p DynExpManager instance
	 * @param parent Parent Qt widget owning this main window. Pass @p nullptr.
	*/
	DynExpManager(DynExp::DynExpCore& DynExpCore, QWidget *parent = Q_NULLPTR);

	~DynExpManager() = default;

private:
	/**
	 * @brief Retrieves the name of a DynExp::Object instance from its parameter class instance.
	 * @param Object DynExp::Object whose name to retrieve
	 * @return Returns the name of @p Object or a string indicating an error if retrieving the
	 * name failed.
	*/
	static std::string GetObjectNameSafe(DynExp::Object* Object);

	/**
	 * @brief Adds the library entries from @p Lib to submenus below @p MenuBase and assigns
	 * actions to them invoking @p Slot when clicked. Submenus are defined by the library entries'
	 * categories and the actions assigned to the submenus by the entries' names.
	 * @tparam LibraryVectorT Type of the library vector
	 * @param Lib Library vector whose items to add to the menu
	 * @param MenuBase Parent menu where to add submenus to
	 * @param IconPath Icon to display next to the actions
	 * @param Slot Qt slot function to call when the action related to a library entry is triggered.
	*/
	template <typename LibraryVectorT>
	void RegisterItemsFromLibrary(const LibraryVectorT& Lib, QMenu* const MenuBase,
		const QString IconPath, void(DynExpManager::*Slot)());

	/**
	 * @brief Creates a DynExp::Object instance according to a @p QAction instance @p SenderAction, which contains
	 * the index (as its @p data property) of the item within a library vector @p Lib to instantiate.
	 * Displays a configuration dialog (@p ParamsConfigDialog) to let the user set the object's parameters.
	 * @tparam LibraryVectorT Type of a list of library entries (DynExp::LibraryEntry)
	 * @tparam ManagerT Type of the resource manager managing instances of type extracted from @p SenderAction
	 * @param SenderAction QAction which has been triggered to generate a certain DynExp::Object instance.
	 * @param Lib List of library entries to select the entry to be instantiated from according to the
	 * @p QAction::data() property stored in @p SenderAction
	 * @param ResourceManager Resource manager to insert the newly generated DynExp::Object instance into
	 * 
	*/
	template <typename LibraryVectorT, typename ManagerT>
	void MakeItem(QAction* SenderAction, LibraryVectorT& Lib, ManagerT& ResourceManager);

	/**
	 * @brief Invokes a configuration dialog to let the user adjust the parameters of a DynExp::Object instance.
	 * @tparam LibraryVectorT Type of the library vector the object to be configured belongs to
	 * @tparam ManagerT Type of the resource manager owning the object to be configured
	 * @param Object DynExp::Object instance to be configured
	 * @param Lib Library vector the object to be configured belongs to
	 * @param ResourceManager Resource manager owning the object to be configured
	 * @return Returns true if resetting the Object is required to apply the changed settings, false otherwise.
	 * @throws Util::InvalidArgException is thrown if @p Object is nullptr.
	*/
	template <typename LibraryVectorT, typename ManagerT>
	bool UpdateItemConfig(DynExp::Object* Object, LibraryVectorT& Lib, ManagerT& ResourceManager);

	/**
	 * @brief Calls DynExp::Object::EnsureReadyState() on a DynExp::Object instance.
	 * @param Object DynExp::Object instance to make ready. Does nothing if @p Object is @p nullptr.
	*/
	void EnsureItemReadyState(DynExp::Object* Object);
	
	/**
	 * @brief Calls DynExp::Object::Reset() on a DynExp::Object instance.
	 * @param Object DynExp::Object instance to reset. Does nothing if @p Object is @p nullptr.
	*/
	void ResetItem(DynExp::Object* Object);

	/** @name UI functions
	 * Functions updating the user interface. Called periodically in @p OnUpdateUI().
	*/
	///@{
	void UpdateLog();
	void ResetLogColors();
	void UpdateModulesUI() noexcept;
	void UpdateTitleBar();
	void UpdateStatusBar();
	void UpdateCircuitDiagram();
	void UpdateItemTree();
	void UpdateItemTreeItem(const DynExp::HardwareAdapterManager::ResourceType& Resource);
	void UpdateItemTreeItem(const DynExp::InstrumentManager::ResourceType& Resource);
	void UpdateItemTreeItem(const DynExp::ModuleManager::ResourceType& Resource);
	///@}

	/** @name UI item tree functions
	 * Functions to update and manipulate the main @p QTreeWidget containing an item for each DynExp::Object
	 * instance within the currently opened project.
	*/
	///@{
	/**
	 * @brief Loops through resources managed by @p ResourceManager and adds respective @p QTreeWidgetItem instances as
	 * childs to @p Section. Data is assigned to the user role of the columns of added items:
	 * column 0's data contains a boolean flag which is true if column 0's text (object name) needs to be updated.
	 * column 1's data contains a pointer (@p QTreeWidgetItem*) to the respectve item.
	 * column 2's data contains a @p ItemTreeItemDataType instance.
	 * @tparam ManagerT Type of @p ResourceManager (type derived from class DynExp::ResourceManagerBase).
	 * @param Section Parent @p QTreeWidgetItem listing @p ResourceManager's resources as child items.
	 * @param ResourceManager Instance of type @p ManagerT containing resources to be processed.
	 * @return Returns a pointer to the item added most recently by this function or nullptr if no item has been added to @p Section.
	*/
	template <typename ManagerT>
	QTreeWidgetItem* UpdateItemTreeSection(QTreeWidgetItem* Section, ManagerT& ResourceManager);

	/**
	 * @brief Updates the text of a @p QTreeWidgetItem instance to match it to the related
	 * DynExp::Object instance.
	 * @param Item @p QTreeWidgetItem instance to adjust
	 * @param Object DynExp::Object instance related to @p Item
	*/
	void UpdateItemTreeItemObjectName(QTreeWidgetItem* Item, const DynExp::Object* Object);

	/**
	 * @brief Selects the given @p QTreeWidgetItem instance and brings this @p DynExpManager
	 * window to the front.
	 * @param SelectedEntry @p QTreeWidgetItem instance to select
	*/
	void SelectItemTreeItem(QTreeWidgetItem* SelectedEntry);

	void ChangeItemTreeItemToNormalState(QTreeWidgetItem& ItemTreeItem, const ItemTreeItemDataType& ItemTreeItemData,
		const QString& Description, const QString& StateTitle, const char* IconPath = DynExpUI::Icons::Stopped);
	void ChangeItemTreeItemToRunningState(QTreeWidgetItem& ItemTreeItem, const ItemTreeItemDataType& ItemTreeItemData,
		const QString& Description);
	void ChangeItemTreeItemToPausedState(QTreeWidgetItem& ItemTreeItem, const ItemTreeItemDataType& ItemTreeItemData,
		const QString& Description);
	void ChangeItemTreeItemToWarningState(QTreeWidgetItem& ItemTreeItem, const ItemTreeItemDataType& ItemTreeItemData,
		const QString& Description);
	void ChangeItemTreeItemToErrorState(QTreeWidgetItem& ItemTreeItem, const ItemTreeItemDataType& ItemTreeItemData,
		const std::exception_ptr& ExceptionPtr);
	void ChangeItemTreeItemToNotConnectedState(QTreeWidgetItem& ItemTreeItem, const ItemTreeItemDataType& ItemTreeItemData,
		const QString& Description);
	void ChangeItemTreeItemToNotRespondingState(QTreeWidgetItem& ItemTreeItem, const ItemTreeItemDataType& ItemTreeItemData,
		const QString& Description);
	///@}

	/**
	 * @brief Constructs a @p QColor instance from an HTML color string depending on the currently
	 * active UI theme.
	 * @param HTMLColor HTML color string to construct a @p QColor instance from
	 * @return Returns the constructed @p QColor instance.
	*/
	QColor HTMLColorStringToThemeColor(const std::string& HTMLColor) const;

	/**
	 * @brief Transforms a @p QColor instance depending on the currently active UI theme.
	 * @param Color Original @p QColor instance
	 * @return Returns the adjusted new @p QColor instance.
	*/
	QColor AdjustColorToThemeColor(const QColor& Color) const;

	/**
	 * @copydoc DynExp::DynExpCore::Reset()
	*/
	void Reset(bool Force = false);

	/**
	 * @brief Closes the current project and opens a new, empty one.
	 * @return Returns true if closing was successful or if it should be forced by e.g.
	 * terminating the application in case of an error. Returns false when closing should
	 * be aborted in case of an error.
	 */
	bool CloseProject() noexcept;

	/**
	 * @copybrief DynExp::DynExpCore::Shutdown()
	 * @brief If the worker thread cannot be stopped, offers to terminate the application.
	 * @return Returns true if shutdown was successful, false otherwise.
	*/
	bool Shutdown() noexcept;

	/**
	 * @copybrief DynExp::DynExpCore::SaveProject()
	 * @param Filename Path where to save the %DynExp project XML file
	*/
	void SaveProject(std::string_view Filename) noexcept;

	/**
	 * @brief Disables all user interface actions such that item-specific ones can be
	 * reenabled upon a selection change in the main tree widget showing DynExp::Object instances.
	*/
	void DisableAllActions() noexcept;

	/**
	 * @brief Establishes shortcuts to switch between module windows.
	 * Refer to #ModuleWindowsActionGroup.
	*/
	void UpdateModuleWindowsActionShortcuts() noexcept;

	/**
	 * @brief Invokes DynExp::QModuleBase::UpdateModuleWindowFocusAction() on each module.
	*/
	void UpdateModuleWindowsActionIcons() noexcept;

	/**
	 * @brief Determines the DynExp::QModuleBase instance of the current project whose
	 * UI window is active (has the focus).
	 * @return DynExp::QModuleBase instance with the active UI window
	*/
	DynExp::QModuleBase* GetModuleByActiveUIWindow() noexcept;

	/** @name Qt events
	 * Overridden Qt event functions
	*/
	///@{
	virtual void closeEvent(QCloseEvent* event) override;
	///@}

	DynExp::DynExpCore& DynExpCore;						//!< Handle to %DynExp's internal data

	Ui::DynExpManagerClass ui;							//!< Qt widgets belonging to the main window's user interface

	QTimer* UpdateUITimer;								//!< Timer for periodically updating the user interface
	DynExpAbout* AboutDialog;							//!< Dialog showing license and copyright information

	/**
	 * @brief Dialog showing the circuit diagram of the current project. It has no parent to
	 * not stay on top of the main window, so delete it manually.
	 */
	std::unique_ptr<CircuitDiagram> CircuitDiagramDlg;

	ErrorListDialog* ErrorListDlg;						//!< Dialog showing the list of currently active warnings and errors (#ErrorEntries)
	QActionGroup* ModuleWindowsActionGroup;				//!< Actions to switch between module windows with CTRL + < number key > shortcuts
	QActionGroup* UIThemeActionGroup;					//!< Actions to switch in between different UI themes
	QAction* UIBrightThemeAction;						//!< Action for a bright UI theme
	QAction* UIDarkThemeAction;							//!< Action for a dark UI theme
	QMenu* LogContextMenu;								//!< Context menu appearing when right-clicking on the message log widget
	QMenu* ItemTreeContextMenu;							//!< Context menu appearing when right-clicking on the main tree widget showing DynExp::Object instances
	QAction* ClearWarningAction;						//!< Action to clear the warning of an item within the main tree widget showing DynExp::Object instances
	QTreeWidgetItem* ItemTreeHardwareAdapters;			//!< Item for the hardware adapters section within the main tree widget showing DynExp::Object instances
	QTreeWidgetItem* ItemTreeInstruments;				//!< Item for the instruments section within the main tree widget showing DynExp::Object instances
	QTreeWidgetItem* ItemTreeModules;					//!< Item for the modules section within the main tree widget showing DynExp::Object instances

	/**
	 * @brief Type bundling widgets to be displayed in the main window's status bar
	*/
	struct StatusBarType
	{
		/**
		 * @brief Constructs a @p StatusBarType instance.
		 * @param Owner Parent @p DynExpManager instance where this status bar is displayed in
		 */
		StatusBarType(DynExpManager* Owner);

		size_t NumItemsInWarningState;
		size_t NumItemsInErrorState;

		QPushButton* State;
		QWidget* NumRunningInstrGroup;
		QHBoxLayout* NumRunningInstrLayout;
		QLabel* NumRunningInstrImage;
		QLabel* NumRunningInstr;
		QWidget* NumRunningModuleGroup;
		QHBoxLayout* NumRunningModuleLayout;
		QLabel* NumRunningModuleImage;
		QLabel* NumRunningModule;
	} StatusBar;

	/**
	 * @brief List of all currently active warnings and errors
	*/
	ErrorListDialog::ErrorEntriesType ErrorEntries;

	/**
	 * @brief Indicates whether DynExpManager is currently deleting all resources to empty the project.
	*/
	bool IsResetting;

	/**
	 * @brief Indicates whether the circuit diagram has to be rebuilt. Particularly, this must be set to true
	 * directly after invalidating any pointers to QTreeWidgetItem instances of the main window's item tree.
	*/
	bool ShouldRedrawCircuitDiagram;
	
	/**
	 * @brief Indicates whether any item state has changed. Set to true to update the circuit diagram.
	 * The circuit diagram saves pointers to QTreeWidgetItem instances of the main window's item tree.
	 * Make sure that these pointers are still valid. If they have been invalidated, rebuild the circuit
	 * diagram first!
	*/
	bool ShouldUpdateCircuitDiagram;

public:
	/**
	 * @brief Initializes the UI of the module @p Object (expecting that @p Object is a DynExp::QModuleBase
	 * instance. Does nothing if this is not the case or if the module is not running. Resets the module in
	 * case the initialization of its UI fails.
	 * @param Object Module whose UI to initialize
	 * @throws Util::InvalidArgException is thrown if @p Object is @p nullptr.
	*/
	void RegisterModuleUI(DynExp::Object* const Object);
	
	/**
	 * @brief Calls DynExp::RunnableObject::Terminate() on a DynExp::RunnableObject instance.
	 * @param Object DynExp::RunnableObject instance to terminate. Does nothing if @p Object is @p nullptr.
	 * @param Force If true, @p Object is terminated even if it is still used.
	 * @return Returns true in case of success, otherwise false.
	*/
	bool StopItem(DynExp::RunnableObject* Object, bool Force = false) noexcept;
	
	/**
	 * @brief Focuses/activates %DynExp's main window and moves it on top of other windows if possible.
	*/
	void FocusMainWindow() noexcept;

	/**
	 * @brief Delegates a Qt key press event (e.g. the key sequences Ctrl+1 - Ctrl+9) from module
	 * widgets to the main window.
	 * @param Event Qt event to delegate. Refer to Qt documentation.
	*/
	void PostKeyPressEvent(QKeyEvent* Event) noexcept;
	
	/**
	 * @brief @copydoc DynExp::QModuleWidget::GetDataSaveDirectory
	*/
	std::string GetDataSaveDirectory() const { return DynExpCore.GetDataSaveDirectory(); }

	/**
	 * @copydoc DynExp::QModuleWidget::SetDataSaveDirectory
	*/
	void SetDataSaveDirectory(std::string_view Directory) const { return DynExpCore.SetDataSaveDirectory(Directory); }

private slots:
	/** @name Qt slots
	 * Qt slot functions invoked upon UI/widget interaction.
	*/
	///@{
	void OnUpdateUI();
	void OnUIThemeChanged(QAction* ThemeAction);
	void OnNewProject();
	void OnOpenProject();
	void OnSaveProject();
	void OnSaveProjectAs();
	void OnRunProject();
	void OnStopProject();
	void OnResetFailedItems();
	void OnRestoreWindowStatesFromParams();
	void OnProjectSettingsClicked();
	void OnRunItem();
	void OnStopItem(bool Force = false);
	void OnForceStopItem();
	void OnResetItem();
	void OnConfigureItem();
	void OnDeleteItem();
	void OnWindowMenuOpened();
	void OnWindowMenuClosed();
	void OnDockUndockWindow();
	void OnShowCircuitDiagram();
	void OnAboutClicked();
	void OnStatusBarStateClicked();
	void OnLogContextMenuRequested(const QPoint& Position);
	void OnItemTreeContextMenuRequested(const QPoint& Position);
	void OnClearLog();
	void OnClearWarning();
	void OnAddHardwareAdapter();
	void OnAddInstrument();
	void OnAddModule();
	void OnItemSelectionChanged();
	void OnItemDoubleClicked(QTreeWidgetItem* Item, int Column);
	void OnModuleWindowActivated(QMdiSubWindow* Window);
	///@}
};

Q_DECLARE_METATYPE(DynExpManager::ItemTreeItemDataType);

template <typename LibraryVectorT>
void DynExpManager::RegisterItemsFromLibrary(const LibraryVectorT& Lib, QMenu* const MenuBase,
	const QString IconPath, void(DynExpManager::*Slot)())
{
	std::string LastCategoryString("");
	QMenu* CurrentCategoryMenu = nullptr;

	for (size_t i = 0; i < Lib.size(); ++i)
	{
		const auto& LibEntry = Lib[i];

		std::string CategoryString(LibEntry.Category);
		QMenu* MenuToAdd = CategoryString.empty() ? MenuBase : CurrentCategoryMenu;

		if (!CategoryString.empty() && CategoryString != LastCategoryString)
		{
			// ...because one & only makes the next letter a shortcut (see QAction and QMenu)
			CurrentCategoryMenu = MenuBase->addMenu(QString::fromStdString(CategoryString).replace("&", "&&"));
			LastCategoryString = CategoryString;
			MenuToAdd = CurrentCategoryMenu;
		}

		auto AddedAction = MenuToAdd->addAction(QIcon(IconPath),
			QString::fromStdString(LibEntry.Name).replace("&", "&&"), this, Slot);
		AddedAction->setData(QVariant::fromValue(i));
	}
}

template <typename LibraryVectorT, typename ManagerT>
void DynExpManager::MakeItem(QAction* SenderAction, LibraryVectorT& Lib, ManagerT& ResourceManager)
{
	if (!SenderAction)
	{
		Util::EventLog().Log("SenderAction cannot be nullptr.",
			Util::ErrorType::Error, Util::DynExpErrorCodes::InvalidArg);
		return;
	}

	auto Variant = SenderAction->data();
	if (!Variant.canConvert<size_t>())
	{
		Util::EventLog().Log("No library entry has been assigned to this item.",
			Util::ErrorType::Error, Util::DynExpErrorCodes::TypeError);
		return;
	}

	const auto& LibEntry = Lib[Variant.value<size_t>()];

	const std::string ObjectType = DynExp::Object::CategoryAndNameToStr(LibEntry.Category, LibEntry.Name);
	std::string ObjectName = "";
	std::string ErrorMessage = "";

	DynExp::ItemIDType NewItemID = DynExp::ItemIDNotSet;
	try
	{
		auto Params = LibEntry.ConfigFactoryPtr()->MakeConfigFromDialog(ResourceManager.GetNextID(), DynExpCore, this);
		if (!Params)	// e.g. cancelled by user
			return;

		ObjectName = Params->ObjectName;

		NewItemID = DynExpCore.MakeItem(LibEntry, std::move(Params));
	}
	catch (const Util::EmptyException& e)
	{
		ErrorMessage = e.what();
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
				"Creating item " + (ObjectName.empty() ? "< Unnamed >" : ObjectName) +
				+ " (" + ObjectType + "), the following error occurred:\n\n"
				+ ErrorMessage
			));
	}

	if (NewItemID != DynExp::ItemIDNotSet)
	{
		const auto Object = ResourceManager.GetResource(NewItemID);

		EnsureItemReadyState(Object);
		RegisterModuleUI(Object);
	}
}

template <typename LibraryVectorT, typename ManagerT>
bool DynExpManager::UpdateItemConfig(DynExp::Object* Object, LibraryVectorT& Lib, ManagerT& ResourceManager)
{
	if (!Object)
		throw Util::InvalidArgException("Object cannot be nullptr.");
	
	auto LibEntry = DynExp::FindInLibraryVector(Lib, Object->GetCategory(), Object->GetName());
	auto ResultState = LibEntry.ConfigFactoryPtr()->UpdateConfigFromDialog(Object, DynExpCore, this);

	if (ResultState.IsAccepted())
		ResourceManager.DeleteTreeWidgetItem(Object->GetID());

	return ResultState.IsResetRequired();
}

template <typename ManagerT>
QTreeWidgetItem* DynExpManager::UpdateItemTreeSection(QTreeWidgetItem* Section, ManagerT& ResourceManager)
{
	QTreeWidgetItem* LastAdded = nullptr;

	for (auto Resource = ResourceManager.cbegin(); Resource != ResourceManager.cend(); ++Resource)
	{
		if (!Resource->second.TreeWidgetItem)
		{
			auto CategoryAndName = QString::fromStdString(Resource->second.ResourcePointer->GetCategoryAndName());
			auto ID = Resource->second.ResourcePointer->GetID();

			Resource->second.TreeWidgetItem = std::make_unique<QTreeWidgetItem>(Section,
				QStringList{ "", CategoryAndName, "" });
			UpdateItemTreeItemObjectName(Resource->second.TreeWidgetItem.get(), Resource->second.ResourcePointer.get());
			Resource->second.TreeWidgetItem->setToolTip(1, CategoryAndName);
			Resource->second.TreeWidgetItem->setData(1, Qt::ItemDataRole::UserRole, QVariant::fromValue(Resource->second.TreeWidgetItem.get()));
			Resource->second.TreeWidgetItem->setData(2, Qt::ItemDataRole::UserRole, QVariant::fromValue(ItemTreeItemDataType(ID)));

			// Show and sort by item name.
			Section->setExpanded(true);
			Section->sortChildren(0, Qt::SortOrder::AscendingOrder);

			LastAdded = Resource->second.TreeWidgetItem.get();

			ShouldRedrawCircuitDiagram = true;
		}
		else if (Resource->second.TreeWidgetItem->data(0, Qt::ItemDataRole::UserRole).template value<bool>())
		{
			UpdateItemTreeItemObjectName(Resource->second.TreeWidgetItem.get(), Resource->second.ResourcePointer.get());

			ShouldRedrawCircuitDiagram = true;
		}

		UpdateItemTreeItem(Resource->second);
	}

	return LastAdded;
}