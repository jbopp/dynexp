// This file is part of DynExp.

/**
 * @file ChoiceListDialog.h
 * @brief Implements a dialog with a list of available items on the left and a list of
 * selected items on the right to let the user make a selection of multiple items of
 * a specific order.
*/

#pragma once

#include "stdafx.h"

#include <QWidget>
#include "ui_ChoiceListDialog.h"

class ChoiceListDialog : public QDialog
{
	Q_OBJECT

	// Better DynExp::ItemIDType, but this is incompatible with g++ compiler. Should equal ParamsConfigDialog::IndexType.
	using IndexType = qulonglong;

public:
	ChoiceListDialog(QWidget* parent, Util::TextValueListType<IndexType>&& ItemIDsWithLabels,
		std::string_view ParamName, bool IsOptional, std::string_view IconResourcePath, const std::vector<DynExp::ItemIDType>& Values);
	~ChoiceListDialog() = default;

	std::string_view GetParamName() const { return ParamName; }
	bool IsOptional() const noexcept { return Optional; }
	const auto& GetSelection() const { return Values; }

private:
	virtual void showEvent(QShowEvent* event) override;

	Ui::ChoiceListDialog ui;

	const Util::TextValueListType<IndexType> ItemIDsWithLabels;
	const std::string ParamName;
	const bool Optional;
	const QIcon ItemIcon;
	std::vector<DynExp::ItemIDType> Values;

private slots:
	void OnAddItemClicked();
	void OnRemoveItemClicked();
	void OnMoveItemUpClicked();
	void OnMoveItemDownClicked();

	virtual void accept() override;
	virtual void reject() override;
};

Q_DECLARE_METATYPE(ChoiceListDialog*);