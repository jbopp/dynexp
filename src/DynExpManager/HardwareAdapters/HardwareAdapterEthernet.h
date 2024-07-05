// This file is part of DynExp.

/**
 * @file HardwareAdapterEthernet.h
 * @brief Implementation of a hardware adapter to communicate text-based
 * commands over TCP sockets.
*/

#pragma once

#include <QTcpSocket>

#include "stdafx.h"
#include "HardwareAdapter.h"

namespace DynExp
{
	class HardwareAdapterTcpSocket;

	/**
	 * @brief Defines an exception caused by a hardware operation on an ethernet interface
	 * (e.g. reading/writing data to a TCP socket).
	*/
	class NetworkException : public SerialCommunicationException
	{
	public:
		/**
		 * @brief Constructs a @p NetworkException instance.
		 * @param Description Description explaining the cause of the exception in detail for the user.
		 * @param ErrorCode Error code from Util::DynExpErrorCodes::DynExpErrorCodes
		 * @param Location Origin of the exception. Refer to Util::Exception::Exception().
		*/
		NetworkException(std::string Description, const int ErrorCode,
			const std::source_location Location = std::source_location::current()) noexcept
			: SerialCommunicationException(std::move(Description), ErrorCode, Location)
		{}
	};

	/**
	 * @brief Bundles several parameters to describe a network connection. Use in parameter classes.
	*/
	class NetworkParamsExtension
	{
	public:
		/**
		 * @brief Constructs a @p NetworkParamsExtension instance.
		 * @param Owner Parameter class owning the parameters bundled by this instance
		*/
		NetworkParamsExtension(DynExp::ParamsBase& Owner)
			: ServerName{ Owner, "ServerName", "Server name",
				"IP address or host name of the server to connect to", true, "localhost" },
			Port{ Owner, "Port", "Server port",
				"Port of the remote server to connect to", true, 1000, 1, std::numeric_limits<uint16_t>::max(), 1, 0 }
		{}

		/**
		 * @brief Builds a network address from the parameters' content.
		 * @return Network address as a string in the format [ip]:[port]
		*/
		std::string MakeAddress() const { return ServerName.Get() + ":" + Util::ToStr(Port.Get()); }

		DynExp::ParamsBase::Param<ParamsConfigDialog::TextType> ServerName;		//!< IP address
		DynExp::ParamsBase::Param<ParamsConfigDialog::NumberType> Port;			//!< Network port
	};

	/**
	 * @brief Parameter class for @p HardwareAdapterTcpSocket
	*/
	class HardwareAdapterTcpSocketParams : public QSerialCommunicationHardwareAdapterParams
	{
	public:
		/**
		 * @brief Constructs the parameters for a @p HardwareAdapterTcpSocket instance.
		 * @copydetails ParamsBase::ParamsBase
		*/
		HardwareAdapterTcpSocketParams(ItemIDType ID, const DynExpCore& Core)
			: QSerialCommunicationHardwareAdapterParams(ID, Core), NetworkParams(*this) {}

		virtual ~HardwareAdapterTcpSocketParams() = default;

		virtual const char* GetParamClassTag() const noexcept override { return "HardwareAdapterTcpSocketParams"; }

		NetworkParamsExtension NetworkParams;												//!< @copydoc NetworkParamsExtension

	private:
		void ConfigureParamsImpl(dispatch_tag<QSerialCommunicationHardwareAdapterParams>) override final { ConfigureParamsImpl(dispatch_tag<HardwareAdapterTcpSocketParams>()); }
		virtual void ConfigureParamsImpl(dispatch_tag<HardwareAdapterTcpSocketParams>) {}	//!< @copydoc ConfigureParamsImpl(dispatch_tag<QSerialCommunicationHardwareAdapterParams>)
	};

	/**
	 * @brief Configurator class for @p HardwareAdapterTcpSocket
	*/
	class HardwareAdapterTcpSocketConfigurator : public QSerialCommunicationHardwareAdapterConfigurator
	{
	public:
		using ObjectType = HardwareAdapterTcpSocket;
		using ParamsType = HardwareAdapterTcpSocketParams;

		HardwareAdapterTcpSocketConfigurator() = default;
		virtual ~HardwareAdapterTcpSocketConfigurator() = default;

	private:
		virtual ParamsBasePtrType MakeParams(ItemIDType ID, const DynExpCore& Core) const override { return DynExp::MakeParams<HardwareAdapterTcpSocketConfigurator>(ID, Core); }
	};

	/**
	 * @brief Qt worker for network communication in a separate thread
	*/
	class HardwareAdapterTcpSocketWorker : public QSerialCommunicationHardwareAdapterWorker
	{
		Q_OBJECT

	public:
		HardwareAdapterTcpSocketWorker();
		virtual ~HardwareAdapterTcpSocketWorker() {}

	public slots:
		/**
		 * @brief Initializes HardwareAdapterTcpSocketWorker::Socket for TCP
		 * communication with the given settings
		 * @param ServerName IP address
		 * @param Port Network port
		*/
		void Init(QString ServerName, quint16 Port);

	private slots:
		/**
		 * @brief Qt slot called when #Socket has been connected.
		 * Calls QSerialCommunicationHardwareAdapterWorker::SetCommunicationChannelOpened().
		*/
		void OnConnected();

		/**
		 * @brief Qt slot called when #Socket has been disconnected.
		 * Calls QSerialCommunicationHardwareAdapterWorker::SetCommunicationChannelClosed().
		*/
		void OnDisconnected();

		/**
		 * @brief Qt slot called when #Socket transitioned into an error state.
		 * Calls QSerialCommunicationHardwareAdapterWorker::SetException() to store a
		 * @p NetworkException constructed from @p SocketError.
		 * @param SocketError Information about the error occurred to #Socket.
		*/
		void OnErrorOccurred(QAbstractSocket::SocketError SocketError);

		/**
		 * @brief Qt slot called when #Socket has received data which can be read.
		 * Calls QSerialCommunicationHardwareAdapterWorker::Read().
		*/
		void OnDataAvailable();

	private:
		/**
		 * @brief Checks whether #Socket is connected.
		 * @return Returns true if #Socket is in a connected or bound state, false otherwise.
		*/
		bool IsOpen() const noexcept;

		/**
		 * @brief Checks whether #Socket is connecting right now.
		 * @return Returns true if #Socket is in a connecting or host look-up state, false otherwise.
		*/
		bool IsOpening() const noexcept;

		virtual void OpenChild() override;
		virtual void CloseChild() override;
		virtual void ResetChild() override;
		virtual void ClearChild() override;
		virtual void FlushChild() override;
		virtual void ReadChild() override;
		virtual void WriteChild(const QString& String) override;
		virtual void Write_endl_Child() override;

		QString ServerName;					//!< IP address to connect to
		quint16 Port;						//!< Network port to connect to

		QTcpSocket Socket;					//!< COM port for serial communication
	};

	/**
	 * @brief Implements a hardware adapter to communicate with text-based
	 * commands over TCP sockets.
	*/
	class HardwareAdapterTcpSocket : public QSerialCommunicationHardwareAdapter
	{
		Q_OBJECT

	public:
		using ParamsType = HardwareAdapterTcpSocketParams;											//!< @copydoc Object::ParamsType
		using ConfigType = HardwareAdapterTcpSocketConfigurator;									//!< @copydoc Object::ConfigType

		constexpr static auto Name() noexcept { return "TCP Socket"; }								//!< @copydoc SerialCommunicationHardwareAdapter::Name()

		HardwareAdapterTcpSocket(const std::thread::id OwnerThreadID, ParamsBasePtrType&& Params);	//!< @copydoc QSerialCommunicationHardwareAdapter::QSerialCommunicationHardwareAdapter
		virtual ~HardwareAdapterTcpSocket() = default;

		virtual std::string GetName() const override { return Name(); }

	private:
		QWorkerPtrType MakeWorker() override;
		void InitWorker() override { Init(); }

		/**
		 * @copydoc SerialCommunicationHardwareAdapter::Init()
		 * @throws Util::OutOfRangeException is thrown if the port specified in
		 * NetworkParamsExtension::Port of the parameter class instance related to
		 * this hardware adaper instance is lower than 1 or higher than 65535.
		*/
		void Init();

		void ResetImpl(dispatch_tag<QSerialCommunicationHardwareAdapter>) override final;
		virtual void ResetImpl(dispatch_tag<HardwareAdapterTcpSocket>) {}							//!< @copydoc ResetImpl(dispatch_tag<QSerialCommunicationHardwareAdapter>)

	signals:
		/** @name Qt signals
		 * Signals emitted by this hardware adapter. The worker instance's slots
		 * are connected to them to handles the signals.
		*/
		///@{
		/**
		 * @copydoc HardwareAdapterTcpSocketWorker::Init()
		*/
		void InitSig(QString ServerName, quint16 Port);
		///@}
	};
}