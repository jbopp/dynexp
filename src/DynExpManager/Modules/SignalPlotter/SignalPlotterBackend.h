// This file is part of DynExp.

/**
 * @file SignalPlotterBackend.h
 * @brief C++ backend of the SignalPlotter QML module.
*/

#pragma once

#include <qqml.h>

#include "stdafx.h"
#include "qml/QDynExpLineGraph.h"

namespace DynExpModule::SignalPlotter
{
	class SignalPlotterBackend : public QObject
	{
		Q_OBJECT
		QML_ELEMENT

		Q_PROPERTY(bool Running READ IsRunning WRITE SetRunning NOTIFY runningChanged)

	public:
		SignalPlotterBackend(QObject* parent = nullptr);
		~SignalPlotterBackend() = default;

		bool IsRunning() const noexcept { return Running; }
		void SetRunning(bool Running) noexcept;

		DynExpQuick::DynExpLineGraphBackend* GetGraph() const;
		DynExpQuick::DynExpLineGraphBackend* GetGraph();

		void SetRollingView(bool State) { emit qrollingViewChanged(State); }
		void SetAutoscale(bool State) { emit qautoscaleChanged(State); }

		Q_INVOKABLE void SetGraphBackend(QObject* Object);

	signals:
		void runningChanged(bool);

		// to QML
		void qrollingViewChanged(bool);
		void qautoscaleChanged(bool);

		// from QML
		void saveData();
		void rollingViewChanged(bool);
		void autoscaleChanged(bool);
		void clearStream();

	private:
		bool Running = true;

		DynExpQuick::DynExpLineGraphBackend* Graph = nullptr;
	};
}