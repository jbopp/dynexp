// This file is part of DynExp.

/**
 * @file Util.h
 * @brief Provides general utilities within %DynExp's %Util namespace.
*/

#pragma once

#include "Exception.h"

/**
 * @brief %DynExp's %Util namespace contains commonly used functions and templates as well as extensions
 * to Qt and its widgets.
*/
namespace Util
{
	/**
	 * @brief Interface to delete copy constructor and copy assignment operator and thus make derived
	 * classes non-copyable.
	*/
	class INonCopyable
	{
	protected:
		constexpr INonCopyable() = default;
		~INonCopyable() = default;

	public:
		INonCopyable(const INonCopyable&) = delete;
		INonCopyable& operator=(const INonCopyable&) = delete;
	};

	/**
	 * @brief Interface to delete move constructor and move assignment operator and thus make derived
	 * classes non-movable.
	*/
	class INonMovable
	{
	protected:
		constexpr INonMovable() = default;
		~INonMovable() = default;

	public:
		INonMovable(const INonMovable&) = default;
		INonMovable& operator=(const INonMovable&) = default;

		INonMovable(INonMovable&&) = delete;
		INonMovable& operator=(INonMovable&&) = delete;
	};

	/**
	 * @brief Interface to allow synchronizing the access to derived classes between different threads
	 * by providing a mutex and a method to lock that mutex. Recursive locking is @b not allowed.
	*/
	class ILockable : public INonCopyable
	{
	public:
		/**
		 * @brief Duration which is used as a default timeout within all methods of this class if no
		 * different duration is passed to them.
		*/
		static constexpr std::chrono::milliseconds DefaultTimeout = std::chrono::milliseconds(10);

	protected:
		using MutexType = std::timed_mutex;
		using LockType = std::unique_lock<MutexType>;

		ILockable() = default;
		~ILockable() = default;

		/**
		 * @brief Locks the internal mutex. Blocks until the mutex is locked or until the timeout
		 * duration is exceeded.
		 * @param Timeout Time to wait trying to lock the internal mutex.
		 * 0 ms means that the functions waits for the mutex without timing out ever.
		 * @return Returns a lock guard which unlocks the locked mutex upon destruction.
		 * @throws TimeoutException is thrown if timeout duration is exceeded.
		*/
		[[nodiscard]] LockType AcquireLock(const std::chrono::milliseconds Timeout = DefaultTimeout) const;

	private:
		mutable MutexType LockMutex;	//!< Internal mutex used for locking.
	};

	class TimeoutException;
	template <typename> class SynchronizedPointer;

	/**
	 * @brief Interface to allow synchronizing the access to derived classes between different threads
	 * by making the class lockable by SynchronizedPointer smart pointer objects. Recursive locking is allowed.
	*/
	class ISynchronizedPointerLockable : public INonCopyable
	{
		// Every SynchronizedPointer<...> should be friend to allow SynchronizedPointers to derived classes
		template <typename>
		friend class SynchronizedPointer;

	protected:
		ISynchronizedPointerLockable() : OwnedCount(0) {}
		~ISynchronizedPointerLockable() { assert(!OwnedCount); }	//!< Object should never be destroyed before completely unlocked.

	private:
		/**
		 * @brief Locks the internal mutex. Blocks until the mutex is locked or until the timeout
		 * duration is exceeded. Recursive locking is allowed.
		 * @param Timeout Time to wait trying to lock the internal mutex.
		 * 0 ms means that the functions waits for the mutex without timing out ever.
		 * Called by class SynchronizedPointer's constructor. So, default value is given there.
		 * @throws TimeoutException is thrown if timeout duration is exceeded.
		*/
		void AcquireLock(const std::chrono::milliseconds Timeout) const
		{
			using namespace std::chrono_literals;

			// In order to compensate for spurious failures by retrying
			// (see https://en.cppreference.com/w/cpp/thread/timed_mutex/try_lock_for)
			constexpr int NumTries = 2;

			if (OwnerID != std::this_thread::get_id())
			{
				if (Timeout == 0ms)
					LockMutex.lock();
				else
				{
					bool Success = false;
					auto TimeoutPerTry = Timeout / NumTries;
					for (auto i = NumTries; i > 0 && !Success; --i)
						Success = LockMutex.try_lock_for(TimeoutPerTry);

					if (!Success)
						throw TimeoutException("Timeout occurred while trying to lock a mutex.");
				}

				OwnerID = std::this_thread::get_id();
			}

			++OwnedCount;
		}

		/**
		 * @brief Releases the internal mutex. Does nothing if the mutex was not locked or if the calling
		 * thread is not the current owner of the muetx.
		*/
		void ReleaseLock() const
		{
			if (!OwnedCount || OwnerID != std::this_thread::get_id())
				return;
			
			--OwnedCount;
			if (!OwnedCount)
			{
				OwnerID = std::thread::id();
				LockMutex.unlock();
			}
		}

		mutable std::timed_mutex LockMutex;					//!< Internal mutex used for locking.
		mutable std::atomic<std::thread::id> OwnerID;		//!< ID of the thread which currently owns the internal mutex.
		mutable std::atomic<size_t> OwnedCount;				//!< Counts the lock requests of the current owning thread.
	};

	/**
	 * @brief Pointer to lock a class derived from @p ISynchronizedPointerLockable
	 * for synchronizing between threads. Instances of this class are not intended to be stored
	 * somewhere since they make other threads block. Only use as temporary objects.
	 * @tparam T Type derived from @p ISynchronizedPointerLockable to be managed by this pointer.
	*/
	template <typename T>
	class SynchronizedPointer : public INonCopyable
	{
		template <typename>
		friend class SynchronizedPointer;

	public:
		/**
		 * @brief Contructs an instance with an empty pointer.
		*/
		SynchronizedPointer() noexcept : LockableObject(nullptr) {}

		/**
		 * @brief Constructs a pointer locking @p LockableObject. Blocks until @p LockableObject's
		 * mutex is locked or until the timeout duration is exceeded. Recursive locking is allowed.
		 * @param LockableObject Pointer to an instance of a class derived from
		 * @p ISynchronizedPointerLockable to be locked.
		 * @param Timeout Time to wait trying to lock the internal mutex.
		 * 0 ms means that the functions waits for the mutex without timing out ever.
		*/
		SynchronizedPointer(T* const LockableObject,
			const std::chrono::milliseconds Timeout = ILockable::DefaultTimeout)
			: LockableObject(LockableObject) { if (LockableObject) LockableObject->AcquireLock(Timeout); }

		/**
		 * @brief Moves the @p LockableObject from another SynchronizedPointer instance @p Other
		 * to a new instance. @p Other becomes empty.
		 * @param Other SynchronizedPointer instance to move from
		*/
		SynchronizedPointer(SynchronizedPointer&& Other) noexcept : LockableObject(Other.LockableObject) { Other.LockableObject = nullptr; }

		/**
		 * @brief Move-assigns the @p LockableObject from another SynchronizedPointer instance @p Other
		 * to this instance. @p Other becomes empty.
		 * @param Other SynchronizedPointer instance to move from 
		 * @return Reference to this instance
		*/
		SynchronizedPointer& operator=(SynchronizedPointer&& Other) noexcept
		{
			LockableObject = Other.LockableObject;
			Other.LockableObject = nullptr;

			return *this;
		}

		/**
		 * @brief Moves the @p LockableObject from a SynchronizedPointer\<U\> of another type @p U
		 * to a new instance of SynchronizedPointer\<T\> using dynamic_cast. @p Other becomes empty.
		 * @tparam U Type derived from @p ISynchronizedPointerLockable managed by @p Other
		 * @param Other SynchronizedPointer instance to move from
		 * @throws TypeErrorException is thrown if dynamic_cast fails due to an incompatible type @p U.
		*/
		template <typename U>
		explicit SynchronizedPointer(SynchronizedPointer<U>&& Other)
		{
			if (!Other.LockableObject)
				LockableObject = nullptr;
			else
			{
				LockableObject = dynamic_cast<T*>(Other.LockableObject);
				if (!LockableObject)
					throw Util::TypeErrorException();

				Other.LockableObject = nullptr;
			}
		}

		~SynchronizedPointer() { if (LockableObject) LockableObject->ReleaseLock(); }

		/**
		 * @brief Returns the managed (locked) object.
		 * @return Pointer to an instance of a class derived from @p ISynchronizedPointerLockable
		*/
		auto get() const noexcept { return LockableObject; }

		bool operator==(const T* rhs) const noexcept { return LockableObject == rhs; }
		bool operator!=(const T* rhs) const noexcept { return LockableObject != rhs; }
		bool operator==(const SynchronizedPointer& rhs) const noexcept { return LockableObject == rhs.get(); }
		bool operator!=(const SynchronizedPointer& rhs) const noexcept { return LockableObject != rhs.get(); }
		explicit operator bool() const noexcept { return LockableObject != nullptr; }

		auto operator->() const noexcept { return LockableObject; }
		auto& operator*() const noexcept { return *LockableObject; }

	private:
		/**
		 * @brief Pointer to the locakable object managed by this class
		*/
		T* LockableObject;
	};

	/**
	 * @brief Helper class to communicate flags between different threads based on a condition variable and a mutex.
	 * Two threads make use of the same instance of this class. One of the thread awaits a flag to be set by the
	 * other thread.
	*/
	class OneToOneNotifier : public INonCopyable
	{
	public:
		OneToOneNotifier() : EventOccurred(false), SomeoneIsWaiting(false), MutexCanBeDestroyed(true) {}
		~OneToOneNotifier();

		/**
		 * @brief Makes current thread wait until it is notified or until given timeout duration is exceeded.
		 * Destructor might be called while Wait() is waiting (the waiting thread will be notified then).
		 * Wait() must not be called (of course) while destructor is running.
		 * @param Timeout Time to wait for a notification. 0 ms means that the functions waits without timing out ever.
		 * @return Returns true if the function returned due to an exceeded timeout duration. Returns false
		 * if the function returned due to the OneToOneNotifier having been notified.
		 * @throws InvalidCallException is thrown if a thread is already waiting for this notifier.
		*/
		bool Wait(const std::chrono::milliseconds Timeout = std::chrono::milliseconds(0));

		void Notify();	//!< Set notification to stop waiting (sets EventOccurred to true).
		void Ignore();	//!< Ignore last notification (sets EventOccurred to false).

	private:
		bool EventOccurred;
		bool SomeoneIsWaiting;
		std::atomic<bool> MutexCanBeDestroyed;

		std::mutex Mutex;
		std::condition_variable ConditionVariable;
	};

	/**
	 * @brief Checks whether a type @p T is contained in a template parameter pack of types @p ListTs.
	*/
	template <typename T, typename... ListTs>
	struct is_contained_in : std::disjunction<std::is_same<T, ListTs>...> {};

	/**
	 * @brief Value type of @p is_contained_in
	*/
	template <typename T, typename... ListTs>
	inline constexpr bool is_contained_in_v = is_contained_in<T, ListTs...>::value;

	/**
	 * @brief Extracts the return value type, the class type the callable is member of, and the argument types
	 * of a callable type @p CallableT which is a member function of some class.
	 * The extracted types are stored using type aliases @p return_type and @p instance_type and an alias
	 * of a tuple of types @p argument_types.
	*/
	template <typename CallableT>
	struct member_fn_ptr_traits;

	/**
	 * @copydoc member_fn_ptr_traits
	*/
	template <typename ReturnT, typename ObjectT, typename... ArgumentTs>
	struct member_fn_ptr_traits<ReturnT (ObjectT::*)(ArgumentTs...) const>
	{
		using return_type = ReturnT;
		using instance_type = ObjectT;
		using argument_types = std::tuple<ArgumentTs...>;
	};

	/**
	 * @copydoc member_fn_ptr_traits
	*/
	template <typename ReturnT, typename ObjectT, typename... ArgumentTs>
	struct member_fn_ptr_traits<ReturnT(ObjectT::*)(ArgumentTs...) noexcept>
	{
		using return_type = ReturnT;
		using instance_type = ObjectT;
		using argument_types = std::tuple<ArgumentTs...>;
	};

	/**
	 * @copydoc member_fn_ptr_traits
	*/
	template <typename ReturnT, typename ObjectT, typename... ArgumentTs>
	struct member_fn_ptr_traits<ReturnT(ObjectT::*)(ArgumentTs...) const noexcept>
	{
		using return_type = ReturnT;
		using instance_type = ObjectT;
		using argument_types = std::tuple<ArgumentTs...>;
	};

	/**
	 * @copydoc member_fn_ptr_traits
	*/
	template <typename ReturnT, typename ObjectT, typename... ArgumentTs>
	struct member_fn_ptr_traits<ReturnT (ObjectT::*)(ArgumentTs...)>
	{
		using return_type = ReturnT;
		using instance_type = ObjectT;
		using argument_types = std::tuple<ArgumentTs...>;
	};

	/**
	 * @brief Alias for the return type of a member function callable of type @p CallableT
	*/
	template <typename CallableT>
	using return_of_t = typename member_fn_ptr_traits<CallableT>::return_type;

	/**
	 * @brief Alias for the class type a member function callable of type @p CallableT is member of
	*/
	template <typename CallableT>
	using instance_of_t = typename member_fn_ptr_traits<CallableT>::instance_type;

	/**
	 * @brief Alias for a tuple of argument types the member function callable of type @p CallableT expects
	*/
	template <typename CallableT>
	using argument_of_t = typename member_fn_ptr_traits<CallableT>::argument_types;

	/**
	 * @brief Removes first type from a tuple of types @p TupleT.
	*/
	template <typename TupleT>
	struct remove_first_from_tuple;

	/**
	 * @copydoc remove_first_from_tuple
	*/
	template <typename FirstElementT, typename... ElementTs>
	struct remove_first_from_tuple<std::tuple<FirstElementT, ElementTs...>> { using type = std::tuple<ElementTs...>; };

	/**
	 * @brief Alias for a tuple of types where the first type of the input tuple @p TupleT is removed
	*/
	template <typename TupleT>
	using remove_first_from_tuple_t = typename remove_first_from_tuple<TupleT>::type;

	// Index sequence manipulation to define a starting point 

	/**
	 * @brief Holds an alias for a std::index_sequence where all indices are shifted by an offset.
	 * @tparam IndexSequence Indices passes as template arguments to std::index_sequence
	 * @tparam Offset Offset added to all indices
	*/
	template <size_t Offset, typename IndexSequence>
	struct OffsetIndexSequence;

	/**
	 * @copydoc OffsetIndexSequence
	*/
	template <size_t Offset, size_t... Indices>
	struct OffsetIndexSequence<Offset, std::index_sequence<Indices...>>
	{
		/**
		 * @brief Alias for offset index sequence 
		*/
		using type = std::index_sequence<Indices + Offset...>;
	};

	/**
	 * @brief Alias for type contained in @p OffsetIndexSequence
	*/
	template <size_t Offset, typename IndexSequence>
	using OffsetIndexSequence_t = typename OffsetIndexSequence<Offset, IndexSequence>::type;

	/**
	 * @brief Holds an alias for a std::index_sequence spanning a certain range
	 * @tparam From Start value the index sequency begins with
	 * @tparam To Last value contained in the index sequence
	*/
	template <size_t From, size_t To>
	struct RangeIndexSequence
	{
		using type = OffsetIndexSequence_t<From, std::make_index_sequence<To - From>>;
	};

	/**
	 * @brief Alias for type contained in @p RangeIndexSequence
	*/
	template <size_t From, size_t To>
	using RangeIndexSequence_t = typename RangeIndexSequence<From, To>::type;

	/**
	 * @brief Wraps a member function of some object and stores its default arguments.
	 * Moving from CallableMemberWrapper does not work since this class holds const members
	 * and a reference to the object the member function is invoked on. Copy instead.
	 * @tparam ObjectT Type of the object the member function belongs to
	 * @tparam CallableT The member function's function pointer type
	*/
	template <typename ObjectT, typename CallableT>
	class CallableMemberWrapper : public INonMovable
	{
		using ArgumentTs = argument_of_t<CallableT>;

	public:
		/**
		 * @brief Constructs a @p CallableMemberWrapper instance.
		 * @param Object Instance the member function is invoked on
		 * @param Callable Pointer to a member function
		 * @param DefaultArgs Default arguments to be passed to the member function upon
		 * invocation without passing other arguments
		*/
		constexpr CallableMemberWrapper(ObjectT& Object, const CallableT Callable, ArgumentTs DefaultArgs = {}) noexcept
			: Object(Object), Callable(Callable), DefaultArgs(std::move(DefaultArgs)) {}

		/**
		 * @brief Invokes the stored member function.
		 * If arguments are passed, they are are forwarded instead of the stored default arguments.
		 * @tparam ...ArgTs Types of the arguments expected by the wrapped member function
		 * @param ...Args Arguments to be forwarded to the wrapped member function
		 * @return Returns the result of the wrapped member function.
		*/
		template <typename... ArgTs>
		auto operator()(ArgTs&& ...Args) const
		{
			return Invoke(RangeIndexSequence_t<sizeof...(ArgTs), std::tuple_size_v<ArgumentTs>>(), std::forward<ArgTs>(Args)...);
		}

	private:
		template <size_t... Indices, typename... ArgTs>
		auto Invoke(std::integer_sequence<size_t, Indices...>, ArgTs&& ...Args) const
		{
			return (Object.*Callable)(std::forward<ArgTs>(Args)..., std::get<Indices>(DefaultArgs)...);
		}

		ObjectT& Object;				//!< Instance of class @p Callable belongs to. @p Callable is invoked on this instance.
		const CallableT Callable;		//!< Pointer to class member function to be invoked
		const ArgumentTs DefaultArgs;	//!< Default arguments to be passed when invoking operator() with less arguments @p Callable expects
	};

	/**
	 * @brief Holds a @p CallableMemberWrapper and invokes its callable when being destroyed.
	 * @tparam ObjectT Type of the object the member function belongs to
	 * @tparam CallableT The member function's function pointer type
	*/
	template <typename ObjectT, typename CallableT>
	class OnDestruction
	{
	public:
		/**
		 * @brief Constructs a @p OnDestruction instance which calls @p Callable upon destruction of this instance.
		 * @tparam ...ArgTs Types of the arguments expected by the wrapped member function
		 * @param Object Instance the member function is invoked on
		 * @param Callable Pointer to a member function
		 * @param ...Args Arguments to be passed to the member function upon invocation
		*/
		template <typename... ArgTs>
		OnDestruction(ObjectT& Object, const CallableT Callable, ArgTs&& ...Args)
			: CallableWrapper(Object, std::move(Callable), { std::forward<ArgTs>(Args)... }) {}

		~OnDestruction() { CallableWrapper(); }

	private:
		CallableMemberWrapper<ObjectT, CallableT> CallableWrapper;
	};

	/**
	 * @brief Data type which manages a binary large object. The reserved memory is freed upon destruction.
	*/
	class BlobDataType
	{
	public:
		using DataType = unsigned char[];						//!< Type of the buffer's data

	private:
		using DataPtrType = std::unique_ptr<DataType>;			//!< Type of the underlying smart pointer managing the buffer

	public:
		BlobDataType() = default;								//!< Constructs an empty object.
		BlobDataType(const BlobDataType& Other);				//!< Constructs an object copying data from @p Other.
		BlobDataType(BlobDataType&& Other) noexcept;			//!< Constructs an object moving from @p Other. @p Other is empty afterwards.

		BlobDataType& operator=(const BlobDataType& Other);		//!< Copy-assigns data from @p Other.
		BlobDataType& operator=(BlobDataType&& Other) noexcept;	//!< Move-assigns data from @p Other. @p Other is empty afterwards.

		void Reserve(size_t Size);								//!< Reserves @p Size bytes of memory freeing any previously reserved memory.
		void Assign(size_t Size, const DataType Data);			//!< Copies @p Size bytes from @p Data to the buffer freeing any previously reserved memory.
		void Reset();											//!< Frees any reserved memory.
		DataPtrType::element_type* Release() noexcept;			//!< Releases ownership of the stored buffer returning a pointer to it and leaving this instance empty.
		auto GetPtr() noexcept { return DataPtr.get(); }		//!< Returns a pointer to the stored buffer.
		auto Size() const noexcept { return DataSize; }			//!< Returns the size of the stored data in bytes.

	private:
		DataPtrType DataPtr;									//!< Pointer to the buffer
		size_t DataSize = 0;									//!< Size of the stored data in bytes
	};

	/**
	 * @brief Data type which stores an optional bool value (unknown, false, true).
	 * The type evaluates to bool while an unknown value is considered false.
	*/
	class OptionalBool
	{
	public:
		/**
		 * @brief Possible values. @p Values::Unknown evaluates to false.
		*/
		enum class Values { Unknown, False, True };

		constexpr OptionalBool() noexcept : Value(Values::Unknown) {}													//!< Contructs an instance holding @p Values::Unknown.
		constexpr OptionalBool(Values Value) noexcept : Value(Value) {}													//!< Contructs an instance holding @p Value.
		constexpr OptionalBool(bool b) noexcept : Value(b ? Values::True : Values::False) {}							//!< Contructs an instance holding @p b.
		constexpr OptionalBool(const OptionalBool& Other) noexcept : Value(Other.Value) {}								//!< Contructs a copy of @p Other.

		constexpr OptionalBool& operator=(Values Value) noexcept { this->Value = Value; return *this; }					//!< Assigns @p Value, returns reference to this.
		constexpr OptionalBool& operator=(bool b) noexcept { Value = b ? Values::True : Values::False; return *this; }	//!< Assigns @p b, returns reference to this.
		constexpr OptionalBool& operator=(OptionalBool& Other) noexcept { Value = Other.Value; return *this; }			//!< Assigns value of @p Other, returns reference to this.

		constexpr bool operator==(Values Value) const noexcept { return this->Value == Value; }							//!< Returns true when @p Value matches stored value, false otherwise.
		constexpr bool operator!=(Values Value) const noexcept { return this->Value != Value; }							//!< Returns false when @p Value matches stored value, true otherwise.

		constexpr operator bool() const noexcept { return Value == Values::True; }										//!< Converts internal value to bool.
		constexpr Values Get() const noexcept { return Value; }															//!< Returns internal value.

	private:
		Values Value;																									//!< Internal value
	};

	/** @name Conversion functions
	 * These functions can be used to convert between different number and string types.
	*/
	///@{
	/**
	 * @brief Formats a time point to a human-readable string describing the time in the current time zone
	 * and writes the string to a stream.
	 * @tparam T @p TimePoint's clock type
	 * @param stream Stream to write to
	 * @param TimePoint Time point to format and to write to stream
	 * @return Stream which was passed to the function
	*/
	template <typename T>
	std::ostream& operator<<(std::ostream& stream, const std::chrono::time_point<T>& TimePoint)
	{
		const auto ZonedTime = std::chrono::zoned_time(std::chrono::current_zone(), std::chrono::round<std::chrono::seconds>(TimePoint));
		stream << std::format("{:%T %d.%m.%Y}", ZonedTime);

		return stream;
	}

	/**
	 * @brief Converts a std::string to a value of type @p T using operator<< of std::stringstream.
	 * @tparam T Type of the value the string should be converted to
	 * @param String String to convert
	 * @return Returns the converted value of type @p T.
	 * @throws InvalidDataException is thrown in case of an incompatible conversion.
	*/
	template <typename T>
	T StrToT(const std::string& String)
	{
		std::stringstream ss(String);
		T Value;
		ss >> Value;

		if (ss.fail())
			throw InvalidDataException("String cannot be converted to " + std::string(typeid(T).name()) + ".");

		return Value;
	}

	/**
	 * @brief Converts a (numeric) value of type @p T to a std::string using operator<< of std::stringstream.
	 * @tparam T Type of the value to convert
	 * @param Value Value to convert
	 * @param Precision Fixed precision of the numeric conversion for @p Precision >= 0.
	 * For any @p Precision < 0 (default) the precision is not set. 
	 * @return Returns @p Value converted to a string.
	*/
	template <typename T>
	std::string ToStr(const T& Value, int Precision = -1)
	{
		std::stringstream ss;

		if (Precision >= 0)
			ss << std::fixed << std::setprecision(Precision);

		ss << Value;
		return ss.str();
	}

	/**
	 * @brief Converts a time point to a human-readable string describing the time in the current time zone.
	 * @tparam T @p TimePoint's clock type
	 * @param TimePoint Time point to format and to convert
	 * @return Returns @p TimePoint converted to a string.
	*/
	template <typename T>
	std::string ToStr(const std::chrono::time_point<T>& TimePoint)
	{
		std::stringstream ss;
		
		Util::operator<<(ss, TimePoint);
		return ss.str();
	}

	/**
	 * @brief Converts @p Value to a std::string using operator<< of std::stringstream.
	 * @param Value Value to convert
	 * @return Returns @p Value converted to a string.
	*/
	inline std::string ToStr(const char Value) { return ToStr(static_cast<int>(Value)); }

	/**
	 * @copydoc ToStr(const char Value)
	*/
	inline std::string ToStr(const uint8_t Value) { return ToStr(static_cast<int>(Value)); }

	/**
	 * @brief Converts the Qt QString @p Str to a std::string.
	 * @param Str String to convert
	 * @return Returns @p Str converted to a string.
	*/
	inline std::string ToStr(const QString& Str) { return Str.toStdString(); }

	/**
	 * @brief Converts a value of a numeric type to a value of another numeric type checking the conversion for its bounds.
	 * @tparam ToT Numeric type to convert to
	 * @tparam FromT Numeric type to convert from
	 * @param Value Value to be converted
	 * @return Converted value
	 * @throws OutOfRangeException is thrown in case @p Value of type @p FromT does not fit into type @p ToT
	 * when both integral types, @p FromT and @p ToT, are either signed or unsigned.
	 * @throws OverflowException is thrown in case the conversion of @p Value from type @p FromT to type @p ToT would yield an overflow
	 * when one of the types, @p FromT or @p ToT, is signed and the respective other type is unsigned or when @p FromT is non-integral.
	 * @throws UnderflowException is thrown in case the conversion of @p Value from type @p FromT to type @p ToT would yield an underflow
	 * when one of the types, @p FromT or @p ToT, is signed and the respective other type is unsigned or when @p FromT is non-integral.
	*/
	template <typename ToT, typename FromT, std::enable_if_t<
		std::is_integral_v<ToT> && std::is_integral_v<FromT> &&
		std::is_same_v<std::remove_cv_t<ToT>, std::remove_cv_t<FromT>>, int> = 0
	>
	inline ToT NumToT(const FromT Value)
	{
		return Value;
	}

	/**
	 * @overload NumToT
	*/
	template <typename ToT, typename FromT, std::enable_if_t<
		std::is_integral_v<ToT> && std::is_integral_v<FromT> &&
		!std::is_same_v<std::remove_cv_t<ToT>, std::remove_cv_t<FromT>> &&
		((std::is_signed_v<ToT> && std::is_signed_v<FromT>) || (std::is_unsigned_v<ToT> && std::is_unsigned_v<FromT>)), int> = 0
	>
	ToT NumToT(const FromT Value)
	{
		if (Value < std::numeric_limits<ToT>::lowest() || Value > std::numeric_limits<ToT>::max())
			throw OutOfRangeException("Cannot convert Value into destiny type since this would cause an underflow or an overflow.");

		return static_cast<ToT>(Value);
	}

	/**
	 * @overload NumToT
	*/
	template <typename ToT, typename FromT, std::enable_if_t<
		std::is_integral_v<ToT> && std::is_integral_v<FromT> &&
		!std::is_same_v<std::remove_cv_t<ToT>, std::remove_cv_t<FromT>> &&
		std::is_signed_v<ToT> && std::is_unsigned_v<FromT>, int> = 0
	>
	ToT NumToT(const FromT Value)
	{
		if (Value > static_cast<std::make_unsigned_t<ToT>>(std::numeric_limits<ToT>::max()))
			throw OverflowException("Cannot convert Value into destiny type since this would cause an overflow.");

		return static_cast<ToT>(Value);
	}

	/**
	 * @overload NumToT
	*/
	template <typename ToT, typename FromT, std::enable_if_t<
		std::is_integral_v<ToT> && std::is_integral_v<FromT> &&
		!std::is_same_v<std::remove_cv_t<ToT>, std::remove_cv_t<FromT>> &&
		std::is_unsigned_v<ToT> && std::is_signed_v<FromT>, int> = 0
	>
	ToT NumToT(const FromT Value)
	{
		if (Value < 0)
			throw UnderflowException("Cannot convert Value into destiny type since this would cause an underflow.");
		if (static_cast<std::make_unsigned_t<FromT>>(Value) > std::numeric_limits<ToT>::max())
			throw OverflowException("Cannot convert Value into destiny type since this would cause an overflow.");

		return static_cast<ToT>(Value);
	}

	/**
	 * @copydoc NumToT
	*/
	template <typename ToT, std::enable_if_t<
		std::is_integral_v<ToT> &&
		!std::is_same_v<std::remove_cv_t<ToT>, double>, int> = 0
	>
	ToT NumToT(const double Value)
	{
		const double RoundedValue = std::round(Value);

		if (RoundedValue < static_cast<double>(std::numeric_limits<ToT>::lowest()))
			throw UnderflowException("Cannot convert Value into double since this would cause an underflow.");
		if (RoundedValue > static_cast<double>(std::numeric_limits<ToT>::max()))
			throw OverflowException("Cannot convert Value into double since this would cause an overflow.");

		return static_cast<ToT>(RoundedValue);
	}

	using seconds = std::chrono::duration<double>;					//!< Extends std::chrono by a duration data type for seconds capable of storing fractions of seconds.
	using picoseconds = std::chrono::duration<double, std::pico>;	//!< Extends std::chrono by a duration data type for picoseconds.

	/**
	 * @brief Returns a string describing the physical unit associated with type @p T.
	 * For example, if @p T is std::chrono::milliseconds, the function returns the string "ms".
	 * @tparam T Type which has a physical unit associated with
	 * @return String describing the abbreviation of a physical unit
	*/
	template <typename T>
	inline std::string ToUnitStr();

	/**
	 * @copydoc ToUnitStr
	*/
	template <>
	inline std::string ToUnitStr<std::chrono::seconds>()
	{
		return "s";
	}

	/**
	 * @copydoc ToUnitStr
	*/
	template <>
	inline std::string ToUnitStr<std::chrono::milliseconds>()
	{
		return "ms";
	}

	/**
	 * @copydoc ToUnitStr
	*/
	template <>
	inline std::string ToUnitStr<std::chrono::microseconds>()
	{
		return "us";
	}

	/**
	 * @copydoc ToUnitStr
	*/
	template <>
	inline std::string ToUnitStr<std::chrono::nanoseconds>()
	{
		return "ns";
	}

	/**
	 * @copydoc ToUnitStr
	*/
	template <>
	inline std::string ToUnitStr<seconds>()
	{
		return "s";
	}

	/**
	 * @copydoc ToUnitStr
	*/
	template <>
	inline std::string ToUnitStr<picoseconds>()
	{
		return "ps";
	}
	///@}

	/**
	 * @brief Removes trailing zeros ('\\0') from a string
	 * @param Str String to remove zeros from
	 * @return String with trailing zeros removed
	*/
	inline std::string TrimTrailingZeros(const std::string& Str) { return Str.substr(0, Str.find('\0')); }

	/**
	 * @brief Returns a human-readable string describing the current time and date in the current time zone.
	 * @return Time and date string
	*/
	inline auto CurrentTimeAndDateString() { return ToStr(std::chrono::system_clock::now()); }

	/**
	 * @brief Extracts the filename from a path.
	 * @param Path Path ending with a filename
	 * @return Substring of @p Path containing the filename
	*/
	inline auto FilenameFromPath(std::string Path) { return Path.substr(Path.find_last_of("\\") + 1, Path.length() - Path.find_last_of("\\") - 1); }

	/**
	 * @brief Removes the filename's extension from a path.
	 * @param Path Path ending with a filename
	 * @return Substring of @p Path containing the original path and filename but no file extension
	*/
	inline auto RemoveExtFromPath(std::string Path) { return Path.substr(0,  Path.find_last_of(".")); }

	/**
	 * @brief Data type describing %DynExp's program version in the form Major.Minor.Patch
	*/
	struct VersionType
	{
		unsigned int Major{};
		unsigned int Minor{};
		unsigned int Patch{};
	};

	/**
	 * @brief Compares two program version types with each other
	 * @param lhs Left-hand side of the comparison
	 * @param rhs Right-hand side of the comparison
	 * @return Returns a std::strong_ordering object as the result of the comparison.
	*/
	std::strong_ordering operator<=>(const VersionType& lhs, const VersionType& rhs);

	/**
	 * @brief Extracts a program version from a string.
	 * @param Str String containing a programm version in the form specified by @p VersionType
	 * @return @p VersionType object containing the extracted program version
	 * @throws InvalidDataException if @p Str does not contain a valid version number.
	*/
	VersionType VersionFromString(std::string_view Str);

	/**
	 * @brief Converts a program version to a string in the form specified by @p VersionType
	 * @param Version Program version to convert
	 * @return String containing the program version
	*/
	inline std::string ToStr(const VersionType& Version) { return ToStr(Version.Major) + "." + ToStr(Version.Minor) + "." + ToStr(Version.Patch); }

	/**
	 * @brief Parses a string containing comma-separated values (csv) and inserts each row as one tuple containing
	 * column data into a vector of tuples.
	 * @tparam ...Ts Data types of the columns
	 * @param CSVData csv data to parse
	 * @param Delimiter Delimiter character the csv data is separated with
	 * @param SkipLines Amount of header lines (separated by '\\n') to skip at the beginning of @p CSVData
	 * @return Vector of parsed csv data
	*/
	template <typename... Ts>
	std::vector<std::tuple<Ts...>> ParseCSV(const std::string& CSVData, const char Delimiter = ';', const size_t SkipLines = 0)
	{
		std::vector<std::tuple<Ts...>> ParsedLines;
		std::istringstream CSVDataStream(CSVData);

		// Ignore header lines
		for (auto i = SkipLines; i > 0; --i)
			CSVDataStream.ignore(std::numeric_limits<std::streamsize>::max(), CSVDataStream.widen('\n'));

		// Function to parse one field from a single line
		const auto GetValue = [Delimiter]<typename T>(std::istringstream& LineStream) {
			std::string ValueStr;
			std::getline(LineStream, ValueStr, Delimiter);
			std::istringstream ValueStream(ValueStr);
			ValueStream.exceptions(std::istringstream::failbit | std::istringstream::badbit);

			T Value;
			ValueStream >> Value;

			return Value;
		};

		// Loop through each line and fill ParsedLines with tuples of column data.
		std::string Line;
		while (std::getline(CSVDataStream, Line))
		{
			std::istringstream LineStream(Line);
			LineStream.exceptions(std::istringstream::failbit | std::istringstream::badbit);

			// Braced initialization to ensure correct evaluation order (left to right). Refer to
			// https://stackoverflow.com/questions/14056000/how-to-avoid-undefined-execution-order-for-the-constructors-when-using-stdmake
			ParsedLines.push_back({ GetValue.template operator()<Ts>(LineStream)... });
		}

		return ParsedLines;
	}

	/**
	 * @brief Returns the what() information of an exception derived from std::exception and stored in an exception pointer.
	 * @param ExceptionPtr Pointer to an exception derived from std::exception.
	 * @return std::exception::what() if @p ExceptionPtr contains an exception. Empty string if @p ExceptionPtr does not
	 * contain an exception. Placeholder string if the exception stored in @p ExceptionPtr is not derived from std::exception.
	*/
	std::string ExceptionToStr(const std::exception_ptr ExceptionPtr);

	/**
	 * @brief Transforms a string into lower case.
	 * @param Str String to transform
	 * @return Copy of the content of @p Str transformed to lower case
	*/
	std::string ToLower(std::string_view Str);

	/**
	 * @brief Computes the Fast Fourier Transform (FFT) a vector of complex values.
	 * @param Data Vector of complex values to be transformed
	 * @param InverseTransform If true, the inverse FFT is computed. False is default.
	 * @return Vector of complex values containing the computed FFT
	 * @throws OverflowException if @p Data contains more items than half of the maximal value @p size_t can represent.
	 * @throws NotAvailableException if GSL functions fail reserving memory.
	 * @throws InvalidDataException if GSL fails to perform the FFT on @p Data.
	*/
	std::vector<std::complex<double>> FFT(const std::vector<std::complex<double>>& Data, bool InverseTransform = false);

	/**
	 * @brief Class to store information about warnings in a thread-safe manner (deriving from @p ILockable).
	 * All function calls are thread-safe.
	*/
	class Warning : public ILockable
	{
	public:
		/**
		 * @brief Data associated with a warning.
		 * The class is convertible to bool (true if it describes an error/warning, false otherwise).
		*/
		struct WarningData
		{
			/**
			 * @brief Default constructor sets @p ErrorCode to a non-error code.
			*/
			WarningData() : ErrorCode(DynExpErrorCodes::NoError), Line(0) {}

			WarningData(std::string Description, const int ErrorCode = DynExpErrorCodes::GeneralError,
				const std::source_location Location = std::source_location::current())
				: Description(std::move(Description)), ErrorCode(ErrorCode),
				Line(Location.line()), Function(Location.function_name()), File(Location.file_name()) {}
			WarningData(std::string Description, const int ErrorCode = DynExpErrorCodes::GeneralError,
				const size_t Line = 0, std::string Function = "", std::string File = "")
				: Description(std::move(Description)), ErrorCode(ErrorCode),
				Line(Line), Function(std::move(Function)), File(std::move(File)) {}

			explicit operator bool() const noexcept { return ErrorCode != DynExpErrorCodes::NoError; }

			const std::string Description;	//!< String describing the reason and consequences of the warning
			const int ErrorCode;			//!< %DynExp error code from DynExpErrorCodes::DynExpErrorCodes
			const size_t Line;				//!< Line in source code where the warning occurred
			const std::string Function;		//!< Function in source code where the warning occurred
			const std::string File;			//!< Source code file where the warning occurred
		};

		/**
		 * @brief Constructs an empty Warning.
		*/
		Warning() : Data(std::make_unique<WarningData>()) {}

		/**
		 * @brief Constructs a Warning from specified information.
		 * @param Description String describing the reason and consequences of the warning
		 * @param ErrorCode %DynExp error code from DynExpErrorCodes::DynExpErrorCodes
		 * @param Location Origin of the warning. Do not pass anything except when deriving from this class.
		*/
		Warning(std::string Description, const int ErrorCode = DynExpErrorCodes::GeneralError,
			const std::source_location Location = std::source_location::current())
			: Data(std::make_unique<WarningData>(std::move(Description), ErrorCode, Location)) {}

		/**
		 * @brief Constructs a Warning retrieving the warning data from an exception @p e.
		 * @param e Exception derived from class @p Exception
		*/
		Warning(const Exception& e)
			: Data(std::make_unique<WarningData>(e.what(), e.ErrorCode, e.Line, e.Function, e.File)) {}

		/**
		 * @brief Constructs a Warning moving @p Other's warning data to this instance clearing @p Other's warning data.
		 * @param Other Warning to move from
		*/
		Warning(Warning&& Other) noexcept;

		virtual ~Warning() = default;

		void Reset();									//!< Clears the warning data.

		Warning& operator=(const Exception& e);			//!< Retrives the warning data from an exception @p e derived from Exception.
		Warning& operator=(Warning&& Other) noexcept;	//!< Swaps in the warning data of this instance with @p Other's warning data.

		WarningData Get() const;						//!< Returns a copy of the warning data.

	private:
		/**
		 * @brief Pointer to warning data. Must never be nullptr.
		*/
		std::unique_ptr<WarningData> Data;
	};

	/**
	 * @brief Data type of a single entry in %DynExp's log
	*/
	struct LogEntry
	{
		LogEntry(std::string Message, ErrorType Type, std::chrono::system_clock::time_point TimePoint)
			: Message(std::move(Message)), Type(Type), TimePoint(TimePoint) {}

		const std::string Message;								//!< String describing the log entry including reasons and consequences of the message
		const ErrorType Type;									//!< %DynExp error code from DynExpErrorCodes::DynExpErrorCodes associated with the log enty
		const std::chrono::system_clock::time_point TimePoint;	//!< Time point associated with the log enty
	};

	/**
	 * @brief Logs events like errors and writes them immediately to a HTML file in a human-readable format.
	 * The logger also stores the events in an internal event log to be displayed within %DynExp.
	 * The class is designed such that instances can be shared between different threads. Member function
	 * calls are synchronized.
	*/
	class EventLogger : public ILockable
	{
	public:
		/**
		 * @brief Constructs the event logger without opening a log file on disk. Events are only
		 * stored in the internal log until OpenLogFile() is called to open a log file on disk.
		*/
		EventLogger();

		/**
		 * @brief Constructs the event logger with opening a log file on disk.
		 * @param Filename Name and path of the HTML log file to write the events to
		*/
		EventLogger(std::string Filename) : EventLogger() { OpenLogFile(Filename); }

		/**
		 * @brief Destructor closes the log file on disk.
		*/
		~EventLogger() { CloseLogFileUnsafe(); }

		/** @name Noexcept logging
		 * These functions do not throw exceptions. Instead, they do not perform logging if an exception occurs.
		 * This ensures that catching and logging exceptions does not possibly cause itself an exception which
		 * propagates through functions meant to be noexcept by catching and logging exceptions occurring within.
		*/
		///@{
		/**
		 * @brief Logs an event from information specified manually.
		 * @param Message Message string describing the reason and consequences of the message
		 * @param Type %DynExp error type from Util::ErrorType
		 * @param Line Line in source code where the message occurred
		 * @param Function Function in source code where the message occurred
		 * @param File Source code file where the message occurred
		 * @param ErrorCode %DynExp error code from DynExpErrorCodes::DynExpErrorCodes
		 * @param Trace Stack trace object created where the message occurred.
		*/
		void Log(const std::string& Message, const ErrorType Type = ErrorType::Info,
			const size_t Line = 0, const std::string& Function = "", const std::string& File = "", const int ErrorCode = 0
#ifdef DYNEXP_HAS_STACKTRACE
			, const std::stacktrace& Trace = {}
#endif // DYNEXP_HAS_STACKTRACE
		) noexcept;

		/**
		 * @brief Logs a %DynExp exception.
		 * @param E Exception to log
		*/
		void Log(const Exception& E) noexcept;

		/**
		 * @brief Logs a warning.
		 * @param W Warning to log
		*/
		void Log(const Warning& W) noexcept;
		///@}

		/**
		 * @brief Formats a log entry as plain text to be displayed within %DynExp.
		 * @param Message Message string describing the reason and consequences of the message
		 * @param Line Line in source code where the message occurred
		 * @param Function Function in source code where the message occurred
		 * @param Filename Source code file where the message occurred
		 * @param ErrorCode %DynExp error code from DynExpErrorCodes::DynExpErrorCodes
		 * @param PrefixMessage If true or @p Filename or @p Function are not empty, @p Message is prepended by a colon.
		 * @return Returns the formatted log entry.
		*/
		static std::string FormatLog(const std::string& Message, const size_t Line = 0,
			const std::string& Function = "", const std::string& Filename = "", const int ErrorCode = 0, const bool PrefixMessage = true);
		
		/**
		 * @brief Formats a log entry as HTML code to be displayed in a web browser.
		 * @param Message Message string describing the reason and consequences of the message
		 * @param Type %DynExp error type from Util::ErrorType
		 * @param Line Line in source code where the message occurred
		 * @param Function Function in source code where the message occurred
		 * @param Filename Source code file where the message occurred
		 * @param ErrorCode %DynExp error code from DynExpErrorCodes::DynExpErrorCodes
		 * @param Trace Stack trace object created where the message occurred.
		 * If it contains entries, they are displayed in an accordion-like style.
		 * @return Returns the formatted log entry.
		*/
		static std::string FormatLogHTML(const std::string& Message, const ErrorType Type = ErrorType::Info,
			const size_t Line = 0, const std::string& Function = "", const std::string& Filename = "", const int ErrorCode = 0
#ifdef DYNEXP_HAS_STACKTRACE
			, const std::stacktrace& Trace = {}
#endif // DYNEXP_HAS_STACKTRACE
		);

		/**
		 * @brief Opens the HTML log file on disk. If it already exists, the file gets overwritten.
		 * @param Filename Name and path of the HTML log file to write the events to.
		*/
		void OpenLogFile(std::string Filename);

		/**
		 * @brief Closes the log file on disk and writes terminating HTML tags.
		*/
		void CloseLogFile() { auto lock = AcquireLock(LogOperationTimeout); CloseLogFileUnsafe(); }

		/**
		 * @brief Determines whether the log file has been openend on disk.
		 * @return True if the log file is opened, false otherwise.
		*/
		bool IsOpen() const { auto lock = AcquireLock(LogOperationTimeout); return IsOpenUnsafe(); }

		/**
		 * @brief Determines the full file path to the currently openend log file.
		 * @return Path to the openend log file. Empty string if the log file is not opened.
		*/
		std::string GetLogFilename() const { auto lock = AcquireLock(LogOperationTimeout); return Filename; }

		/**
		 * @brief Clears the internal event log.
		*/
		void ClearLog() { auto lock = AcquireLock(LogOperationTimeout); ClearLogUnsafe(); }

		/**
		 * @brief Returns the internal event log starting from the n-th stored element
		 * @param FirstElement n-th element from which the returned event log starts
		 * @return Event log as a vector of log entries of type Util::LogEntry
		 * @throws OutOfRangeException is thrown if @p FirstElement exceeds the log size.
		*/
		std::vector<LogEntry> GetLog(size_t FirstElement = 0) const;

		/**
		 * @brief Determines the number of entries in the internal event log.
		 * @return Number of events stored in the event log
		*/
		auto GetLogSize() const { auto lock = AcquireLock(LogOperationTimeout); return LogEntries.size(); }

	private:
		/** @name Not thread-safe
		 * These functions are not thread-safe. To be called from thread-safe functions only.
		 * Thread-safe functions must only call unsafe (private) functions.
		*/
		///@{
		bool IsOpenUnsafe() const { return LogFile.is_open(); }
		void CloseLogFileUnsafe();
		void ClearLogUnsafe() { LogEntries.clear(); }
		///@}

		/**
		 * @brief Internal timeout for locking the mutex which synchronizes the calls to member function calls.
		 * If the timeout is exceeded while locking the mutex to log an event, the event is not logged.
		*/
		static constexpr auto LogOperationTimeout = std::chrono::milliseconds(100);

		std::ofstream LogFile;				//!< Stream object to write to the log file on disk
		std::string Filename;				//!< Filename and path to the log file on disk

		std::vector<LogEntry> LogEntries;	//!< Internally stored log entries
	};

	/**
	 * @brief This function holds a static EventLogger instance and returns a reference to it.
	 * %DynExp uses only one EventLogger instance to log events from any thread. A local static
	 * object instead of a global object is employed to avoid initialization order problems.
	 * @return Reference to %DynExp's unique EventLogger instance
	*/
	EventLogger& EventLog();

	/**
	 * @brief Holds a bitset containing flags to indicate which features a certain instrument/ module etc.
	 * supports. 
	 * @tparam EnumType Enum class type containing flags the respective FeatureTester operates on.
	 * @p NUM_ELEMENTS must be the last element in EnumType. No values must be assigned to the enum's elements!
	*/
	template <typename EnumType, std::enable_if_t<
		std::is_enum_v<EnumType>, int> = 0
	>
	class FeatureTester
	{
	public:
		/**
		 * @brief Constructs a FeatureTester instance with no flags set.
		*/
		constexpr FeatureTester() noexcept = default;

		/**
		 * @brief Constructs a FeatureTester instance with the Flags passed as an array being set
		 * @tparam N Amount of flags contained in @p Flags. Automatically derived. Do not specify.
		 * @param Flags Flags to set
		*/
		template <size_t N>
		FeatureTester(const std::array<EnumType, N>& Flags)
		{
			for (const auto Flag : Flags)
				Set(Flag);
		}

		/**
		 * @brief Tests whether all of the flags passed as an array are set.
		 * @tparam N Amount of flags contained in @p Flags. Automatically derived. Do not specify.
		 * @param Flags Flags Flags to check
		 * @return Returns true if all flags contained in @p Flags are set, false otherwise.
		*/
		template <size_t N>
		bool Test(const std::array<EnumType, N>& Flags) const
		{
			for (const auto Flag : Flags)
			{
				auto Result = Features.test(static_cast<size_t>(Flag));

				if (!Result)
					false;
			}

			return true;
		}

		/**
		 * @brief Tests whether a single flag is set.
		 * @param Flag Flag to be checked
		 * @return Returns true if the flag is set, false otherwise.
		*/
		bool Test(EnumType Flag) const { return Features.test(static_cast<size_t>(Flag)); }

		/**
		 * @brief Sets a flag.
		 * @param Flag Flag to set
		*/
		void Set(EnumType Flag) { Features.set(static_cast<size_t>(Flag)); }

	private:
		/**
		 * @brief Bitset containing the flags and their states
		*/
		std::bitset<static_cast<size_t>(EnumType::NUM_ELEMENTS)> Features;
	};

	/**
	 * @brief State machine state as used by class StateMachine. A state mainly wraps a state function
	 * of the member function pointer type CallableT to be invoked by the state machine. 
	 * @tparam CallableT Type of the state function associated with the state machine state.
	 * The state function is expected to be a member function of the class also managing the state machine.
	 * It can accept an arbitrary amount of arguments (such as a reference to DynExp::ModuleInstance).
	 * Furthermore, it is expected to return a value of type @p StateEnumType as the identifier of the
	 * next state machine state. @p StateEnumType is an enum class type of elements being identifiers
	 * of all states associated with a state machine.
	*/
	template <
		typename CallableT,
		std::enable_if_t<std::is_enum_v<return_of_t<CallableT>>, int> = 0
	>
	class StateMachineState
	{
	public:
		using StateEnumType = return_of_t<CallableT>;
		using CallableType = CallableT;

		/**
		 * @brief Constructs a state machine state and assigns fixed parameters to it.
		 * @param State Unique identifier of the state
		 * @param StateFunction Function pointer to the state's state function
		 * @param Description Human-readable description of what the state does
		 * @param IsFinal Indicates whether this is a final state. Refer to field StateMachineState::Final.
		*/
		constexpr StateMachineState(StateEnumType State, CallableT StateFunction, const char* Description = "", const bool IsFinal = false) noexcept
			: State(State), StateFunction(StateFunction), Description(Description), Final(IsFinal)
		{}

		constexpr StateEnumType GetState() const noexcept { return State; }		//!< Returns the state's unique identifier.
		constexpr auto GetDescription() const noexcept { return Description; }	//!< Returns the state's description.
		constexpr bool IsFinal() const noexcept { return Final; }				//!< Returns whether this is a final state.

		/**
		 * @brief Invokes the state function associated with this state on an
		 * instance of the class the state function is a member functions of.
		 * @tparam ...ArgTs Type of the parameter pack to forward to the state function
		 * @param Instance Instance to invoke the state function on
		 * @param ...Args Parameters to forward to the state function
		 * @return Returns the identifer of the next state machine state to transition into.
		*/
		template <typename... ArgTs>
		StateEnumType Invoke(instance_of_t<CallableT>& Instance, ArgTs&&... Args) const
		{
			return (Instance.*StateFunction)(std::forward<ArgTs>(Args)...);
		}
		
	private:
		const StateEnumType State;
		const std::decay_t<CallableT> StateFunction;
		const char* Description;

		/**
		 * @brief For final states, it is ensured that the state's state function can delete the state machine.
		 * No memory assigned to the state machine will be accessed after the call to Invoke().
		*/
		const bool Final;
	};

	/**
	 * @brief State machine context as used by class StateMachine. A state machine context holds
	 * a map with keys and values of unique state identifiers of type StateEnumType (refer to class StateMachineState).
	 * Each map entry indicates a state (key) to be replaced by another state (value). StateMachine::Invoke
	 * checks the state identifiers returned by StateMachineState::Invoke and possibly replaces them
	 * according to the current context before setting the new state machine state.
	 * Contexts can be derived from base contexts unifying and including their replacement lists.
	 * @tparam StateMachineStateT State type of type StateMachineState the state machine operates on
	*/
	template <typename StateMachineStateT>
	class StateMachineContext
	{
	public:
		using StateType = StateMachineStateT;
		using StateEnumType = typename StateType::StateEnumType;						//!< Refer to class StateMachineState.
		using ReplacementListType = std::unordered_map<StateEnumType, StateEnumType>;

		/**
		 * @brief Default constructor constructs an empty context not performing any state replacement.
		*/
		StateMachineContext() = default;

		/**
		 * @brief Constructs a StateMachineContext from a replacement list appending the replacement lists of each
		 * context in @p BaseContexts
		 * @param ReplacementList Map containing the @p StateType to replace (keys) with other state types (values)
		 * @param Description Human-readable description of what the context does
		 * @param BaseContexts List of pointers to other state machine contexts this context should be derived from.
		 * If this context's replacement list already contains a specific key to replace, further tuples with the
		 * same key are ignored and not inserted. Thus, base contexts should be passed as a list ordered in
		 * descending priority.
		*/
		StateMachineContext(ReplacementListType&& ReplacementList, const char* Description = "",
			std::initializer_list<const StateMachineContext*> BaseContexts = {})
			: ReplacementList(std::move(ReplacementList)), Description(Description)
		{
			for (auto BaseContext : BaseContexts)
				if (BaseContext)
					this->ReplacementList.insert(BaseContext->ReplacementList.cbegin(), BaseContext->ReplacementList.cend());
		}

		/**
		 * @brief Returns the context's description.
		 * @return Context's description
		*/
		constexpr auto GetDescription() const noexcept { return Description; }

		/**
		 * @brief Checks whether the context contains a replacement entry for the state identified by @p State
		 * and returns the identifier of the state to replace @p State with.
		 * @param State State identifier to check for replacement
		 * @return Returns a replacement of @p State or @p State itself if there is no replacement registered.
		*/
		StateEnumType AdaptState(StateEnumType State) const
		{
			auto AdaptedState = ReplacementList.find(State);

			return AdaptedState == ReplacementList.cend() ? State : AdaptedState->second;
		}

	private:
		/**
		 * @brief Within this context, the map's key states are replaced by the corresponding value states.
		 * The list must not be modified after the StateMachineContext's constructor has run.
		*/
		ReplacementListType ReplacementList;

		const char* Description;
	};

	/**
	 * @brief This class models a state machine. It keeps track of the current state and allows
	 * to invoke its associated state function. The return value of the state function determines
	 * the new state to transition into. Additionally, a context of type StateMachineContext
	 * can be assigned to the state machine. Contexts allow for replacing states with other states.
	 * This makes sense to write inner protocols BeginState -> StateA -> ... -> StateN -> EndState and
	 * to embed them into outer protocols. The outer protocol can transition into BeginState and take
	 * control back by replacing EndState with one of its own states by setting a respective context.
	 * All states (of type StateMachineState) have to be registered upon construction of the
	 * StateMachine instance. 
	 * @tparam StateMachineStateT State type of type StateMachineState the state machine operates on
	*/
	template <typename StateMachineStateT>
	class StateMachine
	{
	public:
		using StateType = StateMachineStateT;
		using StateEnumType = typename StateType::StateEnumType;					//!< Refer to class StateMachineState.
		using ContextType = StateMachineContext<StateMachineStateT>;

		/**
		 * @brief Constructs a state machine assigning possible states to it.
		 * Automatically also adds @p InitialState, so don't include it into @p States
		 * @tparam ...StateMachineStateTs Type of the @p States parameter pack
		 * @param InitialState Initial state of the state machine. 
		 * @param ...States Possible states the state machine can transition into.
		 * Pass states by reference to prevent from null pointers.
		*/
		template <typename... StateMachineStateTs>
		StateMachine(const StateType& InitialState, const StateMachineStateTs&... States)
			: StatesList{ { InitialState.GetState(), &InitialState }, { States.GetState(), &States }... },
			CurrentState(&InitialState), CurrentContext(nullptr)
		{}

		const StateType* GetCurrentState() const noexcept { return CurrentState; }	//!< Returns a pointer to the current state.
		const ContextType* GetContext() const noexcept { return CurrentContext; }	//!< Returns a pointer to the current context.

		/**
		 * @brief Sets the current state as identified by an element from @p StateEnumType
		 * @param NewState Identifier of the new state machine state
		 * @throws throw std::out_of_range is thrown if @p NewState does not exist in @p StatesList.
		*/
		void SetCurrentState(StateEnumType NewState)
		{
			if (CurrentContext)
				CurrentState = StatesList.at(CurrentContext.load()->AdaptState(NewState));
			else
				CurrentState = StatesList.at(NewState);
		}

		/**
		 * @brief Sets the current state machine context.
		 * @param NewContext Context to set
		*/
		void SetContext(const ContextType* NewContext) { CurrentContext = NewContext; }

		/**
		 * @brief Removes the current state machine context.
		*/
		void ResetContext() { CurrentContext = nullptr; }

		/**
		 * @brief Invokes the state function associated with the current state machine state on an
		 * instance of the class the state functions are member functions of. Refer to class StateMachineState.
		 * Also sets the state machine's state to the return value of the state function.
		 * @tparam ...ArgTs Type of the parameter pack to forward to the state function
		 * @param Instance Instance to invoke the state function on
		 * @param ...Args Parameters to forward to the state function
		 * @throws InvalidStateException is thrown if the current state is nullptr.
		*/
		template <typename... ArgTs>
		void Invoke(instance_of_t<typename StateType::CallableType>& Instance, ArgTs&&... Args)
		{
			if (!CurrentState)
				throw InvalidStateException("CurrentState must not be nullptr in order to be invoked.");

			if (CurrentState.load()->IsFinal())
				CurrentState.load()->Invoke(Instance, std::forward<ArgTs>(Args)...);
			else
				SetCurrentState(CurrentState.load()->Invoke(Instance, std::forward<ArgTs>(Args)...));
		}

	private:
		/**
		 * @brief Map of possible states. All states are uniquely identified by an element from @p StateEnumType.
		 * Refer to StateMachineState.
		*/
		const std::unordered_map<StateEnumType, const StateType*> StatesList;

		/** @name Atomic members for thread-safety 
		 * Atomic to allow reading the current state and context from one thread and setting the current
		 * state and context from another thread without further synchronization.
		*/
		///@{
		std::atomic<const StateType*> CurrentState;
		std::atomic<const ContextType*> CurrentContext;
		///@}
	};
}