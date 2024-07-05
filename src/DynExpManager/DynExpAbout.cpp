// This file is part of DynExp.

#include "stdafx.h"
#include "moc_DynExpAbout.cpp"
#include "DynExpAbout.h"

DynExpAbout::DynExpAbout(QWidget *parent)
	: QDialog(parent, Qt::Dialog | Qt::WindowTitleHint | Qt::WindowCloseButtonHint)
{
	ui.setupUi(this);

	setFixedSize(size());

	ui.labelVersion->setText("Version " + QString(DynExp::DynExpVersion));
	ui.labelCopyright->setText("Copyright (C) 2020-" + QString::number(CompilationYear()) + " Julian M. Bopp");
	ui.labelQtVersion->setText("Using Qt " + QString(QT_VERSION_STR)
		+ " under the terms of the GNU General Public License version 3.");
	ui.labelGSLVersion->setText("Using the GNU Scientific Library " + QString(GSL_VERSION)
		+ " under the terms of the GNU General Public License version 3.");
	ui.labelgRPCVersion->setText("Using gRPC " + QString(grpc_version_string())
		+ " under the terms of the <a href=\"http://www.apache.org/licenses/LICENSE-2.0\">Apache License, Version 2.0</a>.");
	ui.labelPythonVersion->setText("Using Python " + QString(PY_VERSION)
		+ " under the terms of the <a href=\"https://docs.python.org/3/license.html#psf-license\">Python Software Foundation License Agreement</a>.");
	ui.labelpybind11Version->setText("Using pybind11 "
		+ QString::fromStdString(Util::ToStr(Util::VersionType{ PYBIND11_VERSION_MAJOR, PYBIND11_VERSION_MINOR, PYBIND11_VERSION_PATCH }))
		+ " under a <a href=\"https://github.com/pybind/pybind11/blob/master/LICENSE\">BSD-style license</a>.");
}

DynExpAbout::~DynExpAbout()
{
}