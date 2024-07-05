// This file is part of DynExp.

/**
 * @file HardwareAdapterSmarAct.h
 * @brief Implementation of a hardware adapter to control SmarAct MCS2
 * hardware.
*/

#pragma once

#include "stdafx.h"
#include "HardwareAdapter.h"
#include "../MetaInstruments/Stage.h"

namespace DynExpHardware::SmarActSyms
{
	#include "../include/SmarAct/SmarActControl.h"
}

namespace DynExpHardware
{
	class SmarActHardwareAdapter;

	class SmarActException : public Util::Exception
	{
	public:
		SmarActException(std::string Description, const int ErrorCode,
			const std::source_location Location = std::source_location::current()) noexcept
			: Exception(std::move(Description), Util::ErrorType::Error, ErrorCode, Location)
		{}
	};

	class SmarActHardwareAdapterParams : public DynExp::HardwareAdapterParamsBase
	{
	public:
		SmarActHardwareAdapterParams(DynExp::ItemIDType ID, const DynExp::DynExpCore& Core) : HardwareAdapterParamsBase(ID, Core) {}
		virtual ~SmarActHardwareAdapterParams() = default;

		virtual const char* GetParamClassTag() const noexcept override { return "SmarActHardwareAdapterParams"; }

		Param<TextList> DeviceDescriptor = { *this, {}, "DeviceDescriptor", "Device descriptor",
			"Descriptor of the SmarAct controller to connect with" };

	private:
		void ConfigureParamsImpl(dispatch_tag<HardwareAdapterParamsBase>) override final;
		virtual void ConfigureParamsImpl(dispatch_tag<SmarActHardwareAdapterParams>) {}
	};

	class SmarActHardwareAdapterConfigurator : public DynExp::HardwareAdapterConfiguratorBase
	{
	public:
		using ObjectType = SmarActHardwareAdapter;
		using ParamsType = SmarActHardwareAdapterParams;

		SmarActHardwareAdapterConfigurator() = default;
		virtual ~SmarActHardwareAdapterConfigurator() = default;

	private:
		virtual DynExp::ParamsBasePtrType MakeParams(DynExp::ItemIDType ID, const DynExp::DynExpCore& Core) const override { return DynExp::MakeParams<SmarActHardwareAdapterConfigurator>(ID, Core); }
	};

	class SmarActHardwareAdapter : public DynExp::HardwareAdapterBase
	{
	public:
		using ParamsType = SmarActHardwareAdapterParams;
		using ConfigType = SmarActHardwareAdapterConfigurator;

		using ChannelType = int8_t;
		using PositionType = int64_t;
		using DirectionType = DynExpInstr::PositionerStage::DirectionType;

		constexpr static auto Name() noexcept { return "SmarAct"; }
		constexpr static auto Category() noexcept { return "Positioners"; }
		static auto Enumerate();

		SmarActHardwareAdapter(const std::thread::id OwnerThreadID, DynExp::ParamsBasePtrType&& Params);
		virtual ~SmarActHardwareAdapter();

		virtual std::string GetName() const override { return Name(); }
		virtual std::string GetCategory() const override { return Category(); }

		bool IsOpened() const noexcept { return SmarActHandleValid; }

		// Position in pm or ndeg
		PositionType GetCurrentPosition(const ChannelType Channel) const;
		PositionType GetTargetPosition(const ChannelType Channel) const;

		// Velocity in pm/s or ndeg/s
		PositionType GetVelocity(const ChannelType Channel) const;

		int32_t GetChannelState(const ChannelType Channel) const;

		void Calibrate(const ChannelType Channel) const;
		void Reference(const ChannelType Channel) const;

		// Velocity in pm/s or ndeg/s
		void SetVelocity(const ChannelType Channel, PositionType Velocity) const;

		// Position in pm or ndeg
		void MoveAbsolute(const ChannelType Channel, PositionType Position) const;
		void MoveRelative(const ChannelType Channel, PositionType Position) const;

		void StopMotion(const ChannelType Channel) const;

	private:
		void Init();

		void ResetImpl(dispatch_tag<HardwareAdapterBase>) override final;
		virtual void ResetImpl(dispatch_tag<SmarActHardwareAdapter>) {}

		void EnsureReadyStateChild() override final;
		bool IsReadyChild() const override final;
		bool IsConnectedChild() const noexcept override final;

		// Not thread-safe, must be called from function calling AcquireLock().
		void CheckError(const SmarActSyms::SA_CTL_Result_t Result, const std::source_location Location = std::source_location::current()) const;

		void OpenUnsafe();
		void CloseUnsafe();

		std::string DeviceDescriptor;
		std::atomic<bool> SmarActHandleValid;
		
		SmarActSyms::SA_CTL_DeviceHandle_t SmarActHandle;
	};
}