// This file is part of DynExp.

#include "stdafx.h"
#include "moc_Trajectory1D.cpp"
#include "Trajectory1D.h"

namespace DynExpModule
{
	Trajectory1DWidget::Trajectory1DWidget(Trajectory1D& Owner, QModuleWidget* parent) : QModuleWidget(Owner, parent)
	{
		ui.setupUi(this);
	}

	void Trajectory1DData::ResetImpl(dispatch_tag<QModuleDataBase>)
	{
		Init();
	}

	void Trajectory1DData::Init()
	{
		TriggerMode = TriggerModeType::Manual;
		RepeatCount = 1;
		DwellTime = std::chrono::milliseconds(100);

		LastWrittenSampleID = 0;
		Samples.clear();

		CurrentPlaybackPos = 0;
		CurrentRepeatCount = 0;
		Ready = false;
		TrajectoryStartedTime = {};
	}

	Util::TextValueListType<Trajectory1DData::TriggerModeType> Trajectory1DParams::TriggerModeTypeStrList()
	{
		Util::TextValueListType<Trajectory1DData::TriggerModeType> List = {
			{ "Trigger disabled (start immediately)", Trajectory1DData::TriggerModeType::Continuous },
			{ "Trigger only manually and stop after playback", Trajectory1DData::TriggerModeType::ManualOnce },
			{ "Trigger only manually each time a trigger event occurs", Trajectory1DData::TriggerModeType::Manual },
			{ "Trigger when the trajectory data stream changes", Trajectory1DData::TriggerModeType::OnStreamChanged }
		};

		return List;
	}

	Util::DynExpErrorCodes::DynExpErrorCodes Trajectory1D::ModuleMainLoop(DynExp::ModuleInstance& Instance)
	{
		try
		{
			auto ModuleData = DynExp::dynamic_ModuleData_cast<Trajectory1D>(Instance.ModuleDataGetter());

			UpdateStream(ModuleData);
			Move(ModuleData);

			NumFailedUpdateAttempts = 0;
		} // ModuleData and instruments' data unlocked here.
		catch (const Util::TimeoutException& e)
		{
			if (NumFailedUpdateAttempts++ >= 3)
				Instance.GetOwner().SetWarning(e);
		}

		return Util::DynExpErrorCodes::NoError;
	}

	void Trajectory1D::ResetImpl(dispatch_tag<QModuleBase>)
	{
		NumFailedUpdateAttempts = 0;
	}

	std::unique_ptr<DynExp::QModuleWidget> Trajectory1D::MakeUIWidget()
	{
		auto Widget = std::make_unique<Trajectory1DWidget>(*this);

		Connect(Widget->GetUI().BStart, &QPushButton::clicked, this, &Trajectory1D::OnStartClicked);
		Connect(Widget->GetUI().BStop, &QPushButton::clicked, this, &Trajectory1D::OnStopClicked);
		Connect(Widget->GetUI().BForce, &QPushButton::clicked, this, &Trajectory1D::OnTriggerClicked);

		return Widget;
	}

	void Trajectory1D::UpdateUIChild(const ModuleBase::ModuleDataGetterType& ModuleDataGetter)
	{
		auto Widget = GetWidget<Trajectory1DWidget>();
		auto ModuleData = DynExp::dynamic_ModuleData_cast<Trajectory1D>(ModuleDataGetter());

		Widget->GetUI().LESampleCount->setText(QString::number(ModuleData->GetSamples().size()));

		Widget->GetUI().LProgress->setVisible(ModuleData->IsReady() && ModuleData->GetSamples().size());
		Widget->GetUI().PBProgress->setVisible(ModuleData->IsReady() && ModuleData->GetSamples().size());
		Widget->GetUI().LRepetition->setVisible(ModuleData->IsReady() && ModuleData->GetSamples().size());
		if (ModuleData->GetSamples().size())
			Widget->GetUI().PBProgress->setValue(static_cast<int>((static_cast<double>(ModuleData->GetCurrentRepeatCount()) / ModuleData->GetRepeatCount() +
				std::max(0.0, static_cast<double>(ModuleData->GetCurrentPlaybackPos()) - 1.0)
				/ ModuleData->GetRepeatCount() / ModuleData->GetSamples().size()) * 100.0));
		Widget->GetUI().LRepetition->setText(QString("Repetition ") + QString::number(ModuleData->GetCurrentRepeatCount() + 1)
			+ " / " + QString::number(ModuleData->GetRepeatCount()));

		Widget->GetUI().BStart->setEnabled(!ModuleData->IsReady());
		Widget->GetUI().BStop->setEnabled(ModuleData->IsReady());
		Widget->GetUI().BForce->setEnabled(ModuleData->IsReady());
	}

	void Trajectory1D::UpdateStream(Util::SynchronizedPointer<ModuleDataType>& ModuleData)
	{
		auto TrajectoryDataInstrData = DynExp::dynamic_InstrumentData_cast<DynExpInstr::DataStreamInstrument>(ModuleData->GetTrajectoryDataInstr()->GetInstrumentData());
		auto SampleStream = TrajectoryDataInstrData->GetSampleStream();

		if (SampleStream->GetNumSamplesWritten() != ModuleData->GetLastWrittenSampleID())
		{
			if (SampleStream->GetNumSamplesWritten() < ModuleData->GetLastWrittenSampleID())
				ModuleData->SetLastWrittenSampleID(0);	// e.g. if SampleStream has been cleared.

			SampleStream->SeekBeg(std::ios_base::in);
			ModuleData->SetSamples(SampleStream->ReadBasicSamples(SampleStream->GetStreamSizeRead()));
			ModuleData->SetLastWrittenSampleID(SampleStream->GetNumSamplesWritten());

			if (ModuleData->GetSamples().empty())
				return;

			if (!SampleStream->IsBasicSampleTimeUsed())
				for (size_t i = 0; i < ModuleData->GetSamples().size(); ++i)
					ModuleData->GetSamples()[i].Time = static_cast<DynExpInstr::BasicSample::DataType>(ModuleData->GetDwellTime().count())
						* i / decltype(ModuleData->GetDwellTime())::period::den;

			switch (ModuleData->GetTriggerMode())
			{
			case Trajectory1DData::TriggerModeType::Continuous: [[fallthrough]];
			case Trajectory1DData::TriggerModeType::OnStreamChanged:
				Trigger(ModuleData);
				break;
			case Trajectory1DData::TriggerModeType::Manual:
				ModuleData->SetCurrentPlaybackPos(0);
				ModuleData->SetCurrentRepeatCount(0);
				break;
			default:
				Stop(ModuleData);
			}
		}
	}

	void Trajectory1D::Move(Util::SynchronizedPointer<ModuleDataType>& ModuleData)
	{
		if (!ModuleData->IsReady() || !ModuleData->GetCurrentPlaybackPos())
			return;

		std::chrono::system_clock::duration TimeEllapsed;
		if (ModuleData->GetCurrentPlaybackPos() == 1)
		{
			ModuleData->SetTrajectoryStartedTime(std::chrono::system_clock::now());
			TimeEllapsed = std::chrono::milliseconds(0);
		}
		else
			TimeEllapsed = std::chrono::system_clock::now() - ModuleData->GetTrajectoryStartedTime();

		const auto& Samples = ModuleData->GetSamples();
		for (size_t i = ModuleData->GetCurrentPlaybackPos() - 1; i < Samples.size(); ++i)
		{
			auto ThisSampleStart = std::chrono::milliseconds(Util::NumToT<std::chrono::milliseconds::rep>(Samples[i].Time * std::chrono::milliseconds::period::den));
			auto NextSampleStart = i + 1 >= Samples.size() ? std::chrono::milliseconds(0)
				: std::chrono::milliseconds(Util::NumToT<std::chrono::milliseconds::rep>(Samples[i + 1].Time * std::chrono::milliseconds::period::den));

			// Move to sample i if time for next, but not for second-next sample has ellapsed or if time for next sample has ellapsed and end of sample list.
			if ((ThisSampleStart <= TimeEllapsed && NextSampleStart > TimeEllapsed) ||
				(ThisSampleStart <= TimeEllapsed && i + 1 == Samples.size()))
			{
				ModuleData->GetPositionerStage()->MoveAbsolute(Samples[i].Value);
				ModuleData->SetCurrentPlaybackPos(i + 2);

				break;
			}
		}

		// End reached? If applicable, retrigger.
		if (ModuleData->GetCurrentPlaybackPos() > ModuleData->GetSamples().size())
		{
			ModuleData->SetCurrentRepeatCount(ModuleData->GetCurrentRepeatCount() + 1);
			if (ModuleData->GetCurrentRepeatCount() < ModuleData->GetRepeatCount())
				ModuleData->SetCurrentPlaybackPos(1);
			else
			{
				switch (ModuleData->GetTriggerMode())
				{
				case Trajectory1DData::TriggerModeType::Continuous:
					Trigger(ModuleData);
					break;
				case Trajectory1DData::TriggerModeType::Manual: [[fallthrough]];
				case Trajectory1DData::TriggerModeType::OnStreamChanged:
					ModuleData->SetCurrentPlaybackPos(0);
					ModuleData->SetCurrentRepeatCount(0);
					break;
				default:
					ModuleData->SetReady(false);	// Do not call Stop() to let the positioner complete its motion.
				}
			}
		}
	}

	void Trajectory1D::Start(Util::SynchronizedPointer<ModuleDataType>& ModuleData) const
	{
		if (ModuleData->GetSamples().size())
		{
			ModuleData->SetReady(true);
			ModuleData->SetCurrentPlaybackPos(0);
			ModuleData->SetCurrentRepeatCount(0);

			if (ModuleData->GetTriggerMode() == Trajectory1DData::TriggerModeType::Continuous)
				Trigger(ModuleData);
		}
	}

	void Trajectory1D::Stop(Util::SynchronizedPointer<ModuleDataType>& ModuleData) const
	{
		ModuleData->GetPositionerStage()->StopMotion();
		ModuleData->SetReady(false);
	}

	void Trajectory1D::Trigger(Util::SynchronizedPointer<ModuleDataType>& ModuleData) const
	{
		if (ModuleData->IsReady() && ModuleData->GetSamples().size() &&
			(ModuleData->GetTriggerMode() != Trajectory1DData::TriggerModeType::ManualOnce ||
				(!ModuleData->GetCurrentPlaybackPos() && !ModuleData->GetCurrentRepeatCount())))
		{
			ModuleData->SetCurrentPlaybackPos(1);
			ModuleData->SetCurrentRepeatCount(0);
		}
	}

	void Trajectory1D::OnInit(DynExp::ModuleInstance* Instance) const
	{
		StartEvent::Register(*this, &Trajectory1D::OnStart);
		StopEvent::Register(*this, &Trajectory1D::OnStop);
		TriggerEvent::Register(*this, &Trajectory1D::OnTrigger);

		auto ModuleParams = DynExp::dynamic_Params_cast<Trajectory1D>(Instance->ParamsGetter());
		auto ModuleData = DynExp::dynamic_ModuleData_cast<Trajectory1D>(Instance->ModuleDataGetter());

		Instance->LockObject(ModuleParams->TrajectoryDataInstr, ModuleData->GetTrajectoryDataInstr());
		Instance->LockObject(ModuleParams->PositionerStage, ModuleData->GetPositionerStage());
		if (ModuleParams->Communicator.ContainsID())
			Instance->LockObject(ModuleParams->Communicator, ModuleData->GetCommunicator());

		ModuleData->SetTriggerMode(ModuleParams->TriggerMode);
		ModuleData->SetRepeatCount(std::max(static_cast<size_t>(1), Util::NumToT<size_t>(ModuleParams->RepeatCount)));
		ModuleData->SetDwellTime(std::chrono::milliseconds(Util::NumToT<std::chrono::milliseconds::rep>(std::max(1.0, ModuleParams->DwellTime.Get()))));

		ModuleData->SetReady(ModuleData->GetTriggerMode() != Trajectory1DData::TriggerModeType::ManualOnce);
	}

	void Trajectory1D::OnExit(DynExp::ModuleInstance* Instance) const
	{
		auto ModuleData = DynExp::dynamic_ModuleData_cast<Trajectory1D>(Instance->ModuleDataGetter());

		Stop(ModuleData);

		Instance->UnlockObject(ModuleData->GetTrajectoryDataInstr());
		Instance->UnlockObject(ModuleData->GetPositionerStage());
		Instance->UnlockObject(ModuleData->GetCommunicator());

		StartEvent::Deregister(*this);
		StopEvent::Deregister(*this);
		TriggerEvent::Deregister(*this);
	}

	void Trajectory1D::OnStart(DynExp::ModuleInstance* Instance) const
	{
		auto ModuleData = DynExp::dynamic_ModuleData_cast<Trajectory1D>(Instance->ModuleDataGetter());

		Start(ModuleData);
	}

	void Trajectory1D::OnStop(DynExp::ModuleInstance* Instance) const
	{
		auto ModuleData = DynExp::dynamic_ModuleData_cast<Trajectory1D>(Instance->ModuleDataGetter());

		Stop(ModuleData);
	}

	void Trajectory1D::OnTrigger(DynExp::ModuleInstance* Instance) const
	{
		auto ModuleData = DynExp::dynamic_ModuleData_cast<Trajectory1D>(Instance->ModuleDataGetter());

		Trigger(ModuleData);
	}
}