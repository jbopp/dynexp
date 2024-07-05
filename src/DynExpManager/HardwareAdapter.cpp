// This file is part of DynExp.

#include "stdafx.h"
#include "moc_HardwareAdapter.cpp"
#include "HardwareAdapter.h"

namespace DynExp
{
	HardwareAdapterParamsBase::~HardwareAdapterParamsBase()
	{
	}

	HardwareAdapterConfiguratorBase::~HardwareAdapterConfiguratorBase()
	{
	}

	HardwareAdapterBase::~HardwareAdapterBase()
	{
	}

	void HardwareAdapterBase::ResetException() const
	{
		// Short (default) timeout only since main thread should not block.
		auto lock = AcquireLock();

		LastException = std::exception_ptr();
	}

	void HardwareAdapterBase::ThrowException(std::exception_ptr Exception) const
	{
		// If AcquireLock() throws here, Exception does not get stored and is replaced
		// by the one thrown by AcquireLock(). This could in principal cause unintended
		// effects. On the other hand, next calls to hardware adapter's functions can
		// cause another exception which is then stored.
		auto lock = AcquireLock(HardwareOperationTimeout);

		ThrowExceptionUnsafe(Exception);
	}

	void HardwareAdapterBase::ThrowExceptionUnsafe(std::exception_ptr Exception) const
	{
		if (!Exception)
			return;

		try
		{
			std::rethrow_exception(Exception);
		}
// Object making use of this hardware adapter handles exception or fails which causes the exception to be logged.
#ifdef DYNEXP_DEBUG
		catch (const Util::Exception& e)
		{
			Util::EventLog().Log(e);
			LastException = std::current_exception();

			Util::ForwardException(LastException);
		}
#endif // DYNEXP_DEBUG
		catch (...)
		{
			LastException = std::current_exception();

			Util::ForwardException(LastException);
		}
	}

	void HardwareAdapterBase::SetExceptionUnsafe(std::exception_ptr Exception) const
	{
		if (!Exception)
			return;

		try
		{
			std::rethrow_exception(Exception);
		}
		catch (const Util::Exception& e)
		{
			Util::EventLog().Log(e);
			LastException = std::current_exception();
		}
		catch (...)
		{
			LastException = std::current_exception();
		}
	}

	void HardwareAdapterBase::ResetImpl(dispatch_tag<Object>)
	{
		// auto lock = AcquireLock(); not necessary here, since DynExp ensures that Object::Reset() can only
		// be called if respective object is not in use.

		LastException = nullptr;
		ResetImpl(dispatch_tag<HardwareAdapterBase>());

		EnsureReadyState(false);
	}

	void HardwareAdapterBase::EnsureReadyStateChild(bool IsAutomaticStartup)
	{
		if (GetException(IsAutomaticStartup ? HardwareOperationTimeout : ShortTimeoutDefault))
			throw Util::InvalidStateException(
				"A hardware adapter is in an error state. It requires to be reset in order to transition into a ready state.");

		EnsureReadyStateChild();
	}

	std::exception_ptr HardwareAdapterBase::GetExceptionChild(const std::chrono::milliseconds Timeout) const
	{
		// Short (default) timeout only since main thread should not block.
		auto lock = AcquireLock(Timeout);

		return GetExceptionUnsafe();
	}

	SerialCommunicationHardwareAdapterParams::~SerialCommunicationHardwareAdapterParams()
	{
	}

	Util::TextValueListType<SerialCommunicationHardwareAdapterParams::LineEndingType> SerialCommunicationHardwareAdapterParams::AvlblLineEndingsStrList()
	{
		Util::TextValueListType<LineEndingType> List = {
			{ "None", LineEndingType::None },
			{ "Zero", LineEndingType::Zero },
			{ "LF", LineEndingType::LF },
			{ "CRLF", LineEndingType::CRLF },
			{ "CR", LineEndingType::CR }
		};

		return List;
	}

	SerialCommunicationHardwareAdapterConfigurator::~SerialCommunicationHardwareAdapterConfigurator()
	{
	}

	SerialCommunicationHardwareAdapter::~SerialCommunicationHardwareAdapter()
	{
	}

	SerialCommunicationHardwareAdapter::SerialCommunicationHardwareAdapter(const std::thread::id OwnerThreadID, ParamsBasePtrType&& Params)
		: HardwareAdapterBase(OwnerThreadID, std::move(Params))
	{
		Init();
	}

	std::string SerialCommunicationHardwareAdapter::ReadLine() const
	{
		auto lock = AcquireLock(HardwareOperationTimeout);

		ReadIntoBuffer();

		// TODO: replace by C++20 view() method as soon as available
		auto Contents = ReadBuffer.str();
		auto Pos = Contents.find(LineEndingString, ReadBuffer.tellg());

		if (Pos == std::string::npos)
			return "";

		auto Line = Contents.substr(ReadBuffer.tellg(), Pos - ReadBuffer.tellg());

		ReadBuffer.str(Contents.substr(Pos + LineEndingString.length()));
		ReadBuffer.clear();

		return Line;
	}

	std::string SerialCommunicationHardwareAdapter::ReadAll() const
	{
		auto lock = AcquireLock(HardwareOperationTimeout);

		ReadIntoBuffer();

		auto Text = ReadBuffer.str().substr(ReadBuffer.tellg());
		ClearReadBuffer();

		return Text;
	}

	std::string SerialCommunicationHardwareAdapter::WaitForLine(unsigned int NumTries, std::chrono::milliseconds DelayBetweenTries) const
	{
		for (; NumTries > 0; --NumTries)
		{
			auto Line = ReadLine();

			if (!Line.empty())
				return Line;

			std::this_thread::sleep_for(DelayBetweenTries);
		}

		return "";
	}

	void SerialCommunicationHardwareAdapter::Clear() const
	{
		auto lock = AcquireLock(HardwareOperationTimeout);

		ClearChild();
		ClearReadBuffer();
	}

	void SerialCommunicationHardwareAdapter::Flush() const
	{
		auto lock = AcquireLock(HardwareOperationTimeout);

		FlushChild();
		ReadIntoBuffer();
	}

	const SerialCommunicationHardwareAdapter& SerialCommunicationHardwareAdapter::operator>>(std::stringstream& OutStream) const
	{
		OutStream << ReadAll();

		return *this;
	}

	const SerialCommunicationHardwareAdapter& SerialCommunicationHardwareAdapter::operator<<(endl) const
	{
		auto lock = AcquireLock(HardwareOperationTimeout);

		Write_endl();

		return *this;
	}

	const SerialCommunicationHardwareAdapter& SerialCommunicationHardwareAdapter::operator<<(const std::stringstream& InStream) const
	{
		auto lock = AcquireLock(HardwareOperationTimeout);

		Write(InStream.str());
		Write_endl();

		return *this;
	}

	const SerialCommunicationHardwareAdapter& SerialCommunicationHardwareAdapter::operator<<(const std::string& Text) const
	{
		auto lock = AcquireLock(HardwareOperationTimeout);

		Write(Text);
		Write_endl();

		return *this;
	}

	const SerialCommunicationHardwareAdapter& SerialCommunicationHardwareAdapter::operator<<(const std::string_view Text) const
	{
		auto lock = AcquireLock(HardwareOperationTimeout);

		Write(Text.data());
		Write_endl();

		return *this;
	}

	const SerialCommunicationHardwareAdapter& SerialCommunicationHardwareAdapter::operator<<(const char* Text) const
	{
		auto lock = AcquireLock(HardwareOperationTimeout);

		Write(Text);
		Write_endl();

		return *this;
	}

	const SerialCommunicationHardwareAdapter& SerialCommunicationHardwareAdapter::operator<<(const char Char) const
	{
		auto lock = AcquireLock(HardwareOperationTimeout);

		Write(std::string(1, Char));
		Write_endl();

		return *this;
	}

	void SerialCommunicationHardwareAdapter::InsertIntoBuffer(const std::string& String) const
	{
		if (String.empty())
			return;

		auto lock = AcquireLock(HardwareOperationTimeout);
		if (!CheckOverflow())
			return;

		ReadBuffer.write(String.c_str(), String.length());

		if (ReadBuffer.fail())
			throw Util::InvalidStateException("Writing data into read buffer failed.");
	}

	void SerialCommunicationHardwareAdapter::Init()
	{
		auto DerivedParams = dynamic_Params_cast<SerialCommunicationHardwareAdapter>(GetParams());

		LineEnding = DerivedParams->LineEnding;
		LineEndingString = std::string(LineEndingToChar(LineEnding).data(), GetLineEndingLength(LineEnding));
	}

	void SerialCommunicationHardwareAdapter::ResetImpl(dispatch_tag<HardwareAdapterBase>)
	{
		Init();
		ClearReadBuffer();

		ResetImpl(dispatch_tag<SerialCommunicationHardwareAdapter>());
	}

	void SerialCommunicationHardwareAdapter::ReadIntoBuffer() const
	{
		if (!CheckOverflow())
			return;

		auto DataRead = Read();

		if (!DataRead.empty())
			ReadBuffer.write(DataRead.c_str(), DataRead.length());

		if (ReadBuffer.fail())
			throw Util::InvalidStateException("Writing data into read buffer failed.");
	}

	void SerialCommunicationHardwareAdapter::ClearReadBuffer() const
	{
		ReadBuffer.str("");
		ReadBuffer.clear();
	}

	bool SerialCommunicationHardwareAdapter::CheckOverflow() const
	{
		auto initial_pos = ReadBuffer.tellg();
		ReadBuffer.seekg(0, std::ios::end);
		auto size = ReadBuffer.tellg();
		ReadBuffer.seekg(initial_pos, std::ios::beg);

		// Limit buffer length
		if (size > GetMaxBufferSize())
		{
			SetWarning("Read buffer exceeds 100 MB. No further data is read from the hardware device.", Util::DynExpErrorCodes::Overflow);

			return false;
		}

		return true;
	}

	QSerialCommunicationHardwareAdapterParams::~QSerialCommunicationHardwareAdapterParams()
	{
	}

	QSerialCommunicationHardwareAdapterConfigurator::~QSerialCommunicationHardwareAdapterConfigurator()
	{
	}

	QSerialCommunicationHardwareAdapterWorker::~QSerialCommunicationHardwareAdapterWorker()
	{
	}

	void QSerialCommunicationHardwareAdapterWorker::SetCommunicationChannelOpened() const noexcept
	{
		auto Owner = std::dynamic_pointer_cast<const QSerialCommunicationHardwareAdapter>(GetOwner());
		if (!Owner)
			return;

		Owner->QSerialCommunicationHardwareAdapterWorkerOnly.SetCommunicationChannelOpened();
	}

	void QSerialCommunicationHardwareAdapterWorker::SetCommunicationChannelClosed() const noexcept
	{
		auto Owner = std::dynamic_pointer_cast<const QSerialCommunicationHardwareAdapter>(GetOwner());
		if (!Owner)
			return;

		Owner->QSerialCommunicationHardwareAdapterWorkerOnly.SetCommunicationChannelClosed();
	}

	void QSerialCommunicationHardwareAdapterWorker::DataRead(const std::string& String) const
	{
		auto Owner = std::dynamic_pointer_cast<const QSerialCommunicationHardwareAdapter>(GetOwner());
		if (!Owner)
			return;

		Owner->QSerialCommunicationHardwareAdapterWorkerOnly.DataRead(String);
	}

	QSerialCommunicationHardwareAdapter::QSerialCommunicationHardwareAdapter(const std::thread::id OwnerThreadID, ParamsBasePtrType&& Params)
		: SerialCommunicationHardwareAdapter(OwnerThreadID, std::move(Params)),
		QSerialCommunicationHardwareAdapterWorkerOnly(*this), Worker(nullptr), CommunicationChannelOpened(false)
	{
	}

	QSerialCommunicationHardwareAdapter::~QSerialCommunicationHardwareAdapter()
	{
		if (Worker)
		{
			emit CloseSig();

			Worker->deleteLater();
		}
	}

	void QSerialCommunicationHardwareAdapter::DataRead(const std::string& String) const
	{
		InsertIntoBuffer(String);
	}

	void QSerialCommunicationHardwareAdapter::ResetImpl(dispatch_tag<SerialCommunicationHardwareAdapter>)
	{
		PendingException = std::exception_ptr();

		// Derived classes have to push new parameters to the Worker and emit the ResetSig() to reset the worker.
		ResetImpl(dispatch_tag<QSerialCommunicationHardwareAdapter>());
	}

	void QSerialCommunicationHardwareAdapter::EnsureReadyStateChild()
	{
		if (!Worker)
		{
			auto WorkerUniquePtr = MakeWorker();
			if (!WorkerUniquePtr)
				throw Util::InvalidArgException("Worker cannot be nullptr.");
			WorkerUniquePtr->MoveToWorkerThread(GetID());

			// Worker destroyed in DynExpCore's worker thread by calling QObject::deleteLater() on it.
			Worker = WorkerUniquePtr.release();

			// Connect Worker's slots to signals which are emitted here.
			QObject::connect(this, &QSerialCommunicationHardwareAdapter::OpenSig, Worker, &QSerialCommunicationHardwareAdapterWorker::Open);
			QObject::connect(this, &QSerialCommunicationHardwareAdapter::CloseSig, Worker, &QSerialCommunicationHardwareAdapterWorker::Close);
			QObject::connect(this, &QSerialCommunicationHardwareAdapter::ResetSig, Worker, &QSerialCommunicationHardwareAdapterWorker::Reset);
			QObject::connect(this, &QSerialCommunicationHardwareAdapter::ClearSig, Worker, &QSerialCommunicationHardwareAdapterWorker::Clear);
			QObject::connect(this, &QSerialCommunicationHardwareAdapter::FlushSig, Worker, &QSerialCommunicationHardwareAdapterWorker::Flush);
			QObject::connect(this, &QSerialCommunicationHardwareAdapter::ReadSig, Worker, &QSerialCommunicationHardwareAdapterWorker::Read);
			QObject::connect(this, &QSerialCommunicationHardwareAdapter::WriteSig, Worker, &QSerialCommunicationHardwareAdapterWorker::Write);
			QObject::connect(this, &QSerialCommunicationHardwareAdapter::Write_endl_Sig, Worker, &QSerialCommunicationHardwareAdapterWorker::Write_endl);

			InitWorker();
		}

		emit OpenSig();
	}

	bool QSerialCommunicationHardwareAdapter::IsReadyChild() const
	{
		auto lock = AcquireLock(HardwareOperationTimeout);

		auto Exception = GetExceptionUnsafe();
		Util::ForwardException(Exception);

		return CommunicationChannelOpened;
	}

	bool QSerialCommunicationHardwareAdapter::IsConnectedChild() const noexcept
	{
		return CommunicationChannelOpened;
	}

	void QSerialCommunicationHardwareAdapter::ClearChild() const
	{
		ThrowPendingException();

		emit ClearSig();
	}

	void QSerialCommunicationHardwareAdapter::FlushChild() const
	{
		ThrowPendingException();

		emit FlushSig();
	}

	std::string QSerialCommunicationHardwareAdapter::Read() const
	{
		ThrowPendingException();

		emit ReadSig();

		// See SerialCommunicationHardwareAdapter::InsertIntoBuffer() declaration.
		return "";
	}

	void QSerialCommunicationHardwareAdapter::Write(const std::string& String) const
	{
		ThrowPendingException();

		emit WriteSig(QString::fromStdString(String));
	}

	void QSerialCommunicationHardwareAdapter::Write_endl() const
	{
		ThrowPendingException();

		emit Write_endl_Sig();
	}

	void QSerialCommunicationHardwareAdapter::ThrowPendingException() const
	{
		if (!PendingException)
			return;

		std::exception_ptr e;
		std::swap(PendingException, e);

		// AcquireLock() has already been called by an (in)direct caller of this function.
		ThrowExceptionUnsafe(e);
	}
}