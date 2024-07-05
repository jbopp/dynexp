// This file is part of DynExp.

#include "stdafx.h"
#include "moc_ChoiceListDialog.cpp"
#include "ChoiceListDialog.h"

ChoiceListDialog::ChoiceListDialog(QWidget* parent, Util::TextValueListType<IndexType>&& ItemIDsWithLabels,
	std::string_view ParamName, bool IsOptional, std::string_view IconResourcePath, const std::vector<DynExp::ItemIDType>& Values)
	: QDialog(parent, Qt::Dialog | Qt::WindowTitleHint),
	ItemIDsWithLabels(std::move(ItemIDsWithLabels)), ParamName(ParamName), Optional(IsOptional),
	ItemIcon(IconResourcePath.data()), Values(Values)
{
	ui.setupUi(this);

	setWindowTitle(QString("Select items - ") + ParamName.data());
	
	connect(ui.LWAvailable, &QListWidget::itemChanged, [this](QListWidgetItem*) { ui.LWAvailable->sortItems(); });
}

void ChoiceListDialog::showEvent(QShowEvent* event)
{
	const auto SelectionFilter = [this](bool SelectionState, const auto& IDLabelPair) {
		return (std::find_if(Values.cbegin(), Values.cend(), [&IDLabelPair = std::as_const(IDLabelPair)](const auto ID) {
			return IDLabelPair.second == ID;
		}) == Values.cend()) != SelectionState;
	};

	const auto SelectedFilter = [&SelectionFilter](const auto& IDLabelPair) { return SelectionFilter(true, IDLabelPair); };
	const auto NotSelectedFilter = [&SelectionFilter](const auto& IDLabelPair) { return SelectionFilter(false, IDLabelPair); };

	const auto AddListWidgetItem = [this](const decltype(ItemIDsWithLabels)::value_type& Item, QListWidget* List) {
		auto ListItem = new QListWidgetItem(QString::fromStdString(Item.first), List);
		ListItem->setIcon(ItemIcon);
		ListItem->setToolTip("ID " + QString::number(Item.second));
		ListItem->setData(Qt::UserRole, QVariant::fromValue(Item.second));
	};

	ui.LWAvailable->clear();
	for (const auto& Item : ItemIDsWithLabels | std::views::filter(NotSelectedFilter))
		AddListWidgetItem(Item, ui.LWAvailable);

	// Do not directly insert, to sort first according to Values vector.
	Util::TextValueListType<DynExp::ItemIDType> SelectedItems;
	for (const auto& Item : ItemIDsWithLabels | std::views::filter(SelectedFilter))
		SelectedItems.push_back(Item);

	std::sort(SelectedItems.begin(), SelectedItems.end(), [&Values = std::as_const(Values)](const auto& a, const auto& b) {
		const auto ItA = std::find(Values.cbegin(), Values.cend(), a.second);
		const auto ItB = std::find(Values.cbegin(), Values.cend(), b.second);

		return ItA == Values.cend() || ItB == Values.cend() || ItA - Values.cbegin() < ItB - Values.cbegin();
	});

	ui.LWSelected->clear();
	for (const auto& Item : SelectedItems)
		AddListWidgetItem(Item, ui.LWSelected);

	event->accept();
}

void ChoiceListDialog::OnAddItemClicked()
{
	auto SelectedItems = ui.LWAvailable->selectedItems();
	ui.LWAvailable->clearSelection();
	ui.LWSelected->clearSelection();

	for (auto Item : SelectedItems)
	{
		ui.LWSelected->addItem(ui.LWAvailable->takeItem(ui.LWAvailable->row(Item)));
		Item->setSelected(true);
	}
}

void ChoiceListDialog::OnRemoveItemClicked()
{
	auto SelectedItems = ui.LWSelected->selectedItems();
	ui.LWAvailable->clearSelection();
	ui.LWSelected->clearSelection();

	for (auto Item : SelectedItems)
	{
		ui.LWAvailable->addItem(ui.LWSelected->takeItem(ui.LWSelected->row(Item)));
		Item->setSelected(true);
	}
}

void ChoiceListDialog::OnMoveItemUpClicked()
{
	auto SelectedItems = ui.LWSelected->selectedItems();
	std::sort(SelectedItems.begin(), SelectedItems.end(), [this](const auto& a, const auto& b) {
		return ui.LWSelected->row(a) < ui.LWSelected->row(b);
	});

	for (auto Item : SelectedItems)
	{
		const auto Row = ui.LWSelected->row(Item);
		if (Row <= 0)
			continue;

		ui.LWSelected->insertItem(Row - 1, ui.LWSelected->takeItem(Row));
		Item->setSelected(true);
	}
}

void ChoiceListDialog::OnMoveItemDownClicked()
{
	auto SelectedItems = ui.LWSelected->selectedItems();
	std::sort(SelectedItems.begin(), SelectedItems.end(), [this](const auto& a, const auto& b) {
		return ui.LWSelected->row(a) > ui.LWSelected->row(b);
	});

	for (auto Item : SelectedItems)
	{
		const auto Row = ui.LWSelected->row(Item);
		if (Row >= ui.LWSelected->count() - 1)
			continue;

		ui.LWSelected->insertItem(Row + 1, ui.LWSelected->takeItem(Row));
		Item->setSelected(true);
	}
}

void ChoiceListDialog::accept()
{
	if (!IsOptional() && !ui.LWSelected->count())
	{
		QMessageBox::warning(this, "DynExp - Parameter empty",
			QString("The parameter \"") + GetParamName().data() + "\" must not be empty. Please assign at least one item.");
		return;
	}

	Values.clear();
	for (Util::return_of_t<decltype(&QListWidget::count)> i = 0; i < ui.LWSelected->count(); ++i)
		Values.push_back(ui.LWSelected->item(i)->data(Qt::UserRole).value<decltype(Values)::value_type>());

	QDialog::accept();
}

void ChoiceListDialog::reject()
{
	QDialog::reject();
}