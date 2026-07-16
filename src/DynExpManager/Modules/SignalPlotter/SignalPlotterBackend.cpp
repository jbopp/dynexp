// This file is part of DynExp.

#include "stdafx.h"
#include "SignalPlotterBackend.h"

namespace DynExpModule::SignalPlotter
{
	SignalPlotterBackend::SignalPlotterBackend(QObject* parent) : QObject(parent)
	{
	}

	void SignalPlotterBackend::SetRunning(bool Running) noexcept
	{
		if (this->Running == Running)
			return;

		this->Running = Running;
		
		emit qrunningChanged(Running);
	}

	DynExpQuick::DynExpLineGraphBackend* SignalPlotterBackend::GetGraph() const
	{
		if (!Graph)
			throw Util::InvalidStateException("Graph backend has not been assigned yet.");

		return Graph;
	}

	DynExpQuick::DynExpLineGraphBackend* SignalPlotterBackend::GetGraph()
	{
		return const_cast<DynExpQuick::DynExpLineGraphBackend*>(std::as_const(*this).GetGraph());
	}

	Q_INVOKABLE void SignalPlotterBackend::SetGraphBackend(QObject* Object)
	{
		Graph = qobject_cast<DynExpQuick::DynExpLineGraphBackend*>(Object);
	}
}