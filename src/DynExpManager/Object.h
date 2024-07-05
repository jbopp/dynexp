// This file is part of DynExp.

/**
 * @file Object.h
 * @brief Implementation of %DynExp objects as the base for derived resources and implementation of the
 * object parameter system.
*/

#pragma once

#include "stdafx.h"
#include "BusyDialog.h"
#include "ParamsConfig.h"

class DynExpManager;

namespace DynExp
{
	class DynExpCore;
	class Object;
	class RunnableObject;
	class LinkedObjectWrapperContainerBase;
	class ObjectLinkBase;
	class RunnableInstance;
	class HardwareAdapterBase;
	class InstrumentBase;
	class ModuleBase;
	class CommonResourceManagerBase;
	class HardwareAdapterManager;
	class InstrumentManager;
	class ModuleManager;
	class NetworkParamsExtension;

	template <typename>
	class ObjectLink;

	/**
	 * @brief Type of a list of IDs belonging to objects managed by %DynExp
	*/
	using ItemIDListType = std::vector<ItemIDType>;

	/**
	 * @brief Type trait relating a type @p ObjectType derived from either class @p HardwareAdapterBase or class
	 * @p InstrumentBase to the resource manager type (derived from DynExp::CommonResourceManagerBase) managing
	 * instances of the type @p ObjectType.
	 * @tparam ObjectType Type to relate to a corresponding resource manager
	*/
	template <
		typename ObjectType,
		std::enable_if_t<
			std::is_base_of_v<HardwareAdapterBase, ObjectType> || std::is_base_of_v<InstrumentBase, ObjectType>,
		int> = 0
	>
	struct ManagerTypeOfObjectType
	{
		/**
		 * @brief Resource manager type managing instances of type @p ObjectType.
		*/
		using type = std::conditional_t<
			std::is_base_of_v<HardwareAdapterBase, ObjectType>,
			HardwareAdapterManager,
			InstrumentManager
		>;
	};

	/**
	 * @brief Alias for a resource manager type (derived from DynExp::CommonResourceManagerBase) managing
	 * resources of type @p ObjectType
	 * @tparam ObjectType Refer to DynExp::ManagerTypeOfObjectType.
	*/
	template <typename ObjectType>
	using ManagerTypeOfObjectType_t = typename ManagerTypeOfObjectType<ObjectType>::type;

	/**
	 * @brief Helper class to enable keeping track of instances of class @p Object making use of the owner of
	 * the respective ObjectUserList's instance. Instances of class @p Object can register or deregister multiple
	 * times as an user which increments or decrements their respective registration count in ObjectUserList::UserList.
	*/
	class ObjectUserList : public Util::ILockable
	{
	public:
		ObjectUserList() = default;
		virtual ~ObjectUserList() = default;

		/**
		 * @brief Locks the user list for thread-safe manipulation.
		 * @param Timeout Timeout of the mutex-locking operation.
		 * @return Lock-guard of the underlying mutex. Only use temporarily. Do not store longer than necessary.
		*/
		[[nodiscard]] Util::ILockable::LockType AcquireLock(const std::chrono::milliseconds Timeout = DefaultTimeout) const;

		/**
		 * @brief Registers a user in a thread-safe way.
		 * @param User Instance of class @p Object to be registered as a user.
		 * @param Timeout Timeout of the mutex-locking operation.
		*/
		void Register(const Object& User, const std::chrono::milliseconds Timeout = std::chrono::milliseconds(0));

		/**
		 * @brief Registers a user making use of a lock-guard previously acquired by a call to AcquireLock().
		 * @param User Instance of class @p Object to be registered as a user.
		 * @param Lock Lock-guard of the underlying mutex. Move to this function.
		 * @return Lock-guard of the underlying mutex (the same one passed to this function).
		*/
		LockType Register(const Object& User, LockType&& Lock);
		
		/**
		 * @brief Deregisters a user in a thread-safe way.
		 * @param User Instance of class @p Object to be deregistered as a user.
		 * @param Timeout Timeout of the mutex-locking operation.
		*/
		void Deregister(const Object& User, const std::chrono::milliseconds Timeout = std::chrono::milliseconds(0));

		/**
		 * @brief Counts the registered useres in a thread-safe way.
		 * @param Timeout Timeout of the mutex-locking operation.
		 * @return Number of registered users.
		*/
		size_t CountUsers(std::chrono::milliseconds Timeout = std::chrono::milliseconds(0)) const;

		/**
		 * @brief Returns a list of the IDs of the registered users in a thread-safe way.
		 * @param Timeout Timeout of the mutex-locking operation.
		 * @return List of IDs of registered users.
		*/
		ItemIDListType GetUserIDs(std::chrono::milliseconds Timeout = std::chrono::milliseconds(0)) const;

		/**
		 * @brief Builds a string describing which users are registered containing their
		 * object names, categories and type names in a thread-safe way.
		 * @param Timeout Timeout of the mutex-locking operation.
		 * @return String containing information about the registered users.
		*/
		std::string GetUserNamesString(std::chrono::milliseconds Timeout = std::chrono::milliseconds(0)) const;

		/** @name Not thread-safe
		 * AcquireLock() has to be called manually before and returned LockType has to be still in scope.
		*/
		///@{
		/**
		 * @brief Deregisters all users and notifies them that they need to check the states of their used linked objects.
		*/
		void DeregisterAllUnsafe();

		size_t CountUsersUnsafe() const;
		ItemIDListType GetUserIDsUnsafe() const;
		std::string GetUserNamesStringUnsafe() const;
		///@}

	private:
		/** @name Not thread-safe
		 * AcquireLock() has to be called manually before and returned LockType has to be still in scope.
		*/
		///@{
		/**
		 * @brief Registers a user
		 * @param User Instance of class @p Object to be registered as a user.
		 * @throws Util::OverflowException is thrown if a value in @p UserList cannot be incremented due to
		 * size limitations in @p UserList's value data type.
		*/
		void RegisterUnsafe(const Object& User);

		/**
		 * @brief Deregisters a user
		 * @param User Instance of class @p Object to be deregistered as a user.
		 * @throws Util::UnderflowException is thrown if @p UserList contains an entry for @p User with a
		 * registration count of zero. This should never happen.
		*/
		void DeregisterUnsafe(const Object& User);
		///@}

		/**
		 * @brief Map containing pointers to all users making use of this ObjectUserList instance's owner as keys
		 * and the amount of their active registrations as values
		*/
		std::unordered_map<const Object*, size_t> UserList;
	};

	/**
	 * @brief Abstract base class of link parameters (to be saved in project files) describing relations between
	 * multiple @p Object (e.g. a module making use of an instrument).
	*/
	class LinkBase
	{
	public:
		/**
		 * @brief Constructs a LinkBase object.
		 * @param IconResourcePath Qt resource path describing an icon being displayed along with this parameter
		 * in user interface dialogs.
		 * @param Optional Determines whether this parameter is optional. Optional parameters do not have to point
		 * to valid object IDs.
		*/
		LinkBase(std::string_view IconResourcePath = {}, bool Optional = false)
			: IconResourcePath(IconResourcePath), Optional(Optional) {}

	protected:
		virtual ~LinkBase() {}

		/**
		 * @brief Returns a shared_ptr pointing to the resource with the given @p ID contained in the given resource manager @p Manager.
		 * The returned pointer points to a non-const instance of class @p Object. Throws an exception
		 * of type Util::NotFoundException if the resource is not found.
		 * @tparam ResourceManagerType Type ot the resource manager containing the resource specified by @p ID.
		 * @param Manager Resource manager containing the resource specified by @p ID.
		 * @param ID ID of the resource to share.
		 * @return shared_ptr pointing to the non-const resource.
		*/
		template <typename ResourceManagerType>
		auto ShareResource(const ResourceManagerType& Manager, ItemIDType ID) { return Manager.LinkBaseOnly.ShareResourceAsNonConst(ID); }

		/**
		 * @brief Finds all resources managed by the given resource manager matching type @p ObjectTpye and returns a list of
		 * information about these objects.
		 * @tparam ObjectType Type of objects assignable to this link.
		 * @param Manager Resource manager to find resources in.
		 * @return @p Util::TextValueListType containing object IDs and their description (object names, categories and type names)
		 * of objects assignable to this link.
		 * @throws Util::EmptyException is thrown if @p Manager does not contain any resource matching type @p ObjectTpye and if
		 * LinkBase::Optional is false.
		*/
		template <typename ObjectType>
		auto MakeObjectIDsWithLabels(const ManagerTypeOfObjectType_t<ObjectType>& Manager) const
		{
			// template disambiguator (see ISO C++03 14.2/4)
			auto FoundIDs = Manager.template Filter<ObjectType>();
			if (FoundIDs.empty() && !IsOptional())
			{
				constexpr auto FilterTypeBaseName = std::is_base_of_v<HardwareAdapterBase, ObjectType> ? "hardware adapter" :
					(std::is_base_of_v<InstrumentBase, ObjectType> ? "instrument" : "< error-type >");

				throw Util::EmptyException("At least one " + std::string(FilterTypeBaseName) + " of type " +
					ObjectType::CategoryAndNameToStr(ObjectType::Category(), ObjectType::Name()) + " is needed before this item can be created.");
			}

			std::sort(FoundIDs.begin(), FoundIDs.end(), [&Manager](const auto& a, const auto& b) {
				return Manager.GetResource(a)->GetObjectName() < Manager.GetResource(b)->GetObjectName();
			});

			Util::TextValueListType<typename ParamsConfigDialog::IndexType> FoundIDsWithLabels;
			for (const auto& ID : FoundIDs)
			{
				const auto Resource = Manager.GetResource(ID);
				std::string Label = Resource->GetObjectName() + " (" + Resource->GetCategoryAndName() + ")";

				FoundIDsWithLabels.emplace_back(std::move(Label), ID);
			}

			return FoundIDsWithLabels;
		}

	public:
		/** @name Thread-safe
		 * Thread-safe since const member variables are returned.
		*/
		///@{
		std::string_view GetIconResourcePath() const noexcept { return IconResourcePath; }
		bool IsOptional() const noexcept { return Optional; }
		///@}

		/**
		 * @brief Makes sure that the object where this link parameter points to is in a ready state.
		 * Only to be called by main thread. Otherwise, RunnableObject::Run() throws Util::InvalidCallException.
		*/
		void EnsureReadyState() { EnsureReadyStateChild(); }

		/**
		 * @brief Returns whether the object where this link parameter points to is in a ready state.
		 * @return Returns true if it is in a ready state, false otherwise.
		*/
		bool IsReady() { return IsReadyChild(); }

		/**
		 * @brief Returns a reference to this link parameter's title.
		 * @return String view to this link parameter's title. 
		*/
		std::string_view GetLinkTitle() const noexcept { return GetLinkTitleChild(); }

		/**
		 * @brief Returns a list of all object IDs assigned to this parameter.
		 * @return List of assigned object IDs.
		*/
		ItemIDListType GetLinkedIDs() const { return GetLinkedIDsChild(); }

		/**
		 * @brief This function can be used to determine the object type the parameter is expecting by comparing
		 * the resource manager's address to resource managers owned by a @p DynExpCore instance.
		 * @return Returns the address of the resource manager managing resources to be possibly linked to this parameter.
		*/
		const CommonResourceManagerBase* GetCommonManager() const noexcept { return GetCommonManagerChild(); }

	private:
		/** @name Override
		 * Override by derived class to make public versions of these functions behave as described above.
		*/
		///@{
		virtual void EnsureReadyStateChild() = 0;												//!< @copydoc EnsureReadyState()
		virtual bool IsReadyChild() = 0;														//!< @copydoc IsReady()

		virtual std::string_view GetLinkTitleChild() const noexcept = 0;						//!< @copydoc GetLinkTitle()
		virtual ItemIDListType GetLinkedIDsChild() const = 0;									//!< @copydoc GetLinkedIDs()
		virtual const CommonResourceManagerBase* GetCommonManagerChild() const noexcept = 0;	//!< @copydoc GetCommonManager()
		///@}

		/**
		 * @brief Qt resource path describing an icon being displayed along with this parameter in user interface dialogs.
		*/
		const std::string_view IconResourcePath;

		/**
		 * @brief Determines whether this parameter is optional. Optional parameters do not have to point to valid object IDs.
		*/
		const bool Optional;
	};

	/**
	 * @brief Abstract base class for object parameter classes. Each class derived from class @p Object
	 * must be accompanied by a parameter class derived from @p ParamsBase. These parameter classes
	 * are intended to contain the object's parameters (sub-classes of @p ParamsBase::ParamBase) which
	 * are stored in %DynExp project files and which can be configured in the respective object instance's
	 * settings dialog or programmatically. Parameter classes are synchronized in between different threads
	 * (e.g. the main thread and an instrument/module thread).
	 * @warning For the same @p Object, always lock the mutex of the corresponding parameter class
	 * before the mutex of the corresponding data class (or only one of them).
	*/
	class ParamsBase : public Util::ISynchronizedPointerLockable
	{
	private:
		struct OwnedParamInfo;

		/**
		 * @brief List type of information on parameters owned by this ParamsBase instance (and its derived classes)
		*/
		using OwnedParamsType = std::vector<OwnedParamInfo>;

	protected:
		/**
		 * @brief Tag for function dispatching mechanism within this class used when derived classes are not
		 * intended to override a specific function, but to add functionality. Some class B derived from class A
		 * overrides a function f accepting an argument of type dispatch_tag< A > declared as virtual in class A.
		 * Class B itself declares a virtual function accepting an argument of type dispatch_tag< B > to be
		 * overriden by a class C derived from class B. The function defined in class B is called by the base
		 * class A and it itself has to call the function overriden in class C. Calls to respective functions are
		 * performed using an instance of the respective dispatch tag. This allows to form a chain of calls
		 * f() [defined in A] -> f(dispatch_tag< A >) [declared in A, defined in B] ->
		 * f(dispatch_tag< B >) [declared in B, defined in C] -> ...
		 * @tparam Type (derived from class @p ParamsBase) to instantiate the dispatch tag template with.
		*/
		template <typename>
		struct dispatch_tag {};

	public:
		using ObjectLinkParamsType = std::vector<std::reference_wrapper<LinkBase>>;	//!< Type of a list of all owned object link parameters
		using EnumParamSignedIntegerType = intmax_t;								//!< Parameter type to convert signed eumeration parameters to
		using EnumParamUnsignedIntegerType = uintmax_t;								//!< Parameter type to convert unsigned eumeration parameters to

		/**
		 * @brief Type trait providing an integer type for enumeration types which allows to store the value of enumeration
		 * variables of that type in a file.
		 * @tparam EnumType Integer enumeration type - do not use strongly-typed enumerations as DynExp parameter types.
		*/
		template <typename EnumType>
		using LargestEnumUnderlyingType = std::conditional_t<
			std::is_signed_v<std::underlying_type_t<EnumType>>,
			EnumParamSignedIntegerType,
			EnumParamUnsignedIntegerType
		>;

		using Text = Util::TextType;	//!< @copydoc Util::TextType
		class TextList {};				//!< Type to identify a text list parameter (one string selected from a list of strings)
		class IndexedTextList {};		//!< Type to identify an indexed text list parameter (one number selected from a list of numbers associated with strings)

		/**
		 * @brief Abstract base class for a single object parameter. Parameters derived from this class are automatically stored in
		 * the DynExp project file. Additionally, derived parameters might be configured in settings dialogs dynamically generated
		 * for objects according to the parameters belonging to it.
		*/
		class ParamBase
		{
		private:
			/**
			 * @brief Provides the class @p ParamsBase access to some private members of class @p ParamBase.
			*/
			class ParamsBaseOnlyType
			{
				friend class ParamsBase;

				/**
				 * @brief Construcs an instance - one for each @p ParamBase instance
				 * @param Parent Owning @p ParamBase instance
				*/
				constexpr ParamsBaseOnlyType(ParamBase& Parent) noexcept : Parent(Parent) {}

				void DisableUserEditable() noexcept { Parent.DisableUserEditable(); }			//!< @copydoc ParamBase::DisableUserEditable
				void AddToDialog(ParamsConfigDialog& Dialog) { Parent.AddToDialog(Dialog); }	//!< @copydoc ParamBase::AddToDialog

				ParamBase& Parent;																//!< Owning @p ParamBase instance
			};

		protected:
			/**
			 * @brief Base constructor of any parameter to be used if a parameter should be displayed in a settings dialog  (UserEditable is set to true).
			 * @param Owner ParamsBase instance owning this parameter.
			 * @param ParamName Name of the parameter to be used in the XML config file. std::string to allow for auto-generated names.
			 * @param ParamTitle Title of the parameter to be displayed in settings dialogs. A static string literal with a constant address is expected.
			 * @param ParamDescription Detailed description of the parameter to be displayed in settings dialogs. A static string literal with a constant address is expected.
			 * @param NeedsResetToApplyChange Indicated whether the object this parameter belongs to needs to be reset to apply changes if this parameter's value has changed.
			*/
			ParamBase(ParamsBase& Owner, std::string ParamName, std::string_view ParamTitle, std::string_view ParamDescription, bool NeedsResetToApplyChange);

			/**
			 * @brief Base constructor of any parameter to be used if a parameter should not be displayed in a settings dialog (UserEditable is set to false).
			 * @param Owner ParamsBase instance owning this parameter.
			 * @param ParamName Name of the parameter to be used in the XML config file. std::string to allow for auto-generated names.
			*/
			ParamBase(ParamsBase& Owner, std::string ParamName);
			
			virtual ~ParamBase() = 0;

		public:
			ParamBase(const ParamBase&) = delete;
			ParamBase& operator=(const ParamBase&) = delete;

			/** @name Thread-safe getter functions
			 * Thread-safe since const member variables are returned.
			*/
			///@{
			bool IsUserEditable() const noexcept { return UserEditable; }							//!< Returns ParamBase::UserEditable.
			std::string_view GetParamName() const noexcept { return ParamName; }					//!< Returns ParamBase::ParamName.
			std::string_view GetParamTitle() const noexcept { return ParamTitle; }					//!< Returns ParamBase::ParamTitle.
			std::string_view GetParamDescription() const noexcept { return ParamDescription; }		//!< Returns ParamBase::ParamDescription.
			bool GetNeedsResetToApplyChange() const noexcept { return NeedsResetToApplyChange; }	//!< Returns ParamBase::NeedsResetToApplyChange.
			///@}

			/**
			 * @brief Converts this parameter to a Qt dom element
			 * (describing an XML node containing this parameter's name and value).
			 * @param Document Qt dom document within to create the dom element.
			 * @return Qt dom element containing information about this parameter's name and value.
			*/
			QDomElement ToXMLNode(QDomDocument& Document) const;

			/**
			 * @brief Restores this parameter's value from the given Qt dom element (describing an XML node)
			 * @param XMLElement Qt dom element containing information about this parameter.
			*/
			void FromXMLNode(const QDomElement& XMLElement);

			/**
			 * @brief Checks whether a valid value is assigned to this parameter.
			 * This function is not const since it is also intended to reset the parameter if it is invalid.
			 * @return Returns true if the parameter is valid. Returns false if it was invalid and has been reset.
			*/
			bool Validate();
			
			/**
			 * @brief Resets this parameter to its default value.
			*/
			void Reset() { ResetChild(); }

			ParamsBaseOnlyType ParamsBaseOnly;						//!< @copydoc ParamsBaseOnlyType

		protected:
			const auto& GetOwner() const noexcept { return Owner; }	//!< Returns the ParamsBase instance owning this ParamBase instance.
			auto& GetOwner() noexcept { return Owner; }				//!< Returns the ParamsBase instance owning this ParamBase instance.

		private:
			/**
			 * @brief Sets the @p UserEditable property to false.
			*/
			void DisableUserEditable() noexcept { UserEditable = false; }

			/**
			 * @brief Appends this parameter to a settings dialog making it configurable by the user.
			 * @param Dialog Reference to the settings dialog to append the parameter to
			*/
			void AddToDialog(ParamsConfigDialog& Dialog) { AddToDialogChild(Dialog); }

			/** @name Override
			 * Override by derived class to make public versions of these functions behave as described above.
			*/
			///@{
			/**
			 * @copybrief ToXMLNode
			 * @param Document Qt dom document within to create the dom element.
			 * @param XMLElement Qt dom element to save information about this parameter to.
			*/
			virtual void ToXMLNodeChild(QDomDocument& Document, QDomElement& XMLElement) const = 0;

			virtual void FromXMLNodeChild(const QDomElement& XMLElement) = 0;						//!< @copydoc FromXMLNode
			virtual void AddToDialogChild(ParamsConfigDialog& Dialog) = 0;							//!< @copydoc AddToDialog
			virtual bool ValidateChild() const { return true; }										//!< @copydoc Validate Returns true by default.
			virtual void ResetChild() = 0;															//!< @copydoc Reset
			///@}

			/**
			 * @brief Owner of this parameter. Owner always lives longer than this object.
			*/
			ParamsBase& Owner;

			/**
			 * @brief Is this parameter displayed in a settings dialog und thus editable by the user directly?
			 * Not const since derived objects might want to hide a parameter declared by a base class.
			*/
			bool UserEditable;

			const std::string ParamName;				//!< Identifier which is stored in project file.
			const std::string_view ParamTitle;			//!< String which is displayed as a label when editing the parameter using ParamsConfigDialog.
			const std::string_view ParamDescription;	//!< String which describes the parameter as a tooltip when using ParamsConfigDialog.

			/**
			 * @brief Indicates whether the object owning this parameter needs to be reset to apply changes to this parameter.
			*/
			const bool NeedsResetToApplyChange;
		};

		/**
		 * @brief Dummy parameter which is to be owned once by parameter classes that do not contain
		 * any other parameter. It ensures the presence of the respective level in the XML hierarchy.
		 * Parameter is skipped when saving/loading because of its empty name.
		*/
		class DummyParam : public ParamBase
		{
		public:
			DummyParam(ParamsBase& Owner) : ParamBase(Owner, "") {}

		private:
			virtual void ToXMLNodeChild(QDomDocument& Document, QDomElement& XMLElement) const override {}
			virtual void FromXMLNodeChild(const QDomElement& XMLElement) override {}
			virtual void AddToDialogChild(ParamsConfigDialog& Dialog) override {}
			virtual void ResetChild() override {}
		};

		/**
		 * @brief Base class of parameters containing a single value
		 * @tparam ParamType Underlying parameter value type
		*/
		template <typename ParamType>
		class TypedParamBase : public ParamBase
		{
		public:
			using UnderlyingType = ParamType;

		protected:
			/**
			 * @copydoc ParamBase::ParamBase(ParamsBase&, std::string, std::string_view, std::string_view, bool)
			 * @param DefaultValue Default value to assign to the constructed parameter
			*/
			TypedParamBase(ParamsBase& Owner, std::string ParamName, std::string_view ParamTitle, std::string_view ParamDescription,
				bool NeedsResetToApplyChange = true, ParamType DefaultValue = ParamType())
				: ParamBase(Owner, ParamName, ParamTitle, ParamDescription, NeedsResetToApplyChange),
				DefaultValue(std::move(DefaultValue)), Value(this->DefaultValue) {}

			/**
			 * @copydoc ParamBase::ParamBase(ParamsBase&, std::string)
			 * @param DefaultValue Default value to assign to the constructed parameter
			*/
			TypedParamBase(ParamsBase& Owner, std::string ParamName, ParamType DefaultValue = ParamType())
				: ParamBase(Owner, ParamName),
				DefaultValue(std::move(DefaultValue)), Value(this->DefaultValue) {}

		public:
			/**
			 * @brief Returns this parameter's default value. Thread-safe since a const member variable is returned.
			 * @return Parameter's default value
			*/
			ParamType GetDefaultValue() const noexcept { return DefaultValue; }

			/**
			 * @brief Converts the parameter implicitly to its underlying type returning its current value.
			*/
			operator ParamType() const noexcept { return Value; }

			/**
			 * @brief Returns the parameter's value.
			 * @return Current parameter value
			*/
			const ParamType& Get() const noexcept { return Value; }

			/**
			 * @brief Assigns a new value to this parameter. The operator cannot be accessed by @p Object because
			 * this function is not const.
			 * @param NewValue New value to assign
			 * @return Reference to this parameter
			 * @throws Util::InvalidArgException is thrown if the value to assign to the parameter is not considered
			 * valid as determined by a call to ParamBase::Validate().
			*/
			const ParamType& operator=(const ParamType& NewValue)
			{
				if (!ValidateValue(NewValue))
					throw Util::InvalidArgException(
						"An assignment to a parameter cannot be executed since the given value is considered invalid.");

				Value = NewValue;
				return Get();
			}

		protected:
			virtual void ToXMLNodeChild(QDomDocument& Document, QDomElement& XMLElement) const override
			{
				QDomText NodeValue = Document.createTextNode(QString::fromStdString(Util::ToStr(Value)));
				XMLElement.appendChild(NodeValue);
			}

			virtual void FromXMLNodeChild(const QDomElement& XMLElement) override
			{
				Value = Util::GetTFromDOMElement<ParamType>(XMLElement, GetParamName().data());
			}

		private:
			virtual bool ValidateChild() const override final { return ValidateValue(Value); }
			
			/**
			 * @brief Called by ValidateChild(). Validates a single value.
			 * Override to validate the value which is about to be assigned to the parameter. Returns true by default.
			 * @param NewValue Value to be validated before the assignment
			 * @return Returns true if @p NewValue is considered valid, false otherwise.
			*/
			virtual bool ValidateValue(const ParamType& NewValue) const { return true; }
			
			virtual void ResetChild() override { Value = DefaultValue; }

			const ParamType DefaultValue;	//!< Value to assign to the parameter upon construction and reset
			ParamType Value;				//!< Current parameter value
		};

		/**
		 * @brief Base class of parameters containing lists of values
		 * @tparam ParamType Underlying value type of the parameter's list entries
		*/
		template <typename ParamType>
		class TypedListParamBase : public ParamBase
		{
		public:
			/**
			 * @brief List type to be used for the parameter
			*/
			using UnderlyingType = std::vector<ParamType>;

		protected:
			/**
			 * @copydoc ParamBase::ParamBase(ParamsBase&, std::string, std::string_view, std::string_view, bool)
			 * @param DefaultValues List of default values to assign to the constructed parameter
			*/
			TypedListParamBase(ParamsBase& Owner, std::string ParamName, std::string_view ParamTitle, std::string_view ParamDescription,
				bool NeedsResetToApplyChange = true, UnderlyingType DefaultValues = {})
				: ParamBase(Owner, ParamName, ParamTitle, ParamDescription, NeedsResetToApplyChange),
				DefaultValues(std::move(DefaultValues)), Values(this->DefaultValues) {}

			/**
			 * @copydoc ParamBase::ParamBase(ParamsBase&, std::string)
			 * @param DefaultValues List of default values to assign to the constructed parameter
			*/
			TypedListParamBase(ParamsBase& Owner, std::string ParamName, UnderlyingType DefaultValues = {})
				: ParamBase(Owner, ParamName),
				DefaultValues(std::move(DefaultValues)), Values(this->DefaultValues) {}

		public:
			/**
			 * @brief Returns this parameter's default value list. Thread-safe since a reference to a const member variable is returned.
			 * @return Parameter's default values
			*/
			const auto& GetDefaultValue() const noexcept { return DefaultValues; }

			/**
			 * @brief Returns the parameter's values.
			 * @return Current parameter value list
			*/
			const auto& Get() const noexcept { return Values; }

			/**
			 * @brief Assigns new values to this parameter. The operator cannot be accessed by @p Object because
			 * this function is not const.
			 * @param NewValues New values to assign
			 * @return Reference to this parameter
			 * @throws Util::InvalidArgException is thrown if the values to assign to the parameter are not considered
			 * valid as determined by a call to ParamBase::Validate().
			*/
			auto& operator=(const UnderlyingType& NewValues)
			{
				if (!ValidateValues(NewValues))
					throw Util::InvalidArgException(
						"An assignment to a list parameter cannot be executed since the given value vector is considered invalid.");

				Values = NewValues;
				return Get();
			}

		protected:
			virtual void ToXMLNodeChild(QDomDocument& Document, QDomElement& XMLElement) const override
			{
				QDomElement ValuesNode = Document.createElement("Values");

				for (const auto& value : Values)
				{
					QDomElement ValueNode = Document.createElement("v");
					QDomText NodeValue = Document.createTextNode(QString::fromStdString(Util::ToStr(value)));

					ValueNode.appendChild(NodeValue);
					ValuesNode.appendChild(ValueNode);
				}

				XMLElement.appendChild(ValuesNode);
			}

			virtual void FromXMLNodeChild(const QDomElement& XMLElement) override
			{
				QDomElement ValuesNode;

				try
				{
					auto ParamNode = Util::GetSingleChildDOMElement(XMLElement, GetParamName().data());
					ValuesNode = Util::GetSingleChildDOMElement(ParamNode, "Values");
				}
				catch ([[maybe_unused]] const Util::NotFoundException& e)
				{
					// Assume that a single value is assigned to the parameter (if it has been converted to a list parameter later on).
					const auto Value = Util::GetTFromDOMElement<ParamType>(XMLElement, GetParamName().data());

					Values.clear();
					Values.push_back(Value);

					return;
				}

				auto ValueNodes = Util::GetChildDOMNodes(ValuesNode, "v");

				Values.clear();
				for (const auto& value : ValueNodes)
					Values.push_back(Util::StrToT<ParamType>(value.toElement().text().toStdString()));
			}

		private:
			virtual bool ValidateChild() const override final { return ValidateValues(Values); }

			/**
			 * @brief Called by ValidateChild(). Validates a list of values.
			 * Override to validate the vector which is about to be assigned to the list parameter. Returns true by default.
			 * @param NewValues Values to be validated before the assignment
			 * @return Returns true if @p NewValues is considered valid, false otherwise.
			*/
			virtual bool ValidateValues(const UnderlyingType& NewValues) const { return true; }
			
			virtual void ResetChild() override { Values = DefaultValues; }

			const UnderlyingType DefaultValues;		//!< Values to assign to the parameter upon construction and reset
			UnderlyingType Values;					//!< Current parameter values
		};

		template <typename ParamType, typename = void>
		class Param;

		/**
		 * @brief General parameter for any @p ParamType (except the ones which are further specified below).
		 * @tparam ParamType Underlying parameter value type 
		*/
		template <typename ParamType>
		class Param<ParamType,
			std::enable_if_t<
				!std::is_arithmetic_v<ParamType> &&
				!std::is_base_of_v<Text, ParamType> &&
				!std::is_base_of_v<TextList, ParamType> &&
				!std::is_base_of_v<IndexedTextList, ParamType> &&
				!std::is_enum_v<ParamType> &&
				!std::is_base_of_v<ObjectLinkBase, ParamType>
			>>
			: public TypedParamBase<ParamType>
		{
		public:
			using TypedParamBase<ParamType>::operator=;

			/**
			 * @copydoc TypedParamBase::TypedParamBase(ParamsBase&, std::string, std::string_view, std::string_view, bool, ParamType)
			*/
			Param(ParamsBase& Owner, std::string ParamName, std::string_view ParamTitle, std::string_view ParamDescription, bool NeedsResetToApplyChange = true,
				ParamType DefaultValue = ParamType())
				: TypedParamBase<ParamType>(Owner, ParamName, ParamTitle, ParamDescription, NeedsResetToApplyChange, std::move(DefaultValue)) {}
			
			/**
			 * @copydoc TypedParamBase::TypedParamBase(ParamsBase&, std::string, ParamType)
			*/
			Param(ParamsBase& Owner, std::string ParamName, ParamType DefaultValue = ParamType())
				: TypedParamBase<ParamType>(Owner, ParamName, std::move(DefaultValue)) {}

		private:
			virtual void AddToDialogChild(ParamsConfigDialog& Dialog) override final
			{
				throw Util::NotImplementedException(
					"A general (unspecified) parameter cannot be added to a settings dialog.");
			}
		};

		template <typename ArithmeticType>
		using UnderlyingArithmeticParamType = TypedParamBase<ArithmeticType>;

		/**
		 * @brief Parameter for numbers. If @p ParamType is an arithmetic type (integral or floating point),
		 * then extend @p TypedParamBase's functionality by specifying the allowed number range and format.
		 * @tparam ArithmeticType Number type (std::is_arithmetic fulfilled)
		*/
		template <typename ArithmeticType>
		class Param<ArithmeticType, std::enable_if_t<std::is_arithmetic_v<ArithmeticType>>> : public UnderlyingArithmeticParamType<ArithmeticType>
		{
		public:
			using PrecisionType = unsigned short;
			using UnderlyingArithmeticParamType<ArithmeticType>::operator=;

			/**
			 * @copydoc TypedParamBase::TypedParamBase(ParamsBase&, std::string, std::string_view, std::string_view, bool, ParamType)
			 * @param MinValue Minimal allowed value
			 * @param MaxValue Maximal allowed value
			 * @param Increment Increment/decrement step size (used in the settings dialog for manual parameter adjustment)
			 * @param Precision Floating point precision (used in the settings dialog for manual parameter adjustment)
			*/
			Param(ParamsBase& Owner, std::string ParamName, std::string_view ParamTitle, std::string_view ParamDescription, bool NeedsResetToApplyChange = true,
				ArithmeticType DefaultValue = ArithmeticType(),
				ArithmeticType MinValue = std::numeric_limits<ArithmeticType>::lowest(), ArithmeticType MaxValue = std::numeric_limits<ArithmeticType>::max(),
				ArithmeticType Increment = 1, PrecisionType Precision = 0)
				: UnderlyingArithmeticParamType<ArithmeticType>(Owner, ParamName, ParamTitle, ParamDescription, NeedsResetToApplyChange, std::move(DefaultValue)),
				MinValue(MinValue), MaxValue(MaxValue), Increment(Increment), Precision(Precision) {}
			
			/**
			 * @copydoc TypedParamBase::TypedParamBase(ParamsBase&, std::string, ParamType)
			 * @param MinValue Minimal allowed value
			 * @param MaxValue Maximal allowed value
			 * @param Increment Increment/decrement step size (used in the settings dialog for manual parameter adjustment)
			 * @param Precision Floating point precision (used in the settings dialog for manual parameter adjustment)
			*/
			Param(ParamsBase& Owner, std::string ParamName, ArithmeticType DefaultValue = ArithmeticType(),
				ArithmeticType MinValue = std::numeric_limits<ArithmeticType>::lowest(), ArithmeticType MaxValue = std::numeric_limits<ArithmeticType>::max(),
				ArithmeticType Increment = 1, PrecisionType Precision = 0)
				: UnderlyingArithmeticParamType<ArithmeticType>(Owner, ParamName, std::move(DefaultValue)),
				MinValue(MinValue), MaxValue(MaxValue), Increment(Increment), Precision(Precision) {}

			/** @name Thread-safe getter functions
			 * Thread-safe since const member variables are returned.
			*/
			///@{
			ArithmeticType GetMinValue() const noexcept { return MinValue; }	//!< Returns the minimal allowed value.
			ArithmeticType GetMaxValue() const noexcept { return MaxValue; }	//!< Returns the maximal allowed value.
			ArithmeticType GetIncrement() const noexcept { return Increment; }	//!< Returns the value increment.
			PrecisionType GetPrecision() const noexcept { return Precision; }	//!< Returns the value precision.
			///@}

			/**
			 * @brief Allo explicit casting to bool. Instances of this class evaluate to zero if and only if the stored value is 0.
			*/
			explicit operator bool() const { return UnderlyingArithmeticParamType<ArithmeticType>::Get() != 0; }

		private:
			virtual void AddToDialogChild(ParamsConfigDialog& Dialog) override final
			{
				Dialog.AddParam({ ParamBase::GetParamTitle(), ParamBase::GetParamDescription() }, std::ref(*static_cast<UnderlyingArithmeticParamType<ArithmeticType>*>(this)),
					UnderlyingArithmeticParamType<ArithmeticType>::Get(), MinValue, MaxValue, Precision, Increment);
			}

			virtual bool ValidateValue(const ArithmeticType& Value) const override
			{
				return Value >= MinValue && Value <= MaxValue;
			}

			const ArithmeticType MinValue;	//!< Minimal allowed value
			const ArithmeticType MaxValue;	//!< Maximal allowed value
			const ArithmeticType Increment;	//!< Value increment (used in the settings dialog for manual parameter adjustment)
			const PrecisionType Precision;	//!< Value precision (used in the settings dialog for manual parameter adjustment)
		};

		using UnderlyingTextParamType = TypedParamBase<Util::TextType>;

		/**
		 * @brief Parameter for strings. If @p ParamType is of type ParamsBase::Text,
		 * then extend @p TypedParamBase's functionality by indicating the text's purpose and by overriding AddToDialogChild().
		 * @tparam ParamType Text type (same as or derived from ParamsBase::Text)
		*/
		template <typename ParamType>
		class Param<ParamType, std::enable_if_t<std::is_base_of_v<Text, ParamType>>> : public UnderlyingTextParamType
		{
		public:
			using UnderlyingTextParamType::operator=;

			/**
			 * @copydoc TypedParamBase::TypedParamBase(ParamsBase&, std::string, std::string_view, std::string_view, bool, ParamType)
			 * @param TextUsage Purpose of this parameter's text. Refer to DynExp::TextUsageType.
			*/
			Param(ParamsBase& Owner, std::string ParamName, std::string_view ParamTitle,
				std::string_view ParamDescription, bool NeedsResetToApplyChange = true, UnderlyingTextParamType::UnderlyingType DefaultValue = "",
				TextUsageType TextUsage = TextUsageType::Standard)
				: UnderlyingTextParamType(Owner, ParamName, ParamTitle, ParamDescription, NeedsResetToApplyChange, DefaultValue), TextUsage(TextUsage) {}
			
			/**
			 * @copydoc TypedParamBase::TypedParamBase(ParamsBase&, std::string, ParamType)
			*/
			Param(ParamsBase& Owner, std::string ParamName, UnderlyingTextParamType::UnderlyingType DefaultValue = "")
				: UnderlyingTextParamType(Owner, ParamName, DefaultValue), TextUsage(TextUsageType::Standard) {}

			/**
			 * @brief Returns the parameter's value as a path. Relative paths are resolved using ParamsBase::ToAbsolutePath().
			 * @return Current parameter value as a path
			*/
			std::filesystem::path GetPath() const { return GetOwner().ToAbsolutePath(Get()); }

			/** @name Thread-safe getter functions
			 * Thread-safe since const member variables are returned.
			*/
			///@{
			auto GetTextUsage() const noexcept { return TextUsage; }	//!< Returns the purpose of this parameter's text.
			///@}

		private:
			TextUsageType TextUsage;									//!< Purpose of this parameter's text

			virtual void AddToDialogChild(ParamsConfigDialog& Dialog) override final
			{
				Dialog.AddParam({ ParamBase::GetParamTitle(), ParamBase::GetParamDescription() }, std::ref(*static_cast<UnderlyingTextParamType*>(this)), Get(), TextUsage);
			}
		};

		using UnderlyingTextListParamType = TypedParamBase<Util::TextType>;

		/**
		 * @brief Parameter for strings selected from a predefined list of strings. If @p ParamType is of type ParamsBase::TextList,
		 * then extend @p TypedParamBase's functionality by providing text list functionality and by overriding AddToDialogChild().
		 * @tparam ParamType Text list type (same as or derived from ParamsBase::TextList)
		*/
		template <typename ParamType>
		class Param<ParamType, std::enable_if_t<std::is_base_of_v<TextList, ParamType>>> : public UnderlyingTextListParamType
		{
		public:
			using UnderlyingTextListParamType::operator=;

			/**
			 * @copydoc TypedParamBase::TypedParamBase(ParamsBase&, std::string, std::string_view, std::string_view, bool, ParamType)
			 * @param TextList Predefined list of strings to select a string from
			*/
			Param(ParamsBase& Owner, Util::TextListType&& TextList, std::string ParamName, std::string_view ParamTitle,
				std::string_view ParamDescription, bool NeedsResetToApplyChange = true, UnderlyingTextListParamType::UnderlyingType DefaultValue = "")
				: UnderlyingTextListParamType(Owner, ParamName, ParamTitle, ParamDescription, NeedsResetToApplyChange, DefaultValue),
				TextList(std::move(TextList)) {}
			
			/**
			 * @copydoc TypedParamBase::TypedParamBase(ParamsBase&, std::string, ParamType)
			 * @param TextList List of strings to select a string from
			*/
			Param(ParamsBase& Owner, Util::TextListType&& TextList, std::string ParamName,
				UnderlyingTextListParamType::UnderlyingType DefaultValue = "")
				: UnderlyingTextListParamType(Owner, ParamName, DefaultValue),
				TextList(std::move(TextList)) {}

			/** @name Thread-safe getter functions
			 * Thread-safe since const member variables are returned.
			*/
			///@{
			const auto& GetTextList() const noexcept { return TextList; }	//!< Returns the predefined selection options as a list of strings.
			///@}
	
			/** @name Not thread-safe
			 * Not (!) thread-safe, but these functions are intended to be called through ParamsBase::ConfigureParamsImpl() which
			 * takes care of synchronizing if an existing parameter object is to be edited by the user and not newly created.
			*/
			///@{
			/**
			 * @brief Resets the predefined list of strings to select a string from 
			 * @param NewTextList New predefined selection options as a list of strings
			*/
			void SetTextList(Util::TextListType&& NewTextList) { TextList = std::move(NewTextList); }
			///@}

		private:
			virtual void AddToDialogChild(ParamsConfigDialog& Dialog) override final
			{
				Dialog.AddParam({ ParamBase::GetParamTitle(), ParamBase::GetParamDescription() }, std::ref(*static_cast<UnderlyingTextListParamType*>(this)),
					Get(), TextList, !GetDefaultValue().empty());
			}

			/**
			 * @brief Predefined selection options.
			 * Not constant because this should be changeable dynamically (e.g. to enumerate connected devices just before
			 * displaying a settings dialog where a connected device can be chosen). Since the underlying type is just
			 * a string, there is no constraint on the allowed values - in contrast to e.g. an indexed text list.
			*/
			Util::TextListType TextList;
		};

		using UnderlyingIndexedTextListParamType = TypedParamBase<Util::TextListIndexType>;

		/**
		 * @brief Parameter for indexed strings selected from a predefined list of strings. If @p ParamType is of type ParamsBase::IndexedTextList,
		 * then extend @p TypedParamBase's functionality by providing text list functionality and by overriding AddToDialogChild().
		 * In this case, the parameter does not store the string itself, but the selected index (of type Util::TextListIndexType) of the predefined text list.
		 * @tparam ParamType Indexed text list type (same as or derived from ParamsBase::IndexedTextList)
		*/
		template <typename ParamType>
		class Param<ParamType, std::enable_if_t<std::is_base_of_v<IndexedTextList, ParamType>>> : public UnderlyingIndexedTextListParamType
		{
		public:
			using UnderlyingIndexedTextListParamType::operator=;

			/**
			 * @copydoc TypedParamBase::TypedParamBase(ParamsBase&, std::string, std::string_view, std::string_view, bool, ParamType)
			 * @param TextList Predefined list of strings to select a string from
			*/
			Param(ParamsBase& Owner, Util::TextListType&& TextList, std::string ParamName, std::string_view ParamTitle,
				std::string_view ParamDescription, bool NeedsResetToApplyChange = true, UnderlyingIndexedTextListParamType::UnderlyingType DefaultValue = 0)
				: UnderlyingIndexedTextListParamType(Owner, ParamName, ParamTitle, ParamDescription, NeedsResetToApplyChange, DefaultValue),
				TextList(std::move(TextList)) {}
			
			/**
			 * @copydoc TypedParamBase::TypedParamBase(ParamsBase&, std::string, ParamType)
			 * @param TextList List of strings to select a string from
			*/
			Param(ParamsBase& Owner, Util::TextListType&& TextList, std::string ParamName,
				UnderlyingIndexedTextListParamType::UnderlyingType DefaultValue = 0)
				: UnderlyingIndexedTextListParamType(Owner, ParamName, DefaultValue),
				TextList(std::move(TextList)) {}

			/** @name Thread-safe getter functions
			 * Thread-safe since const member variables are returned.
			*/
			///@{
			const auto& GetTextList() const noexcept { return TextList; }	//!< Returns the predefined selection options as a list of strings.
			///@}

		private:
			virtual void AddToDialogChild(ParamsConfigDialog& Dialog) override final
			{
				Dialog.AddParam({ ParamBase::GetParamTitle(), ParamBase::GetParamDescription() }, std::ref(*static_cast<UnderlyingIndexedTextListParamType*>(this)),
					Get(), GetDefaultValue(), TextList);
			}

			virtual bool ValidateValue(const ParamType& Value) const override { return Value < TextList.size(); }

			const Util::TextListType TextList;								//!< Predefined selection options
		};

		/**
		 * @brief If @p ParamType is an enum, then use largest signed/unsigned integral type as the underlying type
		 * since the enum's underlying type has to be integral.
		 * @tparam EnumType Enumeration type which is convertible to a numeric type (only unscoped enumerations)
		*/
		template <typename EnumType>
		using UnderlyingEnumParamType = TypedParamBase<LargestEnumUnderlyingType<EnumType>>;

		/**
		 * @brief Parameter for enumerations which are convertible to a numeric type. If @p ParamType is an enum,
		 * then extend @p TypedParamBase's functionality by providing conversion functionality and by overriding AddToDialogChild().
		 * @tparam EnumType Enumeration type which is convertible to a numeric type (only unscoped enumerations)
		*/
		template <typename EnumType>
		class Param<EnumType, std::enable_if_t<std::is_enum_v<EnumType>>> : public UnderlyingEnumParamType<EnumType>
		{
		public:
			/**
			 * @copydoc TypedParamBase::TypedParamBase(ParamsBase&, std::string, std::string_view, std::string_view, bool, ParamType)
			 * @param TextValueList Predefined list mapping strings to @p EnumType's items
			*/
			Param(ParamsBase& Owner, Util::TextValueListType<EnumType>&& TextValueList, std::string ParamName, std::string_view ParamTitle,
				std::string_view ParamDescription, bool NeedsResetToApplyChange = true, EnumType DefaultValue = EnumType())
				: UnderlyingEnumParamType<EnumType>(Owner, ParamName, ParamTitle, ParamDescription, NeedsResetToApplyChange, DefaultValue),
				TextValueList(std::move(TextValueList)) {}
			
			/**
			 * @copydoc TypedParamBase::TypedParamBase(ParamsBase&, std::string, ParamType)
			*/
			Param(ParamsBase& Owner, std::string ParamName, EnumType DefaultValue = EnumType())
				: UnderlyingEnumParamType<EnumType>(Owner, ParamName, DefaultValue) {}

			/** @name Thread-safe getter functions
			 * Thread-safe since const member variables are returned.
			*/
			///@{
			
			/**
			 * @brief Returns the default value of this parameter.
			 * @return @p EnumType's item which is selected by default
			*/
			EnumType GetDefaultValue() const noexcept { return static_cast<EnumType>(UnderlyingEnumParamType<EnumType>::GetDefaultValue()); }
			
			const auto& GetTextValueList() const noexcept { return TextValueList; }		//!< Returns the selection options as a mapping between strings and corresponding @p EnumType's items.
			///@}

			/**
			 * @brief Implicitely casts this parameter to the selected EnumType's item.
			*/
			operator EnumType() const noexcept { return static_cast<EnumType>(UnderlyingEnumParamType<EnumType>::Get()); }
			
			/**
			 * @brief Returns the selected EnumType's item of this parameter.
			 * @return Current parameter value cast to @p EnumType
			*/
			const EnumType Get() const noexcept { return static_cast<EnumType>(UnderlyingEnumParamType<EnumType>::Get()); }

			/**
			 * @brief Assigns a new value to this parameter.
			 * Attention: operator= can change the parameter to a value which is not included in @p EnumType!
			 * @param NewValue @p EnumType item to be assigned to the paramerer
			 * @return Current parameter value cast to @p EnumType
			*/
			const EnumType operator=(const EnumType& NewValue)
			{
				UnderlyingEnumParamType<EnumType>::operator=(NewValue);

				return Get();
			}

		private:
			virtual void AddToDialogChild(ParamsConfigDialog& Dialog) override final
			{
				Dialog.AddParam<EnumType>({ ParamBase::GetParamTitle(), ParamBase::GetParamDescription() }, std::ref(*static_cast<UnderlyingEnumParamType<EnumType>*>(this)),
					Get(), GetDefaultValue(), TextValueList);
			}

			const Util::TextValueListType<EnumType> TextValueList;						//!< Predefined selection options
		};

		using UnderlyingLinkParamType = TypedParamBase<ItemIDType>;

		/**
		 * @brief Base class for link parameters to a single @p Object of any type specifying the underlying
		 * parameter type. The underlying type is the integral DynExp::ItemIDType type, which stores the ID
		 * of the linked object.
		*/
		class LinkParamBase : public DynExp::LinkBase, public UnderlyingLinkParamType
		{
		protected:
			/**
			 * @copydoc TypedParamBase::TypedParamBase(ParamsBase&, std::string, std::string_view, std::string_view, bool, ParamType)
			 * @copydoc LinkBase::LinkBase
			 * @brief No further constructor since LinkParamBase parameters must be editable by the user.
			*/
			LinkParamBase(ParamsBase& Owner, std::string ParamName, std::string_view ParamTitle, std::string_view ParamDescription,
				bool NeedsResetToApplyChange = true, std::string_view IconResourcePath = {}, bool Optional = false)
				: LinkBase(IconResourcePath, Optional),
				UnderlyingLinkParamType(Owner, ParamName, ParamTitle, ParamDescription, NeedsResetToApplyChange, ItemIDNotSet) {}

		public:
			using UnderlyingLinkParamType::operator=;

		private:
			virtual std::string_view GetLinkTitleChild() const noexcept override { return GetParamTitle(); }
			virtual ItemIDListType GetLinkedIDsChild() const override { return Get() != ItemIDNotSet ? ItemIDListType{ Get() } : ItemIDListType{}; }
		};

		/**
		 * @brief Parameter for links to a single @p Object. If @p LinkType is derived from ObjectLinkBase,
		 * then extend @p TypedParamBase's functionality by providing link instantiation functionality and
		 * by overriding AddToDialogChild().
		 * @tparam LinkType Type derived from ObjectLinkBase (usually ObjectLink< ObjectT > instantiated
		 * with a type @p ObjectT derived from @p Object).
		*/
		template <typename LinkType>
		class Param<LinkType, std::enable_if_t<std::is_base_of_v<ObjectLinkBase, LinkType>>> : public LinkParamBase
		{
		public:
			using ObjectType = typename LinkType::ObjectType;			//!< Type of the linked @p Object
			using ManagerType = ManagerTypeOfObjectType_t<ObjectType>;	//!< Type of the manager managing the linked @p Object
			using LinkParamBase::operator=;

			/**
			 * @copydoc LinkParamBase::LinkParamBase
			 * @param Manager Reference to the manager instance containing the @p Object the link refers to
			*/
			Param(ParamsBase& Owner, const ManagerType& Manager,
				std::string ParamName, std::string_view ParamTitle, std::string_view ParamDescription,
				std::string_view IconResourcePath = {}, bool Optional = false, bool NeedsResetToApplyChange = true)
				: LinkParamBase(Owner, ParamName, ParamTitle, ParamDescription, NeedsResetToApplyChange, IconResourcePath, Optional),
				Manager(Manager)
			{
				// Since classes derived from ParamBase are declared within the scope of class ParamsBase, this is allowed.
				// This is safe since ObjectLinkParams is declared before any Param<LinkType> within the inheritance hierarchy.
				GetOwner().ObjectLinkParams.emplace_back(std::ref(*this));
			}

			/**
			 * @brief Returns the manager instance containing the @p Object the link refers to.
			 * @return Reference to manager instance
			*/
			const auto& GetManager() const noexcept { return Manager; }

			/**
			 * @brief Checks whether an item has been assigned to this parameter.
			 * @return Returns true if the assigned ID is not 'ItemIDNotSet', false otherwise.
			*/
			bool ContainsID() const noexcept { return Get() != ItemIDNotSet; }

			/**
			 * @brief Returns the link of type @p LinkType derived from ObjectLinkBase to the @p Object
			 * the parameter refers to.
			 * @return Returns const LinkType& in order to prohibit foreign classes to change ObjectLink. 
			*/
			const LinkType& GetLink() const noexcept { return Link; }

			/**
			 * @brief Retrives the linked resource using its ID stored in the parameter and establishes a
			 * link to the resource. Before using the resource, it has to be locked through the link by
			 * the owner of this parameter. This function is also called by ParamsConfigDialog::accept().
			 * @throws Util::InvalidStateException is thrown if @p Manager returns a resource of a type
			 * not matching @p ObjectType.
			*/
			void MakeLink()
			{
				if (!ContainsID())
					Link.Reset();
				else
				{
					auto Resource = std::dynamic_pointer_cast<ObjectType>(ShareResource(Manager, Get()));
					if (!Resource)
						throw Util::InvalidStateException(
							"A resource assigned to a param does not match the expected type.");

					Link = Resource;
				}
			}

		private:
			virtual const CommonResourceManagerBase* GetCommonManagerChild() const noexcept override { return &Manager; }

			virtual void FromXMLNodeChild(const QDomElement& XMLElement) override
			{
				UnderlyingLinkParamType::FromXMLNodeChild(XMLElement);
				MakeLink();
			}

			virtual void AddToDialogChild(ParamsConfigDialog& Dialog) override final
			{
				// Call MakeLink() after "OK" (accept) has been clicked to really link the parameter to the chosen object by creating pointers.
				auto FunctionToCallIfAccepted = std::bind_front(&Param::MakeLink, this);

				Dialog.AddParam({ ParamBase::GetParamTitle(), ParamBase::GetParamDescription() }, std::ref(*static_cast<LinkParamBase*>(this)),
					Get(), IsOptional(), GetIconResourcePath(), FunctionToCallIfAccepted, MakeObjectIDsWithLabels<ObjectType>(GetManager()));
			}

			/**
			 * @brief Thread-safe since only main thread is allowed to change parameters and to call this funtion.
			*/
			void EnsureReadyStateChild() override final
			{
				if (!ContainsID())
					return;

				auto LinkedObj = Link.TryLockDestinyRaw();
				if (LinkedObj)
					LinkedObj->EnsureReadyState(true);
			}

			bool IsReadyChild() override final
			{
				if (!ContainsID())
					return true;

				auto LinkedObj = Link.TryLockDestinyRaw();
				if (LinkedObj)
				{
					try
					{
						return LinkedObj->IsReady();
					}
					catch ([[maybe_unused]] const Util::TimeoutException& e)
					{
						// Swallow TimeoutException since this is thrown if IsReady() cannot check the object's exception state
						// because the object is busy and locked its data. This is considered not ready.
						return false;
					}
				}
				else
					return true;
			}

			/**
			 * @brief Reference to the manager instance containing the @p Object the link refers to.
			 * Managers live longer than their managed Objects which own the Params.
			*/
			const ManagerType& Manager;

			/**
			 * @brief Link of type @p LinkType derived from ObjectLinkBase referring to the linked @p Object.
			 * The link can be locked to access the underlying resource in a thread-safe way.
			*/
			LinkType Link;
		};

		template <typename ParamType, typename = void>
		class ListParam;

		/**
		 * @brief General list parameter for any @p ParamType (except the ones which are further specified below).
		 * @tparam ParamType Underlying value type of the parameter's list entries
		*/
		template <typename ParamType>
		class ListParam<ParamType,
			std::enable_if_t<
				!std::is_arithmetic_v<ParamType> &&
				!std::is_base_of_v<ObjectLinkBase, ParamType>
			>>
			: public TypedListParamBase<ParamType>
		{
		public:
			using TypedListParamBase<ParamType>::operator=;

			/**
			 * @copydoc TypedListParamBase::TypedListParamBase(ParamsBase&, std::string, std::string_view, std::string_view, bool, TypedListParamBase::UnderlyingType)
			*/
			ListParam(ParamsBase& Owner, std::string ParamName, std::string_view ParamTitle, std::string_view ParamDescription, bool NeedsResetToApplyChange = true,
				typename TypedListParamBase<ParamType>::UnderlyingType DefaultValues = {})
				: TypedListParamBase<ParamType>(Owner, ParamName, ParamTitle, ParamDescription, NeedsResetToApplyChange, std::move(DefaultValues)) {}
			
			/**
			 * @copydoc TypedListParamBase::TypedListParamBase(ParamsBase&, std::string, TypedListParamBase::UnderlyingType)
			*/
			ListParam(ParamsBase& Owner, std::string ParamName, typename TypedListParamBase<ParamType>::UnderlyingType DefaultValues = {})
				: TypedListParamBase<ParamType>(Owner, ParamName, std::move(DefaultValues)) {}

		private:
			virtual void AddToDialogChild(ParamsConfigDialog& Dialog) override final
			{
				throw Util::NotImplementedException(
					"A general (unspecified) list parameter cannot be added to a settings dialog.");
			}
		};

		template <typename ArithmeticType>
		using UnderlyingArithmeticListParamType = TypedListParamBase<ArithmeticType>;

		/**
		 * @brief List parameter for numbers. If @p ParamType is an arithmetic type (integral or floating point),
		 * then extend TypedListParamBase's functionality by specifying the allowed number range and format.
		 * @tparam ArithmeticType Number type (std::is_arithmetic fulfilled)
		*/
		template <typename ArithmeticType>
		class ListParam<ArithmeticType, std::enable_if_t<std::is_arithmetic_v<ArithmeticType>>> : public UnderlyingArithmeticListParamType<ArithmeticType>
		{
		public:
			using PrecisionType = unsigned short;
			using UnderlyingArithmeticListParamType<ArithmeticType>::operator=;

			/**
			 * @copydoc TypedListParamBase::TypedListParamBase(ParamsBase&, std::string, std::string_view, std::string_view, bool, TypedListParamBase::UnderlyingType)
			 * @param MinValue Minimal allowed value
			 * @param MaxValue Maximal allowed value
			 * @param Increment Increment/decrement step size (used in the settings dialog for manual parameter adjustment)
			 * @param Precision Floating point precision (used in the settings dialog for manual parameter adjustment)
			*/
			ListParam(ParamsBase& Owner, std::string ParamName, std::string_view ParamTitle, std::string_view ParamDescription, bool NeedsResetToApplyChange = true,
				typename TypedListParamBase<ArithmeticType>::UnderlyingType DefaultValues = {},
				ArithmeticType MinValue = std::numeric_limits<ArithmeticType>::lowest(), ArithmeticType MaxValue = std::numeric_limits<ArithmeticType>::max(),
				ArithmeticType Increment = 1, PrecisionType Precision = 0)
				: UnderlyingArithmeticListParamType<ArithmeticType>(Owner, ParamName, ParamTitle, ParamDescription, NeedsResetToApplyChange, std::move(DefaultValues)),
				MinValue(MinValue), MaxValue(MaxValue), Increment(Increment), Precision(Precision) {}
			
			/**
			 * @copydoc TypedListParamBase::TypedListParamBase(ParamsBase&, std::string, TypedListParamBase::UnderlyingType)
			 * @param MinValue Minimal allowed value
			 * @param MaxValue Maximal allowed value
			 * @param Increment Increment/decrement step size (used in the settings dialog for manual parameter adjustment)
			 * @param Precision Floating point precision (used in the settings dialog for manual parameter adjustment)
			*/
			ListParam(ParamsBase& Owner, std::string ParamName, typename TypedListParamBase<ArithmeticType>::UnderlyingType DefaultValues = {},
				ArithmeticType MinValue = std::numeric_limits<ArithmeticType>::lowest(), ArithmeticType MaxValue = std::numeric_limits<ArithmeticType>::max(),
				ArithmeticType Increment = 1, PrecisionType Precision = 0)
				: UnderlyingArithmeticListParamType<ArithmeticType>(Owner, ParamName, std::move(DefaultValues)),
				MinValue(MinValue), MaxValue(MaxValue), Increment(Increment), Precision(Precision) {}

			/** @name Thread-safe getter functions
			 * Thread-safe since const member variables are returned.
			*/
			///@{
			ArithmeticType GetMinValue() const noexcept { return MinValue; }	//!< Returns the minimal allowed value of the list items.
			ArithmeticType GetMaxValue() const noexcept { return MaxValue; }	//!< Returns the maximal allowed value of the list items.
			ArithmeticType GetIncrement() const noexcept { return Increment; }	//!< Returns the value increment of the list items.
			PrecisionType GetPrecision() const noexcept { return Precision; }	//!< Returns the value precision of the list items.
			///@}

		private:
			virtual void AddToDialogChild(ParamsConfigDialog& Dialog) override final
			{
				throw Util::NotImplementedException(
					"An arithmetic list parameter cannot be added to a settings dialog currently.");
			}

			bool ValidateValues(const typename TypedListParamBase<ArithmeticType>::UnderlyingType& NewValues) const override
			{
				for (const auto value : NewValues)
					if (value < MinValue || value > MaxValue)
						return false;

				return true;
			}

			const ArithmeticType MinValue;	//!< Minimal allowed list item value
			const ArithmeticType MaxValue;	//!< Maximal allowed list item value
			const ArithmeticType Increment;	//!< List item value increment (used in the settings dialog for manual parameter adjustment)
			const PrecisionType Precision;	//!< List item value precision (used in the settings dialog for manual parameter adjustment)
		};

		using UnderlyingLinkListParamType = TypedListParamBase<ItemIDType>;

		/**
		 * @brief Base class for link list parameters to multiple @p Object of any (but all the same)
		 * type specifying the underlying parameter type. The underlying type is the
		 * integral DynExp::ItemIDType type, which stores the IDs of the linked objects.
		*/
		class LinkListParamBase : public DynExp::LinkBase, public UnderlyingLinkListParamType
		{
		protected:
			/**
			 * @copydoc TypedParamBase::TypedParamBase(ParamsBase&, std::string, std::string_view, std::string_view, bool, ParamType)
			 * @copydoc LinkBase::LinkBase
			 * @brief No further constructor since LinkParamBase parameters must be editable by the user.
			*/
			LinkListParamBase(ParamsBase& Owner, std::string ParamName, std::string_view ParamTitle, std::string_view ParamDescription,
				bool NeedsResetToApplyChange = true, std::string_view IconResourcePath = {}, bool Optional = false)
				: LinkBase(IconResourcePath, Optional),
				UnderlyingLinkListParamType(Owner, ParamName, ParamTitle, ParamDescription, NeedsResetToApplyChange) {}

		public:
			using UnderlyingLinkListParamType::operator=;

		private:
			virtual std::string_view GetLinkTitleChild() const noexcept override { return GetParamTitle(); }
			virtual ItemIDListType GetLinkedIDsChild() const override { return Get(); }
		};

		/**
		 * @brief Parameter for link lists to multiple @p Object. If @p LinkType is derived from ObjectLinkBase,
		 * then extend @p TypedParamBase's functionality by providing link instantiation functionality and
		 * by overriding AddToDialogChild().
		 * @tparam LinkType Type derived from ObjectLinkBase (usually ObjectLink< ObjectT > instantiated
		 * with a type @p ObjectT derived from @p Object).
		*/
		template <typename LinkType>
		class ListParam<LinkType, std::enable_if_t<std::is_base_of_v<ObjectLinkBase, LinkType>>> : public LinkListParamBase
		{
		public:
			using ObjectType = typename LinkType::ObjectType;			//!< Type of the linked @p Object
			using ManagerType = ManagerTypeOfObjectType_t<ObjectType>;	//!< Type of the manager managing the linked @p Object
			using LinkListParamBase::operator=;

			/**
			 * @copydoc LinkParamBase::LinkParamBase
			 * @param Manager Reference to the manager instance containing the @p Object the link refers to
			*/
			ListParam(ParamsBase& Owner, const ManagerType& Manager,
				std::string ParamName, std::string_view ParamTitle, std::string_view ParamDescription,
				std::string_view IconResourcePath = {}, bool Optional = false, bool NeedsResetToApplyChange = true)
				: LinkListParamBase(Owner, ParamName, ParamTitle, ParamDescription, NeedsResetToApplyChange, IconResourcePath, Optional),
				Manager(Manager)
			{
				// Since classes derived from ParamBase are declared within the scope of class ParamsBase, this is allowed.
				// This is safe since ObjectLinkParams is declared before any Param<LinkType> within the inheritance hierarchy.
				GetOwner().ObjectLinkParams.emplace_back(std::ref(*this));
			}

			/**
			 * @brief Returns the manager instance containing the @p Object the link refers to
			 * @return Reference to manager instance
			*/
			const auto& GetManager() const noexcept { return Manager; }

			/**
			 * @brief Checks whether at least one item has been assigned to this parameter.
			 * Assigned items are not allowed to have the ID 'ItemIDNotSet' (see @p ValidateValues).
			 * @return Returns true if there is at least one ID assigned, false otherwise.
			*/
			bool ContainsID() const noexcept { return !Get().empty(); }

			/**
			 * @brief Returns a list of links of type @p LinkType derived from ObjectLinkBase to the @p Object
			 * this parameter refers to.
			 * @return Returns const vector<...>& in order to prohibit foreign classes to change ObjectLinks.
			*/
			const auto& GetLinks() const noexcept { return Links; }

			/**
			 * @brief Retrives the linked resource using its ID stored in the parameter and establishes a
			 * link to the resource. Before using the resource, it has to be locked through the link by
			 * the owner of this parameter. This function is also called by ParamsConfigDialog::accept().
			 * @throws Util::InvalidStateException is thrown if @p Manager returns a resource of a type
			 * not matching @p ObjectType.
			*/
			void MakeLinks()
			{
				Links.clear();

				if (ContainsID())
				{
					for (auto ID : Get())
					{
						auto Resource = std::dynamic_pointer_cast<ObjectType>(ShareResource(Manager, ID));
						if (!Resource)
							throw Util::InvalidStateException(
								"A resource assigned to a list parameter does not match the expected type.");

						Links.emplace_back(Resource);
					}
				}
			}

		private:
			virtual const CommonResourceManagerBase* GetCommonManagerChild() const noexcept override { return &Manager; }

			virtual void FromXMLNodeChild(const QDomElement& XMLElement) override
			{
				UnderlyingLinkListParamType::FromXMLNodeChild(XMLElement);
				MakeLinks();
			}

			virtual void AddToDialogChild(ParamsConfigDialog& Dialog) override final
			{
				// Call MakeLink() after "OK" (accept) has been clicked to really link the parameter to the chosen object by creating pointers.
				auto FunctionToCallIfAccepted = std::bind_front(&ListParam::MakeLinks, this);

				Dialog.AddParam({ ParamBase::GetParamTitle(), ParamBase::GetParamDescription() }, std::ref(*static_cast<LinkListParamBase*>(this)),
					Get(), IsOptional(), GetIconResourcePath(), FunctionToCallIfAccepted, MakeObjectIDsWithLabels<ObjectType>(GetManager()));
			}

			/**
			 * @brief Ensures that list entries do not have the ItemIDNotSet value.
			 * @copydetails TypedListParamBase::ValidateValues()
			*/
			bool ValidateValues(const typename LinkListParamBase::UnderlyingType& NewValues) const override
			{
				for (const auto value : NewValues)
					if (value == ItemIDNotSet)
						return false;

				return true;
			}

			/**
			 * @brief Thread-safe since only main thread is allowed to change parameters and to call this funtion.
			*/
			void EnsureReadyStateChild() override final
			{
				if (!ContainsID())
					return;

				for (auto& Link : Links)
				{
					auto LinkedObj = Link.TryLockDestinyRaw();
					if (LinkedObj)
						LinkedObj->EnsureReadyState(true);
				}
			}

			bool IsReadyChild() override final
			{
				if (!ContainsID())
					return true;

				for (auto& Link : Links)
				{
					auto LinkedObj = Link.TryLockDestinyRaw();
					if (!LinkedObj)
						continue;

					try
					{
						if (!LinkedObj->IsReady())
							return false;
					}
					catch ([[maybe_unused]] const Util::TimeoutException& e)
					{
						// Swallow TimeoutException since this is thrown if IsReady() cannot check the object's exception state
						// because the object is busy and locked its data. This is considered not ready.
						return false;
					}
				}

				return true;
			}

			/**
			 * @brief Reference to the manager instance containing the @p Object the link refers to.
			 * Managers live longer than their managed Objects which own the Params.
			*/
			const ManagerType& Manager;

			/**
			 * @brief List of links of type @p LinkType derived from ObjectLinkBase referring to the linked @p Object.
			 * The links can be locked to access the underlying resources in a thread-safe way.
			*/
			std::vector<LinkType> Links;
		};

		/**
		 * @brief Makes sure the object link parameters of the associated ParamsBase instance are in a ready state by
		 * performing the initialization stepwise to allow updating the UI in between.
		 * Instances of this class can only be used once.
		*/
		class LinkParamStarter
		{
		public:
			/**
			 * @brief Constructs a LinkParamStarter instance for single use.
			 * @param Params Reference to a parameter class whose object link parameters are to be moved into a ready state
			*/
			LinkParamStarter(const ParamsBase& Params) noexcept
				: Params(Params), CurrentParam(Params.ObjectLinkParams.cbegin()), EnsureReadyStateCalledForCurrentParam(false) {}

			/**
			 * @brief Performs the initialization stepwise. This functions is to be called as long as it returns true.
			 * It returns after every step to allow doing other work in the meantime (e.g. updating the UI).
			 * @return Returns true if EnsureReadyState() of every item in Params' ObjectLinkParams vector has been called, otherwise false.
			*/
			bool operator()();

		private:
			const ParamsBase& Params;										//!< Reference to a parameter class whose object link parameters have to be ready
			ParamsBase::ObjectLinkParamsType::const_iterator CurrentParam;	//!< Iterator to the parameter which is currently being made ready
			bool EnsureReadyStateCalledForCurrentParam;						//!< Waiting for the current parameter to become ready or moving on to the next parameter?
		};

		/**
		 * @brief Determines whether an @p Object can be linked to only one (unique) or multiple (shared) other objects
		*/
		enum UsageType { Unique, Shared };
		/**
		 * @var ParamsBase::UsageType ParamsBase::Unique
		 * The @p Object instance can only be used by one other instance.
		*/
		/**
		 * @var ParamsBase::UsageType ParamsBase::Shared
		 * The @p Object instance can be used by multiple other instances at the same time.
		*/

		/**
		 * @brief Constructs the base class of an object parameter class.
		 * @param ID ID of the @p Object this parameter class instance belongs to
		 * @param Core Reference to %DynExp's core
		*/
		ParamsBase(ItemIDType ID, const DynExpCore& Core) : ID(ID), Core(Core) {}

		virtual ~ParamsBase() = 0;

		/**
		 * @brief This function is intended to be overridden once in each derived class returning the name of the respective class.
		 * Parameters defined in any class within the ParamsBase hierarchy will receive a 'this' pointer pointing to ParamsBase
		 * when their ParamBase constructors are called. The ParamBase constructors will register the respective parameter in
		 * ParamsBase::OwnedParams calling GetParamClassTag() on the given 'this' pointer. This allows the ParamBase constructor
		 * to obtain the name of the class where the respective parameter was declared in. The name is used as an XML tag
		 * containing related parameters in the project files. Here, it is fully intended that the virtual call to
		 * GetParamClassTag() leads only to a call of GetParamClassTag() of the current dynamic type of ParamsBase!
		 * @return Name of the class this function is defined in
		*/
		virtual const char* GetParamClassTag() const noexcept { return "ParamsBase"; }

		/**
		 * @brief Creates an XML node with a tag name as determined by GetParamClassTag() containing all
		 * parameters belonging to this ParamsBase instance
		 * @param Document XML document to create the DOM elements in
		 * @return DOM element containing the XML root node of this ParamsBase instance
		*/
		QDomElement ConfigToXML(QDomDocument& Document) const;

		/**
		 * @brief Retrieves all parameters belonging to this ParamsBase instance from an XML node with
		 * a tag name as determined by GetParamClassTag()
		 * @param XMLElement XML element containing a single child node with a tag name as determined
		 * by GetParamClassTag()
		*/
		void ConfigFromXML(const QDomElement& XMLElement) const;

		/**
		 * @brief Adds all parameters belonging to this ParamsBase instance to a settings dialog to
		 * let the user configure the parameter values.
		 * @param Dialog Settings dialog to add UI controls for adjusting the parameters to. The dialog
		 * is not automatically opened/shown by this function.
		*/
		void ConfigFromDialog(ParamsConfigDialog& Dialog);

		/**
		 * @brief Refer to ParamBase::Validate().
		 * @return Returns true if all owned parameters are valid. Returns false if at least one was invalid
		 * and has been reset.
		*/
		bool Validate() const;

		ItemIDType GetID() const noexcept { return ID; }								//!< Returns the ID of the @p Object this parameter class instance belongs to.
		const auto& GetCore() const noexcept { return Core; }							//!< Returns a reference to %DynExp's core.
		const auto& GetObjectLinkParams() const noexcept { return ObjectLinkParams; }	//!< Returns a list of all object link parameters owned by this parameter class instance.

		/**
		 * @brief Maps description strings to the ParamsBase::UsageType enum's items.
		 * @return List containing the description-value mapping
		*/
		static Util::TextValueListType<UsageType> AvlblUsageTypeStrList();

	private:
		/**
		 * @brief Helper type owning a reference to a parameter and its class tag
		*/
		struct OwnedParamInfo
		{
			/**
			 * @brief Denotes the result of GetParamClassTag() of the class where the respective OwnedParam was declared in.
			*/
			const char* const ClassTag;

			/**
			 * @brief Reference to the parameter
			*/
			const std::reference_wrapper<ParamBase> OwnedParam;
		};

		/** @name Important declaration order
		 * These vectors must be declared before any members derived from ParamBase are declared! This is the case since
		 * ParamBase's constructor and constructors of classes derived from ParamBase access these vectors. Declaring
		 * these vectors first ensures that their lifetimes exceed those of any members derived from ParamBase.
		 * The order of OwnedParams is important! Subsequent entries with equal OwnedParamInfo::ClassTag will be stored
		 * within the same XML tag. If OwnedParamInfo::ClassTag differs from the previous one, a respective sub-tag is
		 * introduced.
		*/
		///@{
		OwnedParamsType OwnedParams;			//!< List of all parameters owned by this parameter class instance
		ObjectLinkParamsType ObjectLinkParams;	//!< List of all object link parameters owned by this parameter class instance
		///@}

	public:
		/**
		 * @brief String set by the user to identify the object this parameter class instance belongs to
		*/
		Param<ParamsConfigDialog::TextType> ObjectName = { *this, "ObjectName", "Name", "Name to identify this item", false };

		/**
		 * @brief Determines whether an object can be used by only one other ("unique") or by multiple other ("shared") objects
		*/
		Param<UsageType> Usage = { *this, AvlblUsageTypeStrList(), "Usage", "Usage type",
			"Determines how this item can be used by other items", false, UsageType::Shared };

		/**
		 * @brief Determines whether the Usage parameter should be configurable in the settings dialog.
		 * Override ConfigureUsageTypeChild() in order to adjust.
		 * @return Returns true if the Usage parameter should be configurable in the settings dialog.
		*/
		bool ConfigureUsageType() const noexcept { return ConfigureUsageTypeChild(); }

		/**
		 * @brief Returns the network address parameters of a derived gRPC instrument.
		 * Override GetNetworkAddressParamsChild() in order to adjust.
		 * @return Pointer to parameters describing a network address and a network port. nullptr if not
		 * applicable (if the derived object is neither a gRPC server nor a gRPC client).
		*/
		const NetworkParamsExtension* GetNetworkAddressParams() const noexcept { return GetNetworkAddressParamsChild(); }

		/**
		 * @brief Sets the @p UserEditable property of the parameter @p Param to false.
		 * Refer to ParamBase::UserEditable
		 * @param Param Parameter for which to set UserEditable to false
		*/
		static void DisableUserEditable(ParamBase& Param) noexcept;

	private:
		/**
		 * @brief Called by ConfigFromDialog() to apply changes to parameters owned by this parameter class instance
		 * before a settings dialog is displayed to let the user configure the parameters.
		*/
		void ConfigureParams();

		/**
		 * @brief Called by DynExp::ParamsBase::ConfigureParams() as a starting point for the tag dispatch
		 * mechanism to descend the inheritance hierarchy. Override to add functionality to ConfigureParams().
		 * Refer to DynExp::ParamsBase::dispatch_tag.
		*/
		virtual void ConfigureParamsImpl(dispatch_tag<ParamsBase>) {};

		/**
		 * @copydoc ConfigureUsageType()
		*/
		virtual bool ConfigureUsageTypeChild() const noexcept { return true; }

		/**
		 * @copydoc GetNetworkAddressParams()
		*/
		virtual const NetworkParamsExtension* GetNetworkAddressParamsChild() const noexcept { return nullptr; }

		/**
		 * @copydoc DynExpCore::ToAbsolutePath()
		*/
		std::filesystem::path ToAbsolutePath(const std::filesystem::path& Path) const;

		const ItemIDType ID;		//!< ID of the @p Object this parameter class instance belongs to
		const DynExpCore& Core;		//!< Reference to %DynExp's core
	};

	/**
	 * @brief Provides (in)equality comparison operators for two parameters.
	 * Since C++20, inequality operators (operator!=) are generated by compiler.
	 * @tparam ParamType Underlying parameter value type
	 * @param lhs Left-hand side of the comparison
	 * @param rhs Right-hand side of the comparison
	 * @return Returns true if the value of @p lhs equals the value of @p rhs, false otherwise.
	*/
	template <typename ParamType>
	auto operator==(const ParamsBase::TypedParamBase<ParamType>& lhs, const ParamType& rhs) { return lhs.Get() == rhs; }

	/**
	 * @brief Provides three-way comparison for two parameters.
	 * @tparam ParamType Underlying parameter value type
	 * @param lhs Left-hand side of the comparison
	 * @param rhs Right-hand side of the comparison
	 * @return Returns the result of a three-way comparison between the values of @p lhs and @p rhs.
	*/
	template <typename ParamType>
	auto operator<=>(const ParamsBase::TypedParamBase<ParamType>& lhs, const ParamType& rhs) { return lhs.Get() <=> rhs; }

	/**
	 * @brief Alias for a pointer to the parameter system base class @p ParamsBase
	*/
	using ParamsBasePtrType = std::unique_ptr<ParamsBase>;

	/**
	 * @brief Factory function to generate an @p Object's parameters.
	 * @tparam ConfiguratorT The object type is determined by the @p Object's configurator
	 * derived from @p ConfiguratorBase.
	 * @param ID ID of the @p Object to generate parameters for
	 * @param Core Reference to %DynExp's core
	 * @return Pointer to the @p Object's parameters
	*/
	template <typename ConfiguratorT>
	ParamsBasePtrType MakeParams(ItemIDType ID, const DynExpCore& Core)
	{
		auto Params = std::make_unique<typename ConfiguratorT::ParamsType>(ID, Core);

		Params->ObjectName = ConfiguratorT::ObjectType::Name();

		return Params;
	}

	/**
	 * @brief Casts the parameter base class to a derived @p Object's parameter class.
	 * @tparam T Type derived from @p Object to whose parameter type to cast
	 * @param Params Pointer to the parameter base class
	 * @return Returns a pointer to the cast parameter class. Returns nullptr if @p Params is nullptr.
	 * @throws Util::TypeErrorException is thrown if the cast fails.
	*/
	template <typename T>
	typename T::ParamsType* dynamic_Params_cast(ParamsBasePtrType::element_type* Params)
	{
		if (!Params)
			return nullptr;

		auto DerivedParams = dynamic_cast<typename T::ParamsType*>(Params);
		if (!DerivedParams)
			throw Util::TypeErrorException();

		return DerivedParams;
	}

	/**
	 * @brief Casts the parameter base class @p From to a derived @p Object's (@p To) parameter
	 * class keeping the parameters locked by Util::SynchronizedPointer for thread-safe casting.
	 * @tparam To Type derived from @p Object to whose parameter type to cast.
	 * @p const is added to @p To if @p From is @p const.
	 * @tparam From Parameter type to cast from (must be @p ParamsBase).
	 * This type is automatically deduced.
	 * @param ParamsPtr Locked parameters to cast from. @p ParamsPtr is empty after the cast.
	 * @return Locked parameters cast to the parameter class belonging to @p To.
	 * @throws Util::InvalidArgException is thrown if @p ParamsPtr is empty.
	 * @throws Util::TypeErrorException is thrown if the cast fails.
	*/
	template <typename To, typename From, std::enable_if_t<
		std::is_same_v<ParamsBase, std::remove_cv_t<From>>, int> = 0
	>
	auto dynamic_Params_cast(Util::SynchronizedPointer<From>&& ParamsPtr)
	{
		if (!ParamsPtr)
			throw Util::InvalidArgException("ParamsPtr must not be nullptr.");

		return Util::SynchronizedPointer<
			std::conditional_t<std::is_const_v<From>, std::add_const_t<typename To::ParamsType>, typename To::ParamsType>
		>(std::move(ParamsPtr));
	}

	/**
	 * @brief The configurator classes have the task to generate parameter objects
	 * (refer to DynExp::ParamsBase) of the corresponding type for %DynExp objects
	 * DynExp::Object. For each class derived from DynExp::Object, there must be a
	 * respective configurator derived from ConfiguratorBase.
	*/
	class ConfiguratorBase
	{
	public:
		using ObjectType = Object;
		using ParamsType = ParamsBase;

		/**
		 * @brief Return type of ConfiguratorBase::UpdateConfigFromDialog() indicating the result
		 * of the configurator dialog shown to the user to configure an @p Object's parameters.
		*/
		struct UpdateConfigFromDialogResult
		{
			/**
			 * @brief Constructs an UpdateConfigFromDialogResult instance.
			 * @param Accepted Refer to UpdateConfigFromDialogResult::Accepted.
			 * @param ResetRequired Refer to UpdateConfigFromDialogResult::ResetRequired.
			*/
			constexpr UpdateConfigFromDialogResult(bool Accepted, bool ResetRequired) noexcept
				: Accepted(Accepted), ResetRequired(ResetRequired) {}

			constexpr bool IsAccepted() const noexcept { return Accepted; }		//!< Returns UpdateConfigFromDialogResult::Accepted.

			/**
			 * @brief Checks whether the corresponding @p Object has to be reset to apply changed parameters.
			 * @return Returns whether UpdateConfigFromDialogResult::Accepted and UpdateConfigFromDialogResult::ResetRequired are true.
			*/
			constexpr bool IsResetRequired() const noexcept { return IsAccepted() && ResetRequired; }

		private:
			const bool Accepted;		//!< Has the dialog been accepted clicking 'OK'?
			const bool ResetRequired;	//!< Has the user changed settings which require resetting the corresponding @p Object?
		};

		ConfiguratorBase() = default;
		virtual ~ConfiguratorBase() = 0;

		/**
		 * @brief Sets up and displays a configuration dialog. The user input is used to create
		 * an instance of the related @p Object's parameter class.
		 * @param ID ID to assign to the new @p Object this configurator class instance belongs to
		 * @param Core Reference to %DynExp's core
		 * @param DialogParent Qt QWidget acting as a parent of the modal configuration dialog
		 * @return Pointer to the created parameter class instance
		*/
		ParamsBasePtrType MakeConfigFromDialog(ItemIDType ID, const DynExpCore& Core, QWidget* const DialogParent) const;

		/**
		 * @brief Retrieves the configuration from an XML node to create an instance of the related
		 * @p Object's parameter class. Refer to ParamsBase::ConfigFromXM().
		 * @param ID ID to assign to the new @p Object this configurator class instance belongs to
		 * @param Core Reference to %DynExp's core
		 * @param XMLElement Refer to ParamsBase::ConfigFromXML().
		 * @return Pointer to the created parameter class instance
		*/
		ParamsBasePtrType MakeConfigFromXML(ItemIDType ID, const DynExpCore& Core, const QDomElement& XMLElement) const;

		/**
		 * @brief Updates an @p Object's @p Obj parameters from a configuration dialog.
		 * Sets up and displays this dialog.
		 * @param Obj @p Object whose parameters are to be reconfigured
		 * @param Core Reference to %DynExp's core
		 * @param DialogParent Qt QWidget acting as a parent of the modal configuration dialog
		 * @return Refer to ConfiguratorBase::UpdateConfigFromDialogResult.
		*/
		UpdateConfigFromDialogResult UpdateConfigFromDialog(Object* Obj, const DynExpCore& Core, QWidget* const DialogParent) const;

	private:
		/**
		 * @brief Override to make derived classes call DynExp::MakeParams with the correct
		 * configurator type derived from @p ConfiguratorBase. This lets this factory function
		 * create an instance of the correct parameter class.
		 * @param ID ID to assign to the new @p Object this configurator class instance belongs to
		 * @param Core Reference to %DynExp's core
		 * @return Pointer to the created parameter class instance
		*/
		virtual ParamsBasePtrType MakeParams(ItemIDType ID, const DynExpCore& Core) const = 0;
	};

	/**
	 * @brief Alias for a pointer to the configurator base class @p ConfiguratorBase
	*/
	using ConfiguratorBasePtrType = std::shared_ptr<ConfiguratorBase>;

	/**
	 * @brief Base class for all %DynExp Objects like hardware adapters (DynExp::HardwareAdapterBase),
	 * instruments (DynExp::InstrumentBase) and modules (DynExp::ModuleBase).
	 * Subclasses should also derive either form Util::INonCopyable or Util::ILockable since Object
	 * is not intended to be copied. Especially, it must not be moved to another thread (than the one
	 * creating it).
	 * Logical const-ness: const member functions can be called by other threads, non-const functions are
	 * visible only to main thread.
	*/
	class Object
	{
	public:
		/**
		 * @brief Type of the parameter class belonging to this @p Object type. Declare this alias in every
		 * derived class with the respective parameter class accompanying the derived @p Object.
		*/
		using ParamsType = ParamsBase;

		/**
		 * @brief Alias for the return type of Object::GetParams(). Parameters wrapped into
		 * Util::SynchronizedPointer can be accessed in a thread-safe way.
		*/
		using ParamsTypeSyncPtrType = Util::SynchronizedPointer<ParamsType>;

		/**
		 * @brief Alias for the return type of Object::GetParams() const. Parameters wrapped into
		 * Util::SynchronizedPointer can be accessed in a thread-safe way.
		*/
		using ParamsConstTypeSyncPtrType = Util::SynchronizedPointer<const ParamsType>;

		/**
		 * @brief Type of the configurator class belonging to this @p Object type. Declare this alias in every
		 * derived class with the respective configurator class accompanying the derived @p Object.
		*/
		using ConfigType = ConfiguratorBase;

		/**
		 * @brief Builds a string from an @p Object's category and name to allow the user to identify an
		 * @p Object's type.
		 * @param Category Refer to Object::GetCategory().
		 * @param Name Refer to Object::GetName().
		 * @return Returns a human-readable string to identify a (derived) @p Object's type.
		*/
		static std::string CategoryAndNameToStr(const std::string& Category, const std::string& Name);

		/**
		 * @brief Default timeout used by Object::GetParams() to lock the mutex of the parameter instance
		 * assigned to this @p Object instance.
		*/
		static constexpr std::chrono::milliseconds GetParamsTimeoutDefault = std::chrono::milliseconds(100);

	protected:
		/**
		 * @brief Refer to ParamsBase::dispatch_tag.
		 * @tparam Type (derived from class @p Object) to instantiate the dispatch tag template with.
		*/
		template <typename>
		struct dispatch_tag {};

		/**
		 * @brief Constructs an @p Object instance.
		 * @param OwnerThreadID Thread id of the thread owning the @p Object instance to be constructed.
		 * @param Params Parameter class instance to be assigned to the @p Object instance to be constructed. 
		 * @throws Util::InvalidArgException is thrown if @p OwnerThreadID is an invalid thread id or if
		 * @p Params is nullptr.
		*/
		Object(const std::thread::id OwnerThreadID, ParamsBasePtrType&& Params);

		virtual ~Object() = 0;

	private:
		/**
		 * @brief Allow exclusive access to some of @p Object's private methods to any LinkedObjectWrapper<T>.
		*/
		class LinkedObjectWrapperOnlyType
		{
			friend class Object;

			template <typename>
			friend class LinkedObjectWrapper;

			/**
			 * @brief Construcs an instance - one for each @p Object instance
			 * @param Parent Owning @p Object instance
			*/
			constexpr LinkedObjectWrapperOnlyType(Object& Parent) noexcept : Parent(Parent) {}

			/** @name (De)Registering @p Object's making use of this @p Object instance
			 * Logical const-ness: users are allowed to make use of const Objects. So, registering and deregistering
			 * them to const objects must be allowed
			*/
			///@{
			/**
			 * @copydoc ObjectUserList::Register(const Object&, const std::chrono::milliseconds)
			 * @throws Util::InvalidStateException is thrown if @p Parent is not ready. Refer to
			 * Object::IsReady().
			 * @throws Util::NotAvailableException is thrown if shared usage for @p Parent is not
			 * enabled and if @p Parent is not unused. Refer to ParamsBase::UsageType.
			*/
			void RegisterUser(const Object& User, const std::chrono::milliseconds Timeout) const;

			/**
			 * @copydoc ObjectUserList::Deregister
			*/
			void DeregisterUser(const Object& User, const std::chrono::milliseconds Timeout) const;
			///@}

			Object& Parent;		//!< Owning @p Object instance
		};

	public:
		/** @name Not thread-safe
		 * These functions must be called by the thread owning this @p Object instance only!
		 * Additionally, these functions must only be called when this @p Object instance is not
		 * in use by another @p Object instance!
		*/
		///@{
		/**
		 * @brief Resets this @p Object instance (including all its derived classes) by calling @p ResetImpl().
		 * A reset implies that the derived classes reload all their parameters from #Params and that
		 * all @p LinkedObjectWrapper instances owned by this @p Object instance become invalidated.
		 * @throws Util::NotAvailableException is thrown if this @p Object instance is currently being
		 * used by another @p Object instance.
		*/
		void Reset();

		/**
		 * @brief Blocks this @p Object instance setting Object::IsBlocked to true. Refer to
		 * Object::IsBlocked for the consequences.
		 * @param Timeout Time to wait for locking the mutex of Object::UserList.
		 * @throws Util::NotAvailableException is thrown if this @p Object instance is currently being
		 * used by another @p Object instance.
		*/
		void BlockIfUnused(const std::chrono::milliseconds Timeout = Util::ILockable::DefaultTimeout);
		///@}

		/** @name Thread-safe public functions
		 * Thread-safe since mutex is locked.
		*/
		///@{
		/**
		 * @brief Locks the mutex of the parameter class instance #Params assigned to this @p Object instance
		 * and returns a pointer to the locked #Params.
		 * @param Timeout Time to wait for locking the mutex of #Params.
		 * @return Returns a pointer to const @p ParamsType, since users of this @p Object instance are not allowed
		 * to access non-const members of @p ParamsType. These are only to be accessed by the main thread!
		*/
		ParamsConstTypeSyncPtrType GetParams(const std::chrono::milliseconds Timeout = GetParamsTimeoutDefault) const;

		/**
		 * @brief Invoking an instance of this alias is supposed to call Object::GetParams() of the instance
		 * the Util::CallableMemberWrapper has been constructed with.
		*/
		using ParamsGetterType = Util::CallableMemberWrapper<Object, decltype(&Object::GetParams)>;

		/**
		 * @copybrief GetParams(const std::chrono::milliseconds) const
		 * @param Timeout Time to wait for locking the mutex of #Params.
		 * @return Returns a pointer to @p ParamsType (non-const) to allow access to all of its members.
		*/
		ParamsTypeSyncPtrType GetParams(const std::chrono::milliseconds Timeout = GetParamsTimeoutDefault);

		/**
		 * @brief Returns the name of this @p Object instance.
		 * @param Timeout Time to wait for locking the mutex of #Params.
		 * @return User-defined name of this @p Object instance
		*/
		auto GetObjectName(const std::chrono::milliseconds Timeout = GetParamsTimeoutDefault) const { return GetParams(Timeout)->ObjectName.Get(); }

		/**
		 * @brief Returns whether shared usage has been enabled for this @p Object instance.
		 * Refer to ParamsBase::UsageType.
		 * @param Timeout Time to wait for locking the mutex of #Params.
		 * @return Returns true when shared usage is enabled, false otherwise.
		*/
		bool IsSharedUsageEnabled(const std::chrono::milliseconds Timeout = GetParamsTimeoutDefault) const { return GetParams(Timeout)->Usage == ParamsType::UsageType::Shared; }
		///@}

		/**
		 * @brief Returns the ID of this @p Object instance. Thread-safe since ID is const.
		 * @return ID of this @p Object instance
		*/
		ItemIDType GetID() const noexcept { return Params->GetID(); }

		/** @name Override
		 * Override by derived classes which have to determine these properties.
		 * The @p Object name and category together uniquely define the type of a derived class.
		*/
		///@{
		virtual std::string GetName() const = 0;		//!< Returns the name of this @p Object type.
		virtual std::string GetCategory() const = 0;	//!< Returns the category of this @p Object type.
		///@}

		/**
		 * @copybrief Object::CategoryAndNameToStr
		 * @return Returns a human-readable string to identify this @p Object instance's type.
		*/
		std::string GetCategoryAndName() const { return CategoryAndNameToStr(GetCategory(), GetName()); }

		/**
		 * @brief Ensures that this @p Object instance is ready by possibly starting its worker thread
		 * or by opening connections to hardware devices.
		 * @param IsAutomaticStartup Pass true if ensuring other @p Object instances to be ready have
		 * caused a call to this function. Pass false if an action performed by the user has caused the
		 * call to this function.
		*/
		void EnsureReadyState(bool IsAutomaticStartup);

		/** @name Thread-safe public functions
		 * Thread-safe since mutex is locked.
		*/
		///@{
		/**
		 * @brief Setter for Object::Warning. Sets the warning by a description and an error code.
		 * @param Description String describing the reason and consequences of the warning
		 * @param ErrorCode %DynExp error code from DynExpErrorCodes::DynExpErrorCodes
		*/
		void SetWarning(std::string Description, int ErrorCode) const;

		/**
		 * @brief Setter for Object::Warning. Sets the warning by retrieving the warning data from
		 * an exception @p e.
		 * @param e Exception derived from class Util::Exception
		*/
		void SetWarning(const Util::Exception& e) const;

		/**
		 * @brief Resets Object::Warning.
		*/
		void ClearWarning() const { Warning.Reset(); }

		/**
		 * @brief Returns Object::Warning in a thread-safe way by copying its internal data.
		 * @return Data of type Util::Warning::WarningData stored in Object::Warning.
		*/
		auto GetWarning() const { return Warning.Get(); }

		/**
		 * @brief Returns a pointer to the exception which has caused this @p Object instance to fail.
		 * @param Timeout Time to wait for locking the mutex used to protect the stored exception data.
		 * @return Pointer to the stored exception or a default-constructed std::exception_ptr if no
		 * exception occurred.
		*/
		std::exception_ptr GetException(const std::chrono::milliseconds Timeout = Util::ILockable::DefaultTimeout) const { return GetExceptionChild(Timeout); }

		/**
		 * @brief Returns wheter this @p Object instance is ready (e.g. it is running or connected to
		 * a hardware device) and not blocked (refer to Object::IsBlocked).
		 * @return Returns true if this @p Object instance is ready, false otherwise.
		*/
		bool IsReady() const { return !IsBlocked && IsReadyChild(); }

		/**
		 * @copydoc ObjectUserList::CountUsers
		*/
		auto GetUseCount(const std::chrono::milliseconds Timeout = Util::ILockable::DefaultTimeout) const { return UserList.CountUsers(Timeout); }

		/**
		 * @brief Returns whether this @p Object instance is used by other instances.
		 * @param Timeout Time to wait for locking the mutex of Object::UserList.
		 * @return Returns true if Object::GetUseCount evaluates to zero, false otherwise.
		*/
		bool IsUnused(const std::chrono::milliseconds Timeout = Util::ILockable::DefaultTimeout) const { return GetUseCount(Timeout) == 0; }

		/**
		 * @copydoc ObjectUserList::GetUserIDs
		*/
		auto GetUserIDs(const std::chrono::milliseconds Timeout = Util::ILockable::DefaultTimeout) const { return UserList.GetUserIDs(Timeout); }

		/**
		 * @copydoc ObjectUserList::GetUserNamesString
		*/
		auto GetUserNamesString(const std::chrono::milliseconds Timeout = Util::ILockable::DefaultTimeout) const { return UserList.GetUserNamesString(Timeout); }
		///@}

		/**
		 * @brief Checks whether @p Object instances this instance uses are in a ready state.
		 * Override @p CheckLinkedObjectStatesChild() to implement this behavior.
		*/
		void CheckLinkedObjectStates() const { CheckLinkedObjectStatesChild(); };

		LinkedObjectWrapperOnlyType LinkedObjectWrapperOnly;												//!< @copydoc LinkedObjectWrapperOnlyType

	protected:
		/**
		 * @brief Asserts that the call to this function is performed from the thread which constructed
		 * this @p Object instance (the thread with the id stored in Object::OwnerThreadID).
		 * @throws Util::InvalidCallException is thrown if the assertion fails.
		*/
		void EnsureCallFromOwningThread() const;

		/**
		 * @brief Allows derived @p Objects to edit their own parameters - even in const
		 * task functions (for instruments) or event functions (for modules).
		 * @param Timeout Time to wait for locking the mutex of #Params.
		 * @return Returns a pointer to @p ParamsType (non-const) to allow access all of its members.
		*/
		ParamsTypeSyncPtrType GetNonConstParams(const std::chrono::milliseconds Timeout = GetParamsTimeoutDefault) const;

		/** @name Non-const methods
		 * Not const, to avoid usage by e.g. tasks of derived instruments.
		*/
		///@{
		auto LockUserList(const std::chrono::milliseconds Timeout = Util::ILockable::DefaultTimeout) { return UserList.AcquireLock(Timeout); }	//!< @copydoc ObjectUserList::AcquireLock
		void DeregisterAllUnsafe() { UserList.DeregisterAllUnsafe(); }																			//!< @copydoc ObjectUserList::DeregisterAllUnsafe
		auto GetUseCountUnsafe() { return UserList.CountUsersUnsafe(); }																		//!< @copydoc ObjectUserList::CountUsersUnsafe
		auto GetUserNamesStringUnsafe() const { return UserList.GetUserNamesStringUnsafe(); }													//!< @copydoc ObjectUserList::GetUserNamesStringUnsafe

		/**
		 * @brief Returns whether this @p Object instance is used by other instances (not thread-safe).
		 * @return Returns true if Object::GetUseCountUnsafe evaluates to zero, false otherwise.
		*/
		bool IsUnusedUnsafe() { return GetUseCountUnsafe() == 0; }
		///@}

	private:
		/** @name Override
		 * Override by derived class to make public versions of these functions behave as described above.
		*/
		///@{
		/**
		 * @brief Refer to DynExp::Object::Reset(). Using tag dispatch mechanism to ensure that @p ResetImpl() of every
		 * derived class gets called - starting from DynExp::Object, descending the inheritance hierarchy.
		*/
		virtual void ResetImpl(dispatch_tag<Object>) = 0;													

		virtual std::exception_ptr GetExceptionChild(const std::chrono::milliseconds Timeout) const = 0;	//!< @copydoc Object::GetException
		virtual void EnsureReadyStateChild(bool IsAutomaticStartup) = 0;									//!< @copydoc Object::EnsureReadyState
		virtual bool IsReadyChild() const = 0;																//!< @copydoc IsReady

		/**
		 * @brief Override to implement a check whether linked objects are in a ready state.
		*/
		virtual void CheckLinkedObjectStatesChild() const {};
		///@}

		/**
		 * @brief Writes Object::Warning to the event log returned by Util::EventLog().
		*/
		void LogWarning() const;

		const std::thread::id OwnerThreadID;	//!< Thread id of the thread which has constructed (and owns) this @p Object instance
		const ParamsBasePtrType Params;			//!< Pointer to the parameter class instance belonging to this @p Object instance

		mutable Util::Warning Warning;			//!< Last warning which occurred within this @p Object instance. (Logical const-ness: see above.)

		/**
		 * @brief List of @p Object instances making use of this @p Object instance.
		 * Other @p Object instances making use of this @p Object instance have to register and deregister
		 * themselves (via class @p LinkedObjectWrapper).
		*/
		ObjectUserList UserList;

		/**
		 * @brief This flag indicates whether this @p Object instance is blocked in order to be destroyed
		 * subsequently. Blocked objects are never ready (Object::IsReady() returns always false for them).
		 * Thus, @p RunnableInstance cannot lock this object instance anymore. Unblocking is not possible
		 * since blocking indicates approaching destruction.
		*/
		bool IsBlocked = false;
	};

	/**
	 * @brief Casts an @p Object to a derived type.
	 * @tparam To Type derived from @p Object to cast to. @p const is added to @p To if @p From is @p const.
	 * @tparam From Type derived from @p Object to cast from. This type is automatically deduced.
	 * @param Obj Pointer to the @p Object instance to cast
	 * @return Returns a pointer to the cast @p Object instance.
	*/
	template <typename To, typename From, std::enable_if_t<
		std::is_base_of_v<Object, To> && std::is_base_of_v<Object, From>, int> = 0
	>
	auto dynamic_Object_cast(From* Obj)
	{
		if (!Obj)
			throw Util::InvalidArgException("Obj must not be nullptr.");

		auto DerivedObj = dynamic_cast<std::conditional_t<std::is_const_v<From>, std::add_const_t<To>*, To*>>(Obj);
		if (!DerivedObj)
			throw Util::TypeErrorException();

		return DerivedObj;
	}

	/**
	 * @brief Parameter class for @p RunnableObject
	*/
	class RunnableObjectParams : public ParamsBase
	{
	public:
		/**
		 * @brief Determines when a @p RunnableObject instance is started.
		*/
		enum StartupType { OnCreation, Automatic, Manual };
		/**
		 * @var RunnableObjectParams::StartupType RunnableObjectParams::OnCreation
		 * The runnable instance is started as soon as it is created or when the entire project is started.
		*/
		/**
		 * @var RunnableObjectParams::StartupType RunnableObjectParams::Automatic
		 * The runnable instance is started as soon as it is needed by another @p Object instance to become ready.
		*/
		/**
		 * @var RunnableObjectParams::StartupType RunnableObjectParams::Manual
		 * The runnable instance is started only manually by the user.
		*/

		/**
		 * @copydoc ParamsBase::ParamsBase
		*/
		RunnableObjectParams(ItemIDType ID, const DynExpCore& Core) : ParamsBase(ID, Core) {}
		
		virtual ~RunnableObjectParams() = 0;

		virtual const char* GetParamClassTag() const noexcept override { return "RunnableObjectParams"; }

		/**
		 * @brief Maps description strings to the RunnableObjectParams::StartupType enum's items
		 * @return List containing the description-value mapping
		*/
		static Util::TextValueListType<StartupType> AvlblStartupTypeStrList();

		/**
		 * @brief Determines when the runnable object is started. Refer to RunnableObjectParams::StartupType. 
		*/
		Param<StartupType> Startup = { *this, AvlblStartupTypeStrList(), "Startup", "Startup type",
			"Determines when the item is started", false, StartupType::Automatic };

		/**
		 * @brief Determines whether the @p Startup parameter should be user-configurable in settings dialogs.
		 * Override @p ConfigureStartupTypeChild() in order to adjust.
		 * @return Returns true if the @p Startup parameter should be included in settings dialogs, false otherwise.
		*/
		bool ConfigureStartupType() const noexcept { return ConfigureStartupTypeChild(); }

	private:
		void ConfigureParamsImpl(dispatch_tag<ParamsBase>) override final;
		virtual void ConfigureParamsImpl(dispatch_tag<RunnableObjectParams>) {}		//!< @copydoc DynExp::ParamsBase::ConfigureParamsImpl

		/** @name Override
		 * Override by derived class to make public versions of these functions behave as described above.
		*/
		///@{
		virtual bool ConfigureStartupTypeChild() const noexcept { return true; }	//!< @copydoc ConfigureStartupType
		///@}
	};

	/**
	 * @brief Configurator class for @p RunnableObject
	*/
	class RunnableObjectConfigurator : public ConfiguratorBase
	{
	public:
		using ObjectType = RunnableObject;
		using ParamsType = RunnableObjectParams;

		RunnableObjectConfigurator() = default;
		virtual ~RunnableObjectConfigurator() = 0;
	};

	/**
	 * @brief Defines an @p Object which possesses a thread it runs in. The @p RunnableObject can be started
	 * and stopped as well as paused and resumed.
	*/
	class RunnableObject : public Util::INonCopyable, public Object
	{
		/**
		 * @brief Allow exclusive access to some of @p RunnableObject's private methods to class @p RunnableInstance.
		*/
		class RunnableInstanceOnlyType
		{
			friend class RunnableObject;
			friend class RunnableInstance;

			/**
			 * @brief Construcs an instance - one for each @p RunnableObject instance
			 * @param Parent Owning @p RunnableObject instance
			*/
			constexpr RunnableInstanceOnlyType(RunnableObject& Parent) noexcept : Parent(Parent) {}

			void OnThreadHasExited() const noexcept { Parent.OnThreadHasExited(); }										//!< @copydoc RunnableObject::OnThreadHasExited
			bool IsLinkedObjStateCheckRequested() const noexcept { return Parent.LinkedObjStateCheckRequested; }		//!< @copydoc RunnableObject::LinkedObjStateCheckRequested
			void ResetLinkedObjStateCheckRequested() const noexcept { Parent.LinkedObjStateCheckRequested = false; }	//!< Sets RunnableObject::LinkedObjStateCheckRequested to false.

			RunnableObject& Parent;																						//!< Owning @p RunnableObject instance
		};

	public:
		/**
		 * @brief Exception type thrown by TerminateImpl() if the @p RunnableObject cannot be terminated
		 * for being used by another @p Object instance.
		*/
		class NotUnusedException
		{
		public:
			/**
			 * @brief Constructs an exception instance. Initializes @p UserNames with the content of
			 * ObjectUserList::GetUserNamesStringUnsafe() invoked on @p Parent.
			 * @param Parent @p RunnableObject instance which constructs this exception
			*/
			NotUnusedException(const RunnableObject& Parent) : UserNames(Parent.GetUserNamesStringUnsafe()) {}

			/**
			 * @brief Getter for @p UserNames
			 * @return Returns @p UserNames
			*/
			std::string_view GetUserNames() const { return UserNames; }

			/**
			 * @brief Genereates a user-readable error message containing the content of @p UserNames.
			 * @return Error message describing which @p Object instances make use of the throwing @p RunnableObject instance.
			*/
			std::string GetErrorMessage() const;

		private:
			/**
			 * @brief String with identifiers of the @p Object instances making use of the throwing @p RunnableObject instance.
			*/
			std::string UserNames;	
		};

		using ParamsType = RunnableObjectParams;											//!< @copydoc Object::ParamsType
		using ConfigType = RunnableObjectConfigurator;										//!< @copydoc Object::ConfigType

		/**
		 * @brief Default timeout e.g. used as a default for calls to InstrumentBase::GetInstrumentData or
		 * ModuleBase::GetModuleData.
		*/
		static constexpr auto ShortTimeoutDefault = std::chrono::milliseconds(50);

		/**
		 * @brief Default timeout used as a default for calls to RunnableObject::Terminate.
		*/
		static constexpr auto TerminateTimeoutDefault = std::chrono::milliseconds(3000);

		RunnableObject(const std::thread::id OwnerThreadID, ParamsBasePtrType&& Params);	//!< @copydoc Object::Object
		virtual ~RunnableObject() = 0;

		/** @name Not thread-safe
		 * These functions must be called by the thread owning this @p Object instance only!
		*/
		///@{
		/**
		 * @brief Starts the @p RunnableObject instance's thread and ensures that all @p Object instances linked to this
		 * instance via @p ObjectLink parameters are in a ready state. Calls @p RunChild() which is to be overridden
		 * by derived classes to start a thread and to pass this thread back by calling @p StoreThread().
		 * @param ParentWidget Qt QWidget acting as a parent of the modal busy dialog (@p BusyDialog) to be displayed
		 * while starting the linked @p Object instances.
		 * @return Returns true if the @p RunnableObject instance has been started now, false otherwise
		 * (e.g. if it is already running or if the startup policy (@p RunnableObjectParams::Startup) prohibits so).
		 * @throws Util::InvalidStateException is thrown if the @p RunnableObject instance is in an error state.
		 * Call Object::Reset() to resolve this.
		*/
		bool Run(QWidget* ParentWidget = nullptr);

		/**
		 * @brief Calls @p Run() if RunnableObjectParams::Startup is set to RunnableObjectParams::Automatic.
		 * @return Returns true if the @p RunnableObject instance has been started now, false otherwise
		 * (e.g. if it is already running or if the startup policy (@p RunnableObjectParams::Startup) prohibits so).
		 * @throws Util::NotAvailableException is thrown if the @p RunnableObject instance is not already running and
		 * if RunnableObjectParams::Startup is not set to RunnableObjectParams::Automatic.
		*/
		bool RunIfRunAutomatic();

		/**
		 * @brief Calls @p Run() if RunnableObjectParams::Startup is set to RunnableObjectParams::OnCreation.
		 * @return Returns true if the @p RunnableObject instance has been started now, false otherwise
		 * (e.g. if it is already running or if the startup policy (@p RunnableObjectParams::Startup) prohibits so).
		*/
		bool RunIfRunOnCreation();

		/**
		 * @brief Notifies the @p RunnableObject instance's thread to terminate and waits until it has ended.
		 * Calls TerminateImpl().
		 * @param Force If true, the @p RunnableObject is terminated even if it is still used.
		 * @param Timeout Time to wait until the @p RunnableObject's thread has ended.
		*/
		void Terminate(bool Force = false, const std::chrono::milliseconds Timeout = TerminateTimeoutDefault);
		///@}

		/**
		 * @brief Pauses or resumes a @p RunnableObject instance. Its thread stays running, but the instance does
		 * not perform any action while it is paused.
		 * @param Pause Pass true to pause the @p RunnableObject instance or false to resume it.
		 * If @p Pause is false, @p ClearReasonWhyPaused() is called.
		 * @param Description If @p Pause is true, @p Description is passed to @p SetReasonWhyPaused().
		*/
		void SetPaused(bool Pause, std::string Description = "");

		bool IsRunning() const noexcept { return Running; }					//!< Returns #Running.
		bool IsPaused() const noexcept { return Paused; }					//!< Returns #Paused.
		bool IsExiting() const noexcept { return ShouldExit; }				//!< Returns #ShouldExit.

		auto GetStartupType() const noexcept { return Startup.load(); }		//!< Returns #Startup.
		auto GetReasonWhyPaused() const { return ReasonWhyPaused.Get(); }	//!< Returns #ReasonWhyPaused.

		RunnableInstanceOnlyType RunnableInstanceOnly;						//!< @copydoc RunnableInstanceOnlyType

	protected:
		void Init();														//!< Initializes member variables in case of a reset.

		/**
		 * @brief Helper function to be used by overridden @p RunChild() functions in derived classes to
		 * (re)initialize the #ThreadExitedSignal future and to provide the derived class a promise
		 * to signal the @p RunnableObject instance that its thread has terminated.
		 * @return Promise bound to the #ThreadExitedSignal future
		*/
		std::promise<void> MakeThreadExitedPromise();
		
		/**
		 * @brief Stores a thread constructed by a derived class overriding @p RunChild() in @p Thread
		 * taking ownership of the thread. Only call this function within @p RunChild()!
		 * @param Thread Thread to store. 
		*/
		void StoreThread(std::thread&& Thread) noexcept;

		/**
		 * @brief Checks whether @p Thread's id matches the id of the calling thread.
		 * This is thread-safe if the function is called by the @p RunnableObject instance's thread
		 * since @p Terminate() joins the threads before changing the @p Thread member.
		 * It is also thread-safe if the function is called by the thread owning the the @p RunnableObject
		 * instance since @p Run() and @p Terminate() can only be called by this thread.
		 * Only @p Run() and @p Terminate() (indirectly) modify @p Thread.
		 * @return Returns true of both ids match, false otherwise.
		*/
		bool IsCallFromRunnableThread() const;

		/**
		 * @brief Asserts that the call to this function is performed from the @p RunnableObject instance's thread
		 * by calling IsCallFromRunnableThread().
		 * @throws Util::InvalidCallException is thrown if the assertion fails.
		*/
		void EnsureCallFromRunnableThread() const;

		/**
		 * @brief Sets the reason why this @p RunnableObject instance has been paused.
		 * @param Description Human-readable string describing the reason
		*/
		void SetReasonWhyPaused(std::string Description) { ReasonWhyPaused = Util::Warning(std::move(Description)); }

		/**
		 * @copybrief SetReasonWhyPaused(std::string)
		 * @param e Exception containing the reason (including a human-readable description)
		*/
		void SetReasonWhyPaused(const Util::Exception& e) { ReasonWhyPaused = e; }

		/**
		 * @brief Removes the reason why this @p RunnableObject instance has been paused (since it is resumed).
		*/
		void ClearReasonWhyPaused() { ReasonWhyPaused.Reset(); }

	private:
		void ResetImpl(dispatch_tag<Object>) override final;
		virtual void ResetImpl(dispatch_tag<RunnableObject>) = 0;			//!< @copydoc ResetImpl(dispatch_tag<DynExp::Object>)

		void EnsureReadyStateChild(bool IsAutomaticStartup) override final;
		void CheckLinkedObjectStatesChild() const override final { LinkedObjStateCheckRequested = true; }

		/** @name Override
		 * Override by derived classes.
		*/
		///@{
		virtual void RunChild() = 0;										//!< Refer to @p Run().

		/**
		 * @brief Notify derived classes that some state has changed (e.g. the termination of
		 * @p Thread is requested) and that the child's event/task queue should run now.
		*/
		virtual void NotifyChild() {}

		/**
		 * @brief Signals derived classes that terminating the RunnableObject instance's thread is
		 * about to be requested. Derived classes might now enqueue respective exit tasks/events
		 * into their task/event queues.
		 * Refer to @p TerminateUnsafe().
		 * @param Timeout Time to wait until the @p RunnableObject's thread has ended.
		*/
		virtual void TerminateChild(const std::chrono::milliseconds Timeout) {}

		/**
		 * @brief Override to make this function return a pointer to a @p BusyDialog instance.
		 * Refer to @p Run().
		 * @param ParentWidget Qt QWidget acting as a parent of the modal busy dialog
		 * @return Pointer to a @p BusyDialog instance
		*/
		virtual std::unique_ptr<BusyDialog> MakeStartupBusyDialogChild(QWidget* ParentWidget) const { return nullptr; }
		///@}
		
		/**
		 * @copydoc Terminate()
		 * @brief Bypasses the assertion check provided by Object::EnsureCallFromOwningThread().
		 * Ensures thread-safety and calls @p TerminateUnsafe().
		 * @throws NotUnusedException is thrown in case @p Force is false and if this @p RunnableObject
		 * instance is used by other @p Object instances.
		*/
		void TerminateImpl(bool Force, const std::chrono::milliseconds Timeout = TerminateTimeoutDefault);

		/**
		 * @copydoc Terminate()
		 * @brief Joins this @p RunnableObject instance's thread. Does nothing if @p Thread is not joinable.
		 * Not thread-safe. Called by @p TerminateImpl(). Calls @p TerminateChild() before any other action.
		 * @throws Util::ThreadDidNotRespondException is thrown if the thread does not terminate within the
		 * @p TerminateTimeoutDefault timeout after setting @p ShouldExit to true and calling @p NotifyChild().
		*/
		void TerminateUnsafe(bool Force, const std::chrono::milliseconds Timeout = TerminateTimeoutDefault);

		/**
		 * @brief This function is called when the @p RunnableObject instance's thread terminates.
		 * The thread receives a @p RunnableInstance instance as a parameter. When it exits, this
		 * @p RunnableInstance instance is destroyed. So, @p RunnableInstance's destructor is
		 * invoked. It calls RunnableInstance::SetThreadExited(), which in turn calls this function,
		 * and signals the thread termination to #ThreadExitedSignal. 
		*/
		void OnThreadHasExited() noexcept;

		/**
		 * @brief Reflects the value of @p RunnableObjectParams::Startup. This variable is only updated
		 * when @p Run() is called. It is atomic to allow the UI thread reading it safely.
		*/
		std::atomic<RunnableObjectParams::StartupType> Startup = RunnableObjectParams::StartupType::Automatic;

		std::thread Thread;													//!< The @p RunnableObject instance's thread
		std::future<void> ThreadExitedSignal;								//!< Future which signals that @p Thread has terminated. Refer to @p OnThreadHasExited().
		std::atomic<bool> Running;											//!< Indicates whether the @p RunnableObject instance is running.
		std::atomic<bool> Paused;											//!< Indicates whether the @p RunnableObject instance is paused.
		Util::Warning ReasonWhyPaused;										//!< Holds information about why the @p RunnableObject instance is paused.

		/** @name Inter-thread communication
		 * These variables are for communication from the thread owning this @p RunnableObject instance
		 * to its thread @p Thread only.
		*/
		///@{
		std::atomic<bool> ShouldExit;										//!< Indicates whether this @p RunnableObject instance's thread should terminate.
		///@}

		/**
		 * @brief Indicates whether the @p RunnableInstance instance belonging to this @p RunnableObject
		 * instance's thread should recheck the states of all @p Object instances this @p RunnableObject uses.
		*/
		mutable std::atomic<bool> LinkedObjStateCheckRequested = false;
	};

	/**
	 * @brief Polymorphic base class to allow storing @p LinkedObjectWrapper of any type
	 * in a single list.
	*/
	class LinkedObjectWrapperBase : public Util::INonCopyable
	{
		friend class RunnableInstance;

	private:
		/**
		 * @brief Alias for a smart pointer owning instances of this class.
		*/
		using LinkedObjectWrapperBasePtrType = std::unique_ptr<LinkedObjectWrapperBase>;
		
		/**
		 * @brief Helper type linking a @p LinkedObjectWrapperBasePtrType to the corresponding
		 * @p LinkedObjectWrapperContainerBase instance.
		*/
		struct OwnedLinkedObjectWrapperType
		{
			/**
			 * @brief Constructs a OwnedLinkedObjectWrapperType instance.
			 * @param OwnedLinkedObjectWrapperPtr Refer to #OwnedLinkedObjectWrapperPtr.
			 * @param OwnedLinkedObjectWrapperContainer Refer to #OwnedLinkedObjectWrapperContainer.
			*/
			OwnedLinkedObjectWrapperType(LinkedObjectWrapperBasePtrType&& OwnedLinkedObjectWrapperPtr,
				LinkedObjectWrapperContainerBase& OwnedLinkedObjectWrapperContainer)
				: OwnedLinkedObjectWrapperPtr(std::move(OwnedLinkedObjectWrapperPtr)),
				OwnedLinkedObjectWrapperContainer(OwnedLinkedObjectWrapperContainer) {}

			/**
			 * @brief Pointer owning an instance of @p LinkedObjectWrapperBase
			*/
			LinkedObjectWrapperBasePtrType OwnedLinkedObjectWrapperPtr;

			/**
			 * @brief Reference to a @p LinkedObjectWrapperContainerBase instance holding a non-owning
			 * pointer to the instance of @p LinkedObjectWrapperBase #OwnedLinkedObjectWrapperPtr refers to
			*/
			LinkedObjectWrapperContainerBase& OwnedLinkedObjectWrapperContainer;
		};

		/**
		 * @brief Type of a list hold by @p RunnableInstance (RunnableInstance::OwnedLinkedObjectWrappers)
		 * that contains pointers to all @p LinkedObjectWrapperBase instances and their corresponding containers
		 * the @p RunnableInstance instance owns.
		*/
		using ListType = std::list<OwnedLinkedObjectWrapperType>;

	public:
		/**
		 * @brief Constructs a LinkedObjectWrapperBase instance.
		 * @param Owner Refer to #Owner.
		*/
		LinkedObjectWrapperBase(const RunnableInstance& Owner) : Owner(Owner) {}

		virtual ~LinkedObjectWrapperBase() = 0;

		/** @name Override
		 * Override by derived class.
		*/
		///@{
		/**
		 * @brief Returns whether the wrapper has registered its owning @p Object instance (through
		 * #Owner) as a user of the target resource (refer to LinkedObjectWrapper::DestinyResource).
		 * @return Returns true if already registered, false otherwise.
		*/
		virtual bool IsRegistered() const noexcept = 0;
		///@}

		/**
		 * @brief Returns the owner of this wrapper.
		 * @return Instance of @p Object which acts as a user of the wrapped @p Object instance
		*/
		const Object& GetOwner() const noexcept;

	private:
		/** @name Override (methods for class RunnableInstance)
		 * Override by derived class. Accessed directly by class @p RunnableInstance.
		*/
		///@{
		/**
		 * @brief Registers the wrapper's owning @p Object instance (through #Owner) as a user
		 * of the target resource (refer to LinkedObjectWrapper::DestinyResource).
		 * @param Timeout Timeout of the mutex-locking operation in @p ObjectUserList
		*/
		virtual void Register(const std::chrono::milliseconds Timeout) = 0;

		/**
		 * @brief Deregisters the wrapper's owning @p Object instance (through #Owner) as a user
		 * of the target resource (refer to LinkedObjectWrapper::DestinyResource).
		 * @param Timeout Timeout of the mutex-locking operation in @p ObjectUserList
		*/
		virtual void Deregister(const std::chrono::milliseconds Timeout) = 0;

		/**
		 * @brief Position of this wrapper instance in the list of @p LinkedObjectWrapperBase instances kept
		 * by the owning @p RunnableInstance instance #Owner (refer to RunnableInstance::OwnedLinkedObjectWrappers).
		 * For std::list, iterators are not invalidaded upon insertion or erasure (for non-affected entries).
		*/
		ListType::const_iterator ListPos;
		///@}

		/**
		 * @brief Instance of class @p RunnableInstance managing the wrapper
		*/
		const RunnableInstance& Owner;
	};

	/**
	 * @brief Holds a shared_ptr to a resource (instance of class @p Object) and lets the resource keep
	 * track of its usage count by increasing a respective counter of the resource on construction
	 * and by decreasing the counter on destruction of the @p LinkedObjectWrapper instance again.
	 * Not copyable in order not to mess up the usage counter stored in LinkedObjectWrapper::DestinyResource.
	 * Only accessed via a thread's instance of class @p RunnableInstance in that thread.
	 * Also refer to @p RunnableInstance.
	 * @tparam ObjectT Type of the managed @p Object.
	*/
	template <typename ObjectT>
	class LinkedObjectWrapper : public LinkedObjectWrapperBase
	{
	public:
		using ObjectType = std::add_const_t<ObjectT>;	//!< Const type of the managed @p Object.

		/**
		 * @brief Constructs a @p LinkedObjectWrapper instance and calls LinkedObjectWrapper::Register().
		 * @param Owner Refer to LinkedObjectWrapperBase::Owner.
		 * @param DestinyResource Refer to #DestinyResource.
		 * @param Timeout Timeout of the mutex-locking operation in @p ObjectUserList
		*/
		LinkedObjectWrapper(const RunnableInstance& Owner, std::shared_ptr<ObjectType>&& DestinyResource, const std::chrono::milliseconds Timeout)
			: LinkedObjectWrapperBase(Owner),
			DestinyResource(std::move(DestinyResource)), IsRegisteredFlag(false)
		{
			// Checks for nullptr.
			LinkedObjectWrapper::Register(Timeout);
		}

		/**
		 * @brief Destructor calls LinkedObjectWrapper::Deregister().
		*/
		virtual ~LinkedObjectWrapper()
		{
			try
			{
				LinkedObjectWrapper::Deregister(std::chrono::milliseconds(1000));
			}
			catch (const Util::TimeoutException& e)
			{
				Util::EventLog().Log(e);
				Util::EventLog().Log("Could not deregister a linked object wrapper. Timeout occurred. Execution cannot continue.",
					Util::ErrorType::Fatal);

				std::terminate();
			}
			catch (const Util::Exception& e)
			{
				Util::EventLog().Log("Could not deregister a linked object wrapper. The error listed below occurred. Execution cannot continue.",
					Util::ErrorType::Fatal);
				Util::EventLog().Log(e);

				std::terminate();
			}
			catch (const std::exception& e)
			{
				Util::EventLog().Log("Could not deregister a linked object wrapper. The error listed below occurred. Execution cannot continue.",
					Util::ErrorType::Fatal);
				Util::EventLog().Log(e.what());

				std::terminate();
			}
			catch (...)
			{
				Util::EventLog().Log("Could not deregister a linked object wrapper. An unknown error occurred. Execution cannot continue.",
					Util::ErrorType::Fatal);

				std::terminate();
			}
		}

		virtual bool IsRegistered() const noexcept override { return IsRegisteredFlag; }

		/**
		 * @brief Builds and returns a human-readable string uniquely identifying the
		 * @p Object instance #DestinyResource. The function does not perform a check whether
		 * the owning @p Object instance has been registered (i.e. #IsRegisteredFlag is true).
		 * @return String containing the type category, type name, and user-defined
		 * name identifying #DestinyResource.
		*/
		std::string GetLinkedObjectDesc() const { return DestinyResource->GetObjectName() + " (" + DestinyResource->GetCategoryAndName() + ")"; }

		/**
		 * @brief Getter for the target resource which is used by the owning @p Object instance
		 * @return Returns #DestinyResource.
		 * @throws Util::InvalidStateException is thrown if the owning @p Object instance has not been registered
		 * (i.e. #IsRegisteredFlag is false).
		*/
		decltype(auto) get() const { return &this->operator*(); }

		decltype(auto) get() { return &this->operator*(); }					//!< @copydoc get() const

		//!< Dereferencing getter. @copydoc get() const
		const auto& operator*() const
		{
			// ->
			// Checking for errors with
			
			/* if (!DestinyResource->IsReady())
				throw Util::InvalidCallException("The requested object is in an invalid state or in an error state."); */

			// does not work here, because the OnError() handlers of instruments or modules might call
			// functions through a LinkedObjectWrapper to perform shutdown operations. This would not be
			// possible with a respective error check taking place here. This is not considered a problem,
			// since DestinyResource exists in any case and modifying its underlying Object simply does
			// not have an effect or throws an exception itself if Object is not ready. Instead, just
			// indiate errors by setting LinkedObjectWrapperContainerBase's LinkedObjectState to NotReady
			// (see LinkedObjectWrapperContainer<...>::get()).
			// <-

			if (!IsRegisteredFlag)
				throw Util::InvalidStateException(
					"This linked object wrapper instance is deregistered from the requested object.");

			return *DestinyResource.get();
		}

		//!< Dereferencing getter. @copydoc get() const
		auto& operator*()
		{
			// Call const version of operator*
			return const_cast<typename std::shared_ptr<ObjectType>::element_type&>(
				static_cast<const LinkedObjectWrapper<ObjectT>&>(*this).operator*());
		}

		const auto operator->() const { return &this->operator*(); }		//!< @copydoc get() const
		auto operator->() { return &this->operator*(); }					//!< @copydoc get() const

	private:
		virtual void Register(const std::chrono::milliseconds Timeout) override
		{
			if (!DestinyResource)
				throw Util::InvalidArgException("DestinyResource must not be nullptr.");
			if (IsRegisteredFlag)
				return;

			// Could throw Util::TimeoutException if resource is currently modified (e.g. call to Object::Reset()).
			DestinyResource->LinkedObjectWrapperOnly.RegisterUser(GetOwner(), Timeout);

			IsRegisteredFlag = true;
		}

		virtual void Deregister(const std::chrono::milliseconds Timeout) override
		{
			if (!IsRegisteredFlag)
				return;

			DestinyResource->LinkedObjectWrapperOnly.DeregisterUser(GetOwner(), Timeout);

			IsRegisteredFlag = false;
		}

		/**
		 * @brief Target resource which is used by the @p Object instance LinkedObjectWrapperBase::Owner
		 * belongs to.
		*/
		const std::shared_ptr<ObjectType> DestinyResource;

		/**
		 * @brief Indicates whether the wrapper has registered its owning @p Object instance (through
		 * LinkedObjectWrapperBase::Owner) as a user of the target resource #DestinyResource.
		*/
		bool IsRegisteredFlag;
	};

	/**
	 * @brief This class describes a pointer to a @p LinkedObjectWrapper instance. It is a
	 * simple wrapper class which only adds specific behavior like always returning a
	 * const pointer to a @p LinkedObjectWrapper instance wrapping a const @p Object instance.
	 * @tparam ObjectT Type of the managed @p Object.
	*/
	template <typename ObjectT>
	class LinkedObjectWrapperPointer
	{
	public:
		/**
		 * @brief Type of the @p LinkedObjectWrapper instance this pointer points to
		*/
		using LinkedObjectWrapperType = LinkedObjectWrapper<std::add_const_t<ObjectT>>;

		/**
		 * @brief Pointer type of @p LinkedObjectWrapperType
		*/
		using LinkedObjectWrapperPtrType = LinkedObjectWrapperType*;

		/**
		 * @brief Constructs an empty pointer.
		*/
		LinkedObjectWrapperPointer() noexcept : LinkedObjectWrapperPtr(nullptr) {}

		/**
		 * @brief Constructs a pointer pointing to @p LinkedObjectWrapperPtr.  
		 * @param LinkedObjectWrapperPtr Pointer to a @p LinkedObjectWrapper instance to point to
		*/
		LinkedObjectWrapperPointer(LinkedObjectWrapperPtrType LinkedObjectWrapperPtr) noexcept
			: LinkedObjectWrapperPtr(LinkedObjectWrapperPtr) {}

		/**
		 * @brief Always returns a const pointer so that @p Object instances using another linked
		 * @p Object instance are only allowed to call the linked object's const member functions.
		 * @return Returns #LinkedObjectWrapperPtr.
		*/
		LinkedObjectWrapperPtrType get() const noexcept { return LinkedObjectWrapperPtr; }

		/**
		 * @brief Checks whether #LinkedObjectWrapperPtr equals another pointer to a @p LinkedObjectWrapper instance.
		 * @param rhs Pointer to a @p LinkedObjectWrapper instance
		 * @return Returns true if both pointers equal, false otherwise.
		*/
		bool operator==(const LinkedObjectWrapperPtrType rhs) const noexcept { return LinkedObjectWrapperPtr == rhs; }

		/**
		 * @brief Checks whether #LinkedObjectWrapperPtr does not equal another pointer to a @p LinkedObjectWrapper instance.
		 * @param rhs Pointer to a @p LinkedObjectWrapper instance
		 * @return Returns false if both pointers equal, true otherwise.
		*/
		bool operator!=(const LinkedObjectWrapperPtrType rhs) const noexcept { return LinkedObjectWrapperPtr != rhs; }

		/**
		 * @brief Checks whether this @p LinkedObjectWrapperPointer instance equals another instance.
		 * @param rhs Other @p LinkedObjectWrapperPointer instance
		 * @return Returns true if case of equality, false otherwise.
		*/
		bool operator==(const LinkedObjectWrapperPointer& rhs) const noexcept { return LinkedObjectWrapperPtr == rhs.get(); }

		/**
		 * @brief Checks whether this @p LinkedObjectWrapperPointer instance does not equal another instance.
		 * @param rhs Other @p LinkedObjectWrapperPointer instance
		 * @return Returns false if case of equality, true otherwise.
		*/
		bool operator!=(const LinkedObjectWrapperPointer& rhs) const noexcept { return LinkedObjectWrapperPtr != rhs.get(); }

		/**
		 * @brief Evaluates this @p LinkedObjectWrapperPointer instance to true if #LinkedObjectWrapperPtr
		 * is not nullptr and to false otherwise.
		*/
		explicit operator bool() const noexcept { return LinkedObjectWrapperPtr != nullptr; }

		/**
		 * @brief Always returns a const pointer so that @p Object instances using another linked
		 * @p Object instance are only allowed to call the linked object's const member functions.
		 * @return Returns #LinkedObjectWrapperPtr.
		*/
		LinkedObjectWrapperPtrType operator->() const noexcept { return LinkedObjectWrapperPtr; }

		/**
		 * @brief Always returns a const reference so that @p Object instances using another linked
		 * @p Object instance are only allowed to call the linked object's const member functions.
		 * @return Dereferences and returns #LinkedObjectWrapperPtr.
		*/
		LinkedObjectWrapperType& operator*() const noexcept { return *LinkedObjectWrapperPtr; }

	private:
		/**
		 * @brief @p LinkedObjectWrapper instance this pointer points to
		*/
		LinkedObjectWrapperPtrType LinkedObjectWrapperPtr;
	};

	/**
	 * @brief Polymorphic base class to allow storing @p LinkedObjectWrapperContainer of any type
	 * in a single list.
	*/
	class LinkedObjectWrapperContainerBase : public Util::INonCopyable
	{
		friend class RunnableInstance;

	public:
		/**
		 * @brief Indicates the current state of the @p Object referenced by the linked @p LinkedObjectWrapper.
		*/
		enum class LinkedObjectStateType { NotLinked, Ready, NotReady };
		/**
		 * @var LinkedObjectWrapperContainerBase::LinkedObjectStateType LinkedObjectWrapperContainerBase::NotLinked
		 * This container instance is currently not linked to any @p LinkedObjectWrapper.
		*/
		/**
		 * @var LinkedObjectWrapperContainerBase::LinkedObjectStateType LinkedObjectWrapperContainerBase::Ready
		 * The @p Object instance the linked @p LinkedObjectWrapper refers to is in a ready state.
		*/
		/**
		 * @var LinkedObjectWrapperContainerBase::LinkedObjectStateType LinkedObjectWrapperContainerBase::NotReady
		 * The @p Object instance the linked @p LinkedObjectWrapper refers to is not in a ready state
		 * (e.g. since it has been stopped or since it has failed).
		*/

	protected:
		/**
		 * @brief Constructs a LinkedObjectWrapperContainerBase instance which is not linked to any
		 * @p LinkedObjectWrapper.
		*/
		LinkedObjectWrapperContainerBase() noexcept : LinkedObjectState(LinkedObjectStateType::NotLinked) {}

		virtual ~LinkedObjectWrapperContainerBase() = 0;

		/**
		 * @brief Removes any linked @p LinkedObjectWrapper and updates #LinkedObjectState.
		*/
		void Reset() noexcept;

		/**
		 * @brief Stores the current state of this @p LinkedObjectWrapperContainerBase instance.
		 * Refer to @p LinkedObjectStateType.
		 * Mutable to be updated in calls to LinkedObjectWrapperContainer::get().
		*/
		mutable LinkedObjectStateType LinkedObjectState;

	public:
		auto GetState() const noexcept { return LinkedObjectState; }	//!< Returns #LinkedObjectState.

		/**
		 * @brief Builds and returns a human-readable string uniquely identifying the
		 * @p Object instance which is wrapped by the linked @p LinkedObjectWrapper.
		 * @return String containing e.g. the type category, type name, and user-defined
		 * name of the related @p Object instance
		*/
		std::string GetLinkedObjectDesc() const { return GetLinkedObjectDescChild(); }

		/**
		 * @brief Returns whether the @p Object instance which is wrapped by the linked
		 * @p LinkedObjectWrapper is in a ready state.
		 * @return Returns e.g. the result of a call to @p Object::IsReady() on the
		 * related @p Object instance.
		*/
		bool CheckIfReady() { return CheckIfReadyChild(); }

	private:
		/** @name Override
		 * Override by derived class to make public versions of these functions behave as described above.
		*/
		///@{
		virtual void ResetChild() noexcept = 0;							//!< @copydoc Reset
		virtual std::string GetLinkedObjectDescChild() const = 0;		//!< @copydoc GetLinkedObjectDesc
		virtual bool CheckIfReadyChild() = 0;							//!< @copydoc CheckIfReady
		///@}
	};

	/**
	 * @brief This class holds a pointer (@p LinkedObjectWrapperPointer) to a @p LinkedObjectWrapper.
	 * Intances of this class should be owned by classes derived from class @p InstrumentDataBase or
	 * class @p ModuleDataBase. These instances provide access to objects referenced by object link
	 * parameters. @p LinkedObjectWrapperContainer does not own the referenced @p LinkedObjectWrapper.
	 * The @p LinkedObjectWrapper itself is owned by class @p RunnableInstance. The pointer hold here
	 * is also set and managed by class @p RunnableInstance. Also refer to class @p RunnableInstance.
	 * @tparam ObjectT Type of the managed @p Object.
	*/
	template <typename ObjectT>
	class LinkedObjectWrapperContainer : public LinkedObjectWrapperContainerBase
	{
		friend class RunnableInstance;

	public:
		/**
		 * @brief Constructs a LinkedObjectWrapperContainer instance.
		 * @param PerformReadyCheck Refer to LinkedObjectWrapperContainer::PerformReadyCheck.
		*/
		LinkedObjectWrapperContainer(bool PerformReadyCheck = true) noexcept
			: PerformReadyCheck(PerformReadyCheck) {}

		virtual ~LinkedObjectWrapperContainer() = default;

		/** @name Logical const-ness
		 * Const methods to also operate on const @p LinkedObjectWrapperContainer instances.
		*/
		///@{
		/**
		 * @brief Returns the #LinkedObjWrapperPtr and checks (depending on #PerformReadyCheck)
		 * whether the @p Object #LinkedObjWrapperPtr points to is ready.
		 * If this is not the case, this @p LinkedObjectWrapperContainer instance remembers the
		 * linked object's state updating LinkedObjectWrapperContainerBase::LinkedObjectState.
		 * @return The returned pointer/reference is always const.
		 * Refer to @p LinkedObjectWrapperPointer.
		*/
		auto get() const
		{
			const auto Destiny = GetLinkedObjWrapperPtr()->get();

			if (PerformReadyCheck)
			{
				try
				{
					// This flag is reset either by SetLinkedObjWrapperPtr() or by
					// RunnableInstance::CareAboutWrappers().
					if (!Destiny->IsReady())
						LinkedObjectState = LinkedObjectStateType::NotReady;
				}
				// ForwardedException signals that the underlying exception arose in the destiny resource.
				// All other exceptions are not to be handled here.
				catch ([[maybe_unused]] const Util::ForwardedException& e)
				{
					LinkedObjectState = LinkedObjectStateType::NotReady;
				}
			}

			return Destiny;
		}

		auto& operator*() const { return *get(); }	//!< Dereferences and returns the result of a call to @p get().
		auto operator->() const { return get(); }	//!< Returns the result of a call to @p get().

		/**
		 * @brief Checks whether #LinkedObjWrapperPtr points to a valid distination.
		 * @return Returns true if #LinkedObjWrapperPtr is not nullptr, false otherwise.
		*/
		bool valid() const noexcept { return static_cast<bool>(LinkedObjWrapperPtr); }
		///@}

	private:
		virtual void ResetChild() noexcept override { LinkedObjWrapperPtr = nullptr; }

		virtual std::string GetLinkedObjectDescChild() const override
		{
			if (!LinkedObjWrapperPtr)
				return "< unknown >";

			return GetLinkedObjWrapperPtr()->GetLinkedObjectDesc();
		}

		virtual bool CheckIfReadyChild() override
		{
			if (LinkedObjectState != LinkedObjectStateType::Ready)
				return false;

			const auto Destiny = GetLinkedObjWrapperPtr()->get();

			bool IsReady = Destiny->IsReady();
			if (!IsReady)
				LinkedObjectState = LinkedObjectStateType::NotReady;

			return IsReady;
		}

		/**
		 * @brief Checks whether #LinkedObjWrapperPtr is nullptr. Does nothing if this is not the case.
		 * @throws Util::LinkedObjectNotLockedException is thrown if #LinkedObjWrapperPtr is nullptr.
		*/
		void CheckIfNullptr() const
		{
			if (!LinkedObjWrapperPtr)
				throw Util::LinkedObjectNotLockedException();
		}

		/**
		 * @brief Returns #LinkedObjWrapperPtr after checking whether #LinkedObjWrapperPtr is nullptr.
		 * @return Refer to #LinkedObjWrapperPtr.
		*/
		auto& GetLinkedObjWrapperPtr() const
		{
			CheckIfNullptr();

			return LinkedObjWrapperPtr;
		}

		/**
		 * @brief Sets #LinkedObjWrapperPtr to a new destination and updates
		 * LinkedObjectWrapperContainerBase::LinkedObjectState.
		 * @param NewLinkedObjWrapperPtr Pointer to a @p LinkedObjectWrapper as a new destination
		*/
		void SetLinkedObjWrapperPtr(LinkedObjectWrapperPointer<ObjectT> NewLinkedObjWrapperPtr) noexcept
		{
			LinkedObjWrapperPtr = NewLinkedObjWrapperPtr;
			LinkedObjectState = LinkedObjWrapperPtr ? LinkedObjectStateType::Ready : LinkedObjectStateType::NotLinked;

			CheckIfReady();
		}

		/**
		 * @brief Pointer to a @p LinkedObjectWrapper instance holding a pointer to the destiny
		 * object instance of type const @p ObjectT.
		*/
		LinkedObjectWrapperPointer<ObjectT> LinkedObjWrapperPtr;

		/**
		 * @brief Determines whether a call to @p get() checks the return value of
		 * Object::IsReady() before returning #LinkedObjWrapperPtr.
		*/
		const bool PerformReadyCheck;
	};

	/**
	 * @brief This class defines a list of @p LinkedObjectWrapperContainer instances. The list owns the
	 * contained @p LinkedObjectWrapperContainer instances. Intances of this class should be owned by
	 * classes derived from class @p InstrumentDataBase or class @p ModuleDataBase. These instances
	 * provide access to objects referenced by object link list parameters (@p ListParam).
	 * The list's entries are exclusively managed by the respective instance of class
	 * @p RunnableInstance. @p RunnableInstance stores references to list entries. Every list
	 * manipulation @p RunnableInstance is not aware of might cause dangling references!
	 * @tparam ObjectT Type of the managed @p Object.
	*/
	template <typename ObjectT>
	class LinkedObjectWrapperContainerList
	{
		friend class RunnableInstance;

		/**
		 * @brief Type of the owned @p LinkedObjectWrapperContainer instances
		*/
		using ContainerType = LinkedObjectWrapperContainer<ObjectT>;

	public:
		LinkedObjectWrapperContainerList() = default;
		~LinkedObjectWrapperContainerList() = default;

		const auto& GetList() const noexcept { return Containers; }			//!< Returns #Containers.
		const auto& GetLabels() const noexcept { return ObjectLabels; }		//!< Returns #ObjectLabels.
		std::string_view GetIconPath() const { return IconPath; }			//!< Returns #IconPath.

		/**
		 * @brief Returns a stored @p LinkedObjectWrapperContainer instance selected by its list index.
		 * @tparam IndexType Type of @p Index. Needs to be castable to @p size_t
		 * @param Index Index of the entry in #Containers to return
		 * @return Returns the element of #Containers at the position @p Index.
		 * @throws Util::OutOfRangeException is thrown if @p Index exceeds the amount of list entries in #Containers.
		*/
		template <typename IndexType>
		auto& operator[](IndexType Index)
		{
			auto i = Util::NumToT<size_t>(Index);

			if (i >= Containers.size())
				throw Util::OutOfRangeException("The specified object index is higher than the amount of linked objects.");

			return *Containers[i];
		}

	private:
		/**
		 * @brief List of the owned @p LinkedObjectWrapperContainer instances. @p std::vector of pointers
		 * since insertion and erasure do invalidate references to elements.
		*/
		std::vector<std::unique_ptr<ContainerType>> Containers;

		Util::TextListType ObjectLabels;	//!< Holds a human-readable identifier for each linked object.
		std::string IconPath;				//!< Holds the path to the icon of the linked objects' base type (e.g. hardware adapter, instrument).
	};

	/**
	 * @brief Base class for object link parameter types to allow SFINAE in ParamsBase::Param
	 * for @p ObjectLink of any type
	*/
	class ObjectLinkBase
	{
	public:
		/**
		 * @brief Default timeout used by classes @p ObjectLinkt and @p RunnableInstance to
		 * be passed to LinkedObjectWrapper::LinkedObjectWrapper.
		*/
		static constexpr std::chrono::milliseconds LockObjectTimeoutDefault = std::chrono::milliseconds(100);

	protected:
		ObjectLinkBase() = default;
		virtual ~ObjectLinkBase() = 0;
	};

	/**
	 * @brief Type to define object link parameters as
	 * ParamsBase::Param< ObjectLink< ObjectT > >
	 * or to define object link list parameters as
	 * ParamsBase::ListParam< ObjectLink< ObjectT > >.
	 * Such parameters own (one or multiple) @p ObjectLink instances and assign a resource to each instance.
	 * @p ObjectLink instances hold a @p std::weak_ptr to the resource and lock this @p weak_ptr if
	 * requested returning a respective @p LinkedObjectWrapper instance. A new resource can be assigned
	 * to an @p ObjectLink instance while @p LinkedObjectWrapper(s) exist since they hold an independent
	 * @p shared_ptr to the resource they have been created from. @p ObjectLink does not need to be
	 * thread-safe itself. All operations on it are performed through the respective ParamsBase::Param or
	 * ParamsBase::ListParam instance. To operate on those instances, the @p ParamsBase instance owning
	 * the respective parameter needs to be locked which renders @p ObjectLink implicitely thread-safe.
	 * @tparam ObjectT Type of the managed @p Object.
	*/
	template <typename ObjectT>
	class ObjectLink : public ObjectLinkBase
	{
	public:
		using ObjectType = ObjectT;								//!< Equals @p ObjectT
		using ConstObjectType = std::add_const_t<ObjectT>;		//!< Equals @p const @p ObjectT

		friend class RunnableInstance;

		/**
		 * @brief Constructs an empty object link.
		*/
		ObjectLink() : DestinyResource() {}

		/**
		 * @brief Constructs an object link pointing to @p DestinyResource.
		 * @param DestinyResource Resource this object link points to
		*/
		ObjectLink(const std::shared_ptr<ObjectType>& DestinyResource) : DestinyResource(DestinyResource) {}

		virtual ~ObjectLink() {}

		/**
		 * @brief Resets this object link rendering it empty.
		*/
		void Reset() { DestinyResource.reset(); }

		/**
		 * @brief Assigns a new resource to this object link.
		 * @param NewDestinyResource Resource this object link now points to
		 * @return Returns the object link instance itself.
		*/
		auto& operator=(const std::shared_ptr<ObjectType>& NewDestinyResource)
		{
			// Throwing an exception here could lead to only partially loaded Param possessing an
			// invalid ObjectLink. Additionally, ParamsConfigDialog::accept() calls
			// ParamsBase::Param<LinkType>::MakeLink() which calls this. ParamsConfigDialog::accept()
			// should not face exceptions. So, do not throw anything here.
			DestinyResource = NewDestinyResource;

			return *this;
		}

		/**
		 * @brief Locks the resource this object link points to by creating a @p shared_ptr from it.
		 * This function can only be directly called by the main thread since only the respective
		 * ParamsBase::Param < LinkType > class has non-const access.
		 * @return @p shared_ptr pointing to the resource this object link points to
		*/
		auto TryLockDestinyRaw()
		{
			// See above.
			return DestinyResource.lock();
		}

	private:
		/**
		 * @brief Locks #DestinyResource and wraps the locked @p Object in a @p LinkedObjectWrapper
		 * instance. Logical const-ness: the locked objects are const, so that users can only call
		 * const member functions on them.
		 * @param WrapperOwner @p RunnableInstance instance owning the @p LinkedObjectWrapper instance
		 * this function attempts to create.
		 * @param Timeout Timeout to be passed to LinkedObjectWrapper::LinkedObjectWrapper
		 * @return Returns a pointer to a @p LinkedObjectWrapper instance containing the locked
		 * @p Object instance. Returns nullptr if this object link is empty.
		*/
		std::unique_ptr<LinkedObjectWrapper<ConstObjectType>> TryLockObject(const RunnableInstance& WrapperOwner,
			std::chrono::milliseconds Timeout = LockObjectTimeoutDefault) const
		{
			std::shared_ptr<ConstObjectType> DestinyShared;

			// While object where DestinyResource points to is being destroyed, DestinyResource
			// should already be marked as expired, so this is thread-safe.
			DestinyShared = DestinyResource.lock();

			if (!DestinyShared)
				return nullptr;

			return std::make_unique<LinkedObjectWrapper<ConstObjectType>>(WrapperOwner, std::move(DestinyShared), Timeout);
		}

		/**
		 * @copybrief TryLockObject
		 * @param WrapperOwner @p RunnableInstance instance owning the @p LinkedObjectWrapper instance
		 * this function attempts to create.
		 * @param Timeout Timeout to be passed to LinkedObjectWrapper::LinkedObjectWrapper
		 * @return  Returns a pointer to a @p LinkedObjectWrapper instance containing the locked
		 * @p Object instance.
		 * @throws Util::InvalidObjectLinkException is thrown if this object link is empty. 
		*/
		auto LockObject(const RunnableInstance& WrapperOwner, std::chrono::milliseconds Timeout = LockObjectTimeoutDefault) const
		{
			auto ObjectWrapperPtr = TryLockObject(WrapperOwner, Timeout);

			if (!ObjectWrapperPtr)
				throw Util::InvalidObjectLinkException();

			return ObjectWrapperPtr;
		}

		/**
		 * @brief Pointer to the (unlocked) resource this object link points to.
		 * Logical const-ness: the @p weak_ptr's type (type of the unlocked @p Object) is not const,
		 * so that the main thread (which locks the objects) can perform any operation on unlocked objects.
		*/
		std::weak_ptr<ObjectType> DestinyResource;
	};

	/**
	 * @brief Defines data for a thread belonging to a @p RunnableObject instance. This data is only
	 * accessed by the RunnableObject instance's thread. So, no synchronization mechanism is needed.
	 * Furthermore, this class is designed to exist only on the stack. @p RunnableInstance instances
	 * are to be passed as a parameter to the respective thread functions. This ensures that a
	 * @p RunnableInstance instances is deleted when its thread function returns. This class is
	 * responsible for locking an object link (list) parameter
	 * (ParamsBase::Param< ObjectLink< ObjectT > > or ParamsBase::ListParam< ObjectLink< ObjectT > >)
	 * hold by a class derived (indirectly) from class @p ParamsBase and for storing the locked resource
	 * in a @p LinkedObjectWrapper instance. In turn, the created @p LinkedObjectWrapper instance is
	 * stored in a @p LinkedObjectWrapperContainer instance hold by a class derived (indirectly) from
	 * @p InstrumentDataBase or from @p ModuleDataBase. Furthermore, this class is responsible for
	 * unlocking @p LinkedObjectWrapper instances it created. Unlocking is triggered either on request
	 * or on destruction of the @p RunnableInstance instance owning the @p LinkedObjectWrapper instance.
	*/
	class RunnableInstance : public Util::INonCopyable
	{
	protected:
		/**
		 * @brief Constructs a non-empty @p RunnableInstance instance.
		 * @param Owner Refer to #Owner.
		 * @param ThreadExitedPromise Refer to #ThreadExitedPromise.
		*/
		RunnableInstance(RunnableObject& Owner, std::promise<void>&& ThreadExitedPromise);

		/**
		 * @brief Move-constructs a @p RunnableInstance instance. @p Other becomes empty.
		 * Not noexcept since LinkedObjectWrapperBase::ListType (@p std::list) move constructor might throw.
		 * @param Other @p RunnableInstance instance to move from
		*/
		RunnableInstance(RunnableInstance&& Other);

		~RunnableInstance();

	public:
		const auto& GetOwner() const noexcept { return Owner; }		//!< Returns #Owner.

		/**
		 * @brief Locks an @p Object instance referenced by a parameter @p LinkParam of type
		 * ParamsBase::Param< ObjectLink< ObjectT > > and makes the @p LinkedObjectWrapperContainer
		 * @p ObjectWrapperContainer point to the locked resource.
		 * @tparam ObjectT Type of the managed @p Object
		 * @param LinkParam Object link parameter describing the destiny resource to lock
		 * @param ObjectWrapperContainer @p LinkedObjectWrapperContainer to store a reference to the
		 * locked resource
		 * @param Timeout Timeout to be passed to ObjectLink::TryLockObject
		*/
		template <typename ObjectT>
		void TryLockObject(const ParamsBase::Param<ObjectLink<ObjectT>>& LinkParam,
			LinkedObjectWrapperContainer<ObjectT>& ObjectWrapperContainer,
			std::chrono::milliseconds Timeout = ObjectLinkBase::LockObjectTimeoutDefault)
		{
			ObjectWrapperContainer.SetLinkedObjWrapperPtr(StoreLockedObject(LinkParam.GetLink().TryLockObject(*this, Timeout), ObjectWrapperContainer));
		}

		/**
		 * @copybrief TryLockObject
		 * @brief Throwing version of @p TryLockObject(). Refer to ObjectLink::LockObject.
		 * @tparam ObjectT Type of the managed @p Object
		 * @param LinkParam Object link parameter describing the destiny resource to lock
		 * @param ObjectWrapperContainer @p LinkedObjectWrapperContainer to store a reference to the
		 * locked resource
		 * @param Timeout Timeout to be passed to ObjectLink::LockObject
		*/
		template <typename ObjectT>
		void LockObject(const ParamsBase::Param<ObjectLink<ObjectT>>& LinkParam,
			LinkedObjectWrapperContainer<ObjectT>& ObjectWrapperContainer,
			std::chrono::milliseconds Timeout = ObjectLinkBase::LockObjectTimeoutDefault)
		{
			ObjectWrapperContainer.SetLinkedObjWrapperPtr(StoreLockedObject(LinkParam.GetLink().LockObject(*this, Timeout), ObjectWrapperContainer));
		}

		/**
		 * @brief Unlocks an @p Object instance stored in the @p LinkedObjectWrapperContainer
		 * @p ObjectWrapperContainer. The @p LinkedObjectWrapperPointer contained in
		 * @p ObjectWrapperContainer is invalidated (set to nullptr)!
		 * @tparam ObjectT Type of the managed @p Object
		 * @param ObjectWrapperContainer @p LinkedObjectWrapperContainer holding a reference to the
		 * locked resource
		*/
		template <typename ObjectT>
		void UnlockObject(LinkedObjectWrapperContainer<ObjectT>& ObjectWrapperContainer)
		{
			// Destroys corresponding LinkedObjectWrapperBase and thereby deregisters from
			// LinkedObjectWrapper::DestinyResource.
			if (ObjectWrapperContainer.LinkedObjWrapperPtr)
				OwnedLinkedObjectWrappers.erase(ObjectWrapperContainer.LinkedObjWrapperPtr->ListPos);

			ObjectWrapperContainer.Reset();
		}

		// TryLockObject() is not overloaded for object link list parameters since it is not clear what
		// to do with already locked objects when trying to lock another list of objects fails. If
		// the previously locked objects were unlocked before, then the state after a failed call to
		// TryLockObject() would not be the same as before.

		/**
		 * @brief Locks @p Object instances referenced by a list parameter @p LinkListParam of type
		 * ParamsBase::ListParam< ObjectLink< ObjectT > > and creates containers in
		 * @p LinkedObjectWrapperContainerList @p ObjectWrapperContainerList which point to the
		 * locked resources.
		 * @tparam ObjectT Type of the (multiple) managed @p Object
		 * @param LinkListParam Object link list parameter describing the destiny resources to lock
		 * @param ObjectWrapperContainerList @p LinkedObjectWrapperContainerList to store references
		 * to the locked resources
		 * @param Timeout Timeout to be passed to ObjectLink::LockObject
		 * @throws Util::EmptyException is thrown if @p LinkListParam is empty and non-optional.
		*/
		template <typename ObjectT>
		void LockObject(const ParamsBase::ListParam<ObjectLink<ObjectT>>& LinkListParam,
			LinkedObjectWrapperContainerList<ObjectT>& ObjectWrapperContainerList,
			std::chrono::milliseconds Timeout = ObjectLinkBase::LockObjectTimeoutDefault)
		{
			// Ensure that ObjectWrapperContainerList is empty before locking.
			UnlockObject(ObjectWrapperContainerList);

			for (const auto& Link : LinkListParam.GetLinks())
			{
				// Create new container in place since StoreLockedObject() stores a reference to it...
				ObjectWrapperContainerList.Containers.emplace_back(std::make_unique<LinkedObjectWrapperContainer<ObjectT>>());
				const auto& Container = ObjectWrapperContainerList.Containers.back();	// noexcept

				try
				{
					// ... and fill it.
					Container->SetLinkedObjWrapperPtr(StoreLockedObject(Link.LockObject(*this, Timeout), *Container));

					ObjectWrapperContainerList.ObjectLabels.emplace_back((*Container)->GetObjectName() + " (" + (*Container)->GetCategoryAndName() + ")");
				}
				catch (...)
				{
					// In case locking object or storing label fails, remove the container again.
					UnlockObject(*Container);
					ObjectWrapperContainerList.Containers.pop_back();

					throw;
				}
			}

			if (!LinkListParam.IsOptional() && ObjectWrapperContainerList.Containers.empty())
				throw Util::EmptyException("A non-optional object link list parameter is empty.");

			ObjectWrapperContainerList.IconPath = LinkListParam.GetIconResourcePath();
		}

		/**
		 * @brief Unlocks @p Object instances stored in the @p LinkedObjectWrapperContainerList
		 * @p ObjectWrapperContainerList.
		 * @tparam ObjectT Type of the (multiple) managed @p Object
		 * @param ObjectWrapperContainerList @p LinkedObjectWrapperContainerList holding
		 * references to the locked resources
		*/
		template <typename ObjectT>
		void UnlockObject(LinkedObjectWrapperContainerList<ObjectT>& ObjectWrapperContainerList)
		{
			for (const auto& Container : ObjectWrapperContainerList.Containers)
				UnlockObject(*Container);

			ObjectWrapperContainerList.Containers.clear();
			ObjectWrapperContainerList.ObjectLabels.clear();
			ObjectWrapperContainerList.IconPath.clear();
		}

		/**
		 * @brief Unregisters owned @p LinkedObjectWrapper instances from their destiny resources in case
		 * of the respective destiny resource not being in a ready state (e.g. a stopped/error state).
		 * Registers again after the respective destiny resource has returned to a ready state.
		 * @return Returns true if the destiny resources of every owned @p LinkedObjectWrapper instance
		 * is in a ready state. Returns false if any destiny resource is in an error state.
		*/
		bool CareAboutWrappers();

		/**
		 * @brief Finds all linked @p Object instances #Owner makes use of which are not in a ready state
		 * and builds a string identifying all these @p Object instances.
		 * @return Human-readable string identifying all linked resources not being in a ready state
		*/
		std::string GetNotReadyObjectNamesString() const;

		/**
		 * @brief Invoke to obtain the parameters (derived from @p ParamsBase) of #Owner.
		*/
		const Object::ParamsGetterType ParamsGetter;

	private:
		/**
		 * @brief Signals that #Owner's thread has exited. Refer to RunnableObject::OnThreadHasExited().
		*/
		void SetThreadExited();

		/**
		 * @brief Stores a newly created @p LinkedObjectWrapper instance in #OwnedLinkedObjectWrappers.
		 * @tparam ObjectT Type of the managed @p Object
		 * @param ObjectWrapperPtr Pointer to the newly created @p LinkedObjectWrapper instance to take
		 * ownership of
		 * @param ObjectWrapperContainer Reference to the @p ObjectWrapperContainer referencing the
		 * newly created @p LinkedObjectWrapper instance
		 * @return Pointer to the stored @p LinkedObjectWrapper instance
		*/
		template <typename ObjectT>
		decltype(auto) StoreLockedObject(std::unique_ptr<LinkedObjectWrapper<ObjectT>>&& ObjectWrapperPtr,
			LinkedObjectWrapperContainerBase& ObjectWrapperContainer)
		{
			OwnedLinkedObjectWrappers.emplace_back(std::move(ObjectWrapperPtr), ObjectWrapperContainer);

			auto ObjectWrapperIterator = --OwnedLinkedObjectWrappers.end();
			ObjectWrapperIterator->OwnedLinkedObjectWrapperPtr.get()->ListPos = ObjectWrapperIterator;

			// Cast should never fail. Reference version since this would throw if it failed.
			return &dynamic_cast<typename LinkedObjectWrapperPointer<ObjectT>::LinkedObjectWrapperType&>(*ObjectWrapperIterator->OwnedLinkedObjectWrapperPtr.get());
		}

		/**
		 * @brief @p RunnableObject instance which operates on this @p RunnableInstance (by its thread).
		 * The @p RunnableObject instance's tasks/events might access the object itself via @p GetOwner().
		*/
		const RunnableObject& Owner;

		/**
		 * @brief Signals the @p RunnableObject instance owning the thread that its thread has terminated.
		 * This is signaled on destruction of this @p RunnableInstance instance.
		 * Refer to RunnableObject::ThreadExitedSignal and to RunnableObject::OnThreadHasExited().
		*/
		std::promise<void> ThreadExitedPromise;

		/**
		 * @brief Set to true if it was moved from this instance to indicate that #ThreadExitedPromise
		 * has no shared state anymore.
		*/
		bool Empty = false;

		/**
		 * @brief List of all @p LinkedObjectWrapper instances owned by this @p RunnableInstance instance
		 * (thus belonging to a @p RunnableObject instance). If the @p RunnableInstance's parent
		 * @p RunnableObject is terminated, by deleting the corresponding @p RunnableInstance, all these
		 * @p LinkedObjectWrapper instances are also deleted to unlock their linked resources.
		*/
		LinkedObjectWrapperBase::ListType OwnedLinkedObjectWrappers;
	};
}