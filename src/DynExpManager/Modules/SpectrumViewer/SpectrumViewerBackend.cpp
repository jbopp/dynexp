// This file is part of DynExp.

#include "stdafx.h"
#include "SpectrumViewerBackend.h"

namespace DynExpModule::SpectrumViewer
{
	SpectrumViewerBackend::SpectrumViewerBackend(QObject* parent) : QObject(parent)
	{
	}

	void SpectrumViewerBackend::SetExposureTimeRange(QPoint Range) noexcept
	{
		if (ExposureTimeRange == Range)
			return;

		ExposureTimeRange = Range;

		emit rangesChanged();
	}

	void SpectrumViewerBackend::SetExposureTimeUnit(QString Unit) noexcept
	{
		if (ExposureTimeUnit == Unit)
			return;

		ExposureTimeUnit = Unit;

		emit rangesChanged();
	}

	void SpectrumViewerBackend::SetLimitRange(QPointF Range) noexcept
	{
		if (LimitRange == Range)
			return;

		LimitRange = Range;

		emit rangesChanged();
	}

	void SpectrumViewerBackend::SetLimitUnit(QString Unit) noexcept
	{
		if (LimitUnit == Unit)
			return;

		LimitUnit = Unit;

		emit rangesChanged();
	}

	void SpectrumViewerBackend::SetProgress(double Progress) noexcept
	{
		if (this->Progress == Progress)
			return;

		this->Progress = Progress;

		emit progressChanged(Progress);
	}

	void SpectrumViewerBackend::SetState(StateType State) noexcept
	{
		if (this->State == State)
			return;

		this->State = State;

		emit stateChanged(State);
	}

	void SpectrumViewerBackend::SetSilent(bool Silent) noexcept
	{
		if (this->Silent == Silent)
			return;

		this->Silent = Silent;

		emit silentChanged(Silent);
	}

	void SpectrumViewerBackend::SetExposureTime(int ExposureTime) noexcept
	{
		if (this->ExposureTime == ExposureTime)
			return;

		this->ExposureTime = ExposureTime;

		emit exposureTimeChanged(ExposureTime);
	}

	void SpectrumViewerBackend::SetLowerLimit(double LowerLimit) noexcept
	{
		if (this->LowerLimit == LowerLimit)
			return;

		this->LowerLimit = LowerLimit;

		emit lowerLimitChanged(LowerLimit);
	}

	void SpectrumViewerBackend::SetUpperLimit(double UpperLimit) noexcept
	{
		if (this->UpperLimit == UpperLimit)
			return;

		this->UpperLimit = UpperLimit;

		emit upperLimitChanged(UpperLimit);
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