// This file is part of DynExp.

/**
 * @file ErrorListDialog.h
 * @brief Implements a frame-less pop-up dialog to show errors and warning of DynExp::Object instances.
*/

#pragma once

#include <QWidget>
#include "ui_ErrorListDialog.h"

class ErrorListDialog : public QDialog
{
	Q_OBJECT

public:
	struct ErrorEntryType
	{
		bool operator==(const ErrorEntryType& Other) const;
		bool operator!=(const ErrorEntryType& Other) const { return !(*this == Other); }

		QString Origin;
		QString Text;
		Util::ErrorType ErrorType;

		/**
		 * @brief Pointer to QTreeWidgetItem listed in main window's tree view. Do not dereference this pointer!
		 * It is only meant for address comparison purposes and may point to deleted data even if it is not nullptr.
		*/
		QTreeWidgetItem* TreeWidgetItem;
	};

	using ErrorEntriesType = std::vector<ErrorEntryType>;

	ErrorListDialog(QWidget *parent, QWidget* WidgetToOpenThisDialog);
	~ErrorListDialog();

	bool HasBeenClosedByClickingOpenWidget() const noexcept;
	void ResetClosedByClickingOpenWidget() noexcept { ClosedByClickingOpenWidget = false; }

	void SetErrorEntries(const ErrorEntriesType& ErrorEntries);
	QTreeWidgetItem* GetSelectedEntry();

protected:
	virtual void paintEvent(QPaintEvent* event) override;
	virtual void focusOutEvent(QFocusEvent* event) override;

private:
	Ui::ErrorListDialog ui;

	QWidget* WidgetToOpenThisDialog;
	bool ClosedByClickingOpenWidget;
	std::chrono::time_point<std::chrono::system_clock> LastCloseTime;

	ErrorEntriesType CurrentErrorEntries;
	bool SelectionChanged;
	QTreeWidgetItem* SelectedTreeWidgetItem;

private slots:
	void ErrorEntryDoubleClicked(QTableWidgetItem* item);
};