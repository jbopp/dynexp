// This file is part of DynExp.

#include "stdafx.h"
#include "moc_EventSender.cpp"
#include "EventSender.h"

namespace DynExpModule
{
	EventSenderWidget::EventSenderWidget(EventSender& Owner, QModuleWidget* parent)
		: QModuleWidget(Owner, parent), EventID(0)
	{
		ui.setupUi(this);
	}

	void EventSenderWidget::OnEventDoubleClicked(QListWidgetItem* Item)
	{
		EventID = Item->data(Qt::UserRole).value<size_t>();
	}

	void EventSenderData::ResetImpl(dispatch_tag<QModuleDataBase>)
	{
		Init();
	}

	void EventSenderData::Init()
	{
		UIInitialized = false;
		EventID = 0;
	}

	Util::DynExpErrorCodes::DynExpErrorCodes EventSender::ModuleMainLoop(DynExp::ModuleInstance& Instance)
	{
		auto ModuleData = DynExp::dynamic_ModuleData_cast<EventSender>(Instance.ModuleDataGetter());

		if (ModuleData->EventID)
		{
			auto Event = DynExp::InterModuleEventLibrary::Get().GetEvents().at(ModuleData->EventID)();
			ModuleData->Communicator->PostEvent(*this, *Event);

			ModuleData->EventID = 0;
		}

		return Util::DynExpErrorCodes::NoError;
	}

	void EventSender::ResetImpl(dispatch_tag<QModuleBase>)
	{
	}

	std::unique_ptr<DynExp::QModuleWidget> EventSender::MakeUIWidget()
	{
		auto Widget = std::make_unique<EventSenderWidget>(*this);

		return Widget;
	}

	void EventSender::UpdateUIChild(const ModuleBase::ModuleDataGetterType& ModuleDataGetter)
	{
		auto Widget = GetWidget<EventSenderWidget>();
		auto ModuleData = DynExp::dynamic_ModuleData_cast<EventSender>(ModuleDataGetter());

		if (!ModuleData->UIInitialized)
		{
			auto& Events = DynExp::InterModuleEventLibrary::Get().GetEvents();

			for (auto& Event : Events)
			{
				const auto E = Event.second();

				auto ListItem = new QListWidgetItem(QString::fromStdString(E->GetName()), Widget->ui.LWEvents);
				ListItem->setToolTip("ID " + QString::number(Event.first));
				ListItem->setData(Qt::UserRole, QVariant::fromValue(Event.first));
			}

			ModuleData->UIInitialized = true;
		}

		if (Widget->EventID)
		{
			ModuleData->EventID = Widget->EventID;
			Widget->EventID = 0;

			ModuleData->RunQueue();
		}
	}

	void EventSender::OnInit(DynExp::ModuleInstance* Instance) const
	{
		auto ModuleParams = DynExp::dynamic_Params_cast<EventSender>(Instance->ParamsGetter());
		auto ModuleData = DynExp::dynamic_ModuleData_cast<EventSender>(Instance->ModuleDataGetter());

		Instance->LockObject(ModuleParams->Communicator, ModuleData->Communicator);
	}

	void EventSender::OnExit(DynExp::ModuleInstance* Instance) const
	{
		auto ModuleData = DynExp::dynamic_ModuleData_cast<EventSender>(Instance->ModuleDataGetter());

		Instance->UnlockObject(ModuleData->Communicator);
	}
}