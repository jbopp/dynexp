// This file is part of DynExp.

/**
 * @file Stage1D.h
 * @brief Implementation of a module to control a 1D positioner stage.
*/

#pragma once

#include "stdafx.h"
#include "DynExpCore.h"
#include "../MetaInstruments/Stage.h"

#include <QWidget>
#include "ui_Stage1D.h"

namespace DynExpModule
{
	class Stage1D;

	class Stage1DWidget : public DynExp::QModuleWidget
	{
		Q_OBJECT

	public:
		Stage1DWidget(Stage1D& Owner, QModuleWidget* parent = nullptr);
		~Stage1DWidget() = default;

		Ui::Stage1D ui;
	};

	class Stage1DData : public DynExp::QModuleDataBase
	{
	public:
		Stage1DData() { Init(); }
		virtual ~Stage1DData() = default;

		DynExp::LinkedObjectWrapperContainer<DynExpInstr::PositionerStage> PositionerStage;

		double Velocity;
		double Position;
		bool IsUsingSIUnits;
		bool IsMoving;
		bool HasFailed;
		bool LabelsUpdated;

	private:
		void ResetImpl(dispatch_tag<QModuleDataBase>) override final;
		virtual void ResetImpl(dispatch_tag<Stage1DData>) {};

		void Init();
	};

	class Stage1DParams : public DynExp::QModuleParamsBase
	{
	public:
		Stage1DParams(DynExp::ItemIDType ID, const DynExp::DynExpCore& Core) : QModuleParamsBase(ID, Core) {}
		virtual ~Stage1DParams() = default;

		virtual const char* GetParamClassTag() const noexcept override { return "Stage1DParams"; }

		Param<DynExp::ObjectLink<DynExpInstr::PositionerStage>> PositionerStage = { *this, GetCore().GetInstrumentManager(),
			"PositionerStage", "Positioner", "Underlying positioner to be controlled by this module", DynExpUI::Icons::Instrument };

	private:
		void ConfigureParamsImpl(dispatch_tag<QModuleParamsBase>) override final {}
	};

	class Stage1DConfigurator : public DynExp::QModuleConfiguratorBase
	{
	public:
		using ObjectType = Stage1D;
		using ParamsType = Stage1DParams;

		Stage1DConfigurator() = default;
		virtual ~Stage1DConfigurator() = default;

	private:
		virtual DynExp::ParamsBasePtrType MakeParams(DynExp::ItemIDType ID, const DynExp::DynExpCore& Core) const override final { return DynExp::MakeParams<Stage1DConfigurator>(ID, Core); }
	};

	class Stage1D : public DynExp::QModuleBase
	{
	public:
		using ParamsType = Stage1DParams;
		using ConfigType = Stage1DConfigurator;
		using ModuleDataType = Stage1DData;

		constexpr static auto Name() noexcept { return "1D Positioner Control"; }
		constexpr static auto Category() noexcept { return "Motion Control"; }

		Stage1D(const std::thread::id OwnerThreadID, DynExp::ParamsBasePtrType&& Params)
			: QModuleBase(OwnerThreadID, std::move(Params)) {}
		virtual ~Stage1D() = default;

		virtual std::string GetName() const override { return Name(); }
		virtual std::string GetCategory() const override { return Category(); }

		std::chrono::milliseconds GetMainLoopDelay() const override final { return std::chrono::milliseconds(20); }

	private:
		Util::DynExpErrorCodes::DynExpErrorCodes ModuleMainLoop(DynExp::ModuleInstance& Instance) override final;

		void ResetImpl(dispatch_tag<QModuleBase>) override final;

		std::unique_ptr<DynExp::QModuleWidget> MakeUIWidget() override final;
		void UpdateUIChild(const ModuleBase::ModuleDataGetterType& ModuleDataGetter) override final;

		// Events, run in module thread
		void OnInit(DynExp::ModuleInstance* Instance) const override final;
		void OnExit(DynExp::ModuleInstance* Instance) const override final;
		void OnFindReferenceClicked(DynExp::ModuleInstance* Instance, bool) const;
		void OnSetHomeClicked(DynExp::ModuleInstance* Instance, bool) const;
		void OnCalibrateClicked(DynExp::ModuleInstance* Instance, bool) const;
		void OnMoveFirstClicked(DynExp::ModuleInstance* Instance, bool) const;
		void OnMoveLastClicked(DynExp::ModuleInstance* Instance, bool) const;
		void OnMoveLeftClicked(DynExp::ModuleInstance* Instance, bool) const;
		void OnMoveRightClicked(DynExp::ModuleInstance* Instance, bool) const;
		void OnMoveHomeClicked(DynExp::ModuleInstance* Instance, bool) const;
		void OnStopClicked(DynExp::ModuleInstance* Instance, bool) const;
		void OnVelocityValueChanged(DynExp::ModuleInstance* Instance, const double Value) const;
		void OnPositionValueChanged(DynExp::ModuleInstance* Instance, const double Value) const;

		size_t NumFailedUpdateAttempts = 0;
	};
}