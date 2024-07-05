// This file is part of DynExp.

/**
 * @file ParamsConfig.h
 * @brief Implements a configuration dialog which allows users to set values of parameters
 * derived from DynExp::ParamsBase::ParamBase.
*/

#pragma once

#include <QWidget>
#include "ui_ParamsConfig.h"
#include "ChoiceListDialog.h"
#include "TextEditor.h"

#include "stdafx.h"

namespace DynExp
{
	class DynExpCore;
	class Object;
	class ObjectLinkBase;

	/**
	 * @brief Specifies the usage of a text-type parameter. Setting the right usage allows the @p ParamsConfigDialog
	 * to add specific functionality to a text field to edit the parameter's value.
	*/
	enum class TextUsageType { Standard, Path, Code };
	/**
	 * @var DynExp::TextUsageType TextUsageType::Standard
	 * Plain text, no functionality is added.
	*/
	/**
	 * @var DynExp::TextUsageType TextUsageType::Path
	 * Interprets the text as a file path and adds a 'browse' button.
	*/
	/**
	 * @var DynExp::TextUsageType TextUsageType::Code
	 * Behaves like TextUsageType::Path, but also adds a button to open a text editor to edit the code file the
	 * text (interpreted as a file path) points to.
	*/
}

/**
 * @brief Bundles a parameter's title and description texts
*/
struct ParamInfo
{
	/**
	 * @brief Constructs an instance and initializes #Title and #Description.
	 * @param Title Refer to #Title.
	 * @param Description Refer to #Description.
	*/
	ParamInfo(std::string Title, std::string Description)
		: Title(std::move(Title)), Description(std::move(Description)) {}

	/**
	 * @copydoc ParamInfo(std::string, std::string)
	*/
	ParamInfo(std::string_view Title, std::string_view Description)
		: Title(Title), Description(Description) {}

	const std::string Title;											//!< Refer to DynExp::ParamsBase::ParamBase::ParamTitle.
	const std::string Description;										//!< Refer to DynExp::ParamsBase::ParamBase::ParamDescription.
};

/**
 * @brief Defines the configuration dialog. The dialog must be displayed by calling
 * ParamsConfigDialog::Display(). Do not use methods of QDialog!
*/
class ParamsConfigDialog : public QDialog
{
	Q_OBJECT

public:
	using NumberType = double;											//!< Number type used for numeric parameters (DynExp::ParamsBase::Param)
	using TextType = Util::TextType;									//!< @copydoc Util::TextType
	using TextRefType = Util::TextRefType;								//!< @copydoc Util::TextRefType
	using TextListType = Util::TextListType;							//!< @copydoc Util::TextListType
	using TextListIndexType = Util::TextListIndexType;					//!< @copydoc Util::TextListIndexType

	/**
	 * @copydoc DynExp::ItemIDType
	 * @brief It would be better to use DynExp::ItemIDType here, but this is incompatible with the g++ compiler.
	 * @p IndexType should equal ChoiceListDialog::IndexType.
	*/
	using IndexType = qulonglong;

private:
	/**
	 * @brief Signature of a function which is called when the @p ParamsConfigDialog is closed
	 * by the user by clicking the 'accept' button.
	*/
	using FunctionsToCallIfAcceptedType = std::function<void(void)>;

	/**
	 * @brief Collection of data defining a single parameter to be managed by a @p ParamsConfigDialog
	 * instance. Used by ParamsConfigDialog::InsertWidget().
	*/
	struct Param
	{
		/**
		 * @brief Identifies the type of a parameter shown in the user interface. Also refer to DynExp::TextUsageType.
		*/
		enum class ParamType { Number, SignedInteger, UnsignedInteger, Text, Path, Code, TextList, IndexedTextList, Index, IndexList };
		/**
		 * @var DynExp::Param::ParamType ParamType::Number
		 * Numeric parameter consisting of a double-type number
		*/
		/**
		 * @var DynExp::Param::ParamType ParamType::SignedInteger
		 * Numeric parameter consisting of a signed integer number
		*/
		/**
		 * @var DynExp::Param::ParamType ParamType::UnsignedInteger
		 * Numeric parameter consisting of an unsigned integer number
		*/
		/**
		 * @var DynExp::Param::ParamType ParamType::Text
		 * @copydoc DynExp::TextUsageType::Standard
		*/
		/**
		 * @var DynExp::Param::ParamType ParamType::Path
		 * @copydoc DynExp::TextUsageType::Path
		*/
		/**
		 * @var DynExp::Param::ParamType ParamType::Code
		 * @copydoc DynExp::TextUsageType::Code
		*/
		/**
		 * @var DynExp::Param::ParamType ParamType::TextList
		 * Parameter consisting of a list of plain texts. The parameter stores the selected text itself.
		*/
		/**
		 * @var DynExp::Param::ParamType ParamType::IndexedTextList
		 * Parameter consisting of a list of plain texts. The parameter does not store the selected text
		 * itself but its index within a list of predefined options
		*/
		/**
		 * @var DynExp::Param::ParamType ParamType::Index
		 * Numeric parameter consisting of an unsigned integer number which refers to the ID of
		 * a @p DynExp::Object instance.
		*/
		/**
		 * @var DynExp::Param::ParamType ParamType::IndexList
		 * Numeric parameter consisting of a list of unsigned integer numbers which refer to an ID of
		 * a @p DynExp::Object instance each.
		*/

		Param() = delete;

		/**
		 * @brief Constructs a @p Param instance
		 * @param Type Refer to #Type.
		 * @param Widget Refer to #Widget.
		 * @param Destiny Refer to #Destiny.
		 * @param DefaultIndex Refer to #DefaultIndex.
		 * @param AllowResetToDefault Refer to #AllowResetToDefault.
		*/
		Param(const ParamType Type, QWidget* const Widget, const std::any Destiny,
			IndexType DefaultIndex = 0, const bool AllowResetToDefault = true)
			: Type(Type), Widget(Widget), Destiny(Destiny),
			DefaultIndex(DefaultIndex), AllowResetToDefault(AllowResetToDefault) {}

		const ParamType Type;				//!< Type of the managed parameter
		QWidget* const Widget;				//!< Widget to edit/show the parameter's value in the UI
		const std::any Destiny;				//!< Reference to destination variable where to store the parameter's value after editing
		const IndexType DefaultIndex;		//!< Default selection index for combo box-based parameters used in case of a parameter reset
		const bool AllowResetToDefault;		//!< Determines whether to show a reset button in the UI to allow resetting the parameter to its default value
	};

public:
	/**
	 * @brief Constructs a @p ParamsConfigDialog instance.
	 * @param parent QWidget acting as a parent of this modal dialog
	 * @param Core Reference to %DynExp's core
	 * @param Title Title to be displayed in the dialog's title bar
	*/
	ParamsConfigDialog(QWidget* parent, const DynExp::DynExpCore& Core, std::string Title);

	~ParamsConfigDialog() = default;

	/**
	 * @brief Appends a parameter to the configuration dialog.
	 * @param Info Parameter title and description to be displayed in the dialog
	 * @param Destiny Parameter holding the value to be edited. Expects the type @link ParamRefWrapperType @endlink instantiated with @p DestinyType as #NumberType.
	 * @param Value Initial value to be displayed in the configuration dialog for this parameter
	 * @param MinValue Minimal allowed value
	 * @param MaxValue Maximal allowed value
	 * @param Precision Value precision
	 * @param Increment Value increment
	*/
	void AddParam(ParamInfo&& Info, const std::any Destiny, const NumberType Value,
		const NumberType MinValue, const NumberType MaxValue, const NumberType Precision, const NumberType Increment);

	/**
	 * @copybrief AddParam(ParamInfo&&, const std::any, const NumberType, const NumberType, const NumberType, const NumberType, const NumberType)
	 * @param Info Parameter title and description to be displayed in the dialog
	 * @param Destiny Parameter holding the value to be edited. Expects the type @link ParamRefWrapperType @endlink instantiated with @p DestinyType as #TextType.
	 * @param Value Initial value to be displayed in the configuration dialog for this parameter
	 * @param TextUsage Refer to DynExp::TextUsageType.
	*/
	void AddParam(ParamInfo&& Info, const std::any Destiny, const TextType Value, const DynExp::TextUsageType TextUsage);

	/**
	 * @copybrief AddParam(ParamInfo&&, const std::any, const NumberType, const NumberType, const NumberType, const NumberType, const NumberType)
	 * @param Info Parameter title and description to be displayed in the dialog
	 * @param Destiny Parameter holding the value to be edited. Expects the type @link ParamRefWrapperType @endlink instantiated with @p DestinyType as #TextType.
	 * @param Value Initial value to be displayed in the configuration dialog for this parameter. The text must match one of the entries in @p TextList.
	 * If it does not match any entry, the first entry of @p TextList will be selected.
	 * @param TextList List of text entries from which the user can select one
	 * @param AllowResetToDefault Refer to ParamsConfigDialog::Param::AllowResetToDefault.
	 * @throws Util::EmptyException is thrown if @p TextList is empty.
	*/
	void AddParam(ParamInfo&& Info, const std::any Destiny, const TextRefType Value,
		const TextListType& TextList, const bool AllowResetToDefault);

	/**
	 * @copybrief AddParam(ParamInfo&&, const std::any, const NumberType, const NumberType, const NumberType, const NumberType, const NumberType)
	 * @param Info Parameter title and description to be displayed in the dialog
	 * @param Destiny Parameter holding the value to be edited. Expects the type @link ParamRefWrapperType @endlink instantiated with @p DestinyType as #IndexType.
	 * @param Value Index of the entry in @p TextList to select initially
	 * @param DefaultValue Refer to ParamsConfigDialog::Param::DefaultIndex.
	 * @param TextList List of text entries from which the user can select one
	 * @throws Util::EmptyException is thrown if @p TextList is empty.
	 * @throws Util::OutOfRangeException is thrown if @p Value or @p DefaultValue are larger than the amount of entries in @p TextList.
	*/
	void AddParam(ParamInfo&& Info, const std::any Destiny, const TextListIndexType Value,
		const TextListIndexType DefaultValue, const TextListType& TextList);

	/**
	 * @copybrief AddParam(ParamInfo&&, const std::any, const NumberType, const NumberType, const NumberType, const NumberType, const NumberType)
	 * @tparam EnumType Enumeration type which is convertible to a numeric type (only unscoped enumerations)
	 * @param Info Parameter title and description to be displayed in the dialog
	 * @param Destiny Parameter holding the value to be edited. Expects the type @link ParamRefWrapperType @endlink instantiated with @p DestinyType as
	 * DynExp::ParamsBase::EnumParamSignedIntegerType if @p EnumType's underlying type is signed or with @p DestinyType as
	 * DynExp::ParamsBase::EnumParamUnsignedIntegerType if @p EnumType's underlying type is unsigned.
	 * @param Value Initial enum value to be displayed in the configuration dialog for this parameter.
	 * @param DefaultValue DefaultValue Refer to ParamsConfigDialog::Param::DefaultIndex.
	 * @param TextValueList List mapping the enum items' values to text entries
	*/
	template <typename EnumType, std::enable_if_t<std::is_enum_v<EnumType>, int> = 0>
	void AddParam(ParamInfo&& Info, const std::any Destiny, const EnumType Value,
		const EnumType DefaultValue, const Util::TextValueListType<EnumType>& TextValueList);

	/**
	 * @copybrief AddParam(ParamInfo&&, const std::any, const NumberType, const NumberType, const NumberType, const NumberType, const NumberType)
	 * @param Info Parameter title and description to be displayed in the dialog
	 * @param Destiny Parameter holding the value to be edited. Expects the type @link LinkParamRefWrapperType @endlink.
	 * @param Value Index of the entry in @p ItemIDsWithLabels to select initially
	 * @param IsOptional Determines whether this parameter is optional. Optional parameters do not have to point to a valid object ID.
	 * If true, a 'None' entry with value DynExp::ItemIDNotSet is appended to @p ItemIDsWithLabels.
	 * @param IconResourcePath Qt resource path describing an icon being displayed along with this parameter
	 * @param FunctionToCallIfAccepted Refer to #FunctionsToCallIfAccepted.
	 * @param ItemIDsWithLabels List mapping the IDs of selectable DynExp::Object instances to text entries
	*/
	void AddParam(ParamInfo&& Info, const std::any Destiny, const DynExp::ItemIDType Value,
		bool IsOptional, std::string_view IconResourcePath, FunctionsToCallIfAcceptedType FunctionToCallIfAccepted,
		Util::TextValueListType<IndexType>&& ItemIDsWithLabels);

	/**
	 * @copybrief AddParam(ParamInfo&&, const std::any, const NumberType, const NumberType, const NumberType, const NumberType, const NumberType)
	 * @param Info Parameter title and description to be displayed in the dialog
	 * @param Destiny Parameter holding the value to be edited. Expects the type @link LinkListParamRefWrapperType @endlink.
	 * @param Values Indices of the entries in @p ItemIDsWithLabels to select initially
	 * @param IsOptional Determines whether this parameter is optional. Optional parameters do not have to point to valid object IDs.
	 * @param IconResourcePath Qt resource path describing an icon being displayed along with this parameter
	 * @param FunctionToCallIfAccepted Refer to #FunctionsToCallIfAccepted.
	 * @param ItemIDsWithLabels List mapping the IDs of selectable DynExp::Object instances to text entries
	*/
	void AddParam(ParamInfo&& Info, const std::any Destiny, const std::vector<DynExp::ItemIDType>& Values,
		bool IsOptional, std::string_view IconResourcePath, FunctionsToCallIfAcceptedType FunctionToCallIfAccepted,
		Util::TextValueListType<IndexType>&& ItemIDsWithLabels);

	/**
	 * @brief Returns the number of parameters added to the configuration dialog.
	 * @return Size of #ParamList
	*/
	size_t GetNumParams() const noexcept { return ParamList.size(); }

	/**
	 * @brief Determines whether the DynExp::Object instance whose parameters have been edited needs
	 * to be reset to apply the changes.
	 * @return Returns #ResetRequired.
	*/
	bool IsResetRequired() const noexcept { return ResetRequired; }

	/**
	 * @brief Displays the configuration dialog.
	 * @param Object DynExp::Object whose parameters are edited. Refer to ParamsConfigDialog::Object.
	 * @return Returns true if 'OK' has been clicked accepting the dialog. Otherwise, returns false.
	*/
	bool Display(DynExp::Object* Object = nullptr);

private:
	/**
	 * @brief Creates Qt widgets for editing a parameter and inserts them into the configuration dialog.
	 * @param Info Parameter title and description to be displayed in the dialog
	 * @param ParamData Type and properties of the parameter to add controls to the user interface for
	*/
	void InsertWidget(ParamInfo&& Info, Param&& ParamData);

	/**
	 * @brief When the settings dialog is closed by accepting it, assign a single value from the user
	 * interface to the actual parameter retrieved from ParamsConfigDialog::Param::Destiny.
	 * Updates #ResetRequired.
	 * @tparam ParamT Parameter type derived from DynExp::ParamsBase::TypedParamBase
	 * @param Param Parameter to update
	 * @param Value Value to set the parameter to
	*/
	template <typename ParamT>
	void Assign(ParamT& Param, typename ParamT::UnderlyingType Value);

	/**
	 * @brief When the settings dialog is closed by accepting it, assign a list of values from the user
	 * interface to the actual parameter retrieved from ParamsConfigDialog::Param::Destiny.
	 * Updates #ResetRequired.
	 * @tparam ParamT Parameter type derived from DynExp::ParamsBase::TypedParamBase
	 * @param Param Parameter to update
	 * @param Values List of value to set the parameter to
	*/
	template <typename ParamT>
	void Assign(ParamT& Param, const std::vector<typename ParamT::UnderlyingType::value_type>& Values);

	/**
	 * @brief Called when the settings dialog is closed. Removes the parent widget of text editors
	 * listed in #TextEditorDialogs to keep them open after closing this dialog.
	*/
	void HandleTextEditorDialogsOnClose();

	/**
	 * @brief Reference to %DynExp's core
	*/
	const DynExp::DynExpCore& Core;

	/**
	 * @brief List of all parameters assigned to this @p ParamsConfigDialog instance
	*/
	std::vector<Param> ParamList;

	/**
	 * @brief List of functions to be called when acceptig the dialog.
	 * Refer to ParamsConfigDialog::FunctionsToCallIfAcceptedType.
	*/
	std::vector<FunctionsToCallIfAcceptedType> FunctionsToCallIfAccepted;

	/**
	 * @brief DynExp::Object whose parameters are edited. nullptr if a new DynExp::Object is
	 * created and synchronization to achieve thread-safety does not apply.
	*/
	DynExp::Object* Object;

	/**
	 * @brief Becomes true if an existing DynExp::Object is edited and needs to be reset to apply changes
	 * after clicking the dialog's 'accept' button.
	*/
	bool ResetRequired; 

	/**
	 * @brief Keeps a list of text editor dialogs so that their ownership can be removed when the
	 * @p ParamsConfigDialog instance is closed.
	*/
	std::vector<TextEditor*> TextEditorDialogs;

	/**
	 * @brief Bundles Qt widgets of the @p ParamsConfigDialog instance's user interface.
	*/
	Ui::ParamsConfig ui;

private slots:
	void OnOpenParam();					//!< Called when clicking the 'Browse' button for a DynExp::TextUsageType::Path or DynExp::TextUsageType::Code parameter.
	void OnEditParam();					//!< Called when opening a @p TextEditor for a DynExp::TextUsageType::Code parameter.
	void OnResetParam();				//!< Called when resetting a parameter to its default value.
	virtual void accept() override;		//!< Called when the settings dialog is accepted clicking its 'OK' button.
	virtual void reject() override;		//!< Called when the settings dialog is rejected clicking its 'Cancel' button.
};

template <typename EnumType, std::enable_if_t<std::is_enum_v<EnumType>, int>>
void ParamsConfigDialog::AddParam(ParamInfo&& Info, const std::any Destiny, const EnumType Value,
	const EnumType DefaultValue, const Util::TextValueListType<EnumType>& TextValueList)
{
	using IntegerType = std::underlying_type_t<EnumType>;
	constexpr Param::ParamType ParamType = std::is_signed_v<IntegerType> ? Param::ParamType::SignedInteger : Param::ParamType::UnsignedInteger;

	auto ComboBox = new QComboBox(this);
	int SelectedIndex = 0;
	int DefaultIndex = 0;

	for (const auto& TextValuePair : TextValueList)
	{
		if (Value == TextValuePair.second)
			SelectedIndex = ComboBox->count();
		if (DefaultValue == TextValuePair.second)
			DefaultIndex = ComboBox->count();

		ComboBox->addItem(QString::fromStdString(TextValuePair.first), QVariant(static_cast<IntegerType>(TextValuePair.second)));
	}
	ComboBox->setCurrentIndex(SelectedIndex);

	Param ParamData(ParamType, ComboBox, Destiny, DefaultIndex);
	InsertWidget(std::move(Info), std::move(ParamData));
}

template <typename ParamT>
void ParamsConfigDialog::Assign(ParamT& Param, typename ParamT::UnderlyingType Value)
{
	ResetRequired |= Param != Value && Param.GetNeedsResetToApplyChange();
	Param = std::move(Value);
}

template <typename ParamT>
void ParamsConfigDialog::Assign(ParamT& Param, const std::vector<typename ParamT::UnderlyingType::value_type>& Values)
{
	ResetRequired |= Param.GetNeedsResetToApplyChange();
	Param = std::move(Values);
}