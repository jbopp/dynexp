// This file is part of DynExp.

/**
 * @file Exception.h
 * @brief Provides exception type and error definitions used by %DynExp.
*/

#pragma once

namespace Util
{
	/**
	 * @brief %DynExp's error types
	*/
	enum class ErrorType { Info, Warning, Error, Fatal };

	namespace DynExpErrorCodes
	{
		/**
		 * @brief %DynExp's error codes
		*/
		enum DynExpErrorCodes : int {
			NoError = 0,
			GeneralError = -1,
			InvalidArg = -2,
			InvalidState = -3,
			InvalidData = -4,
			InvalidCall = -5,
			Underflow = -6,
			Overflow = -7,
			OutOfRange = -8,
			Empty = -9,
			NotFound = -10,
			TypeError = -11,
			Timeout = -12,
			ThreadDidNotRespond = -13,
			NotAvailable = -14,
			NotImplemented = -15,
			FileIOError = -16,
			LinkedObjectNotLocked = -17,
			InvalidObjectLink = -18,
			ServiceFailed = -19
		};
	}

	/**
	 * @brief %DynExp exceptions are derived from this class. It contains basic
	 * information about the cause of the exception as well as a stacktrace.
	*/
	class Exception : public std::runtime_error
	{
	public:
		/**
		 * @brief Constructs an exception. Constructor is noexcept, although std::runtime_error() might throw
		 * std::bad_alloc issuing a call to std::terminate(), which might be the best solution in that case.
		 * @param Description Description explaining the cause of the exception in detail for the user.
		 * @param Type Error type from @p Util::ErrorType
		 * @param ErrorCode Error code from @p DynExpErrorCodes
		 * @param Location Used to derive the origin of the exception. Pass the result of
		 * std::source_location::current() as called by the constructor of the derived classes.
		*/
		Exception(std::string Description, const ErrorType Type = ErrorType::Error, const int ErrorCode = -1,
			const std::source_location Location = std::source_location::current()) noexcept;
		virtual ~Exception() = default;

		/**
		 * @brief Converts an error type to a user-readable label for logging.
		 * @param Type Error type from @p Util::ErrorType
		 * @return String describing the error type 
		*/
		constexpr static const char* GetErrorLabel(const ErrorType Type)
		{
			switch (Type)
			{
			case ErrorType::Warning: return "Warning";
			case ErrorType::Error: return "Error";
			case ErrorType::Fatal: return "Fatal Error";
			default: return "Info";
			}
		}

		/**
		 * @brief Converts an error type to an HTML color name for logging.
		 * @param Type Error type from @p Util::ErrorType
		 * @return HTML color name matching the error type
		*/
		constexpr static const char* GetErrorLabelColor(const ErrorType Type)
		{
			switch (Type)
			{
			case ErrorType::Warning: return "Orange";
			case ErrorType::Error: return "Red";
			case ErrorType::Fatal: return "DarkRed";
			default: return "Blue";
			}
		}

		constexpr const char* GetErrorLabel() const { return GetErrorLabel(Type); }
		constexpr const char* GetErrorLabelColor() const { return GetErrorLabelColor(Type); }

#ifdef DYNEXP_HAS_STACKTRACE
		auto& GetStackTrace() const { return Trace; }
#endif // DYNEXP_HAS_STACKTRACE

		const ErrorType Type;			//!< %DynExp error type from Util::ErrorType
		const int ErrorCode;			//!< %DynExp error code from DynExpErrorCodes::DynExpErrorCodes
		const size_t Line;				//!< Line in source code where the exception occurred
		const std::string Function;		//!< Function in source code where the exception occurred	
		const std::string File;			//!< Source code file where the exception occurred

	private:
#ifdef DYNEXP_HAS_STACKTRACE
		/**
		 * @brief Stack trace initialized in Exception's constructor, thus containing the origin of the exception.
		*/
		const std::stacktrace Trace;
#endif // DYNEXP_HAS_STACKTRACE
	};

	/**
	 * @brief Class to forward an @p Exception instance from one @p DynExp::Object
	 * instance to another @p DynExp::Object instance.
	 * Refer to class @p DynExp::LinkedObjectWrapperContainer.
	*/
	class ForwardedException : public Exception
	{
	public:
		ForwardedException() noexcept : Exception("Unknown forwarded exception") {}
		ForwardedException(const std::exception& e) noexcept : Exception(e.what()) {}
		ForwardedException(const Exception& e) noexcept : Exception(e) {}
	};

	/**
	 * @brief An invalid argument like a null pointer has been passed to a function. 
	*/
	class InvalidArgException : public Exception
	{
	public:
		InvalidArgException(std::string Description,
			const std::source_location Location = std::source_location::current()) noexcept
			: Exception(std::move(Description), ErrorType::Error, DynExpErrorCodes::InvalidArg, Location)
		{}
	};

	/**
	 * @brief An operation cannot be performed currently since the related object is in an invalid state
	 * like an error state.
	*/
	class InvalidStateException : public Exception
	{
	public:
		InvalidStateException(std::string Description, const int ErrorCode = DynExpErrorCodes::InvalidState,
			const std::source_location Location = std::source_location::current()) noexcept
			: Exception(std::move(Description), ErrorType::Error, ErrorCode, Location)
		{}
	};

	/**
	 * @brief Data to operate on is invalid for a specific purpose. This indicates a corrupted data structure
	 * or function calls not returning data in the expected format.
	*/
	class InvalidDataException : public Exception
	{
	public:
		InvalidDataException(std::string Description,
			const std::source_location Location = std::source_location::current()) noexcept
			: Exception(std::move(Description), ErrorType::Error, DynExpErrorCodes::InvalidData, Location)
		{}
	};

	/**
	 * @brief Thrown when a function call is not allowed to a specific thread in a multi-threading context.
	*/
	class InvalidCallException : public Exception
	{
	public:
		InvalidCallException(std::string Description,
			const std::source_location Location = std::source_location::current()) noexcept
			: Exception(std::move(Description), ErrorType::Error, DynExpErrorCodes::InvalidCall, Location)
		{}
	};

	/**
	 * @brief Thrown when a numeric operation would result in an underflow (e.g. due to incompatible data types)
	*/
	class UnderflowException : public Exception
	{
	public:
		UnderflowException(std::string Description,
			const std::source_location Location = std::source_location::current()) noexcept
			: Exception(std::move(Description), ErrorType::Error, DynExpErrorCodes::Underflow, Location)
		{}
	};

	/**
	 * @brief Thrown when a numeric operation would result in an overflow (e.g. due to incompatible data types)
	*/
	class OverflowException : public Exception
	{
	public:
		OverflowException(std::string Description,
			const std::source_location Location = std::source_location::current()) noexcept
			: Exception(std::move(Description), ErrorType::Error, DynExpErrorCodes::Overflow, Location)
		{}
	};

	/**
	 * @brief Thrown when an argument passed to a function exceeds the valid range.
	*/
	class OutOfRangeException : public Exception
	{
	public:
		OutOfRangeException(std::string Description,
			const std::source_location Location = std::source_location::current()) noexcept
			: Exception(std::move(Description), ErrorType::Error, DynExpErrorCodes::OutOfRange, Location)
		{}
	};

	/**
	 * @brief Thrown when a list is expected to contain entries and when a query results in an empty answer
	 * or an empty argument has been passed to a function, or a stored list is empty.
	*/
	class EmptyException : public Exception
	{
	public:
		EmptyException(std::string Description,
			const std::source_location Location = std::source_location::current()) noexcept
			: Exception(std::move(Description), ErrorType::Error, DynExpErrorCodes::Empty, Location)
		{}
	};

	/**
	 * @brief Thrown when a requested ressource does not exist.
	*/
	class NotFoundException : public Exception
	{
	public:
		NotFoundException(std::string Description,
			const std::source_location Location = std::source_location::current()) noexcept
			: Exception(std::move(Description), ErrorType::Error, DynExpErrorCodes::NotFound, Location)
		{}
	};

	/**
	 * @brief Thrown when an attempt was made to convert two incompatible types into each other.
	*/
	class TypeErrorException : public Exception
	{
	public:
		TypeErrorException(const std::source_location Location = std::source_location::current()) noexcept
			: Exception("A base pointer did not belong to the expected derived type.",
				ErrorType::Error, DynExpErrorCodes::TypeError, Location)
		{}
	};

	/**
	 * @brief Thrown when an operation timed out before it could be completed, especially used for locking
	 * shared data in a multi-threading context.
	*/
	class TimeoutException : public Exception
	{
	public:
		TimeoutException(std::string Description, const ErrorType Type = ErrorType::Warning, const int ErrorCode = DynExpErrorCodes::Timeout,
			const std::source_location Location = std::source_location::current()) noexcept
			: Exception(std::move(Description), Type, ErrorCode, Location)
		{}
	};

	/**
	 * @brief Thrown in a multi-threading context when an answer is expected from another thread an when
	 * the communication timed out.
	*/
	class ThreadDidNotRespondException : public TimeoutException
	{
	public:
		ThreadDidNotRespondException(const std::source_location Location = std::source_location::current()) noexcept
			: TimeoutException("Timeout occurred. A thread did not respond. Retry or terminate the program.",
				ErrorType::Error, DynExpErrorCodes::ThreadDidNotRespond, Location)
		{}
	};

	/**
	 * @brief Thrown when some operation or feature is temporarily or permanently not available.
	*/
	class NotAvailableException : public Exception
	{
	public:
		NotAvailableException(std::string Description, const ErrorType Type = ErrorType::Warning,
			const std::source_location Location = std::source_location::current()) noexcept
			: Exception(std::move(Description), Type, DynExpErrorCodes::NotAvailable, Location)
		{}
	};

	/**
	 * @brief Thrown when a requested feature is either under development and thus not implemented yet
	 * or when a specific instrument does not support certain operation.
	*/
	class NotImplementedException : public Exception
	{
	public:
		NotImplementedException(const std::source_location Location = std::source_location::current()) noexcept
			: Exception("This function has not been implemented for the given item type.",
				ErrorType::Error, DynExpErrorCodes::NotImplemented, Location)
		{}
		NotImplementedException(std::string Description,
			const std::source_location Location = std::source_location::current()) noexcept
			: Exception(std::move(Description), ErrorType::Error, DynExpErrorCodes::NotImplemented, Location)
		{}
	};

	/**
	 * @brief Thrown when reading from or writing to a file failed.
	*/
	class FileIOErrorException : public Exception
	{
	public:
		FileIOErrorException(std::string Filename,
			const std::source_location Location = std::source_location::current()) noexcept
			: Exception("Error reading from or writing to file \"" + Filename + "\".",
				ErrorType::Error, DynExpErrorCodes::FileIOError, Location)
		{}
	};

	/**
	 * @brief Thrown when RunnableInstance::LockObject() has not been called on an object link parameter
	 * to establish the relation between the object using another object and the used object.
	*/
	class LinkedObjectNotLockedException : public InvalidStateException
	{
	public:
		LinkedObjectNotLockedException(const std::source_location Location = std::source_location::current()) noexcept
			: InvalidStateException(
				"A linked object wrapper container is empty. Probably, the respective object link parameter has not been locked calling RunnableInstance::LockObject().",
				DynExpErrorCodes::LinkedObjectNotLocked, Location)
		{}
	};

	/**
	 * @brief Thrown when RunnableInstance cannot lock an object to be used by another object due to an
	 * invalid link between the objects. This happens when the used object the link points to has been
	 * removed from the project.
	*/
	class InvalidObjectLinkException : public InvalidStateException
	{
	public:
		InvalidObjectLinkException(const std::source_location Location = std::source_location::current()) noexcept
			: InvalidStateException(
				"An object link does not point to a valid destination. Probably, an object referenced by an object link parameter has been removed from the project.",
				DynExpErrorCodes::InvalidObjectLink, Location)
		{}
	};

	/**
	 * @brief Denotes that e.g. a remote gRPC service failed.
	*/
	class ServiceFailedException : public Exception
	{
	public:
		ServiceFailedException(std::string Description, const ErrorType Type = ErrorType::Error,
			const std::source_location Location = std::source_location::current()) noexcept
			: Exception(std::move(Description), Type, DynExpErrorCodes::ServiceFailed, Location)
		{}
	};

	/**
	 * @brief Writes a %DynExp exception in a user-readable way to a stream.
	 * @param stream Stream to write to
	 * @param e Exception to be logged to the stream
	 * @return Returns the same stream which was passed to the operator.
	*/
	std::ostream& operator<<(std::ostream& stream, const Exception& e);

	/**
	 * @brief Wraps the exception passed to the function in a ForwardedException
	 * and throws the ForwardedException. Does nothing if e is not an exception.
	 * @param e Pointer to an exception
	*/
	void ForwardException(std::exception_ptr e);
}