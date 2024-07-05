// This file is part of DynExp.

#include "stdafx.h"
#include "moc_ParamsConfig.cpp"
#include "ParamsConfig.h"
#include "Object.h"
#include "DynExpCore.h"

/**
 * @brief Alias for a typed parameter holding a single value.
 * @tparam DestinyType Type of the typed parameter's value
*/
template <typename DestinyType>
using ParamWrapperType = DynExp::ParamsBase::TypedParamBase<DestinyType>;

/**
 * @brief Wraps @p ParamWrapperType into a @p std::reference_wrapper, thus defining a reference
 * to a typed parameter.
 * @tparam DestinyType Type of the typed parameter's value
*/
template <typename DestinyType>
using ParamRefWrapperType = std::reference_wrapper<ParamWrapperType<DestinyType>>;

/**
 * @brief Alias for a reference to a parameter describing a link to another DynExp::Object instance
*/
using LinkParamRefWrapperType = std::reference_wrapper<DynExp::ParamsBase::LinkParamBase>;

/**
 * @brief Alias for a reference to a list parameter describing links to multiple DynExp::Object instances
*/
using LinkListParamRefWrapperType = std::reference_wrapper<DynExp::ParamsBase::LinkListParamBase>;

ParamsConfigDialog::ParamsConfigDialog(QWidget* parent, const DynExp::DynExpCore& Core, std::string Title)
	: QDialog(parent, Qt::Dialog | Qt::WindowTitleHint | Qt::WindowCloseButtonHint),
	Core(Core), Object(nullptr), ResetRequired(false)
{
	ui.setupUi(this);

	setWindowTitle(QString::fromStdString(Title));
}

void ParamsConfigDialog::AddParam(ParamInfo&& Info, const std::any Destiny, const NumberType Value,
	const NumberType MinValue, const NumberType MaxValue, const NumberType Precision, const NumberType Increment)
{
	auto SpinBox = new QDoubleSpinBox(this);
	SpinBox->setDecimals(Precision);
	SpinBox->setMinimum(MinValue);
	SpinBox->setMaximum(MaxValue);
	SpinBox->setSingleStep(Increment);
	SpinBox->setValue(Value);

	Param ParamData(Param::ParamType::Number, SpinBox, Destiny);
	InsertWidget(std::move(Info), std::move(ParamData));
}

void ParamsConfigDialog::AddParam(ParamInfo&& Info, const std::any Destiny, const TextType Value, const DynExp::TextUsageType TextUsage)
{
	auto LineEdit = new QLineEdit(this);
	LineEdit->setText(QString::fromStdString(Value));

	Param ParamData(TextUsage == DynExp::TextUsageType::Path ? Param::ParamType::Path :
		(TextUsage == DynExp::TextUsageType::Code ? Param::ParamType::Code : Param::ParamType::Text), LineEdit, Destiny);
	InsertWidget(std::move(Info), std::move(ParamData));
}

void ParamsConfigDialog::AddParam(ParamInfo&& Info, const std::any Destiny, const TextRefType Value,
	const TextListType& TextList, const bool AllowResetToDefault)
{
	if (TextList.empty())
		throw Util::EmptyException("A parameter's text list must not be empty.");

	auto ComboBox = new QComboBox(this);
	int SelectedIndex = 0;

	for (const auto& Entry : TextList)
	{
		if (Value == Entry)
			SelectedIndex = ComboBox->count();

		ComboBox->addItem(QString::fromStdString(Entry));
	}
	ComboBox->setCurrentIndex(SelectedIndex);

	Param ParamData(Param::ParamType::TextList, ComboBox, Destiny, 0, AllowResetToDefault);
	InsertWidget(std::move(Info), std::move(ParamData));
}

void ParamsConfigDialog::AddParam(ParamInfo&& Info, const std::any Destiny, const TextListIndexType Value,
	const TextListIndexType DefaultValue, const TextListType& TextList)
{
	if (TextList.empty())
		throw Util::EmptyException("A parameter's text list must not be empty.");
	if (Value >= TextList.size())
		throw Util::OutOfRangeException("A parameter's text list does not contain enough entries to select the specified initial value.");
	if (DefaultValue >= TextList.size())
		throw Util::OutOfRangeException("A parameter's text list does not contain enough entries to define a default value.");

	auto ComboBox = new QComboBox(this);
	for (const auto& Entry : TextList)
		ComboBox->addItem(QString::fromStdString(Entry));
	ComboBox->setCurrentIndex(Util::NumToT<int>(Value));

	Param ParamData(Param::ParamType::IndexedTextList, ComboBox, Destiny, DefaultValue);
	InsertWidget(std::move(Info), std::move(ParamData));
}

void ParamsConfigDialog::AddParam(ParamInfo&& Info, const std::any Destiny, const DynExp::ItemIDType Value,
	bool IsOptional, std::string_view IconResourcePath, FunctionsToCallIfAcceptedType FunctionToCallIfAccepted,
	Util::TextValueListType<IndexType>&& ItemIDsWithLabels)
{
	auto ComboBox = new QComboBox(this);
	int SelectedIndex = 0;

	if (IsOptional)
		ComboBox->addItem("< None >", QVariant(static_cast<IndexType>(DynExp::ItemIDNotSet)));

	FunctionsToCallIfAccepted.push_back(FunctionToCallIfAccepted);

	for (const auto& TextValuePair : ItemIDsWithLabels)
	{
		if (TextValuePair.second == Value)
			SelectedIndex = ComboBox->count();

		if (!IconResourcePath.empty())
			ComboBox->addItem(QIcon(IconResourcePath.data()), QString::fromStdString(TextValuePair.first), QVariant(static_cast<IndexType>(TextValuePair.second)));
		else
			ComboBox->addItem(QString::fromStdString(TextValuePair.first), QVariant(static_cast<IndexType>(TextValuePair.second)));
	}
	ComboBox->setCurrentIndex(SelectedIndex);

	Param ParamData(Param::ParamType::Index, ComboBox, Destiny, false);
	InsertWidget(std::move(Info), std::move(ParamData));
}

void ParamsConfigDialog::AddParam(ParamInfo&& Info, const std::any Destiny, const std::vector<DynExp::ItemIDType>& Values,
	bool IsOptional, std::string_view IconResourcePath, FunctionsToCallIfAcceptedType FunctionToCallIfAccepted,
	Util::TextValueListType<IndexType>&& ItemIDsWithLabels)
{
	auto ChoiceListDlg = new ChoiceListDialog(this, std::move(ItemIDsWithLabels), Info.Title, IsOptional, IconResourcePath, Values);
	
	FunctionsToCallIfAccepted.push_back(FunctionToCallIfAccepted);

	auto SetButton = new QPushButton(this);
	SetButton->setProperty("ChoiceListDlg", QVariant::fromValue(ChoiceListDlg));
	SetButton->setText("Select...");
	if (!IconResourcePath.empty())
		SetButton->setIcon(QIcon(IconResourcePath.data()));
	connect(SetButton, &QPushButton::clicked, ChoiceListDlg, &QDialog::open);

	Param ParamData(Param::ParamType::IndexList, SetButton, Destiny, false);
	InsertWidget(std::move(Info), std::move(ParamData));
}

bool ParamsConfigDialog::Display(DynExp::Object* Object)
{
	this->Object = Object;
	ResetRequired = false;

	TextEditorDialogs.clear();

	adjustSize();
	layout()->setSizeConstraint(QLayout::SetFixedSize);

	return exec() == DialogCode::Accepted;
}

void ParamsConfigDialog::InsertWidget(ParamInfo&& Info, Param&& ParamData)
{
	auto Label = new QLabel(QString::fromStdString(Info.Title), this);
	Label->setToolTip(QString::fromStdString(Info.Description));
	ParamData.Widget->setToolTip(QString::fromStdString(Info.Description));

	auto SubLayout = std::make_unique<QHBoxLayout>();
	SubLayout->addWidget(ParamData.Widget);

	ParamList.push_back(std::move(ParamData));

	if (ParamData.Type == Param::ParamType::Path || ParamData.Type == Param::ParamType::Code)
	{
		auto OpenButton = new QPushButton(QIcon(DynExpUI::Icons::Open), "", this);
		OpenButton->setToolTip("Browse...");
		OpenButton->setMaximumWidth(24);
		OpenButton->setProperty("ParamID", QVariant::fromValue(ParamList.size() - 1));

		// functor-based syntax does not work with inheriting QDialog privately, so use string-based syntax
		connect(OpenButton, SIGNAL(clicked()), this, SLOT(OnOpenParam()));

		SubLayout->addWidget(OpenButton);

		if (ParamData.Type == Param::ParamType::Code)
		{
			auto EditButton = new QPushButton(QIcon(DynExpUI::Icons::Edit), "", this);
			EditButton->setToolTip("Edit...");
			EditButton->setMaximumWidth(24);
			EditButton->setProperty("ParamID", QVariant::fromValue(ParamList.size() - 1));

			// functor-based syntax does not work with inheriting QDialog privately, so use string-based syntax
			connect(EditButton, SIGNAL(clicked()), this, SLOT(OnEditParam()));

			SubLayout->addWidget(EditButton);
		}
	}

	if (ParamData.Type != Param::ParamType::Index && ParamData.Type != Param::ParamType::IndexList && ParamData.AllowResetToDefault)
	{
		auto ResetButton = new QPushButton(QIcon(DynExpUI::Icons::Undo), "", this);
		ResetButton->setToolTip("Reset to default value");
		ResetButton->setMaximumWidth(24);
		ResetButton->setProperty("ParamID", QVariant::fromValue(ParamList.size() - 1));

		// functor-based syntax does not work with inheriting QDialog privately, so use string-based syntax
		connect(ResetButton, SIGNAL(clicked()), this, SLOT(OnResetParam()));

		SubLayout->addWidget(ResetButton);
	}

	ui.MainLayout->addRow(Label, SubLayout.release());
}

void ParamsConfigDialog::HandleTextEditorDialogsOnClose()
{
	for (auto EditorDlg : TextEditorDialogs)
	{
		if (EditorDlg->isVisible())
		{
			// setParent() hides the dialog and resets flags, so show it again and restore flags.
			auto Flags = EditorDlg->windowFlags();
			EditorDlg->setParent(nullptr);
			EditorDlg->setWindowFlags(Flags);
			EditorDlg->setWindowIcon(QIcon(DynExpUI::Icons::DynExp));
			EditorDlg->showNormal();
			EditorDlg->setAttribute(Qt::WA_DeleteOnClose);
		}
	}
}

// Thread-safe since it only reads constants
void ParamsConfigDialog::OnOpenParam()
{
	auto& ParamToOpen = ParamList.at(qobject_cast<QPushButton*>(sender())->property("ParamID").value<size_t>());

	if (ParamToOpen.Type != Param::ParamType::Path && ParamToOpen.Type != Param::ParamType::Code)
		throw Util::InvalidStateException("A parameter's type is corrupted.");

	QString Filename = Util::PromptOpenFilePath(this, "Select file for parameter", "", "", "");
	if (!Filename.isEmpty())
		static_cast<QLineEdit*>(ParamToOpen.Widget)->setText(Filename);
}

// Thread-safe since it only reads constants
void ParamsConfigDialog::OnEditParam()
{
	auto& ParamToEdit = ParamList.at(qobject_cast<QPushButton*>(sender())->property("ParamID").value<size_t>());

	if (ParamToEdit.Type != Param::ParamType::Code)
		throw Util::InvalidStateException("A parameter's type is corrupted.");

	auto Filename = static_cast<QLineEdit*>(ParamToEdit.Widget)->text();
	if (!Filename.isEmpty())
	{
		// ParamsConfigDialog is parent to allow editing despite ParamsConfigDialog being a modal dialog.
		TextEditorDialogs.emplace_back(new TextEditor(this, Core.ToAbsolutePath(Filename.toStdString())));
		TextEditorDialogs.back()->show();
	}
}

// Thread-safe since it only reads constants
void ParamsConfigDialog::OnResetParam()
{
	auto& ParamToReset = ParamList.at(qobject_cast<QPushButton*>(sender())->property("ParamID").value<size_t>());

	switch (ParamToReset.Type)
	{
	case Param::ParamType::Number:
		static_cast<QDoubleSpinBox*>(ParamToReset.Widget)->setValue(
			std::any_cast<ParamRefWrapperType<NumberType>>(ParamToReset.Destiny).get().GetDefaultValue());
		break;
	case Param::ParamType::SignedInteger: [[fallthrough]];
	case Param::ParamType::UnsignedInteger:
		static_cast<QComboBox*>(ParamToReset.Widget)->setCurrentIndex(Util::NumToT<int>(ParamToReset.DefaultIndex));
		break;
	case Param::ParamType::Text:
	case Param::ParamType::Path:
	case Param::ParamType::Code:
		static_cast<QLineEdit*>(ParamToReset.Widget)->setText(
			QString::fromStdString(std::any_cast<ParamRefWrapperType<TextType>>(ParamToReset.Destiny).get().GetDefaultValue()));
		break;
	case Param::ParamType::TextList:
		static_cast<QComboBox*>(ParamToReset.Widget)->setCurrentText(
			QString::fromStdString(std::any_cast<ParamRefWrapperType<TextType>>(ParamToReset.Destiny).get().GetDefaultValue()));
		break;
	case Param::ParamType::IndexedTextList:
		static_cast<QComboBox*>(ParamToReset.Widget)->setCurrentIndex(Util::NumToT<int>(
			std::any_cast<ParamRefWrapperType<IndexType>>(ParamToReset.Destiny).get().GetDefaultValue()));
		break;
	case Param::ParamType::Index: [[fallthrough]];
	case Param::ParamType::IndexList: [[fallthrough]];
	default:
		throw Util::InvalidStateException("A parameter's type is corrupted.");
	}
}

void ParamsConfigDialog::accept()
{
	try
	{
		// Ensure thread-safety in case of editing an existing Object. See e.g. class ObjectLink<T>!
		DynExp::Object::ParamsTypeSyncPtrType lock;
		if (Object)
			lock = Object->GetParams();

		ChoiceListDialog* ChoiceListDlg = nullptr;

		for (const auto& ParamEntry : ParamList)
			switch (ParamEntry.Type)
			{
			case Param::ParamType::Number:
				Assign(std::any_cast<ParamRefWrapperType<NumberType>>(ParamEntry.Destiny).get(),
					static_cast<QDoubleSpinBox*>(ParamEntry.Widget)->value());
				break;
			case Param::ParamType::SignedInteger:
				Assign(std::any_cast<ParamRefWrapperType<DynExp::ParamsBase::EnumParamSignedIntegerType>>(ParamEntry.Destiny).get(),
					static_cast<QComboBox*>(ParamEntry.Widget)->currentData().value<DynExp::ParamsBase::EnumParamSignedIntegerType>());
				break;
			case Param::ParamType::UnsignedInteger:
				Assign(std::any_cast<ParamRefWrapperType<DynExp::ParamsBase::EnumParamUnsignedIntegerType>>(ParamEntry.Destiny).get(),
					static_cast<QComboBox*>(ParamEntry.Widget)->currentData().value<DynExp::ParamsBase::EnumParamUnsignedIntegerType>());
				break;
			case Param::ParamType::Text:
			case Param::ParamType::Path:
			case Param::ParamType::Code:
				Assign(std::any_cast<ParamRefWrapperType<TextType>>(ParamEntry.Destiny).get(),
					static_cast<QLineEdit*>(ParamEntry.Widget)->text().toStdString());
				break;
			case Param::ParamType::TextList:
				Assign(std::any_cast<ParamRefWrapperType<TextType>>(ParamEntry.Destiny).get(),
					static_cast<QComboBox*>(ParamEntry.Widget)->currentText().toStdString());
				break;
			case Param::ParamType::IndexedTextList:
				Assign(std::any_cast<ParamRefWrapperType<IndexType>>(ParamEntry.Destiny).get(),
					static_cast<QComboBox*>(ParamEntry.Widget)->currentIndex());
				break;
			case Param::ParamType::Index:
				Assign(std::any_cast<LinkParamRefWrapperType>(ParamEntry.Destiny).get(),
					static_cast<QComboBox*>(ParamEntry.Widget)->currentData().value<IndexType>());
				break;
			case Param::ParamType::IndexList:
				ChoiceListDlg = static_cast<QPushButton*>(ParamEntry.Widget)->property("ChoiceListDlg").value<decltype(ChoiceListDlg)>();
				if (!ChoiceListDlg)
					throw Util::InvalidDataException("A parameter's \"ChoiceListDlg\" property is corrupted.");
				if (!ChoiceListDlg->IsOptional() && ChoiceListDlg->GetSelection().empty())
				{
					QMessageBox::warning(this, "DynExp - Parameter empty",
						QString("The parameter \"") + ChoiceListDlg->GetParamName().data() + "\" must not be empty. Please assign at least one item.");
					return;
				}
				Assign(std::any_cast<LinkListParamRefWrapperType>(ParamEntry.Destiny).get(), ChoiceListDlg->GetSelection());
				break;
			default:
				throw Util::InvalidStateException("A parameter's type is corrupted.");
			}

		for (const auto& Function : FunctionsToCallIfAccepted)
			Function();
	}
	catch (...)
	{
		reject();
	}

	HandleTextEditorDialogsOnClose();

	QDialog::accept();
}

void ParamsConfigDialog::reject()
{
	HandleTextEditorDialogsOnClose();

	QDialog::reject();
}