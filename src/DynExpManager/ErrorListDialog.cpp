// This file is part of DynExp.

#include "stdafx.h"
#include "moc_ErrorListDialog.cpp"
#include "ErrorListDialog.h"

bool ErrorListDialog::ErrorEntryType::operator==(const ErrorEntryType& Other) const
{
	return Other.Origin == Origin && Other.Text == Text && Other.ErrorType == ErrorType && Other.TreeWidgetItem == TreeWidgetItem;
}

ErrorListDialog::ErrorListDialog(QWidget *parent, QWidget* WidgetToOpenThisDialog)
	: QDialog(parent, Qt::Widget | Qt::CustomizeWindowHint | Qt::FramelessWindowHint | Qt::NoDropShadowWindowHint),
	WidgetToOpenThisDialog(WidgetToOpenThisDialog), ClosedByClickingOpenWidget(false),
	SelectionChanged(false), SelectedTreeWidgetItem(nullptr)
{
	setAttribute(Qt::WA_TranslucentBackground);

	ui.setupUi(this);

	ui.TWErrorList->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeMode::Fixed);
	ui.TWErrorList->verticalHeader()->setSectionResizeMode(QHeaderView::ResizeMode::ResizeToContents);
}

ErrorListDialog::~ErrorListDialog()
{
}

bool ErrorListDialog::HasBeenClosedByClickingOpenWidget() const noexcept
{
	return ClosedByClickingOpenWidget && std::chrono::system_clock::now() < LastCloseTime + std::chrono::milliseconds(500);
}

void ErrorListDialog::SetErrorEntries(const ErrorEntriesType& ErrorEntries)
{
	if (ui.TWErrorList->rowCount() && ErrorEntries == CurrentErrorEntries)
		return;

	ui.TWErrorList->clear();
	ui.TWErrorList->setRowCount(0);

	for (auto& ErrorEntry : ErrorEntries)
	{
		QIcon Icon(ErrorEntry.ErrorType == Util::ErrorType::Error ? DynExpUI::Icons::Error :
			(ErrorEntry.ErrorType == Util::ErrorType::Warning ? DynExpUI::Icons::Warning : DynExpUI::Icons::Info));

		const auto Row = ui.TWErrorList->rowCount();
		ui.TWErrorList->insertRow(Row);

		ui.TWErrorList->setItem(Row, 0, new QTableWidgetItem(Icon, ErrorEntry.Text));
		ui.TWErrorList->item(Row, 0)->setToolTip("Double-click to go to item.");
		ui.TWErrorList->item(Row, 0)->setData(Qt::ItemDataRole::UserRole, QVariant::fromValue(ErrorEntry.TreeWidgetItem));
		ui.TWErrorList->setItem(Row, 1, new QTableWidgetItem(ErrorEntry.Origin));
		ui.TWErrorList->item(Row, 1)->setToolTip(ui.TWErrorList->item(Row, 0)->toolTip());
		ui.TWErrorList->item(Row, 1)->setData(Qt::ItemDataRole::UserRole, QVariant::fromValue(ErrorEntry.TreeWidgetItem));
	}

	if (ErrorEntries.empty())
	{
		ui.TWErrorList->insertRow(0);
		ui.TWErrorList->setItem(0, 0, new QTableWidgetItem(QIcon(DynExpUI::Icons::Info), "No errors or warnings."));
		ui.TWErrorList->item(0, 0)->setToolTip(ui.TWErrorList->item(0, 0)->text());
		ui.TWErrorList->item(0, 0)->setData(Qt::ItemDataRole::UserRole, QVariant::fromValue(static_cast<decltype(ErrorEntryType::TreeWidgetItem)>(nullptr)));
	}

	CurrentErrorEntries = ErrorEntries;
}

QTreeWidgetItem* ErrorListDialog::GetSelectedEntry()
{
	auto OldSelectionChanged = SelectionChanged;
	SelectionChanged = false;

	return OldSelectionChanged ? SelectedTreeWidgetItem : nullptr;
}

void ErrorListDialog::paintEvent(QPaintEvent* event)
{
	QVector<QPointF> BalloonVertices;
	const float Offset = 0;
	BalloonVertices << QPointF(Offset, Offset)
		<< QPointF(width() - Offset, Offset)
		<< QPointF(width() - Offset, height() * 0.97)
		<< QPointF(width() * 0.04, height() * 0.97)
		<< QPointF(width() * 0.03, height() - Offset)
		<< QPointF(width() * 0.02, height() * 0.97)
		<< QPointF(Offset, height() * 0.97);
	auto BalloonPolygon = QPolygonF(BalloonVertices);

	QPainter painter(this);
	painter.setRenderHint(QPainter::Antialiasing, true);
	painter.setBrush(QBrush(ui.TWErrorList->palette().color(QWidget::backgroundRole())));
	painter.setPen(QPen(ui.TWErrorList->palette().color(QWidget::foregroundRole())));

	QRegion Mask(BalloonPolygon.toPolygon(), Qt::WindingFill);
	painter.drawPolygon(BalloonPolygon);
	setMask(Mask);

	ui.TWErrorList->horizontalHeader()->resizeSection(0, width() / 4 * 3);
}

void ErrorListDialog::focusOutEvent(QFocusEvent* event)
{
	if (!ui.TWErrorList->hasFocus())
	{
		// Remember wheter this dialog has been closed by clicking on the widget which opens the dialog again.
		// Direct reopening would be strange behavior, so avoid that (refer to DynExpManager::OnStatusBarStateClicked()).
		ClosedByClickingOpenWidget = WidgetToOpenThisDialog && WidgetToOpenThisDialog->underMouse();
		LastCloseTime = std::chrono::system_clock::now();
		
		hide();
	}
	else
		setFocus();
}

void ErrorListDialog::ErrorEntryDoubleClicked(QTableWidgetItem* item)
{
	SelectionChanged = true;
	SelectedTreeWidgetItem = item->data(Qt::ItemDataRole::UserRole).value<decltype(ErrorEntryType::TreeWidgetItem)>();

	hide();
}