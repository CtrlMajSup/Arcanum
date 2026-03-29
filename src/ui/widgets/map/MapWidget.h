#pragma once

#include <QWidget>
#include <QScrollArea>
#include <QToolButton>
#include <QLabel>
#include <QLineEdit>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPainter>
#include <optional>
#include <memory>

#include "ui/viewmodels/MapViewModel.h"

namespace CF::UI {

/**
 * MapWidget renders a schematic map of all places.
 *
 * Visual elements:
 *  - Region panels: translucent rounded rectangles containing their nodes
 *  - Place nodes:   circles, size proportional to character count
 *  - Mobile nodes:  dashed border (spaceships, vehicles)
 *  - Parent nodes:  hexagon shape (places that contain children)
 *  - Edges:         lines connecting parent → child places
 *  - Labels:        name + type below each node
 *  - Mini-map:      bottom-right corner overview (when zoomed in)
 *
 * Interactions:
 *  - Left click:     select node → detail panel updates
 *  - Left drag:      drag selected node to reposition
 *  - Right click:    context menu (rename region, etc.)
 *  - Scroll wheel:   zoom in/out around cursor
 *  - Middle drag:    pan the canvas
 *  - Double click:   emit placeOpened (navigate to place detail)
 */
class MapWidget : public QWidget {
    Q_OBJECT

public:
    explicit MapWidget(
        std::shared_ptr<MapViewModel> viewModel,
        QWidget* parent = nullptr);

    void activate();

signals:
    void placeSelected(Domain::PlaceId id);
    void placeOpened(Domain::PlaceId id);

protected:
    void paintEvent(QPaintEvent* event)        override;
    void mousePressEvent(QMouseEvent* event)   override;
    void mouseMoveEvent(QMouseEvent* event)    override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void mouseDoubleClickEvent(QMouseEvent* event) override;
    void wheelEvent(QWheelEvent* event)        override;
    void leaveEvent(QEvent* event)             override;
    void resizeEvent(QResizeEvent* event)      override;

private slots:
    void onNodesChanged();
    void onSelectionChanged();

private:
    // ── Setup ─────────────────────────────────────────────────────────────────
    void setupUi();
    void setupToolbar(QVBoxLayout* root);
    void setupDetailPanel();
    void setupConnections();

    // ── Painting ──────────────────────────────────────────────────────────────
    void paintBackground(QPainter& p);
    void paintGrid(QPainter& p);
    void paintRegionPanels(QPainter& p);
    void paintEdges(QPainter& p);
    void paintNodes(QPainter& p);
    void paintNode(QPainter& p, const MapViewModel::PlaceNode& node,
                   bool isSelected, bool isHovered);
    void paintLabels(QPainter& p);
    void paintMiniMap(QPainter& p);
    void paintDetailPanel(QPainter& p);

    // ── Coordinate transforms ─────────────────────────────────────────────────

    // Normalised [0,1] ↔ canvas pixels (accounting for zoom + pan)
    [[nodiscard]] QPointF normToCanvas(double normX, double normY) const;
    [[nodiscard]] QPointF canvasToNorm(const QPointF& canvasPos)   const;

    // Canvas pixel rect for a node (circle bounding box)
    [[nodiscard]] QRectF  nodeRect(const MapViewModel::PlaceNode& node,
                                   bool large = false) const;

    // Node radius in pixels (scaled by character count)
    [[nodiscard]] double  nodeRadius(const MapViewModel::PlaceNode& node) const;

    // ── Hit testing ───────────────────────────────────────────────────────────
    [[nodiscard]] std::optional<int> hitTestNode(const QPointF& canvasPos) const;

    // ── Region bounding box (in canvas coords) ────────────────────────────────
    [[nodiscard]] QRectF regionBounds(const MapViewModel::RegionPanel& region,
                                      double paddingNorm = 0.03) const;

    // ── Zoom & pan state ──────────────────────────────────────────────────────
    double  m_zoom       = 1.0;    // 1.0 = fit entire canvas in widget
    QPointF m_panOffset;           // Canvas origin offset in pixels

    void clampPan();
    void zoomAround(const QPointF& pivot, double factor);
    [[nodiscard]] QRectF canvasRect() const;  // The drawable area

    // ── Drag state ────────────────────────────────────────────────────────────
    bool            m_isDraggingNode = false;
    bool            m_isPanning      = false;
    std::optional<int> m_dragNodeIndex;
    QPointF         m_dragStartCanvas;
    double          m_dragStartNormX = 0.0;
    double          m_dragStartNormY = 0.0;
    QPointF         m_panStartOffset;
    QPointF         m_panStartMouse;

    // ── Hover state ───────────────────────────────────────────────────────────
    std::optional<int> m_hoveredNodeIndex;

    // ── UI children ───────────────────────────────────────────────────────────
    QWidget*    m_canvas      = nullptr;
    QWidget*    m_detailPanel = nullptr;
    QLabel*     m_detailName  = nullptr;
    QLabel*     m_detailType  = nullptr;
    QLabel*     m_detailRegion= nullptr;
    QLabel*     m_detailChars = nullptr;
    QToolButton* m_fitButton  = nullptr;

    std::shared_ptr<MapViewModel> m_viewModel;
};

} // namespace CF::UI