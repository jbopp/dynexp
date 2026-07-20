#include "QDynExpSurfaceGraph.h"
// This file is part of DynExp.

#include "QDynExpSurfaceGraph.h"

namespace DynExpQuick
{
	DynExpSurfaceGraphBackend::DynExpSurfaceGraphBackend(QObject* parent) : QObject(parent),
		XValueAxis(std::make_unique<QValue3DAxis>()), YValueAxis(std::make_unique<QValue3DAxis>()),
		ZValueAxis(std::make_unique<QValue3DAxis>())
	{
	}

	void DynExpSurfaceGraphBackend::SetSelectedPoint(QPoint SelectedPoint) noexcept
	{
		if (this->SelectedPoint == SelectedPoint)
			return;

		this->SelectedPoint = SelectedPoint;

		emit selectedPointChanged(SelectedPoint);
	}

	void DynExpQuick::DynExpSurfaceGraphBackend::UpdateData()
	{
		emit qdataChanged();
	}
}