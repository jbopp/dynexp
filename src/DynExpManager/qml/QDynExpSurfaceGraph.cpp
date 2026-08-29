// This file is part of DynExp.

#include "QDynExpSurfaceGraph.h"

namespace DynExpQuick
{
	DynExpSurfaceGraphBackend::DynExpSurfaceGraphBackend(QObject* parent) : QObject(parent),
		Proxy(new QSurfaceDataProxy()),
		XValueAxis(std::make_unique<QValue3DAxis>()), YValueAxis(std::make_unique<QValue3DAxis>()),
		ZValueAxis(std::make_unique<QValue3DAxis>())
	{
		// QSurface3DSeries picks up proxy and transfers ownership to it via qml
	}

	void DynExpSurfaceGraphBackend::SetSelectedPoint(QPoint SelectedPoint) noexcept
	{
		if (this->SelectedPoint == SelectedPoint)
			return;

		this->SelectedPoint = SelectedPoint;

		emit selectedPointChanged(SelectedPoint);
	}

	void DynExpSurfaceGraphBackend::ResetSelectedPoint() noexcept
	{
		SetSelectedPoint({ -1, -1 });
	}

	void DynExpQuick::DynExpSurfaceGraphBackend::UpdateData()
	{
		emit qdataChanged();
	}

	QVector3D DynExpSurfaceGraphBackend::GetDataItemPosition() const
	{
		if (SelectedPoint.x() >= 0 && SelectedPoint.x() < Proxy->rowCount() &&
			SelectedPoint.y() >= 0 && SelectedPoint.y() < Proxy->columnCount())
			return Proxy->itemAt(SelectedPoint).position();
		else
			return QVector3D();
	}
}