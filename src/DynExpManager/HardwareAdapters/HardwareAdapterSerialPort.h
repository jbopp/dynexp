// This file is part of DynExp.

/**
 * @file HardwareAdapterSerialPort.h
 * @brief Implementation of a hardware adapter to communicate text-based
 * commands over COM ports.
*/

#pragma once

#include <QtSerialPort/QSerialPort>
#include <QtSerialPort/QSerialPortInfo>

#include "stdafx.h"
#include "HardwareAdapter.h"

namespace DynExp
{
	class HardwareAdapterSerialPort;

	/**
	 * @brief Defines an exception caused by a hardware operation on a COM port
	 * (e.g. reading/writing data to a COM port).
	*/
	class SerialPortException : public SerialCommunicationException
	{
	public:
		/**
		 * @brief Constructs a @p SerialPortException instance.
		 * @param Description Description explaining the cause of the exception in detail for the user.
		 * @param ErrorCode Error code from Util::DynExpErrorCodes::DynExpErrorCodes
		 * @param Location Origin of the exception. Refer to Util::Exception::Exception().
		*/
		SerialPortException(std::string Description, const int ErrorCode,
			const std::source_location Location = std::source_location::current()) noexcept
			: SerialCommunicationException(std::move(Description), ErrorCode, Location)
		{}
	};

	/**
	 * @brief Parameter class for @p HardwareAdapterSerialPort
	*/
	class HardwareAdapterSerialPortParams : public QSerialCommunicationHardwareAdapterParams
	{
	public:
		/**
		 * @brief Constructs the parameters for a @p HardwareAdapterSerialPort instance.
		 * @copydetails ParamsBase::ParamsBase
		*/
		HardwareAdapterSerialPortParams(ItemIDType ID, const DynExpCore& Core) : QSerialCommunicationHardwareAdapterParams(ID, Core) {}

		virtual ~HardwareAdapterSerialPortParams() = default;

		virtual const char* GetParamClassTag() const noexcept override { return "HardwareAdapterSerialPortParams"; }

		Param<TextList> PortName = { *this, {}, "PortName", "Port",
			"Name of the serial port to be assigned to this hardware adapter" };			//!< COM port name
		Param<QSerialPort::BaudRate> BaudRate = { *this, Util::QtEnumToTextValueList<QSerialPort::BaudRate>(0, 0, 4), "BaudRate", "Baud rate",
			"Data rate in bits per second", true, QSerialPort::Baud9600 };					//!< Baud rate
		Param<QSerialPort::DataBits> DataBits = { *this, Util::QtEnumToTextValueList<QSerialPort::DataBits>(0, 0, 4), "DataBits", "Data bits",
			"Number of data bits in each character", true, QSerialPort::Data8 };			//!< Amount of data bits
		Param<QSerialPort::StopBits> StopBits = { *this, Util::QtEnumToTextValueList<QSerialPort::StopBits>(0, 0, 0, 4), "StopBits", "Stop bits",
			"Number of stop bits after each character", true, QSerialPort::OneStop };		//!< Amount of stop bits
		Param<QSerialPort::Parity> Parity = { *this, Util::QtEnumToTextValueList<QSerialPort::Parity>(0, 0, 0, 6), "Parity", "Parity",
			"Error detection method", true, QSerialPort::NoParity };						//!< Parity setting

	private:
		/**
		 * @copydoc QSerialCommunicationHardwareAdapterParams::ConfigureParamsImpl(dispatch_tag<QSerialCommunicationHardwareAdapterParams>)
		 * @throws Util::EmptyException is thrown if there is no serial/COM port available on the system.
		*/
		void ConfigureParamsImpl(dispatch_tag<QSerialCommunicationHardwareAdapterParams>) override final;

		virtual void ConfigureParamsImpl(dispatch_tag<HardwareAdapterSerialPortParams>) {}	//!< @copydoc ConfigureParamsImpl(dispatch_tag<QSerialCommunicationHardwareAdapterParams>)
	};

	/**
	 * @brief Configurator class for @p HardwareAdapterSerialPort
	*/
	class HardwareAdapterSerialPortConfigurator : public QSerialCommunicationHardwareAdapterConfigurator
	{
	public:
		using ObjectType = HardwareAdapterSerialPort;
		using ParamsType = HardwareAdapterSerialPortParams;

		HardwareAdapterSerialPortConfigurator() = default;
		virtual ~HardwareAdapterSerialPortConfigurator() = default;

	private:
		virtual DynExp::ParamsBasePtrType MakeParams(ItemIDType ID, const DynExpCore& Core) const override { return DynExp::MakeParams<HardwareAdapterSerialPortConfigurator>(ID, Core); }
	};

	/**
	 * @brief Qt worker for serial/COM port communication in a separate thread
	*/
	class HardwareAdapterSerialPortWorker : public QSerialCommunicationHardwareAdapterWorker
	{
		Q_OBJECT

	public:
		HardwareAdapterSerialPortWorker();
		virtual ~HardwareAdapterSerialPortWorker() {}

	public slots:
		/**
		 * @brief Initializes HardwareAdapterSerialPortWorker::Port for serial
		 * communication with the given settings
		 * @param PortName COM port name
		 * @param BaudRate Baud rate
		 * @param DataBits Amount of data bits
		 * @param StopBits Amount of stop bits
		 * @param Parity Parity setting
		*/
		void Init(QString PortName, QSerialPort::BaudRate BaudRate, QSerialPort::DataBits DataBits,
			QSerialPort::StopBits StopBits, QSerialPort::Parity Parity);

	private slots:
		void OnDataAvailable();				//!< Qt slot called when #Port has received data which can be read.

	private:
		/**
		 * @brief Checks whether #Port is in an error state. If this is the case, sets an
		 * exception using QSerialCommunicationHardwareAdapterWorker::SetException().
		 * @param Location Location from which this function is called. Do not pass anything.
		 * @return Returns false in case of an error, true otherwise.
		*/
		bool CheckError(const std::source_location Location = std::source_location::current()) const;

		virtual void OpenChild() override;
		virtual void CloseChild() override;
		virtual void ResetChild() override;
		virtual void ClearChild() override;
		virtual void FlushChild() override;
		virtual void ReadChild() override;
		virtual void WriteChild(const QString& String) override;
		virtual void Write_endl_Child() override;

		QSerialPort Port;					//!< COM port for serial communication
	};

	/**
	 * @brief Implements a hardware adapter to communicate with text-based
	 * commands over COM ports.
	*/
	class HardwareAdapterSerialPort : public QSerialCommunicationHardwareAdapter
	{
		Q_OBJECT

	public:
		using ParamsType = HardwareAdapterSerialPortParams;												//!< @copydoc Object::ParamsType
		using ConfigType = HardwareAdapterSerialPortConfigurator;										//!< @copydoc Object::ConfigType

		constexpr static auto Name() noexcept { return "Serial Port"; }									//!< @copydoc SerialCommunicationHardwareAdapter::Name()

		/**
		 * @brief Enumerates all serial ports available on the system.
		 * @return Returns a @p std::vector< @p std::string > of the serial ports' names.
		*/
		static auto Enumerate();

		HardwareAdapterSerialPort(const std::thread::id OwnerThreadID, ParamsBasePtrType&& Params);		//!< @copydoc QSerialCommunicationHardwareAdapter::QSerialCommunicationHardwareAdapter
		virtual ~HardwareAdapterSerialPort() = default;

		virtual std::string GetName() const override { return Name(); }

	private:
		QWorkerPtrType MakeWorker() override;
		void InitWorker() override { Init(); }

		/**
		 * @brief Registers QSerialPort enumerations as Qt meta types using qRegisterMetaType()
		 * (refer to Qt documentation). Necessary to use the enumerations in Qt's signal/slot mechanism.
		*/
		void RegisterQTypesAsMetaTypes();

		void Init();																					//!< @copydoc SerialCommunicationHardwareAdapter::Init()

		void ResetImpl(dispatch_tag<QSerialCommunicationHardwareAdapter>) override final;
		virtual void ResetImpl(dispatch_tag<HardwareAdapterSerialPort>) {}								//!< @copydoc ResetImpl(dispatch_tag<QSerialCommunicationHardwareAdapter>)

	signals:
		/** @name Qt signals
		 * Signals emitted by this hardware adapter. The worker instance's slots
		 * are connected to them to handles the signals.
		*/
		///@{
		/**
		 * @copydoc HardwareAdapterSerialPortWorker::Init()
		*/
		void InitSig(QString PortName, QSerialPort::BaudRate BaudRate, QSerialPort::DataBits DataBits,
			QSerialPort::StopBits StopBits, QSerialPort::Parity Parity);
		///@}
	};
}