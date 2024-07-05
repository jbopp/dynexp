// This file is part of DynExp.

/**
 * @file HardwareAdapter.h
 * @brief Implementation of %DynExp hardware adapter objects.
*/

#pragma once

#include "stdafx.h"
#include "Object.h"

namespace DynExp
{
	class HardwareAdapterBase;
	class SerialCommunicationHardwareAdapter;
	class QSerialCommunicationHardwareAdapter;

	/**
	 * @brief Pointer type to store a hardware adapter (DynExp::HardwareAdapterBase) with
	*/
	using HardwareAdapterPtrType = std::shared_ptr<HardwareAdapterBase>;

	/**
	 * @brief Factory function to generate a configurator for a specific hardware adapter type
	 * @tparam HardwareAdapterT Type of the hardware adapter to generate a configurator for
	 * @return Pointer to the configurator for the specified hardware adapter type
	*/
	template <typename HardwareAdapterT>
	ConfiguratorBasePtrType MakeHardwareAdapterConfig()
	{
		return std::make_shared<typename HardwareAdapterT::ConfigType>();
	}

	/**
	 * @brief Factory function to generate a hardware adapter of a specific type
	 * @tparam HardwareAdapterT Type of the hardware adapter to generate
	 * @param OwnerThreadID ID of the thread owning the hardware adapter
	 * @param Params Reference to the hardware adapter's parameters to take ownership of
	 * @return Pointer to the hardware adapter of the specified type
	*/
	template <typename HardwareAdapterT>
	HardwareAdapterPtrType MakeHardwareAdapter(const std::thread::id OwnerThreadID, ParamsBasePtrType&& Params)
	{
		return std::make_shared<HardwareAdapterT>(OwnerThreadID, std::move(Params));
	}

	/**
	 * @brief Parameter class for @p HardwareAdapterBase
	*/
	class HardwareAdapterParamsBase : public ParamsBase
	{
	public:
		/**
		 * @brief Constructs the parameters for a @p HardwareAdapterBase instance.
		 * @copydetails ParamsBase::ParamsBase
		*/
		HardwareAdapterParamsBase(ItemIDType ID, const DynExpCore& Core) : ParamsBase(ID, Core) {}

		virtual ~HardwareAdapterParamsBase() = 0;

		virtual const char* GetParamClassTag() const noexcept override { return "HardwareAdapterParamsBase"; }

	private:
		void ConfigureParamsImpl(dispatch_tag<ParamsBase>) override final { ConfigureParamsImpl(dispatch_tag<HardwareAdapterParamsBase>()); }
		virtual void ConfigureParamsImpl(dispatch_tag<HardwareAdapterParamsBase>) {}	//!< @copydoc ConfigureParamsImpl(dispatch_tag<ParamsBase>)

		DummyParam Dummy = { *this };													//!< @copydoc ParamsBase::DummyParam
	};

	/**
	 * @brief Configurator class for @p HardwareAdapterBase
	*/
	class HardwareAdapterConfiguratorBase : public ConfiguratorBase
	{
	public:
		using ObjectType = HardwareAdapterBase;
		using ParamsType = HardwareAdapterParamsBase;

		HardwareAdapterConfiguratorBase() = default;
		virtual ~HardwareAdapterConfiguratorBase() = 0;
	};

	/**
	 * @brief Defines the base class for a hardware adapter object. Hardware adapters describe
	 * interfaces/connections to physical hardware.
	*/
	class HardwareAdapterBase : public Util::ILockable, public Object
	{
	public:
		using ParamsType = HardwareAdapterParamsBase;			//!< @copydoc Object::ParamsType
		using ConfigType = HardwareAdapterConfiguratorBase;		//!< @copydoc Object::ConfigType

		/**
		 * @brief Default timeout e.g. used as a default for calls to Object::GetException().
		*/
		static constexpr auto ShortTimeoutDefault = std::chrono::milliseconds(10);

		/**
		 * @brief Default timeout used to lock the mutex provided by the base class Util::ILockable
		 * to synchronize access to the hardware interface in between multiple instrument threads.
		*/
		static constexpr auto HardwareOperationTimeout = std::chrono::milliseconds(100);

		/**
		 * @brief Every derived class has to redefine this function.
		 * @return Returns the category of this hardware adapter type. 
		*/
		constexpr static auto Category() noexcept { return ""; }

		/**
		 * @brief Constructs a hardware adapter instance.
		 * @copydetails DynExp::Object::Object
		*/
		HardwareAdapterBase(const std::thread::id OwnerThreadID, ParamsBasePtrType&& Params)
			: Object(OwnerThreadID, std::move(Params)) {}

		virtual ~HardwareAdapterBase() = 0;

		virtual std::string GetCategory() const override { return Category(); }

		/**
		 * @brief Sets #LastException to nullptr in a thread-safe way.
		*/
		void ResetException() const;

		/**
		 * @brief Determines the connection status of the hardware interface.
		 * @return Returns true when the hardware adapter is connected to the physical
		 * hardware device, false otherwise.
		*/
		bool IsConnected() const noexcept { return IsConnectedChild(); }

	protected:
		/**
		 * @brief Stores @p Exception in #LastException, wraps it in a Util::ForwardedException
		 * and throws the wrapped excetion by calling Util::ForwardException().
		 * @param Exception Exception to store and rethrow
		*/
		void ThrowException(std::exception_ptr Exception) const;

		/** @name Not thread-safe
		 * AcquireLock() has to be called manually before and returned LockType has to be still in scope.
		*/
		///@{
		/**
		 * @copydoc ThrowException()
		*/
		void ThrowExceptionUnsafe(std::exception_ptr Exception) const;

		/**
		 * @brief Stores @p Exception in #LastException.
		 * @param Exception Exception to store
		*/
		void SetExceptionUnsafe(std::exception_ptr Exception) const;

		/**
		 * @brief Getter for #LastException.
		 * @return Returns #LastException.
		*/
		auto GetExceptionUnsafe() const { return LastException; }
		///@}

	private:
		void ResetImpl(dispatch_tag<Object>) override final;
		virtual void ResetImpl(dispatch_tag<HardwareAdapterBase>) = 0;	//!< @copydoc ResetImpl(dispatch_tag<Object>)

		/**
		 * @copydoc Object::EnsureReadyStateChild()
		 * @throws Util::InvalidStateException is thrown if @p GetException() does not return nullptr.
		*/
		void EnsureReadyStateChild([[maybe_unused]] bool IsAutomaticStartup) override final;

		virtual void EnsureReadyStateChild() = 0;						//!< @copybrief DynExp::Object::EnsureReadyState()
		std::exception_ptr GetExceptionChild(const std::chrono::milliseconds Timeout) const override final;

		/** @name Override
		 * Override by derived classes.
		*/
		///@{
		virtual bool IsConnectedChild() const noexcept = 0;				//!< @copydoc IsConnected()
		///@}

		mutable std::exception_ptr LastException;						//!< Stores the most recent exception caused by a hardware operation.
	};

	/**
	 * @brief Defines an exception caused by a serial communication operation of a hardware adapter.
	*/
	class SerialCommunicationException : public Util::Exception
	{
	public:
		/**
		 * @brief Constructs a @p SerialCommunicationException instance.
		 * @param Description Description explaining the cause of the exception in detail for the user.
		 * @param ErrorCode Error code from Util::DynExpErrorCodes::DynExpErrorCodes
		 * @param Location Origin of the exception. Refer to Util::Exception::Exception().
		*/
		SerialCommunicationException(std::string Description, const int ErrorCode,
			const std::source_location Location = std::source_location::current()) noexcept
			: Exception(std::move(Description), Util::ErrorType::Error, ErrorCode, Location)
		{}
	};

	/**
	 * @brief Parameter class for @p SerialCommunicationHardwareAdapter
	*/
	class SerialCommunicationHardwareAdapterParams : public HardwareAdapterParamsBase
	{
	public:
		/**
		 * @brief Possible line endings sent after writing a line and used to
		 * determine the end of a line while reading.
		*/
		enum LineEndingType { None, Zero, LF, CRLF, CR };

		/**
		 * @brief Constructs the parameters for a @p SerialCommunicationHardwareAdapter instance.
		 * @copydetails ParamsBase::ParamsBase
		*/
		SerialCommunicationHardwareAdapterParams(ItemIDType ID, const DynExpCore& Core) : HardwareAdapterParamsBase(ID, Core) {}

		virtual ~SerialCommunicationHardwareAdapterParams() = 0;

		virtual const char* GetParamClassTag() const noexcept override { return "SerialCommunicationHardwareAdapterParams"; }

		/**
		 * @brief Assigns labels to the entries of @p LineEndingType.
		 * @return Mapping between the entries of @p LineEndingType and human-readable descriptions
		*/
		static Util::TextValueListType<LineEndingType> AvlblLineEndingsStrList();

		/**
		 * @brief Parameter storing the line ending type for serial communication.
		*/
		Param<LineEndingType> LineEnding = { *this, AvlblLineEndingsStrList(), "LineEnding", "Line ending",
			"Characters which terminate a single line", true, LineEndingType::LF };

	private:
		void ConfigureParamsImpl(dispatch_tag<HardwareAdapterParamsBase>) override final { ConfigureParamsImpl(dispatch_tag<SerialCommunicationHardwareAdapterParams>()); }
		virtual void ConfigureParamsImpl(dispatch_tag<SerialCommunicationHardwareAdapterParams>) {}		//!< @copydoc ConfigureParamsImpl(dispatch_tag<HardwareAdapterParamsBase>)
	};

	/**
	 * @brief Configurator class for @p SerialCommunicationHardwareAdapter
	*/
	class SerialCommunicationHardwareAdapterConfigurator : public HardwareAdapterConfiguratorBase
	{
	public:
		using ObjectType = SerialCommunicationHardwareAdapter;
		using ParamsType = SerialCommunicationHardwareAdapterParams;

		SerialCommunicationHardwareAdapterConfigurator() = default;
		virtual ~SerialCommunicationHardwareAdapterConfigurator() = 0;
	};

	/**
	 * @brief Defines a hardware adapter for serial communication.
	 * Logical const-ness: see declaration of class DynExp::Object.
	*/
	class SerialCommunicationHardwareAdapter : public HardwareAdapterBase
	{
	public:
		using ParamsType = SerialCommunicationHardwareAdapterParams;				//!< @copydoc Object::ParamsType
		using ConfigType = SerialCommunicationHardwareAdapterConfigurator;			//!< @copydoc Object::ConfigType

		/**
		 * @brief Type denoting the end of a line when piped to @p operator<<(endl)
		*/
		struct endl {};

		/**
		 * @brief Every derived class has to redefine this function.
		 * @return Returns the name of this %DynExp object type.
		*/
		constexpr static auto Name() noexcept { return "< any >"; }
		
		constexpr static auto Category() noexcept { return "Communication"; }		//!< @copydoc HardwareAdapterBase::Category

		/**
		 * @brief Converts SerialCommunicationHardwareAdapterParams::LineEndingType to two characters
		 * being used as the respective line ending.
		 * @param LineEnding Line ending type
		 * @return Array of two char(acters) defining a line ending. If the respective line ending type
		 * consists of only one character, the second character in the returned array is '\0'.
		*/
		constexpr static std::array<char, 2> LineEndingToChar(SerialCommunicationHardwareAdapterParams::LineEndingType LineEnding) noexcept;

		/**
		 * @brief Determines the amount of characters required to express a line ending type
		 * defined by SerialCommunicationHardwareAdapterParams::LineEndingType.
		 * @param LineEnding Line ending type
		 * @return Number of char(acters) the specified line ending consists of
		*/
		constexpr static unsigned int GetLineEndingLength(SerialCommunicationHardwareAdapterParams::LineEndingType LineEnding) noexcept;

		/**
		 * @brief Defines the maximal size of the hardware adapter's (read) buffer.
		 * @return Always returns a buffer length of 100 MB.
		*/
		constexpr static auto GetMaxBufferSize() noexcept { return 104857600; }

		/**
		 * @copydoc HardwareAdapterBase::HardwareAdapterBase
		*/
		SerialCommunicationHardwareAdapter(const std::thread::id OwnerThreadID, ParamsBasePtrType&& Params);

		virtual ~SerialCommunicationHardwareAdapter() = 0;

		virtual std::string GetName() const override { return Name(); }
		virtual std::string GetCategory() const override { return Category(); }

		/**
		 * @brief Getter for #LineEnding. Thread-safe since #LineEnding only changes when Object::Reset()
		 * is called (itself calling @p ResetImpl(dispatch_tag<HardwareAdapterBase>) which calls @p Init()).
		 * #LineEnding is still std::atomic since @p QSerialCommunicationHardwareAdapterWorker running in 
		 * @p DynExpCore's worker thread might read it anytime - even if there is no @p Object instance
		 * which makes use of this hardware adapter instance.
		 * @return Returns #LineEnding.
		*/
		auto GetLineEnding() const { return LineEnding.load(); }

		/**
		 * @brief Calls @p ReadIntoBuffer() before it extracts the first line from the read buffer.
		 * @return First line from the read buffer or an empty string if the read buffer does not
		 * contain a full line.
		*/
		std::string ReadLine() const;

		/**
		 * @brief Calls @p ReadIntoBuffer() before it extracts the entire content from the read buffer.
		 * @return Content of the read buffer
		*/
		std::string ReadAll() const;

		/**
		 * @brief Calls @p ReadLine() for @p NumTries times until a line is received. Makes the calling
		 * thread sleep for @p DelayBetweenTries in between the calls to @p ReadLine().
		 * @param NumTries Number of attempts to read a line from the read buffer
		 * @param DelayBetweenTries Time to wait between subsequent attempts to read a line
		 * @return First line from the read buffer or an empty string if the read buffer did not receive
		 * a full line during the read attempts.
		*/
		std::string WaitForLine(unsigned int NumTries = 10, std::chrono::milliseconds DelayBetweenTries = std::chrono::milliseconds(10)) const;
		
		/**
		 * @brief Clears the content and state flags of the read buffer (#ReadBuffer) as well as
		 * possible internal buffers of the underlying hardware interface.
		*/
		void Clear() const;

		/**
		 * @brief Flushes the underlying hardware interface and calls @p ReadIntoBuffer().
		*/
		void Flush() const;

		/**
		 * @brief Writes the read buffer's content into @p OutStream calling @p ReadAll().
		 * @param OutStream Stream to write the read data to
		 * @return Reference to the @p SerialCommunicationHardwareAdapter instance the function is invoked on
		*/
		const SerialCommunicationHardwareAdapter& operator>>(std::stringstream& OutStream) const;

		/**
		 * @brief Writes a new line calling @p Write_endl().
		 * @return Reference to the @p SerialCommunicationHardwareAdapter instance the function is invoked on
		*/
		const SerialCommunicationHardwareAdapter& operator<<(endl) const;

		/**
		 * @brief Writes the input parameter calling @p Write(). Also writes @p endl at the end automatically.
		 * @param InStream Stream whose contents to write
		 * @return Reference to the @p SerialCommunicationHardwareAdapter instance the function is invoked on
		*/
		const SerialCommunicationHardwareAdapter& operator<<(const std::stringstream& InStream) const;

		/**
		 * @copybrief operator<<(const std::stringstream&) const
		 * @param Text Text to write
		 * @return Reference to the @p SerialCommunicationHardwareAdapter instance the function is invoked on
		*/
		const SerialCommunicationHardwareAdapter& operator<<(const std::string& Text) const;

		/**
		 * @copydoc operator<<(const std::string&) const
		*/
		const SerialCommunicationHardwareAdapter& operator<<(const std::string_view Text) const;

		/**
		 * @copydoc operator<<(const std::string&) const
		*/
		const SerialCommunicationHardwareAdapter& operator<<(const char* Text) const;

		/**
		 * @copybrief operator<<(const std::stringstream&) const
		 * @param Char Single character to write
		 * @return Reference to the @p SerialCommunicationHardwareAdapter instance the function is invoked on
		*/
		const SerialCommunicationHardwareAdapter& operator<<(const char Char) const;

		/**
		 * @copybrief operator<<(const std::stringstream&) const
		 * @tparam T Type of the value to write
		 * @param Value Value to write converting it to a string with Util::ToStr()
		 * @return Reference to the @p SerialCommunicationHardwareAdapter instance the function is invoked on
		*/
		template <typename T>
		auto& operator<<(const T& Value) const
		{
			auto lock = AcquireLock(HardwareOperationTimeout);

			Write(Util::ToStr(Value));
			Write_endl();

			return *this;
		}

	protected:
		/**
		 * @brief This method can be called from derived classes to manually insert data into
		 * the read buffer. This can be especially useful if there is some callback mechanism
		 * which notifies the derived object as soon as data is available. If this mechanism
		 * is used, it is fine to let the overridden @p Read() method just return en empty string.
		 * This function is thread-safe.
		 * @param String String to write to the buffer
		 * @throws Util::InvalidStateException is thrown if fail flags of #ReadBuffer are set
		 * after writing to it.
		*/
		void InsertIntoBuffer(const std::string& String) const;

	private:
		void Init();	//!< Initializes the instance at construction or in case Object::Reset() is called.

		void ResetImpl(dispatch_tag<HardwareAdapterBase>) override final;
		virtual void ResetImpl(dispatch_tag<SerialCommunicationHardwareAdapter>) {}		//!< @copydoc ResetImpl(dispatch_tag<HardwareAdapterBase>)

		/** @name Not thread-safe
		 * AcquireLock() has to be called manually before and returned LockType has to be still in scope.
		*/
		///@{
		/**
		 * @brief Calls @p Read() to retrieve data from the underlying hardware interface. Writes
		 * the retrieved data to the read buffer (#ReadBuffer).
		 * @throws Util::InvalidStateException is thrown if fail flags of #ReadBuffer are set
		 * after writing to it.
		*/
		void ReadIntoBuffer() const;

		/**
		 * @brief Clears the content and state flags of the read buffer (#ReadBuffer)
		*/
		void ClearReadBuffer() const;

		/**
		 * @brief Checks whether the length of the read buffer (#ReadBuffer) exceeds the
		 * maximal allowed length given by @p GetMaxBufferSize().
		 * @return Returns false in case of an overflow, true otherwise.
		*/
		bool CheckOverflow() const;
		///@}

		/** @name Override, not thread-safe
		 * Override by derived classes.
		 * AcquireLock() has to be called manually before and returned LockType has to be still in scope.
		*/
		///@{
		virtual void ClearChild() const {}		//!< Clears internal buffers of the underlying hardware interface.
		virtual void FlushChild() const {}		//!< Flushes the underlying hardware interface.

		/**
		 * @brief Reads a string from the underlying hardware interface.
		 * @return String read from the hardware interface
		*/
		virtual std::string Read() const = 0;

		/**
		 * @brief Writes a string to the underlying hardware interface.
		 * @param String String to write
		*/
		virtual void Write(const std::string& String) const = 0;

		/**
		 * @brief Writes end of the line characters to the underlying hardware interface.
		*/
		virtual void Write_endl() const = 0;
		///@}

		/**
		 * @brief Copy of SerialCommunicationHardwareAdapterParams::LineEnding to avoid
		 * locking the corresponding @p SerialCommunicationHardwareAdapterParams instance every
		 * time data is read or sent. Refer to @p GetLineEnding().
		*/
		std::atomic<SerialCommunicationHardwareAdapterParams::LineEndingType> LineEnding = SerialCommunicationHardwareAdapterParams::LineEndingType::LF;
		
		std::string LineEndingString;			//!< String corresponding to #LineEnding. Refer to @p LineEndingToChar()
		mutable std::stringstream ReadBuffer;	//!< Buffer storing data read from the underlying physical hardware
	};

	constexpr std::array<char, 2> SerialCommunicationHardwareAdapter::LineEndingToChar(SerialCommunicationHardwareAdapterParams::LineEndingType LineEnding) noexcept
	{
		switch (LineEnding)
		{
		case SerialCommunicationHardwareAdapterParams::LineEndingType::LF: return { '\n', '\0' };
		case SerialCommunicationHardwareAdapterParams::LineEndingType::CRLF: return { '\r', '\n' };
		case SerialCommunicationHardwareAdapterParams::LineEndingType::CR: return { '\r', '\0' };
		default: return { '\0', '\0' };
		}
	}

	constexpr unsigned int SerialCommunicationHardwareAdapter::GetLineEndingLength(SerialCommunicationHardwareAdapterParams::LineEndingType LineEnding) noexcept
	{
		if (LineEnding == SerialCommunicationHardwareAdapterParams::LineEndingType::None)
			return 0;
		if (LineEnding == SerialCommunicationHardwareAdapterParams::LineEndingType::CRLF)
			return 2;

		return 1;
	}

	/**
	 * @brief Parameter class for @p QSerialCommunicationHardwareAdapter
	*/
	class QSerialCommunicationHardwareAdapterParams : public SerialCommunicationHardwareAdapterParams
	{
	public:
		/**
		 * @brief Constructs the parameters for a @p QSerialCommunicationHardwareAdapter instance.
		 * @copydetails ParamsBase::ParamsBase
		*/
		QSerialCommunicationHardwareAdapterParams(ItemIDType ID, const DynExpCore& Core) : SerialCommunicationHardwareAdapterParams(ID, Core) {}
		
		virtual ~QSerialCommunicationHardwareAdapterParams() = 0;

		virtual const char* GetParamClassTag() const noexcept override { return "QSerialCommunicationHardwareAdapterParams"; }

	private:
		void ConfigureParamsImpl(dispatch_tag<SerialCommunicationHardwareAdapterParams>) override final { ConfigureParamsImpl(dispatch_tag<QSerialCommunicationHardwareAdapterParams>()); }
		virtual void ConfigureParamsImpl(dispatch_tag<QSerialCommunicationHardwareAdapterParams>) {}	//!< @copydoc ConfigureParamsImpl(dispatch_tag<SerialCommunicationHardwareAdapterParams>)

		DummyParam Dummy = { *this };																	//!< @copydoc DynExp::ParamsBase::DummyParam
	};

	/**
	 * @brief Configurator class for @p QSerialCommunicationHardwareAdapter
	*/
	class QSerialCommunicationHardwareAdapterConfigurator : public SerialCommunicationHardwareAdapterConfigurator
	{
	public:
		using ObjectType = QSerialCommunicationHardwareAdapter;
		using ParamsType = QSerialCommunicationHardwareAdapterParams;

		QSerialCommunicationHardwareAdapterConfigurator() = default;
		virtual ~QSerialCommunicationHardwareAdapterConfigurator() = 0;
	};

	/**
	 * @brief Qt worker which performs actual serial communication hardware
	 * operations in a separate thread in order not to interfer with processor
	 * time used to update the user interface. All functions are inherently
	 * thead-safe since they are only called by the worker thread the respective
	 * instance of this class lives in.
	*/
	class QSerialCommunicationHardwareAdapterWorker : public Util::QWorker
	{
		Q_OBJECT

	public:
		QSerialCommunicationHardwareAdapterWorker() = default;
		virtual ~QSerialCommunicationHardwareAdapterWorker() = 0;

	public slots:
		void Open() { OpenChild(); }								//!< Opens the communication connection.
		void Close() { CloseChild(); }								//!< Closes the communication connection.
		void Reset() { ResetChild(); }								//!< Resets the worker and the communication connection.
		void Clear() { ClearChild(); }								//!< Clears the communication connection's buffers and state.
		void Flush() { FlushChild(); }								//!< Flushes the communication connection's buffers.
		void Read() { ReadChild(); }								//!< Reads from the communication connection's hardware interface.
		void Write(const QString String) { WriteChild(String); }	//!< Writes @p String to the communication connection's hardware interface.
		void Write_endl() { Write_endl_Child(); }					//!< Writes end of the line character(s) to the communication connection's hardware interface.

	protected:
		/**
		 * @brief Calls QSerialCommunicationHardwareAdapter::SetCommunicationChannelOpened
		 * on QWorker::Owner. Does nothing if QWorker::Owner is nullptr.
		*/
		void SetCommunicationChannelOpened() const noexcept;

		/**
		 * @brief Calls QSerialCommunicationHardwareAdapter::SetCommunicationChannelClosed
		 * on QWorker::Owner. Does nothing if QWorker::Owner is nullptr.
		*/
		void SetCommunicationChannelClosed() const noexcept;

		/**
		 * @brief Calls QSerialCommunicationHardwareAdapter::DataRead
		 * on QWorker::Owner. Does nothing if QWorker::Owner is nullptr.
		*/
		void DataRead(const std::string& String) const;

		/**
		 * @brief Calls QSerialCommunicationHardwareAdapter::SetException
		 * on QWorker::Owner. Does nothing if QWorker::Owner is nullptr.
		 * @tparam ExceptionType Type of the exception to be stored
		 * @param Exception Exception to be stores
		*/
		template <typename ExceptionType>
		void SetException(const ExceptionType& Exception) const;

	private:
		virtual void OpenChild() = 0;								//!< @copydoc Open()
		virtual void CloseChild() = 0;								//!< @copydoc Close()
		virtual void ResetChild() = 0;								//!< @copydoc Reset()
		virtual void ClearChild() = 0;								//!< @copydoc Clear()
		virtual void FlushChild() = 0;								//!< @copydoc Flush()
		virtual void ReadChild() = 0;								//!< @copydoc Read()
		virtual void WriteChild(const QString& String) = 0;			//!< @copydoc Write()
		virtual void Write_endl_Child() = 0;						//!< @copydoc Write_endl()
	};

	/**
	 * @brief @p SerialCommunicationHardwareAdapter is based on a Qt communication object (wrapped by
	 * @p QSerialCommunicationHardwareAdapterWorker) which cannot be shared between different threads.
	 * Instead, it lives in a @p DynExpCore's worker thread. @p QSerialCommunicationHardwareAdapter
	 * just pushes messages to it and allows the @p QSerialCommunicationHardwareAdapterWorker instance
	 * to communicate its state and data to the corresponding @p QSerialCommunicationHardwareAdapter
	 * instance.
	*/
	class QSerialCommunicationHardwareAdapter : public QObject, public SerialCommunicationHardwareAdapter
	{
		Q_OBJECT

	private:
		/**
		 * @brief Allow exclusive access to some of @p QSerialCommunicationHardwareAdapter's private
		 * methods to QSerialCommunicationHardwareAdapterWorkerOnlyType.
		*/
		class QSerialCommunicationHardwareAdapterWorkerOnlyType
		{
			friend class QSerialCommunicationHardwareAdapter;
			friend class QSerialCommunicationHardwareAdapterWorker;

			/**
			 * @brief Construcs an instance - one for each @p QSerialCommunicationHardwareAdapter instance
			 * @param Parent Owning @p QSerialCommunicationHardwareAdapter instance
			*/
			constexpr QSerialCommunicationHardwareAdapterWorkerOnlyType(QSerialCommunicationHardwareAdapter& Parent) noexcept : Parent(Parent) {}

			void SetCommunicationChannelOpened() const noexcept { Parent.SetCommunicationChannelOpened(); }		//!< @copydoc QSerialCommunicationHardwareAdapter::SetCommunicationChannelOpened
			void SetCommunicationChannelClosed() const noexcept { Parent.SetCommunicationChannelClosed(); }		//!< @copydoc QSerialCommunicationHardwareAdapter::SetCommunicationChannelClosed
			void DataRead(const std::string& String) const { Parent.DataRead(String); }							//!< @copydoc QSerialCommunicationHardwareAdapter::DataRead

			/**
			 * @copydoc QSerialCommunicationHardwareAdapter::SetException
			*/
			template <typename ExceptionType>
			void SetException(const ExceptionType& Exception) const { Parent.SetException(Exception); }

			QSerialCommunicationHardwareAdapter& Parent;			//!< Owning @p QSerialCommunicationHardwareAdapter instance
		};

	public:
		using ParamsType = QSerialCommunicationHardwareAdapterParams;											//!< @copydoc Object::ParamsType
		using ConfigType = QSerialCommunicationHardwareAdapterConfigurator;										//!< @copydoc Object::ConfigType
		using QWorkerPtrType = std::unique_ptr<QSerialCommunicationHardwareAdapterWorker>;						//!< Pointer-type owning the related @p QSerialCommunicationHardwareAdapterWorker instance

		QSerialCommunicationHardwareAdapter(const std::thread::id OwnerThreadID, ParamsBasePtrType&& Params);	//!< @copydoc SerialCommunicationHardwareAdapter::SerialCommunicationHardwareAdapter
		virtual ~QSerialCommunicationHardwareAdapter() = 0;

		QSerialCommunicationHardwareAdapterWorkerOnlyType QSerialCommunicationHardwareAdapterWorkerOnly;		//!< @copydoc QSerialCommunicationHardwareAdapterWorkerOnlyType

	private:
		/** @name Override
		 * Override by derived class.
		*/
		///@{
		/**
		 * @brief Abstract factory function for a worker instance derived from
		 * @p QSerialCommunicationHardwareAdapterWorker. Also connects Qt signals to the
		 * worker instance's slots.
		 * @return Pointer to a new instance of a worker this hardware adapter takes
		 * ownership of
		*/
		virtual QWorkerPtrType MakeWorker() = 0;

		/**
		 * @brief Tells the worker instance to perform initialization steps, e.g. by
		 * emitting a Qt signal which is received by the worker's slots.
		*/
		virtual void InitWorker() {}
		///@}

		void ResetImpl(dispatch_tag<SerialCommunicationHardwareAdapter>) override final;
		virtual void ResetImpl(dispatch_tag<QSerialCommunicationHardwareAdapter>) {}							//!< @copydoc ResetImpl(dispatch_tag<SerialCommunicationHardwareAdapter>)

		/**
		 * @copydoc HardwareAdapterBase::EnsureReadyStateChild()
		 * @throws Util::InvalidArgException is thrown if #Worker is nullptr.
		*/
		void EnsureReadyStateChild() override final;

		bool IsReadyChild() const override final;
		bool IsConnectedChild() const noexcept override final;

		/** @name Thread-safe public functions
		 * Thread-safe since mutex is locked. To be called from @p QSerialCommunicationHardwareAdapterWorker.
		 * Logical const-ness: @p QSerialCommunicationHardwareAdapterWorker can only access const members.
		*/
		///@{
		void SetCommunicationChannelOpened() const noexcept { CommunicationChannelOpened = true; }				//!< Sets #CommunicationChannelOpened to true.
		void SetCommunicationChannelClosed() const noexcept { CommunicationChannelOpened = false; }				//!< Sets #CommunicationChannelOpened to false.

		/**
		 * @brief Passes data to the hardware adapter which has been received by the worker instance.
		 * @param String String read by the worker instance
		*/
		void DataRead(const std::string& String) const;

		/**
		 * @brief Sets @p Exception as #PendingException and stores it by calling
		 * HardwareAdapterBase::SetExceptionUnsafe().
		 * @tparam ExceptionType Type of the exception to be stored
		 * @param Exception Exception to be stores
		*/
		template <typename ExceptionType>
		void SetException(const ExceptionType& Exception) const noexcept
		{
			try
			{
				auto lock = AcquireLock(HardwareOperationTimeout);

				auto ExceptionPtr = std::make_exception_ptr(Exception);
				SetExceptionUnsafe(ExceptionPtr);
				PendingException = ExceptionPtr;
			}
			catch (...)
			{
				// Swallow exception if an error occurs setting it. This is not optimal but there
				// is no other way since exceptions are not allowed to leave Qt's slots.
			}
		}
		///@}

		/** @name Not thread-safe
		 * These functions must be called by a function calling @p AcquireLock() before.
		*/
		///@{
		void ClearChild() const override final;
		void FlushChild() const override final;
		std::string Read() const override final;
		void Write(const std::string& String) const override final;
		void Write_endl() const override final;

		/**
		 * @brief If #PendingException is not nullptr, sets it to nullptr and throws the
		 * exception calling Util::ForwardException(). Called by overridden methods which
		 * perform hardware operations to ensure that the hardware adapter handles
		 * exceptions which have occurred in its worker instance.
		*/
		void ThrowPendingException() const;
		///@}

	signals:
		/** @name Qt signals
		 * Signals emitted by this hardware adapter. The worker instance's slots
		 * are connected to them to handles the signals.
		*/
		///@{
		void OpenSig();												//!< @copydoc QSerialCommunicationHardwareAdapterWorker::Open()
		void CloseSig();											//!< @copydoc QSerialCommunicationHardwareAdapterWorker::Close()
		void ResetSig();											//!< @copydoc QSerialCommunicationHardwareAdapterWorker::Reset()
		void ClearSig() const;										//!< @copydoc QSerialCommunicationHardwareAdapterWorker::Clear()
		void FlushSig() const;										//!< @copydoc QSerialCommunicationHardwareAdapterWorker::Flush()
		void ReadSig() const;										//!< @copydoc QSerialCommunicationHardwareAdapterWorker::Read()
		void WriteSig(const QString String) const;					//!< @copydoc QSerialCommunicationHardwareAdapterWorker::Write()
		void Write_endl_Sig() const;								//!< @copydoc QSerialCommunicationHardwareAdapterWorker::Write_endl()
		///@}

	private:
		QSerialCommunicationHardwareAdapterWorker* Worker;			//!< Worker instance running in its own thread to handle the actual communication there.
		mutable std::atomic<bool> CommunicationChannelOpened;		//!< Indicates whether the worker instance has opened the communication channel.
		mutable std::exception_ptr PendingException;				//!< Stores exceptions which have occurred in the worker instance thread.
	};

	// Needs to be defined after declaration of class QSerialCommunicationHardwareAdapter.
	template <typename ExceptionType>
	void QSerialCommunicationHardwareAdapterWorker::SetException(const ExceptionType& Exception) const
	{
		auto Owner = std::dynamic_pointer_cast<const QSerialCommunicationHardwareAdapter>(GetOwner());
		if (!Owner)
			return;

		Owner->QSerialCommunicationHardwareAdapterWorkerOnly.SetException(Exception);
	}
}

/**
 * @brief %DynExp's hardware namespace contains the implementation of %DynExp hardware adapters
 * which extend %DynExp's core functionality in a modular way.
*/
namespace DynExpHardware {};