// This file is part of DynExp.

/**
 * @file CircuitDiagram.h
 * @brief Implements a window drawing the relations between all DynExp::Object instances as a graph.
 * The arrangement of graph items is optimized by simulated annealing.
*/

#pragma once

#include <QWidget>
#include "ui_CircuitDiagram.h"
#include "DynExpDefinitions.h"
#include "Object.h"

namespace DynExp
{
	class DynExpCore;
}

class CircuitDiagram : public QDialog
{
	Q_OBJECT

	class CircuitDiagramItem;

	using NodeMapType = std::unordered_map<DynExp::ItemIDType, CircuitDiagramItem>;
	using NodeListType = std::vector<typename NodeMapType::mapped_type*>;

	static constexpr double ZoomFactor = 1.6;

	struct GraphType
	{
		NodeListType HardwareAdapters;
		NodeListType Instruments;
		NodeListType Modules;

		double EvaluateEnergy() const;
		double DistanceTo(const GraphType& Other) const;

		bool operator==(const GraphType& Other) const;
		bool operator!=(const GraphType& Other) const { return !(*this == Other); }
	};

	class CircuitDiagramItem
	{
	public:
		CircuitDiagramItem() : TreeWidgetItem(nullptr), Empty(true) {}
		CircuitDiagramItem(QTreeWidgetItem* TreeWidgetItem) : TreeWidgetItem(TreeWidgetItem), Empty(false) {}

		void InsertLinks(const DynExp::ParamsBase::ObjectLinkParamsType& ObjectLinkParams, const DynExp::DynExpCore& DynExpCore,
			NodeMapType* HardwareAdapterNodes, NodeMapType* InstrumentNodes);

		size_t GetNumLinkedItems();

		/**
		 * @brief Pointer to QTreeWidgetItem listed in main window's tree view. Pointer may be dereferenced after
		 * this CircuitDiagramItem has been created and control has returned to the main window's event loop since 
		 * DynExpManager pays attention to call Redraw() instead of UpdateStates() after tree view has changed.
		 * Note that there is no guarantee for other function calls that TreeWidgetItem is in a valid state!
		 * Other functions may only use TreeWidgetItem for address comparison purposes.
		*/
		QTreeWidgetItem* TreeWidgetItem;

		/**
		 * @brief String of a network address and a network port indicating whether this item connects to a network.
		 * Empty if it does not.
		*/
		QString NetworkAddress = nullptr;

		/**
		 * @brief Node's top left position in the diagram in painting area's coordinates.
		*/
		QPointF TopLeftPos;

		qreal ParamSectionHeight{};
		qreal ItemHeight{};

		struct LinkedParamType
		{
			struct LinkedItemType
			{
				LinkedItemType() noexcept : Item(nullptr), SocketIndex(0) {}
				LinkedItemType(CircuitDiagramItem* Item) noexcept : Item(Item), SocketIndex(0) {}

				CircuitDiagramItem* Item;
				size_t SocketIndex;
			};

			std::string_view LinkTitle;
			std::vector<LinkedItemType> LinkedItems;	// May contain LinkedItemType with Item = nullptr.
			size_t ParamGraphicsItemIndex{};
		};

		/**
		 * @brief List of linked params making use of this item.
		*/
		std::vector<LinkedParamType> LinkedParams;

		/**
		 * @brief Indicates whether this item points to an existing object.
		*/
		const bool Empty;

		// Item's displayed parts
		struct ParamGraphicsItemsType
		{
			QGraphicsPathItem* Frame = nullptr;
			QGraphicsSimpleTextItem* Label = nullptr;
			std::vector<QGraphicsEllipseItem*> Sockets;
			std::vector<QGraphicsPathItem*> Links;
		};

		QGraphicsPathItem* StateFrame = nullptr;
		QIcon StateIcon;
		QGraphicsPixmapItem* StatePixmap = nullptr;
		QGraphicsSimpleTextItem* StateLabel = nullptr;
		QGraphicsPathItem* ItemFrame = nullptr;
		QGraphicsSimpleTextItem* ItemLabel = nullptr;
		QIcon ItemIcon;
		QGraphicsPixmapItem* ItemPixmap = nullptr;
		QGraphicsEllipseItem* OutputSocket = nullptr;
		std::vector<ParamGraphicsItemsType> ParamGraphicsItems;
		QIcon NetworkIcon;
		QGraphicsPixmapItem* NetworkPixmap = nullptr;
		QGraphicsPathItem* NetworkLink = nullptr;
	};

public:
	CircuitDiagram(QWidget *parent);
	~CircuitDiagram();

	QTreeWidgetItem* GetSelectedEntry();

	/**
	 * @brief Rebuilds the complete circuit diagram.
	 * @param DynExpCore Reference to the DynExpCore instance owning the resources to be visualized.
	 * @return Returns true in case of success, false otherwise.
	*/
	bool Redraw(const DynExp::DynExpCore& DynExpCore);

	/**
	 * @brief Updates the items' states shown in the circuit diagram displayed currently.
	 * @param DynExpCore Reference to the DynExpCore instance owning the resources to be visualized.
	 * @return Returns true in case of success, false otherwise.
	*/
	bool UpdateStates(const DynExp::DynExpCore& DynExpCore);

protected:
	virtual void mouseDoubleClickEvent(QMouseEvent* Event) override;
	virtual void wheelEvent(QWheelEvent* Event) override;

private:
	// Constants for rendering the circuit diagram
	static constexpr int NodeVSep = 40;
	static constexpr int NodeHSep = 220;
	static constexpr int TreeWidgetParentTypeColumn = 0;
	static constexpr int TreeWidgetItemNameColumn = 0;
	static constexpr int TreeWidgetItemTypeColumn = 1;
	static constexpr int TreeWidgetItemStateColumn = 2;
	static constexpr int OuterPenLineWidth = 4;
	static constexpr int InnerPenLineWidth = 1;
	static constexpr int CornerRoundingRadius = 8;
	static constexpr int InnerWidth = 140;
	static constexpr int InnerStartHeight = 50;
	static constexpr int InnerMargin = 6;
	static constexpr int InnerMarginTop = 10;
	static constexpr int InnerMarginBottom = 20;
	static constexpr int StateIconSize = 16;
	static constexpr int TypeIconSize = 24;
	static constexpr int IconMargin = 4;
	static constexpr int AdditionalHeightPerParam = 12;
	static constexpr int ParamSep = 6;
	static constexpr int SocketDiameter = 6;
	static constexpr int SocketPenLineWidth = 2;
	static constexpr int NetworkIconDistance = 30;
	static const QColor SocketOuterColor;
	static const QColor SocketInnerColor;

	static QLinearGradient GetGrayLinearGradient();

	// Helper functions
	void Clear();
	void BuildTree(const DynExp::DynExpCore& DynExpCore);
	void ArrangeTree();
	void RefineBySimulatedAnnealing(GraphType& Graph);
	void Render();
	void RenderItem(CircuitDiagramItem& Item, bool DrawOutputSocket);
	void RenderLinks(CircuitDiagramItem& Item);
	void UpdateItem(CircuitDiagramItem& Item);
	int TransformIconSize(int Size) const;
	void RescaleIcons();

	// Zoom functions
	void ZoomIn();
	void ZoomOut();
	void ZoomReset();

	// Displayed items
	NodeMapType HardwareAdapterNodes;
	NodeMapType InstrumentNodes;
	NodeMapType ModuleNodes;

	// Item currently selected by a double-click
	bool SelectionChanged;
	QTreeWidgetItem* SelectedTreeWidgetItem;

	// UI
	Ui::CircuitDiagram ui;
	std::unique_ptr<QGraphicsScene> Scene;
	QMenu* ContextMenu;

private slots:
	void OnContextMenuRequested(QPoint Position);
	void OnZoomIn();
	void OnZoomOut();
	void OnZoomReset();
	void OnSaveDiagram();
};