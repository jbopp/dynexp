#include "stdafx.h"
#include "Exception.h"

// This file is part of DynExp.

namespace Util
{
	Exception::Exception(std::string Description, const ErrorType Type, const int ErrorCode, const std::source_location Location) noexcept
		: std::runtime_error(std::move(Description)), Type(Type), ErrorCode(ErrorCode),
		Line(Location.line()), Function(Location.function_name()), File(Location.file_name())
#ifdef DYNEXP_HAS_STACKTRACE
		, Trace(std::stacktrace::current())
#endif // DYNEXP_HAS_STACKTRACE
	{
	}

	std::ostream& operator<<(std::ostream& stream, const Exception& e)
	{
		stream << CurrentTimeAndDateString() << " - " << e.GetErrorLabel();

		auto Filename = FilenameFromPath(e.File);
		if (!Filename.empty() || !e.Function.empty())
			stream << " ";

		stream << EventLogger::FormatLog(e.what(), e.Line, e.Function, Filename, e.ErrorCode);

		return stream;
	}

	void ForwardException(std::exception_ptr e)
	{
		try
		{
			if (e)
				std::rethrow_exception(e);
		}
		catch (const Exception& e)
		{
			std::throw_with_nested(ForwardedException(e));
		}
		catch (const std::exception& e)
		{
			std::throw_with_nested(ForwardedException(e));
		}
		catch (...)
		{
			std::throw_with_nested(ForwardedException());
		}
	}
}