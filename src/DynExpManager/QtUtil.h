// This file is part of DynExp.

/**
 * @file QtUtil.h
 * @brief Provides utilities related to Qt and Qt widgets within %DynExp's %Util namespace.
*/

#pragma once

#include "stdafx.h"

namespace DynExp
{
	class DynExpCore;
	class HardwareAdapterBase;
	class QModuleWidget;
}

namespace Util
{
	/**
	 * @brief Returns the default locale properties to be assigned to Qt widgets.
	 * @return QLocale object initialized with %DynExp's default locale settings
	*/
	const QLocale& GetDefaultQtLocale();

	using TextType = std::string;					//!< String type of text-type parameters (DynExp::ParamsBase::Param)
	using TextRefType = std::string_view;			//!< Reference-to-string type of text-type parameters (DynExp::ParamsBase::Param)
	using TextListType = std::vector<TextType>;		//!< List type of text-type parameters
	using TextListIndexType = size_t;				//!< List index type of @p Util::TextListType

	/**
	 * @brief Type of a list containing key-value pairs where key is a text of type @p Util::TextType
	 * @tparam ValueType Value-type assocaited with a text-type key
	*/
	template <typename ValueType>
	using TextValueListType = std::vector<std::pair<TextType, ValueType>>;

	/**
	 * @brief Returns a TextValueListType containing entries which reflect the items (names and values) of an
	 * enumeration registered to Qt's meta-object system.
	 * @tparam EnumType Enumeration type to extract entries from. The enumeration type has to be registered
	 * to Qt's meta-object system with the Q_ENUM macro.
	 * @param SkipEntriesFront Number of enumeration entries to skip from the beginning of the enum declaration
	 * @param SkipEntriesEnd Number of enumeration entries to skip from the end of the enum declaration
	 * @param SkipCharsFront Number of characters to remove from the beginning of each enum entry's name
	 * @param SkipCharsEnd Number of characters to remove from the end of each enum entry's name
	 * @return List containing key-value pairs of @p EnumType's entries with entries skipped according to
	 * @p SkipEntriesFront and @p SkipEntriesEnd ans entry names trimmed according to @p SkipCharsFront and
	 * @p SkipCharsEnd.
	 * @throws Util::OutOfRangeException is thrown when @p SkipEntriesFront and @p SkipEntriesEnd are
	 * incompatible with the actual amount of enum entries in @p EnumType.
	*/
	template <typename EnumType>
	auto QtEnumToTextValueList(unsigned short SkipEntriesFront = 0, unsigned short SkipEntriesEnd = 0,
		unsigned short SkipCharsFront = 0, unsigned short SkipCharsEnd = 0)
	{
		Util::TextValueListType<EnumType> List;
		auto QtEnum = QMetaEnum::fromType<EnumType>();

		if (static_cast<long>(QtEnum.keyCount()) - SkipEntriesFront - SkipEntriesEnd <= 0)
			throw Util::OutOfRangeException("Qt enum does not contain any entries (after subtraction).");

		for (int i = SkipEntriesFront; i < QtEnum.keyCount() - SkipEntriesEnd; ++i)
		{
			std::string ElementTitle(QtEnum.key(i));
			ElementTitle = ElementTitle.substr(SkipCharsFront, ElementTitle.length() - SkipCharsFront - SkipCharsEnd);

			List.emplace_back(ElementTitle.c_str(), static_cast<EnumType>(QtEnum.value(i)));
		}

		return List;
	}

	/** @name DOM functions
	* These functions can be used to walk through an (XML-)DOM tree.
	*/
	///@{
	/**
	 * @brief Finds child nodes with a certain tag name.
	 * @param Parent Parent node whose child nodes are searched
	 * @param ChildTagName Tag name to search for
	 * @return Returns a list of matching child nodes.
	 * @throws Util::InvalidDataException is thrown when @p Parent is a null node.
	*/
	std::vector<QDomNode> GetChildDOMNodes(const QDomElement& Parent, const QString& ChildTagName);
	
	/**
	 * @brief Finds a single child node with a certain tag name.
	 * @param Parent Parent node whose child nodes are searched
	 * @param ChildTagName Tag name to search for
	 * @return Returns the matching child node.
	 * @throws Util::InvalidDataException is thrown when @p Parent is a null node or when multiple
	 * child nodes with matching tag name have been found.
	 * @throws Util::NotFoundException is thrown when not any matching node has been found.
	*/
	QDomNode GetSingleChildDOMNode(const QDomElement& Parent, const QString& ChildTagName);

	/**
	 * @brief Behaves like GetSingleChildDOMNode() but returns the node converted to a DOM element.
	 * @param Parent Refer to GetSingleChildDOMNode().
	 * @param ChildTagName Refer to GetSingleChildDOMNode().
	 * @return Returns the matching child DOM element.
	 * @throws Util::InvalidDataException is additionally thrown if the matching DOM element is null.
	*/
	QDomElement GetSingleChildDOMElement(const QDomElement& Parent, const QString& ChildTagName);

	/**
	 * @brief Behaves like GetSingleChildDOMElement() but returns the text from a DOM element.
	 * @param Parent Refer to GetSingleChildDOMNode().
	 * @param ChildTagName Refer to GetSingleChildDOMNode().
	 * @return Text extracted from a DOM element
	*/
	std::string GetStringFromDOMElement(const QDomElement& Parent, const QString& ChildTagName);

	/**
	 * @brief Behaves like GetSingleChildDOMElement() but returns the content from a DOM element
	 * converted to a certain type using Util::StrToT().
	 * @tparam T Type to convert the DOM element's content to
	 * @param Parent Refer to GetSingleChildDOMNode().
	 * @param ChildTagName Refer to GetSingleChildDOMNode().
	 * @return Value of type @p T extracted from the matching DOM element
	*/
	template <typename T>
	T GetTFromDOMElement(const QDomElement& Parent, const QString& ChildTagName)
	{
		return Util::StrToT<T>(GetStringFromDOMElement(Parent, ChildTagName));
	}

	/**
	 * @overload GetTFromDOMElement
	*/
	template <>
	std::string GetTFromDOMElement(const QDomElement& Parent, const QString& ChildTagName);

	/**
	 * @brief Extracts an attribute from a DOM element
	 * @param Element DOM element containing the attribute
	 * @param AttributeName Name of the attribute to extract
	 * @return Returns the matching attribute.
	 * @throws Util::InvalidDataException is thrown if @p Element is null or if the requested
	 * attribute is invalid or does not exist.
	*/
	QDomAttr GetDOMAttribute(const QDomElement& Element, const QString& AttributeName);

	/**
	 * @brief Behaves like GetDOMAttribute() but returns the text from the attribute.
	 * @param Element Refer to GetDOMAttribute().
	 * @param AttributeName Refer to GetDOMAttribute().
	 * @return Text extracted from the matching attribute
	*/
	std::string GetStringFromDOMAttribute(const QDomElement& Element, const QString& AttributeName);

	/**
	 * @brief Behaves like GetDOMAttribute() but returns the content from an attribute
	 * converted to a certain type using Util::StrToT().
	 * @tparam T Type to convert the attribute's content to
	 * @param Element Refer to GetDOMAttribute().
	 * @param AttributeName Refer to GetDOMAttribute().
	 * @return Value of type @p T extracted from the matching attribute
	*/
	template <typename T>
	T GetTFromDOMAttribute(const QDomElement& Element, const QString& AttributeName)
	{
		return Util::StrToT<T>(GetStringFromDOMAttribute(Element, AttributeName));
	}

	/**
	 * @overload GetTFromDOMAttribute
	*/
	template <>
	std::string GetTFromDOMAttribute(const QDomElement& Element, const QString& AttributeName);
	///@}

	/** @name File open and save dialogs
	*/
	///@{
	/**
	 * @brief Opens a file dialog to ask the user to select a single existing file.
	 * @param Parent Parent widget of this dialog
	 * @param Title Title used for the dialog window
	 * @param DefaultSuffix Default file extension (e.g. ".dynp")
	 * @param NameFilter Filter to select files (e.g. "DynExp project files (*.dynp)")
	 * @param InitialDirectory Initial directory to show in the dile dialog. If empty,
	 * the default directory is specified.
	 * @return Returns the full path to the selected file or an empty string if the user
	 * cancels the dialog.
	*/
	QString PromptOpenFilePath(QWidget* Parent,
		const QString& Title, const QString& DefaultSuffix, const QString& NameFilter, const QString& InitialDirectory = "");

	/**
	 * @brief Works as PromptOpenFilePath() but asks the user to select a single file which does
	 * not need to exist.
	 * @param Parent Refer to PromptOpenFilePath().
	 * @param Title Refer to PromptOpenFilePath().
	 * @param DefaultSuffix Refer to PromptOpenFilePath().
	 * @param NameFilter Refer to PromptOpenFilePath().
	 * @param InitialDirectory Refer to PromptOpenFilePath().
	 * @return Refer to PromptOpenFilePath().
	*/
	QString PromptSaveFilePath(QWidget* Parent,
		const QString& Title, const QString& DefaultSuffix, const QString& NameFilter, const QString& InitialDirectory = "");

	/**
	 * @brief Works as PromptOpenFilePath() but asks the user to select a single file which does
	 * not need to exist. Furthermore, the dialog automatically derives the initial directory using
	 * QModuleWidget::GetDataSaveDirectory() and sets the new global directory for saving data to the
	 * folder containing the selected file when the user accepts the dialog.
	 * @param Parent Refer to PromptOpenFilePath().
	 * @param Title Refer to PromptOpenFilePath().
	 * @param DefaultSuffix Refer to PromptOpenFilePath().
	 * @param NameFilter Refer to PromptOpenFilePath().
	 * @return Refer to PromptOpenFilePath().
	*/
	QString PromptSaveFilePathModule(DynExp::QModuleWidget* Parent,
		const QString& Title, const QString& DefaultSuffix, const QString& NameFilter);
	///@}

	/** @name Image manipulation
	*/
	///@{
	/**
	 * @brief Converts raw pixel data stored in a Util::BlobDataType object to a QImage
	 * transfering the ownership of @p BlobData's content.
	 * @param BlobData Binary large object containing pixel data
	 * @param Width Width of the image represented by @p BlobData in pixels
	 * @param Height Height of the image represented by @p BlobData in pixels
	 * @param BytesPerLine Number of bytes which represent a single row of pixels
	 * @param Format Format of the pixel data stored in @p BlobData.
	 * @return Returns the QImage constructed from raw pixel data.
	 * @throws Util::InvalidArgException is thrown if any of @p Width, @p Height, or @p BytesPerLine
	 * is 0 or if @p Format is @p QImage::Format::Format_Invalid.
	*/
	QImage QImageFromBlobData(BlobDataType&& BlobData, int Width, int Height, int BytesPerLine, QImage::Format Format);

	// Calculates histograms from Image for RGB/intensity channels. Each array in ImageRGBHistogramType conatins values
	// assigned to bins given by the element indices. The three arrays contain histograms for RGB channels in this order.

	/**
	 * @brief Alias which represents a histogram as a std::array with 256 numeric bins. The lowest (highest) index
	 * represents the darkest (brightest) intensity.
	*/
	using ImageHistogramType = std::array<unsigned long long, 256>;

	/**
	 * @brief Alias which represents a RGB histogram as a std::tuple of three @p ImageHistogramType elements.
	 * The first tuple element represents the histogram for red, the second for green, the third for blue.
	*/
	using ImageRGBHistogramType = std::tuple<ImageHistogramType, ImageHistogramType, ImageHistogramType>;

	/**
	 * @brief Computes an intensity (grayscale) histogram from a QImage object.
	 * @param Image Image to compute histogram from
	 * @return Returns the histogram as an std::array.
	*/
	ImageHistogramType ComputeIntensityHistogram(const QImage& Image);

	/**
	 * @brief Computes a RGB histogram from a QImage object.
	 * @param Image Image to compute histogram from
	 * @return Returns the RGB histogram as a std::tuple of three intensity histograms.
	*/
	ImageRGBHistogramType ComputeRGBHistogram(const QImage& Image);

	/**
	 * @brief Computes an intensity (grayscale) histogram from a RGB histogram.
	 * @param RGBHistogram RGB histogram to compute the grayscale histogram from
	 * @return Returns the grayscale histogram as an std::array.
	*/
	ImageHistogramType ConvertRGBToIntensityHistogram(const ImageRGBHistogramType& RGBHistogram);

	/**
	 * @brief Returns a QPolygonF representing a cross-style marker
	 * @param Center Center coordinate of the marker
	 * @param ArmLength Length of one of the four arms of the cross.
	 * @return Polygon representing the marker
	*/
	QPolygonF MakeCrossPolygon(QPointF Center, unsigned int ArmLength);
	///@}

	/**
	 * @brief Renders a window active and brings it to the front.
	 * @param Widget representing a Qt (dialog) window
	*/
	void ActivateWindow(QWidget& Widget);

	/** @name Reading and writing files
	*/
	///@{
	/**
	 * @brief Saves a std::string_view to a file (using QFile).
	 * Creates a new file or truncates an existing file's content.
	 * @param Filename Path of the file to write to.
	 * @param Text Content to be written.
	 * @return Returns true in case of success, otherwise false.
	*/
	bool SaveToFile(const QString& Filename, std::string_view Text);

	/**
	 * @brief Reads the entire content from a text file.
	 * @param Filename Full filename of the file to read from
	 * @return Content read from the file
	 * @throws Util::FileIOErrorException is thrown if the specified file cannot be opened.
	*/
	std::string ReadFromFile(const QString& Filename);

	/**
	 * @copydoc ReadFromFile
	*/
	std::string ReadFromFile(const std::string& Filename);

	/**
	 * @copydoc ReadFromFile
	*/
	std::string ReadFromFile(const std::filesystem::path& Filename);
	///@}

	/**
	 * @brief Implements a QObject belonging to a hardware adapter (derived from DynExp::HardwareAdapterBase)
	 * that operates in DynExpCore's worker thread, not in the main user interface thread. This is useful to
	 * move e.g. ethernet/serial communication operations from the user interface thread to a worker thread.
	 * After construction, QWorker instances have to be moved to the worker thread calling
	 * QWorker::MoveToWorkerThread().
	*/
	class QWorker : public QObject
	{
		Q_OBJECT

		friend class DynExp::DynExpCore;

	protected:
		QWorker() = default;
		~QWorker() = default;

	public:
		/**
		 * @brief Moves the instance to DynExpCore's worker thread.
		 * Do not call from constructor since derived classes have not been instantiated yet in that case.
		 * This would prevent QObjects contained in derived classes to be also moved to the worker thread.
		 * @param ID ID of the hardware adapter owning this worker instance
		*/
		void MoveToWorkerThread(DynExp::ItemIDType ID);

		/**
		 * @brief Returns this worker instance's owner. 
		 * @return Pointer to owning hardware adapter
		*/
		auto GetOwner() const noexcept { return Owner.lock(); }

	private:
		using OwnerPtrType = std::weak_ptr<const DynExp::HardwareAdapterBase>;

		/**
		 * @brief Returns the application's DynExp::DynExpCore instance which is globally set in
		 * constructor DynExp::DynExpCore::DynExpCore()
		 * @param DynExpCore Used by DynExp::DynExpCore::DynExpCore(). Do not pass anything.
		 * @return Reference to the application's DynExp::DynExpCore instance
		 * @throws Util::InvalidArgException is thrown if this function is called without letting
		 * DynExp::DynExpCore::DynExpCore() perform the initialization.
		*/
		static const DynExp::DynExpCore& GetDynExpCore(const DynExp::DynExpCore* DynExpCore = nullptr);

		/**
		 * @brief Sets the hardware adapter owning this worker instance.
		 * @param Owner Pointer to the hardware adapter owning this instance
		*/
		void SetOwner(OwnerPtrType Owner) noexcept { this->Owner = Owner; }

		bool HasBeenMovedToWorkerThread = false;	//!< Indicates whether the worker has already been moved to the worker thread.
		OwnerPtrType Owner;							//!< Pointer to the hardware adapter owning this instance.
	};

	/**
	 * @brief Implements a QGraphicsView the user can interact with to insert graphical markers.
	 * Furthermore, the graphics view enables zooming into its content.
	*/
	class MarkerGraphicsView : public QGraphicsView
	{
		Q_OBJECT

		static constexpr double ZoomFactor = 1.6;				//!< Determines the magnification of one zoom step. 
		static constexpr double DeselectedMarkerOpacity = .5;	//!< Determines the opacity of a marker which is not selected.
		static constexpr double SelectedMarkerOpacity = 1;		//!< Determines the opacity of a selected marker.

	public:
		/**
		 * @brief Data associated with one marker
		*/
		struct MarkerType
		{
			using IDType = signed long long;

			/**
			 * @brief Constructs a marker assigning the properties passed as arguments. @p Name will stay empty.
			 * @param Marker Refer to MarkerType::Marker.
			 * @param MarkerPos Refer to MarkerType::MarkerPos.
			 * @param IsUserDeletable Refer to MarkerType::UserDeletable.
			 * @param ID Refer to MarkerType::ID.
			 * @param ImagePos Refer to MarkerType::ImagePos.
			*/
			constexpr MarkerType(QGraphicsPolygonItem* Marker, const QPoint& MarkerPos, bool IsUserDeletable = true, IDType ID = -1, const QPointF& ImagePos = {}) noexcept
				: Marker(Marker), MarkerPos(MarkerPos), UserDeletable(IsUserDeletable), ID(ID), ImagePos(ImagePos) {}

			/** @name Getters
			*/
			///@{
			constexpr auto GetMarker() noexcept { return Marker; }
			constexpr const auto GetMarker() const noexcept { return Marker; }
			constexpr const auto& GetMarkerPos() const noexcept { return MarkerPos; }
			constexpr bool IsUserDeletable() const noexcept { return UserDeletable; }
			constexpr auto GetID() const noexcept { return ID; }
			constexpr std::string_view GetName() const noexcept { return Name; }
			constexpr const auto& GetImagePos() const noexcept { return ImagePos; }
			///@}

			/** @name Setters
			*/
			///@{
			void SetName(std::string_view NewName);
			///@}

		private:
			/** @name Non-const properties
			 * Not const to ensure move-assignability
			*/
			///@{
			QGraphicsPolygonItem* Marker;						//!< Qt polygon object to draw the marker onto the graphics view
			QPoint MarkerPos;									//!< Position of the marker within the graphics view in pixels
			bool UserDeletable;									//!< Determines whether the user can interact with this marker to delete it.
			IDType ID;											//!< ID of the marker to identify it amongst other markers in the same graphics view
			std::string Name;									//!< Name of the marker to describe it
			QPointF ImagePos;									//!< Position in nm (e.g. of a sample under a microscope) where the associated image the marker belongs to was recorded.
			///@}
		};

		/**
		 * @brief Constructs a MarkerGraphicsView.
		 * @param parent Parent Qt object owning this item delegate
		*/
		MarkerGraphicsView(QWidget* parent);

		/** @name Getters
		 */
		///@{
		auto contextMenu() const noexcept { return ContextMenu; }
		bool HaveMarkersChanged() noexcept;		//!< Returns whether a marker operation has changed the stored markers. Resets the flag.
		const std::vector<MarkerType>& GetMarkers() const noexcept { return Markers; }
		const auto& GetCurrentImagePos() const noexcept { return CurrentImagePos; }
		///@}

		/** @name Setters
		 */
		///@{
		void SetCurrentImagePos(const QPointF& Pos) { CurrentImagePos = Pos; }
		///@}

		/** @name Marker operations
		 */
		///@{
		/**
		 * @brief Adds a marker to the graphics view at position @p MarkerPos assigning the properties passed as arguments.
		 * Affects @p MarkersChanged.
		 * @param MarkerPos Refer to MarkerType::MarkerPos.
		 * @param Color Color of the marker when displayed in the graphics view
		 * @param IsUserDeletable Refer to MarkerType::UserDeletable.
		 * @param ID Refer to MarkerType::ID. It is the caller's responsibility to pass a unique ID. The function does not perform a check.
		 * @param Name Refer to MarkerType::Name.
		*/
		void AddMarker(const QPoint& MarkerPos, const QColor& Color, bool IsUserDeletable = true, MarkerType::IDType ID = -1, std::string Name = {});

		/**
		 * @brief Removes the n-th marker specified by @p Index. Affects @p MarkersChanged.
		 * @param Index Index of the marker to remove
		 * @param OnlyUserDeletableMarkers If true, the marker is also removed if it is not user-deletable. If false,
		 * non-user-deletable markers are not removed.
		 * @throws OutOfRangeException is thrown if Index exceeds the amount of markers stored in the graphics view.
		*/
		void RemoveMarker(size_t Index, bool OnlyUserDeletableMarkers = false);

		/**
		 * @brief Removes markers at the position @p MarkerPos. Affects @p MarkersChanged.
		 * @param MarkerPos Refer to MarkerType::MarkerPos.
		 * @param OnlyUserDeletableMarkers If true, markers are also removed if they are not user-deletable. If false,
		 * non-user-deletable markers are not removed.
		*/
		void RemoveMarker(const QPoint& MarkerPos, bool OnlyUserDeletableMarkers = false);

		/**
		 * @brief Removes markers with name @p Name. Affects @p MarkersChanged.
		 * @param Name Refer to MarkerType::Name.
		 * @param OnlyUserDeletableMarkers If true, markers are also removed if they are not user-deletable. If false,
		 * non-user-deletable markers are not removed.
		*/
		void RemoveMarker(std::string_view Name, bool OnlyUserDeletableMarkers = false);

		/**
		 * @brief Removes all markers from the graphics view. Affects @p MarkersChanged.
		 * @param OnlyUserDeletableMarkers If true, markers are also removed if they are not user-deletable. If false,
		 * non-user-deletable markers are not removed.
		*/
		void RemoveMarkers(bool OnlyUserDeletableMarkers);

		/**
		 * @brief Hides or shows all markers.
		 * @param MarkersHidden If true, all markers are hidden. If false, all markers become visible.
		*/
		void setMarkersHidden(bool MarkersHidden);

		/**
		 * @brief Assigns a name to the marker at position @p MarkerPos. Affects @p MarkersChanged.
		 * @param MarkerPos Refer to MarkerType::MarkerPos.
		 * @param NewName Name to assign
		*/
		void RenameMarker(const QPoint& MarkerPos, std::string_view NewName);

		/**
		 * @brief Selects the marker at position @p MarkerPos.
		 * @param MarkerPos Refer to MarkerType::MarkerPos.
		*/
		void SelectMarker(const QPoint& MarkerPos);

		/**
		 * @brief Deselects all selected markers.
		*/
		void DeselectMarkers();
		///@}

		/** @name Zoom operations
		 */
		///@{
		void ZoomIn();						//!< Zooms in one step.
		void ZoomOut();						//!< Zooms out one step.
		void ZoomReset();					//!< Resets the zoom.
		///@}

		void EnableActions(bool Enable);

	protected:
		virtual void mousePressEvent(QMouseEvent* Event) override;			//!< Adds a marker to the mouse pointer position assigning the highest ID amongst all markers in the graphics view incremented by 1 as the ID.
		virtual void mouseDoubleClickEvent(QMouseEvent* Event) override;	//!< Removes the marker at the mouse pointer position if it is user-deletable. Affects @p MarkersChanged.
		virtual void wheelEvent(QWheelEvent* Event) override;				//!< Zooms in or out when Control is pressed on the keyboard at the same time.

	private:
		std::vector<MarkerType> Markers;	//!< List of the markers
		bool MarkersHidden;					//!< Determines whether the markers are currently displayed or not.
		bool MarkersChanged;				//!< Holds whether the markers have changed by any of the marker operations.
		QPointF CurrentImagePos;			//!< Sample position where the image has been recorded (in nm). Refer to MarkerType::ImagePos.

		/** @name Context menu and associated actions
		 */
		///@{
		QMenu* ContextMenu;
		QAction* EditMarkersAction;
		QAction* ShowMarkersAction;
		QAction* RemoveMarkersAction;
		QAction* SaveMarkersAction;
		///@}

	signals:
		void mouseClickEvent(QPoint Position);

	private slots:
		void OnContextMenuRequested(QPoint Position);	//!< Shows the context menu at position @p Position.
		void OnShowMarkers(bool Checked);				//!< If @p Checked is true, all markers are made visible, otherwise they become hidden.
		void OnRemoveMarkers(bool);						//!< Removes all user-deletable markers. Affects @p MarkersChanged.
		void OnSaveMarkers(bool);						//!< Asks the user for a file name and saves all markers to a CSV file.
	};

	/**
	 * @brief Implements a QListWidget that sorts its items after inserting an item by drag & drop.
	*/
	class QSortingListWidget : public QListWidget
	{
		Q_OBJECT

	public:
		QSortingListWidget(QWidget* parent = nullptr) : QListWidget(parent) {}
		~QSortingListWidget() = default;

	private:
		virtual void dropEvent(QDropEvent* event) override;
	};

	/**
	 * @brief Implements a QTableWidgetItem which contains numeric content such that table widget items can be
	 * numerically compared with each other.
	*/
	class NumericSortingTableWidgetItem : public QTableWidgetItem
	{
	public:
		/** @name Constructors
		 * These constructors only forward parameters to the respective QTableWidgetItem constructor with the
		 * same signature.
		*/
		///@{
		NumericSortingTableWidgetItem() {}
		NumericSortingTableWidgetItem(const QString& text) : QTableWidgetItem(text) {}
		NumericSortingTableWidgetItem(const QIcon& icon, const QString& text) : QTableWidgetItem(icon, text) {}
		///@}

		/**
		 * @brief Compares the content of this table widget item with the content of another table widget item
		 * by converting the items' texts into numeric values.
		 * @param Other Table widget item to compare this item with
		 * @return Returns true if the numeric content of this item is smaller than the numeric content of @p Other,
		 * false otherwise.
		*/
		bool operator<(const QTableWidgetItem& Other) const;

		virtual QTableWidgetItem* clone() const override;
	};

	/**
	 * @brief Implements a QItemDelegate which forces e.g. a QTableWidgetItem's content to be numeric (double-precision).
	*/
	class NumericOnlyItemDelegate : public QItemDelegate
	{
		Q_OBJECT

	public:
		/**
		 * @brief Constructs a NumericOnlyItemDelegate and sets constraints on the numeric values it considers valid.
		 * @param parent Parent Qt object owning this item delegate
		 * @param min Minimal allowed numeric value
		 * @param max Maximal allowed numeric value
		 * @param precision Precision of the numeric value which is accepted when editing the item
		*/
		NumericOnlyItemDelegate(QObject* parent = nullptr, double min = std::numeric_limits<double>::lowest(), double max = std::numeric_limits<double>::max(), int precision = -1)
			: QItemDelegate(parent), min(min), max(max), precision(precision) {}

		virtual QWidget* createEditor(QWidget* parent, const QStyleOptionViewItem& option, const QModelIndex& index) const override;

	private:
		const double min;
		const double max;
		const int precision;
	};

	/**
	 * @brief Implements a QItemDelegate which forces e.g. a QTableWidgetItem's content to be boolean (0 or 1).
	*/
	class DigitalOnlyItemDelegate : public NumericOnlyItemDelegate
	{
		Q_OBJECT

	public:
		DigitalOnlyItemDelegate(QObject* parent = nullptr) : NumericOnlyItemDelegate(parent, 0, 1, 0) {}
	};
}

Q_DECLARE_METATYPE(Util::MarkerGraphicsView::MarkerType::IDType);