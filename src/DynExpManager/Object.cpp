// This file is part of DynExp.

#include "stdafx.h"
#include "Object.h"
#include "DynExpCore.h"

namespace DynExp
{
	Util::ILockable::LockType ObjectUserList::AcquireLock(const std::chrono::milliseconds Timeout) const
	{
		return ILockable::AcquireLock(Timeout);
	}

	void ObjectUserList::Register(const Object& User, const std::chrono::milliseconds Timeout)
	{
		auto lock = AcquireLock(Timeout);

		RegisterUnsafe(User);
	}

	ObjectUserList::LockType ObjectUserList::Register(const Object& User, LockType&& Lock)
	{
		RegisterUnsafe(User);

		return std::move(Lock);
	}

	void ObjectUserList::Deregister(const Object& User, const std::chrono::milliseconds Timeout)
	{
		auto lock = AcquireLock(Timeout);

		DeregisterUnsafe(User);
	}

	size_t ObjectUserList::CountUsers(const std::chrono::milliseconds Timeout) const
	{
		auto lock = AcquireLock(Timeout);

		return CountUsersUnsafe();
	}

	ItemIDListType ObjectUserList::GetUserIDs(std::chrono::milliseconds Timeout) const
	{
		auto lock = AcquireLock(Timeout);

		return GetUserIDsUnsafe();
	}

	std::string ObjectUserList::GetUserNamesString(const std::chrono::milliseconds Timeout) const
	{
		auto lock = AcquireLock(Timeout);

		return GetUserNamesStringUnsafe();
	}

	void ObjectUserList::DeregisterAllUnsafe()
	{
		for (auto& User : UserList)
			User.first->CheckLinkedObjectStates();

		UserList.clear();
	}

	size_t ObjectUserList::CountUsersUnsafe() const
	{
		return std::accumulate<decltype(UserList.cbegin()), size_t>(UserList.cbegin(), UserList.cend(),
			0, [](size_t CurrentValue, const auto& User) {
			return CurrentValue + User.second;
		});
	}

	ItemIDListType ObjectUserList::GetUserIDsUnsafe() const
	{
		ItemIDListType IDs;

		for (const auto& User : UserList)
			IDs.push_back(User.first->GetID());

		return IDs;
	}

	std::string ObjectUserList::GetUserNamesStringUnsafe() const
	{
		std::string Names("Items making use of this item:");
		for (const auto& User : UserList)
			Names += "\n- " + User.first->GetObjectName() + " (" + User.first->GetCategoryAndName() + ")";

		return Names;
	}

	void ObjectUserList::RegisterUnsafe(const Object& User)
	{
		auto& Value = UserList[&User];

		if (Value == std::numeric_limits<std::remove_reference_t<decltype(Value)>>::max())
			throw Util::OverflowException("Cannot increment the user list counter since an overflow would occur.");

		++Value;
	}

	void ObjectUserList::DeregisterUnsafe(const Object& User)
	{
		auto ValueIt = UserList.find(&User);
		if (ValueIt == UserList.cend())
			return;

		if (ValueIt->second == 0)
			throw Util::UnderflowException("Cannot decrement the user list counter since an underflow would occur.");
		if (--ValueIt->second == 0)
			UserList.erase(ValueIt);
	}

	ParamsBase::ParamBase::ParamBase(ParamsBase& Owner,
		std::string ParamName, std::string_view ParamTitle, std::string_view ParamDescription, bool NeedsResetToApplyChange)
		: ParamsBaseOnly(*this), Owner(Owner), UserEditable(true),
		ParamName(ParamName), ParamTitle(ParamTitle), ParamDescription(ParamDescription), NeedsResetToApplyChange(NeedsResetToApplyChange)
	{
		// Since classes derived from ParamBase are declared within the scope of class ParamsBase, this is allowed.
		// This is safe since OwnedParams is declared before any member derived from ParamBase within the inheritance hierarchy.
		// Intended call to a virtual function (Owner.GetParamClassTag()) during construction of ParamsBase. See declaration of
		// ParamsBase::GetParamClassTag().
		Owner.OwnedParams.emplace_back(Owner.GetParamClassTag(), std::ref(*this));
	}

	ParamsBase::ParamBase::ParamBase(ParamsBase& Owner, std::string ParamName)
		: ParamsBaseOnly(*this), Owner(Owner), UserEditable(false),
		ParamName(ParamName), NeedsResetToApplyChange(false)
	{
		// See above.
		Owner.OwnedParams.emplace_back(Owner.GetParamClassTag(), std::ref(*this));
	}

	ParamsBase::ParamBase::~ParamBase()
	{
	}

	QDomElement ParamsBase::ParamBase::ToXMLNode(QDomDocument& Document) const
	{
		QDomElement Node = Document.createElement(GetParamName().data());
		ToXMLNodeChild(Document, Node);

		return Node;
	}

	void ParamsBase::ParamBase::FromXMLNode(const QDomElement& XMLElement)
	{
		try
		{
			FromXMLNodeChild(XMLElement);
		}
		catch ([[maybe_unused]] const Util::NotFoundException& e)
		{
			Reset();

			Util::EventLog().Log("Parameter \"" + std::string(GetParamName()) + "\" has not been found in configuration file. Assuming default value.",
				Util::ErrorType::Warning);
		}
		catch (...)
		{
			throw;
		}
	}

	bool ParamsBase::ParamBase::Validate()
	{
		bool IsValid = ValidateChild();

		if (!IsValid)
		{
			Reset();

			Util::EventLog().Log("Parameter \"" + std::string(GetParamName()) + "\" has been reset to its default value since it was invalid.",
				Util::ErrorType::Warning);
		}

		return IsValid;
	}

	bool ParamsBase::LinkParamStarter::operator()()
	{
		if (CurrentParam == Params.ObjectLinkParams.cend())
			return true;

		if (!EnsureReadyStateCalledForCurrentParam)
		{
			CurrentParam->get().EnsureReadyState();
			EnsureReadyStateCalledForCurrentParam = true;
		}
		else if (CurrentParam->get().IsReady())
		{
			++CurrentParam;
			EnsureReadyStateCalledForCurrentParam = false;
		}

		return false;
	}

	ParamsBase::~ParamsBase()
	{
	}

	QDomElement ParamsBase::ConfigToXML(QDomDocument& Document) const
	{
		QDomElement MainNode = Document.createElement(ParamsBase::GetParamClassTag());
		QDomElement CurrentNode = MainNode;	// shallow copy

		for (const auto& OwnedParam : OwnedParams)
		{
			if (CurrentNode.tagName() != OwnedParam.ClassTag)
			{
				QDomElement SubNode = Document.createElement(OwnedParam.ClassTag);
				CurrentNode = CurrentNode.appendChild(SubNode).toElement();
			}

			if (!OwnedParam.OwnedParam.get().GetParamName().empty())
				CurrentNode.appendChild(OwnedParam.OwnedParam.get().ToXMLNode(Document));
		}

		return MainNode;
	}

	void ParamsBase::ConfigFromXML(const QDomElement& XMLElement) const
	{
		QDomElement CurrentNode = Util::GetSingleChildDOMElement(XMLElement, ParamsBase::GetParamClassTag());

		for (const auto& OwnedParam : OwnedParams)
		{
			try
			{
				if (CurrentNode.tagName() != OwnedParam.ClassTag)
					CurrentNode = Util::GetSingleChildDOMElement(CurrentNode, OwnedParam.ClassTag);
			}
			catch ([[maybe_unused]] const Util::NotFoundException& e)
			{
				std::string MissingParamName(OwnedParam.OwnedParam.get().GetParamName());
				Util::EventLog().Log("Node \"" + std::string(OwnedParam.ClassTag) +
					"\" has not been found in configuration file." +
					(MissingParamName.empty() ? "" :
						(" Assuming default value for parameter \"" + MissingParamName + "\".")),
					Util::ErrorType::Warning);

				// Assume default values if hierarchy levels in the XML structure are missing.
				// Params classes without any parameters do get reflected in the XML hierarchy.
				OwnedParam.OwnedParam.get().Reset();

				continue;
			}
			catch (...)
			{
				throw;
			}

			if (!OwnedParam.OwnedParam.get().GetParamName().empty())
				OwnedParam.OwnedParam.get().FromXMLNode(CurrentNode);
		}

		Validate();
	}

	void ParamsBase::ConfigFromDialog(ParamsConfigDialog& Dialog)
	{
		ConfigureParams();

		for (const auto& OwnedParam : OwnedParams)
			if (OwnedParam.OwnedParam.get().IsUserEditable())
				OwnedParam.OwnedParam.get().ParamsBaseOnly.AddToDialog(Dialog);

		Validate();
	}

	bool ParamsBase::Validate() const
	{
		bool AllValid = true;

		std::for_each(OwnedParams.cbegin(), OwnedParams.cend(), [&AllValid](auto OwnedParams) {
			AllValid &= OwnedParams.OwnedParam.get().Validate();
		});

		return AllValid;
	}

	Util::TextValueListType<ParamsBase::UsageType> ParamsBase::AvlblUsageTypeStrList()
	{
		Util::TextValueListType<UsageType> List = {
			{ "Allow usage by only a single other item", UsageType::Unique },
			{ "Allow usage by multiple other items", UsageType::Shared }
		};

		return List;
	}

	void ParamsBase::DisableUserEditable(ParamBase& Param) noexcept
	{
		Param.ParamsBaseOnly.DisableUserEditable();
	}

	void ParamsBase::ConfigureParams()
	{
		if (!ConfigureUsageType())
			DisableUserEditable(Usage);

		ConfigureParamsImpl(dispatch_tag<ParamsBase>());
	}

	std::filesystem::path ParamsBase::ToAbsolutePath(const std::filesystem::path& Path) const
	{
		return GetCore().ToAbsolutePath(Path);
	}

	ConfiguratorBase::~ConfiguratorBase()
	{
	}

	ParamsBasePtrType ConfiguratorBase::MakeConfigFromDialog(ItemIDType ID, const DynExpCore& Core, QWidget* const DialogParent) const
	{
		auto Params = MakeParams(ID, Core);
		auto ConfigDlg = std::make_unique<ParamsConfigDialog>(DialogParent, Core, std::string("New ") + Params->ObjectName.Get());

		Params->ConfigFromDialog(*ConfigDlg);

		return ConfigDlg->Display() ? std::move(Params) : nullptr;
	}

	ParamsBasePtrType ConfiguratorBase::MakeConfigFromXML(ItemIDType ID, const DynExpCore& Core, const QDomElement& XMLElement) const
	{
		auto Params = MakeParams(ID, Core);

		Params->ConfigFromXML(XMLElement);

		return Params;
	}

	ConfiguratorBase::UpdateConfigFromDialogResult ConfiguratorBase::UpdateConfigFromDialog(Object* Obj, const DynExpCore& Core,
		QWidget* const DialogParent) const
	{
		std::unique_ptr<ParamsConfigDialog> ConfigDlg;

		{
			auto ParamsPtr = Obj->GetParams();
			ConfigDlg = std::make_unique<ParamsConfigDialog>(DialogParent, Core, std::string("Edit ") + ParamsPtr->ObjectName.Get());
			
			ParamsPtr->ConfigFromDialog(*ConfigDlg);
		} // Params unlocked here. ParamsConfigDialog::accept() locks itself again when user has accepted.

		auto Accepted = ConfigDlg->Display(Obj);

		return { Accepted, ConfigDlg->IsResetRequired() };
	}

	std::string Object::CategoryAndNameToStr(const std::string& Category, const std::string& Name)
	{
		if (Category.empty())
			return Name;
		else
			return Category + " -> " + Name;
	}

	Object::Object(const std::thread::id OwnerThreadID, ParamsBasePtrType&& Params)
		: LinkedObjectWrapperOnly(*this), OwnerThreadID(OwnerThreadID), Params(std::move(Params))
	{
		if (OwnerThreadID == std::thread::id())
			throw Util::InvalidArgException("OwnerThreadID is not a valid thread identifier.");

		if (!this->Params)
			throw Util::InvalidArgException("Params cannot be nullptr.");
	}

	Object::~Object()
	{
#ifdef DYNEXP_DEBUG
		Util::EventLog().Log("Item \"" + GetObjectName() + "\" is being removed.");
#endif // DYNEXP_DEBUG
	}

	void Object::LinkedObjectWrapperOnlyType::RegisterUser(const Object& User, const std::chrono::milliseconds Timeout) const
	{
		// Longer timeout in case a module is started which itself starts an instrument (empiric value...)
		// since the instrument might have locked its Params while starting up.
		// If longer timeout is not allowed, this might throw Util::TimeoutException.
		auto IsSharedUsage = Parent.IsSharedUsageEnabled(std::chrono::milliseconds(1000));

		// Now, race condition between Object::IsReady() and Object::BlockIfUnused() is avoided.
		auto lock = Parent.UserList.AcquireLock(Timeout);

		// Just a check for the moment, no guarantee, that the required object stays in a healthy state...
		if (!Parent.IsReady())
			throw Util::InvalidStateException("The required item is in an invalid state or in an error state.");

		if (!IsSharedUsage && !Parent.IsUnusedUnsafe())
			throw Util::NotAvailableException(
				"According to the item's \"Usage type\" setting, it can only be used by a single other item. Since it is already in use, it cannot be used by another item."
				+ std::string("\n\n") + Parent.UserList.GetUserNamesStringUnsafe(),
				Util::ErrorType::Error);

		Parent.UserList.Register(User, std::move(lock));
	}

	void Object::LinkedObjectWrapperOnlyType::DeregisterUser(const Object& User, const std::chrono::milliseconds Timeout) const
	{
		Parent.UserList.Deregister(User, Timeout);
	}

	void Object::Reset()
	{
		EnsureCallFromOwningThread();

		// Now, objects trying to make use of this object cannot register themselves here anymore.
		// They have to wait until reset is done.
		auto lock = UserList.AcquireLock();

		if (!IsUnusedUnsafe())
			throw Util::NotAvailableException(
				"This item is currently being used by at least another item. Stop and reset these items before resetting this item."
				+ std::string("\n\n") + UserList.GetUserNamesStringUnsafe());

		ClearWarning();

		ResetImpl(dispatch_tag<Object>());
	}

	void Object::BlockIfUnused(const std::chrono::milliseconds Timeout)
	{
		EnsureCallFromOwningThread();

		// Now, race condition between Object::IsReady() and Object::BlockIfUnused() is avoided.
		auto lock = UserList.AcquireLock(Timeout);

		if (!IsUnusedUnsafe())
			throw Util::NotAvailableException(
				"This item is currently being used by at least another item. Stop and reset these items before deleting this item."
				+ std::string("\n\n") + UserList.GetUserNamesStringUnsafe());

		IsBlocked = true;
	}

	Object::ParamsConstTypeSyncPtrType Object::GetParams(const std::chrono::milliseconds Timeout) const
	{
		return ParamsConstTypeSyncPtrType(Params.get(), Timeout);
	}

	Object::ParamsTypeSyncPtrType Object::GetParams(const std::chrono::milliseconds Timeout)
	{
		return ParamsTypeSyncPtrType(Params.get(), Timeout);
	}

	void Object::EnsureReadyState(bool IsAutomaticStartup)
	{
		// EnsureCallFromOwningThread(); does not work here because hardware adapters are connected asynchronously
		// from another thread in DynExpManager::OnRunProject().

		EnsureReadyStateChild(IsAutomaticStartup);
	}

	void Object::SetWarning(std::string Description, int ErrorCode) const
	{
		Warning = Util::Warning(std::move(Description), ErrorCode);

		LogWarning();
	}

	void Object::SetWarning(const Util::Exception& e) const
	{
		Warning = e;

		LogWarning();
	}

	void Object::EnsureCallFromOwningThread() const
	{
		if (std::this_thread::get_id() != OwnerThreadID)
			throw Util::InvalidCallException(
				"This function must be called from the thread managing this object. A call from another instrument or module thread is not supported.");
	}

	Object::ParamsTypeSyncPtrType Object::GetNonConstParams(const std::chrono::milliseconds Timeout) const
	{
		return ParamsTypeSyncPtrType(Params.get(), Timeout);
	}

	void Object::LogWarning() const
	{
		Util::EventLog().Log(Warning);
	}

	RunnableObjectParams::~RunnableObjectParams()
	{
	}

	Util::TextValueListType<RunnableObjectParams::StartupType> RunnableObjectParams::AvlblStartupTypeStrList()
	{
		Util::TextValueListType<StartupType> List = {
			{ "Start item as soon as it is created", RunnableObjectParams::StartupType::OnCreation },
			{ "Start item as soon as it is required by another item", RunnableObjectParams::StartupType::Automatic },
			{ "Start item only manually", RunnableObjectParams::StartupType::Manual }
		};

		return List;
	}

	void RunnableObjectParams::ConfigureParamsImpl(dispatch_tag<ParamsBase>)
	{
		if (!ConfigureStartupType())
			DisableUserEditable(Startup);

		ConfigureParamsImpl(dispatch_tag<RunnableObjectParams>());
	}

	RunnableObjectConfigurator::~RunnableObjectConfigurator()
	{
	}

	std::string RunnableObject::NotUnusedException::GetErrorMessage() const
	{
		return "This item is currently being used by at least another item. Stop and reset these items before stopping this item."
			+ std::string("\n\n") + GetUserNames().data();
	}

	RunnableObject::RunnableObject(const std::thread::id OwnerThreadID, ParamsBasePtrType&& Params)
		: Object(OwnerThreadID, std::move(Params)), RunnableInstanceOnly(*this)
	{
		Init();
	}

	RunnableObject::~RunnableObject()
	{
		try
		{
			// Better call terminate before destruction manually to handle errors.
			TerminateImpl(false);
		}
		catch (const Util::TimeoutException& e)
		{
			Util::EventLog().Log(e);
			Util::EventLog().Log("Could not terminate thread in the runnable's destructor. Timeout occurred. Execution cannot continue.",
				Util::ErrorType::Fatal);

			std::terminate();
		}
		catch (const Util::NotAvailableException& e)
		{
			// Should never happen since modules are destroyed first in DynExpCore.
			Util::EventLog().Log(e);
			Util::EventLog().Log("Could not terminate thread in the runnable's destructor. The runnable is still in use. Execution cannot continue.",
				Util::ErrorType::Fatal);

			std::terminate();
		}
	}

	bool RunnableObject::Run(QWidget* ParentWidget)
	{
		EnsureCallFromOwningThread();

		if (GetException())
			throw Util::InvalidStateException(
				"A runnable is in an error state. It requires to be reset in order to transition into a ready state.");

		auto StartupDialog = MakeStartupBusyDialogChild(ParentWidget);
		int Result = QDialog::Accepted;
		if (StartupDialog)
		{
			auto Params = GetParams();
			ParamsBase::LinkParamStarter LinkParamStarter(*Params);
			StartupDialog->SetCheckFinishedFunction(LinkParamStarter);

			// StartupDialog->exec() blocks and starts new event loop for the modal dialog (in the same thread).
			Result = StartupDialog->exec();
		}

		if (StartupDialog && Result != QDialog::Accepted)
		{
			if (StartupDialog->GetException())
				std::rethrow_exception(StartupDialog->GetException());

			return false;
		}
		if (IsRunning())
			return false;

		// Always call Reset() before starting to allow using Reset() for initialization.
		// 'if (IsExiting())' could in principle be used here to determine whether the
		// RunnableObject is not being started for the first time.
		Reset();

		auto Params = dynamic_Params_cast<RunnableObject>(GetParams());
		Startup = Params->Startup;

		RunChild();
		Running = true;

		return true;
	}

	bool RunnableObject::RunIfRunAutomatic()
	{
		auto Params = dynamic_Params_cast<RunnableObject>(GetParams());

		if (Params->Startup == RunnableObjectParams::StartupType::Automatic)
			return Run();
		else if (!IsRunning())
			throw Util::NotAvailableException("The required item " + Params->ObjectName.Get() + " (" + GetCategoryAndName() +
				") cannot be started since automatic startup has been disabled for this item.", Util::ErrorType::Error);

		return false;
	}

	bool RunnableObject::RunIfRunOnCreation()
	{
		auto Params = dynamic_Params_cast<RunnableObject>(GetParams());

		if (Params->Startup == RunnableObjectParams::StartupType::OnCreation)
			return Run();

		return false;
	}

	void RunnableObject::Terminate(bool Force, const std::chrono::milliseconds Timeout)
	{
		EnsureCallFromOwningThread();

		TerminateImpl(Force, Timeout);
	}

	void RunnableObject::SetPaused(bool Pause, std::string Description)
	{
		Paused = Pause;

		if (Pause)
			SetReasonWhyPaused(std::move(Description));
		else
			ClearReasonWhyPaused();
	}

	void RunnableObject::Init()
	{
		Running = false;
		Paused = false;
		ReasonWhyPaused.Reset();
		ShouldExit = false;
		LinkedObjStateCheckRequested = false;
	}

	std::promise<void> RunnableObject::MakeThreadExitedPromise()
	{
		std::promise<void> ThreadExitedPromise;
		ThreadExitedSignal = ThreadExitedPromise.get_future();

		return ThreadExitedPromise;
	}

	void RunnableObject::StoreThread(std::thread&& Thread) noexcept
	{
		this->Thread = std::move(Thread);
	}

	bool RunnableObject::IsCallFromRunnableThread() const
	{
		return std::this_thread::get_id() == Thread.get_id();
	}

	void RunnableObject::EnsureCallFromRunnableThread() const
	{
		if (!IsCallFromRunnableThread())
			throw Util::InvalidCallException(
				"This function must be called from this runnable's thread. A call from another thread is not supported.");
	}

	void RunnableObject::ResetImpl(dispatch_tag<Object>)
	{
		TerminateUnsafe(false);

		ResetImpl(dispatch_tag<RunnableObject>());
		Init();
	}

	void RunnableObject::EnsureReadyStateChild(bool IsAutomaticStartup)
	{
		IsAutomaticStartup ? RunIfRunAutomatic() : RunIfRunOnCreation();
	}

	void RunnableObject::TerminateImpl(bool Force, const std::chrono::milliseconds Timeout)
	{
		// Now, objects trying to make use of this object cannot register themselves here anymore.
		auto lock = LockUserList();
		auto IsUnused = IsUnusedUnsafe();

		if (!Force && !IsUnused)
			throw NotUnusedException(*this);

		if (!IsUnused)
			DeregisterAllUnsafe();
		TerminateUnsafe(Force, Timeout);
	}

	void RunnableObject::TerminateUnsafe(bool Force, const std::chrono::milliseconds Timeout)
	{
		if (!Thread.joinable())
			return;

		TerminateChild(Timeout);

		ShouldExit = true;
		NotifyChild();
		if (ThreadExitedSignal.wait_for(Timeout) != std::future_status::ready)
			throw Util::ThreadDidNotRespondException();

		Thread.join();
		Thread = std::thread();
		Running = false;
		Paused = false;
	}

	void RunnableObject::OnThreadHasExited() noexcept
	{
		// Just indicate status changes. All the clean up is performed in the main thread when
		// 1) Terminate() is called (calls TerminateUnsafe(), Thread is still joinable)
		// 2) Reset() is called (calls TerminateUnsafe())
		// 3) Run() is called (it calls Reset())
		// There is no race condition since TerminateChild() also sets Running to false and
		// Running is not read in between (IsRunning() is not called).
		ShouldExit = true;
		Running = false;
		Paused = false;

		// This is necessary if the RunnableObject is not terminated regularly but by an error.
		DeregisterAllUnsafe();
	}

	LinkedObjectWrapperBase::~LinkedObjectWrapperBase()
	{
	}

	const Object& LinkedObjectWrapperBase::GetOwner() const noexcept
	{
		return Owner.GetOwner();
	}

	LinkedObjectWrapperContainerBase::~LinkedObjectWrapperContainerBase()
	{
	}

	void LinkedObjectWrapperContainerBase::Reset() noexcept
	{
		ResetChild();

		LinkedObjectState = LinkedObjectStateType::NotLinked;
	}

	ObjectLinkBase::~ObjectLinkBase()
	{
	}

	RunnableInstance::RunnableInstance(RunnableObject& Owner, std::promise<void>&& ThreadExitedPromise)
		: ParamsGetter({ Owner, &Object::GetParams, { Object::GetParamsTimeoutDefault } }),
		Owner(Owner), ThreadExitedPromise(std::move(ThreadExitedPromise))
	{
	}

	// Not noexcept since move-constructor of std::list is not noexcept.
	DynExp::RunnableInstance::RunnableInstance(RunnableInstance&& Other)
		: ParamsGetter(Other.ParamsGetter), Owner(Other.Owner), ThreadExitedPromise(std::move(Other.ThreadExitedPromise)),
		OwnedLinkedObjectWrappers(std::move(Other.OwnedLinkedObjectWrappers))
	{
		Other.Empty = true;
	}

	RunnableInstance::~RunnableInstance()
	{
		SetThreadExited();

		std::for_each(OwnedLinkedObjectWrappers.cbegin(), OwnedLinkedObjectWrappers.cend(), [](const auto& i) {
			i.OwnedLinkedObjectWrapperContainer.Reset();
		});
	}

	bool RunnableInstance::CareAboutWrappers()
	{
		bool AnyDestinyNotReady = false;

		if (GetOwner().RunnableInstanceOnly.IsLinkedObjStateCheckRequested())
		{
			GetOwner().RunnableInstanceOnly.ResetLinkedObjStateCheckRequested();

			for (auto& Wrapper : OwnedLinkedObjectWrappers)
				Wrapper.OwnedLinkedObjectWrapperContainer.CheckIfReady();
		}

		for (auto& Wrapper : OwnedLinkedObjectWrappers)
		{
			if (Wrapper.OwnedLinkedObjectWrapperContainer.GetState() == LinkedObjectWrapperContainerBase::LinkedObjectStateType::NotReady)
			{
				AnyDestinyNotReady = true;

				if (Wrapper.OwnedLinkedObjectWrapperPtr->IsRegistered())
					Wrapper.OwnedLinkedObjectWrapperPtr->Deregister(std::chrono::milliseconds(100));
				else
				{
					try
					{
						Wrapper.OwnedLinkedObjectWrapperPtr->Register(ObjectLinkBase::LockObjectTimeoutDefault);
						Wrapper.OwnedLinkedObjectWrapperContainer.LinkedObjectState = LinkedObjectWrapperContainerBase::LinkedObjectStateType::Ready;
					}
					catch (...)
					{
						// Registering failed because underlying object is still in an error state.
						// Swallow the exception and try again next time this function is called.
					}
				}
			}
		}

		return !AnyDestinyNotReady;
	}

	std::string RunnableInstance::GetNotReadyObjectNamesString() const
	{
		std::string Names("Linked items not being in a ready state:");
		for (auto& Wrapper : OwnedLinkedObjectWrappers)
			if (Wrapper.OwnedLinkedObjectWrapperContainer.GetState() == LinkedObjectWrapperContainerBase::LinkedObjectStateType::NotReady)
				Names += "\n- " + Wrapper.OwnedLinkedObjectWrapperContainer.GetLinkedObjectDesc();

		return Names;
	}

	void RunnableInstance::SetThreadExited()
	{
		if (!Empty)
		{
			// ThreadExitedPromise.set_value_at_thread_exit(); does not work, because
			// ThreadExitedSignal.wait_for(Timeout) in InstrumentBase::Terminate() would not
			// return in case of timeout since the value is already stored.
			ThreadExitedPromise.set_value();

			GetOwner().RunnableInstanceOnly.OnThreadHasExited();
		}
	}
}