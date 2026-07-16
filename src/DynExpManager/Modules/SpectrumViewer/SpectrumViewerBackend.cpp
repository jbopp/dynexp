// This file is part of DynExp.

#include "stdafx.h"
#include "SpectrumViewerBackend.h"

namespace DynExpModule::SpectrumViewer
{
	SpectrumViewerBackend::SpectrumViewerBackend(QObject* parent) : QObject(parent)
	{
	}

	void SpectrumViewerBackend::SetSilent(bool Silent) noexcept
	{
		if (this->Silent == Silent)
			return;

		this->Silent = Silent;

		emit qsilentChanged(Silent);
	}

	void SpectrumViewerBackend::SetExposureTime(int ExposureTime) noexcept
	{
		if (this->ExposureTime == ExposureTime)
			return;

		this->ExposureTime = ExposureTime;

		emit qexposureTimeChanged(ExposureTime);
	}

	void SpectrumViewerBackend::SetLowerLimit(double LowerLimit) noexcept
	{
		if (this->LowerLimit == LowerLimit)
			return;

		this->LowerLimit = LowerLimit;

		emit qlowerLimitChanged(LowerLimit);
	}

	void SpectrumViewerBackend::SetUpperLimit(double UpperLimit) noexcept
	{
		if (this->UpperLimit == UpperLimit)
			return;

		this->UpperLimit = UpperLimit;

		emit qupperLimitChanged(UpperLimit);
	}

	DynExpQuick::DynExpLineGraphBackend* SpectrumViewerBackend::GetGraph() const
	{
		if (!Graph)
			throw Util::InvalidStateException("Graph backend has not been assigned yet.");

		return Graph;
	}

	DynExpQuick::DynExpLineGraphBackend* SpectrumViewerBackend::GetGraph()
	{
		return const_cast<DynExpQuick::DynExpLineGraphBackend*>(std::as_const(*this).GetGraph());
	}

	Q_INVOKABLE void SpectrumViewerBackend::SetGraphBackend(QObject* Object)
	{
		Graph = qobject_cast<DynExpQuick::DynExpLineGraphBackend*>(Object);
	}
}