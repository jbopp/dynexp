// This file is part of DynExp.

/**
 * @file DynExpAbout.h
 * @brief Implements a dialog to show license information about %DynExp.
*/

#pragma once

#include <QWidget>

namespace Ui
{
	class DynExpAbout;
}

class DynExpAbout : public QDialog
{
	Q_OBJECT

public:
	DynExpAbout(QWidget *parent);
	~DynExpAbout();

private:
	std::unique_ptr<Ui::DynExpAbout> ui;
};