// This file is part of DynExp.

/**
 * @file QDynExpSurfaceGraph.h
 * @brief C++ backend of the QDynExpSurfaceGraph QML component.
*/

#pragma once

#include <qqml.h>
#include <QtGraphsWidgets/Q3DSurfaceWidgetItem>

namespace DynExpQuick
{
	class DynExpSurfaceGraphBackend : public QObject
	{
		Q_OBJECT
		QML_ELEMENT

		Q_PROPERTY(QValue3DAxis* XValueAxis READ GetXValueAxis CONSTANT)
		Q_PROPERTY(QValue3DAxis* YValueAxis READ GetYValueAxis CONSTANT)
		Q_PROPERTY(QValue3DAxis* ZValueAxis READ GetZValueAxis CONSTANT)
		Q_PROPERTY(QString ItemLabelFormat READ GetItemLabelFormat WRITE SetItemLabelFormat NOTIFY itemLabelFormatChanged)
		Q_PROPERTY(QPoint SelectedPoint READ GetSelectedPoint WRITE SetSelectedPoint NOTIFY selectedPointChanged)

	public:
		DynExpSurfaceGraphBackend(QObject* parent = nullptr);
		~DynExpSurfaceGraphBackend() = default;

		auto GetXValueAxis() const noexcept { return XValueAxis.get(); }
		auto GetXValueAxis() noexcept { return XValueAxis.get(); }
		auto GetYValueAxis() const noexcept { return YValueAxis.get(); }
		auto GetYValueAxis() noexcept { return YValueAxis.get(); }
		auto GetZValueAxis() const noexcept { return ZValueAxis.get(); }
		auto GetZValueAxis() noexcept { return ZValueAxis.get(); }

		auto GetItemLabelFormat() const noexcept { return ItemLabelFormat; }
		void SetItemLabelFormat(QString ItemLabelFormat) noexcept;
		auto GetSelectedPoint() const noexcept { return SelectedPoint; }
		void SetSelectedPoint(QPoint SelectedPoint) noexcept;

		void SetProxy(QSurfaceDataProxy* Proxy) { emit qproxyChanged(Proxy); }
		void UpdateData();
		void ResetCamera() { emit qresetCamera(); }

	signals:
		void itemLabelFormatChanged(QString);
		void selectedPointChanged(QPoint);

		// to QML
		void qproxyChanged(QSurfaceDataProxy*);
		void qdataChanged();
		void qresetCamera();

	private:
		const std::unique_ptr<QValue3DAxis> XValueAxis;
		const std::unique_ptr<QValue3DAxis> YValueAxis;
		const std::unique_ptr<QValue3DAxis> ZValueAxis;

		QString ItemLabelFormat;
		QPoint SelectedPoint;
	};
}