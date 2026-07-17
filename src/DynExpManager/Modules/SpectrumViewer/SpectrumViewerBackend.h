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

		Q_PROPERTY(bool SilentFocused MEMBER SilentFocused READ IsSilentFocused)
		Q_PROPERTY(QPoint ExposureTimeRange READ GetExposureTimeRange WRITE SetExposureTimeRange NOTIFY rangesChanged)
		Q_PROPERTY(QString ExposureTimeUnit READ GetExposureTimeUnit WRITE SetExposureTimeUnit NOTIFY rangesChanged)
		Q_PROPERTY(bool ExposureTimeFocused MEMBER ExposureTimeFocused READ IsExposureTimeFocused)
		Q_PROPERTY(QPointF LimitRange READ GetLimitRange WRITE SetLimitRange NOTIFY rangesChanged)
		Q_PROPERTY(QString LimitUnit READ GetLimitUnit WRITE SetLimitUnit NOTIFY rangesChanged)
		Q_PROPERTY(bool LowerLimitFocused MEMBER LowerLimitFocused READ IsLowerLimitFocused)
		Q_PROPERTY(bool UpperLimitFocused MEMBER UpperLimitFocused READ IsUpperLimitFocused)
		Q_PROPERTY(double Progress READ GetProgress WRITE SetProgress NOTIFY progressChanged)
		Q_PROPERTY(StateType State READ GetState WRITE SetState NOTIFY stateChanged)

		Q_PROPERTY(bool Silent READ IsSilent WRITE SetSilent NOTIFY silentChanged)
		Q_PROPERTY(int ExposureTime READ GetExposureTime WRITE SetExposureTime NOTIFY exposureTimeChanged)
		Q_PROPERTY(double LowerLimit READ GetLowerLimit WRITE SetLowerLimit NOTIFY lowerLimitChanged)
		Q_PROPERTY(double UpperLimit READ GetUpperLimit WRITE SetUpperLimit NOTIFY upperLimitChanged)

	public:
		enum StateType { Ready, Warning, Error, Capturing };
		Q_ENUM(StateType)

		SpectrumViewerBackend(QObject* parent = nullptr);
		~SpectrumViewerBackend() = default;

		bool IsSilentFocused() const noexcept { return SilentFocused; }
		QPoint GetExposureTimeRange() const noexcept { return ExposureTimeRange; }
		void SetExposureTimeRange(QPoint Range) noexcept;
		QString GetExposureTimeUnit() const noexcept { return ExposureTimeUnit; }
		void SetExposureTimeUnit(QString Unit) noexcept;
		bool IsExposureTimeFocused() const noexcept { return ExposureTimeFocused; }
		QPointF GetLimitRange() const noexcept { return LimitRange; }
		void SetLimitRange(QPointF Range) noexcept;
		QString GetLimitUnit() const noexcept { return LimitUnit; }
		void SetLimitUnit(QString Unit) noexcept;
		bool IsLowerLimitFocused() const noexcept { return LowerLimitFocused; }
		bool IsUpperLimitFocused() const noexcept { return UpperLimitFocused; }
		double GetProgress() const noexcept { return Progress; }
		void SetProgress(double Progress) noexcept;
		StateType GetState() const noexcept { return State; }
		void SetState(StateType State) noexcept;

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
		void saveData();
		void runClicked();
		void stopClicked();
		void silentChanged(bool);
		void exposureTimeChanged(int);
		void lowerLimitChanged(double);
		void upperLimitChanged(double);
		void rangesChanged();
		void progressChanged(double);
		void stateChanged(StateType);

	private:
		bool SilentFocused = false;
		QPoint ExposureTimeRange = { 0, 1 };
		QString ExposureTimeUnit;
		bool ExposureTimeFocused = false;
		QPointF LimitRange = { 0., 1. };
		QString LimitUnit;
		bool LowerLimitFocused = false;
		bool UpperLimitFocused = false;
		double Progress = 0;
		StateType State = StateType::Ready;

		bool Silent = false;
		int ExposureTime = 0;
		double LowerLimit = .0;
		double UpperLimit = .0;

		DynExpQuick::DynExpLineGraphBackend* Graph = nullptr;
	};
}