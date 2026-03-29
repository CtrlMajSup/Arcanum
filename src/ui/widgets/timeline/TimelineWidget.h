#pragma once

#include <QWidget>
#include <QScrollBar>
#include <QToolButton>
#include <QLabel>
#include <QCheckBox>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <optional>
#include <memory>

#include "ui/viewmodels/TimelineViewModel.h"

namespace CF::UI {

/**
 * TimelineWidget renders the world timeline as a custom-painted canvas.
 *
 * Layout:
 * ┌─────────────────────────────────────────────────────┐
 * │  Toolbar: zoom in/out, pan, lane toggles            │
 * ├─────────────────────────────────────────────────────┤
 * │                                                     │
 * │  [Characters] ──●────────●──────●───────────────   │
 * │  [Factions  ] ─────●──────────────────●──────────   │
 * │  [Places    ] ──●─────────────────●──────────────   │
 * │  [Scenes    ] ──────●──────●──────────────────────  │
 * │  [Custom    ] ─────────────────────────────────────  │
 * │                                                     │
 * ├─────────────────────────────────────────────────────┤
 * │  Scrollbar (pans the window)                        │
 * └─────────────────────────────────────────────────────┘
 *
 * Clicking an event opens a tooltip popup with full event details.
 * Scroll wheel zooms, middle-click-drag pans.
 */
class TimelineWidget : public QWidget {
    Q_OBJECT

public:
    explicit TimelineWidget(
        std::shared_ptr<TimelineViewModel> viewModel,
        QWidget* parent = nullptr);

    // Called by MainWindow when this panel becomes visible
    void activate();

signals:
    void eventClicked(Domain::TimelineId eventId);

protected:
    void paintEvent(QPaintEvent* event)       override;
    void mousePressEvent(QMouseEvent* event)  override;
    void mouseMoveEvent(QMouseEvent* event)   override;
    void mouseReleaseEvent(QMouseEvent* event)override;
    void wheelEvent(QWheelEvent* event)       override;
    void resizeEvent(QResizeEvent* event)     override;
    void leaveEvent(QEvent* event)            override;

private slots:
    void onEventsChanged();
    void onWindowChanged();
    void onZoomIn();
    void onZoomOut();
    void onResetView();
    void onScrollBarMoved(int value);
    void onLaneToggled(int laneIndex, bool visible);

private:
    // ── Setup ─────────────────────────────────────────────────────────────────
    void setupUi();
    void setupToolbar(QVBoxLayout* root);
    void setupCanvas();
    void setupScrollBar(QVBoxLayout* root);
    void setupConnections();

    // ── Painting ──────────────────────────────────────────────────────────────
    void paintBackground(QPainter& p);
    void paintRuler(QPainter& p);
    void paintLaneLabels(QPainter& p);
    void paintLaneSeparators(QPainter& p);
    void paintEvents(QPainter& p);
    void paintEvent(QPainter& p,
                    const TimelineViewModel::DisplayEvent& event,
                    int laneY, int laneHeight);
    void paintTooltip(QPainter& p);

    // ── Geometry helpers ──────────────────────────────────────────────────────
    [[nodiscard]] QRect  canvasRect()              const;
    [[nodiscard]] int    laneY(int laneIndex)      const;
    [[nodiscard]] int    laneHeight()              const;
    [[nodiscard]] int    xForNormalisedX(double nx)const;
    [[nodiscard]] double normalisedXForX(int x)   const;
    [[nodiscard]] int    rulerHeight()             const { return 32; }
    [[nodiscard]] int    labelWidth()              const { return 100; }

    // ── Ruler tick computation ────────────────────────────────────────────────
    struct RulerTick { double normalisedX; QString label; bool isMajor; };
    [[nodiscard]] std::vector<RulerTick> computeRulerTicks() const;

    // ── Hit testing ───────────────────────────────────────────────────────────
    [[nodiscard]] std::optional<TimelineViewModel::DisplayEvent>
    hitTest(const QPoint& pos) const;

    // ── Colours ───────────────────────────────────────────────────────────────
    [[nodiscard]] QColor laneColour(TimelineViewModel::DisplayEvent::Lane lane) const;
    // [[nodiscard]] QColor eventColour(TimelineViewModel::DisplayEvent::EventType type) const;
    [[nodiscard]] QColor eventColour(Domain::TimelineEvent::EventType type) const;

    // ── Scrollbar sync ────────────────────────────────────────────────────────
    void updateScrollBar();

    std::shared_ptr<TimelineViewModel> m_viewModel;

    // The actual painted canvas lives inside the layout
    QWidget*    m_canvas    = nullptr;
    QScrollBar* m_scrollBar = nullptr;

    // Tooltip state
    std::optional<TimelineViewModel::DisplayEvent> m_hoveredEvent;
    QPoint m_tooltipPos;

    // Pan state (middle-mouse drag)
    bool   m_isPanning    = false;
    QPoint m_panStartPos;
    Domain::TimePoint m_panStartWindowStart;

    bool m_scrollBarUpdating = false; // prevents feedback loop
};

} // namespace CF::UI