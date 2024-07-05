// This file is part of DynExp.

#include "stdafx.h"
#include "moc_HardwareAdapterEthernet.cpp"
#include "HardwareAdapterEthernet.h"

namespace DynExp
{
	HardwareAdapterTcpSocketWorker::HardwareAdapterTcpSocketWorker()
		: ServerName("localhost"), Port(1000), Socket(this)
	{
		Socket.setSocketOption(QAbstractSocket::KeepAliveOption, 1);
		Socket.setReadBufferSize(SerialCommunicationHardwareAdapter::GetMaxBufferSize());

		QObject::connect(&Socket, &QTcpSocket::connected, this, &HardwareAdapterTcpSocketWorker::OnConnected);
		QObject::connect(&Socket, &QTcpSocket::disconnected, this, &HardwareAdapterTcpSocketWorker::OnDisconnected);
		QObject::connect(&Socket, &QTcpSocket::errorOccurred, this, &HardwareAdapterTcpSocketWorker::OnErrorOccurred);
		QObject::connect(&Socket, &QTcpSocket::readyRead, this, &HardwareAdapterTcpSocketWorker::OnDataAvailable);
	}

	void HardwareAdapterTcpSocketWorker::Init(QString ServerName, quint16 Port)
	{
		this->ServerName = ServerName;
		this->Port = Port;
	}

	void HardwareAdapterTcpSocketWorker::OnConnected()
	{
		SetCommunicationChannelOpened();
	}

	void HardwareAdapterTcpSocketWorker::OnDisconnected()
	{
		SetCommunicationChannelClosed();
	}

	void HardwareAdapterTcpSocketWorker::OnErrorOccurred(QAbstractSocket::SocketError SocketError)
	{
		SetException(NetworkException(Util::ToStr(Socket.errorString()), SocketError));
	}

	void HardwareAdapterTcpSocketWorker::OnDataAvailable()
	{
		Read();
	}

	bool HardwareAdapterTcpSocketWorker::IsOpen() const noexcept
	{
		return Socket.state() == QAbstractSocket::SocketState::ConnectedState ||
			Socket.state() == QAbstractSocket::SocketState::BoundState;
	}

	bool HardwareAdapterTcpSocketWorker::IsOpening() const noexcept
	{
		return Socket.state() == QAbstractSocket::SocketState::HostLookupState ||
			Socket.state() == QAbstractSocket::SocketState::ConnectingState;
	}

	void HardwareAdapterTcpSocketWorker::OpenChild()
	{
		if (!IsOpen() && !IsOpening())
			Socket.connectToHost(ServerName, Port);
	}

	void HardwareAdapterTcpSocketWorker::CloseChild()
	{
		if (IsOpen())
			Socket.disconnectFromHost();
	}

	void HardwareAdapterTcpSocketWorker::ResetChild()
	{
		if (IsOpen())
			Socket.abort();
	}

	void HardwareAdapterTcpSocketWorker::ClearChild()
	{
		// No action needed.
	}

	void HardwareAdapterTcpSocketWorker::FlushChild()
	{
		if (IsOpen())
			Socket.flush();
	}

	void HardwareAdapterTcpSocketWorker::ReadChild()
	{
		QByteArray BytesRead(Socket.readAll());

		if (BytesRead.size())
			DataRead(BytesRead.constData());
	}

	void HardwareAdapterTcpSocketWorker::WriteChild(const QString& String)
	{
		auto StdString(String.toStdString());

		Socket.write(StdString.c_str(), StdString.size());	// crop terminating '\0'
	}

	void HardwareAdapterTcpSocketWorker::Write_endl_Child()
	{
		auto Owner = std::dynamic_pointer_cast<const HardwareAdapterTcpSocket>(GetOwner());
		if (!Owner)
			return;

		const auto LineEnding = Owner->GetLineEnding();

		Socket.write(Owner->LineEndingToChar(LineEnding).data(), Owner->GetLineEndingLength(LineEnding));
	}

	HardwareAdapterTcpSocket::HardwareAdapterTcpSocket(const std::thread::id OwnerThreadID, ParamsBasePtrType&& Params)
		: QSerialCommunicationHardwareAdapter(OwnerThreadID, std::move(Params))
	{
	}

	QSerialCommunicationHardwareAdapter::QWorkerPtrType HardwareAdapterTcpSocket::MakeWorker()
	{
		auto Worker = std::make_unique<HardwareAdapterTcpSocketWorker>();

		QObject::connect(this, &HardwareAdapterTcpSocket::InitSig, Worker.get(), &HardwareAdapterTcpSocketWorker::Init);

		return Worker;
	}

	void HardwareAdapterTcpSocket::Init()
	{
		auto DerivedParams = dynamic_Params_cast<HardwareAdapterTcpSocket>(GetParams());

		if (DerivedParams->NetworkParams.Port < 1 || DerivedParams->NetworkParams.Port > std::numeric_limits<uint16_t>::max())
			ThrowExceptionUnsafe(std::make_exception_ptr(Util::OutOfRangeException(
				"Port must be in range 1 to " + Util::ToStr(std::numeric_limits<uint16_t>::max()))));

		emit InitSig(QString::fromStdString(DerivedParams->NetworkParams.ServerName), static_cast<uint16_t>(DerivedParams->NetworkParams.Port));
	}

	void HardwareAdapterTcpSocket::ResetImpl(dispatch_tag<QSerialCommunicationHardwareAdapter>)
	{
		emit ResetSig();
		Init();

		ResetImpl(dispatch_tag<HardwareAdapterTcpSocket>());
	}
}