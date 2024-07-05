// This file is part of DynExp.

#include "stdafx.h"
#include "moc_TextEditor.cpp"
#include "TextEditor.h"

TextEditor::TextEditor(QWidget* parent, const std::filesystem::path& Filename)
	: QWidget(parent, Qt::Window | Qt::CustomizeWindowHint | Qt::WindowTitleHint | Qt::WindowMinMaxButtonsHint | Qt::WindowCloseButtonHint),
	Filename(Filename)
{
	ui.setupUi(this);

	setWindowTitle(QString("Edit \"") + QString::fromStdString(Filename.string()) + "\"");

	if (this->Filename.extension().string() == ".py")
	{
		SyntaxHighlighter = new PythonSyntaxHighlighter(ui.TEText);
	}
}

void TextEditor::showEvent(QShowEvent* event)
{
	// Not just retoring from minimization...
	if (!event->spontaneous())
	{
		std::string Text;

		try
		{
			Text = Util::ReadFromFile(Filename.string());
		}
		catch (const Util::FileIOErrorException& e)
		{
			QMessageBox::warning(this, "DynExp - Error", e.what());

			QMetaObject::invokeMethod(this, "close", Qt::QueuedConnection);

			return;
		}

		ui.TEText->setPlainText(QString::fromStdString(Text));
	}

	event->accept();
}

void TextEditor::closeEvent(QCloseEvent* event)
{
	std::string Text;

	try
	{
		Text = Util::ReadFromFile(Filename.string());
	}
	catch ([[maybe_unused]] const Util::FileIOErrorException& e)
	{
		QWidget::closeEvent(event);
	}

	if (ui.TEText->toPlainText().toStdString() != Text)
	{
		auto Reply = QMessageBox::question(this, "DynExp - Save changes?",
			QString::fromStdString("The file \"" + Filename.string() + "\" has been edited. Save the changes?"),
			QMessageBox::StandardButton::Yes | QMessageBox::StandardButton::No | QMessageBox::StandardButton::Abort,
			QMessageBox::StandardButton::Abort);

		if (Reply == QMessageBox::StandardButton::Abort)
		{
			event->ignore();

			return;
		}
		else if (Reply == QMessageBox::StandardButton::Yes)
		{
			if (!DoSave())
			{
				event->ignore();

				return;
			}
		}
	}

	QWidget::closeEvent(event);
}

bool TextEditor::DoSave()
{
	if (!Util::SaveToFile(QString::fromStdString(Filename.string()), ui.TEText->toPlainText().toStdString()))
	{
		QMessageBox::warning(this, "DynExp - Error", "Error writing data to file.");

		return false;
	}

	return true;
}

void TextEditor::OnSave()
{
	DoSave();
}

void TextEditor::OnClose()
{
	close();
}