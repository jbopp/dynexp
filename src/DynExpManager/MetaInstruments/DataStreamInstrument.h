// This file is part of DynExp.

/**
 * @file DataStreamInstrument.h
 * @brief Implementation of a data stream meta instrument and of data streams
 * input/output devices might work on.
*/

#pragma once

#include "stdafx.h"
#include "Instrument.h"

namespace DynExpInstr
{
	class DataStreamInstrument;

	/**
	 * @brief Tasks for @p DataStreamInstrument
	*/
	namespace DataStreamInstrumentTasks
	{
		/**
		 * @copydoc DynExp::InitTaskBase
		*/
		class InitTask : public DynExp::InitTaskBase
		{
			void InitFuncImpl(dispatch_tag<InitTaskBase>, DynExp::InstrumentInstance& Instance) override final { InitFuncImpl(dispatch_tag<InitTask>(), Instance); }

			/**
			 * @copydoc InitFuncImpl(dispatch_tag<DynExp::InitTaskBase>, DynExp::InstrumentInstance&)
			*/
			virtual void InitFuncImpl(dispatch_tag<InitTask>, DynExp::InstrumentInstance& Instance) {}
		};

		/**
		 * @copydoc DynExp::ExitTaskBase
		*/
		class ExitTask : public DynExp::ExitTaskBase
		{
			void ExitFuncImpl(dispatch_tag<ExitTaskBase>, DynExp::InstrumentInstance& Instance) override final { ExitFuncImpl(dispatch_tag<ExitTask>(), Instance); }

			/**
			 * @copydoc ExitFuncImpl(dispatch_tag<DynExp::ExitTaskBase>, DynExp::InstrumentInstance&)
			*/
			virtual void ExitFuncImpl(dispatch_tag<ExitTask>, DynExp::InstrumentInstance& Instance) {}
		};

		/**
		 * @copydoc DynExp::UpdateTaskBase
		*/
		class UpdateTask : public DynExp::UpdateTaskBase
		{
			void UpdateFuncImpl(dispatch_tag<UpdateTaskBase>, DynExp::InstrumentInstance& Instance) override final { UpdateFuncImpl(dispatch_tag<UpdateTask>(), Instance); }

			/**
			 * @copydoc UpdateFuncImpl(dispatch_tag<DynExp::UpdateTaskBase>, DynExp::InstrumentInstance&)
			*/
			virtual void UpdateFuncImpl(dispatch_tag<UpdateTask>, DynExp::InstrumentInstance& Instance) {}
		};

		/**
		 * @brief Task to set the size of the related data stream instrument's stream.
		*/
		class SetStreamSizeTask final : public DynExp::TaskBase
		{
		public:
			/**
			 * @brief Constructs a @p SetStreamSizeTask instance.
			 * @param BufferSizeInSamples @copybrief #BufferSizeInSamples
			 * @param CallbackFunc @copybrief DynExp::TaskBase::CallbackFunc
			*/
			SetStreamSizeTask(size_t BufferSizeInSamples, CallbackType CallbackFunc) noexcept : TaskBase(CallbackFunc), BufferSizeInSamples(BufferSizeInSamples) {}

		private:
			virtual DynExp::TaskResultType RunChild(DynExp::InstrumentInstance& Instance) override;

			const size_t BufferSizeInSamples;	//!< New stream buffer size in samples
		};
	}

	/**
	 * @brief Defines a trivially-copyable basic sample as time (t)-value (f(t)) pairs (t, f(t)).
	*/
	struct BasicSample
	{
		using DataType = double;	//!< Data type of time and value.

		/**
		 * @brief Constructs a @p BasicSample instance setting #Value and #Time to zero.
		*/
		constexpr BasicSample() noexcept : Value(0), Time(0) {}

		/**
		 * @brief Constructs a @p BasicSample instance setting #Time to zero.
		 * @param Value Value to initialize #Value with
		*/
		constexpr BasicSample(DataType Value) noexcept : Value(Value), Time(0) {}

		/**
		 * @brief Constructs a @p BasicSample instance.
		 * @param Value Value to initialize #Value with
		 * @param Time Time to initialize #Time with
		*/
		constexpr BasicSample(DataType Value, DataType Time) noexcept : Value(Value), Time(Time) {}

		DataType Value;				//!< Value in a unit as specified by the class derived from @p DataStreamInstrument
		DataType Time;				//!< Time in seconds
	};

	/**
	 * @brief Base class for all data streams compatible with the data stream
	 * instrument's data class @p DataStreamInstrumentData.
	*/
	class DataStreamBase
	{
	public:
		/**
		 * @brief Type of a list containing data stream samples of type @p BasicSample.
		*/
		using BasicSampleListType = std::vector<BasicSample>;

		virtual ~DataStreamBase() = default;

		/** @name Override
		 * Override by derived classes.
		*/
		///@{
		/**
		 * @brief Determines whether the data stream holds samples which are compatible
		 * to @p BasicSample.
		 * @return Return true if the derived data stream's samples can be converted
		 * to @p BasicSample instances, false otherwise.
		*/
		virtual bool IsBasicSampleConvertible() const noexcept { return false; }

		/**
		 * @brief Determines whether the data stream holds basic samples
		 * (@p IsBasicSampleConvertible() returns true) which contain
		 * information in their BasicSample::Time fields.
		 * @return Return true if the derived data stream's basic samples use the
		 * BasicSample::Time variable, false otherwise.
		*/
		virtual bool IsBasicSampleTimeUsed() const noexcept { return false; }

		/**
		 * @brief Moves the read/write pointer to the first sample in the stream.
		 * @param Which Combination of flags @p std::ios_base::in (read) and
		 * @p std::ios_base::out (write) specifying the pointers to move.
		*/
		virtual void SeekBeg(std::ios_base::openmode Which = std::ios_base::in | std::ios_base::out) = 0;

		/**
		 * @brief Moves the read pointer to the last sample in the stream and/or
		 * moves the write pointer after the last written sample in the stream.
		 * @param Which Combination of flags @p std::ios_base::in (read) and
		 * @p std::ios_base::out (write) specifying the pointers to move.
		*/
		virtual void SeekEnd(std::ios_base::openmode Which = std::ios_base::in | std::ios_base::out) = 0;

		/**
		 * @brief Moves the read/write pointer to the respective other one.
		 * @param Which Combination of flags @p std::ios_base::in (read) and
		 * @p std::ios_base::out (write) specifying the pointers to move.
		 * @return Return true in case of success, false otherwise.
		*/
		virtual bool SeekEqual(std::ios_base::openmode Which = std::ios_base::in | std::ios_base::out) = 0;

		/**
		 * @brief Determines the stream's read buffer size in samples.
		 * @return Read buffer size in samples
		*/
		virtual size_t GetStreamSizeRead() const noexcept = 0;

		/**
		 * @brief Determines the stream's write buffer size in samples.
		 * @return Write buffer size in samples
		*/
		virtual size_t GetStreamSizeWrite() const noexcept = 0;

		/**
		 * @brief Determines the number of samples which have been written to
		 * the stream in total. Before overflowing, this function should keep
		 * continuing to return the largest possible value.
		 * @return Total number of samples written to the stream
		*/
		virtual size_t GetNumSamplesWritten() const noexcept = 0;

		/**
		 * @brief Sets the stream size in samples.
		 * @param BufferSizeInSamples New stream size in samples
		*/
		virtual void SetStreamSize(size_t BufferSizeInSamples) = 0;
		///@}

		/**
		 * @brief Determines whether the stream contains at least one sample which
		 * can be read (i.e. @p GetStreamSizeRead() returns a value greater than 0).
		 * @return Returns true if there is a sample to read, false otherwise.
		*/
		bool CanRead() const { return GetStreamSizeRead(); }

		/**
		 * @brief Removes all samples from the stream's buffer.
		*/
		void Clear() { ClearChild(); }

		/**
		 * @brief Writes a single basic sample to the stream.
		 * @param Sample Sample to write
		*/
		void WriteBasicSample(const BasicSample& Sample) { WriteBasicSampleChild(Sample); }

		/**
		 * @brief Reads a single basic sample from the stream.
		 * @return Read sample
		*/
		BasicSample ReadBasicSample() { return ReadBasicSampleChild(); }

		/**
		 * @brief Writes a list of basic sample to the stream.
		 * @param Samples List of samples to write
		*/
		void WriteBasicSamples(const BasicSampleListType& Samples);

		/**
		 * @brief Reads a list of basic sample from the stream.
		 * @return Read samples
		*/
		BasicSampleListType ReadBasicSamples(size_t Count);

	private:
		/** @name Override
		 * Override by derived classes.
		*/
		///@{
		/**
		 * @copydoc WriteBasicSample
		 * @throws Util::NotImplementedException is thrown by the default implementation, which
		 * does not do anything more.
		*/
		virtual void WriteBasicSampleChild(const BasicSample& Sample);

		/**
		 * @copydoc ReadBasicSample
		 * @throws Util::NotImplementedException is thrown by the default implementation, which
		 * does not do anything more.
		*/
		virtual BasicSample ReadBasicSampleChild();

		virtual void ClearChild() = 0;		//!< @copydoc Clear
		///@}
	};

	/**
	 * @brief Type of a pointer owning a data stream instance derived from @p DataStreamBase.
	*/
	template <typename T>
	using DataStreamPtrType = std::unique_ptr<T>;

	/**
	 * @brief Type of a pointer owning a @p DataStreamBase instance.
	*/
	using DataStreamBasePtrType = DataStreamPtrType<DataStreamBase>;

	/**
	 * @brief Base class for all circular data streams based on Util::circularbuf.
	*/
	class CircularDataStreamBase : public DataStreamBase
	{
	protected:
		CircularDataStreamBase() = default;
		virtual ~CircularDataStreamBase() = default;

	public:
		/** @name Override
		 * Override by derived classes.
		*/
		///@{
		/**
		 * @brief Determines the amount of samples which can be read from the stream's
		 * current get pointer position till the stream's end.
		 * @return Returns the amount of available samples.
		*/
		virtual size_t GetNumAvailableSamplesToReadTillEnd() const noexcept = 0;

		/**
		 * @brief Determines the amount of samples which can be written to the stream
		 * untill the stream's end is reached.
		 * @return Returns the amount of free samples.
		*/
		virtual size_t GetNumFreeSamplesToWrite() const noexcept = 0;

		/**
		 * @brief Determines the current position of the stream's read (get) pointer
		 * in samples.
		 * @return Returns the get pointer position in samples.
		*/
		virtual std::streampos GetReadPosition() const noexcept = 0;

		/**
		 * @brief Determines the current position of the stream's write (put) pointer
		 * in samples.
		 * @return Returns the put pointer position in samples.
		*/
		virtual std::streampos GetWritePosition() const noexcept = 0;

		/**
		 * @brief Moves the stream's read/write pointer(s) to a position relative to @p SeekDir.
		 * @param OffsetInSamples Destiny position (relative to @p SeekDir)
		 * @param SeekDir Set position relative to stream's beginning (@p std::ios_base::beg), its end
		 * (@p std::ios_base::end) or the current position (@p std::ios_base::cur).
		 * @param Which Combination of flags @p std::ios_base::in (read) and
		 * @p std::ios_base::out (write) specifying the pointers to move.
		 * @return Returns true in case of success, false otherwise.
		*/
		virtual bool SeekRel(signed long long OffsetInSamples, std::ios_base::seekdir SeekDir,
			std::ios_base::openmode Which = std::ios_base::in | std::ios_base::out) = 0;

		/**
		 * @brief Moves the stream's read/write pointer(s) to an absolute position.
		 * @param PositionInSamples Destiny position
		 * @param Which Combination of flags @p std::ios_base::in (read) and
		 * @p std::ios_base::out (write) specifying the pointers to move.
		 * @return Returns true in case of success, false otherwise.
		*/
		virtual bool SeekAbs(unsigned long long PositionInSamples, std::ios_base::openmode Which = std::ios_base::in | std::ios_base::out) = 0;
		///@}

		/**
		 * @brief Determines the amount of samples which have been written to the
		 * stream after the last @p Count samples. This is useful if the caller
		 * just wants to obtain recent samples but not the entire stream. Then, the
		 * caller should remember the amount of samples it knows already and call
		 * @p ReadRecentBasicSamples() next.
		 * @param Count Amount of samples which are known by the caller.
		 * @return Number of samples written to the stream recently.
		 */
		size_t GetNumRecentBasicSamples(size_t Count) const;

		/**
		 * @brief Reads the most recent samples from the stream skipping
		 * @p Count samples. Also refer to @p GetNumRecentBasicSamples().
		 * @param Count Amount of samples which are known by the caller.
		 * @return Read samples
		*/
		BasicSampleListType ReadRecentBasicSamples(size_t Count);
	};

	/**
	 * @brief Implements a circular data stream based on Util::circularbuf
	 * @tparam SampleT Type of the samples stored in the data stream
	*/
	template <
		typename SampleT,
		std::enable_if_t<std::is_trivially_copyable_v<SampleT>, int> = 0
	>
	class CircularDataStream : public CircularDataStreamBase
	{
	public:
		using SampleType = SampleT;		//!< Alias for @p SampleT

		/**
		 * @brief Constructs a @p CircularDataStream instance.
		 * @param BufferSizeInSamples Initial stream buffer size in samples
		*/
		CircularDataStream(size_t BufferSizeInSamples)
			: StreamBuffer(sizeof(SampleT) * BufferSizeInSamples), Stream(&StreamBuffer), NumSamplesWritten(0)
		{
			Stream.exceptions(std::iostream::failbit | std::iostream::badbit);
		}

		virtual ~CircularDataStream() = default;

		/**
		 * @brief Determines the size of a single sample in bytes
		 * @return Returns @p sizeof(SampleT).
		*/
		constexpr auto GetBytesPerSample() noexcept { return sizeof(SampleT); }

		size_t GetNumAvailableSamplesToReadTillEnd() const noexcept override { return (StreamBuffer.gsize() - StreamBuffer.gtellp()) / sizeof(SampleT); }
		size_t GetNumFreeSamplesToWrite() const noexcept override { return (StreamBuffer.psize() - StreamBuffer.ptellp()) / sizeof(SampleT); }
		std::streampos GetReadPosition() const noexcept override { return StreamBuffer.gtellp() / sizeof(SampleT); }
		std::streampos GetWritePosition() const noexcept override { return StreamBuffer.ptellp() / sizeof(SampleT); }

		virtual bool SeekRel(signed long long OffsetInSamples, std::ios_base::seekdir SeekDir,
			std::ios_base::openmode Which = std::ios_base::in | std::ios_base::out) override
		{
			return StreamBuffer.pubseekoff(OffsetInSamples * sizeof(SampleT), SeekDir, Which) !=
				Util::circularbuf::pos_type(Util::circularbuf::off_type(-1));
		}

		virtual bool SeekAbs(unsigned long long PositionInSamples,
			std::ios_base::openmode Which = std::ios_base::in | std::ios_base::out) override
		{
			return StreamBuffer.pubseekpos(PositionInSamples * sizeof(SampleT), Which) !=
				Util::circularbuf::pos_type(Util::circularbuf::off_type(-1));
		}

		virtual void SeekBeg(std::ios_base::openmode Which = std::ios_base::in | std::ios_base::out) override
		{
			SeekAbs(0, Which);
		}

		virtual void SeekEnd(std::ios_base::openmode Which = std::ios_base::in | std::ios_base::out) override
		{
			if (Which & std::ios_base::in)
				SeekRel(-1, std::ios_base::end, std::ios_base::in);
			if (Which & std::ios_base::out)
				SeekRel(0, std::ios_base::end, std::ios_base::out);
		}

		virtual bool SeekEqual(std::ios_base::openmode Which = std::ios_base::in | std::ios_base::out) override
		{
			if (Which & std::ios_base::in)
			{
				StreamBuffer.pubsync();

				if (GetStreamSizeRead() >= GetStreamSizeWrite())
					SeekAbs(GetWritePosition(), std::ios_base::in);
				else
					return false;
			}

			// Read buffer is never larger than write buffer.
			if (Which & std::ios_base::out)
				SeekAbs(GetReadPosition(), std::ios_base::out);

			return true;
		}

		virtual size_t GetStreamSizeRead() const noexcept override { return StreamBuffer.gsize() / sizeof(SampleT);}
		virtual size_t GetStreamSizeWrite() const noexcept override { return StreamBuffer.psize() / sizeof(SampleT); }
		virtual size_t GetNumSamplesWritten() const noexcept override { return NumSamplesWritten; }
		virtual void SetStreamSize(size_t BufferSizeInSamples) override { StreamBuffer.resize(sizeof(SampleT) * BufferSizeInSamples); }

		/** @name (De)serialization
		 * Functions for (de)serialization of trivially-copyable types
		*/
		///@{
		/**
		 * @brief Writes a single sample to the stream's buffer #StreamBuffer.
		 * @param Sample Sample to write
		*/
		void WriteSample(const SampleT& Sample)
		{
			ValidateSample(Sample);

			Stream.write(reinterpret_cast<const char*>(&Sample), sizeof(SampleT));
			
			// Should never overflow. Let's assume 10 Gb/s = 1.25e9 B/s transmission rate:
			// Overflows in (2^64 - 1) B / 1.25e9 B/s / 60 / 60 / 24 / 365 = 468 years.
			if (NumSamplesWritten < std::numeric_limits<decltype(NumSamplesWritten)>::max())
				++NumSamplesWritten;
		}

		/**
		 * @brief Writes a single sample to the stream's buffer #StreamBuffer by implicitly
		 * constructing the sample from a @p std::chrono::duration instance.
		 * @tparam Rep Type of the number of ticks of a @p std::chrono::duration instance.
		 * @tparam Period @p std::ratio representing the type of a fraction, which denotes the
		 * time in seconds in between subsequent ticks, of a @p std::chrono::duration instance.
		 * @param Sample Time sample to write
		*/
		template <typename Rep, typename Period>
		void WriteSample(const std::chrono::duration<Rep, Period>& Sample)
		{
			WriteSample(Sample.count());
		}

		/**
		 * @brief Reads a single sample from the stream's buffer #StreamBuffer.
		 * @return Sample read
		*/
		SampleT ReadSample()
		{
			SampleT Sample;
			Stream.read(reinterpret_cast<char*>(&Sample), sizeof(SampleT));

			return Sample;
		}

		/**
		 * @brief Writes multiple samples to the stream's buffer #StreamBuffer.
		 * @tparam T Type implicitly convertible to @p SampleT
		 * @param Samples List of samples to write
		*/
		template <typename T>
		void WriteSamples(const std::vector<T>& Samples)
		{
			for (const auto& Sample : Samples)
				WriteSample(Sample);
		}

		/**
		 * @brief Reads multiple samples from the stream's buffer #StreamBuffer.
		 * @param Count Amount of samples to read
		 * @return Samples read
		*/
		std::vector<SampleT> ReadSamples(size_t Count)
		{
			std::vector<SampleT> Samples;

			for (decltype(Count) i = 0; i < Count; ++i)
				Samples.push_back(ReadSample());

			return Samples;
		}

	protected:
		virtual void ClearChild() override
		{
			Stream.clear();
			StreamBuffer.clear();
		}

	private:
		/** @name Override
		 * Override by derived classes.
		*/
		///@{
		/**
		 * @brief Checks whether a sample is considered valid. Derived classes
		 * can define rules to check the sample for validity. Does nothing if
		 * the sample is valid.
		 * @param Sample Sample to check
		 * @throws Util::InvalidDataException is thrown if @p Sample is invalid.
		*/
		virtual void ValidateSample(const SampleT& Sample) const {}
		///@}

		mutable Util::circularbuf StreamBuffer;		//!< Circular stream buffer
		std::iostream Stream;						//!< Stream to operate on #StreamBuffer
		size_t NumSamplesWritten;					//!< Amount of samples which have been written to #Stream in total
	};

	/**
	 * @brief Implements a circular data stream based on Util::circularbuf
	 * using samples of type @p BasicSample.
	*/
	class BasicSampleStream : public CircularDataStream<BasicSample>
	{
	public:
		/**
		 * @brief Constructs a @p BasicSampleStream instance.
		 * @copydetails CircularDataStream::CircularDataStream
		*/
		BasicSampleStream(size_t BufferSizeInSamples) : CircularDataStream(BufferSizeInSamples) {}

		virtual ~BasicSampleStream() = default;

		bool IsBasicSampleConvertible() const noexcept override final { return true; }
		bool IsBasicSampleTimeUsed() const noexcept override final { return true; }

	private:
		virtual void WriteBasicSampleChild(const BasicSample& Sample) override { WriteSample(Sample); }
		virtual BasicSample ReadBasicSampleChild() override { return ReadSample(); }
	};

	/**
	 * @brief Implements a circular data stream based on Util::circularbuf
	 * using samples of an arithmetic type @p SampleT.
	 * @tparam SampleT Arithmetic type of the samples stored in the data stream
	*/
	template <
		typename SampleT,
		std::enable_if_t<std::is_arithmetic_v<SampleT>, int> = 0
	>
	class NumericSampleStream : public CircularDataStream<SampleT>
	{
	public:
		/**
		 * @brief Constructs a @p NumericSampleStream instance setting the limits
		 * on the sample values to the respective data type's (@p SampleT) full range.
		 * @copydetails CircularDataStream::CircularDataStream
		*/
		NumericSampleStream(size_t BufferSizeInSamples) : CircularDataStream<SampleT>(BufferSizeInSamples),
			MinValue(std::numeric_limits<SampleT>::lowest()), MaxValue(std::numeric_limits<SampleT>::max()) {}
		
		/**
		 * @brief Constructs a @p NumericSampleStream instance setting the limits
		 * on the sample values to the the range [@p MinValue, @p MaxValue].
		 * @copydetails CircularDataStream::CircularDataStream
		 * @param MinValue Minimal allowed sample value
		 * @param MaxValue Maximal allowed sample value
		*/
		NumericSampleStream(size_t BufferSizeInSamples, SampleT MinValue, SampleT MaxValue)
			: CircularDataStream<SampleT>(BufferSizeInSamples), MinValue(MinValue), MaxValue(MaxValue) {}
		
		virtual ~NumericSampleStream() = default;

		bool IsBasicSampleConvertible() const noexcept override final { return true; }

		/**
		 * @brief Sets new sample value limits.
		 * @param MinValue Minimal allowed sample value
		 * @param MaxValue Maximal allowed sample value
		*/
		void SetLimits(SampleT MinValue, SampleT MaxValue)
		{
			this->MinValue = MinValue;
			this->MaxValue = MaxValue;
		}

		/**
		 * @brief Getter for the minimal allowed sample value
		 * @return Returns #MinValue.
		*/
		auto GetMinValue() const noexcept { return MinValue; }

		/**
		 * @brief Getter for the maximal allowed sample value
		 * @return Returns #MaxValue.
		*/
		auto GetMaxValue() const noexcept { return MaxValue; }

	private:
		/**
		 * @copydoc DataStreamBase::WriteBasicSample
		 * @brief The time value of @p Sample is ignored.
		 * @warning Overflow might occur if BasicSample::DataType does not fit into @p SampleT!
		*/
		virtual void WriteBasicSampleChild(const BasicSample& Sample) override
		{
			CircularDataStream<SampleT>::WriteSample(static_cast<SampleT>(Sample.Value));
		}

		/**
		 * @copydoc DataStreamBase::ReadBasicSample
		 * @brief The time value of the returned sample is set to 0.
		*/
		virtual BasicSample ReadBasicSampleChild() override
		{
			BasicSample Sample;

			// Ignore time value (set it to 0). It has been considered (and discarded) to set it to
			// std::chrono::system_clock::now() instead. This would be misleading since a timestamp
			// which is set here refers to the time when the sample is read from the buffer - not
			// to the time when it is actually acquired by the underlying hardware.
			Sample.Time = 0;
			Sample.Value = static_cast<decltype(Sample.Value)>(CircularDataStream<SampleT>::ReadSample());

			return Sample;
		}

		virtual void ValidateSample(const SampleT& Sample) const override
		{
			if (Sample < MinValue || Sample > MaxValue)
				throw Util::InvalidDataException(
					"A sample cannot be written to a stream buffer because it exceeds the specified limits (min: " + std::to_string(MinValue)
					+ ", max: " + std::to_string(MaxValue) + ", sample: " + std::to_string(Sample) + ")");
		}

		SampleT MinValue;		//!< Minimal allowed sample value
		SampleT MaxValue;		//!< Maximal allowed sample value
	};

	using AnalogSampleStream = NumericSampleStream<double>;						//!< Alias for a numeric sample stream of @p double samples
	using DigitalSampleStream = NumericSampleStream<uint8_t>;					//!< Alias for a numeric sample stream of @p uint8_t (single byte) samples
	using AnalogSampleStreamPtrType = DataStreamPtrType<AnalogSampleStream>;	//!< Alias for a pointer owning a @p AnalogSampleStream instance
	using DigitalSampleStreamPtrType = DataStreamPtrType<DigitalSampleStream>;	//!< Alias for a pointer owning a @p DigitalSampleStream instance

	/**
	 * @brief Bundles parameters to describe a data stream's stream size.
	*/
	class StreamSizeParamsExtension
	{
		static constexpr ParamsConfigDialog::NumberType DefaultStreamSize = 1000;	//!< Default stream size in samples

	public:
		/**
		 * @brief Type containing the values of all the parameters
		 * belonging to @p StreamSizeParamsExtension.
		*/
		struct ValueType
		{
			ParamsConfigDialog::NumberType StreamSize = DefaultStreamSize;			//!< @copydoc StreamSizeParamsExtension::StreamSize
		};

		/**
		 * @brief Constructs a @p StreamSizeParamsExtension instance.
		 * @param Owner Parameter class owning the parameters bundled by this instance.
		 * @param DefaultValue Default value of #StreamSize
		 * @param MinValue Minimal value of #StreamSize
		 * @param MaxValue Maximal value of #StreamSize
		*/
		StreamSizeParamsExtension(DynExp::ParamsBase& Owner,
			ParamsConfigDialog::NumberType DefaultValue = DefaultStreamSize, ParamsConfigDialog::NumberType MinValue = 1,
			ParamsConfigDialog::NumberType MaxValue = static_cast<double>(std::numeric_limits<size_t>::max()))
			: StreamSize{ Owner, "StreamSize", "Stream size",
				"Size of the instrument's sample stream buffer in samples.", true, DefaultValue, MinValue, MaxValue, 1, 0 }
		{}

		/**
		 * @brief Calls DynExp::ParamsBase::DisableUserEditable() on all bundled parameters.
		*/
		void DisableUserEditable();

		/**
		 * @brief Creates and returns a @p ValueType instance containing the bundled
		 * parameters' values.
		 * @return Bundled parameters' current values
		*/
		ValueType Values() const { return { StreamSize }; }

		/**
		 * @brief Stream size of the related @p DataStreamInstrument instance's sample
		 * stream in samples.
		*/
		DynExp::ParamsBase::Param<ParamsConfigDialog::NumberType> StreamSize;
	};

	/**
	 * @brief Bundles parameters to describe a @p NumericSampleStream's sampling properties.
	*/
	class NumericSampleStreamParamsExtension
	{
	public:
		/**
		 * @brief Type to determine how to record/play back to/from the stream.
		*/
		enum SamplingModeType { Single, Continuous };
		/**
		 * @var NumericSampleStreamParamsExtension::SamplingModeType NumericSampleStreamParamsExtension::Single
		 * Record/play back to/from the stream only once (until the stream is full
		 * or all data has been read).
		*/
		/**
		 * @var NumericSampleStreamParamsExtension::SamplingModeType NumericSampleStreamParamsExtension::Continuous
		 * Record/play back to/from the stream continuously.
		*/

	private:
		static constexpr ParamsConfigDialog::NumberType DefaultSamplingRate = 1;			//!< Default sampling rate in samples/s
		static constexpr SamplingModeType DefaultSamplingMode = SamplingModeType::Single;	//!< Default sampling mode

	public:
		/**
		 * @brief Type containing the values of all the parameters
		 * belonging to @p NumericSampleStreamParamsExtension.
		*/
		struct ValueType
		{
			ParamsConfigDialog::NumberType SamplingRate = DefaultSamplingRate;				//!< @copydoc NumericSampleStreamParamsExtension::SamplingRate
			SamplingModeType SamplingMode = DefaultSamplingMode;							//!< @copydoc NumericSampleStreamParamsExtension::SamplingMode
		};

		/**
		 * @brief Maps description strings to the @p SamplingModeType enum's items.
		 * @return List containing the description-value mapping
		*/
		static Util::TextValueListType<SamplingModeType> SamplingModeTypeStrList();

		/**
		 * @brief Constructs a @p NumericSampleStreamParamsExtension instance.
		 * @param Owner Parameter class owning the parameters bundled by this instance.
		*/
		NumericSampleStreamParamsExtension(DynExp::ParamsBase& Owner)
			: SamplingRate{ Owner, "SamplingRate", "Sampling rate",
				"Sampling rate in samples per second.", true, DefaultSamplingRate, 0, std::numeric_limits<ParamsConfigDialog::NumberType>::max(), 1, 3 },
			SamplingMode{ Owner, SamplingModeTypeStrList(), "SamplingMode", "Sampling mode",
				"Determines whether reading/writing happens only once or continuously.", true, DefaultSamplingMode }
		{}

		/**
		 * @brief Calls DynExp::ParamsBase::DisableUserEditable() on all bundled parameters.
		*/
		void DisableUserEditable();

		/**
		 * @brief Creates and returns a @p ValueType instance containing the bundled
		 * parameters' values.
		 * @return Bundled parameters' current values
		*/
		ValueType Values() const { return { SamplingRate, SamplingMode}; }

		/**
		 * @brief Sampling rate of the related @p DataStreamInstrument instance's sample
		 * stream in samples per second.
		*/
		DynExp::ParamsBase::Param<ParamsConfigDialog::NumberType> SamplingRate;

		/**
		 * @brief Sampling mode of the related @p DataStreamInstrument instance's sample
		 * stream. Indicates whether reading/writing happens only once or continuously.
		*/
		DynExp::ParamsBase::Param<SamplingModeType> SamplingMode;
	};

	/**
	 * @brief Data class for @p DataStreamInstrument
	*/
	class DataStreamInstrumentData : public DynExp::InstrumentDataBase
	{
	public:
		/**
		 * @brief Data type to represent hardware limits on the sample values to write
		 * to the hardware adapter assigned to the related @p DataStreamInstrument instance.
		*/
		using ValueType = double;

		/**
		 * @brief Units which can be used for data stream instruments.
		 * @warning If this is changed, also change @p ConvertUnitType() functions in
		 * @p NetworkDataStreamInstrument.h and the @p UnitType enumeration in @p Common.proto.
		*/
		enum class UnitType {
			Arbitrary,		//!< Arbitrary units (a.u.)
			LogicLevel,		//!< Logic level (TTL) units (1 or 0)
			Counts,			//!< Count rate in counts per second (cps)
			Volt,			//!< Voltage in Volt (V)
			Ampere,			//!< Electric current in Ampere (A)
			Power_W,		//!< Power in Watt (W)
			Power_dBm		//!< Power in dBm
		};

		/**
		 * @brief Returns a descriptive string of a respective unit to be e.g. used in plots.
		 * @param Unit Unit type as used by data stream instruments. 
		 * @return Returns the human-readable unit string.
		*/
		static const char* UnitTypeToStr(const UnitType& Unit);

		/**
		 * @brief Constructs a @p DataStreamInstrumentData instance.
		 * @param SampleStream Data stream the related @p DataStreamInstrument instance
		 * operates on. The @p DataStreamInstrumentData instance takes ownership of the stream.
		 * @throws Util::InvalidArgException is thrown if @p SampleStream is @p nullptr.
		*/
		DataStreamInstrumentData(DataStreamBasePtrType&& SampleStream);

		virtual ~DataStreamInstrumentData() = default;

		/**
		 * @brief Getter for the data stream instrument's sample stream.
		 * Logical const-ness: always returns a non-const type to allow modules to
		 * directly manipulate the sample stream.
		 * @return Returns #SampleStream.
		*/
		DataStreamBasePtrType::element_type* GetSampleStream() const noexcept { return SampleStream.get(); }

		/**
		 * @brief Casts the data stream instrument's sample stream to a derived data stream type.
		 * @tparam T Type derived from @p DataStreamBase to cast to
		 * @return Returns #SampleStream cast to @p using @p dynamic_cast.
		 * @throws Util::TypeErrorException is thrown if the cast fails.
		*/
		template <typename T>
		auto GetCastSampleStream() const
		{
			auto CastSampleStream = dynamic_cast<T*>(GetSampleStream());
			if (!CastSampleStream)
				throw Util::TypeErrorException();

			return CastSampleStream;
		}

		ValueType GetHardwareMinValue() const noexcept { return HardwareMinValue; }			//!< Returns #HardwareMinValue.
		ValueType GetHardwareMaxValue() const noexcept { return HardwareMaxValue; }			//!< Returns #HardwareMaxValue.
		UnitType GetValueUnit() const noexcept { return ValueUnit; }						//!< Returns #ValueUnit.
		void SetHardwareMinValue(ValueType Value) noexcept { HardwareMinValue = Value; }	//!< Sets #HardwareMinValue.
		void SetHardwareMaxValue(ValueType Value) noexcept { HardwareMaxValue = Value; }	//!< Sets #HardwareMaxValue.
		void SetValueUnit(UnitType Unit) noexcept { ValueUnit = Unit; }						//!< Sets #ValueUnit.

	private:
		void ResetImpl(dispatch_tag<InstrumentDataBase>) override final;
		virtual void ResetImpl(dispatch_tag<DataStreamInstrumentData>) {};	//!< @copydoc ResetImpl(dispatch_tag<DynExp::InstrumentDataBase>)

		const DataStreamBasePtrType SampleStream;							//!< Pointer to the base class of the data stream instrument's sample stream

		/** @name Hardware-specific
		 * These values denote hardware limits/settings and should be obtained from the
		 * hardware adapter assigned to the related @p DataStreamInstrument instance.
		*/
		///@{
		ValueType HardwareMinValue;		//!< Minimal possible value to read/write from/to the hardware adapter
		ValueType HardwareMaxValue;		//!< Maximal possible value to read/write from/to the hardware adapter
		UnitType ValueUnit;				//!< Unit type of the values to be read/written from/to the hardware adapter
		///@}
	};

	/**
	 * @brief Parameter class for @p DataStreamInstrument.
	*/
	class DataStreamInstrumentParams : public DynExp::InstrumentParamsBase
	{
	public:
		/**
		 * @brief Constructs the parameters for a @p DataStreamInstrument instance.
		 * @copydetails DynExp::ParamsBase::ParamsBase
		*/
		DataStreamInstrumentParams(DynExp::ItemIDType ID, const DynExp::DynExpCore& Core) : InstrumentParamsBase(ID, Core) {}

		virtual ~DataStreamInstrumentParams() = 0;

		virtual const char* GetParamClassTag() const noexcept override { return "DataStreamInstrumentParams"; }

	private:
		void ConfigureParamsImpl(dispatch_tag<InstrumentParamsBase>) override final { ConfigureParamsImpl(dispatch_tag<DataStreamInstrumentParams>()); }
		virtual void ConfigureParamsImpl(dispatch_tag<DataStreamInstrumentParams>) {}	//!< @copydoc ConfigureParamsImpl(dispatch_tag<DynExp::InstrumentParamsBase>)

		DummyParam Dummy = { *this };													//!< @copydoc DynExp::ParamsBase::DummyParam
	};

	/**
	 * @brief Configurator class for @p DataStreamInstrument
	*/
	class DataStreamInstrumentConfigurator : public DynExp::InstrumentConfiguratorBase
	{
	public:
		using ObjectType = DataStreamInstrument;
		using ParamsType = DataStreamInstrumentParams;

		DataStreamInstrumentConfigurator() = default;
		virtual ~DataStreamInstrumentConfigurator() = 0;
	};

	/**
	 * @brief Implementation of the data stream meta instrument, which is a base class
	 * for all instruments reading/writing single/multiple values from/to input/output
	 * devices.
	*/
	class DataStreamInstrument : public DynExp::InstrumentBase
	{
	public:
		using ParamsType = DataStreamInstrumentParams;								//!< @copydoc DynExp::Object::ParamsType
		using ConfigType = DataStreamInstrumentConfigurator;						//!< @copydoc DynExp::Object::ConfigType
		using InstrumentDataType = DataStreamInstrumentData;						//!< @copydoc DynExp::InstrumentBase::InstrumentDataType

		/** @name gRPC aliases
		 * Redefined to use this instrument with DynExpInstr::gRPCInstrument.
		*/
		///@{
		using InitTaskType = DataStreamInstrumentTasks::InitTask;					//!< @copydoc DynExp::InitTaskBase
		using ExitTaskType = DataStreamInstrumentTasks::ExitTask;					//!< @copydoc DynExp::ExitTaskBase
		using UpdateTaskType = DataStreamInstrumentTasks::UpdateTask;				//!< @copydoc DynExp::UpdateTaskBase
		///@}

		constexpr static auto Name() noexcept { return "Data Stream Instrument"; }	//!< @copydoc DynExp::SerialCommunicationHardwareAdapter::Name
		constexpr static auto Category() noexcept { return "I/O"; }					//!< @copydoc DynExp::InstrumentBase::Category

		/**
		 * @copydoc DynExp::InstrumentBase::InstrumentBase
		*/
		DataStreamInstrument(const std::thread::id OwnerThreadID, DynExp::ParamsBasePtrType&& Params)
			: InstrumentBase(OwnerThreadID, std::move(Params)) {}

		virtual ~DataStreamInstrument() = 0;

		virtual std::string GetName() const override { return Name(); }
		virtual std::string GetCategory() const override { return Category(); }

		/** @name Override (instrument information)
		 * Override by derived classes to provide information about the instrument.
		*/
		///@{
		/**
		 * @brief Determines which unit corresponds to the values managed by this
		 * @p DataStreamInstrument instance.
		 * Do not enforce @p noexcept to allow overriding functions which throw exceptions.
		 * @return Unit of values in the instrument's data stream
		*/
		virtual DataStreamInstrumentData::UnitType GetValueUnit() const = 0;
		///@}

		/**
		 * @brief Builds and returns a descriptive string of the unit corresponding
		 * to the values managed by this @p DataStreamInstrument instance.
		 * @return Returns the result of a call to DataStreamInstrumentData::UnitTypeToStr()
		 * on @p GetValueUnit().
		*/
		const char* GetValueUnitStr() const noexcept { return DataStreamInstrumentData::UnitTypeToStr(GetValueUnit()); }

		/** @name Override (instrument tasks)
		 * Override by derived classes to insert tasks into the instrument's task queue,
		 * which read/write data from a hardware adapter into the buffer of this instrument's
		 * data stream or vice versa.
		 * Logical const-ness: const member functions to allow modules inserting tasks into
		 * the instrument's task queue.
		*/
		///@{
		/**
		 * @brief Enqueues a task to read data from the hardware to the data stream.
		 * The default implementation does nothing.
		 * @param CallbackFunc @copybrief DynExp::TaskBase::CallbackFunc
		*/
		virtual void ReadData(DynExp::TaskBase::CallbackType CallbackFunc = nullptr) const;

		/**
		 * @brief Enqueues a task to write data from the data stream to the hardware.
		 * @param CallbackFunc @copybrief DynExp::TaskBase::CallbackFunc
		 * @throws Util::NotImplementedException is thrown by the default implementation, which
		 * does not do anything more.
		*/
		virtual void WriteData(DynExp::TaskBase::CallbackType CallbackFunc = nullptr) const;

		/**
		 * @brief Enqueues a task to clear the underlying hardware adapter's buffer.
		 * @param CallbackFunc @copybrief DynExp::TaskBase::CallbackFunc
		*/
		virtual void ClearData(DynExp::TaskBase::CallbackType CallbackFunc = nullptr) const {}

		/**
		 * @brief Enqueues a task to make the underlying hardware adapter start data
		 * acquisition or writing data.
		 * @param CallbackFunc @copybrief DynExp::TaskBase::CallbackFunc
		*/
		virtual void Start(DynExp::TaskBase::CallbackType CallbackFunc = nullptr) const {}

		/**
		 * @brief Enqueues a task to make the underlying hardware adapter stop data
		 * acquisition or writing data.
		 * @param CallbackFunc @copybrief DynExp::TaskBase::CallbackFunc
		*/
		virtual void Stop(DynExp::TaskBase::CallbackType CallbackFunc = nullptr) const {}

		/**
		 * @brief Enqueues a task to make the underlying hardware adapter restart data
		 * acquisition or writing data. The default implementation calls @p Stop() and
		 * then @p Start().
		 * @param CallbackFunc @copybrief DynExp::TaskBase::CallbackFunc
		*/
		virtual void Restart(DynExp::TaskBase::CallbackType CallbackFunc = nullptr) const;

		/**
		 * @brief Enqueues a task to set the size of the instrument's sample stream.
		 * The default implementation just calls DataStreamBase::SetStreamSize() via a
		 * DataStreamInstrumentTasks::SetStreamSizeTask task.
		 * @param BufferSizeInSamples New stream size in samples
		 * @param CallbackFunc @copybrief DynExp::TaskBase::CallbackFunc
		*/
		virtual void SetStreamSize(size_t BufferSizeInSamples, DynExp::TaskBase::CallbackType CallbackFunc = nullptr) const;

		/**
		 * @brief Enqueues a task to reset the size of the instrument's sample stream
		 * to its default value.
		 * @param CallbackFunc @copybrief DynExp::TaskBase::CallbackFunc
		*/
		virtual void ResetStreamSize(DynExp::TaskBase::CallbackType CallbackFunc = nullptr) const {}

		/**
		 * @brief Determines whether the underlying hardware adapter finished data
		 * acquisition or writing data.
		 * @return Returns Util::OptionalBool::Values::True if the hardware adapter has
		 * completed its action, Util::OptionalBool::Values::False if it is still running,
		 * or Util::OptionalBool::Values::Unknown if its state is not known.
		*/
		virtual Util::OptionalBool HasFinished() const { return Util::OptionalBool::Values::Unknown; }

		/**
		 * @brief Determines whether the underlying hardware adapter is running a data
		 * acquisition or writing data.
		 * @return Returns Util::OptionalBool::Values::True if the hardware adapter is
		 * running an action, Util::OptionalBool::Values::False if it is not running,
		 * or Util::OptionalBool::Values::Unknown if its state is not known.
		*/
		virtual Util::OptionalBool IsRunning() const { return Util::OptionalBool::Values::Unknown; }
		///@}

		/**
		 * @brief Calls @p ReadData(), locks the instrument data, and determines whether
		 * at least one sample can be read from the instrument's data stream.
		 * @param Timeout to wait for locking the mutex of InstrumentBase::InstrumentData.
		 * @return Returns the result of DataStreamBase::CanRead().
		*/
		bool CanRead(const std::chrono::milliseconds Timeout = GetInstrumentDataTimeoutDefault) const;

		/**
		 * @brief Immediately clears the instrument's data stream and then issues a
		 * @p ClearData task by locking the instrument data first, calling DataStreamBase::Clear(),
		 * and finally calling @p ClearData().
		 * @param Timeout to wait for locking the mutex of InstrumentBase::InstrumentData.
		*/
		void Clear(const std::chrono::milliseconds Timeout = GetInstrumentDataTimeoutDefault) const;

	private:
		void ResetImpl(dispatch_tag<InstrumentBase>) override final;
		virtual void ResetImpl(dispatch_tag<DataStreamInstrument>) = 0;		//!< @copydoc ResetImpl(dispatch_tag<DynExp::InstrumentBase>)

		virtual std::unique_ptr<DynExp::InitTaskBase> MakeInitTask() const override { return DynExp::MakeTask<DataStreamInstrumentTasks::InitTask>(); }
		virtual std::unique_ptr<DynExp::ExitTaskBase> MakeExitTask() const override { return DynExp::MakeTask<DataStreamInstrumentTasks::ExitTask>(); }
		virtual std::unique_ptr<DynExp::UpdateTaskBase> MakeUpdateTask() const override { return DynExp::MakeTask<DataStreamInstrumentTasks::UpdateTask>(); }
	};
}