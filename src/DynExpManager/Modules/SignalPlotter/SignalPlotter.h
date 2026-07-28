// This file is part of DynExp.

/**
 * @file SignalPlotter.h
 * @brief Implementation of a module to plot the samples stored in data stream instruments.
*/

#pragma once

#include "stdafx.h"
#include "DynExpCore.h"
#include "../../MetaInstruments/DataStreamInstrument.h"

namespace DynExpModule
{
	class SignalPlotter;

	class SignalPlotterData : public DynExp::QMLModuleDataBase
	{
	public:
		struct PlotInfoType
		{
			/**
			 * @brief Data type of sample values for plotting.
			*/
			using QPointFValueType = decltype(std::declval<QPointF>().x());

			/**
			 * @brief Returns the multiplier prefix associated with @p Multiplier.
			 * @return Prefix of the axis multiplier, like 'n' for nano.
			*/
			QString GetMultiplierLabel() const;

			/**
			 * @brief Removes the stored hovered sample.
			*/
			void ResetHoveredSample();

			/**
			 * @brief Joint unit of the plot's value axis.
			*/
			DynExpInstr::DataStreamInstrumentData::UnitType ValueUnit = DynExpInstr::DataStreamInstrumentData::UnitType::Arbitrary;

			/**
			 * @brief Use sample indices or time data for plot's joint x axis?
			*/
			bool IsBasicSampleTimeUsed = true;

			/**
			 * @brief Best order of magnitude to scale the plot's joint time axis with.
			*/
			unsigned int Multiplier = 0;

			/**
			 * @brief Current lower axes limits of x and y axes.
			*/
			QPointF MinValues;

			/**
			 * @brief Current upper axes limits of x and y axes.
			*/
			QPointF MaxValues;

			/**
			 * @brief Lower axes limits of x and y axes from previous graph update.
			*/
			QPointF LastMinValues;

			/**
			 * @brief Upper axes limits of x and y axes from previous graph update.
			*/
			QPointF LastMaxValues;

			/**
			 * @brief Relative position of the mouse cursor inside the coordinate system (between 0. and 1.).
			*/
			QPointF CursorPosition;

			/**
			 * @brief Hovered point belonging to any series in normalized coordinate system coordinates (between 0. and 1.).
			*/
			QPointF HoveredPoint;

			/**
			 * @brief Hovered sample belonging to any series in the graphs's real coordinate system.
			*/
			QPointF HoveredSample;

			/**
			 * @brief Index of the data series the hovered point belongs to.
			*/
			size_t HoveredSeries = 0;

			/**
			 * @brief Distance between the mouse cursor and the hovered point.
			*/
			typename SignalPlotterData::PlotInfoType::QPointFValueType HoveredDistance = std::numeric_limits<typename SignalPlotterData::PlotInfoType::QPointFValueType>::max();
		};

		struct SampleDataType
		{
			/**
			 * @brief Data points of a single data series to plot.
			*/
			QList<QPointF> Samples;

			/**
			 * @brief Determines whether this series should be processed and plotted.
			*/
			bool Visible = true;
		};

		SignalPlotterData();
		virtual ~SignalPlotterData() = default;

		void LockInstruments(DynExp::ModuleInstance* Instance, const DynExp::ParamsBase::ListParam<DynExp::ObjectLink<DynExpInstr::DataStreamInstrument>>& InstrParam) { Instance->LockObject(InstrParam, DataStreamInstr); }
		void UnlockInstruments(DynExp::ModuleInstance* Instance) { Instance->UnlockObject(DataStreamInstr); }

		auto& GetDataStreamInstr(size_t Index) { return DataStreamInstr[Index]; }
		auto GetDataStreamInstrCount() const noexcept { return DataStreamInstr.GetList().size(); }
		const auto& GetDataStreamInstrLabels() const noexcept { return DataStreamInstr.GetLabels(); }
		std::string_view GetDataStreamInstrIconPath() const { return DataStreamInstr.GetIconPath(); }

		bool IsUIInitialized() const noexcept { return UIInitialized; }
		void SetUIInitialized() noexcept { UIInitialized = true; }

		bool Running;
		bool RollingView;
		bool Autoscale;

		PlotInfoType PlotInfo;
		std::vector<SampleDataType> SampleDataList;

	private:
		void ResetImpl(dispatch_tag<QMLModuleDataBase>) override final;
		virtual void ResetImpl(dispatch_tag<SignalPlotterData>) {};

		void Init();

		bool UIInitialized;

		DynExp::LinkedObjectWrapperContainerList<DynExpInstr::DataStreamInstrument> DataStreamInstr;
	};

	class SignalPlotterParams : public DynExp::QMLModuleParamsBase
	{
	public:
		SignalPlotterParams(DynExp::ItemIDType ID, const DynExp::DynExpCore& Core) : QMLModuleParamsBase(ID, Core) {}
		virtual ~SignalPlotterParams() = default;

		virtual const char* GetParamClassTag() const noexcept override { return "SignalPlotterParams"; }

		ListParam<DynExp::ObjectLink<DynExpInstr::DataStreamInstrument>> DataStreamInstr = { *this, GetCore().GetInstrumentManager(),
			"DataStreamInstr", "Data stream instrument(s)", "Underlying data stream instrument(s) to be used as a data source(s)", DynExpUI::Icons::Instrument };

		Param<bool> RollingView = { *this, "RollingView", false };
		Param<bool> Autoscale = { *this, "Autoscale", true };

	private:
		void ConfigureParamsImpl(dispatch_tag<QMLModuleParamsBase>) override final {}
	};

	class SignalPlotterConfigurator : public DynExp::QMLModuleConfiguratorBase
	{
	public:
		using ObjectType = SignalPlotter;
		using ParamsType = SignalPlotterParams;

		SignalPlotterConfigurator() = default;
		virtual ~SignalPlotterConfigurator() = default;

	private:
		virtual DynExp::ParamsBasePtrType MakeParams(DynExp::ItemIDType ID, const DynExp::DynExpCore& Core) const override final { return DynExp::MakeParams<SignalPlotterConfigurator>(ID, Core); }
	};

	class SignalPlotter : public DynExp::QMLModuleBase
	{
	public:
		using ParamsType = SignalPlotterParams;
		using ConfigType = SignalPlotterConfigurator;
		using ModuleDataType = SignalPlotterData;

		constexpr static auto Name() noexcept { return "Signal Plotter"; }
		constexpr static auto Category() noexcept { return "I/O"; }

		SignalPlotter(const std::thread::id OwnerThreadID, DynExp::ParamsBasePtrType&& Params)
			: QMLModuleBase(OwnerThreadID, std::move(Params)) {}
		virtual ~SignalPlotter() = default;

		virtual std::string GetName() const override { return Name(); }
		virtual std::string GetCategory() const override { return Category(); }

		std::chrono::milliseconds GetMainLoopDelay() const override final { return std::chrono::milliseconds(50); }

	private:
		Util::DynExpErrorCodes::DynExpErrorCodes ModuleMainLoop(DynExp::ModuleInstance& Instance) override final;

		void ResetImpl(dispatch_tag<QMLModuleBase>) override final;

		void MakeConnections(QObject* Backend) override final;
		QAnyStringView GetModuleSourceUri() const noexcept override final { return "Modules.SignalPlotter"; }
		QAnyStringView GetModuleSourceTypeName() const noexcept override final { return "SignalPlotter"; }
		void UpdateUIChild(const ModuleBase::ModuleDataGetterType& ModuleDataGetter) override final;

		/**
		 * @brief Extracts sample timing information for x axis scaling from @p BasicSamples and stores the
		 * results in @p PlotInfo.
		 * @param BasicSamplesSeries Vector of vectors of BasicSamples to investigate. The outer vector represents a
		 * list of data series. The funcion may sort the entries of @p BasicSamples according to their x values in
		 * ascending order.
		 * @param PlotInfo Information about data series in @p BasicSamples. Refer to SignalPlotterData::PlotInfoType.
		 * Takes @p PlotInfo as a parameter and not from @p SignalPlotterData in order not to lock module data while
		 * performing potentially heavy calculation. 
		*/
		void GenerateSampleTimingInfo(std::vector<DynExpInstr::DataStreamBase::BasicSampleListType>& BasicSamplesSeries,
			SignalPlotterData::PlotInfoType& PlotInfo);

		/**
		 * @brief Converts @p BasicSamples to displayable format and stores their minimal and maximal values in @p PlotInfo.
		 * @param BasicSamples Vector of BasicSamples to convert. Samples are moved from this vector.
		 * @param SampleData Destiny to store the processed samples in.
		 * @param PlotInfo Information about data series in @p BasicSamples. Refer to SignalPlotterData::PlotInfoType.
		 * Takes @p PlotInfo as a parameter and not from @p SignalPlotterData in order not to lock module data while
		 * performing potentially heavy calculation.
		 * @param SeriesIndex Index of the data series that is to be processed.
		 * @return Returns true if at least one sample has been processed, false otherwise.
		*/
		bool ProcessBasicSamples(const DynExpInstr::DataStreamBase::BasicSampleListType& BasicSamples,
			SignalPlotterData::SampleDataType& SampleData, SignalPlotterData::PlotInfoType& PlotInfo, const size_t SeriesIndex);

		/**
		 * @brief If data plotting is not enabled (running), the available and already plotted samples have to be
		 * reprocessed, e.g. for data sample hovering detection.
		 * @param SampleData Processed samples to reprocess.
		 * @param PlotInfo Information about data series in @p SampleData. Refer to SignalPlotterData::PlotInfoType.
		 * Takes @p PlotInfo as a parameter and not from @p SignalPlotterData in order not to lock module data while
		 * performing potentially heavy calculation.
		 * @param SeriesIndex Index of the data series that is to be processed.
		*/
		void ReprocessSamples(const SignalPlotterData::SampleDataType& SampleData,
			SignalPlotterData::PlotInfoType& PlotInfo, const size_t SeriesIndex);

		/**
		 * @brief Checks whether the given sample is close to the mouse cursor (hovered).
		 * @param X x value of the sample.
		 * @param Y y value of the sample.
		 * @param PlotInfo Information about data series in @p SampleData. Refer to SignalPlotterData::PlotInfoType.
		 * @param SeriesIndex Index of the data series the sample belongs to.
		*/
		void CheckSampleHovered(const typename SignalPlotterData::PlotInfoType::QPointFValueType X,
			const typename SignalPlotterData::PlotInfoType::QPointFValueType Y,
			SignalPlotterData::PlotInfoType& PlotInfo, const size_t SeriesIndex);

		// Events, run in module thread
		void OnInit(DynExp::ModuleInstance* Instance) const override final;
		void OnExit(DynExp::ModuleInstance* Instance) const override final;
		void OnRollingViewChanged(DynExp::ModuleInstance* Instance, bool State) const;
		void OnAutoscaleChanged(DynExp::ModuleInstance* Instance, bool State) const;
		void OnClearStream(DynExp::ModuleInstance* Instance) const;

		// Events, run in UI thread
		void OnSaveData() const;

		void FinishedSavingData() const noexcept { IsSavingData = false; }
		using FinishedSavingDataGuardType = Util::OnDestruction<const SignalPlotter, decltype(&SignalPlotter::FinishedSavingData)>;

		/**
		 * @brief If data is currently being saved to file, do not update internal data.
		*/
		mutable std::atomic_bool IsSavingData = false;

		size_t NumFailedUpdateAttempts = 0;

		/**
		 * @brief Does nothing but ensures that this module's Qt connections are disconnected when
		 * @p SignalContext is destroyed. Moreover, @p SignalContext is created in the UI thread
		 * (that creates this module instance). Hence, it ensures that Qt connections using this
		 * context are executed in the UI thread.
		*/
		QObject SignalContext;
	};
}