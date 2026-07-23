// This file is part of DynExp.

/**
 * @file QDynExpSurfaceGraph.h
 * @brief C++ backend of the QDynExpSurfaceGraph QML component.
*/

#pragma once

#include <qqml.h>
#include <QtGraphs/QSurface3DSeries>
#include <QtGraphs/QValue3DAxis>

namespace DynExpQuick
{
	class DynExpSurfaceGraphBackend : public QObject
	{
		Q_OBJECT
		QML_ELEMENT

		Q_PROPERTY(QValue3DAxis* XValueAxis READ GetXValueAxis CONSTANT)
		Q_PROPERTY(QValue3DAxis* YValueAxis READ GetYValueAxis CONSTANT)
		Q_PROPERTY(QValue3DAxis* ZValueAxis READ GetZValueAxis CONSTANT)
		Q_PROPERTY(QPoint SelectedPoint READ GetSelectedPoint WRITE SetSelectedPoint NOTIFY selectedPointChanged)

	public:
		DynExpSurfaceGraphBackend(QObject* parent = nullptr);
		~DynExpSurfaceGraphBackend() = default;

		const auto* GetProxy() const noexcept { return Proxy; }
		Q_INVOKABLE QSurfaceDataProxy* GetProxy() noexcept { return Proxy; }
		const auto* GetXValueAxis() const noexcept { return XValueAxis.get(); }
		auto* GetXValueAxis() noexcept { return XValueAxis.get(); }
		const auto* GetYValueAxis() const noexcept { return YValueAxis.get(); }
		auto* GetYValueAxis() noexcept { return YValueAxis.get(); }
		const auto* GetZValueAxis() const noexcept { return ZValueAxis.get(); }
		auto* GetZValueAxis() noexcept { return ZValueAxis.get(); }

		Q_INVOKABLE QVector3D GetDataItemPosition() const;

		auto GetSelectedPoint() const noexcept { return SelectedPoint; }
		void SetSelectedPoint(QPoint SelectedPoint) noexcept;
		void ResetSelectedPoint() noexcept;

		void UpdateData();
		void ResetCamera() { emit qresetCamera(); }

	signals:
		void selectedPointChanged(QPoint);

		// to QML
		void qdataChanged();
		void qresetCamera();

	private:

		QSurfaceDataProxy* const Proxy = nullptr;
		const std::unique_ptr<QValue3DAxis> XValueAxis;
		const std::unique_ptr<QValue3DAxis> YValueAxis;
		const std::unique_ptr<QValue3DAxis> ZValueAxis;

		QPoint SelectedPoint;
	};
}