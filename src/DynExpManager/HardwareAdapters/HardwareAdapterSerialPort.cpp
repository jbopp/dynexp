// This file is part of DynExp.

#include "stdafx.h"
#include "moc_HardwareAdapterSerialPort.cpp"
#include "HardwareAdapterSerialPort.h"

namespace DynExp
{
	auto HardwareAdapterSerialPort::Enumerate()
	{
		auto SerialPortInfos = QSerialPortInfo::availablePorts();
		std::vector<std::string> SerialPortNames;

		for (int i = 0; i < Util::NumToT<int>(SerialPortInfos.length()); ++i)
			SerialPortNames.emplace_back(SerialPortInfos[i].portName().toStdString());

		return SerialPortNames;
	}

	void HardwareAdapterSerialPortParams::ConfigureParamsImpl(dispatch_tag<QSerialCommunicationHardwareAdapterParams>)
	{
		auto SerialPorts = HardwareAdapterSerialPort::Enumerate();
		if (SerialPorts.empty())
			throw Util::EmptyException("There is not any available serial port.");
		PortName.SetTextList(std::move(SerialPorts));

		ConfigureParamsImpl(dispatch_tag<HardwareAdapterSerialPortParams>());
	}

	HardwareAdapterSerialPortWorker::HardwareAdapterSerialPortWorker()
		: Port(this)
	{
		QObject::connect(&Port, &QSerialPort::readyRead, this, &HardwareAdapterSerialPortWorker::OnDataAvailable);
	}

	void HardwareAdapterSerialPortWorker::Init(QString PortName, QSerialPort::BaudRate BaudRate, QSerialPort::DataBits DataBits,
		QSerialPort::StopBits StopBits, QSerialPort::Parity Parity)
	{
		Port.setReadBufferSize(SerialCommunicationHardwareAdapter::GetMaxBufferSize());
		Port.setPort(QSerialPortInfo(PortName));
		Port.setBaudRate(BaudRate);
		Port.setDataBits(DataBits);
		Port.setStopBits(StopBits);
		Port.setParity(Parity);
	}

	void HardwareAdapterSerialPortWorker::OnDataAvailable()
	{
		Read();
	}

	bool HardwareAdapterSerialPortWorker::CheckError(const std::source_location Location) const
	{
		if (Port.error() != QSerialPort::NoError)
		{
			SetException(SerialPortException(Util::ToStr(Port.errorString()), Port.error(), Location));

			return false;
		}

		return true;
	}

	void HardwareAdapterSerialPortWorker::OpenChild()
	{
		if (!Port.isOpen())
		{
			Port.open(QIODevice::ReadWrite);
			if (CheckError())
				SetCommunicationChannelOpened();
		}
	}

	void HardwareAdapterSerialPortWorker::CloseChild()
	{
		if (Port.isOpen())
		{
			Port.clear();
			Port.close();

			SetCommunicationChannelClosed();
		}
	}

	void HardwareAdapterSerialPortWorker::ResetChild()
	{
		Close();

		Port.clearError();
	}

	void HardwareAdapterSerialPortWorker::ClearChild()
	{
		if (Port.isOpen())
			Port.clear();
	}

	void HardwareAdapterSerialPortWorker::FlushChild()
	{
		if (Port.isOpen())
			Port.flush();
	}

	void HardwareAdapterSerialPortWorker::ReadChild()
	{
		QByteArray BytesRead(Port.readAll());
		CheckError();

		if (BytesRead.size())
			DataRead(BytesRead.constData());
	}

	void HardwareAdapterSerialPortWorker::WriteChild(const QString& String)
	{
		auto StdString(String.toStdString());

		auto BytesWritten = Port.write(StdString.c_str(), StdString.size());	// crop terminating '\0'
		if (BytesWritten != Util::NumToT<decltype(BytesWritten)>(StdString.size()))
			SetException(SerialPortException("It was not possible to write the desired amount of data to a serial port.", Port.error()));
		CheckError();
	}

	void HardwareAdapterSerialPortWorker::Write_endl_Child()
	{
		auto Owner = std::dynamic_pointer_cast<const HardwareAdapterSerialPort>(GetOwner());
		if (!Owner)
			return;

		const auto LineEnding = Owner->GetLineEnding();

		auto BytesWritten = Port.write(Owner->LineEndingToChar(LineEnding).data(), Owner->GetLineEndingLength(LineEnding));
		if (BytesWritten != Owner->GetLineEndingLength(LineEnding))
			SetException(SerialPortException("It was not possible to write the desired amount of data to a serial port.", Port.error()));
		else
			Port.flush();
		CheckError();
	}

	HardwareAdapterSerialPort::HardwareAdapterSerialPort(const std::thread::id OwnerThreadID, ParamsBasePtrType&& Params)
		: QSerialCommunicationHardwareAdapter(OwnerThreadID, std::move(Params))
	{
		RegisterQTypesAsMetaTypes();
	}

	QSerialCommunicationHardwareAdapter::QWorkerPtrType HardwareAdapterSerialPort::MakeWorker()
	{
		auto Worker = std::make_unique<HardwareAdapterSerialPortWorker>();

		QObject::connect(this, &HardwareAdapterSerialPort::InitSig, Worker.get(), &HardwareAdapterSerialPortWorker::Init);

		return Worker;
	}

	void HardwareAdapterSerialPort::RegisterQTypesAsMetaTypes()
	{
		static bool AlreadyRegistered = false;

		if (!AlreadyRegistered)
		{
			qRegisterMetaType<QSerialPort::BaudRate>();
			qRegisterMetaType<QSerialPort::DataBits>();
			qRegisterMetaType<QSerialPort::StopBits>();
			qRegisterMetaType<QSerialPort::Parity>();

			AlreadyRegistered = true;
		}
	}

	void HardwareAdapterSerialPort::Init()
	{
		auto DerivedParams = dynamic_Params_cast<HardwareAdapterSerialPort>(GetParams());

		emit InitSig(QString::fromStdString(DerivedParams->PortName), DerivedParams->BaudRate,
			DerivedParams->DataBits, DerivedParams->StopBits, DerivedParams->Parity);
	}

	void HardwareAdapterSerialPort::ResetImpl(dispatch_tag<QSerialCommunicationHardwareAdapter>)
	{
		emit ResetSig();
		Init();

		ResetImpl(dispatch_tag<HardwareAdapterSerialPort>());
	}
}