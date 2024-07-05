// This file is part of DynExp.

#include "stdafx.h"
#include "moc_QtUtil.cpp"
#include "QtUtil.h"

// Placed here to avoid circular dependencies
#include "DynExpCore.h"

namespace Util
{
	const QLocale& GetDefaultQtLocale()
	{
		static QLocale Locale(QLocale::Language::English, QLocale::Country::UnitedStates);

		return Locale;
	}

	std::vector<QDomNode> GetChildDOMNodes(const QDomElement& Parent, const QString& ChildTagName)
	{
		if (Parent.isNull())
			throw Util::InvalidDataException("Error parsing XML tree. The given parent node is invalid.");

		// Must be filtered in the following since it also contains nodes which are down deeper in the
		// DOM hierarchy (not only direct childs).
		auto ChildNodeList = Parent.elementsByTagName(ChildTagName);

		std::vector<QDomNode> Nodes;
		for (int i = 0; i < ChildNodeList.length(); ++i)
			if (ChildNodeList.at(i).parentNode() == Parent)
				Nodes.push_back(ChildNodeList.at(i));

		return Nodes;
	}

	QDomNode GetSingleChildDOMNode(const QDomElement& Parent, const QString& ChildTagName)
	{
		if (Parent.isNull())
			throw Util::InvalidDataException("Error parsing XML tree. The given parent node is invalid.");

		auto ChildNodeList = Parent.elementsByTagName(ChildTagName);
		int FoundDirectChildIndex = -1;
		for (int i = 0; i < ChildNodeList.length(); ++i)
		{
			if (ChildNodeList.at(i).parentNode() == Parent)
			{
				if (FoundDirectChildIndex < 0)
					FoundDirectChildIndex = i;
				else
					throw Util::InvalidDataException(
						"Error parsing XML tree. Expecting exactly one node <" + ChildTagName.toStdString() + ">.");
			}
		}

		if (FoundDirectChildIndex < 0)
			throw Util::NotFoundException(
				"Node <" + ChildTagName.toStdString() + "> has not been found in XML tree.");

		return ChildNodeList.item(FoundDirectChildIndex);
	}

	QDomElement GetSingleChildDOMElement(const QDomElement& Parent, const QString& ChildTagName)
	{
		auto DOMElement = GetSingleChildDOMNode(Parent, ChildTagName).toElement();

		if (DOMElement.isNull())
			throw Util::InvalidDataException(
				"Error parsing XML tree. The node <" + ChildTagName.toStdString() + "> is not a DOM element.");

		return DOMElement;
	}

	std::string GetStringFromDOMElement(const QDomElement& Parent, const QString& ChildTagName)
	{
		return GetSingleChildDOMElement(Parent, ChildTagName).text().toStdString();
	}

	/**
	 * @internal 
	*/
	template <>
	std::string GetTFromDOMElement(const QDomElement& Parent, const QString& ChildTagName)
	{
		return GetStringFromDOMElement(Parent, ChildTagName);
	}

	QDomAttr GetDOMAttribute(const QDomElement& Element, const QString& AttributeName)
	{
		if (Element.isNull())
			throw Util::InvalidDataException("Error parsing XML tree. The given node is invalid.");

		auto Attr = Element.attributes().namedItem(AttributeName).toAttr();
		if (Attr.isNull())
			throw Util::InvalidDataException("Error parsing XML tree. A node contains invalid attributes.");

		return Attr;
	}

	std::string GetStringFromDOMAttribute(const QDomElement& Element, const QString& AttributeName)
	{
		return GetDOMAttribute(Element, AttributeName).value().toStdString();
	}

	/**
	 * @internal
	*/
	template <>
	std::string GetTFromDOMAttribute(const QDomElement& Element, const QString& AttributeName)
	{
		return GetStringFromDOMAttribute(Element, AttributeName);
	}

	QString PromptOpenFilePath(QWidget* Parent,
		const QString& Title, const QString& DefaultSuffix, const QString& NameFilter, const QString& InitialDirectory)
	{
		QFileDialog FileDialog(Parent, Title);
		FileDialog.setAcceptMode(QFileDialog::AcceptMode::AcceptOpen);
		FileDialog.setFilter(QDir::Filter::Files);
		FileDialog.setFileMode(QFileDialog::FileMode::ExistingFile);
		FileDialog.setDefaultSuffix(DefaultSuffix);
		FileDialog.setNameFilter(NameFilter);
		if (!InitialDirectory.isEmpty())
			FileDialog.setDirectory(InitialDirectory);

		return FileDialog.exec() == QDialog::Accepted ? *FileDialog.selectedFiles().begin() : "";
	}

	QString PromptSaveFilePath(QWidget* Parent,
		const QString& Title, const QString& DefaultSuffix, const QString& NameFilter, const QString& InitialDirectory)
	{
		QFileDialog FileDialog(Parent, Title);
		FileDialog.setAcceptMode(QFileDialog::AcceptMode::AcceptSave);
		FileDialog.setFilter(QDir::Filter::Files);
		FileDialog.setFileMode(QFileDialog::FileMode::AnyFile);
		FileDialog.setDefaultSuffix(DefaultSuffix);
		FileDialog.setNameFilter(NameFilter);
		if (!InitialDirectory.isEmpty())
			FileDialog.setDirectory(InitialDirectory);

		return FileDialog.exec() == QDialog::Accepted ? *FileDialog.selectedFiles().begin() : "";
	}

	QString PromptSaveFilePathModule(DynExp::QModuleWidget* Parent, const QString& Title, const QString& DefaultSuffix, const QString& NameFilter)
	{
		const auto FilePath = PromptSaveFilePath(Parent, Title, DefaultSuffix, NameFilter, QString::fromStdString(Parent->GetDataSaveDirectory()));

		if (!FilePath.isEmpty())
			Parent->SetDataSaveDirectory(FilePath.toStdString());

		return FilePath;
	}

	QImage QImageFromBlobData(BlobDataType&& BlobData, int Width, int Height, int BytesPerLine, QImage::Format Format)
	{
		if (!Width || !Height || !BytesPerLine || Format == QImage::Format::Format_Invalid)
			throw InvalidArgException(
				"Width, Height and BytesPerLine cannot be zero, Format must describe a valid image format.");

		if (!BlobData.GetPtr())
			return {};

		// Probably not exception-safe if QImage constructor throws.
		auto BufferPtr = BlobData.Release();
		return QImage(BufferPtr, Width, Height, BytesPerLine, Format, [](void* Info) {
			// Ugly due to an ugly QImage implementation...
			auto CastInfo = static_cast<decltype(BufferPtr)>(Info);
			delete[] CastInfo;
			}, BufferPtr);
	}

	ImageHistogramType ComputeIntensityHistogram(const QImage& Image)
	{
		auto IntensityImage = Image.convertToFormat(QImage::Format_Grayscale8);
		const unsigned char* DataPtr = IntensityImage.constBits();

		// Initialize everything to 0 by {}.
		ImageHistogramType Histogram{};

		for (auto i = IntensityImage.height() * IntensityImage.width(); i > 0; --i)
			++Histogram[*DataPtr++];

		// Histogram moved by copy elision.
		return Histogram;
	}

	ImageRGBHistogramType ComputeRGBHistogram(const QImage& Image)
	{
		auto RGBImage = Image.convertToFormat(QImage::Format_RGB888);
		const unsigned char* DataPtr = RGBImage.constBits();

		// Initialize everything to 0 by {}.
		ImageHistogramType HistogramR{}, HistogramG{}, HistogramB{};

		for (auto i = RGBImage.height() * RGBImage.width(); i > 0; --i)
		{
			++HistogramR[*DataPtr++];
			++HistogramG[*DataPtr++];
			++HistogramB[*DataPtr++];
		}

		return { std::move(HistogramR), std::move(HistogramG), std::move(HistogramB) };
	}

	ImageHistogramType ConvertRGBToIntensityHistogram(const ImageRGBHistogramType& RGBHistogram)
	{
		// Initialize everything to 0 by {}.
		ImageHistogramType IntensityHistogram{};

		for (int i = 0; i < 256; ++i)
			IntensityHistogram[i] = (std::get<0>(RGBHistogram)[i] + std::get<1>(RGBHistogram)[i] + std::get<2>(RGBHistogram)[i]) / 3.f;

		// IntensityHistogram moved by copy elision.
		return IntensityHistogram;
	}

	QPolygonF MakeCrossPolygon(QPointF Center, unsigned int ArmLength)
	{
		return QPolygonF({
			Center,
			QPointF(Center.x() - ArmLength, Center.y()),
			QPointF(Center.x() + ArmLength, Center.y()),
			Center,
			QPointF(Center.x(), Center.y() - ArmLength),
			QPointF(Center.x(), Center.y() + ArmLength),
			Center,
		});
	}

	void ActivateWindow(QWidget& Widget)
	{
		Widget.setWindowState((Widget.windowState() & ~Qt::WindowMinimized) | Qt::WindowActive);
		Widget.raise();				// for MacOS
		Widget.activateWindow();	// for Windows
	}

	bool SaveToFile(const QString& Filename, std::string_view Text)
	{
		QFile File(Filename);

		// QIODevice::WriteOnly implies QIODeviceBase::Truncate.
		if (!File.open(QIODevice::WriteOnly | QIODevice::Text))
			return false;

		const auto BytesWriten = File.write(Text.data(), Text.size());
		File.close();

		return BytesWriten >= 0;
	}

	std::string ReadFromFile(const QString& Filename)
	{
		QFile File(Filename);
		if (!File.open(QIODevice::ReadOnly | QIODevice::Text))
			throw FileIOErrorException(Filename.toStdString());

		auto Content = File.readAll();
		File.close();

		return Content.toStdString();
	}

	std::string ReadFromFile(const std::string& Filename)
	{
		return ReadFromFile(QString::fromStdString(Filename));
	}

	std::string ReadFromFile(const std::filesystem::path& Filename)
	{
		return ReadFromFile(Filename.string());
	}

	void QWorker::MoveToWorkerThread(DynExp::ItemIDType ID)
	{
		if (HasBeenMovedToWorkerThread)
			return;

		GetDynExpCore().MoveQWorkerToWorkerThread(*this, ID);
		HasBeenMovedToWorkerThread = true;
	}

	const DynExp::DynExpCore& QWorker::GetDynExpCore(const DynExp::DynExpCore* DynExpCore)
	{
		static const DynExp::DynExpCore* const Core = DynExpCore;

		if (!Core)
			throw Util::InvalidArgException("Core cannot be nullptr.");

		return *Core;
	}

	void MarkerGraphicsView::MarkerType::SetName(std::string_view NewName)
	{
		Name = NewName;

		if (Marker)
			Marker->setToolTip(QString::fromStdString(Name));
	}

	MarkerGraphicsView::MarkerGraphicsView(QWidget* parent)
		: QGraphicsView(parent), MarkersHidden(false), MarkersChanged(true),
		ContextMenu(new QMenu(this)), EditMarkersAction(nullptr), ShowMarkersAction(nullptr), RemoveMarkersAction(nullptr)
	{
		EditMarkersAction = ContextMenu->addAction("&Edit Markers");
		EditMarkersAction->setCheckable(true);
		EditMarkersAction->setChecked(false);
		ShowMarkersAction = ContextMenu->addAction("S&how Markers", this, &MarkerGraphicsView::OnShowMarkers, QKeySequence(Qt::Key_NumberSign));
		addAction(ShowMarkersAction);	// for shortcuts
		ShowMarkersAction->setCheckable(true);
		ShowMarkersAction->setChecked(true);
		RemoveMarkersAction = ContextMenu->addAction(QIcon(DynExpUI::Icons::Delete), "&Remove all Markers", this, &MarkerGraphicsView::OnRemoveMarkers);
		SaveMarkersAction = ContextMenu->addAction(QIcon(DynExpUI::Icons::Save), "Sa&ve Markers to File", this, &MarkerGraphicsView::OnSaveMarkers);

		setContextMenuPolicy(Qt::CustomContextMenu);
		connect(this, &MarkerGraphicsView::customContextMenuRequested, this, &MarkerGraphicsView::OnContextMenuRequested);
	}

	void MarkerGraphicsView::mousePressEvent(QMouseEvent* Event)
	{
		if (!scene())
			return;

		auto LocalPoint = mapFromGlobal(Event->globalPos());
		const auto MarkerPos = mapToScene(LocalPoint).toPoint();

		if (!MarkersHidden && EditMarkersAction->isChecked() && Event->button() == Qt::MouseButton::LeftButton
			&& !items(LocalPoint).empty())	// Allow markers only on top of the displayed content - not on empty space.
		{
			auto HighestIDMarker = std::max_element(Markers.cbegin(), Markers.cend(), [](const auto& a, const auto& b) {
				return a.GetID() < b.GetID();
			});
			auto NewID = HighestIDMarker != Markers.cend() && HighestIDMarker->GetID() >= 0 ? HighestIDMarker->GetID() + 1 : 0;

			AddMarker(MarkerPos, QColorConstants::Magenta, true, NewID);
		}

		if (!EditMarkersAction->isChecked() && Event->button() == Qt::MouseButton::LeftButton && !items(LocalPoint).empty())
			emit mouseClickEvent(MarkerPos);
	}

	void MarkerGraphicsView::mouseDoubleClickEvent(QMouseEvent* Event)
	{
		if (!scene() || MarkersHidden)
			return;

		for (auto it = Markers.begin(); it != Markers.end();)
			if (it->GetMarker()->isUnderMouse() && it->IsUserDeletable())
			{
				if (EditMarkersAction->isChecked())
				{
						scene()->removeItem(it->GetMarker());
						it = Markers.erase(it);

						MarkersChanged = true;
				}
				else
				{
					bool OKClicked = false;
					QString Name = QInputDialog::getText(this, "Edit name",
						QString("Enter name of marker at position (") + QString::number(it->GetMarkerPos().x())
						+ QString(", ") + QString::number(it->GetMarkerPos().y()) + QString("):"),
						QLineEdit::Normal, it->GetName().data(), &OKClicked);

					if (OKClicked)
					{
						it->SetName(Name.toStdString());

						MarkersChanged = true;
					}

					return;
				}
			}
			else
				++it;
	}

	void MarkerGraphicsView::wheelEvent(QWheelEvent* Event)
	{
		if (Event->modifiers().testFlag(Qt::KeyboardModifier::ControlModifier))
		{
			if (Event->angleDelta().y() < 0)
				ZoomOut();
			if (Event->angleDelta().y() > 0)
				ZoomIn();
		}
		else
			QGraphicsView::wheelEvent(Event);
	}

	bool MarkerGraphicsView::HaveMarkersChanged() noexcept
	{
		bool Changed = MarkersChanged;
		MarkersChanged = false;

		return Changed;
	}

	void MarkerGraphicsView::AddMarker(const QPoint& MarkerPos, const QColor& Color, bool IsUserDeletable, MarkerType::IDType ID, std::string Name)
	{
		if (!scene())
			return;

		float ScaleFactor = std::min(scene()->height(), scene()->width()) / 300;
		QPolygonF MarkerPolygon = Util::MakeCrossPolygon(MarkerPos, 5 * ScaleFactor);
		Markers.emplace_back(scene()->addPolygon(MarkerPolygon, QPen(Color, ScaleFactor)), MarkerPos, IsUserDeletable, ID, CurrentImagePos);
		Markers.back().GetMarker()->setOpacity(DeselectedMarkerOpacity);
		Markers.back().GetMarker()->setVisible(!MarkersHidden);
		Markers.back().SetName(Name);

		MarkersChanged = true;
	}

	void MarkerGraphicsView::RemoveMarker(size_t Index, bool OnlyUserDeletableMarkers)
	{
		if (!scene())
			return;
		if (Index >= Markers.size())
			throw OutOfRangeException("The given marker index exceeds the amount of markers stored in this MarkerGraphicsView.");

		auto Marker = Markers.cbegin() + Index;
		if (!OnlyUserDeletableMarkers || Marker->IsUserDeletable())
		{
			scene()->removeItem(Marker->GetMarker());
			Markers.erase(Marker);

			MarkersChanged = true;
		}
	}

	void MarkerGraphicsView::RemoveMarker(const QPoint& MarkerPos, bool OnlyUserDeletableMarkers)
	{
		if (!scene())
			return;

		for (auto it = Markers.cbegin(); it != Markers.cend();)
			if (it->GetMarkerPos() == MarkerPos && (!OnlyUserDeletableMarkers || it->IsUserDeletable()))
			{
				scene()->removeItem(it->GetMarker());
				it = Markers.erase(it);

				MarkersChanged = true;
			}
			else
				++it;
	}

	void MarkerGraphicsView::RemoveMarker(std::string_view Name, bool OnlyUserDeletableMarkers)
	{
		if (!scene())
			return;

		for (auto it = Markers.cbegin(); it != Markers.cend();)
			if (it->GetName() == Name && (!OnlyUserDeletableMarkers || it->IsUserDeletable()))
			{
				scene()->removeItem(it->GetMarker());
				it = Markers.erase(it);

				MarkersChanged = true;
			}
			else
				++it;
	}

	void MarkerGraphicsView::RemoveMarkers(bool OnlyUserDeletableMarkers)
	{
		if (!scene())
			return;

		for (auto it = Markers.cbegin(); it != Markers.cend();)
			if (!OnlyUserDeletableMarkers || it->IsUserDeletable())
			{
				scene()->removeItem(it->GetMarker());
				it = Markers.erase(it);

				MarkersChanged = true;
			}
			else
				++it;
	}

	void MarkerGraphicsView::setMarkersHidden(bool MarkersHidden)
	{
		this->MarkersHidden = MarkersHidden;

		for (auto it = Markers.cbegin(); it != Markers.cend(); ++it)
			it->GetMarker()->setVisible(!MarkersHidden);
	}

	void MarkerGraphicsView::RenameMarker(const QPoint& MarkerPos, std::string_view NewName)
	{
		for (auto& Marker : Markers)
			if (Marker.GetMarkerPos() == MarkerPos)
			{
				Marker.SetName(NewName);

				MarkersChanged = true;
			}
	}

	void MarkerGraphicsView::SelectMarker(const QPoint& MarkerPos)
	{
		for (auto& Marker : Markers)
			Marker.GetMarker()->setOpacity(Marker.GetMarkerPos() == MarkerPos ? SelectedMarkerOpacity : DeselectedMarkerOpacity);
	}

	void MarkerGraphicsView::DeselectMarkers()
	{
		for (auto& Marker : Markers)
			Marker.GetMarker()->setOpacity(DeselectedMarkerOpacity);
	}

	void MarkerGraphicsView::ZoomIn()
	{
		scale(ZoomFactor, ZoomFactor);
	}

	void MarkerGraphicsView::ZoomOut()
	{
		scale(1 / ZoomFactor, 1 / ZoomFactor);
	}

	void MarkerGraphicsView::ZoomReset()
	{
		resetTransform();
	}

	void MarkerGraphicsView::EnableActions(bool Enable)
	{
		// Except ShowMarkersAction since this action only affects how the markers are displayed.
		EditMarkersAction->setEnabled(Enable);
		RemoveMarkersAction->setEnabled(Enable);
		SaveMarkersAction->setEnabled(Enable);
	}

	void MarkerGraphicsView::OnContextMenuRequested(QPoint Position)
	{
		ContextMenu->exec(mapToGlobal(Position));
	}

	void MarkerGraphicsView::OnShowMarkers(bool Checked)
	{
		setMarkersHidden(!Checked);
	}

	void MarkerGraphicsView::OnRemoveMarkers(bool)
	{
		RemoveMarkers(true);
	}

	void MarkerGraphicsView::OnSaveMarkers(bool)
	{
		if (Markers.empty())
		{
			QMessageBox::warning(this, "DynExp - Error", "There are not any markers set.");
			return;
		}

		auto Filename = Util::PromptSaveFilePath(this, "Save markers to file", ".csv", " Comma-separated values file (*.csv)");
		if (Filename.isEmpty())
			return;

		std::stringstream CSVData;
		CSVData << std::setprecision(9) << "ID;X(px);Y(px);ImagePosX(nm);ImagePosY(nm);Name\n";

		for (const auto& Marker : Markers)
			CSVData << Marker.GetID() << ";" << Marker.GetMarkerPos().x() << ";" << Marker.GetMarkerPos().y() << ";"
				<< Marker.GetImagePos().x() << ";" << Marker.GetImagePos().y() << ";"
				<< Marker.GetName() << "\n";

		if (!Util::SaveToFile(Filename, CSVData.str()))
			QMessageBox::warning(this, "DynExp - Error", "Error writing data to file.");
	}

	void QSortingListWidget::dropEvent(QDropEvent* event)
	{
		QListWidget::dropEvent(event);

		sortItems();
	}

	bool NumericSortingTableWidgetItem::operator<(const QTableWidgetItem& Other) const
	{
		return GetDefaultQtLocale().toDouble(text()) < GetDefaultQtLocale().toDouble(Other.text());
	}

	QTableWidgetItem* NumericSortingTableWidgetItem::clone() const
	{
		return new NumericSortingTableWidgetItem(*this);
	}

	QWidget* NumericOnlyItemDelegate::createEditor(QWidget* parent, const QStyleOptionViewItem& option, const QModelIndex& index) const
	{
		auto LineEdit = new QLineEdit(parent);
		auto Validator = new QDoubleValidator(min, max, precision, LineEdit);
		Validator->setLocale(GetDefaultQtLocale());
		LineEdit->setValidator(Validator);

		return LineEdit;
	}
}