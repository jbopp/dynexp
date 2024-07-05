// This file is part of DynExp.

#include "stdafx.h"
#include "DynExpCore.h"

namespace DynExp
{
	Util::TextValueListType<ProjectParams::StoreWindowStatesType> ProjectParams::AvlblStoreWindowStatesTypeStrList()
	{
		Util::TextValueListType<StoreWindowStatesType> List = {
			{ "Store in project file and apply from file", ProjectParams::StoreWindowStatesType::ApplyStoredWindowStates },
			{ "Don't store in project file but apply defaults", ProjectParams::StoreWindowStatesType::ApplyDefaultWindowStates }
		};

		return List;
	}

	void ProjectParams::ConfigureParamsImpl(dispatch_tag<ParamsBase>)
	{
		DisableUserEditable(Usage);
	}

	DynExpCore::DynExpCore(HardwareAdapterLibraryVectorType HardwareAdapterLib, InstrumentLibraryVectorType InstrumentLib,
		ModuleLibraryVectorType ModuleLib, std::string ProjectFileToOpen)
		: HardwareAdapterLib(std::move(HardwareAdapterLib)), InstrumentLib(std::move(InstrumentLib)),
		ModuleLib(std::move(ModuleLib)),
		Params(std::make_unique<ProjectParams>(*this)), OwnerThreadID(std::this_thread::get_id())
	{
		// Init Util::QWorker by setting its internal pointer to this DynExpCore instance.
		Util::QWorker::GetDynExpCore(this);

		// Start WorkerThread (see DynExpCore.h).
		WorkerThread.start();

		Util::EventLog().OpenLogFile("DynExp.html");
		Util::EventLog().Log("Welcome to DynExp " + std::string(DynExpVersion) + ".");

#ifdef DYNEXP_DEBUG
		Util::EventLog().Log("DynExp was compiled in DEBUG mode and might thus run slowly.");
#endif // DYNEXP_DEBUG

		OpenProjectSafe(ProjectFileToOpen);
	}

	DynExpCore::~DynExpCore()
	{
		Util::EventLog().Log("DynExp shut down successfully.");
	}

	void DynExpCore::Shutdown()
	{
		WorkerThread.quit();

		if (!WorkerThread.wait(std::chrono::milliseconds(3000)))
			throw Util::ThreadDidNotRespondException();
	}

	void DynExpCore::Reset(bool Force)
	{
		// Calls to PrepareReset() and Reset() must be performed in this order to ensure that
		// all objects making use of another object are destroyed first in case of reset.

		if (!Force)
		{
			ModuleMgr.PrepareReset();
			InstrumentMgr.PrepareReset();
			HardwareAdapterMgr.PrepareReset();
		}

		ModuleMgr.Reset();
		InstrumentMgr.Reset();
		HardwareAdapterMgr.Reset();

		GetParams()->ProjectFilename.clear();
		Params = std::make_unique<ProjectParams>(*this);
		
		if (!Force)
			Util::EventLog().Log("Set up new project successfully.");
	}

	void DynExpCore::SaveProject(std::string_view Filename, const QMainWindow& MainWindow, const QDialog& CircuitDiagramDlg, QSplitter& HSplitter, QSplitter& VSplitter)
	{
		QDomDocument Document;
		Document.appendChild(Document.createProcessingInstruction("xml", "version=\"1.0\" standalone=\"yes\""));
		QDomElement RootNode = Document.createElement("DynExp");
		RootNode.setAttribute("DynExpVersion", DynExp::DynExpVersion);
		QDomElement ProjectNode = Document.createElement("Project");

		UpdateParamsFromWindowStates(MainWindow, CircuitDiagramDlg, HSplitter, VSplitter);
		QDomElement ParamsNode = Document.createElement("Params");
		ParamsNode.appendChild(GetParams()->ConfigToXML(Document));
		ProjectNode.appendChild(ParamsNode);

		ProjectNode.appendChild(HardwareAdapterMgr.EntryConfigsToXML(Document));
		ProjectNode.appendChild(InstrumentMgr.EntryConfigsToXML(Document));
		ProjectNode.appendChild(ModuleMgr.EntryConfigsToXML(Document));

		RootNode.appendChild(ProjectNode);
		Document.appendChild(RootNode);

		auto DocumentString = Document.toString().toStdString();

		std::ofstream File;
		File.exceptions(std::ofstream::failbit | std::ofstream::badbit);
		File.open(std::string(Filename), std::ofstream::out | std::ofstream::trunc);
		File.write(DocumentString.c_str(), DocumentString.length());
		File.close();

		GetParams()->ProjectFilename = Filename;

		Util::EventLog().Log(std::string("Saved project successfully to ").append(Filename) + ".");
	}

	void DynExpCore::OpenProject(std::string_view Filename)
	{
		std::ifstream File;
		std::string Contents;
		File.exceptions(std::ifstream::failbit | std::ifstream::badbit);
		File.open(std::string(Filename), std::ifstream::in | std::ifstream::binary);
		File.seekg(0, std::ios::end);
		Contents.resize(File.tellg());
		File.seekg(0, std::ios::beg);
		File.read(&Contents[0], Contents.size());
		File.close();

		QDomDocument Document;
		QString ErrorMsg;
		int Line = 0, Column = 0;
		if (!Document.setContent(QString::fromStdString(Contents), false, &ErrorMsg, &Line, &Column))
			throw Util::InvalidDataException("Error parsing the specified project file at line "
				+ Util::ToStr(Line) + ", column " + Util::ToStr(Column) + ": " + ErrorMsg.toStdString());

		auto RootNode = Document.documentElement();
		auto ProjectNode = Util::GetSingleChildDOMElement(RootNode, "Project");
		auto Version = Util::VersionFromString(Util::GetStringFromDOMAttribute(RootNode, "DynExpVersion"));

		if (Version > Util::VersionType{0, 2})
		{
			auto ParamsNode = Util::GetSingleChildDOMElement(ProjectNode, "Params");
			GetParams()->ConfigFromXML(ParamsNode);
		}

		auto HardwareAdapterNode = Util::GetSingleChildDOMElement(ProjectNode, "HardwareAdapters");
		auto InstrumentNode = Util::GetSingleChildDOMElement(ProjectNode, "Instruments");
		auto ModuleNode = Util::GetSingleChildDOMElement(ProjectNode, "Modules");

		HardwareAdapterMgr.MakeEntriesFromXML(HardwareAdapterNode, HardwareAdapterLib, *this);
		InstrumentMgr.MakeEntriesFromXML(InstrumentNode, InstrumentLib, *this);
		ModuleMgr.MakeEntriesFromXML(ModuleNode, ModuleLib, *this);

		GetParams()->ProjectFilename = Filename;

		Util::EventLog().Log(std::string("Loaded project from ").append(Filename) + " successfully.");
	}

	void DynExpCore::EditProjectSettings(QWidget* const DialogParent)
	{
		auto ConfigDlg = std::make_unique<ParamsConfigDialog>(DialogParent, *this, "Project settings");

		GetParams()->ConfigFromDialog(*ConfigDlg);
		ConfigDlg->Display();
	}

	bool DynExpCore::AllHardwareAdaptersConnected() const
	{
		return HardwareAdapterMgr.AllConnected();
	}

	bool DynExpCore::AllInstrumentsInitialized() const
	{
		return InstrumentMgr.AllInitialized();
	}

	void DynExpCore::RunInstruments(CommonResourceManagerBase::FunctionToCallWhenObjectStartedType FunctionToCallWhenInstrumentStarted)
	{
		InstrumentMgr.Startup(FunctionToCallWhenInstrumentStarted);
	}

	void DynExpCore::RunModules(CommonResourceManagerBase::FunctionToCallWhenObjectStartedType FunctionToCallWhenModuleStarted)
	{
		ModuleMgr.Startup(FunctionToCallWhenModuleStarted);
	}

	void DynExpCore::ShutdownProject()
	{
		ModuleMgr.Shutdown();
		InstrumentMgr.Shutdown();
	}

	void DynExpCore::ResetFailedItems(QWidget& ParentWindow)
	{
		ModuleMgr.ResetFailedResources();
		ModuleMgr.ClearResourcesWarnings();

		InstrumentMgr.ResetFailedResources();
		InstrumentMgr.ClearResourcesWarnings();

		auto FailedAndUsedHardwareAdapters = HardwareAdapterMgr.GetFailedResourceIDs(true);
		ItemIDListType InstrumentsToRestart;
		for (const auto AdapterID : FailedAndUsedHardwareAdapters)
		{
			auto Users = HardwareAdapterMgr.GetResource(AdapterID)->GetUserIDs();
			InstrumentsToRestart.insert(InstrumentsToRestart.cend(), Users.cbegin(), Users.cend());
		}

		if (!InstrumentsToRestart.empty())
		{
			std::string InstrumentsToRestartNames("The following instruments will be stopped and restarted:");
			for (const auto InstrID : InstrumentsToRestart)
			{
				auto Instr = InstrumentMgr.GetResource(InstrID);
				InstrumentsToRestartNames += "\n- " + Instr->GetObjectName() + " (" + Instr->GetCategoryAndName() + ")";
			}
			InstrumentsToRestartNames += "\n\nDo you wish to continue?";
			if (QMessageBox::question(&ParentWindow, "DynExp - Restart instruments?", QString::fromStdString(InstrumentsToRestartNames),
				QMessageBox::StandardButton::Yes | QMessageBox::StandardButton::No, QMessageBox::StandardButton::No)
				!= QMessageBox::StandardButton::Yes)
				return;
		}

		for (const auto InstrID : InstrumentsToRestart)
			InstrumentMgr.GetResource(InstrID)->Terminate(true);

		HardwareAdapterMgr.ResetFailedResources();
		HardwareAdapterMgr.ClearResourcesWarnings();

		for (const auto InstrID : InstrumentsToRestart)
			dynamic_cast<RunnableObject&>(*InstrumentMgr.GetResource(InstrID)).Run(&ParentWindow);
	}

	void DynExpCore::RestoreWindowStatesFromParams(QMainWindow& MainWindow, QDialog& CircuitDiagramDlg, QSplitter& HSplitter, QSplitter& VSplitter,
		bool OnlyMainWindow)
	{
		{
			auto Params = GetParams();

			if (Params->StoreWindowStates != ProjectParams::StoreWindowStatesType::ApplyStoredWindowStates)
				return;

			Params->MainWindowStyleParams.ApplyTo(MainWindow);
			Params->CircuitWindowStyleParams.ApplyTo(CircuitDiagramDlg,
				Params->CircuitWindowStyleParams.WindowDockingState == WindowStyleParamsExtension::WindowDockingStateType::Undocked);
			if (Params->CircuitWindowStyleParams.WindowDockingState == WindowStyleParamsExtension::WindowDockingStateType::Docked)
				CircuitDiagramDlg.hide();

			if (Params->HSplitterWidgetWidths.Get().size() >= Util::NumToT<size_t>(HSplitter.sizes().size()))
				HSplitter.setSizes(QList<int>::fromVector(QVector<int>(Params->HSplitterWidgetWidths.Get().cbegin(), Params->HSplitterWidgetWidths.Get().cend())));
			if (Params->VSplitterWidgetHeights.Get().size() >= Util::NumToT<size_t>(VSplitter.sizes().size()))
				VSplitter.setSizes(QList<int>::fromVector(QVector<int>(Params->VSplitterWidgetHeights.Get().cbegin(), Params->VSplitterWidgetHeights.Get().cend())));
		} // Params unlocked here.

		if (!OnlyMainWindow)
			ModuleMgr.RestoreWindowStatesFromParams();
	}

	ItemIDType DynExpCore::MakeItem(const LibraryEntry<HardwareAdapterPtrType>& LibEntry, ParamsBasePtrType&& Params)
	{
		return MakeItem(LibEntry, std::move(Params), GetHardwareAdapterManager(), "Hardware adapter");
	}

	ItemIDType DynExpCore::MakeItem(const LibraryEntry<InstrumentPtrType>& LibEntry, ParamsBasePtrType&& Params)
	{
		return MakeItem(LibEntry, std::move(Params), GetInstrumentManager(), "Instrument");
	}

	ItemIDType DynExpCore::MakeItem(const LibraryEntry<ModulePtrType>& LibEntry, ParamsBasePtrType&& Params)
	{
		return MakeItem(LibEntry, std::move(Params), GetModuleManager(), "Module");
	}

	bool DynExpCore::HasLoadedProjectFromCommandlineParams() noexcept
	{
		bool LoadedProjectFromCommandlineParamsCopy = LoadedProjectFromCommandlineParams;
		LoadedProjectFromCommandlineParams = false;

		return LoadedProjectFromCommandlineParamsCopy;
	}

	std::filesystem::path DynExpCore::ToAbsolutePath(const std::filesystem::path& Path) const
	{
		return (Path.is_relative() && !Path.empty()) ?
			(GetProjectFilename().remove_filename() / Path) : Path;
	}

	DynExpCore::ParamsConstTypeSyncPtrType DynExpCore::GetParams(const std::chrono::milliseconds Timeout) const
	{
		return ParamsConstTypeSyncPtrType(Params.get(), Timeout);
	}

	void DynExpCore::MoveQWorkerToWorkerThread(Util::QWorker& Worker, ItemIDType ID) const
	{
		Worker.moveToThread(&WorkerThread);
		QObject::connect(&WorkerThread, &QThread::finished, &Worker, &QObject::deleteLater);

		Worker.SetOwner(std::weak_ptr(GetHardwareAdapterManager().ShareResource(ID)));
	}

	std::string DynExpCore::GetDataSaveDirectory(const std::chrono::milliseconds Timeout) const
	{
		auto LastDataSaveDirectory = GetLastDataSaveDirectory(Timeout);
		if (!LastDataSaveDirectory.empty() && std::filesystem::exists(LastDataSaveDirectory))
			return LastDataSaveDirectory.string();

		auto ProjectFilename = GetProjectFilename(Timeout);
		return ProjectFilename.empty() ? "" : ProjectFilename.parent_path().string();
	}

	void DynExpCore::SetDataSaveDirectory(const std::filesystem::path& Directory, const std::chrono::milliseconds Timeout)
	{
		GetParams(Timeout)->LastDataSaveDirectory = Directory.parent_path();
	}

	bool DynExpCore::OpenProjectSafe(const std::string& Filename) noexcept
	{
		if (Filename.empty())
			return false;

		std::string ErrorMessage = "";
		try
		{
			OpenProject(Filename);
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
			Util::EventLog().Log("Opening a project from file " + Filename + ", the following error occurred: " + ErrorMessage,
				Util::ErrorType::Error);

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

			return false;
		}

		return true;
	}

	DynExpCore::ParamsTypeSyncPtrType DynExpCore::GetParams(const std::chrono::milliseconds Timeout)
	{
		return ParamsTypeSyncPtrType(Params.get(), Timeout);
	}

	void DynExpCore::UpdateParamsFromWindowStates(const QMainWindow& MainWindow, const QDialog& CircuitDiagramDlg, QSplitter& HSplitter, QSplitter& VSplitter)
	{
		{
			auto Params = GetParams();

			if (Params->StoreWindowStates != ProjectParams::StoreWindowStatesType::ApplyStoredWindowStates)
				return;

			Params->MainWindowStyleParams.FromWidget(MainWindow);
			Params->CircuitWindowStyleParams.FromWidget(CircuitDiagramDlg);
			Params->CircuitWindowStyleParams.WindowDockingState = CircuitDiagramDlg.isVisible() ?
				WindowStyleParamsExtension::WindowDockingStateType::Undocked : WindowStyleParamsExtension::WindowDockingStateType::Docked;

			const auto HSplitterSizes = HSplitter.sizes().toVector();
			const auto VSplitterSizes = VSplitter.sizes().toVector();
			Params->HSplitterWidgetWidths = std::vector<int>(HSplitterSizes.cbegin(), HSplitterSizes.cend());
			Params->VSplitterWidgetHeights = std::vector<int>(VSplitterSizes.cbegin(), VSplitterSizes.cend());
		} // Params unlocked here.

		ModuleMgr.UpdateParamsFromWindowStates();
	}
}