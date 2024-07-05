// This file is part of DynExp.

/**
 * @file HardwareAdapterZILabOne.h
 * @brief Implementation of a hardware adapter to control Zurich Instruments MFLI
 * hardware.
*/

#pragma once

#include "stdafx.h"
#include "HardwareAdapter.h"
#include "../MetaInstruments/LockinAmplifier.h"

#undef DEPRECATED
namespace DynExpHardware::ZILabOneHardwareAdapterSyms
{
	#include "../include/ZI/ziAPI.h"
}

namespace DynExpHardware
{
	class ZILabOneHardwareAdapter;

	class ZILabOneException : public Util::Exception
	{
	public:
		ZILabOneException(std::string Description, const int ErrorCode,
			const std::source_location Location = std::source_location::current()) noexcept
			: Exception(std::move(Description), Util::ErrorType::Error, ErrorCode, Location)
		{}
	};

	class ZILabOneHardwareAdapterParams : public DynExp::HardwareAdapterParamsBase
	{
	public:
		ZILabOneHardwareAdapterParams(DynExp::ItemIDType ID, const DynExp::DynExpCore& Core) : HardwareAdapterParamsBase(ID, Core) {}
		virtual ~ZILabOneHardwareAdapterParams() = default;

		virtual const char* GetParamClassTag() const noexcept override { return "ZILabOneHardwareAdapterParams"; }

		Param<TextList> DeviceDescriptor = { *this, {}, "DeviceDescriptor", "Device descriptor",
			"Descriptor of the ZI instrument to connect with" };
		Param<TextList> Interface = { *this, { "1GbE", "USB", "PCIe"}, "Interface", "Interface",
			"Interface to use for establishing a connection to the ZI instrument's data server. This is only used if the connection to the data server is not established automatically." };

	private:
		void ConfigureParamsImpl(dispatch_tag<HardwareAdapterParamsBase>) override final;
		virtual void ConfigureParamsImpl(dispatch_tag<ZILabOneHardwareAdapterParams>) {}
	};

	class ZILabOneHardwareAdapterConfigurator : public DynExp::HardwareAdapterConfiguratorBase
	{
	public:
		using ObjectType = ZILabOneHardwareAdapter;
		using ParamsType = ZILabOneHardwareAdapterParams;

		ZILabOneHardwareAdapterConfigurator() = default;
		virtual ~ZILabOneHardwareAdapterConfigurator() = default;

	private:
		virtual DynExp::ParamsBasePtrType MakeParams(DynExp::ItemIDType ID, const DynExp::DynExpCore& Core) const override { return DynExp::MakeParams<ZILabOneHardwareAdapterConfigurator>(ID, Core); }
	};

	class ZILabOneHardwareAdapter : public DynExp::HardwareAdapterBase
	{
	public:
		using ParamsType = ZILabOneHardwareAdapterParams;
		using ConfigType = ZILabOneHardwareAdapterConfigurator;

		enum SignalInputType { Voltage, DifferentialVoltage, Current };
		
		constexpr static bool DetermineOverload(double PosInputLoad, double NegInputLoad) { return PosInputLoad > 0.95 || NegInputLoad < -0.95; }

		constexpr static auto Name() noexcept { return "ZI LabOne"; }
		constexpr static auto Category() noexcept { return "I/O"; }
		static auto Enumerate();

		ZILabOneHardwareAdapter(const std::thread::id OwnerThreadID, DynExp::ParamsBasePtrType&& Params);
		virtual ~ZILabOneHardwareAdapter();

		virtual std::string GetName() const override { return Name(); }
		virtual std::string GetCategory() const override { return Category(); }

		bool IsOpened() const noexcept { return Opened; }

		void ConfigureInput(SignalInputType SignalInput, uint8_t Demodulator) const;

		// Returns duration per run in seconds
		double StartAcquisition(uint8_t Demodulator, size_t NumSamples, size_t NumRuns = 1, bool AverageRuns = false) const;
		
		void StopAcquisition() const;
		bool HasFinishedAcquisition() const;
		double GetAcquisitionProgress() const;
		void ForceTrigger() const;

		void ClearAcquiredData() const;
		std::vector<DynExpInstr::LockinAmplifierDefs::LockinSample> GetAcquiredData() const;

		double GetInputRange(SignalInputType SignalInput) const;
		void SetInputRange(SignalInputType SignalInput, double InputRange) const;
		void AutoAdjustInputRange(SignalInputType SignalInput) const;
		bool IsInputOverload(SignalInputType SignalInput) const;
		double GetNegInputLoad(SignalInputType SignalInput) const;
		double GetPosInputLoad(SignalInputType SignalInput) const;

		// in rad
		double GetDemodPhase(uint8_t Demodulator) const;
		void SetDemodPhase(uint8_t Demodulator, double Phase) const;
		void AutoAdjustDemodPhase(uint8_t Demodulator) const;

		// in s
		double GetDemodTimeConstant(uint8_t Demodulator) const;
		void SetDemodTimeConstant(uint8_t Demodulator, double TimeConstant) const;

		uint8_t GetDemodFilterOrder(uint8_t Demodulator) const;
		void SetDemodFilterOrder(uint8_t Demodulator, uint8_t FilterOrder) const;

		DynExpInstr::LockinAmplifierDefs::TriggerModeType GetTriggerMode() const;
		void SetTriggerMode(DynExpInstr::LockinAmplifierDefs::TriggerModeType TriggerMode, uint8_t Demodulator = 0, uint8_t TriggerChannel = 1) const;

		DynExpInstr::LockinAmplifierDefs::TriggerEdgeType GetTriggerEdge() const;
		void SetTriggerEdge(DynExpInstr::LockinAmplifierDefs::TriggerEdgeType TriggerEdge) const;

		// in samples/s
		double GetDemodSamplingRate(uint8_t Demodulator) const;
		void SetDemodSamplingRate(uint8_t Demodulator, double SamplingRate) const;

		bool GetEnabled(uint8_t Demodulator) const;
		void SetEnabled(uint8_t Demodulator, bool Enabled) const;

		// in Hz
		double GetOscillatorFrequency(uint8_t Oscillator) const;

	private:
		void Init();

		void ResetImpl(dispatch_tag<HardwareAdapterBase>) override final;
		virtual void ResetImpl(dispatch_tag<ZILabOneHardwareAdapter>) {}

		void EnsureReadyStateChild() override final;
		bool IsReadyChild() const override final;
		bool IsConnectedChild() const noexcept override final;

		// Not thread-safe, must be called from function calling AcquireLock().
		void CheckError(const ZILabOneHardwareAdapterSyms::ZIResult_enum Result,
			const std::source_location Location = std::source_location::current()) const;

		void OpenUnsafe();
		void CloseUnsafe();

		void ConfigureInputUnsafe(SignalInputType SignalInput, uint8_t Demodulator) const;

		void StartAcquisitionUnsafe(uint8_t Demodulator, size_t NumSamples, size_t NumRuns, bool AverageRuns) const;
		void StopAcquisitionUnsafe() const;
		bool HasFinishedAcquisitionUnsafe() const;
		double GetAcquisitionProgressUnsafe() const;
		void ForceTriggerUnsafe() const;

		void ClearAcquiredDataUnsafe() const;
		std::vector<DynExpInstr::LockinAmplifierDefs::LockinSample> GetAcquiredDataUnsafe() const;

		std::string SignalInputTypeToCmdStr(SignalInputType SignalInput) const;
		double GetInputRangeUnsafe(SignalInputType SignalInput) const;
		void SetInputRangeUnsafe(SignalInputType SignalInput, double InputRange) const;
		void AutoAdjustInputRangeUnsafe(SignalInputType SignalInput) const;
		bool IsInputOverloadUnsafe(SignalInputType SignalInput) const;
		double GetNegInputLoadUnsafe(SignalInputType SignalInput) const;
		double GetPosInputLoadUnsafe(SignalInputType SignalInput) const;

		double GetDemodPhaseUnsafe(uint8_t Demodulator) const;
		void SetDemodPhaseUnsafe(uint8_t Demodulator, double Phase) const;
		void AutoAdjustDemodPhaseUnsafe(uint8_t Demodulator) const;

		double GetDemodTimeConstantUnsafe(uint8_t Demodulator) const;
		void SetDemodTimeConstantUnsafe(uint8_t Demodulator, double TimeConstant) const;

		uint8_t GetDemodFilterOrderUnsafe(uint8_t Demodulator) const;
		void SetDemodFilterOrderUnsafe(uint8_t Demodulator, uint8_t FilterOrder) const;

		DynExpInstr::LockinAmplifierDefs::TriggerModeType GetTriggerModeUnsafe() const;
		void SetTriggerModeUnsafe(DynExpInstr::LockinAmplifierDefs::TriggerModeType TriggerMode, uint8_t Demodulator, uint8_t TriggerChannel) const;

		DynExpInstr::LockinAmplifierDefs::TriggerEdgeType GetTriggerEdgeUnsafe() const;
		void SetTriggerEdgeUnsafe(DynExpInstr::LockinAmplifierDefs::TriggerEdgeType TriggerEdge) const;

		double GetDemodSamplingRateUnsafe(uint8_t Demodulator) const;
		void SetDemodSamplingRateUnsafe(uint8_t Demodulator, double SamplingRate) const;

		bool GetEnabledUnsafe(uint8_t Demodulator) const;
		void SetEnabledUnsafe(uint8_t Demodulator, bool Enabled) const;

		double GetOscillatorFrequencyUnsafe(uint8_t Oscillator) const;

		double ReadDoubleUnsafe(const std::string& Path) const;
		long long ReadIntUnsafe(const std::string& Path) const;
		void WriteDoubleUnsafe(const std::string& Path, double Value) const;
		void WriteIntUnsafe(const std::string& Path, long long Value) const;

		ZILabOneHardwareAdapterSyms::ZIConnection ZIConnection;
		ZILabOneHardwareAdapterSyms::ZIModuleHandle DAQModuleHandle;

		std::atomic<bool> Opened;
		std::string DeviceDescriptor;
		std::string Interface;
		int Clockbase;
	};
}