#include "MapWidget.h"
#include "core/Assert.h"

#include <QPainter>
#include <QPainterPath>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QApplication>
#include <cmath>

namespace CF::UI {

using namespace CF::Domain;
using namespace CF::Core;

// ── Constants ─────────────────────────────────────────────────────────────────

static constexpr double NODE_BASE_RADIUS  = 14.0;  // px at zoom 1.0
static constexpr double NODE_CHAR_SCALE   = 2.0;   // extra px per character
static constexpr double NODE_MAX_RADIUS   = 32.0;
static constexpr double MINI_MAP_SIZE     = 160.0;
static constexpr double DETAIL_PANEL_W   = 200.0;
static constexpr double ZOOM_MIN         = 0.3;
static constexpr double ZOOM_MAX         = 5.0;

// ── Constructor ───────────────────────────────────────────────────────────────

MapWidget::MapWidget(
    std::shared_ptr<MapViewModel> viewModel, QWidget* parent)
    : QWidget(parent)
    , m_viewModel(std::move(viewModel))
{
    CF_REQUIRE(m_viewModel != nullptr, "MapViewModel must not be null");
    setupUi();
    setupConnections();
    setMouseTracking(true);
}

void MapWidget::activate()
{
    m_viewModel->loadAll();
}

// ── Setup ─────────────────────────────────────────────────────────────────────

void MapWidget::setupUi()
{
    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(0);

    setupToolbar(root);

    // Canvas fills the remaining space
    m_canvas = new QWidget(this);
    m_canvas->setObjectName("mapCanvas");
    m_canvas->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    m_canvas->setMouseTracking(true);
    m_canvas->installEventFilter(this);
    root->addWidget(m_canvas, 1);

    setupDetailPanel();
}

void MapWidget::setupToolbar(QVBoxLayout* root)
{
    auto* toolbar = new QWidget(this);
    toolbar->setObjectName("mapToolbar");
    toolbar->setFixedHeight(40);
    auto* tl = new QHBoxLayout(toolbar);
    tl->setContentsMargins(8, 4, 8, 4);
    tl->setSpacing(6);

    m_fitButton = new QToolButton(toolbar);
    m_fitButton->setText("⊡ Fit all");
    m_fitButton->setObjectName("timelineBtn");
    connect(m_fitButton, &QToolButton::clicked, this, [this]() {
        m_zoom      = 1.0;
        m_panOffset = QPointF(0, 0);
        update();
    });

    auto* zoomInBtn = new QToolButton(toolbar);
    zoomInBtn->setText("+");
    zoomInBtn->setObjectName("timelineBtn");
    connect(zoomInBtn, &QToolButton::clicked, this, [this]() {
        zoomAround(QPointF(width() / 2.0, height() / 2.0), 1.5);
    });

    auto* zoomOutBtn = new QToolButton(toolbar);
    zoomOutBtn->setText("−");
    zoomOutBtn->setObjectName("timelineBtn");
    connect(zoomOutBtn, &QToolButton::clicked, this, [this]() {
        zoomAround(QPointF(width() / 2.0, height() / 2.0), 1.0 / 1.5);
    });

    auto* helpLabel = new QLabel(
        "Drag nodes to reposition  •  Scroll to zoom  •  Middle-drag to pan",
        toolbar);
    helpLabel->setObjectName("countLabel");

    tl->addWidget(m_fitButton);
    tl->addWidget(zoomInBtn);
    tl->addWidget(zoomOutBtn);
    tl->addSpacing(16);
    tl->addWidget(helpLabel);
    tl->addStretch();

    root->addWidget(toolbar);
}

void MapWidget::setupDetailPanel()
{
    // Floating panel — drawn as an overlay in paintEvent, but
    // also has real widgets for keyboard accessibility
    m_detailPanel  = new QWidget(this);
    m_detailPanel->setObjectName("mapDetailPanel");
    m_detailPanel->setFixedWidth(static_cast<int>(DETAIL_PANEL_W));
    m_detailPanel->hide();

    auto* dl = new QVBoxLayout(m_detailPanel);
    dl->setContentsMargins(10, 10, 10, 10);
    dl->setSpacing(4);

    m_detailName   = new QLabel(m_detailPanel);
    m_detailName->setObjectName("sectionTitle");
    m_detailType   = new QLabel(m_detailPanel);
    m_detailRegion = new QLabel(m_detailPanel);
    m_detailChars  = new QLabel(m_detailPanel);
    m_detailType->setObjectName("countLabel");
    m_detailRegion->setObjectName("countLabel");
    m_detailChars->setObjectName("countLabel");

    dl->addWidget(m_detailName);
    dl->addWidget(m_detailType);
    dl->addWidget(m_detailRegion);
    dl->addWidget(m_detailChars);
    dl->addStretch();
}

void MapWidget::setupConnections()
{
    connect(m_viewModel.get(), &MapViewModel::nodesChanged,
            this, &MapWidget::onNodesChanged);
    connect(m_viewModel.get(), &MapViewModel::selectionChanged,
            this, &MapWidget::onSelectionChanged);
}

// ── Slots ─────────────────────────────────────────────────────────────────────

void MapWidget::onNodesChanged()
{
    update();
}

void MapWidget::onSelectionChanged()
{
    const auto node = m_viewModel->selectedNode();
    if (node.has_value()) {
        m_detailName->setText(node->name);
        m_detailType->setText("Type: " + node->type);
        m_detailRegion->setText("Region: " + node->region);
        m_detailChars->setText(
            QString("Characters here: %1").arg(node->characterCount));
        m_detailPanel->show();
        m_detailPanel->move(width() - static_cast<int>(DETAIL_PANEL_W) - 8,
                            48);
        emit placeSelected(node->id);
    } else {
        m_detailPanel->hide();
    }
    update();
}

// ── paintEvent ────────────────────────────────────────────────────────────────

void MapWidget::paintEvent(QPaintEvent*)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.setRenderHint(QPainter::TextAntialiasing);

    paintBackground(p);
    paintGrid(p);
    paintRegionPanels(p);
    paintEdges(p);
    paintNodes(p);
    paintLabels(p);
    paintMiniMap(p);
}

void MapWidget::paintBackground(QPainter& p)
{
    p.fillRect(rect(), QColor("#1e1e2e"));
}

void MapWidget::paintGrid(QPainter& p)
{
    // Subtle dot grid that scales with zoom
    const double gridSpacing = 40.0 * m_zoom;
    if (gridSpacing < 10.0) return;  // Too dense — skip

    p.setPen(QPen(QColor("#313244"), 1));

    const double offX = std::fmod(m_panOffset.x(), gridSpacing);
    const double offY = std::fmod(m_panOffset.y(), gridSpacing);

    for (double x = offX; x < width();  x += gridSpacing)
    for (double y = offY; y < height(); y += gridSpacing) {
        p.drawPoint(static_cast<int>(x), static_cast<int>(y));
    }
}

void MapWidget::paintRegionPanels(QPainter& p)
{
    for (int ri = 0; ri < static_cast<int>(m_viewModel->regions().size()); ++ri) {
        const auto& region = m_viewModel->regions()[ri];
        if (region.nodeIndices.empty()) continue;

        const QRectF bounds = regionBounds(region);
        if (!bounds.isValid()) continue;

        // Translucent filled rounded rect
        QColor fill = region.colour;
        fill.setAlpha(18);
        QColor border = region.colour;
        border.setAlpha(60);

        p.setPen(QPen(border, 1.5));
        p.setBrush(fill);
        p.drawRoundedRect(bounds, 12, 12);

        // Region label in top-left corner of the panel
        p.setPen(region.colour);
        QFont f = p.font();
        f.setPixelSize(11);
        f.setBold(true);
        p.setFont(f);
        p.drawText(bounds.adjusted(10, 6, 0, 0).toRect(),
                   Qt::AlignTop | Qt::AlignLeft,
                   region.name);
    }
}

void MapWidget::paintEdges(QPainter& p)
{
    // Draw edges between parent and child nodes
    const auto& nodes = m_viewModel->nodes();

    p.setPen(QPen(QColor("#45475a"), 1.5, Qt::DotLine));

    for (const auto& node : nodes) {
        // Find children
        for (const auto& other : nodes) {
            // We draw edges based on same region and position proximity
            // (full parent-child edges require loading Place.parentPlaceId
            //  which we skip here for brevity — hook into PlaceService if needed)
        }
    }

    // Draw mobile-place trajectory lines (dashed, faint)
    for (const auto& node : nodes) {
        if (!node.isMobile) continue;
        // Mobile place: draw a subtle arc from its current position
        // (A full trajectory system would need location history — stub here)
        const QPointF pos = normToCanvas(node.normX, node.normY);
        p.setPen(QPen(QColor("#585b70"), 1, Qt::DashLine));
        p.setBrush(Qt::NoBrush);
        p.drawEllipse(pos, nodeRadius(node) + 6, nodeRadius(node) + 6);
    }
}

void MapWidget::paintNodes(QPainter& p)
{
    const auto& nodes = m_viewModel->nodes();

    // Draw unselected nodes first, then hovered, then selected on top
    for (int i = 0; i < static_cast<int>(nodes.size()); ++i) {
        const bool isSelected = m_viewModel->selectedNodeIndex() == i;
        const bool isHovered  = m_hoveredNodeIndex == i;
        if (!isSelected && !isHovered) {
            paintNode(p, nodes[i], false, false);
        }
    }
    for (int i = 0; i < static_cast<int>(nodes.size()); ++i) {
        const bool isSelected = m_viewModel->selectedNodeIndex() == i;
        const bool isHovered  = m_hoveredNodeIndex == i;
        if ((isHovered || isSelected) && !(isSelected && isHovered)) {
            paintNode(p, nodes[i], isSelected, isHovered);
        }
    }
    // Selected on top
    for (int i = 0; i < static_cast<int>(nodes.size()); ++i) {
        if (m_viewModel->selectedNodeIndex() == i) {
            paintNode(p, nodes[i], true, m_hoveredNodeIndex == i);
        }
    }
}

void MapWidget::paintNode(
    QPainter& p, const MapViewModel::PlaceNode& node,
    bool isSelected, bool isHovered)
{
    const QPointF centre = normToCanvas(node.normX, node.normY);
    const double  r      = nodeRadius(node);

    // Node colour — region colour or fallback
    QColor nodeColor("#89b4fa");
    for (const auto& region : m_viewModel->regions()) {
        if (region.name == node.region) {
            nodeColor = region.colour;
            break;
        }
    }

    // Glow ring for selected/hovered
    if (isSelected || isHovered) {
        QColor glow = nodeColor;
        glow.setAlpha(isSelected ? 80 : 40);
        p.setPen(Qt::NoPen);
        p.setBrush(glow);
        p.drawEllipse(centre, r + 8, r + 8);
    }

    if (node.isMobile) {
        // Mobile: dashed border
        p.setPen(QPen(nodeColor, 2.0, Qt::DashLine));
    } else {
        p.setPen(QPen(isSelected ? Qt::white : nodeColor.darker(140),
                      isSelected ? 2.5 : 1.5));
    }

    // Shape: hexagon for parent nodes, circle otherwise
    if (node.hasChildren) {
        QPainterPath hex;
        for (int i = 0; i < 6; ++i) {
            const double angle = M_PI / 180.0 * (60.0 * i - 30.0);
            const QPointF pt(centre.x() + r * std::cos(angle),
                             centre.y() + r * std::sin(angle));
            if (i == 0) hex.moveTo(pt);
            else         hex.lineTo(pt);
        }
        hex.closeSubpath();
        QColor fill = nodeColor;
        fill.setAlpha(isSelected ? 220 : 160);
        p.setBrush(fill);
        p.drawPath(hex);
    } else {
        QColor fill = nodeColor;
        fill.setAlpha(isSelected ? 220 : 160);
        p.setBrush(fill);
        p.drawEllipse(centre, r, r);
    }

    // Character count badge (if > 0)
    if (node.characterCount > 0) {
        const QPointF badgeCenter(centre.x() + r * 0.7, centre.y() - r * 0.7);
        p.setPen(Qt::NoPen);
        p.setBrush(QColor("#f38ba8"));
        p.drawEllipse(badgeCenter, 8, 8);

        p.setPen(QColor("#1e1e2e"));
        QFont f = p.font();
        f.setPixelSize(8);
        f.setBold(true);
        p.setFont(f);
        p.drawText(
            QRectF(badgeCenter.x() - 8, badgeCenter.y() - 8, 16, 16),
            Qt::AlignCenter,
            QString::number(node.characterCount));
    }
}

void MapWidget::paintLabels(QPainter& p)
{
    QFont f = p.font();
    f.setPixelSize(static_cast<int>(10.0 * std::sqrt(m_zoom)));
    f.setPixelSize(std::max(9, std::min(14, f.pixelSize())));
    p.setFont(f);

    const double minZoomForLabel = 0.4;
    if (m_zoom < minZoomForLabel) return;

    for (const auto& node : m_viewModel->nodes()) {
        const QPointF centre = normToCanvas(node.normX, node.normY);
        const double  r      = nodeRadius(node);

        // Name below the node
        p.setPen(QColor("#cdd6f4"));
        const QRectF nameRect(centre.x() - 50, centre.y() + r + 4, 100, 16);
        p.drawText(nameRect, Qt::AlignCenter, node.name);

        // Type in smaller text, dimmer
        if (m_zoom > 0.7) {
            p.setPen(QColor("#6c7086"));
            QFont sf = f;
            sf.setPixelSize(f.pixelSize() - 2);
            p.setFont(sf);
            p.drawText(QRectF(centre.x() - 50, centre.y() + r + 20, 100, 14),
                       Qt::AlignCenter, node.type);
            p.setFont(f);
        }
    }
}

void MapWidget::paintMiniMap(QPainter& p)
{
    // Only show mini-map when zoomed in enough to need it
    if (m_zoom <= 1.2) return;

    const double mmSize = MINI_MAP_SIZE;
    const QRectF mmRect(width() - mmSize - 8,
                        height() - mmSize - 8,
                        mmSize, mmSize);

    // Background
    p.setPen(QPen(QColor("#45475a"), 1));
    p.setBrush(QColor("#1e1e2e").darker(110));
    p.drawRoundedRect(mmRect, 6, 6);

    // Nodes as tiny dots in normalised position
    for (const auto& node : m_viewModel->nodes()) {
        const QPointF mmPos(mmRect.left() + node.normX * mmSize,
                            mmRect.top()  + node.normY * mmSize);
        QColor c("#89b4fa");
        for (const auto& region : m_viewModel->regions()) {
            if (region.name == node.region) { c = region.colour; break; }
        }
        p.setPen(Qt::NoPen);
        p.setBrush(c);
        p.drawEllipse(mmPos, 3, 3);
    }

    // Viewport rectangle on mini-map
    const QRectF canvas = canvasRect();
    // Convert current view corner from canvas coords to normalised
    const QPointF topLeftNorm  = canvasToNorm(canvas.topLeft());
    const QPointF botRightNorm = canvasToNorm(canvas.bottomRight());

    const QRectF vpRect(
        mmRect.left() + topLeftNorm.x()  * mmSize,
        mmRect.top()  + topLeftNorm.y()  * mmSize,
        (botRightNorm.x() - topLeftNorm.x()) * mmSize,
        (botRightNorm.y() - topLeftNorm.y()) * mmSize);

    p.setPen(QPen(QColor("#89b4fa"), 1));
    p.setBrush(QColor(137, 180, 250, 25));
    p.drawRect(vpRect);
}

// ── Mouse events ──────────────────────────────────────────────────────────────

void MapWidget::mousePressEvent(QMouseEvent* event)
{
    const QPointF pos = event->position();

    if (event->button() == Qt::MiddleButton) {
        m_isPanning      = true;
        m_panStartMouse  = pos;
        m_panStartOffset = m_panOffset;
        setCursor(Qt::ClosedHandCursor);
        return;
    }

    if (event->button() == Qt::LeftButton) {
        const auto nodeIdx = hitTestNode(pos);
        if (nodeIdx.has_value()) {
            m_viewModel->selectNode(nodeIdx);

            // Begin drag
            m_isDraggingNode  = true;
            m_dragNodeIndex   = nodeIdx;
            m_dragStartCanvas = pos;
            const auto& node  = m_viewModel->nodes()[*nodeIdx];
            m_dragStartNormX  = node.normX;
            m_dragStartNormY  = node.normY;
            setCursor(Qt::ClosedHandCursor);
        } else {
            m_viewModel->selectNode(std::nullopt);
        }
    }
}

void MapWidget::mouseMoveEvent(QMouseEvent* event)
{
    const QPointF pos = event->position();

    if (m_isPanning) {
        const QPointF delta = pos - m_panStartMouse;
        m_panOffset = m_panStartOffset + delta;
        clampPan();
        update();
        return;
    }

    if (m_isDraggingNode && m_dragNodeIndex.has_value()) {
        const QPointF delta  = pos - m_dragStartCanvas;
        const QRectF  cr     = canvasRect();
        const double  normDX = delta.x() / (cr.width()  * m_zoom);
        const double  normDY = delta.y() / (cr.height() * m_zoom);

        m_viewModel->moveNode(
            *m_dragNodeIndex,
            m_dragStartNormX + normDX,
            m_dragStartNormY + normDY);
        return;
    }

    // Hover detection
    const auto nodeIdx = hitTestNode(pos);
    if (nodeIdx != m_hoveredNodeIndex) {
        m_hoveredNodeIndex = nodeIdx;
        setCursor(nodeIdx.has_value() ? Qt::PointingHandCursor : Qt::ArrowCursor);
        update();
    }
}

void MapWidget::mouseReleaseEvent(QMouseEvent* event)
{
    if (event->button() == Qt::MiddleButton) {
        m_isPanning = false;
        setCursor(Qt::ArrowCursor);
    }
    if (event->button() == Qt::LeftButton) {
        m_isDraggingNode = false;
        m_dragNodeIndex.reset();
        setCursor(m_hoveredNodeIndex.has_value()
                  ? Qt::PointingHandCursor : Qt::ArrowCursor);
    }
}

void MapWidget::mouseDoubleClickEvent(QMouseEvent* event)
{
    const auto nodeIdx = hitTestNode(event->position());
    if (nodeIdx.has_value()) {
        emit placeOpened(m_viewModel->nodes()[*nodeIdx].id);
    }
}

void MapWidget::wheelEvent(QWheelEvent* event)
{
    const double factor = event->angleDelta().y() > 0 ? 1.25 : 1.0 / 1.25;
    zoomAround(event->position(), factor);
    event->accept();
}

void MapWidget::leaveEvent(QEvent*)
{
    m_hoveredNodeIndex.reset();
    update();
}

void MapWidget::resizeEvent(QResizeEvent*)
{
    if (m_detailPanel && m_detailPanel->isVisible()) {
        m_detailPanel->move(
            width() - static_cast<int>(DETAIL_PANEL_W) - 8, 48);
    }
    update();
}

// ── Coordinate transforms ─────────────────────────────────────────────────────

QPointF MapWidget::normToCanvas(double normX, double normY) const
{
    const QRectF cr = canvasRect();
    return QPointF(
        m_panOffset.x() + cr.left() + normX * cr.width()  * m_zoom,
        m_panOffset.y() + cr.top()  + normY * cr.height() * m_zoom);
}

QPointF MapWidget::canvasToNorm(const QPointF& pos) const
{
    const QRectF cr = canvasRect();
    if (cr.width() <= 0 || cr.height() <= 0) return {};
    return QPointF(
        (pos.x() - m_panOffset.x() - cr.left()) / (cr.width()  * m_zoom),
        (pos.y() - m_panOffset.y() - cr.top())  / (cr.height() * m_zoom));
}

QRectF MapWidget::nodeRect(const MapViewModel::PlaceNode& node, bool large) const
{
    const QPointF c = normToCanvas(node.normX, node.normY);
    const double  r = nodeRadius(node) * (large ? 1.5 : 1.0);
    return QRectF(c.x() - r, c.y() - r, r * 2, r * 2);
}

double MapWidget::nodeRadius(const MapViewModel::PlaceNode& node) const
{
    const double base = NODE_BASE_RADIUS * m_zoom;
    const double extra = std::min(
        static_cast<double>(node.characterCount) * NODE_CHAR_SCALE * m_zoom,
        NODE_MAX_RADIUS * m_zoom - base);
    return base + extra;
}

QRectF MapWidget::canvasRect() const
{
    // Reserve top strip for toolbar (40px)
    return QRectF(0, 40, width(), height() - 40);
}

// ── Hit testing ───────────────────────────────────────────────────────────────

std::optional<int>
MapWidget::hitTestNode(const QPointF& canvasPos) const
{
    const auto& nodes = m_viewModel->nodes();
    int    bestIdx  = -1;
    double bestDist = std::numeric_limits<double>::max();

    for (int i = 0; i < static_cast<int>(nodes.size()); ++i) {
        const QPointF centre = normToCanvas(nodes[i].normX, nodes[i].normY);
        const double  r      = nodeRadius(nodes[i]) + 4.0; // small hit buffer
        const double  dx     = canvasPos.x() - centre.x();
        const double  dy     = canvasPos.y() - centre.y();
        const double  dist2  = dx * dx + dy * dy;
        if (dist2 <= r * r && dist2 < bestDist) {
            bestDist = dist2;
            bestIdx  = i;
        }
    }
    return bestIdx >= 0 ? std::optional<int>(bestIdx) : std::nullopt;
}

// ── Region bounds ─────────────────────────────────────────────────────────────

QRectF MapWidget::regionBounds(
    const MapViewModel::RegionPanel& region, double paddingNorm) const
{
    if (region.nodeIndices.empty()) return {};

    const auto& nodes = m_viewModel->nodes();
    double minX = 1.0, minY = 1.0, maxX = 0.0, maxY = 0.0;

    for (const int idx : region.nodeIndices) {
        const auto& n = nodes[idx];
        minX = std::min(minX, n.normX);
        minY = std::min(minY, n.normY);
        maxX = std::max(maxX, n.normX);
        maxY = std::max(maxY, n.normY);
    }

    // Add padding
    minX -= paddingNorm; minY -= paddingNorm;
    maxX += paddingNorm; maxY += paddingNorm;

    // Convert to canvas pixels
    const QPointF tl = normToCanvas(minX, minY);
    const QPointF br = normToCanvas(maxX, maxY);
    return QRectF(tl, br);
}

// ── Zoom & pan ────────────────────────────────────────────────────────────────

void MapWidget::zoomAround(const QPointF& pivot, double factor)
{
    const double newZoom = std::max(ZOOM_MIN,
                           std::min(ZOOM_MAX, m_zoom * factor));
    if (newZoom == m_zoom) return;

    // Adjust pan so the point under the cursor stays fixed
    const double scale = newZoom / m_zoom;
    m_panOffset = pivot + (m_panOffset - pivot) * scale;
    m_zoom      = newZoom;

    clampPan();
    update();
}

void MapWidget::clampPan()
{
    // Allow slight over-pan (10% of widget size) but not more
    const double margin = 0.1;
    m_panOffset.setX(std::max(-width()  * m_zoom * (1.0 - margin),
                              std::min(width()  * margin,
                                       m_panOffset.x())));
    m_panOffset.setY(std::max(-height() * m_zoom * (1.0 - margin),
                              std::min(height() * margin,
                                       m_panOffset.y())));
}

} // namespace CF::UI