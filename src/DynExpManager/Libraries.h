// This file is part of DynExp.

/**
 * @file Libraries.h
 * @brief Provides templates to bundle %DynExp resources at compile time in libraries.
*/

#pragma once

#include "stdafx.h"
#include "HardwareAdapter.h"
#include "Instrument.h"
#include "Module.h"

namespace DynExp
{
	/**
	 * @brief Alias for a factory function pointer type that instantiates library entries to create %DynExp objects
	 * derived from DynExp::Object. As arguments, it expects the thread ID of the thread owning the resource and
	 * a reference to the resource's parameters to take ownership of. Compatible e.g. to DynExp::MakeModule().
	 * @tparam ObjectTypeBasePtr Pointer to the base type of a certain %DynExp resource (e.g. DynExp::ModulePtrType)
	*/
	template <typename ObjectTypeBasePtr>
	using LibraryObjectFactoryPtrType = ObjectTypeBasePtr(*)(const std::thread::id, ParamsBasePtrType&&);

	/**
	 * @brief Alias for a factory function pointer type that creates a configurator for a specific library entry.
	 * Compatible e.g. to DynExp::MakeModuleConfig().
	*/
	using LibraryConfigFactoryPtrType = ConfiguratorBasePtrType(*)();

	/**
	 * @brief String type used for library entry descriptors
	*/
	using LibraryStringType = const char*;

	/**
	 * @brief Pointer to a %DynExp object's static constexpr function returning a string literal. Used for the
	 * Name() and Category() functions which are required for classes derived from DynExp::Object to become
	 * library entries.
	*/
	using LibraryStringParamPtrType = LibraryStringType(*)();

	/**
	 * @brief Type consisting of function pointer types instantiating %DynExp objects. Refer to
	 * DynExp::LibraryObjectFactoryPtrType.
	 * @tparam ObjectTypeBasePtr Refer to DynExp::LibraryObjectFactoryPtrType.
	 * @tparam ... Function pointer types
	*/
	template <typename ObjectTypeBasePtr, LibraryObjectFactoryPtrType<ObjectTypeBasePtr>...>
	class LibraryObjectFactoryParamList {};

	/**
	 * @brief Type consisting of function pointer types returning configurators for %DynExp objects.
	 * Refer to DynExp::ConfiguratorBasePtrType.
	 * @tparam ... Function pointer types
	*/
	template <LibraryConfigFactoryPtrType...>
	class LibraryConfigFactoryParamList {};

	/**
	 * @brief Type consisting of function pointer types returning string literals. Refer to
	 * DynExp::LibraryStringParamPtrType.
	 * @tparam ... Function pointer types
	*/
	template <LibraryStringParamPtrType...>
	class LibraryStringParamList {};

	/**
	 * @brief Represents an entry in the library.
	 * @tparam ObjectTypeBasePtr Refer to DynExp::LibraryObjectFactoryPtrType.
	*/
	template <typename ObjectTypeBasePtr>
	struct LibraryEntry
	{
		/**
		 * @brief Constructs a library entry.
		 * @param ObjectFactoryPtr Pointer to the object factory function. Refer to
		 * DynExp::LibraryObjectFactoryPtrType.
		 * @param ConfigFactoryPtr Pointer to the object configurator factory function.
		 * Refer to DynExp::LibraryConfigFactoryPtrType.
		 * @param Name Name of the entry (human-readable)
		 * @param Category Category of the entry (human-readable)
		*/
		constexpr LibraryEntry(const LibraryObjectFactoryPtrType<ObjectTypeBasePtr> ObjectFactoryPtr,
			const LibraryConfigFactoryPtrType ConfigFactoryPtr,
			const LibraryStringType Name, const LibraryStringType Category) noexcept
			: ObjectFactoryPtr(ObjectFactoryPtr), ConfigFactoryPtr(ConfigFactoryPtr),
			Name(Name), Category(Category)
		{}

		LibraryObjectFactoryPtrType<ObjectTypeBasePtr> ObjectFactoryPtr;	//!< Refer to LibraryEntry::LibraryEntry()
		LibraryConfigFactoryPtrType ConfigFactoryPtr;						//!< Refer to LibraryEntry::LibraryEntry()
		LibraryStringType Name;												//!< Refer to LibraryEntry::LibraryEntry()
		LibraryStringType Category;											//!< Refer to LibraryEntry::LibraryEntry()
	};

	/**
	 * @brief Represents the base class for a library containing library entries (DynExp::LibraryEntry).
	 * @tparam ObjectTypeBasePtr Refer to DynExp::LibraryObjectFactoryPtrType.
	 * @tparam ObjFactories Type of multiple object factory pointers
	 * @tparam ConfFactories Type of multiple configurator factory pointers
	 * @tparam ObjNames Type of multiple function pointers returning the entry names
	 * @tparam ObjCategories Type of multiple function pointers returning the entry categories
	*/
	template <typename ObjectTypeBasePtr,
		typename ObjFactories,
		typename ConfFactories,
		typename ObjNames,
		typename ObjCategories
	>
	class LibraryBase;

	/**
	 * @brief Represents the specialized base class for a library containing library entries (DynExp::LibraryEntry).
	 * @tparam ObjectTypeBasePtr Refer to DynExp::LibraryObjectFactoryPtrType.
	 * @tparam ...ObjFactories Variadic template parameter for object factory pointers
	 * @tparam ...ConfFactories Variadic template parameter for configurator factory pointers
	 * @tparam ...NameFuncs Variadic template parameter for function pointers returning the entry names
	 * @tparam ...CategoryFuncs Variadic template parameter for function pointers returning the entry categories
	*/
	template <typename ObjectTypeBasePtr,
		LibraryObjectFactoryPtrType<ObjectTypeBasePtr>... ObjFactories,
		LibraryConfigFactoryPtrType... ConfFactories,
		LibraryStringParamPtrType... NameFuncs,
		LibraryStringParamPtrType... CategoryFuncs
	>
	class LibraryBase<
		ObjectTypeBasePtr,
		LibraryObjectFactoryParamList<ObjectTypeBasePtr, ObjFactories...>,
		LibraryConfigFactoryParamList<ConfFactories...>,
		LibraryStringParamList<NameFuncs...>,
		LibraryStringParamList<CategoryFuncs...>
	>
	{
	protected:
		/**
		 * @brief Constructs a library and fills its entry array from its type information.
		 * The entry array is sorted in alphabetic order first by the entry categories and
		 * second by the entry names.
		*/
		constexpr LibraryBase() : Entries{ LibraryEntry(ObjFactories, ConfFactories, NameFuncs(), CategoryFuncs())... }
		{
			std::sort(Entries.begin(), Entries.end(), [](const auto& a, const auto& b) constexpr {
				using ct = std::char_traits<char>;

				// Sort by category first...
				const auto CategoryDiff = ct::compare(a.Category, b.Category,
					std::min(ct::length(a.Category), ct::length(b.Category)));

				// ...and by name second if categories equal.
				if (CategoryDiff == 0)
					return std::lexicographical_compare(a.Name, a.Name + ct::length(a.Name),
						b.Name, b.Name + ct::length(b.Name));

				return CategoryDiff < 0;
			});	
		}

		~LibraryBase() = default;

	public:
		/**
		 * @brief Returns the library entry array.
		 * @return Reference to the library entries.
		*/
		constexpr const auto& GetLibrary() const noexcept { return Entries; }

		/**
		 * @brief Converts the library entries to a @p std::vector.
		 * @return @p std::vector containing the library entries
		*/
		auto ToVector() const
		{
			return std::vector<LibraryEntry<ObjectTypeBasePtr>>(Entries.cbegin(), Entries.cend());
		}

	private:
		/**
		 * @brief Array containing the library entries
		*/
		std::array<LibraryEntry<ObjectTypeBasePtr>, sizeof...(ObjFactories)> Entries;
	};

	/**
	 * @brief Represents a hardware adapter library to generate instances of classes
	 * derived from DynExp::HardwareAdapterBase.
	 * @tparam ...HardwareAdapterTypes Variadic template parameter for types derived
	 * from DynExp::HardwareAdapterBase
	*/
	template <typename... HardwareAdapterTypes>
	class HardwareAdapterLibrary : public LibraryBase<
		HardwareAdapterPtrType,
		LibraryObjectFactoryParamList<HardwareAdapterPtrType, &MakeHardwareAdapter<HardwareAdapterTypes>...>,
		LibraryConfigFactoryParamList<&MakeHardwareAdapterConfig<HardwareAdapterTypes>...>,
		LibraryStringParamList<&HardwareAdapterTypes::Name...>,
		LibraryStringParamList<&HardwareAdapterTypes::Category...>
	>
	{};

	/**
	 * @brief Alias for the vector type containing entries of a hardware adapter library
	*/
	using HardwareAdapterLibraryVectorType = std::vector<LibraryEntry<HardwareAdapterPtrType>>;

	/**
	 * @brief Represents an instrument library to generate instances of classes
	 * derived from DynExp::InstrumentBase.
	 * @tparam ...InstrumentTypes Variadic template parameter for types derived
	 * from DynExp::InstrumentBase
	*/
	template <typename... InstrumentTypes>
	class InstrumentLibrary : public LibraryBase<
		InstrumentPtrType,
		LibraryObjectFactoryParamList<InstrumentPtrType, &MakeInstrument<InstrumentTypes>...>,
		LibraryConfigFactoryParamList<&MakeInstrumentConfig<InstrumentTypes>...>,
		LibraryStringParamList<&InstrumentTypes::Name...>,
		LibraryStringParamList<&InstrumentTypes::Category...>
	>
	{};

	/**
	 * @brief Alias for the vector type containing entries of a instrument library
	*/
	using InstrumentLibraryVectorType = std::vector<LibraryEntry<InstrumentPtrType>>;

	/**
	 * @brief Represents a module library to generate instances of classes
	 * derived from DynExp::ModuleBase.
	 * @tparam ...ModuleTypes Variadic template parameter for types derived
	 * from DynExp::ModuleBase
	*/
	template <typename... ModuleTypes>
	class ModuleLibrary : public LibraryBase<
		ModulePtrType,
		LibraryObjectFactoryParamList<ModulePtrType, &MakeModule<ModuleTypes>...>,
		LibraryConfigFactoryParamList<&MakeModuleConfig<ModuleTypes>...>,
		LibraryStringParamList<&ModuleTypes::Name...>,
		LibraryStringParamList<&ModuleTypes::Category...>
	>
	{};

	/**
	 * @brief Alias for the vector type containing entries of a module library
	*/
	using ModuleLibraryVectorType = std::vector<LibraryEntry<ModulePtrType>>;

	/**
	 * @brief Finds an entry in a library vector by category and name.
	 * @tparam LibraryVectorT Type of the library vector
	 * @param LibraryVector Library vector to search in
	 * @param Category Category to search for
	 * @param Name Name to search for
	 * @return Returns a reference to the found entry.
	 * @throws Util::NotFoundException is thrown if the entry is not found.
	*/
	template <typename LibraryVectorT>
	const typename LibraryVectorT::value_type& FindInLibraryVector(const LibraryVectorT& LibraryVector,
		const std::string& Category, const std::string& Name)
	{
		auto Result = std::find_if(LibraryVector.cbegin(), LibraryVector.cend(),
			[&Category, &Name](const auto& LibEntry) {
				return Category == std::string(LibEntry.Category) && Name == std::string(LibEntry.Name);
		});

		if (Result == LibraryVector.cend())
			throw Util::NotFoundException("An item (" + Object::CategoryAndNameToStr(Category, Name)
				+ ") has not been found in the given library vector.");

		return *Result;
	}
}