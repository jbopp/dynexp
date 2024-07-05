// This file is part of DynExp.

/**
 * @file BusyDialog.h
 * @brief Implements a dialog with a progress bar, which shows the user that %DynExp is busy.
*/

#pragma once

#include <QWidget>
#include "ui_BusyDialog.h"

class BusyDialog : public QDialog
{
	Q_OBJECT

public:
	using CheckFinishedFunctionType = std::function<bool(void)>;

	BusyDialog(QWidget* parent);
	~BusyDialog() = default;

	auto GetException() const noexcept { return Exception; }

	void SetDescriptionText(QString Text);
	void SetCheckFinishedFunction(const CheckFinishedFunctionType CheckFinishedFunction);

private:
	virtual void showEvent(QShowEvent* event) override;
	virtual void closeEvent(QCloseEvent* event) override;
	virtual void reject() override;

	Ui::BusyDialog ui;
	QTimer* CheckFinishedTimer;

	CheckFinishedFunctionType CheckFinishedFunction;
	std::exception_ptr Exception;

private slots:
	void OnCheckFinished();
};