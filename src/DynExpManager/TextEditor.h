// This file is part of DynExp.

/**
 * @file TextEditor.h
 * @brief Implements a dialog to edit and save small pieces of text or (Python) code.
*/

#pragma once

#include "PythonSyntaxHighlighter.h"
#include "stdafx.h"

#include <QWidget>
#include "ui_TextEditor.h"

class TextEditor : public QWidget
{
	Q_OBJECT

	// Better DynExp::ItemIDType, but this is incompatible with g++ compiler. Should equal ParamsConfigDialog::IndexType.
	using IndexType = qulonglong;

public:
	TextEditor(QWidget* parent, const std::filesystem::path& Filename);
	~TextEditor() = default;

	auto& GetFilename() const { return Filename; }

private:
	virtual void showEvent(QShowEvent* event) override;
	virtual void closeEvent(QCloseEvent* event) override;

	/**
	 * @brief Saves the editor's content to the file @p Filename.
	 * @return Returns true in case of success, false otherwise.
	*/
	bool DoSave();

	Ui::TextEditor ui;

	const std::filesystem::path Filename;

	QSyntaxHighlighter* SyntaxHighlighter = nullptr;

private slots:
	void OnSave();
	void OnClose();
};