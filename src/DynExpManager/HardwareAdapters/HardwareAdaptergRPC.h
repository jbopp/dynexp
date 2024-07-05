// This file is part of DynExp.

/**
 * @file HardwareAdaptergRPC.h
 * @brief Implementation of a hardware adapter to communicate over
 * TCP sockets using gRPC.
*/

#pragma once

#include "stdafx.h"
#include "HardwareAdapterEthernet.h"

namespace DynExpHardware
{
	template <typename gRPCStub>
	class gRPCHardwareAdapter;

	/**
	 * @brief Defines an exception caused by an operation involving the
	 * gRPC library and communication over a TCP socket.
	*/
	class gRPCException : public Util::Exception
	{
		/**
		 * @brief Message used for the grpc::StatusCode::UNIMPLEMENTED status code.
		*/
		static constexpr auto UNIMPLEMENTED_ErrorMsg = "The connected gRPC server does not implement the called gRPC stub function. Check the server configuration.";

	public:
		/**
		 * @brief Constructs a @p gRPCException instance.
		 * @param Result gRPC status code. Refer to gRPC documentation.
		 * @param Location Origin of the exception. Refer to Util::Exception::Exception().
		*/
		gRPCException(const grpc::Status Result, const std::source_location Location = std::source_location::current()) noexcept;
	};

	/**
	 * @brief Parameter class for @p gRPCHardwareAdapter
	 * @tparam gRPCStub gRPC stub class which is used to transfer data.
	*/
	template <typename gRPCStub>
	class gRPCHardwareAdapterParams : public DynExp::HardwareAdapterParamsBase
	{
	public:
		/**
		 * @brief Constructs the parameters for a @p gRPCHardwareAdapter instance.
		 * @copydetails DynExp::ParamsBase::ParamsBase
		*/
		gRPCHardwareAdapterParams(DynExp::ItemIDType ID, const DynExp::DynExpCore& Core)
			: HardwareAdapterParamsBase(ID, Core), NetworkParams(*this) {}

		virtual ~gRPCHardwareAdapterParams() = default;

		virtual const char* GetParamClassTag() const noexcept override { return "gRPCHardwareAdapterParams"; }

		DynExp::NetworkParamsExtension NetworkParams;									//!< @copydoc DynExp::NetworkParamsExtension

	private:
		void ConfigureParamsImpl(dispatch_tag<HardwareAdapterParamsBase>) override final { ConfigureParamsImpl(dispatch_tag<gRPCHardwareAdapterParams>()); }
		virtual void ConfigureParamsImpl(dispatch_tag<gRPCHardwareAdapterParams>) {}	//!< @copydoc ConfigureParamsImpl(dispatch_tag<HardwareAdapterParamsBase>)
	};

	/**
	 * @brief Configurator class for @p gRPCHardwareAdapter
	*/
	template <typename gRPCStub>
	class gRPCHardwareAdapterConfigurator : public DynExp::HardwareAdapterConfiguratorBase
	{
	public:
		using ObjectType = gRPCHardwareAdapter<gRPCStub>;
		using ParamsType = gRPCHardwareAdapterParams<gRPCStub>;

		gRPCHardwareAdapterConfigurator() = default;
		virtual ~gRPCHardwareAdapterConfigurator() = default;

	private:
		virtual DynExp::ParamsBasePtrType MakeParams(DynExp::ItemIDType ID, const DynExp::DynExpCore& Core) const override { return DynExp::MakeParams<gRPCHardwareAdapterConfigurator>(ID, Core); }
	};

	/**
	 * @brief This template class provides basic functionality to design hardware adapters for
	 * instruments which communicate via gRPC. Derive from this abstract class to design respective
	 * hardware adapters.
	 * @tparam gRPCStub gRPC stub class which is used to transfer data.
	*/
	template <typename gRPCStub>
	class gRPCHardwareAdapter : public DynExp::HardwareAdapterBase
	{
	public:
		using ParamsType = gRPCHardwareAdapterParams<gRPCStub>;											//!< @copydoc DynExp::Object::ParamsType
		using ConfigType = gRPCHardwareAdapterConfigurator<gRPCStub>;									//!< @copydoc DynExp::Object::ConfigType
		using gRPCStubType = gRPCStub;																	//!< gRPCStub gRPC stub class which is used to transfer data.

		constexpr static auto Name() noexcept { return "gRPC Hardware Adapter"; }						//!< @copydoc DynExp::SerialCommunicationHardwareAdapter::Name()
		constexpr static auto Category() noexcept { return "Communication"; }							//!< @copydoc DynExp::HardwareAdapterBase::Category()

		gRPCHardwareAdapter(const std::thread::id OwnerThreadID, DynExp::ParamsBasePtrType&& Params)	//!< @copydoc DynExp::HardwareAdapterBase::HardwareAdapterBase
			: HardwareAdapterBase(OwnerThreadID, std::move(Params)) {}
		virtual ~gRPCHardwareAdapter();

		virtual std::string GetName() const override { return Name(); }
		virtual std::string GetCategory() const override { return Category(); }

	protected:
		/** @name Not thread-safe
		 * These functions must be called by a function calling @p AcquireLock() before.
		*/
		///@{
		/**
		 * @brief Getter for the gRPC stub.
		 * @return Returns the dereferenced #StubPtr.
		 * @throws Util::NotAvailableException is thrown if @p IsOpenedUnsafe() returns false.
		*/
		typename gRPCStub::Stub& GetStubUnsafe() const;

		/**
		 * @brief Checks whether @p Result denotes an error state. If this is the case,
		 * a respective @p gRPCException is constructed from @p Result and thrown.
		 * @param Result gRPC status code. Refer to gRPC documentation.
		 * @param Location Location from which this function is called. Do not pass anything.
		*/
		void CheckError(const grpc::Status Result, const std::source_location Location = std::source_location::current()) const;

		/**
		 * @brief Checks whether the @p gRPCHardwareAdapter is connected to a server.
		 * @return Returns false if #StubPtr is nullptr, true otherwise.
		*/
		bool IsOpenedUnsafe() const noexcept { return StubPtr.get(); }
		///@}

	private:
		void ResetImpl(dispatch_tag<HardwareAdapterBase>) override final;
		virtual void ResetImpl(dispatch_tag<gRPCHardwareAdapter>) {}									//!< @copydoc ResetImpl(dispatch_tag<HardwareAdapterBase>)

		void EnsureReadyStateChild() override final;
		bool IsReadyChild() const override final;
		bool IsConnectedChild() const noexcept override final;

		/** @name Not thread-safe
		 * These functions must be called by a function calling @p AcquireLock() before.
		*/
		///@{
		/**
		 * @brief Establishes the connection to a gRPC server creating a new gRPC stub,
		 * which is stored in #StubPtr.
		 * @todo Implement secure SSL communication.
		*/
		void OpenUnsafe();

		/**
		 * @brief Closes the connection to a gRPC server. #StubPtr becomes nullptr.
		*/
		void CloseUnsafe();
		///@}

		/** @name Override
		 * Override by derived class.
		*/
		///@{
		/**
		 * @brief Override to add additional initialization steps. Gets executed after the gRPC connection has been established.
		*/
		virtual void OpenUnsafeChild() {}

		/**
		 * @brief Override to add additional termination steps. Gets executed before the gRPC connection is disconnected.
		*/
		virtual void CloseUnsafeChild() {}
		///@}

		std::unique_ptr<typename gRPCStub::Stub> StubPtr;												//!< Pointer to the gRPC stub.
	};

	template <typename gRPCStub>
	gRPCHardwareAdapter<gRPCStub>::~gRPCHardwareAdapter()
	{
		// Not locking, since the object is being destroyed. This should be inherently thread-safe.
		CloseUnsafe();
	}

	template <typename gRPCStub>
	typename gRPCStub::Stub& gRPCHardwareAdapter<gRPCStub>::GetStubUnsafe() const
	{
		if (!IsOpenedUnsafe())
			ThrowExceptionUnsafe(std::make_exception_ptr(Util::NotAvailableException("The gRPC stub is not connected yet.", Util::ErrorType::Error)));

		return *StubPtr;
	}

	template <typename gRPCStub>
	void gRPCHardwareAdapter<gRPCStub>::CheckError(const grpc::Status Result, const std::source_location Location) const
	{
		if (Result.ok())
			return;

		// AcquireLock() has already been called by an (in)direct caller of this function.
		ThrowExceptionUnsafe(std::make_exception_ptr(gRPCException(Result, Location)));
	}

	template <typename gRPCStub>
	void gRPCHardwareAdapter<gRPCStub>::ResetImpl(dispatch_tag<HardwareAdapterBase>)
	{
		// auto lock = AcquireLock(); not necessary here, since DynExp ensures that Object::Reset() can only
		// be called if respective object is not in use.

		CloseUnsafe();

		ResetImpl(dispatch_tag<gRPCHardwareAdapter>());
	}

	template <typename gRPCStub>
	void gRPCHardwareAdapter<gRPCStub>::EnsureReadyStateChild()
	{
		auto lock = AcquireLock(HardwareOperationTimeout);

		OpenUnsafe();
	}

	template <typename gRPCStub>
	bool gRPCHardwareAdapter<gRPCStub>::IsReadyChild() const
	{
		auto lock = AcquireLock(HardwareOperationTimeout);

		auto Exception = GetExceptionUnsafe();
		Util::ForwardException(Exception);

		return IsOpenedUnsafe();
	}

	template <typename gRPCStub>
	bool gRPCHardwareAdapter<gRPCStub>::IsConnectedChild() const noexcept
	{
		try
		{
			auto lock = AcquireLock(HardwareOperationTimeout);

			return IsOpenedUnsafe();
		}
		catch (...)
		{
			return false;
		}
	}

	template <typename gRPCStub>
	void gRPCHardwareAdapter<gRPCStub>::OpenUnsafe()
	{
		if (IsOpenedUnsafe())
			return;

		auto DerivedParams = dynamic_Params_cast<gRPCHardwareAdapter>(GetParams());

		// TODO: Offer SSL credentials.
		StubPtr = gRPCStub::NewStub(grpc::CreateChannel(DerivedParams->NetworkParams.MakeAddress(), grpc::InsecureChannelCredentials()));

		OpenUnsafeChild();
	}

	template <typename gRPCStub>
	void gRPCHardwareAdapter<gRPCStub>::CloseUnsafe()
	{
		CloseUnsafeChild();

		StubPtr.reset();
	}
}