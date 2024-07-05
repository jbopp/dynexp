// This file is part of DynExp.

#include "stdafx.h"
#include "Util.h"

namespace Util
{
	std::unique_lock<ILockable::MutexType> ILockable::AcquireLock(const std::chrono::milliseconds Timeout) const
	{
		using namespace std::chrono_literals;

		// In order to compensate for spurious failures by retrying
		// (see https://en.cppreference.com/w/cpp/thread/timed_mutex/try_lock_for)
		constexpr int NumTries = 2;

		if (Timeout == 0ms)
			return std::unique_lock<MutexType>(LockMutex);
		else
		{
			std::unique_lock<MutexType> lock(LockMutex, std::defer_lock);

			for (auto i = NumTries; i > 0; --i)
				if (lock.try_lock_for(Timeout / NumTries))
					return lock;

			throw TimeoutException("Timeout occurred while trying to lock a mutex.");
		}
	}

	OneToOneNotifier::~OneToOneNotifier()
	{
		Notify();

		// See OneToOneNotifier::Wait().
		while (!MutexCanBeDestroyed)
			std::this_thread::yield();
	}

	bool OneToOneNotifier::Wait(const std::chrono::milliseconds Timeout)
	{
		bool TimeoutOccurred = false;

		{
			std::unique_lock<decltype(Mutex)> Lock(Mutex);

			if (SomeoneIsWaiting)
				throw InvalidCallException(
					"There is already a waiting thread. Only one thread can await this notifier.");
			SomeoneIsWaiting = true;
			MutexCanBeDestroyed = false;

			if (Timeout.count())
				TimeoutOccurred = !ConditionVariable.wait_for(Lock, Timeout, [this]() { return EventOccurred; });
			else
				ConditionVariable.wait(Lock, [this]() { return EventOccurred; });

			// Reset the notifier
			EventOccurred = false;
			SomeoneIsWaiting = false;
		}

		// Now, OneToOneNotifier's destructor (if it was called while waiting) is allowed to end which
		// leads to Mutex being destroyed. There is no unique_lock locking Mutex anymore at this point.
		MutexCanBeDestroyed = true;

		return TimeoutOccurred;
	}

	void OneToOneNotifier::Notify()
	{
		{
			std::unique_lock<decltype(Mutex)> Lock(Mutex);
			EventOccurred = true;
		}

		ConditionVariable.notify_one();
	}

	void OneToOneNotifier::Ignore()
	{
		std::unique_lock<decltype(Mutex)> Lock(Mutex);
		EventOccurred = false;
	}

	BlobDataType::BlobDataType(const BlobDataType& Other)
		: DataSize(Other.DataSize)
	{
		if (Other.DataPtr)
		{
			DataPtr = std::make_unique<DataType>(DataSize);
			std::memcpy(DataPtr.get(), Other.DataPtr.get(), DataSize);
		}
	}

	BlobDataType::BlobDataType(BlobDataType&& Other) noexcept
		: DataPtr(std::move(Other.DataPtr)), DataSize(Other.DataSize)
	{
		Other.Reset();
	}

	BlobDataType& BlobDataType::operator=(const BlobDataType& Other)
	{
		Reserve(Other.DataSize);
		std::memcpy(DataPtr.get(), Other.DataPtr.get(), DataSize);

		return *this;
	}

	BlobDataType& BlobDataType::operator=(BlobDataType&& Other) noexcept
	{
		DataSize = Other.DataSize;
		DataPtr = std::move(Other.DataPtr);
		Other.Reset();

		return *this;
	}

	void BlobDataType::Reserve(size_t Size)
	{
		if (DataSize == Size)
			return;

		if (!Size)
			DataPtr.reset();
		else
			DataPtr = std::make_unique<DataType>(Size);

		DataSize = Size;
	}

	void BlobDataType::Assign(size_t Size, const DataType Data)
	{
		Reserve(Size);
		std::memcpy(DataPtr.get(), Data, Size);
	}

	void BlobDataType::Reset()
	{
		DataPtr.reset();
		DataSize = 0;
	}

	BlobDataType::DataPtrType::element_type* BlobDataType::Release() noexcept
	{
		DataSize = 0;
		return DataPtr.release();
	}

	std::strong_ordering operator<=>(const VersionType& lhs, const VersionType& rhs)
	{
		if (lhs.Major == rhs.Major && lhs.Minor == rhs.Minor && lhs.Patch == rhs.Patch)
			return std::strong_ordering::equal;

		if (lhs.Major > rhs.Major ||
			(lhs.Major == rhs.Major && lhs.Minor > rhs.Minor) ||
			(lhs.Major == rhs.Major && lhs.Minor == rhs.Minor && lhs.Patch > rhs.Patch))
			return std::strong_ordering::greater;

		return std::strong_ordering::less;
	}

	VersionType VersionFromString(std::string_view Str)
	{
		VersionType Version;

		static const std::regex VersionRegex("(\\d+)\\.(\\d+)(?:\\.(\\d+))?");
		std::cmatch Results;
		std::regex_search(Str.data(), Results, VersionRegex);
		if (Results.length() <= 1 || Results.length() > 5)	// Matching a suffix (like "-beta") increases Results' length by one.
			throw Util::InvalidDataException("Str does not contain a valid version number.");

		// Results[0] contains entire match, so ignore that.
		for (size_t i = 1; i < Results.size(); ++i)
		{
			if (!Results[i].matched)
				continue;

			switch (i)
			{
			case 1: Version.Major = Util::StrToT<decltype(VersionType::Major)>(Results[i].str()); break;
			case 2: Version.Minor = Util::StrToT<decltype(VersionType::Minor)>(Results[i].str()); break;
			case 3: Version.Patch = Util::StrToT<decltype(VersionType::Patch)>(Results[i].str());
			}
		}

		return Version;
	}

	std::string ExceptionToStr(const std::exception_ptr ExceptionPtr)
	{
		if (!ExceptionPtr)
			return "";

		try
		{
			std::rethrow_exception(ExceptionPtr);
		}
		catch (const std::exception& e)
		{
			return e.what();
		}
		catch (...)
		{
			return "Unknown exception.";
		}
	}

	std::string ToLower(std::string_view Str)
	{
		std::string LowerStr;
		LowerStr.resize(Str.size());

		std::transform(Str.cbegin(), Str.cend(), LowerStr.begin(),
			[](unsigned char c) { return std::tolower(c); }
		);

		return LowerStr;
	}

	std::vector<std::complex<double>> FFT(const std::vector<std::complex<double>>& Data, bool InverseTransform)
	{
		if (Data.size() > std::numeric_limits<size_t>::max() / 2)
			throw OverflowException("Size of Data vector must not exceed half the capacity of size_t.");

		std::vector<double> RawData;
		RawData.resize(2 * Data.size());
		for (size_t i = 0; i < Data.size(); ++i)
		{
			RawData[2 * i] = Data[i].real();
			RawData[2 * i + 1] = Data[i].imag();
		}

		gsl_fft_complex_workspace* Workspace = gsl_fft_complex_workspace_alloc(Data.size());
		gsl_fft_complex_wavetable* Wavetable = gsl_fft_complex_wavetable_alloc(Data.size());
		
		try
		{
			if (!Workspace || !Wavetable)
				throw NotAvailableException("Could not reserve memory for FFT.", ErrorType::Error);

			if (gsl_fft_complex_transform(RawData.data(), 1, Data.size(), Wavetable, Workspace,
				InverseTransform ? gsl_fft_direction::gsl_fft_backward : gsl_fft_direction::gsl_fft_forward))
				throw InvalidDataException("The FFT failed for an unknown reason.");

			gsl_fft_complex_wavetable_free(Wavetable);
			gsl_fft_complex_workspace_free(Workspace);

			std::vector<std::complex<double>> Result;
			Result.resize(Data.size());
			for (size_t i = 0; i < Result.size(); ++i)
				Result[i] = { RawData[2 * i], RawData[2 * i + 1] };

			return Result;
		}
		catch (...)
		{
			if (Wavetable)
				gsl_fft_complex_wavetable_free(Wavetable);
			if (Workspace)
				gsl_fft_complex_workspace_free(Workspace);

			throw;
		}
	}

	Warning::Warning(Warning&& Other) noexcept : Data(std::make_unique<WarningData>())
	{
		auto other_lock = Other.AcquireLock();

		Data.swap(Other.Data);
	}

	void Warning::Reset()
	{
		auto lock = AcquireLock();

		Data = std::make_unique<WarningData>();
	}

	Warning& Warning::operator=(const Exception& e)
	{
		auto lock = AcquireLock();
		
		Data = std::make_unique<WarningData>(e.what(), e.ErrorCode, e.Line, e.Function, e.File);
		return *this;
	}

	Warning& Warning::operator=(Warning&& Other) noexcept
	{
		auto other_lock = Other.AcquireLock();
		auto lock = AcquireLock();

		Data.swap(Other.Data);
		return *this;
	}

	Warning::WarningData Warning::Get() const
	{
		auto lock = AcquireLock();
		
		return *Data;
	}

	EventLogger::EventLogger()
	{
		LogFile.exceptions(std::ofstream::failbit | std::ofstream::badbit);
	}

	void EventLogger::Log(const std::string& Message, const ErrorType Type,
		const size_t Line, const std::string& Function, const std::string& File, const int ErrorCode
#ifdef DYNEXP_HAS_STACKTRACE
		, const std::stacktrace& Trace
#endif // DYNEXP_HAS_STACKTRACE
	) noexcept
	{
		try
		{
			auto Filename = FilenameFromPath(File);

			auto lock = AcquireLock(LogOperationTimeout);

			LogEntry Entry(FormatLog(Message, Line, Function, Filename, ErrorCode, false), Type, std::chrono::system_clock::now());
			LogEntries.emplace_back(std::move(Entry));

			if (!IsOpenUnsafe())
				return;

			LogFile << FormatLogHTML(Message, Type, Line, Function, Filename, ErrorCode
#ifdef DYNEXP_HAS_STACKTRACE
				, Trace
#endif // DYNEXP_HAS_STACKTRACE
			);

			LogFile.flush();
		}
		catch (...)
		{
			// Swallow every exception (refer to function declaration in Util.h)
		}
	}

	void EventLogger::Log(const Exception& E) noexcept
	{
		Log(E.what(), E.Type, E.Line, E.Function, E.File, E.ErrorCode
#ifdef DYNEXP_HAS_STACKTRACE
			, E.GetStackTrace()
#endif // DYNEXP_HAS_STACKTRACE
		);
	}

	void EventLogger::Log(const Warning& W) noexcept
	{
		try
		{
			auto Data = W.Get();

			Log(Data.Description, ErrorType::Warning, Data.Line, Data.Function, Data.File, Data.ErrorCode);
		}
		catch (...)
		{
			// Swallow every exception (refer to function declaration in Util.h)
		}
	}

	std::string EventLogger::FormatLog(const std::string& Message, const size_t Line,
		const std::string& Function, const std::string& Filename, const int ErrorCode, const bool PrefixMessage)
	{
		std::stringstream stream;

		if (!Filename.empty() || !Function.empty())
		{
			stream << "(";
			if (!Filename.empty())
			{
				stream << Filename << ((!Line && !Function.empty()) ? " " : "");
				if (Line)
					stream << ":" << ToStr(Line) << (!Function.empty() ? " " : "");
			}
			if (!Function.empty())
				stream << "in " << Function << "()";
			stream << ")" << (PrefixMessage ? "" : ": ");
		}
		stream << (PrefixMessage ? ": " : "") << Message;
		if (ErrorCode)
			stream << " (Code " << ToStr(ErrorCode) << ")";

		return stream.str();
	}

	std::string EventLogger::FormatLogHTML(const std::string& Message, const ErrorType Type,
		const size_t Line, const std::string& Function, const std::string& Filename, const int ErrorCode
#ifdef DYNEXP_HAS_STACKTRACE
		, const std::stacktrace& Trace
#endif // DYNEXP_HAS_STACKTRACE
	)
	{
		auto Label = Exception::GetErrorLabel(Type);
		auto ColorString = Exception::GetErrorLabelColor(Type);

		std::stringstream stream;

		stream << "\n" <<
#ifdef DYNEXP_HAS_STACKTRACE
			(Trace.empty() ?
#endif // DYNEXP_HAS_STACKTRACE
				"<div class=\"entry_no_details\">"
#ifdef DYNEXP_HAS_STACKTRACE
				: "<details><summary class=\"entry_details\">")
#endif // DYNEXP_HAS_STACKTRACE
			<< "<span class=\"entry_time\"><span class=\"time\">" << CurrentTimeAndDateString() << "</span></span>"
			<< "<span class=\"entry_label\"><span class=\"label\" style=\"color:" << ColorString << "\">" << Label << "</span></span>"
			<< "<span class=\"entry_text\">";
		if (!Filename.empty() || !Function.empty())
		{
			stream << "<span class=\"origin\">(";
			if (!Filename.empty())
			{
				stream << Filename;
				if (Line)
					stream << ":" << Line << (!Function.empty() ? " " : "");
			}
			if (!Function.empty())
				stream << "in " << QString::fromStdString(Function).toHtmlEscaped().toStdString();
			stream << ")</span><br>";
		}
		stream << "<span class=\"msg\">" << QString::fromStdString(Message).toHtmlEscaped().toStdString() << "</span>";
		if (ErrorCode)
			stream << " <span class=\"errno\">(Code " << ErrorCode << ")</span>";
		stream << "\n</span>";

#ifdef DYNEXP_HAS_STACKTRACE
		if (!Trace.empty())
		{
			stream << "\n</summary><div class=\"trace\"><ol>\n";
			for (const auto& entry : Trace)
				stream << "<li>" << QString::fromStdString(std::to_string(entry)).toHtmlEscaped().toStdString() << "</li>\n";
			stream << "</ol></div></details>";
		}
		else
#endif // DYNEXP_HAS_STACKTRACE
			stream << "\n</div>";

		return stream.str();
	}

	void EventLogger::OpenLogFile(std::string Filename)
	{
		auto lock = AcquireLock(LogOperationTimeout);

		ClearLogUnsafe();
		CloseLogFileUnsafe();

		LogFile.open(Filename, std::ios_base::out | std::ios_base::trunc);
		this->Filename = Filename;

		LogFile << "<!DOCTYPE html>\n"
			<< "<html lang=\"en\">\n<head>\n<meta charset=\"utf-8\">\n<style>\n"
			<< "body {background-color:white; font-family:sans-serif}\n"
			<< ".version {font-style:italic}\n"
			<< ".trace {margin-left: 18em}\n"
			<< "summary::marker {content:none}\n"
			<< "summary::before {content:\"+\"; font-weight:bold; float:left; position:absolute; left:.9em}\n"
			<< "details[open] summary::before {content:\"-\"}\n"
			<< ".entry_no_details {margin-left:1.4em; margin-block:.2em}\n"
			<< ".entry_details {margin-left:1.4em; list-style-position:outside}\n"
			<< ".entry_label, .entry_text {display:table-cell; padding-right:1em; vertical-align:top; min-width:6em}\n"
			<< ".entry_time {display:table-cell; padding-right:1em; vertical-align:top; min-width:10em}\n"
			<< "div.entry_no_details:hover, summary.entry_details:hover {background-color:lightgray}\n"
			<< ".label {font-weight:bold}\n"
			<< ".origin {font-family:monospace}\n"
			<< ".errno {font-style:italic}\n"
			<< ".trace ol {font-family:monospace; margin-block-start:.2em; margin-left:2em}\n"
			<< ".end {font-style:italic}\n"
			<< "</style>\n<title>DynExp Logfile</title>\n</head>\n<body>\n"
			<< "<h1>DynExp Logfile</h1>\n"
#ifdef DYNEXP_DEBUG
			<< "<p class=\"version\">(" << DynExp::DynExpVersion << "-Debug)</p>\n"
#else
			<< "<p class=\"version\">(" << DynExp::DynExpVersion << "-Release)</p>\n"
#endif // DYNEXP_DEBUG
			<< "<div class=\"entries\">";

		LogFile.flush();
	}

	std::vector<LogEntry> EventLogger::GetLog(size_t FirstElement) const
	{
		auto lock = AcquireLock(LogOperationTimeout);

		if (FirstElement >= LogEntries.size())
			throw Util::OutOfRangeException("FirstElement exceeds number of log entries.");
			
		return { LogEntries.cbegin() + FirstElement, LogEntries.cend() };
	}

	void EventLogger::CloseLogFileUnsafe()
	{
		if (IsOpenUnsafe())
		{
			if (LogFile.tellp())
				LogFile << "\n</div>\n<p class=\"end\">*** Logfile end.</p>\n</body>\n</html>";
			LogFile.flush();
			LogFile.close();
		}

		Filename = "";
	}

	EventLogger& EventLog()
	{
		static EventLogger EventLog;

		return EventLog;
	}
}