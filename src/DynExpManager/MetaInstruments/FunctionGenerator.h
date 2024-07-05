// This file is part of DynExp.

/**
 * @file FunctionGenerator.h
 * @brief Implementation of a function generator meta instrument to generate
 * waveforms.
*/

#pragma once

#include "stdafx.h"
#include "DataStreamInstrument.h"

namespace DynExpInstr
{
	class FunctionGenerator;

	/**
	 * @brief Definitions to describe waveforms.
	*/
	namespace FunctionGeneratorDefs
	{
		/**
		 * @brief Type to determine the waveform type
		*/
		enum WaveformTypes {
			None,	//!< Unspecified/arbitrary waveform
			Sine,	//!< Sine waveform
			Rect,	//!< Rectangular waveform
			Ramp,	//!< Ramp waveform
			Pulse	//!< Manually defined pulses
		};

		/**
		 * @brief Type describing a generic periodic function
		*/
		struct FunctionDescType
		{
			constexpr FunctionDescType() noexcept = default;

			/**
			 * @brief Constructs a @p FunctionDescType instance.
			 * @param FrequencyInHz @copybrief #FrequencyInHz
			 * @param Amplitude @copybrief #Amplitude
			 * @param Offset @copybrief #Offset
			*/
			constexpr FunctionDescType(double FrequencyInHz, double Amplitude, double Offset) noexcept
				: FrequencyInHz(FrequencyInHz), Amplitude(Amplitude), Offset(Offset) {}

			double FrequencyInHz{};		//!< Frequency of the function in Hz
			double Amplitude{};			//!< Amplitude of the function
			double Offset{};			//!< Offset of the function to be added to each sample
		};

		/**
		 * @brief Type describing a sine function
		*/
		struct SineFunctionDescType : FunctionDescType
		{
			constexpr SineFunctionDescType() noexcept = default;

			/**
			 * @brief Constructs a @p SineFunctionDescType instance.
			 * @param FrequencyInHz @copybrief FunctionDescType::FrequencyInHz
			 * @param Amplitude @copybrief FunctionDescType::Amplitude
			 * @param Offset @copybrief FunctionDescType::Offset
			 * @param PhaseInRad @copybrief #PhaseInRad
			*/
			constexpr SineFunctionDescType(double FrequencyInHz, double Amplitude, double Offset, double PhaseInRad) noexcept
				: FunctionDescType(FrequencyInHz, Amplitude, Offset), PhaseInRad(PhaseInRad) {}

			double PhaseInRad{};		//!< Phase of the function in radians
		};

		/**
		 * @brief Type describing a rectangular function
		*/
		struct RectFunctionDescType : FunctionDescType
		{
			constexpr RectFunctionDescType() noexcept = default;

			/**
			 * @brief Constructs a @p RectFunctionDescType instance.
			 * @param FrequencyInHz @copybrief FunctionDescType::FrequencyInHz
			 * @param Amplitude @copybrief FunctionDescType::Amplitude
			 * @param Offset @copybrief FunctionDescType::Offset
			 * @param PhaseInRad @copybrief #PhaseInRad
			 * @param DutyCycle @copybrief #DutyCycle
			*/
			constexpr RectFunctionDescType(double FrequencyInHz, double Amplitude, double Offset, double PhaseInRad, double DutyCycle) noexcept
				: FunctionDescType(FrequencyInHz, Amplitude, Offset), PhaseInRad(PhaseInRad), DutyCycle(DutyCycle) {}

			double PhaseInRad{};		//!< Phase of the function in radians
			double DutyCycle{};			//!< Duty cycle of the function (in between 0 and 1)
		};

		/**
		 * @brief Type describing a ramp function
		*/
		struct RampFunctionDescType : FunctionDescType
		{
			constexpr RampFunctionDescType() noexcept = default;

			/**
			 * @brief Constructs a @p RampFunctionDescType instance.
			 * @param FrequencyInHz @copybrief FunctionDescType::FrequencyInHz
			 * @param Amplitude @copybrief FunctionDescType::Amplitude
			 * @param Offset @copybrief FunctionDescType::Offset
			 * @param PhaseInRad @copybrief #PhaseInRad
			 * @param RiseFallRatio @copybrief #RiseFallRatio
			*/
			constexpr RampFunctionDescType(double FrequencyInHz, double Amplitude, double Offset, double PhaseInRad, double RiseFallRatio) noexcept
				: FunctionDescType(FrequencyInHz, Amplitude, Offset), PhaseInRad(PhaseInRad), RiseFallRatio(RiseFallRatio) {}

			double PhaseInRad{};		//!< Phase of the function in radians
			double RiseFallRatio{};		//!< Ratio between the ramp's falling and rising edge lengths (in between 0 and 1)
		};

		/**
		 * @brief Type describing function consisting of a series of pulses
		*/
		struct PulsesDescType
		{
			/**
			 * @brief Type containing pulses as tuples (time [s], value).
			 * The time refers to the beginning of a segment where the
			 * resulting function stays at the respective value.
			 * The map always stays sorted by time in ascending order.
			*/
			using PulsesType = std::map<double, double>;

			PulsesDescType() = default;

			/**
			 * @brief Copy constructs a @p PulsesDescType instance.
			 * @param Other @p PulsesDescType instance to copy
			*/
			PulsesDescType(const PulsesDescType& Other) : Pulses(Other.Pulses), Offset(Other.Offset) {}

			/**
			 * @brief Move constructs a @p PulsesDescType instance.
			 * @p Reset() is called on @p Other after the operation.
			 * @param Other @p PulsesDescType instance to move from.
			*/
			PulsesDescType(PulsesDescType&& Other);

			/**
			 * @brief Constructs a @p PulsesDescType instance copying
			 * @p Pulses and setting #Offset to 0.
			 * @param Pulses Pulse sequence to copy
			*/
			PulsesDescType(const PulsesType& Pulses) : Pulses(Pulses), Offset(0.0) {}

			/**
			 * @brief Constructs a @p PulsesDescType instance moving
			 * from @p Pulses and setting #Offset to 0. @p Pulses is
			 * empty after the operation.
			 * @param Pulses Pulse sequence to move from.
			*/
			PulsesDescType(PulsesType&& Pulses);

			/**
			 * @brief Constructs a @p PulsesDescType instance filling
			 * it with pulse segments specified by their start times
			 * and amplitudes given by two separate lists.
			 * @param PulseStarts List of the start times of the pulses
			 * @param PulseAmplitudes List with the amplitudes (values)
			 * of the pulses
			 * @param Offset @copybrief #Offset
			 * @throws Util::InvalidArgException is thrown if @p PulseStarts
			 * and @p PulseAmplitudes do not have the same length.
			*/
			PulsesDescType(const std::vector<double>& PulseStarts, const std::vector<double>& PulseAmplitudes, double Offset = 0.0);

			/**
			 * @brief Move assignment operator.
			 * @p Reset() is called on @p Other after the operation.
			 * @param Other @p PulsesDescType instance to move from
			 * @return Returns a reference to the @p PulsesDescType
			 * instance which has been moved to.
			*/
			PulsesDescType& operator=(PulsesDescType&& Other);

			/**
			 * @brief Removes all pulse segments from #Pulses.
			*/
			void Reset() { Pulses.clear(); }

			/**
			 * @brief Series of segments forming the pulse sequence.
			 * The resulting function stays constant during a segment.
			 * Refer to @p PulsesType.
			*/
			PulsesType Pulses;

			double Offset{};			//!< Offset of the function to be added to each sample
		};

		/**
		 * @brief Type describing modulation parameters for a waveform
		*/
		struct ModulationDescType
		{
			/**
			 * @brief Type to determine the modulation type
			*/
			enum class ModulationType {
				Disabled,	//!< No modulation
				Amplitude,	//!< Modulation affecting the waveform's amplitude
				Frequency,	//!< Modulation affecting the waveform's frequency
				Phase		//!< Modulation affecting the waveform's phase
			};

			/**
			 * @brief Type to determine the shape of the modulation
			*/
			enum class ModulationShapeType {
				Sine,		//!< Sinusoidally modulate the affected quantity
				Pulse		//!< Pulse (binary) modulation switching the affected quantity between two values
			};

			ModulationDescType() noexcept = default;

			/**
			 * @brief Constructs a @p ModulationDescType instance.
			 * @param Type @copybrief #Type
			 * @param Depth @copybrief #Depth
			 * @param FrequencyInHz @copybrief #FrequencyInHz
			 * @param Shape @copybrief #Shape
			 * @param PulseDutyCycle @copybrief #PulseDutyCycle
			*/
			ModulationDescType(ModulationType Type, double Depth, double FrequencyInHz,
				ModulationShapeType Shape = ModulationShapeType::Sine, double PulseDutyCycle = .5) noexcept
				: Type(Type), Depth(Depth), FrequencyInHz(FrequencyInHz),
				Shape(Shape), PulseDutyCycle(PulseDutyCycle) {}

			ModulationType Type = ModulationType::Disabled;			//!< Type of the modulation
			double Depth{};											//!< Depth (amplitude) of the modulation
			double FrequencyInHz{};									//!< Frequency of the modulation in Hz
			ModulationShapeType Shape = ModulationShapeType::Sine;	//!< Shape of the modulation
			double PulseDutyCycle{};								//!< Duty cycle (in between 0 and 1) of the pulses in case of binary modulation (#Shape is ModulationShapeType::Pulse)
		};

		/**
		 * @brief Type describing sweep parameters for a waveform
		*/
		struct SweepDescType
		{
			/**
			 * @brief Type to determine the sweep type
			*/
			enum class SweepType {
				Disabled,	//!< Sweep functionality disabled
				Amplitude,	//!< Sweep the waveform's amplitude
				Frequency	//!< Sweep the waveform's frequency
			};

			SweepDescType() noexcept = default;

			/**
			 * @brief Constructs a @p SweepDescType instance.
			 * @param Type @copybrief #Type
			 * @param Min @copybrief #Min
			 * @param Max @copybrief #Max
			 * @param ValueDiffPerSample @copybrief #ValueDiffPerSample
			 * @param TimeDiffPerSample_ms @copybrief #TimeDiffPerSample_ms
			 * @param Retrace @copybrief #Retrace
			*/
			SweepDescType(SweepType Type, double Min, double Max, double ValueDiffPerSample, double TimeDiffPerSample_ms, bool Retrace) noexcept
				: Type(Type), Min(Min), Max(Max), ValueDiffPerSample(ValueDiffPerSample), TimeDiffPerSample_ms(TimeDiffPerSample_ms), Retrace(Retrace) {}

			SweepType Type = SweepType::Disabled;		//!< Type of the sweep
			double Min{};								//!< Minimal value where the sweep starts
			double Max{};								//!< Maximal value where the sweep ends
			double ValueDiffPerSample{};				//!< Value difference in between subsequent samples of the sweep
			double TimeDiffPerSample_ms{};				//!< Time per sweep sample in milliseconds
			bool Retrace{};								//!< If true, retraces the sweep going back to its start value after its end has been reached.
		};

		/**
		 * @brief Type describing trigger parameters determining when the waveform is generated
		*/
		struct TriggerDescType
		{
			/**
			 * @brief Type to determine the trigger mode.
			 * Not a strongly-typed enum to allow using the enumeration in a
			 * DynExp::ParamsBase::Param in class @p FunctionGeneratorParams.
			*/
			enum TriggerModeType {
				Continuous,		//!< Run continuously disabling the trigger
				ExternSingle,	//!< Run once after an external trigger signal has been detected
				ExternStep,		//!< Advance by a single (sweep) sample after an external trigger signal has been detected
				Manual			//!< Only trigger by a user command
			};

			/**
			 * @brief Type to determine at which edge of a trigger signal to trigger.
			 * Not a strongly-typed enum to allow using the enumeration in a
			 * DynExp::ParamsBase::Param in class @p FunctionGeneratorParams.
			*/
			enum TriggerEdgeType {
				Rise,			//!< Trigger on rising edge
				Fall			//!< Trigger on falling edge
			};

			TriggerDescType() noexcept = default;

			/**
			 * @brief Constructs a @p TriggerDescType instance.
			 * @param TriggerMode @copybrief #TriggerMode
			 * @param TriggerEdge @copybrief #TriggerEdge
			*/
			TriggerDescType(TriggerModeType TriggerMode, TriggerEdgeType TriggerEdge) noexcept
				: TriggerMode(TriggerMode), TriggerEdge(TriggerEdge) {}

			TriggerModeType TriggerMode = TriggerModeType::Continuous;	//!< Trigger mode
			TriggerEdgeType TriggerEdge = TriggerEdgeType::Rise;		//!< Edge to trigger on
		};

		/**
		 * @brief Determines minimal values assignable to a description of a generic periodic function.
		 * @return Returns a @p FunctionDescType instance with minimal possible values assigned to each field.
		*/
		constexpr FunctionDescType GetDefaultMinFunctionDesc() { return { 0.0, std::numeric_limits<double>::lowest(), std::numeric_limits<double>::lowest() }; }

		/**
		 * @brief Determines maximal values assignable to a description of a generic periodic function.
		 * @return Returns a @p FunctionDescType instance with maximal possible values assigned to each field.
		*/
		constexpr FunctionDescType GetDefaultMaxFunctionDesc() { return { std::numeric_limits<double>::max(), std::numeric_limits<double>::max(), std::numeric_limits<double>::max() }; }

		/**
		 * @brief Determines default values to assign to a description of a generic periodic function.
		 * @return Returns a @p FunctionDescType instance with default values assigned to each field.
		*/
		constexpr FunctionDescType GetDefaultDefaultFunctionDesc() { return { 1, 1, 0 }; }

		/**
		 * @brief Calculates the period from the given function description.
		 * @param FunctionDesc Description of a generic periodic function
		 * @return Returns the period in seconds.
		*/
		inline constexpr Util::seconds PeriodFromFunctionDesc(const FunctionDescType& FunctionDesc);

		/**
		 * @brief Calculates the value of a rectangular function depending on the current @p Phase.
		 * @param DutyCycle Duty cycle of the function (in between 0 and 1)
		 * @param Phase Phase the function depends on.
		 * @return Returns
		 * @code
		 * Heaviside(-(Phase mod 2*pi) + 2*pi * DutyCycle)
		 * @endcode
		*/
		inline double RectFunc(double DutyCycle, double Phase);

		/**
		 * @brief Calculates the value of an inverse rectangular function depending on the current @p Phase.
		 * @param DutyCycle Duty cycle of the function (in between 0 and 1)
		 * @param Phase Phase the function depends on.
		 * @return Returns
		 * @code
		 * Heaviside((Phase mod 2*pi) - 2*pi * DutyCycle)
		 * @endcode
		*/
		inline double InvRectFunc(double DutyCycle, double Phase);

		/**
		 * @brief Calculates the value of a ramp function depending on the current @p Phase.
		 * @param RiseFallRatio Ratio between the ramp's falling and rising edge lengths (in between 0 and 1)
		 * @param Phase Phase the function depends on.
		 * @return Returns
		 * @code
		 * RectFunc(Phase, RiseFallRatio) * (Phase mod(2*pi)) / (pi * RiseFallRatio)
		 * + InvRectFunc(Phase, RiseFallRatio) * ((-Phase) mod(2*pi)) / (pi * (1 - RiseFallRatio))
		 * - 1
		 * @endcode
		*/
		inline double RampFunc(double RiseFallRatio, double Phase);
	}

	/**
	 * @brief Tasks for @p FunctionGenerator
	*/
	namespace FunctionGeneratorTasks
	{
		/**
		 * @copydoc DynExp::InitTaskBase
		*/
		class InitTask : public DataStreamInstrumentTasks::InitTask
		{
			void InitFuncImpl(dispatch_tag<DataStreamInstrumentTasks::InitTask>, DynExp::InstrumentInstance& Instance) override final;
			
			/**
			 * @copydoc InitFuncImpl(dispatch_tag<DataStreamInstrumentTasks::InitTask>, DynExp::InstrumentInstance&)
			*/
			virtual void InitFuncImpl(dispatch_tag<InitTask>, DynExp::InstrumentInstance& Instance) {}
		};

		/**
		 * @copydoc DynExp::ExitTaskBase
		*/
		class ExitTask : public DataStreamInstrumentTasks::ExitTask
		{
			void ExitFuncImpl(dispatch_tag<DataStreamInstrumentTasks::ExitTask>, DynExp::InstrumentInstance& Instance) override final { ExitFuncImpl(dispatch_tag<ExitTask>(), Instance); }
			
			/**
			 * @copydoc ExitFuncImpl(dispatch_tag<DataStreamInstrumentTasks::ExitTask>, DynExp::InstrumentInstance&)
			*/
			virtual void ExitFuncImpl(dispatch_tag<ExitTask>, DynExp::InstrumentInstance& Instance) {}
		};

		/**
		 * @copydoc DynExp::UpdateTaskBase
		*/
		class UpdateTask : public DataStreamInstrumentTasks::UpdateTask
		{
			void UpdateFuncImpl(dispatch_tag<DataStreamInstrumentTasks::UpdateTask>, DynExp::InstrumentInstance& Instance) override final;
			
			/**
			 * @copydoc UpdateFuncImpl(dispatch_tag<DataStreamInstrumentTasks::UpdateTask>, DynExp::InstrumentInstance&)
			*/
			virtual void UpdateFuncImpl(dispatch_tag<UpdateTask>, DynExp::InstrumentInstance& Instance) {}
		};

		/**
		 * @brief Task to write a sine function to the related data stream instrument.
		*/
		class SetSineFunctionTask : public DynExp::TaskBase
		{
		public:
			/**
			 * @brief Constructs a @p SetSineFunctionTask instance.
			 * @param FunctionDesc @copybrief FunctionGeneratorTasks::SetSineFunctionTask::FunctionDesc
			 * @param Autostart @copybrief FunctionGeneratorTasks::SetSineFunctionTask::Autostart
			 * @param CallbackFunc @copybrief DynExp::TaskBase::CallbackFunc
			*/
			SetSineFunctionTask(const FunctionGeneratorDefs::SineFunctionDescType& FunctionDesc, bool Autostart, CallbackType CallbackFunc) noexcept
				: TaskBase(CallbackFunc), FunctionDesc(FunctionDesc), Autostart(Autostart) {}

		private:
			virtual DynExp::TaskResultType RunChild(DynExp::InstrumentInstance& Instance) override;

			FunctionGeneratorDefs::SineFunctionDescType FunctionDesc;		//!< Description of the sine function

			/**
			 * @brief Determines whether to directly make the underlying hardware device
			 * start generating the function (true) or whether to await a trigger signal (false).
			*/
			bool Autostart;
		};

		/**
		 * @brief Task to write a rectangular function to the related data stream instrument.
		*/
		class SetRectFunctionTask : public DynExp::TaskBase
		{
		public:
			/**
			 * @brief Constructs a @p SetRectFunctionTask instance.
			 * @param FunctionDesc @copybrief FunctionGeneratorTasks::SetRectFunctionTask::FunctionDesc
			 * @param Autostart @copybrief FunctionGeneratorTasks::SetRectFunctionTask::Autostart
			 * @param CallbackFunc @copybrief DynExp::TaskBase::CallbackFunc
			*/
			SetRectFunctionTask(const FunctionGeneratorDefs::RectFunctionDescType& FunctionDesc, bool Autostart, CallbackType CallbackFunc) noexcept
				: TaskBase(CallbackFunc), FunctionDesc(FunctionDesc), Autostart(Autostart) {}

		private:
			virtual DynExp::TaskResultType RunChild(DynExp::InstrumentInstance& Instance) override;

			FunctionGeneratorDefs::RectFunctionDescType FunctionDesc;		//!< Description of the rectangular function
			bool Autostart;													//!< @copydoc FunctionGeneratorTasks::SetSineFunctionTask::Autostart
		};

		/**
		 * @brief Task to write a ramp function to the related data stream instrument.
		*/
		class SetRampFunctionTask : public DynExp::TaskBase
		{
		public:
			/**
			 * @brief Constructs a @p SetRampFunctionTask instance.
			 * @param FunctionDesc @copybrief FunctionGeneratorTasks::SetRampFunctionTask::FunctionDesc
			 * @param Autostart @copybrief FunctionGeneratorTasks::SetRampFunctionTask::Autostart
			 * @param CallbackFunc @copybrief DynExp::TaskBase::CallbackFunc
			*/
			SetRampFunctionTask(const FunctionGeneratorDefs::RampFunctionDescType& FunctionDesc, bool Autostart, CallbackType CallbackFunc) noexcept
				: TaskBase(CallbackFunc), FunctionDesc(FunctionDesc), Autostart(Autostart) {}

		private:
			virtual DynExp::TaskResultType RunChild(DynExp::InstrumentInstance& Instance) override;

			FunctionGeneratorDefs::RampFunctionDescType FunctionDesc;		//!< Description of the ramp function
			bool Autostart;													//!< @copydoc FunctionGeneratorTasks::SetSineFunctionTask::Autostart
		};

		/**
		 * @brief Task to write a function consisting of pulse segments to the related data stream instrument.
		*/
		class SetPulseFunctionTask : public DynExp::TaskBase
		{
		public:
			/**
			 * @brief Constructs a @p SetPulseFunctionTask instance.
			 * @param FunctionDesc @copybrief FunctionGeneratorTasks::SetPulseFunctionTask::FunctionDesc
			 * @param Autostart @copybrief FunctionGeneratorTasks::SetPulseFunctionTask::Autostart
			 * @param CallbackFunc @copybrief DynExp::TaskBase::CallbackFunc
			*/
			SetPulseFunctionTask(const FunctionGeneratorDefs::PulsesDescType& FunctionDesc, bool Autostart, CallbackType CallbackFunc) noexcept
				: TaskBase(CallbackFunc), FunctionDesc(FunctionDesc), Autostart(Autostart) {}

		private:
			virtual DynExp::TaskResultType RunChild(DynExp::InstrumentInstance& Instance) override;

			FunctionGeneratorDefs::PulsesDescType FunctionDesc;				//!< Description of the pulse function
			bool Autostart;													//!< @copydoc FunctionGeneratorTasks::SetSineFunctionTask::Autostart
		};

		/**
		 * @brief Task to write an arbitrary function defined by a list of samples to the related data stream instrument.
		*/
		class SetArbitraryFunctionTask : public DynExp::TaskBase
		{
		public:
			/**
			 * @brief Constructs a @p SetArbitraryFunctionTask instance.
			 * @param Samples @copybrief FunctionGeneratorTasks::SetArbitraryFunctionTask::Samples
			 * @param Autostart @copybrief FunctionGeneratorTasks::SetArbitraryFunctionTask::Autostart
			 * @param CallbackFunc @copybrief DynExp::TaskBase::CallbackFunc
			*/
			SetArbitraryFunctionTask(DataStreamBase::BasicSampleListType&& Samples, bool Autostart, CallbackType CallbackFunc) noexcept
				: TaskBase(CallbackFunc), Samples(std::move(Samples)), Autostart(Autostart) {}

		private:
			virtual DynExp::TaskResultType RunChild(DynExp::InstrumentInstance& Instance) override;

			DataStreamBase::BasicSampleListType Samples;					//!< Samples the function consists of
			bool Autostart;													//!< @copydoc FunctionGeneratorTasks::SetSineFunctionTask::Autostart
		};

		/**
		 * @brief Task to initialize the trigger of the related function generator instrument.
		 * The task only updates FunctionGeneratorData::CurrentTriggerMode and
		 * FunctionGeneratorData::CurrentTriggerEdge. To configure the actual hardware,
		 * define another task for doing so and let FunctionGenerator::SetTriggerChild()
		 * enqueue it.
		*/
		class SetTriggerTask : public DynExp::TaskBase
		{
		public:
			/**
			 * @brief Constructs a @p SetTriggerTask instance.
			 * @param TriggerDesc @copybrief FunctionGeneratorTasks::SetTriggerTask::TriggerDesc
			*/
			SetTriggerTask(const FunctionGeneratorDefs::TriggerDescType& TriggerDesc) noexcept
				: TaskBase(nullptr), TriggerDesc(TriggerDesc) {}

		private:
			virtual DynExp::TaskResultType RunChild(DynExp::InstrumentInstance& Instance) override;

			FunctionGeneratorDefs::TriggerDescType TriggerDesc;				//!< Trigger settings to apply
		};
	}

	/**
	 * @brief Data class for @p FunctionGenerator
	*/
	class FunctionGeneratorData : public DataStreamInstrumentData
	{
		friend class FunctionGeneratorTasks::UpdateTask;
		friend class FunctionGeneratorTasks::SetSineFunctionTask;
		friend class FunctionGeneratorTasks::SetRectFunctionTask;
		friend class FunctionGeneratorTasks::SetRampFunctionTask;
		friend class FunctionGeneratorTasks::SetPulseFunctionTask;
		friend class FunctionGeneratorTasks::SetArbitraryFunctionTask;
		friend class FunctionGeneratorTasks::SetTriggerTask;

	public:
		/**
		 * @copydoc DataStreamInstrumentData::DataStreamInstrumentData
		*/
		FunctionGeneratorData(DataStreamBasePtrType&& SampleStream) : DataStreamInstrumentData(std::move(SampleStream)) {}

		virtual ~FunctionGeneratorData() = default;

		auto GetCurrentWaveformType() const noexcept { return CurrentWaveformType; }	//!< Returns #CurrentWaveformType.
		auto GetCurrentFrequencyInHz() const noexcept { return CurrentFrequencyInHz; }	//!< Returns #CurrentFrequencyInHz.
		auto GetCurrentAmplitude() const noexcept { return CurrentAmplitude; }			//!< Returns #CurrentAmplitude.
		auto GetCurrentOffset() const noexcept { return CurrentOffset; }				//!< Returns #CurrentOffset.
		auto GetCurrentPhaseInRad() const noexcept { return CurrentPhaseInRad; }		//!< Returns #CurrentPhaseInRad.
		auto GetCurrentDutyCycle() const noexcept { return CurrentDutyCycle; }			//!< Returns #CurrentDutyCycle.
		auto GetCurrentTriggerMode() const noexcept { return CurrentTriggerMode; }		//!< Returns #CurrentTriggerMode.
		auto GetCurrentTriggerEdge() const noexcept { return CurrentTriggerEdge; }		//!< Returns #CurrentTriggerEdge.
		auto GetShouldAutostart() const noexcept { return ShouldAutostart; }			//!< Returns #ShouldAutostart.
		auto GetCurrentPulses() const noexcept { return CurrentPulses; }				//!< Returns #CurrentPulses.

	private:
		void ResetImpl(dispatch_tag<DataStreamInstrumentData>) override final;
		
		/**
		 * @copydoc ResetImpl(dispatch_tag<DataStreamInstrumentData>)
		*/
		virtual void ResetImpl(dispatch_tag<FunctionGeneratorData>) {};

		/**
		 * @brief Indicates whether new data has been written to the instrument's data
		 * stream which needs now to be transferred to the underlying hardware in an
		 * FunctionGeneratorTasks::UpdateTask calling DataStreamInstrument::WriteData().
		*/
		bool DataHasBeenUpdated{ false };

		FunctionGeneratorDefs::WaveformTypes CurrentWaveformType{};					//!< Current waveform type
		double CurrentFrequencyInHz{};												//!< Current waveform's frequency in Hz
		double CurrentAmplitude{};													//!< Current waveform's amplitude
		double CurrentOffset{};														//!< Current waveform's offset
		double CurrentPhaseInRad{};													//!< Current waveform's phase in radians
		double CurrentDutyCycle{};													//!< Current waveform's duty cycle or rise/fall ratio
		FunctionGeneratorDefs::TriggerDescType::TriggerModeType CurrentTriggerMode;	//!< Current trigger mode
		FunctionGeneratorDefs::TriggerDescType::TriggerEdgeType CurrentTriggerEdge;	//!< Current edge type to trigger on
		bool ShouldAutostart{};														//!< @copydoc FunctionGeneratorTasks::SetSineFunctionTask::Autostart

		FunctionGeneratorDefs::PulsesDescType CurrentPulses;						//!< Current waveform's pulse segments
	};

	/**
	 * @brief Parameter class for @p FunctionGenerator
	*/
	class FunctionGeneratorParams : public DataStreamInstrumentParams
	{
	public:
		/**
		 * @brief Constructs the parameters for a @p FunctionGenerator instance.
		 * @copydetails DynExp::ParamsBase::ParamsBase
		*/
		FunctionGeneratorParams(DynExp::ItemIDType ID, const DynExp::DynExpCore& Core) : DataStreamInstrumentParams(ID, Core) {}

		virtual ~FunctionGeneratorParams() = 0;

		virtual const char* GetParamClassTag() const noexcept override { return "FunctionGeneratorParams"; }

		/**
		 * @brief Parameter to store the waveform type
		*/
		Param<FunctionGeneratorDefs::WaveformTypes> WaveformType = { *this, "WaveformTypes", FunctionGeneratorDefs::WaveformTypes::None };

		/**
		 * @brief Parameter to store waveform's frequency in Hz. Refer to FunctionGeneratorDefs::FunctionDescType::FrequencyInHz.
		*/
		Param<double> FrequencyInHz = { *this, "FrequencyInHz", FunctionGeneratorDefs::GetDefaultDefaultFunctionDesc().FrequencyInHz,
			FunctionGeneratorDefs::GetDefaultMinFunctionDesc().FrequencyInHz, FunctionGeneratorDefs::GetDefaultMaxFunctionDesc().FrequencyInHz };

		/**
		 * @brief Parameter to store waveform's amplitude. Refer to FunctionGeneratorDefs::FunctionDescType::Amplitude.
		*/
		Param<double> Amplitude = { *this, "Amplitude", FunctionGeneratorDefs::GetDefaultDefaultFunctionDesc().Amplitude,
			FunctionGeneratorDefs::GetDefaultMinFunctionDesc().Amplitude, FunctionGeneratorDefs::GetDefaultMaxFunctionDesc().Amplitude };

		/**
		 * @brief Parameter to store waveform's offset to be added to each sample. Refer to FunctionGeneratorDefs::FunctionDescType::Offset.
		*/
		Param<double> Offset = { *this, "Offset", FunctionGeneratorDefs::GetDefaultDefaultFunctionDesc().Offset,
			FunctionGeneratorDefs::GetDefaultMinFunctionDesc().Offset, FunctionGeneratorDefs::GetDefaultMaxFunctionDesc().Offset };

		/**
		 * @brief Parameter to store waveform's phase in radians. Refer to FunctionGeneratorDefs::SineFunctionDescType::PhaseInRad,
		 * FunctionGeneratorDefs::RectFunctionDescType::PhaseInRad, and FunctionGeneratorDefs::RampFunctionDescType::PhaseInRad.
		*/
		Param<double> PhaseInRad = { *this, "PhaseInRad", 0, 0, 2.0 * std::numbers::pi };

		/**
		 * @brief Parameter to store waveform's duty cycle or rise/fall ratio. Refer to
		 * FunctionGeneratorDefs::RectFunctionDescType::DutyCycle and FunctionGeneratorDefs::RampFunctionDescType::RiseFallRatio.
		*/
		Param<double> DutyCycle = { *this, "DutyCycle", .5, 0, 1 };

		/**
		 * @brief Parameter to store the start times of the waveform's pulse segments.
		 * Refer to FunctionGeneratorDefs::PulsesDescType::PulsesDescType(const std::vector<double>&, const std::vector<double>&, double).
		*/
		ListParam<double> PulseStarts = { *this, "PulseStarts", {}, 0};

		/**
		 * @brief Parameter to store the amplitudes of the waveform's pulse segments.
		 * Refer to FunctionGeneratorDefs::PulsesDescType::PulsesDescType(const std::vector<double>&, const std::vector<double>&, double).
		*/
		ListParam<double> PulseAmplitudes = { *this, "PulseAmplitudes", {} };

		/**
		 * @brief Parameter to store the trigger mode
		*/
		Param<FunctionGeneratorDefs::TriggerDescType::TriggerModeType> TriggerMode = { *this, "TriggerMode", FunctionGeneratorDefs::TriggerDescType::TriggerModeType::Continuous };

		/**
		 * @brief Parameter to store the edge type to trigger on
		*/
		Param<FunctionGeneratorDefs::TriggerDescType::TriggerEdgeType> TriggerEdge = { *this, "TriggerEdge", FunctionGeneratorDefs::TriggerDescType::TriggerEdgeType::Rise };

		/**
		 * @brief Determines whether to directly make the underlying hardware device
		 * start generating the waveform after the instrument's initialization (true)
		 * or whether to await a trigger/start signal (false).
		*/
		Param<bool> Autostart = { *this, "Autostart", false };

	private:
		void ConfigureParamsImpl(dispatch_tag<DataStreamInstrumentParams>) override final { ConfigureParamsImpl(dispatch_tag<FunctionGeneratorParams>()); }
		virtual void ConfigureParamsImpl(dispatch_tag<FunctionGeneratorParams>) {}	//!< @copydoc ConfigureParamsImpl(dispatch_tag<DataStreamInstrumentParams>)
	};

	/**
	 * @brief Configurator class for @p FunctionGenerator
	*/
	class FunctionGeneratorConfigurator : public DataStreamInstrumentConfigurator
	{
	public:
		using ObjectType = FunctionGenerator;
		using ParamsType = FunctionGeneratorParams;

		FunctionGeneratorConfigurator() = default;
		virtual ~FunctionGeneratorConfigurator() = 0;
	};

	/**
	 * @brief Function generator meta instrument based on the data stream meta
	 * instrument to generate waveforms by either fillig data streams with
	 * waveform samples or by controlling a physical function generator device.
	*/
	class FunctionGenerator : public DataStreamInstrument
	{
	public:
		using ParamsType = FunctionGeneratorParams;								//!< @copydoc DynExp::Object::ParamsType
		using ConfigType = FunctionGeneratorConfigurator;						//!< @copydoc DynExp::Object::ConfigType
		using InstrumentDataType = FunctionGeneratorData;						//!< @copydoc DynExp::InstrumentBase::InstrumentDataType

		/** @name gRPC aliases
		 * Redefined to use this instrument with DynExpInstr::gRPCInstrument.
		*/
		///@{
		using InitTaskType = FunctionGeneratorTasks::InitTask;					//!< @copydoc DynExp::InitTaskBase
		using ExitTaskType = FunctionGeneratorTasks::ExitTask;					//!< @copydoc DynExp::ExitTaskBase
		using UpdateTaskType = FunctionGeneratorTasks::UpdateTask;				//!< @copydoc DynExp::UpdateTaskBase
		///@}

		/**
		 * @brief Type enumerating waveform types the function generator
		 * might be able to produce (in hardware).
		*/
		enum class WaveformCapsType {
			UserDefined,		//!< Arbitrary waveform
			Sine,				//!< Sine waveform
			Rect,				//!< Rectangular waveform
			Ramp,				//!< Ramp waveform
			Pulse,				//!< Manually defined pulses
			NUM_ELEMENTS		//!< Must be last. Refer to Util::FeatureTester.
		};

		/**
		 * @brief Type enumerating quantities the function generator
		 * might be able to modulate or sweep (in hardware).
		*/
		enum class QuantityCapsType {
			Amplitude,			//!< Waveform's amplitude
			Frequency,			//!< Waveform's frequency
			Phase,				//!< Waveform's phase
			NUM_ELEMENTS		//!< Must be last. Refer to Util::FeatureTester.
		};

		/**
		 * @brief Type enumerating trigger features the function generator
		 * might be capable of (in hardware).
		*/
		enum class TriggerCapsType {
			CanConfigure,		//!< Trigger settings can be adjusted by the software.
			CanForce,			//!< Triggering can be forced (executed by the software).
			NUM_ELEMENTS		//!< Must be last. Refer to Util::FeatureTester.
		};

		constexpr static auto Name() noexcept { return "Function Generator"; }	//!< @copydoc DynExp::SerialCommunicationHardwareAdapter::Name
		constexpr static auto Category() noexcept { return "I/O"; }				//!< @copydoc DynExp::InstrumentBase::Category

		/**
		 * @copydoc DynExp::InstrumentBase::InstrumentBase
		*/
		FunctionGenerator(const std::thread::id OwnerThreadID, DynExp::ParamsBasePtrType&& Params)
			: DataStreamInstrument(OwnerThreadID, std::move(Params)) {}

		virtual ~FunctionGenerator() = 0;

		virtual std::string GetName() const override { return Name(); }
		virtual std::string GetCategory() const override { return Category(); }

		FunctionGeneratorDefs::FunctionDescType GetMinCaps() const { return GetMinCapsChild(); }				//!< Returns the minimal values assignable to a description of a generic periodic function.
		FunctionGeneratorDefs::FunctionDescType GetMaxCaps() const { return GetMaxCapsChild(); }				//!< Returns the maximal values assignable to a description of a generic periodic function.
		FunctionGeneratorDefs::FunctionDescType GetParamDefaults() const { return GetParamDefaultsChild(); }	//!< Returns the default values to assign to a description of a generic periodic function.
		Util::FeatureTester<WaveformCapsType> GetWaveformCaps() const { return GetWaveformCapsChild(); }		//!< Returns the waveform types the function generator is able to produce.
		Util::FeatureTester<QuantityCapsType> GetModulationCaps() const { return GetModulationCapsChild(); }	//!< Returns the modulation types the function generator is capable of.
		Util::FeatureTester<QuantityCapsType> GetSweepCaps() const { return GetSweepCapsChild(); }				//!< Returns the sweep types the function generator is capable of.
		Util::FeatureTester<TriggerCapsType> GetTriggerCaps() const { return GetTriggerCapsChild(); }			//!< Returns the trigger features the function generator is capable of.

		/** @name Override (instrument information)
		 * Override by derived classes to provide information about the instrument.
		*/
		///@{
		/**
		 * @brief Determines whether the function generator has the capability to
		 * set the phase of the generated waveform.
		 * @return Return true if the phase is adjustable, false otherwise.
		*/
		virtual bool IsPhaseAdjustable() const noexcept { return true; }
		///@}

		/** @name Override (instrument tasks)
		 * Override by derived classes to insert tasks into the instrument's task queue.
		 * Logical const-ness: const member functions to allow modules inserting tasks into
		 * the instrument's task queue.
		*/
		///@{
		/**
		 * @brief Generates a sine function.
		 * @param PersistParams Determines whether to store (true) or not (false)
		 * the given waveform parameters in the instrument parameters to save them
		 * to the project file.
		 * @copydetails FunctionGeneratorTasks::SetSineFunctionTask::SetSineFunctionTask
		 * @throws Util::NotAvailableException is thrown if @p GetWaveformCaps() does not
		 * contain WaveformCapsType::UserDefined (the instrument cannot generate
		 * software-defined waveforms).
		*/
		virtual void SetSineFunction(const FunctionGeneratorDefs::SineFunctionDescType& FunctionDesc,
			bool PersistParams = false, bool Autostart = false, DynExp::TaskBase::CallbackType CallbackFunc = nullptr) const;

		/**
		 * @brief Generates a rectangular function.
		 * @param PersistParams Determines whether to store (true) or not (false)
		 * the given waveform parameters in the instrument parameters to save them
		 * to the project file.
		 * @copydetails FunctionGeneratorTasks::SetRectFunctionTask::SetRectFunctionTask
		 * @throws Util::NotAvailableException is thrown if @p GetWaveformCaps() does not
		 * contain WaveformCapsType::UserDefined (the instrument cannot generate
		 * software-defined waveforms).
		*/
		virtual void SetRectFunction(const FunctionGeneratorDefs::RectFunctionDescType& FunctionDesc,
			bool PersistParams = false, bool Autostart = false, DynExp::TaskBase::CallbackType CallbackFunc = nullptr) const;

		/**
		 * @brief Generates a ramp function.
		 * @param PersistParams Determines whether to store (true) or not (false)
		 * the given waveform parameters in the instrument parameters to save them
		 * to the project file.
		 * @copydetails FunctionGeneratorTasks::SetRampFunctionTask::SetRampFunctionTask
		 * @throws Util::NotAvailableException is thrown if @p GetWaveformCaps() does not
		 * contain WaveformCapsType::UserDefined (the instrument cannot generate
		 * software-defined waveforms).
		*/
		virtual void SetRampFunction(const FunctionGeneratorDefs::RampFunctionDescType& FunctionDesc,
			bool PersistParams = false, bool Autostart = false, DynExp::TaskBase::CallbackType CallbackFunc = nullptr) const;

		/**
		 * @brief Generates a function consisting of pulse segments.
		 * @param PersistParams Determines whether to store (true) or not (false)
		 * the given waveform parameters in the instrument parameters to save them
		 * to the project file.
		 * @copydetails FunctionGeneratorTasks::SetPulseFunctionTask::SetPulseFunctionTask
		 * @throws Util::NotAvailableException is thrown if @p GetWaveformCaps() does not
		 * contain WaveformCapsType::UserDefined (the instrument cannot generate
		 * software-defined waveforms).
		*/
		virtual void SetPulseFunction(const FunctionGeneratorDefs::PulsesDescType& FunctionDesc,
			bool PersistParams = false, bool Autostart = false, DynExp::TaskBase::CallbackType CallbackFunc = nullptr) const;

		/**
		 * @brief Generates an arbitrary function defined by the function samples.
		 * @copydetails FunctionGeneratorTasks::SetArbitraryFunctionTask::SetArbitraryFunctionTask
		 * @throws Util::NotAvailableException is thrown if @p GetWaveformCaps() does not
		 * contain WaveformCapsType::UserDefined (the instrument cannot generate
		 * software-defined waveforms).
		*/
		virtual void SetArbitraryFunction(DataStreamBase::BasicSampleListType&& Samples,
			bool Autostart = false, DynExp::TaskBase::CallbackType CallbackFunc = nullptr) const;

		/**
		 * @brief Configures the function generator to perform a modulation.
		 * @param ModulationDesc Description of the modulation
		 * @param PersistParams Determines whether to store (true) or not (false)
		 * the given modulation settings in the instrument parameters to save them
		 * to the project file.
		 * @param CallbackFunc @copybrief DynExp::TaskBase::CallbackFunc
		 * @throws Util::NotImplementedException is thrown by the default implementation, which
		 * does not do anything more.
		*/
		virtual void SetModulation(const FunctionGeneratorDefs::ModulationDescType& ModulationDesc,
			bool PersistParams = false, DynExp::TaskBase::CallbackType CallbackFunc = nullptr) const;

		/**
		 * @brief Configures the function generator to perform a sweep.
		 * @param SweepDesc Description of the sweep
		 * @param PersistParams Determines whether to store (true) or not (false)
		 * the given sweep settings in the instrument parameters to save them
		 * to the project file.
		 * @param CallbackFunc @copybrief DynExp::TaskBase::CallbackFunc
		 * @throws Util::NotImplementedException is thrown by the default implementation, which
		 * does not do anything more.
		*/
		virtual void SetSweep(const FunctionGeneratorDefs::SweepDescType& SweepDesc,
			bool PersistParams = false, DynExp::TaskBase::CallbackType CallbackFunc = nullptr) const;

		/**
		 * @brief @p SetTrigger() configures the function generator's trigger, which determines when
		 * the waveform generation is started. It firstly enqueues the task FunctionGeneratorTasks::SetTriggerTask
		 * ignoring @p CallbackFunc. Then, @p SetTriggerChild() is called, which should enqueue
		 * another task making use of @p CallbackFunc to configure the actual hardware accordingly.
		 * This ensures that @p CallbackFunc is called last in this series of multiple tasks.
		 * @copydetails FunctionGeneratorTasks::SetTriggerTask::SetTriggerTask
		 * @param PersistParams Determines whether to store (true) or not (false)
		 * the given trigger settings in the instrument parameters to save them
		 * to the project file.
		 * @param CallbackFunc @copybrief DynExp::TaskBase::CallbackFunc
		 * @throws Util::NotImplementedException is thrown by the default implementation
		 * of @p SetTriggerChild().
		*/
		void SetTrigger(const FunctionGeneratorDefs::TriggerDescType& TriggerDesc,
			bool PersistParams = false, DynExp::TaskBase::CallbackType CallbackFunc = nullptr) const;

		/**
		 * @brief Forces the generation of the waveform ignoring the trigger.
		 * @param CallbackFunc @copybrief DynExp::TaskBase::CallbackFunc
		 * @throws Util::NotImplementedException is thrown by the default implementation, which
		 * does not do anything more.
		*/
		virtual void ForceTrigger(DynExp::TaskBase::CallbackType CallbackFunc = nullptr) const;

		/**
		 * @brief Synchronized version of @p ForceTrigger(), which blocks until the task
		 * enqueued by @p ForceTrigger() has been executed.
		*/
		virtual void ForceTriggerSync() const;
		///@}

	private:
		void ResetImpl(dispatch_tag<DataStreamInstrument>) override final;
		virtual void ResetImpl(dispatch_tag<FunctionGenerator>) = 0;		//!< @copydoc ResetImpl(dispatch_tag<DataStreamInstrument>)

		/** @name Override (instrument information)
		 * Override by derived classes to provide information about the instrument.
		*/
		///@{
		virtual FunctionGeneratorDefs::FunctionDescType GetMinCapsChild() const { return FunctionGeneratorDefs::GetDefaultMinFunctionDesc(); }				//!< @copydoc GetMinCaps
		virtual FunctionGeneratorDefs::FunctionDescType GetMaxCapsChild() const { return FunctionGeneratorDefs::GetDefaultMaxFunctionDesc(); }				//!< @copydoc GetMaxCaps
		virtual FunctionGeneratorDefs::FunctionDescType GetParamDefaultsChild() const { return FunctionGeneratorDefs::GetDefaultDefaultFunctionDesc(); }	//!< @copydoc GetParamDefaults
		virtual Util::FeatureTester<WaveformCapsType> GetWaveformCapsChild() const = 0;																		//!< @copydoc GetWaveformCaps
		virtual Util::FeatureTester<QuantityCapsType> GetModulationCapsChild() const { return {}; }															//!< @copydoc GetModulationCaps
		virtual Util::FeatureTester<QuantityCapsType> GetSweepCapsChild() const { return {}; }																//!< @copydoc GetSweepCaps
		virtual Util::FeatureTester<TriggerCapsType> GetTriggerCapsChild() const { return {}; }																//!< @copydoc GetTriggerCaps
		///@}

		/** @name Override
		 * Override by derived classes.
		*/
		///@{
		/**
		 * @copydoc SetTrigger
		*/
		virtual void SetTriggerChild(const FunctionGeneratorDefs::TriggerDescType& TriggerDesc,
			bool PersistParams, DynExp::TaskBase::CallbackType CallbackFunc) const;
		///@}

		virtual std::unique_ptr<DynExp::InitTaskBase> MakeInitTask() const override { return DynExp::MakeTask<FunctionGeneratorTasks::InitTask>(); }
		virtual std::unique_ptr<DynExp::ExitTaskBase> MakeExitTask() const override { return DynExp::MakeTask<FunctionGeneratorTasks::ExitTask>(); }
		virtual std::unique_ptr<DynExp::UpdateTaskBase> MakeUpdateTask() const override { return DynExp::MakeTask<FunctionGeneratorTasks::UpdateTask>(); }
	};
}