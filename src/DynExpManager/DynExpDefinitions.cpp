// This file is part of DynExp.

#include "stdafx.h"
#include "DynExpDefinitions.h"

QPalette DynExpUI::DarkPalette::GetPalette()
{
	QPalette Palette;
	Palette.setColor(QPalette::Window, darkGray);
	Palette.setColor(QPalette::WindowText, Qt::white);
	Palette.setColor(QPalette::Base, black);
	Palette.setColor(QPalette::AlternateBase, darkGray);
	Palette.setColor(QPalette::ToolTipBase, blue);
	Palette.setColor(QPalette::ToolTipText, Qt::white);
	Palette.setColor(QPalette::Text, Qt::white);
	Palette.setColor(QPalette::Button, darkGray);
	Palette.setColor(QPalette::ButtonText, Qt::white);
	Palette.setColor(QPalette::Link, blue);
	Palette.setColor(QPalette::LinkVisited, blue);
	Palette.setColor(QPalette::Highlight, blue);
	Palette.setColor(QPalette::HighlightedText, Qt::black);
	Palette.setColor(QPalette::BrightText, Qt::red);

	Palette.setColor(QPalette::Active, QPalette::Button, gray.darker());
	Palette.setColor(QPalette::Disabled, QPalette::ButtonText, gray);
	Palette.setColor(QPalette::Disabled, QPalette::WindowText, gray);
	Palette.setColor(QPalette::Disabled, QPalette::Text, gray);
	Palette.setColor(QPalette::Disabled, QPalette::Light, darkGray);

	return Palette;
}

QLinearGradient DynExpUI::GetDefaultLinearGradient()
{
	QLinearGradient Gradient;
	Gradient.setColorAt(0, QColor(35, 21, 87));
	Gradient.setColorAt(.29, QColor(68, 16, 122));
	Gradient.setColorAt(.67, QColor(255, 19, 97));
	Gradient.setColorAt(1, QColor(255, 248, 0));

	return Gradient;
}