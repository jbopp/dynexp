// This file is part of DynExp.

/**
 * @file SpectrumViewerBackend.h
 * @brief C++ backend of the SpectrumViewer QML module.
*/

#pragma once

#include <qqml.h>

#include "stdafx.h"
#include "qml/QDynExpLineGraph.h"

namespace DynExpModule::SpectrumViewer
{
	class SpectrumViewerBackend : public QObject
	{
		Q_OBJECT
		QML_ELEMENT

		Q_PROPERTY(bool Silent READ IsSilent WRITE SetSilent NOTIFY qsilentChanged)
		Q_PROPERTY(int ExposureTime READ GetExposureTime WRITE SetExposureTime NOTIFY qexposureTimeChanged)
		Q_PROPERTY(double LowerLimit READ GetLowerLimit WRITE SetLowerLimit NOTIFY qlowerLimitChanged)
		Q_PROPERTY(double UpperLimit READ GetUpperLimit WRITE SetUpperLimit NOTIFY qupperLimitChanged)

	public:
		SpectrumViewerBackend(QObject* parent = nullptr);
		~SpectrumViewerBackend() = default;

		bool IsSilent() const noexcept { return Silent; }
		void SetSilent(bool Silent) noexcept;
		int GetExposureTime() const noexcept { return ExposureTime; }
		void SetExposureTime(int ExposureTime) noexcept;
		double GetLowerLimit() const noexcept { return LowerLimit; }
		void SetLowerLimit(double LowerLimit) noexcept;
		double GetUpperLimit() const noexcept { return UpperLimit; }
		void SetUpperLimit(double UpperLimit) noexcept;

		DynExpQuick::DynExpLineGraphBackend* GetGraph() const;
		DynExpQuick::DynExpLineGraphBackend* GetGraph();

		Q_INVOKABLE void SetGraphBackend(QObject* Object);

	signals:
		// to QML
		void qsilentChanged(bool);
		void qexposureTimeChanged(int);
		void qlowerLimitChanged(double);
		void qupperLimitChanged(double);

		// from QML
		void saveData();
		void runClicked();
		void stopClicked();
		void silentChanged(bool);
		void exposureTimeChanged(int);
		void lowerLimitChanged(double);
		void upperLimitChanged(double);

	private:
		bool Silent = false;
		int ExposureTime = 0;
		double LowerLimit = .0;
		double UpperLimit = .0;

		DynExpQuick::DynExpLineGraphBackend* Graph = nullptr;
	};
}