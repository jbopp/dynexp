// This file is part of DynExp.

/**
 * @file Managers.h
 * @brief Implementation of %DynExp's resource managers to manage %DynExp objects.
*/

#pragma once

#include "stdafx.h"
#include "HardwareAdapter.h"
#include "Instrument.h"
#include "Module.h"

namespace DynExp
{
	/**
	 * @brief Defines a %DynExp resource, which mainly owns a DynExp::Object instance
	 * wrapping a pointer to it.
	 * @tparam PointerType Type of the pointer pointing to the DynExp::Object instance
	*/
	template <typename PointerType>
	struct Resource
	{
		/**
		 * @brief Constructs a @p Resource instance. 
		 * @tparam ResourcePointerT Type of the pointer to perfectly forward to the constructor
		 * of #ResourcePointer.
		 * @param ResourcePointer Pointer to a %DynExp object (e.g. DynExp::ModulePtrType) to
		 * initialize #ResourcePointer with. 
		*/
		template <typename ResourcePointerT>
		Resource(ResourcePointerT&& ResourcePointer)
			: ResourcePointer(std::forward<ResourcePointerT>(ResourcePointer)) {}

		PointerType ResourcePointer;	//!< Pointer to the DynExp::Object instance owned by this resource.

		/**
		 * @brief For visualization of the resource and its state in DynExp's main window
		 * (@p DynExpManager). Can be modified by @p DynExpManager even for @p const @p Resource
		 * instances since @p DynExpManager manages this @p QTreeWidgetItem.
		*/
		mutable std::unique_ptr<QTreeWidgetItem> TreeWidgetItem;
	};

	/**
	 * @brief Common base class for all derived @p ResourceManagerBase classes.
	 * Logical const-ness: Only const functions can be called from objects possessing
	 * a const handle to the @p DynExpCore instance. To protect the resource manager, some functions
	 * which should be non-const by that argument (ResourceManagerBase::Startup(),
	 * ResourceManagerBase::Shutdown(), ResourceManagerBase::ResetFailedResources())
	 * are still const. These functions indirectly call Object::EnsureCallFromOwningThread()
	 * to ensure that they are only called by the main user interface thread (from the
	 * @p DynExpManager instance through its @p DynExpCore instance). Also refer to @p DynExpCore.
	*/
	class CommonResourceManagerBase
	{
	public:
		/**
		 * @brief Type of a callback function to invoke after a resource has been started
		 * in a call to ResourceManagerBase::Startup(). The DynExp::Object instance owned
		 * by the respective resource is passed to the callback function. Refer e.g. to
		 * DynExpManager::RegisterModuleUI(), which is such a callback function.
		*/
		using FunctionToCallWhenObjectStartedType = const std::function<void(Object* const)>;

	protected:
		CommonResourceManagerBase() = default;
		~CommonResourceManagerBase() = default;

		/**
		 * @brief Getter for the thread id of the thread which constructed (and owns) @p Core.
		 * Implementation necessary in Managers.cpp to prevent cyclic includes caused by usage of
		 * @p DynExpCore in ResourceManagerBase::MakeEntriesFromXML().
		 * @param Core Reference to %DynExp's core
		 * @return Returns the result of DynExpCore::GetOwnerThreadID() invoked on @p Core.
		*/
		static std::thread::id GetOwnerThreadID(const DynExpCore& Core) noexcept;
	};

	/**
	 * @brief Typed resource manager base class deriving from class @p CommonResourceManagerBase.
	 * @tparam PointerType Type of the pointers pointing to the managed DynExp::Object instances
	 */
	template <typename PointerType>
	class ResourceManagerBase : public CommonResourceManagerBase
	{
		/**
		 * @brief Allow exclusive access to some of @p ResourceManagerBase's private methods to @p LinkBase.
		*/
		class LinkBaseOnlyType
		{
			friend class ResourceManagerBase;
			friend class LinkBase;

			/**
			 * @brief Construcs an instance - one for each @p ResourceManagerBase instance
			 * @param Parent Owning @p ResourceManagerBase instance
			*/
			constexpr LinkBaseOnlyType(ResourceManagerBase& Parent) noexcept : Parent(Parent) {}

			/**
			 * @copydoc DynExp::ResourceManagerBase::ShareResource(ItemIDType) const
			*/
			auto ShareResourceAsNonConst(ItemIDType ID) const { return Parent.ShareResource(ID); }

			ResourceManagerBase& Parent;								//!< Owning @p ResourceManagerBase instance
		};

	public:
		using ResourceType = Resource<PointerType>;						//!< Type of the managed %DynExp resources

	private:
		/**
		 * @brief SFINAE alias to define methods assuming shared resource ownership only if @p PointerType
		 * is a @p std::shared_ptr pointing to a type @p T::element_type.
		 * @tparam T Pointer type with an alias @p element_type to perform the test with
		*/
		template <typename T>
		using ShareResourceEnablerType = std::enable_if_t<std::is_same_v<std::shared_ptr<typename T::element_type>, PointerType>, int>;

		using MapType = std::unordered_map<ItemIDType, ResourceType>;	//!< Type of a map mapping %DynExp object IDs (@p ItemIDType) to %DynExp resources (@p ResourceType)

	protected:
		ResourceManagerBase() : LinkBaseOnly(*this) { Reset(); }
		~ResourceManagerBase() = default;

	public:
		/**
		 * @brief Getter for the ID to assign to the next resource to be inserted into the resource manager.
		 * @return Returns #CurrentID.
		*/
		ItemIDType GetNextID() const noexcept { return CurrentID; }

		/**
		 * @brief Determines the amount of resources stored in this resource manager.
		 * @return Returns the size of #Map. 
		*/
		const auto GetNumResources() const noexcept { return Map.size(); }

		/**
		 * @brief Determines whether this resource manager is empty.
		 * @return Returns true if #Map is empty, false otherwise.
		*/
		const bool Empty() const noexcept { return Map.empty(); }

		/**
		 * @copybrief LookUpResource
		 * @param ID ID of the resource to return
		 * @return Returns a pointer to the requested DynExp::Object.
		 * @throws Util::NotFoundException is thrown if the resource manager does not own a
		 * resource with @p ID (resource not found in #Map).
		*/
		const typename PointerType::element_type* GetResource(ItemIDType ID) const;

		/**
		 * @copydoc GetResource(ItemIDType) const
		*/
		typename PointerType::element_type* GetResource(ItemIDType ID);

		/**
		 * @brief Releases ownership of the owned resource with @p ID and returns it.
		 * @param ID ID of the resource to extract
		 * @return Returns a pointer of type @p PointerType pointing to the extracted resource.
		 * @throws Util::NotFoundException is thrown if the resource manager does not own a
		 * resource with @p ID (resource not found in #Map).
		*/
		PointerType ExtractResource(ItemIDType ID);

		/**
		 * @brief Removes the resource identified by @p ID from the resource manager and deletes it.
		 * @param ID ID of the resource to remove
		 * @param Timeout 
		 * @throws Util::NotFoundException is thrown if the resource manager does not own a
		 * resource with @p ID (resource not found in #Map).
		*/
		void RemoveResource(ItemIDType ID, const std::chrono::milliseconds Timeout = Util::ILockable::DefaultTimeout);

		/**
		 * @brief Deletes the Resource::TreeWidgetItem of the requested resource.
		 * @param ID ID of the resource whose Resource::TreeWidgetItem to delete.
		 * @throws Util::NotFoundException is thrown if the resource manager does not own a
		 * resource with @p ID (resource not found in #Map).
		*/
		void DeleteTreeWidgetItem(ItemIDType ID);

		/**
		 * @brief Selects the Resource::TreeWidgetItem of the requested resource.
		 * @param ID ID of the resource whose Resource::TreeWidgetItem to select.
		 * @throws Util::NotFoundException is thrown if the resource manager does not own a
		 * resource with @p ID (resource not found in #Map).
		*/
		void FocusTreeWidgetItem(ItemIDType ID);

		/**
		 * @brief Copies and returns the shared pointer pointing to (and owning) the resource
		 * identified by @p ID. The resource itself is neither copied nor changed.
		 * The @p const version of this method returns a shared pointer pointing to a @p const
		 * resource. This method is only available if the resources stored in the resource
		 * manager are managed by a shared pointer (i.e. @p PointerType is @p std::shared_ptr).
		 * @tparam T Set to @p PointerType. Do not pass anything.
		 * @param ID ID of the resource to share
		 * @return Returns Resource::ResourcePointer of the requested resource.
		 * @throws Util::NotFoundException is thrown if the resource manager does not own a
		 * resource with @p ID (resource not found in #Map).
		*/
		template <typename T = PointerType, ShareResourceEnablerType<T> = 0>
		auto ShareResource(ItemIDType ID) const;

		/**
		 * @copydoc ShareResource(ItemIDType) const
		*/
		template <typename T = PointerType, ShareResourceEnablerType<T> = 0>
		auto ShareResource(ItemIDType ID);

		/**
		 * @brief Returns an iterator to the first resource stored in the resource manager.
		 * @return Returns a @p const iterator pointing to the beginning of #Map.
		*/
		auto cbegin() const noexcept { return Map.cbegin(); }

		/**
		 * @brief Returns an iterator behind the last resource stored in the resource manager.
		 * @return Returns a @p const iterator pointing to the end of #Map.
		*/
		auto cend() const noexcept { return Map.cend(); }

		/**
		 * @copydoc InsertResource(ElementType&&, const ItemIDType)
		*/
		template <typename ElementType>
		ItemIDType InsertResource(ElementType&& Element);

		/**
		 * @brief Builds and returns a list of the IDs of all resources stored in this resource
		 * manager which can be cast to @p DerivedType.
		 * @tparam DerivedType Type derived from DynExp::Object to filter the resources with
		 * @return Returns a list (@p ItemIDListType) of the resource IDs belonging to owned
		 * resources which are of type @p DerivedType.
		*/
		template <typename DerivedType>
		ItemIDListType Filter() const;

		/**
		 * @brief Starts all resources owned by the resource manager. For @p HardwareAdapterBase instances,
		 * Object::EnsureReadyState() is called. For @p InstrumentBase and @p ModuleBase instances,
		 * RunnableObject::RunIfRunOnCreation() is called.
		 * @param FunctionToCallWhenObjectStarted Callback function. Refer to @p FunctionToCallWhenObjectStartedType.
		 */
		void Startup(FunctionToCallWhenObjectStartedType FunctionToCallWhenObjectStarted) const { StartupChild(FunctionToCallWhenObjectStarted); }

		/**
		 * @brief Calls @p ShutdownChild() to terminate and clean up all resources stored in this resource manager.
		*/
		void Shutdown() const { ShutdownChild(); }

		/**
		 * @brief Calls Object::ClearWarning() on all resources stored in this resource manager.
		*/
		void ClearResourcesWarnings() const;

		/**
		 * @brief Builds and returns a list of the IDs of all resources stored in this resource
		 * manager which are in an error state.
		 * @param OnlyResourcesBeingInUse If this argument is true, only resources are considered
		 * for which Object::IsUnused() returns false (i.e. resources which are used by other
		 * DynExp::Object instances). Otherwise, all owned resources are considered.
		 * @return Returns a list (@p ItemIDListType) of the resource IDs belonging to owned
		 * resources which are in an error state (determined by Object::GetException()).
		*/
		ItemIDListType GetFailedResourceIDs(bool OnlyResourcesBeingInUse = false) const;

		/**
		 * @brief Calls Object::Reset() on all resources stored in this resource manager which
		 * are in an error state (determined by Object::GetException()).
		*/
		void ResetFailedResources() const;

		/**
		 * @brief Prepares all resources stored in this resource manager for their deletion by
		 * e.g. asking them to terminate.
		*/
		void PrepareReset() { PrepareResetChild(); }

		/**
		 * @brief Resets the resource manager by calling @p ResetChild(), by removing all resources
		 * from #Map and by resetting #CurrentID. All owned resources are deleted if they are not
		 * shared and still in use elsewhere.
		*/
		void Reset();

		/**
		 * @brief Creates and returns the XML tree containing the configuration of each resource
		 * owned by this resource manager for saving %DynExp projects to XML files. The tree
		 * starts with a root node as produced by @p MakeXMLConfigHeadNode().
		 * @param Document Qt dom document within to create the dom element.
		 * @return Qt dom element containing this resource manager's resources' configuration.
		*/
		QDomElement EntryConfigsToXML(QDomDocument& Document) const;

		/**
		 * @brief Deletes the Resource::TreeWidgetItem of all resources stored in this resource manager.
		*/
		void DeleteAllTreeWidgetItems();

		/**
		 * @brief Creates resources and takes ownership of them according to an XML tree from
		 * a %DynExp project XML file.
		 * @tparam LibraryVectorT Type of the library containing the DynExp::Object types
		 * available to this resource manager (any of @p HardwareAdapterLibraryVectorType,
		 * @p InstrumentLibraryVectorType, or @p ModuleLibraryVectorType)
		 * @param XMLNode XML root node of the (single) resource manager instance of the derived
		 * manager type.
		 * @param Library Library to to create resources from
		 * @param Core Reference to %DynExp's core
		 * @throws Util::InvalidDataException is thrown if the XML tree does not contain data
		 * in the expected format.
		*/
		template <typename LibraryVectorT>
		void MakeEntriesFromXML(const QDomElement& XMLNode, const LibraryVectorT& Library, const DynExpCore& Core);

		LinkBaseOnlyType LinkBaseOnly;		// !< @copydoc LinkBaseOnlyType

	private:
		/**
		 * @brief Retrieves a resource specified by its ID from the resource manager.
		 * @param ID ID of the resource to return
		 * @return Iterator of #Map pointing to the %DynExp resource with @p ID.
		 * @throws Util::NotFoundException is thrown if the resource manager does not own a
		 * resource with @p ID (resource not found in #Map).
		*/
		auto LookUpResource(ItemIDType ID) const;

		/**
		 * @brief Inserts a resource into the resource manager.
		 * @tparam ElementType Type of @p Element, which is perfectly forwarded to the constructor
		 * of @p ResourceType to create a %DynExp resource to insert into the resource manager.
		 * @param Element Pointer to a %DynExp object (e.g. DynExp::ModulePtrType).
		 * @param ID ID of the resource to insert
		 * @return Returns @p ID.
		 * @throws Util::InvalidArgException is thrown if @p ID is DynExp::ItemIDNotSet.
		 * @throws Util::InvalidStateException is thrown if the resource manager owns already
		 * a resource with @p ID (resource already stored in #Map).
		*/
		template <typename ElementType>
		ItemIDType InsertResource(ElementType&& Element, const ItemIDType ID);

		/**
		 * @brief If #CurrentID is less or equal to @p ConsumedID, sets #CurrentID to @p ConsumedID + 1.
		 * If #CurrentID is greater than @p ConsumedID, does nothing.
		 * @param ConsumedID Resource ID which has been consumed by inserting a resource with this ID
		 * into the resource manager.
		*/
		void RaiseID(const ItemIDType ConsumedID);

		/**
		 * @brief Increments #CurrentID.
		 * @throws Util::OverflowException is thrown if #CurrentID cannot be incremented without
		 * causing a numeric overflow.
		*/
		void IncrementID();

		/** @name Override
		 * Override by derived classes.
		*/
		///@{
		virtual void StartupChild(FunctionToCallWhenObjectStartedType FunctionToCallWhenObjectStarted) const = 0;	//!< @copydoc Startup
		virtual void ShutdownChild() const {}																		//!< @copydoc Shutdown
		virtual void PrepareResetChild() {}																			//!< @copydoc PrepareReset
		virtual void ResetChild() {}																				//!< @copydoc Reset

		/**
		 * @brief Creates and returns the XML root node of the (single) resource manager
		 * instance of the derived manager type. Below this node in the XML hierarchy, each resource
		 * owned by this resource manager stores its configuration for saving %DynExp projects to XML files.
		 * Also refer to @p EntryConfigsToXML().
		 * @param Document Qt dom document within to create the dom element.
		 * @return Qt dom element acting as the root node of this resource manager's resources' configuration.
		*/
		virtual QDomElement MakeXMLConfigHeadNode(QDomDocument& Document) const = 0;
		///@}

		MapType Map;				//!< Map storing all resources owned by this resource manager.

		/**
		 * @brief ID, the next resource added to the manager will receive. After initialization and
		 * reset (@p Reset()), the #CurrentID will start with 1, so that 0 can have a special
		 * meaning (e.g. "resource not set").
		*/
		ItemIDType CurrentID;
	};

	template <typename PointerType>
	const typename PointerType::element_type* ResourceManagerBase<PointerType>::GetResource(ItemIDType ID) const
	{	
		return LookUpResource(ID)->second.ResourcePointer.get();
	}

	template <typename PointerType>
	typename PointerType::element_type* ResourceManagerBase<PointerType>::GetResource(ItemIDType ID)
	{
		return LookUpResource(ID)->second.ResourcePointer.get();
	}

	template <typename PointerType>
	PointerType ResourceManagerBase<PointerType>::ExtractResource(ItemIDType ID)
	{
		auto Res = Map.extract(ID);

		if (Res.empty())
			throw Util::NotFoundException(
				"Resource with ID " + Util::ToStr(ID) + " cannot be extracted from a resource manager since it cannot be found.");

		return std::move(Res.mapped().ResourcePointer);
	}

	template <typename PointerType>
	void ResourceManagerBase<PointerType>::RemoveResource(ItemIDType ID, const std::chrono::milliseconds Timeout)
	{
		GetResource(ID)->BlockIfUnused(Timeout);

		// Postpone calling destructor of managed resource to the end of this function and remove
		// from map first. Destructor of managed resource might cause iterating throup map (in
		// case of modules in DynExpManager::GetModuleByActiveUIWindow() when the module's
		// respective QMdiSubWindow is destroyed). An iteration must not happen while erasing
		// from the map is in progress.
		auto Resource = ExtractResource(ID);
	}

	template <typename PointerType>
	void ResourceManagerBase<PointerType>::DeleteTreeWidgetItem(ItemIDType ID)
	{
		LookUpResource(ID)->second.TreeWidgetItem.reset();
	}

	template <typename PointerType>
	void ResourceManagerBase<PointerType>::FocusTreeWidgetItem(ItemIDType ID)
	{
		try
		{
			auto const TreeWidgetItem = LookUpResource(ID)->second.TreeWidgetItem.get();

			// Assuming, all other items have already been deselectd. Now, select chosen one.
			if (TreeWidgetItem)
				TreeWidgetItem->setSelected(true);
		}
		catch (...)	// Swallow especially Util::NotFoundException.
		{
		}
	}

	template <typename PointerType>
	template <typename T, ResourceManagerBase<PointerType>::ShareResourceEnablerType<T>>
	auto ResourceManagerBase<PointerType>::ShareResource(ItemIDType ID) const
	{
		return std::static_pointer_cast<const typename PointerType::element_type>(LookUpResource(ID)->second.ResourcePointer);
	}

	template <typename PointerType>
	template <typename T, ResourceManagerBase<PointerType>::ShareResourceEnablerType<T>>
	auto ResourceManagerBase<PointerType>::ShareResource(ItemIDType ID)
	{
		return LookUpResource(ID)->second.ResourcePointer;
	}

	template <typename PointerType>
	template <typename ElementType>
	ItemIDType ResourceManagerBase<PointerType>::InsertResource(ElementType&& Element)
	{
		return InsertResource(std::forward<ElementType>(Element), GetNextID());
	}

	template <typename PointerType>
	template <typename DerivedType>
	ItemIDListType ResourceManagerBase<PointerType>::Filter() const
	{
		ItemIDListType FoundIDs;

		for (const auto& ResourceEntry : Map)
			if (dynamic_cast<DerivedType*>(ResourceEntry.second.ResourcePointer.get()))
				FoundIDs.push_back(ResourceEntry.first);

		return FoundIDs;
	}

	template <typename PointerType>
	void ResourceManagerBase<PointerType>::ClearResourcesWarnings() const
	{
		std::for_each(cbegin(), cend(), [](const auto& i) {
			i.second.ResourcePointer->ClearWarning();
		});
	}

	template <typename PointerType>
	ItemIDListType ResourceManagerBase<PointerType>::GetFailedResourceIDs(bool OnlyResourcesBeingInUse) const
	{
		ItemIDListType FoundIDs;

		std::for_each(cbegin(), cend(), [&FoundIDs, OnlyResourcesBeingInUse](const auto& i) {
			if (i.second.ResourcePointer->GetException() && (!OnlyResourcesBeingInUse || !i.second.ResourcePointer->IsUnused()))
				FoundIDs.push_back(i.first);
		});

		return FoundIDs;
	}

	template <typename PointerType>
	void ResourceManagerBase<PointerType>::ResetFailedResources() const
	{
		std::for_each(cbegin(), cend(), [](const auto& i) {
			if (i.second.ResourcePointer->GetException())
				i.second.ResourcePointer->Reset();
		});
	}

	template <typename PointerType>
	void ResourceManagerBase<PointerType>::Reset()
	{
		ResetChild();

		Map.clear();
		CurrentID = 1;	// Start with 1, so that 0 can have a special meaning (e.g. "not set").
	}

	template <typename PointerType>
	QDomElement ResourceManagerBase<PointerType>::EntryConfigsToXML(QDomDocument& Document) const
	{
		auto XMLNode = MakeXMLConfigHeadNode(Document);

		std::for_each(cbegin(), cend(), [&XMLNode, &Document](const auto& i) {
			QDomElement ItemNode = Document.createElement("Item");
			ItemNode.setAttribute(QString("Name"), QString::fromStdString(i.second.ResourcePointer->GetName()));
			ItemNode.setAttribute(QString("Category"), QString::fromStdString(i.second.ResourcePointer->GetCategory()));
			ItemNode.setAttribute(QString("ID"), static_cast<qulonglong>(i.first));

			// GetParams() locks the parameters to save. So this is thread-safe.
			QDomElement ParamsNode = Document.createElement("Params");
			ParamsNode.appendChild(i.second.ResourcePointer->GetParams()->ConfigToXML(Document));
			ItemNode.appendChild(ParamsNode);

			XMLNode.appendChild(ItemNode);
		});
		
		return XMLNode;
	}

	template <typename PointerType>
	void ResourceManagerBase<PointerType>::DeleteAllTreeWidgetItems()
	{
		std::for_each(cbegin(), cend(), [](const auto& i) { i.second.TreeWidgetItem.reset(); });
	}

	template <typename PointerType>
	template <typename LibraryVectorT>
	void ResourceManagerBase<PointerType>::MakeEntriesFromXML(const QDomElement& XMLNode, const LibraryVectorT& Library, const DynExpCore& Core)
	{
		auto ItemNodeList = XMLNode.elementsByTagName("Item");
		for (int i = 0; i < ItemNodeList.length(); ++i)
		{
			auto ItemNode = ItemNodeList.item(i).toElement();
			if (ItemNode.isNull())
				throw Util::InvalidDataException(
					"Error parsing the specified project file. An item node contains invalid data.");

			auto ItemCategory = Util::GetStringFromDOMAttribute(ItemNode, "Category");
			auto ItemName = Util::GetStringFromDOMAttribute(ItemNode, "Name");
			auto ItemID = Util::GetTFromDOMAttribute<ItemIDType>(ItemNode, "ID");
			auto ParamsNode = Util::GetSingleChildDOMElement(ItemNode, "Params");

			auto LibEntry = FindInLibraryVector(Library, ItemCategory, ItemName);
			auto Params = LibEntry.ConfigFactoryPtr()->MakeConfigFromXML(ItemID, Core, ParamsNode);
			auto Item = LibEntry.ObjectFactoryPtr(GetOwnerThreadID(Core), std::move(Params));

			InsertResource(std::move(Item), ItemID);
		}
	}

	template <typename PointerType>
	auto ResourceManagerBase<PointerType>::LookUpResource(ItemIDType ID) const
	{
		auto Iter = Map.find(ID);

		if (Iter == Map.cend())
			throw Util::NotFoundException("Resource with ID " + Util::ToStr(ID) + " cannot be found.");

		return Iter;
	}

	template <typename PointerType>
	template <typename ElementType>
	ItemIDType ResourceManagerBase<PointerType>::InsertResource(ElementType&& Element, const ItemIDType ID)
	{
		if (!ID)
			throw Util::InvalidArgException(
				"Resource cannot be inserted into a resource manager since 0 is not a valid ID.");

		auto Result = Map.try_emplace(ID, ResourceType(std::forward<ElementType>(Element)));
		if (!Result.second)
			throw Util::InvalidStateException(
				"Resource cannot be inserted into a resource manager since the chosen ID already exists.");

		RaiseID(ID);

		return ID;
	}

	template <typename PointerType>
	void ResourceManagerBase<PointerType>::RaiseID(const ItemIDType ConsumedID)
	{
		if (CurrentID < ConsumedID)
			CurrentID = ConsumedID;

		if (CurrentID == ConsumedID)
			IncrementID();
	}

	template <typename PointerType>
	void ResourceManagerBase<PointerType>::IncrementID()
	{
		if (CurrentID == std::numeric_limits<ItemIDType>::max())
			throw Util::OverflowException("No ID left to insert a resource into a resource manager.");

		++CurrentID;
	}

	/**
	 * @brief Resource manager for @p HardwareAdapterBase resources deriving from a
	 * specialized @p ResourceManagerBase class.
	*/
	class HardwareAdapterManager : public ResourceManagerBase<HardwareAdapterPtrType>
	{
	public:
		using ResourceType = Resource<HardwareAdapterPtrType>;		//!< @copydoc ResourceManagerBase::ResourceType

		HardwareAdapterManager() = default;
		~HardwareAdapterManager() = default;

		/**
		 * @brief Determines whether all hardware adapters managed by this resource manager
		 * are connected. If any of the managed hardware adapter is in an error state, the
		 * related exception is rethrown.
		 * @return Returns true if all owned hardware adapters are connected (as determined
		 * by HardwareAdapterBase::IsConnected()), false otherwise.
		*/
		bool AllConnected() const;

	private:
		virtual void StartupChild(FunctionToCallWhenObjectStartedType FunctionToCallWhenObjectStarted) const override;
		virtual QDomElement MakeXMLConfigHeadNode(QDomDocument& Document) const override;
	};

	/**
	 * @brief Resource manager for @p InstrumentBase resources deriving from a
	 * specialized @p ResourceManagerBase class.
	*/
	class InstrumentManager : public ResourceManagerBase<InstrumentPtrType>
	{
	public:
		using ResourceType = Resource<InstrumentPtrType>;			//!< @copydoc ResourceManagerBase::ResourceType

		InstrumentManager() = default;
		~InstrumentManager() = default;

		/**
		 * @brief Counts the instruments managed by this resource manager which are
		 * running (as determined by RunnableObject::IsRunning()).
		 * @return Returns the amount of owned running instruments.
		*/
		size_t GetNumRunningInstruments() const;

		/**
		 * @brief Determines whether all instruments managed by this resource manager
		 * are initialized. Only counts instruments with the startup type
		 * RunnableObjectParams::StartupType::OnCreation. If any of the managed instruments
		 * is in an error state, the related exception is rethrown.
		 * @return Returns true if all owned instruments are initialized (as determined
		 * by InstrumentBase::IsInitialized()), false otherwise.
		*/
		bool AllInitialized() const;

		/**
		 * @brief Calls RunnableObject::Terminate() on all instruments managed by this
		 * resource manager.
		 * Logical const-ness: Refer to @p CommonResourceManagerBase.
		*/
		void TerminateAll() const;

	private:
		virtual void StartupChild(FunctionToCallWhenObjectStartedType FunctionToCallWhenObjectStarted) const override;
		virtual void ShutdownChild() const override { TerminateAll(); }
		virtual void PrepareResetChild() override { TerminateAll(); }
		virtual QDomElement MakeXMLConfigHeadNode(QDomDocument& Document) const override;
	};

	/**
	 * @brief Resource manager for @p ModuleBase resources deriving from a
	 * specialized @p ResourceManagerBase class.
	*/
	class ModuleManager : public ResourceManagerBase<ModulePtrType>
	{
	public:
		using ResourceType = Resource<ModulePtrType>;				//!< @copydoc ResourceManagerBase::ResourceType

		ModuleManager() = default;
		~ModuleManager() = default;

		/**
		 * @brief Counts the modules managed by this resource manager which are
		 * running (as determined by RunnableObject::IsRunning()).
		 * @return Returns the amount of owned running modules.
		*/
		size_t GetNumRunningModules() const;

		/**
		 * @brief Calls RunnableObject::Terminate() on all modules managed by this
		 * resource manager.
		 * Logical const-ness: Refer to @p CommonResourceManagerBase.
		*/
		void TerminateAll() const;

		/**
		 * @brief Calls ModuleBase::RestoreWindowStatesFromParams() on all modules managed by this
		 * resource manager.
		 * Logical const-ness: Refer to @p CommonResourceManagerBase.
		*/
		void RestoreWindowStatesFromParams() const;

		/**
		 * @brief Calls ModuleBase::UpdateParamsFromWindowStates() on all modules managed by this
		 * resource manager.
		 * Logical const-ness: Refer to @p CommonResourceManagerBase.
		*/
		void UpdateParamsFromWindowStates() const;

	private:
		virtual void StartupChild(FunctionToCallWhenObjectStartedType FunctionToCallWhenObjectStarted) const override;
		virtual void ShutdownChild() const override { TerminateAll(); }
		virtual void PrepareResetChild() override { TerminateAll(); }
		virtual QDomElement MakeXMLConfigHeadNode(QDomDocument& Document) const override;
	};
}