// This file is part of DynExp.

#include "stdafx.h"
#include "moc_CircuitDiagram.cpp"
#include "CircuitDiagram.h"
#include "DynExpCore.h"
#include "HardwareAdapters/HardwareAdapterEthernet.h"

double CircuitDiagram::GraphType::EvaluateEnergy() const
{
	const auto EnergyFunc = [](const NodeListType& Nodes, const NodeListType& LinkedNodes) {
		double Energy = 0.0;

		// Potentially slow algorithm, but since we are dealing with small graphs, this should be fine.
		size_t CurrentNodeIndex = 0;
		std::unordered_map<size_t, size_t> Links;
		for (size_t i = 0; i < Nodes.size(); ++i)
			for (size_t j = 0; j < Nodes[i]->LinkedParams.size(); ++j)
				for (size_t k = 0; k < Nodes[i]->LinkedParams[j].LinkedItems.size(); ++k)
				{
					const auto LinkedItem = Nodes[i]->LinkedParams[j].LinkedItems[k].Item;
					if (!LinkedItem)
						continue;

					// Ignore links skipping a layer. Such links should hardly occur in DynExp.
					auto LinkedIt = std::find(LinkedNodes.cbegin(), LinkedNodes.cend(), LinkedItem);
					if (LinkedIt == LinkedNodes.cend())
						continue;
					
					// Build a simplified map of all links. Ignore to which nodes they belong to.
					Links[CurrentNodeIndex++] = LinkedIt - LinkedNodes.cbegin();

					// Honor short links.
					double LinkedIndex = static_cast<double>(LinkedIt - LinkedNodes.cbegin());
					double CurrentIndex = static_cast<double>(i);
					Energy += std::abs(LinkedIndex - CurrentIndex);
				}

		// Add huge penalty for each intersecting link.
		for (const auto& A : Links)
			for (const auto& B : Links)
				if (B.first > A.first && A.second > B.second)
					Energy += 10.0;

		return Energy;
	};

	auto Energy = EnergyFunc(Instruments, HardwareAdapters) + EnergyFunc(Modules, Instruments);

	// Normalize to keep energy range in the same range regardless of the graph's size.
	Energy /= static_cast<double>(HardwareAdapters.size()) + static_cast<double>(Instruments.size()) + static_cast<double>(Modules.size());

	return Energy;
}

double CircuitDiagram::GraphType::DistanceTo(const GraphType& Other) const
{
	// Currently, different samples differ by maximally one permutation.
	return *this == Other ? 0.0 : 1.0;
}

bool CircuitDiagram::GraphType::operator==(const GraphType& Other) const
{
	return HardwareAdapters == Other.HardwareAdapters &&
		Instruments == Other.Instruments &&
		Modules == Other.Modules;
}

void CircuitDiagram::CircuitDiagramItem::InsertLinks(const DynExp::ParamsBase::ObjectLinkParamsType& ObjectLinkParams,
	const DynExp::DynExpCore& DynExpCore, NodeMapType* HardwareAdapterNodes, NodeMapType* InstrumentNodes)
{
	for (auto& LinkedParam : ObjectLinkParams)
	{
		const auto LinkedIDs = LinkedParam.get().GetLinkedIDs();
		decltype(LinkedParamType::LinkedItems) Items;

		if (HardwareAdapterNodes && LinkedParam.get().GetCommonManager() == &DynExpCore.GetHardwareAdapterManager())
		{
			// Inserts an empty (default-constructed) node if ID does not exist in HardwareAdapterNodes.
			std::transform(LinkedIDs.cbegin(), LinkedIDs.cend(), std::inserter(Items, Items.begin()),
				[HardwareAdapterNodes](const auto ID) { return &(*HardwareAdapterNodes)[ID]; });
		}
		else if (InstrumentNodes && LinkedParam.get().GetCommonManager() == &DynExpCore.GetInstrumentManager())
		{
			// Inserts an empty (default-constructed) node if ID does not exist in InstrumentNodes.
			std::transform(LinkedIDs.cbegin(), LinkedIDs.cend(), std::inserter(Items, Items.begin()),
				[&InstrumentNodes](const auto ID) { return &(*InstrumentNodes)[ID]; });
		}
		
		if (Items.empty())
			Items.emplace_back(nullptr);

		LinkedParams.emplace_back(LinkedParam.get().GetLinkTitle(), std::move(Items));
	}
}

size_t CircuitDiagram::CircuitDiagramItem::GetNumLinkedItems()
{
	return std::accumulate(LinkedParams.cbegin(), LinkedParams.cend(), static_cast<size_t>(0), [](size_t Count, const LinkedParamType& Param) {
		return Count + Param.LinkedItems.size();
	});
}

const QColor CircuitDiagram::SocketOuterColor = QColor("darkturquoise");
const QColor CircuitDiagram::SocketInnerColor = QColor("turquoise");

CircuitDiagram::CircuitDiagram(QWidget *parent)
	: QDialog(parent, Qt::WindowTitleHint | Qt::WindowMinMaxButtonsHint | Qt::WindowCloseButtonHint),
	SelectionChanged(false), SelectedTreeWidgetItem(nullptr), ContextMenu(new QMenu(this))
{
	ui.setupUi(this);

	ContextMenu->addAction(ui.action_Zoom_in);
	ContextMenu->addAction(ui.action_Zoom_out);
	ContextMenu->addAction(ui.action_Zoom_reset);
	ContextMenu->addSeparator();
	ContextMenu->addAction(ui.action_Save_Image);

	// For shortcuts
	addAction(ui.action_Zoom_in);
	addAction(ui.action_Zoom_out);
	addAction(ui.action_Zoom_reset);
	addAction(ui.action_Save_Image);
}

CircuitDiagram::~CircuitDiagram()
{
}

QTreeWidgetItem* CircuitDiagram::GetSelectedEntry()
{
	auto OldSelectionChanged = SelectionChanged;
	SelectionChanged = false;

	return OldSelectionChanged ? SelectedTreeWidgetItem : nullptr;
}

bool CircuitDiagram::Redraw(const DynExp::DynExpCore& DynExpCore)
{
	try
	{
		BuildTree(DynExpCore);
		ArrangeTree();
		Render();
	}
	catch (...)
	{
		Clear();

		return false;
	}

	return true;
}

bool CircuitDiagram::UpdateStates(const DynExp::DynExpCore& DynExpCore)
{
	if (!Scene)
		return true;

	try
	{
		for (auto& Item : HardwareAdapterNodes)
			UpdateItem(Item.second);
		for (auto& Item : InstrumentNodes)
			UpdateItem(Item.second);
		for (auto& Item : ModuleNodes)
			UpdateItem(Item.second);
	}
	catch (...)
	{
		return false;
	}

	return true;
}

void CircuitDiagram::mouseDoubleClickEvent(QMouseEvent* Event)
{
	QGraphicsItem* Item = ui.GVCircuit->itemAt(Event->pos());

	if (Item && Item->data(Qt::ItemDataRole::UserRole).canConvert<decltype(CircuitDiagramItem::TreeWidgetItem)>())
	{
		SelectionChanged = true;
		SelectedTreeWidgetItem = Item->data(Qt::ItemDataRole::UserRole).value<decltype(CircuitDiagramItem::TreeWidgetItem)>();
	}
}

void CircuitDiagram::wheelEvent(QWheelEvent* Event)
{
	if (Event->modifiers().testFlag(Qt::KeyboardModifier::ControlModifier))
	{
		if (Event->angleDelta().y() < 0)
			ZoomOut();
		if (Event->angleDelta().y() > 0)
			ZoomIn();
	}
	else
		QDialog::wheelEvent(Event);
}

QLinearGradient CircuitDiagram::GetGrayLinearGradient()
{
	QLinearGradient Gradient(0, 0, 0, 1);	// gradient along y-direction
	Gradient.setCoordinateMode(QGradient::ObjectMode);
	Gradient.setColorAt(0, Qt::lightGray);
	Gradient.setColorAt(.3, Qt::darkGray);
	Gradient.setColorAt(1, Qt::darkGray);

	return Gradient;
}

void CircuitDiagram::Clear()
{
	Scene.reset();

	// Clear tree.
	HardwareAdapterNodes.clear();
	InstrumentNodes.clear();
	ModuleNodes.clear();
}

void CircuitDiagram::BuildTree(const DynExp::DynExpCore& DynExpCore)
{
	Clear();

	// Collect resources and links in between resources.
	for (auto Res = DynExpCore.GetHardwareAdapterManager().cbegin(); Res != DynExpCore.GetHardwareAdapterManager().cend(); ++Res)
		HardwareAdapterNodes.emplace(Res->first, CircuitDiagramItem(Res->second.TreeWidgetItem.get()));
	for (auto Res = DynExpCore.GetInstrumentManager().cbegin(); Res != DynExpCore.GetInstrumentManager().cend(); ++Res)
	{
		auto Node = InstrumentNodes.emplace(Res->first, CircuitDiagramItem(Res->second.TreeWidgetItem.get()));
		const auto Params = Res->second.ResourcePointer->GetParams();

		Node.first->second.InsertLinks(Params->GetObjectLinkParams(), DynExpCore, &HardwareAdapterNodes, nullptr);

		auto NetworkAddressPtr = Params->GetNetworkAddressParams();
		if (NetworkAddressPtr)
			Node.first->second.NetworkAddress = QString::fromStdString(NetworkAddressPtr->MakeAddress());
	}
	for (auto Res = DynExpCore.GetModuleManager().cbegin(); Res != DynExpCore.GetModuleManager().cend(); ++Res)
	{
		auto Node = ModuleNodes.emplace(Res->first, CircuitDiagramItem(Res->second.TreeWidgetItem.get()));
		const auto Params = Res->second.ResourcePointer->GetParams();

		Node.first->second.InsertLinks(Params->GetObjectLinkParams(), DynExpCore, &HardwareAdapterNodes, &InstrumentNodes);
		
		auto NetworkAddressPtr = Params->GetNetworkAddressParams();
		if (NetworkAddressPtr)
			Node.first->second.NetworkAddress = QString::fromStdString(NetworkAddressPtr->MakeAddress());
	}
}

void CircuitDiagram::ArrangeTree()
{
	// Sort by item name in lexicographical order.
	GraphType Graph;

	const auto ValuePtrGetter = [](auto& Pair) { return &Pair.second; };
	std::transform(HardwareAdapterNodes.begin(), HardwareAdapterNodes.end(), std::back_inserter(Graph.HardwareAdapters), ValuePtrGetter);
	std::transform(InstrumentNodes.begin(), InstrumentNodes.end(), std::back_inserter(Graph.Instruments), ValuePtrGetter);
	std::transform(ModuleNodes.begin(), ModuleNodes.end(), std::back_inserter(Graph.Modules), ValuePtrGetter);

	const auto CircuitDiagramItemSorter = [](const CircuitDiagramItem* a, const CircuitDiagramItem* b) {
		return a && b && !a->Empty && !b->Empty?
			QString::localeAwareCompare(a->TreeWidgetItem->text(TreeWidgetItemNameColumn), b->TreeWidgetItem->text(TreeWidgetItemNameColumn)) < 0 :
			false;
	};
	std::sort(Graph.HardwareAdapters.begin(), Graph.HardwareAdapters.end(), CircuitDiagramItemSorter);
	std::sort(Graph.Instruments.begin(), Graph.Instruments.end(), CircuitDiagramItemSorter);
	std::sort(Graph.Modules.begin(), Graph.Modules.end(), CircuitDiagramItemSorter);

	// Swap single items to simplify tree and to avoid intersecting links
	RefineBySimulatedAnnealing(Graph);

	// Determine items' real coordinates for painting.
	const auto PositionCalculator = [](CircuitDiagramItem* Item, const qreal x, qreal& y) {
		Item->ParamSectionHeight = Item->GetNumLinkedItems() * AdditionalHeightPerParam + Item->LinkedParams.size() * ParamSep;
		Item->ItemHeight = InnerStartHeight + InnerMargin + InnerMarginBottom + Item->ParamSectionHeight;
		Item->TopLeftPos = { x, y };

		y += Item->ItemHeight + NodeVSep;
	};

	qreal HardwareAdapterCurrentY = 0;
	for (const auto Item : Graph.HardwareAdapters)
		PositionCalculator(Item, 0, HardwareAdapterCurrentY);
	qreal InstrumentCurrentY = 0;
	for (const auto Item : Graph.Instruments)
		PositionCalculator(Item, InnerWidth + NodeHSep, InstrumentCurrentY);
	qreal ModuleCurrentY = 0;
	for (const auto Item : Graph.Modules)
		PositionCalculator(Item, 2 * (InnerWidth + NodeHSep), ModuleCurrentY);

	// Center vertically.
	if (HardwareAdapterCurrentY > InstrumentCurrentY && HardwareAdapterCurrentY > ModuleCurrentY)
	{
		for (const auto Item : Graph.Instruments)
			Item->TopLeftPos += { 0, (HardwareAdapterCurrentY - InstrumentCurrentY) / 2 };
		for (const auto Item : Graph.Modules)
			Item->TopLeftPos += { 0, (HardwareAdapterCurrentY - ModuleCurrentY) / 2 };
	}
	else if (InstrumentCurrentY > HardwareAdapterCurrentY && InstrumentCurrentY > ModuleCurrentY)
	{
		for (const auto Item : Graph.HardwareAdapters)
			Item->TopLeftPos += { 0, (InstrumentCurrentY - HardwareAdapterCurrentY) / 2 };
		for (const auto Item : Graph.Modules)
			Item->TopLeftPos += { 0, (InstrumentCurrentY - ModuleCurrentY) / 2 };
	}
	else
	{
		for (const auto Item : Graph.HardwareAdapters)
			Item->TopLeftPos += { 0, (ModuleCurrentY - HardwareAdapterCurrentY) / 2 };
		for (const auto Item : Graph.Instruments)
			Item->TopLeftPos += { 0, (ModuleCurrentY - InstrumentCurrentY) / 2 };
	}
}

void CircuitDiagram::RefineBySimulatedAnnealing(GraphType& Graph)
{
	auto Rnd = std::unique_ptr<gsl_rng, decltype([](gsl_rng* p) { gsl_rng_free(p); })>(gsl_rng_alloc(gsl_rng_mt19937));

	// Ensure to always obtain the same stream of random numbers (see GSL documentation of gsl_rng_set()).
	gsl_rng_set(Rnd.get(), 1);

	gsl_siman_params_t SimAnParams;
	SimAnParams.n_tries = 1;
	SimAnParams.iters_fixed_T = 3;
	SimAnParams.step_size = 1.0;
	SimAnParams.k = 1.0;
	SimAnParams.t_initial = 10;
	SimAnParams.mu_t = 1.05;
	SimAnParams.t_min = 1.0e-6;

	const auto StepFunc = [](const gsl_rng* Rnd, void* Element, double StepSize) {
		auto Graph = static_cast<GraphType*>(Element);
		NodeListType* NodeListToModify = nullptr;

		// Choose a random vector to modify.
		const auto NodeListToModifyIndex = gsl_rng_uniform_int(Rnd, 3);
		switch (NodeListToModifyIndex)
		{
		case 0:
			NodeListToModify = &Graph->HardwareAdapters;
			break;
		case 1:
			NodeListToModify = &Graph->Instruments;
			break;
		default:
			NodeListToModify = &Graph->Modules;
		}

		if (NodeListToModify->size() < 2)
			return;

		// Choose two random elements to swap.
		const auto MaxIndexPlusOne = static_cast<unsigned long>(std::min(static_cast<size_t>(std::numeric_limits<unsigned long>::max()), NodeListToModify->size()));
		const auto SrcIndex = gsl_rng_uniform_int(Rnd, MaxIndexPlusOne);
		const auto DestIndex = gsl_rng_uniform_int(Rnd, MaxIndexPlusOne);

		if (SrcIndex != DestIndex)
		{
			auto OldNode = NodeListToModify->at(DestIndex);
			NodeListToModify->at(DestIndex) = NodeListToModify->at(SrcIndex);
			NodeListToModify->at(SrcIndex) = OldNode;
		}
	};

	GraphType* StartSample = new GraphType(Graph);
	try
	{
		gsl_siman_solve(Rnd.get(), static_cast<void*>(StartSample),
			[](void* Element) { return static_cast<GraphType*>(Element)->EvaluateEnergy(); },
			StepFunc,
			[](void* a, void* b) { return static_cast<GraphType*>(a)->DistanceTo(*static_cast<GraphType*>(b)); },
			nullptr,
			[](void* Source, void* Dest) { *static_cast<GraphType*>(Dest) = *static_cast<GraphType*>(Source); },
			[](void* Source) { return static_cast<void*>(new GraphType(*static_cast<GraphType*>(Source))); },
			[](void* Element) { delete static_cast<GraphType*>(Element); },
			0, SimAnParams);

		Graph = *StartSample;
	}
	catch (...)
	{
		delete StartSample;

		throw;
	}
	
	delete StartSample;
}

void CircuitDiagram::Render()
{
	Scene = std::make_unique<QGraphicsScene>();

	// Render items.
	for (auto& Item : HardwareAdapterNodes)
		RenderItem(Item.second, true);
	for (auto& Item : InstrumentNodes)
		RenderItem(Item.second, true);
	for (auto& Item : ModuleNodes)
		RenderItem(Item.second, false);

	// Render links (from destiny to origin).
	for (auto& Item : InstrumentNodes)
		RenderLinks(Item.second);
	for (auto& Item : ModuleNodes)
		RenderLinks(Item.second);

	Scene->setSceneRect(Scene->itemsBoundingRect());
	ui.GVCircuit->setScene(Scene.get());
}

void CircuitDiagram::RenderItem(CircuitDiagramItem& Item, bool DrawOutputSocket)
{
	bool Valid = !Item.Empty && Item.TreeWidgetItem && Item.TreeWidgetItem->parent();

	// Inner and outer rects
	QPainterPath Path;
	if (Valid)
	{
		Path.addRoundedRect(Item.TopLeftPos.x(), Item.TopLeftPos.y(),
			InnerWidth + 2 * InnerMargin, Item.ItemHeight,
			CornerRoundingRadius, CornerRoundingRadius);
		Item.StateFrame = Scene->addPath(Path, { Item.TreeWidgetItem->foreground(TreeWidgetItemStateColumn), OuterPenLineWidth },
			Qt::BrushStyle::NoBrush);
		Item.StateFrame->setToolTip(Item.TreeWidgetItem->toolTip(TreeWidgetItemStateColumn));
		Item.StateFrame->setData(Qt::ItemDataRole::UserRole, QVariant::fromValue(Item.TreeWidgetItem));
		Path.clear();
	}
	Path.addRoundedRect(Item.TopLeftPos.x() + InnerMargin, Item.TopLeftPos.y() + InnerMargin,
		InnerWidth, InnerStartHeight + Item.ParamSectionHeight,
		CornerRoundingRadius, CornerRoundingRadius);
	if (Valid)
		Item.ItemFrame = Scene->addPath(Path, { Qt::gray, InnerPenLineWidth }, GetGrayLinearGradient());
	else
		Item.ItemFrame = Scene->addPath(Path, { Qt::red, OuterPenLineWidth }, Qt::BrushStyle::NoBrush);
	if (Valid)
	{
		Item.ItemFrame->setToolTip(Item.TreeWidgetItem->parent()->text(TreeWidgetItemTypeColumn));
		Item.ItemFrame->setData(Qt::ItemDataRole::UserRole, QVariant::fromValue(Item.TreeWidgetItem));
	}

	// State
	if (Valid)
	{
		Item.StateIcon = Item.TreeWidgetItem->icon(TreeWidgetItemStateColumn);
		Item.StatePixmap = Scene->addPixmap(Item.StateIcon.pixmap(TransformIconSize(StateIconSize)));
		Item.StatePixmap->setFlag(QGraphicsItem::ItemIgnoresTransformations);
		Item.StatePixmap->setPos(Item.StateFrame->boundingRect().bottomLeft() + QPointF(InnerMargin, -InnerMarginBottom));
		Item.StatePixmap->setToolTip(Item.TreeWidgetItem->toolTip(TreeWidgetItemStateColumn));
		Item.StatePixmap->setData(Qt::ItemDataRole::UserRole, QVariant::fromValue(Item.TreeWidgetItem));
		Item.StateLabel = Scene->addSimpleText(Item.TreeWidgetItem->text(TreeWidgetItemStateColumn));
		Item.StateLabel->setBrush(Item.TreeWidgetItem->foreground(TreeWidgetItemStateColumn));
		Item.StateLabel->setPos(Item.StateFrame->boundingRect().bottomLeft() + QPointF(InnerMargin + StateIconSize + IconMargin, -InnerMarginBottom));
		Item.StateLabel->setToolTip(Item.TreeWidgetItem->toolTip(TreeWidgetItemStateColumn));
		Item.StateLabel->setData(Qt::ItemDataRole::UserRole, QVariant::fromValue(Item.TreeWidgetItem));
	}

	// Item name and icon
	if (Valid)
		Item.ItemIcon = Item.TreeWidgetItem->parent()->icon(TreeWidgetParentTypeColumn);
	else
		Item.ItemIcon = QIcon(DynExpUI::Icons::Delete);
	Item.ItemPixmap = Scene->addPixmap(Item.ItemIcon.pixmap(TransformIconSize(TypeIconSize)));
	Item.ItemPixmap->setFlag(QGraphicsItem::ItemIgnoresTransformations);
	Item.ItemPixmap->setPos(Item.ItemFrame->boundingRect().topLeft() +
		QPointF((Item.ItemFrame->boundingRect().width() - TypeIconSize) / 2, InnerStartHeight / 3 + IconMargin));
	if (Valid)
	{
		Item.ItemPixmap->setToolTip(Item.TreeWidgetItem->parent()->text(TreeWidgetItemTypeColumn));
		Item.ItemPixmap->setData(Qt::ItemDataRole::UserRole, QVariant::fromValue(Item.TreeWidgetItem));
		Item.ItemLabel = Scene->addSimpleText(Item.TreeWidgetItem->text(TreeWidgetItemNameColumn));
	}
	else
	{
		Item.ItemLabel = Scene->addSimpleText("Deleted item");
		Item.ItemLabel->setBrush(Qt::red);
	}
	auto ItemLabelFont = Item.ItemLabel->font();
	ItemLabelFont.setBold(true);
	Item.ItemLabel->setFont(ItemLabelFont);
	while (Item.ItemLabel->boundingRect().width() > InnerWidth - InnerMargin)
		Item.ItemLabel->setText(Item.ItemLabel->text().remove(Item.ItemLabel->text().length() - 6, 6) + "...");
	Item.ItemLabel->setPos(Item.ItemFrame->boundingRect().topLeft() + QPointF(InnerMargin, InnerMargin / 2));
	if (Valid)
	{
		Item.ItemLabel->setToolTip(Item.TreeWidgetItem->toolTip(TreeWidgetItemTypeColumn));
		Item.ItemLabel->setData(Qt::ItemDataRole::UserRole, QVariant::fromValue(Item.TreeWidgetItem));
	}

	// Output socket
	if (DrawOutputSocket)
	{
		Item.OutputSocket = Scene->addEllipse(Item.ItemFrame->boundingRect().right() - 4 * InnerPenLineWidth,
			Item.ItemFrame->boundingRect().topRight().y() + (Item.ItemFrame->boundingRect().height() - SocketDiameter) / 6.0 *
			(Valid && !Item.NetworkAddress.isEmpty() ? 4.0 : 3.0),
			SocketDiameter, SocketDiameter, { Valid ? SocketOuterColor : Qt::darkRed, SocketPenLineWidth }, Valid ? SocketInnerColor : Qt::red);
		Item.OutputSocket->setZValue(-1);
	}

	// Parameters and links between items
	if (Valid)
	{
		qreal CurrentY = Item.ItemLabel->pos().y() + Item.ItemLabel->boundingRect().height() + TypeIconSize + InnerMarginTop;
		for (auto& LinkedParam : Item.LinkedParams)
		{
			CircuitDiagramItem::ParamGraphicsItemsType ParamGraphicsItem;

			QPainterPath Path;
			Path.addPolygon(QPolygonF({
				{ Item.ItemFrame->boundingRect().left() + InnerMargin, CurrentY},
				{ Item.ItemFrame->boundingRect().left() + InnerMargin, CurrentY + LinkedParam.LinkedItems.size() * AdditionalHeightPerParam}
			}));
			ParamGraphicsItem.Frame = Scene->addPath(Path, { SocketOuterColor, OuterPenLineWidth / 2 }, Qt::BrushStyle::NoBrush);

			ParamGraphicsItem.Label = Scene->addSimpleText(LinkedParam.LinkTitle.data());
			ParamGraphicsItem.Label->setBrush(SocketOuterColor);
			while (ParamGraphicsItem.Label->boundingRect().width() > InnerWidth - InnerMargin - IconMargin)
				ParamGraphicsItem.Label->setText(ParamGraphicsItem.Label->text().remove(ParamGraphicsItem.Label->text().length() - 6, 6) + "...");
			ParamGraphicsItem.Label->setPos(ParamGraphicsItem.Frame->boundingRect().center() + QPointF(IconMargin, 0));
			ParamGraphicsItem.Label->setPos(ParamGraphicsItem.Label->pos() - QPointF(0, ParamGraphicsItem.Label->boundingRect().height() / 2));
			ParamGraphicsItem.Label->setToolTip(LinkedParam.LinkTitle.data());
			ParamGraphicsItem.Label->setData(Qt::ItemDataRole::UserRole, QVariant::fromValue(Item.TreeWidgetItem));

			for (size_t LinkIndex = 0; LinkIndex < LinkedParam.LinkedItems.size(); ++LinkIndex)
			{
				LinkedParam.LinkedItems[LinkIndex].SocketIndex = ParamGraphicsItem.Sockets.size();
				ParamGraphicsItem.Sockets.push_back(Scene->addEllipse(Item.ItemFrame->boundingRect().left() - 2 * InnerPenLineWidth,
					CurrentY + AdditionalHeightPerParam * LinkIndex + (AdditionalHeightPerParam - SocketDiameter) / 2,
					SocketDiameter, SocketDiameter, { SocketOuterColor, SocketPenLineWidth }, SocketInnerColor));
				ParamGraphicsItem.Sockets.back()->setToolTip(ParamGraphicsItem.Label->text());
				ParamGraphicsItem.Sockets.back()->setData(Qt::ItemDataRole::UserRole, QVariant::fromValue(Item.TreeWidgetItem));
				ParamGraphicsItem.Sockets.back()->setZValue(-1);
			}

			LinkedParam.ParamGraphicsItemIndex = Item.ParamGraphicsItems.size();
			Item.ParamGraphicsItems.push_back(std::move(ParamGraphicsItem));

			CurrentY += LinkedParam.LinkedItems.size() * AdditionalHeightPerParam + ParamSep;
		}
	}

	// Network address
	if (Valid && !Item.NetworkAddress.isEmpty())
	{
		Item.NetworkIcon = QIcon(DynExpUI::Icons::Network);
		Item.NetworkPixmap = Scene->addPixmap(Item.NetworkIcon.pixmap(TransformIconSize(TypeIconSize)));
		Item.NetworkPixmap->setFlag(QGraphicsItem::ItemIgnoresTransformations);
		Item.NetworkPixmap->setPos(Item.ItemFrame->boundingRect().topRight() + QPointF(NetworkIconDistance, 0));
		Item.NetworkPixmap->setToolTip(Item.NetworkAddress);
		Item.NetworkPixmap->setData(Qt::ItemDataRole::UserRole, QVariant::fromValue(Item.TreeWidgetItem));

		QPainterPath Path;
		Path.moveTo(Item.ItemFrame->boundingRect().topRight() + QPointF(0, TypeIconSize / 2));
		Path.lineTo(Item.ItemFrame->boundingRect().topRight() + QPointF(NetworkIconDistance, TypeIconSize / 2));
		QPen Pen(DynExpUI::DarkPalette::blue, OuterPenLineWidth, Qt::CustomDashLine, Qt::FlatCap);
		Pen.setDashPattern({ 1.5, 1, 1.5, 1, 1.5, 1, 1.5, 1, 1.5, 1 });
		Pen.setDashOffset(1);
		Item.NetworkLink = Scene->addPath(Path, Pen, Qt::BrushStyle::NoBrush);
		Item.NetworkLink->setToolTip(Item.NetworkAddress);
		Item.NetworkLink->setData(Qt::ItemDataRole::UserRole, QVariant::fromValue(Item.TreeWidgetItem));
		Item.NetworkLink->setZValue(-2);
	}
}

void CircuitDiagram::RenderLinks(CircuitDiagramItem& Item)
{
	if (Item.Empty || !Item.TreeWidgetItem || !Item.TreeWidgetItem->parent())
		return;

	for (const auto& LinkedParam : Item.LinkedParams)
	{
		for (auto LinkedItem : LinkedParam.LinkedItems)
		{
			if (!LinkedItem.Item || !LinkedItem.Item->OutputSocket)
				continue;

			auto From = Item.ParamGraphicsItems[LinkedParam.ParamGraphicsItemIndex].Sockets[LinkedItem.SocketIndex]->boundingRect().center();
			auto To = LinkedItem.Item->OutputSocket->boundingRect().center();

			QPainterPath Path;
			Path.moveTo(From);
			Path.cubicTo(
				{ From.x() + (To.x() - From.x()) / 4 , From.y() },
				{ From.x() + (To.x() - From.x()) / 4 * 3, To.y() }, To);

			Item.ParamGraphicsItems[LinkedParam.ParamGraphicsItemIndex].Links.push_back(Scene->addPath(Path,
				{ !LinkedItem.Item->Empty ? SocketInnerColor : Qt::red, OuterPenLineWidth }, Qt::BrushStyle::NoBrush));
			Item.ParamGraphicsItems[LinkedParam.ParamGraphicsItemIndex].Links.back()->setZValue(-2);
		}
	}
}

void CircuitDiagram::UpdateItem(CircuitDiagramItem& Item)
{
	if (!Item.Empty && Item.TreeWidgetItem && Item.TreeWidgetItem->parent() &&
		Item.StateFrame && Item.StatePixmap && Item.StateLabel)
	{
		// Outer rect
		auto StateFramePen = Item.StateFrame->pen();
		StateFramePen.setBrush(Item.TreeWidgetItem->foreground(TreeWidgetItemStateColumn));
		Item.StateFrame->setPen(StateFramePen);
		Item.StateFrame->setToolTip(Item.TreeWidgetItem->toolTip(TreeWidgetItemStateColumn));

		// State
		Item.StateIcon = Item.TreeWidgetItem->icon(TreeWidgetItemStateColumn);
		Item.StatePixmap->setPixmap(Item.StateIcon.pixmap(TransformIconSize(StateIconSize)));
		Item.StatePixmap->setToolTip(Item.TreeWidgetItem->toolTip(TreeWidgetItemStateColumn));
		Item.StateLabel->setText(Item.TreeWidgetItem->text(TreeWidgetItemStateColumn));
		Item.StateLabel->setBrush(Item.TreeWidgetItem->foreground(TreeWidgetItemStateColumn));
		Item.StateLabel->setToolTip(Item.TreeWidgetItem->toolTip(TreeWidgetItemStateColumn));
	}
}

int CircuitDiagram::TransformIconSize(int Size) const
{
	return ui.GVCircuit->transform().map(QPoint(Size, Size)).x();
}

void CircuitDiagram::RescaleIcons()
{
	const auto RescaleFunc = [this](NodeMapType::value_type& Item) {
		if (Item.second.StatePixmap && !Item.second.StateIcon.isNull())
			Item.second.StatePixmap->setPixmap(Item.second.StateIcon.pixmap(TransformIconSize(StateIconSize)));
		if (Item.second.ItemPixmap && !Item.second.ItemIcon.isNull())
			Item.second.ItemPixmap->setPixmap(Item.second.ItemIcon.pixmap(TransformIconSize(TypeIconSize)));
		if (Item.second.NetworkPixmap && !Item.second.NetworkIcon.isNull())
			Item.second.NetworkPixmap->setPixmap(Item.second.NetworkIcon.pixmap(TransformIconSize(TypeIconSize)));
	};

	std::for_each(HardwareAdapterNodes.begin(), HardwareAdapterNodes.end(), RescaleFunc);
	std::for_each(InstrumentNodes.begin(), InstrumentNodes.end(), RescaleFunc);
	std::for_each(ModuleNodes.begin(), ModuleNodes.end(), RescaleFunc);
}

void CircuitDiagram::ZoomIn()
{
	ui.GVCircuit->scale(ZoomFactor, ZoomFactor);

	RescaleIcons();
}

void CircuitDiagram::ZoomOut()
{
	ui.GVCircuit->scale(1 / ZoomFactor, 1 / ZoomFactor);

	RescaleIcons();
}

void CircuitDiagram::ZoomReset()
{
	ui.GVCircuit->resetTransform();
	ui.GVCircuit->centerOn(ui.GVCircuit->scene()->sceneRect().center());

	RescaleIcons();
}

void CircuitDiagram::OnContextMenuRequested(QPoint Position)
{
	ContextMenu->exec(mapToGlobal(Position));
}

void CircuitDiagram::OnZoomIn()
{
	ZoomIn();
}

void CircuitDiagram::OnZoomOut()
{
	ZoomOut();
}

void CircuitDiagram::OnZoomReset()
{
	ZoomReset();
}

void CircuitDiagram::OnSaveDiagram()
{
	auto Filename = Util::PromptSaveFilePath(this, "Save circuit diagram", ".png", "Portable Network Graphics image (*.png)");
	if (Filename.isEmpty())
		return;

	QPixmap Pixmap = ui.GVCircuit->grab();
	if (!Pixmap.save(Filename))
		Util::EventLog().Log("Saving the current circuit diagram failed.", Util::ErrorType::Error);
}