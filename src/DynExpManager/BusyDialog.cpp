// This file is part of DynExp.

#include "stdafx.h"
#include "moc_BusyDialog.cpp"
#include "BusyDialog.h"

BusyDialog::BusyDialog(QWidget* parent)
	: QDialog(parent, Qt::Dialog | Qt::WindowTitleHint),
	CheckFinishedTimer(new QTimer(this)), CheckFinishedFunction(nullptr)
{
	ui.setupUi(this);

	setWindowTitle(QString("Please wait for ") + DynExp::DynExpName + "...");
	setFixedSize(size());

	// Start timer which regularly checks whether the specified action has finished.
	// If this is the case, the dialog is closed.
	connect(CheckFinishedTimer, &QTimer::timeout, this, &BusyDialog::OnCheckFinished);
	CheckFinishedTimer->setInterval(std::chrono::milliseconds(1));
}

void BusyDialog::SetDescriptionText(QString Text)
{
	if (Text.isEmpty())
		ui.DescriptionLabel->setText("Please wait...");
	else
		ui.DescriptionLabel->setText(Text);
}

void BusyDialog::SetCheckFinishedFunction(const CheckFinishedFunctionType CheckFinishedFunction)
{
	this->CheckFinishedFunction = CheckFinishedFunction;
}

void BusyDialog::showEvent(QShowEvent* event)
{
	Exception = nullptr;

	if (CheckFinishedFunction)
		CheckFinishedTimer->start();

	ui.cancelButton->setEnabled(CheckFinishedFunction != nullptr);

	event->accept();
}

void BusyDialog::closeEvent(QCloseEvent* event)
{
	CheckFinishedTimer->stop();

	event->accept();
}

// If CheckFinishedFunction is nullptr, only the parent decides when this dialog is closed
void BusyDialog::reject()
{
	if (CheckFinishedFunction)
		QDialog::reject();
}

void BusyDialog::OnCheckFinished()
{
	try
	{
		if (CheckFinishedFunction)
			if (CheckFinishedFunction())
				accept();
	}
	catch ([[maybe_unused]] const Util::TimeoutException& e)
	{
		// Swallow TimeoutException, because this is most likely a temporary error.
	}
	catch (...)
	{
		Exception = std::current_exception();

		// Treat any case of an exception as aborting the action.
		reject();
	}
}