// This file is part of DynExp.

/**
 * @file DynExpCore.h
 * @brief Defines %DynExp's core module as an interface between the UI and %DynExp objects.
*/

#pragma once

#include "stdafx.h"
#include "Libraries.h"
#include "Managers.h"

/**
 * @brief %DynExp's main namespace contains the implementation of %DynExp including classes
 * to manage resources (%DynExp objects like hardware adapters, instruments, and modules)
 * as well as their dependencies and parameters.
*/
namespace DynExp
{
	/**
	 * @brief Defines a parameter class with parameters common to all %DynExp projects.
	*/
	class ProjectParams final : public ParamsBase
	{
		friend class DynExpCore;

	public:
		/**
		 * @brief Indicates whether to apply window states (like position and size) from
		 * the project files.
		*/
		enum StoreWindowStatesType { ApplyStoredWindowStates, ApplyDefaultWindowStates };
		/**
		 * @var ProjectParams::StoreWindowStatesType ProjectParams::ApplyStoredWindowStates
		 * The window states stored in the project file are applied to user interface windows.
		*/
		/**
		 * @var ProjectParams::StoreWindowStatesType ProjectParams::ApplyDefaultWindowStates
		 * No window states from the project file are applied to user interface windows.
		*/

		/**
		 * @brief Assigns labels to the entries of @p StoreWindowStatesType.
		 * @return Mapping between the entries of @p StoreWindowStatesType and human-readable descriptions
		*/
		static Util::TextValueListType<StoreWindowStatesType> AvlblStoreWindowStatesTypeStrList();

		/**
		 * @brief Constructs the parameters for a @p ProjectParams instance.
		 * The @p ID argument passed to ParamsBase::ParamsBase() is always 0.
		 * @copydetails ParamsBase::ParamsBase
		*/
		ProjectParams(const DynExpCore& Core) : ParamsBase(0, Core),
			MainWindowStyleParams(*this, "Main"), CircuitWindowStyleParams(*this, "Circuit") {}

		~ProjectParams() {}

		const char* GetParamClassTag() const noexcept override final { return "ProjectParams"; }

		Param<ParamsConfigDialog::TextType> Authors = { *this, "Authors", "Authors", "Name of the project's authors.", false };		//!< Author of project file
		Param<ParamsConfigDialog::TextType> Version = { *this, "Version", "Version", "Current version of the project.", false };	//!< Version of project file
		Param<ParamsConfigDialog::TextType> Comment = { *this, "Comment", "Comment", "Comments related to the project.", false };	//!< Comment to project file

		/**
		 * @copybrief StoreWindowStatesType 
		*/
		Param<StoreWindowStatesType> StoreWindowStates = { *this, AvlblStoreWindowStatesTypeStrList(), "StoreWindowStates", "Remember window states",
			"Determines whether module windows' geometries and states are to be remembered when loading the project from file.",
			false, StoreWindowStatesType::ApplyStoredWindowStates };

		/**
		 * @brief Window states of the main window. WindowStyleParamsExtension::WindowDockingState member is ignored.
		*/
		WindowStyleParamsExtension MainWindowStyleParams;

		/**
		 * @brief Window states of the circuit diagram window (@p CircuitDiagram). WindowStyleParamsExtension::WindowDockingState
		 * determines the window's visibility (WindowStyleParamsExtension::WindowDockingStateType::Undocked means visible, every
		 * other value means hidden).
		*/
		WindowStyleParamsExtension CircuitWindowStyleParams;

		/**
		 * @brief Widths of areas split horizontally by the main splitter widget in the main window.
		*/
		ListParam<int> HSplitterWidgetWidths = { *this, "HSplitterWidgetWidths", {}, 0 };

		/**
		 * @brief Widths of areas split vertically by the main splitter widget in the main window.
		*/
		ListParam<int> VSplitterWidgetHeights = { *this, "VSplitterWidgetHeights", {}, 0 };

	private:
		void ConfigureParamsImpl(dispatch_tag<ParamsBase>) override final;

		/** @name Thread-safe variables
		 * These variables have been defined here, to guarantee thread-safe access. They are not stored
		 * in the project file.
		*/
		///@{
		/**
		 * @brief Path to project file. Only non-empty if project has been saved or if it was loaded from file.
		*/
		std::filesystem::path ProjectFilename;

		/**
		 * @brief Path to directory where modules saved their data most recently.
		*/
		std::filesystem::path LastDataSaveDirectory;
		///@}
	};

	/**
	 * @brief %DynExp's core class acts as the interface between the user interface and
	 * %DynExp's internal data like all DynExp::Object instances and their threads.
	 * There should only be one instance of this class. It owns all %DynExp resources
	 * through the respective @p ResourceManagerBase instances.
	 * Logical const-ness: Non-const functions (like functions to run/stop/reset/save/...
	 * the project) cannot be called from DynExp::Object instances possessing a const
	 * handle to the @p DynExpCore instance. Instead, these functions should be called
	 * through the user interface (from the @p DynExpManager instance).
	 * @warning Ownership of the @p DynExpCore instance must not be transferred to another
	 * thread than the one creating it.
	*/
	class DynExpCore final : public Util::INonCopyable
	{
	public:
		/**
		 * @brief Alias for the return type of DynExpCore::GetParams(). Parameters wrapped into
		 * Util::SynchronizedPointer can be accessed in a thread-safe way.
		*/
		using ParamsTypeSyncPtrType = Util::SynchronizedPointer<ProjectParams>;

		/**
		 * @brief Alias for the return type of DynExpCore::GetParams() const. Parameters wrapped into
		 * Util::SynchronizedPointer can be accessed in a thread-safe way.
		*/
		using ParamsConstTypeSyncPtrType = Util::SynchronizedPointer<const ProjectParams>;

		/**
		 * @brief Default timeout used by DynExpCore::GetParams() to lock the mutex of the project
		 * parameter instance.
		*/
		static constexpr std::chrono::milliseconds GetParamsTimeoutDefault = std::chrono::milliseconds(100);

		/**
		 * @brief Constructs a @p DynExpCore instance. 
		 * @param HardwareAdapterLib Hardware adapters available to %DynExp (result of LibraryBase::ToVector()).
		 * @param InstrumentLib Instruments available to %DynExp (result of LibraryBase::ToVector()).
		 * @param ModuleLib Modules available to %DynExp (result of LibraryBase::ToVector()).
		 * @param ProjectFileToOpen Path to a %DynExp project XML file to load and to start
		*/
		DynExpCore(HardwareAdapterLibraryVectorType HardwareAdapterLib, InstrumentLibraryVectorType InstrumentLib,
			ModuleLibraryVectorType ModuleLib, std::string ProjectFileToOpen = "");
		~DynExpCore();

		/**
		 * @brief Terminates DynExpCore::WorkerThread and waits until the thread has ended.
		 * @throws Util::ThreadDidNotRespondException is thrown if #WorkerThread does not terminate in due time.
		*/
		void Shutdown();

		/**
		 * @brief Resets the currently loaded project removing all resources from the resource managers.
		 * After a call to this function, a new empty project is available.
		 * @param Force If false, ResourceManagerBase::PrepareReset() is called on each owned resource
		 * manager, before calls to ResourceManagerBase::Reset() follow. If true, only the latter
		 * method is called.
		*/
		void Reset(bool Force = false);

		/**
		 * @brief Saves the current %DynExp project to an XML project file.
		 * @param Filename Path where to save the %DynExp project XML file
		 * @param MainWindow %DynExp's main window (@p DynExpManager instance) whose window state to save
		 * @param CircuitDiagramDlg %DynExp's circuit diagram window (@p CircuitDiagram instance) whose window state to save
		 * @param HSplitter Horizontal main splitter widget in the main window
		 * @param VSplitter Vertical main splitter widget in the main window
		*/
		void SaveProject(std::string_view Filename, const QMainWindow& MainWindow, const QDialog& CircuitDiagramDlg, QSplitter& HSplitter, QSplitter& VSplitter);

		/**
		 * @brief Loads a %DynExp project from an XML project file.
		 * @param Filename Path to a %DynExp project XML file to load
		 * @throws Util::InvalidDataException is thrown if the data contained in the file @p Filename
		 * refers to could not be parsed as XML.
		*/
		void OpenProject(std::string_view Filename);
		
		/**
		 * @brief Opens a settings dialog (@p ParamsConfigDialog) to let the user configure the
		 * parameter values of the current project settings stored in #Params.
		 * @param DialogParent QWidget acting as a parent of the modal settings dialog
		*/
		void EditProjectSettings(QWidget* const DialogParent);

		/**
		 * @brief Connects all hardware adapters contained in #HardwareAdapterMgr asynchronously calling
		 * ResourceManagerBase::Startup()
		 * @param FunctionToCallWhenHardwareAdapterConnecting Callback function. Refer to @p FunctionToCallWhenObjectStartedType.
		 * @return Returns a @p std::future instance. Refer to the documentation of @p std::async().
		*/
		auto ConnectHardwareAdapters(CommonResourceManagerBase::FunctionToCallWhenObjectStartedType FunctionToCallWhenHardwareAdapterConnecting = nullptr)
		{
			return std::async(std::launch::async, &HardwareAdapterManager::Startup, &HardwareAdapterMgr, FunctionToCallWhenHardwareAdapterConnecting);
		}

		/**
		 * @brief Checks whether all hardware adapters contained in #HardwareAdapterMgr have been connected successfully.
		 * @return Returns true if this is the case, false otherwise.
		*/
		bool AllHardwareAdaptersConnected() const;

		/**
		 * @brief Checks whether all instruments contained in #InstrumentMgr have been initialized successfully.
		 * @return Returns true if this is the case, false otherwise.
		*/
		bool AllInstrumentsInitialized() const;

		/**
		 * @brief Runs all instruments contained in #InstrumentMgr with RunnableObjectParams::StartupType::OnCreation startup setting.
		 * @param FunctionToCallWhenInstrumentStarted Callback function. Refer to @p FunctionToCallWhenObjectStartedType.
		*/
		void RunInstruments(CommonResourceManagerBase::FunctionToCallWhenObjectStartedType FunctionToCallWhenInstrumentStarted = nullptr);

		/**
		 * @brief Runs all modules contained in #ModuleMgr with RunnableObjectParams::StartupType::OnCreation startup setting.
		 * @param FunctionToCallWhenModuleStarted Callback function. Refer to @p FunctionToCallWhenObjectStartedType.
		*/
		void RunModules(CommonResourceManagerBase::FunctionToCallWhenObjectStartedType FunctionToCallWhenModuleStarted = nullptr);

		/**
		 * @brief Terminates all running instruments and modules.
		*/
		void ShutdownProject();

		/**
		 * @brief Calls Object::Reset() and Object::ClearWarning() on all owned DynExp::Object instances which
		 * are in an error state. Considers dependencies in between these objects and stops and restarts related
		 * objects if required for resetting failed objects and if the user confirms.
		 * @param ParentWindow QWidget acting as a parent of modal dialog boxes, which are potentially displayed.
		*/
		void ResetFailedItems(QWidget& ParentWindow);

		/**
		 * @brief Sets module and main windows' positions, sizes and styles according to parameters stored in
		 * the current project's configuration #Params.
		 * @param MainWindow %DynExp's main window (@p DynExpManager instance) whose window state to save
		 * @param CircuitDiagramDlg %DynExp's circuit diagram window (@p CircuitDiagram instance) whose window state to save
		 * @param HSplitter Horizontal main splitter widget in the main window
		 * @param VSplitter Vertical main splitter widget in the main window
		 * @param OnlyMainWindow If true, only applies window states from the project settings to the main window.
		 * If false, also applies saved window states to module windows.
		*/
		void RestoreWindowStatesFromParams(QMainWindow& MainWindow, QDialog& CircuitDiagramDlg, QSplitter& HSplitter, QSplitter& VSplitter,
			bool OnlyMainWindow = false);

		/**
		 * @brief Getter for the hardware adapter library.
		 * @return Returns #HardwareAdapterLib.
		*/
		auto& GetHardwareAdapterLib() const noexcept { return HardwareAdapterLib; }

		/**
		 * @brief Getter for the instrument library.
		 * @return Returns #InstrumentLib.
		*/
		auto& GetInstrumentLib() const noexcept { return InstrumentLib; }

		/**
		 * @brief Getter for the module library.
		 * @return Returns #ModuleLib.
		*/
		auto& GetModuleLib() const noexcept { return ModuleLib; }

		/**
		 * @brief Getter for the hardware adapter manager.
		 * @return Returns #HardwareAdapterMgr.
		*/
		auto& GetHardwareAdapterManager() noexcept { return HardwareAdapterMgr; }

		/**
		 * @brief Getter for the instrument manager.
		 * @return Returns #InstrumentMgr.
		*/
		auto& GetInstrumentManager() noexcept { return InstrumentMgr; }

		/**
		 * @brief Getter for the module manager.
		 * @return Returns #ModuleMgr.
		*/
		auto& GetModuleManager() noexcept { return ModuleMgr; }

		auto& GetHardwareAdapterManager() const noexcept { return HardwareAdapterMgr; }		//!< @copydoc DynExpCore::GetHardwareAdapterManager()
		auto& GetInstrumentManager() const noexcept { return InstrumentMgr; }				//!< @copydoc DynExpCore::GetInstrumentManager()
		auto& GetModuleManager() const noexcept { return ModuleMgr; }						//!< @copydoc DynExpCore::GetModuleManager()

		/**
		 * @brief Creates a DynExp::HardwareAdapterBase instance from a @p LibraryEntry.
		 * @param LibEntry Entry of #HardwareAdapterLib to instantiate
		 * @param Params Reference to the @p Object's parameters to take ownership of
		 * @return Returns the @p Object ID of the newly generated @p Object instance.
		*/
		ItemIDType MakeItem(const LibraryEntry<HardwareAdapterPtrType>& LibEntry, ParamsBasePtrType&& Params);
		
		/**
		 * @brief Creates a DynExp::InstrumentBase instance from a @p LibraryEntry.
		 * @param LibEntry Entry of #InstrumentLib to instantiate
		 * @param Params Reference to the @p Object's parameters to take ownership of
		 * @return Returns the @p Object ID of the newly generated @p Object instance.
		*/
		ItemIDType MakeItem(const LibraryEntry<InstrumentPtrType>& LibEntry, ParamsBasePtrType&& Params);
		
		/**
		 * @brief Creates a DynExp::ModuleBase instance from a @p LibraryEntry.
		 * @param LibEntry Entry of #ModuleLib to instantiate
		 * @param Params Reference to the @p Object's parameters to take ownership of
		 * @return Returns the @p Object ID of the newly generated @p Object instance.
		*/
		ItemIDType MakeItem(const LibraryEntry<ModulePtrType>& LibEntry, ParamsBasePtrType&& Params);

		/**
		 * @brief Indicates whether a %DynExp project has been loaded from a path specified as a
		 * command line argument when starting %DynExp. After the first call to this method,
		 * #LoadedProjectFromCommandlineParams will always be false.
		 * @return Returns #LoadedProjectFromCommandlineParams.
		*/
		bool HasLoadedProjectFromCommandlineParams() noexcept;

		/** @name Thread-safe public functions
		 * Methods can be called from any thread.
		*/
		///@{
		/**
		 * @brief Locks #Params and returns the current %DynExp project's filename.
		 * @param Timeout Time to wait for locking the mutex of #Params.
		 * @return Returns ProjectParams::ProjectFilename of #Params.
		*/
		auto GetProjectFilename(const std::chrono::milliseconds Timeout = GetParamsTimeoutDefault) const { return GetParams(Timeout)->ProjectFilename; }

		/**
		 * @brief Locks #Params and returns the path to a directory where modules can save data.
		 * @param Timeout Time to wait for locking the mutex of #Params.
		 * @return Returns ProjectParams::LastDataSaveDirectory of #Params.
		*/
		auto GetLastDataSaveDirectory(const std::chrono::milliseconds Timeout = GetParamsTimeoutDefault) const { return GetParams(Timeout)->LastDataSaveDirectory; }

		/**
		 * @brief Locks #Params and determines whether a project has been openend from a project file.
		 * @param Timeout Time to wait for locking the mutex of #Params.
		 * @return Returns true when ProjectParams::ProjectFilename of #Params is not empty, false otherwise.
		*/
		bool IsProjectOpened(const std::chrono::milliseconds Timeout = GetParamsTimeoutDefault) const noexcept { return !GetProjectFilename(Timeout).empty(); }

		/**
		 * @brief Transforms the path @p Path into an absolute path relative to ProjectParams::ProjectFilename.
		 * @param Path Relative path to be transformed
		 * @return Transformed path or @p Path itself if it is either empty or already absolute.
		*/
		std::filesystem::path ToAbsolutePath(const std::filesystem::path& Path) const;

		/**
		 * @brief Locks the mutex of the parameter class instance #Params assigned to the current project
		 * and returns a pointer to the locked #Params.
		 * @param Timeout Time to wait for locking the mutex of #Params.
		 * @return Returns a pointer to const @p ProjectParams, since only this class itself is allowed
		 * to modify #Params.
		*/
		ParamsConstTypeSyncPtrType GetParams(const std::chrono::milliseconds Timeout = GetParamsTimeoutDefault) const;

		/**
		 * @brief Getter for the thread id of the thread which constructed (and owns) this @p DynExpCore instance.
		 * @return Returns #OwnerThreadID.
		*/
		const auto GetOwnerThreadID() const noexcept { return OwnerThreadID; }
		///@}

		/**
		 * @brief Moves a Util::QWorker instance to #WorkerThread to run its Qt event queue there.
		 * This method is thread-safe since it is only called by the main thread after it constructed
		 * a DynExp::Object instance.
		 * @param Worker Util::QWorker instance to move to %DynExp's worker thread #WorkerThread.
		 * Refer to Util::QWorker and to #WorkerThread.
		 * @param ID @p Object ID of the DynExp::Object owning @p Worker.
		*/
		void MoveQWorkerToWorkerThread(Util::QWorker& Worker, ItemIDType ID) const;

		/**
		 * @copydoc DynExp::QModuleWidget::GetDataSaveDirectory
		 * @param Timeout Time to wait for locking the project parameters #Params.
		*/
		std::string GetDataSaveDirectory(const std::chrono::milliseconds Timeout = GetParamsTimeoutDefault) const;

		/**
		 * @copydoc DynExp::QModuleWidget::SetDataSaveDirectory
		 * @param Timeout Time to wait for locking the project parameters #Params.
		*/
		void SetDataSaveDirectory(const std::filesystem::path& Directory, const std::chrono::milliseconds Timeout = GetParamsTimeoutDefault);

	private:
		/**
		 * @brief Calls @p OpenProject() and performs error handling for that function.
		 * @param Filename Path of the project file to open.
		 * @return Returns true if a project has been opened successfully, false otherwise.
		*/
		bool OpenProjectSafe(const std::string& Filename) noexcept;

		/**
		 * @brief Creates a DynExp::Object instance from a @p LibraryEntry.
		 * @tparam LibEntryT Pointer to the base type of a certain %DynExp resource (e.g. DynExp::ModulePtrType)
		 * @tparam ParamsT Type of the parameter base class related to DynExp::Object instances of type @p LibEntryT
		 * @tparam ManagerT Type of the resource manager managing instances of type @p LibEntryT.
		 * @param LibEntry Library entry to instantiate
		 * @param Params Reference to the @p Object's parameters to take ownership of
		 * @param ResourceManager Resource manager to insert the newly generated @p Object instance into
		 * @param ItemTypeName Human-readable type name of the types managed by @p ResourceManager to be displayed
		 * in log messages.
		 * @return Returns the @p Object ID of the newly generated @p Object instance.
		*/
		template <typename LibEntryT, typename ParamsT, typename ManagerT>
		ItemIDType MakeItem(const LibraryEntry<LibEntryT>& LibEntry, ParamsT&& Params, ManagerT& ResourceManager, const std::string ItemTypeName);

		/**
		 * @copybrief GetParams(const std::chrono::milliseconds) const
		 * @param Timeout Time to wait for locking the mutex of #Params.
		 * @return Returns a pointer to @p ProjectParams (non-const) to allow access to all of its members.
		*/
		ParamsTypeSyncPtrType GetParams(const std::chrono::milliseconds Timeout = GetParamsTimeoutDefault);

		/**
		 * @brief Retrieves the module and main windows' positions, sizes and styles from the windows and
		 * updates the parameters stored in the current project's configuration #Params accordingly.
		 * @param MainWindow %DynExp's main window (@p DynExpManager instance) whose window state to save
		 * @param CircuitDiagramDlg %DynExp's circuit diagram window (@p CircuitDiagram instance) whose window state to save
		 * @param HSplitter Horizontal main splitter widget in the main window
		 * @param VSplitter Vertical main splitter widget in the main window
		*/
		void UpdateParamsFromWindowStates(const QMainWindow& MainWindow, const QDialog& CircuitDiagramDlg, QSplitter& HSplitter, QSplitter& VSplitter);

		const HardwareAdapterLibraryVectorType HardwareAdapterLib;	//!< Hardware adapter library vector containing information about all hardware adapters %DynExp knows.
		const InstrumentLibraryVectorType InstrumentLib;			//!< Instrument library vector containing information about all instruments %DynExp knows.
		const ModuleLibraryVectorType ModuleLib;					//!< Module library vector containing information about all modules %DynExp knows.

		/** @name Preserve declaration order
		 * The DynExp::Object managers must be declared in this order to ensure that all @p Object
		 * instances making use of other @p Object instances are destroyed first upon the
		 * destruction of the @p DynExpCore instance.
		*/
		///@{
		HardwareAdapterManager HardwareAdapterMgr;					//!< Hardware adapter manager owning all instantiated hardware adapters.
		InstrumentManager InstrumentMgr;							//!< Instrument manager owning all instantiated instruments.
		ModuleManager ModuleMgr;									//!< Module manager owning all instantiated modules.
		///@}

		/**
		 * @brief This flag will be set to true if @p DynExpCore has been initialized with a
		 * path to a project file to load passed as a command line option.
		*/
		bool LoadedProjectFromCommandlineParams;

		/**
		 * @brief Project parameters (configuration) of the current %DynExp project. Must never be @p nullptr.
		*/
		std::unique_ptr<ProjectParams> Params;

		/**
		 * @brief The ID is set by DynExpCore::DynExpCore() to the id of the thread which constructed
		 * the @p DynExpCore instance. This thread owns the instance. Ownership cannot be transferred
		 * to another thread. The owning thread must also be the user interface thread.
		*/
		const std::thread::id OwnerThreadID;

		/**
		 * @brief One worker thread runs the Qt event queues for all objects derived from Util::QWorker
		 * (e.g. hardware adapters derived from @p QSerialCommunicationHardwareAdapter). The derived
		 * Util::QWorker instances just insert tasks into the #WorkerThread's event queue. Those tasks
		 * get executed asynchronously by Qt. This is required since @p QObject instances (like
		 * @p QSerialPort) cannot be shared between different (instrument) threads. Instead, @p QObject
		 * instances have to belong to one thread only (unfortunately).
		 * @todo In the future, this might be extended to support multiple worker threads using some
		 * load balancing mechanism.
		*/
		mutable QThread WorkerThread;
	};

	template <typename LibEntryT, typename ParamsT, typename ManagerT>
	ItemIDType DynExpCore::MakeItem(const LibraryEntry<LibEntryT>& LibEntry, ParamsT&& Params, ManagerT& ResourceManager, const std::string ItemTypeName)
	{
		auto Item = LibEntry.ObjectFactoryPtr(GetOwnerThreadID(), std::move(Params));
		auto ID = ResourceManager.InsertResource(std::move(Item));
		auto Res = ResourceManager.GetResource(ID);

		Util::EventLog().Log(ItemTypeName + " \"" + Res->GetParams()->ObjectName.Get()
			+ "\" (" + Res->GetCategoryAndName() + ") has been created successfully.");

		return ID;
	}
}