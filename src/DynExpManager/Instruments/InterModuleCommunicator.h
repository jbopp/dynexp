// This file is part of DynExp.

/**
 * @file InterModuleCommunicator.h
 * @brief Implementation of the inter-module communicator instrument to exchange events in
 * between %DynExp modules.
*/

#pragma once

#include "stdafx.h"
#include "DynExpCore.h"

namespace DynExpInstr
{
	class InterModuleCommunicator;

	/**
	 * @brief Tasks for @p InterModuleCommunicator
	*/
	namespace InterModuleCommunicatorTasks
	{
		/**
		 * @copydoc DynExp::InitTaskBase
		*/
		class InitTask : public DynExp::InitTaskBase
		{
			void InitFuncImpl(dispatch_tag<DynExp::InitTaskBase>, DynExp::InstrumentInstance& Instance) override final;

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
			void ExitFuncImpl(dispatch_tag<DynExp::ExitTaskBase>, DynExp::InstrumentInstance& Instance) override final;

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
			void UpdateFuncImpl(dispatch_tag<DynExp::UpdateTaskBase>, DynExp::InstrumentInstance& Instance) override final;

			/**
			 * @copydoc UpdateFuncImpl(dispatch_tag<DynExp::UpdateTaskBase>, DynExp::InstrumentInstance&)
			*/
			virtual void UpdateFuncImpl(dispatch_tag<UpdateTask>, DynExp::InstrumentInstance& Instance) {}
		};
	}

	/**
	 * @brief Data class for @p InterModuleCommunicator
	*/
	class InterModuleCommunicatorData : public DynExp::InstrumentDataBase
	{
	public:
		InterModuleCommunicatorData() = default;
		virtual ~InterModuleCommunicatorData() = default;

	private:
		void ResetImpl(dispatch_tag<DynExp::InstrumentDataBase>) override final;
		virtual void ResetImpl(dispatch_tag<InterModuleCommunicatorData>) {};				//!< @copydoc ResetImpl(dispatch_tag<DynExp::InstrumentDataBase>)
	};

	/**
	 * @brief Parameter class for @p InterModuleCommunicator.
	*/
	class InterModuleCommunicatorParams : public DynExp::InstrumentParamsBase
	{
	public:
		/**
		 * @brief Constructs the parameters for a @p InterModuleCommunicator instance.
		 * @copydetails DynExp::ParamsBase::ParamsBase
		*/
		InterModuleCommunicatorParams(DynExp::ItemIDType ID, const DynExp::DynExpCore& Core) : DynExp::InstrumentParamsBase(ID, Core) {}

		virtual ~InterModuleCommunicatorParams() = default;

		virtual const char* GetParamClassTag() const noexcept override { return "InterModuleCommunicatorParams"; }

	private:
		void ConfigureParamsImpl(dispatch_tag<DynExp::InstrumentParamsBase>) override final { ConfigureParamsImpl(dispatch_tag<InterModuleCommunicatorParams>()); }
		virtual void ConfigureParamsImpl(dispatch_tag<InterModuleCommunicatorParams>) {}	//!< @copydoc ConfigureParamsImpl(dispatch_tag<DynExp::InstrumentParamsBase>)

		DummyParam Dummy = { *this };														//!< @copydoc DynExp::ParamsBase::DummyParam
	};

	/**
	 * @brief Configurator class for @p InterModuleCommunicator
	*/
	class InterModuleCommunicatorConfigurator : public DynExp::InstrumentConfiguratorBase
	{
	public:
		using ObjectType = InterModuleCommunicator;
		using ParamsType = InterModuleCommunicatorParams;

		InterModuleCommunicatorConfigurator() = default;
		virtual ~InterModuleCommunicatorConfigurator() = default;

	private:
		virtual DynExp::ParamsBasePtrType MakeParams(DynExp::ItemIDType ID, const DynExp::DynExpCore& Core) const override { return DynExp::MakeParams<InterModuleCommunicatorConfigurator>(ID, Core); }
	};

	/**
	 * @brief Defines an inter-module communicator instrument to exchange events in
	 * between %DynExp modules. Modules can make use of an instance of this instrument.
	 * If any of the modules using this instance post an event, the event
	 * becomes enqueued in the event queues of all other modules also using this
	 * @p InterModuleCommunicator instance.
	*/
	class InterModuleCommunicator : public DynExp::InstrumentBase
	{
	public:
		using ParamsType = InterModuleCommunicatorParams;								//!< @copydoc DynExp::Object::ParamsType
		using ConfigType = InterModuleCommunicatorConfigurator;							//!< @copydoc DynExp::Object::ConfigType
		using InstrumentDataType = InterModuleCommunicatorData;							//!< @copydoc DynExp::InstrumentBase::InstrumentDataType

		constexpr static auto Name() noexcept { return "Inter-Module Communicator"; }	//!< @copydoc DynExp::SerialCommunicationHardwareAdapter::Name

		/**
		 * @copydoc DynExp::InstrumentBase::InstrumentBase
		*/
		InterModuleCommunicator(const std::thread::id OwnerThreadID, DynExp::ParamsBasePtrType&& Params);

		virtual ~InterModuleCommunicator() {}

		virtual std::string GetName() const override { return Name(); }

		/**
		 * @brief Inserts the event passed to the function into the event queues of the modules making use of
		 * this @p InterModuleCommunicator instance if they are in a ready state (i.e. DynExp::Object::IsReady()
		 * returns true). The event is not inserted into the @p Caller's event queue.
		 * @tparam DerivedEvent Type of the event to post (derived from DynExp::InterModuleEvent).
		 * @param Caller Reference to the module calling this function.
		 * @param InterModuleEvent Event to post to the modules using this @p InterModuleCommunicator instance.
		*/
		template <typename DerivedEvent,
			std::enable_if_t<std::is_base_of_v<DynExp::InterModuleEventBase, DerivedEvent>, int> = 0>
		void PostEvent(const DynExp::ModuleBase& Caller, const DerivedEvent& InterModuleEvent) const
		{
			auto& ModuleMgr = Core.GetModuleManager();
			const auto UserIDs = GetUserIDs();

			for (auto ID : UserIDs)
			{
				// Do not send the event back to the caller.
				if (Caller.GetID() == ID)
					continue;

				auto Resource = ModuleMgr.GetResource(ID);

				// Retry in case of Util::TimeoutException a couple of times to not lose events.
				for (int NumTries = 0; true; NumTries++)
				{
					try
					{
						if (Resource->IsReady())
							Resource->EnqueueEvent(std::make_unique<DerivedEvent>(InterModuleEvent));
					}
					catch ([[maybe_unused]] const Util::TimeoutException& e)
					{
						if (NumTries >= 100)
							throw;
						else
						{
							std::this_thread::yield();

							continue;
						}
					}

					break;
				}
			}
		}

	private:
		void ResetImpl(dispatch_tag<DynExp::InstrumentBase>) override final;
		virtual void ResetImpl(dispatch_tag<InterModuleCommunicator>) {}				//!< @copydoc ResetImpl(dispatch_tag<DynExp::InstrumentBase>)

		// Do not override MakeInitTask(), MakeExitTask(), and MakeUpdateTask() since respective tasks don't do anything.

		const DynExp::DynExpCore& Core;													//!< Reference to %DynExp's core
	};
}