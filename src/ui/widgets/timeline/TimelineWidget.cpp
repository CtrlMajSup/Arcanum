#include "TimelineWidget.h"
#include "core/Assert.h"

#include <QPainter>
#include <QPainterPath>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QResizeEvent>
#include <QFontMetrics>

namespace CF::UI {

using namespace CF::Domain;
using namespace CF::Core;
using Lane = TimelineViewModel::DisplayEvent::Lane;

// ── Constants ─────────────────────────────────────────────────────────────────

static constexpr int LANE_COUNT     = 5;
static constexpr int EVENT_RADIUS   = 7;
static constexpr int MIN_LANE_H     = 48;

static const char* LANE_NAMES[LANE_COUNT] = {
    "Characters", "Factions", "Places", "Scenes", "Custom"
};

// ── Constructor ───────────────────────────────────────────────────────────────

TimelineWidget::TimelineWidget(
    std::shared_ptr<TimelineViewModel> viewModel, QWidget* parent)
    : QWidget(parent)
    , m_viewModel(std::move(viewModel))
{
    CF_REQUIRE(m_viewModel != nullptr, "TimelineViewModel must not be null");
    setupUi();
    setupConnections();
    setMouseTracking(true);
}

void TimelineWidget::activate()
{
    m_viewModel->loadAll();
}

// ── Setup ─────────────────────────────────────────────────────────────────────

void TimelineWidget::setupUi()
{
    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(0);

    setupToolbar(root);
    //setupCanvas();
    //root->addWidget(m_canvas, 1);
    root->addStretch(1);
    setupScrollBar(root);
}

void TimelineWidget::setupToolbar(QVBoxLayout* root)
{
    auto* toolbar = new QWidget(this);
    toolbar->setObjectName("timelineToolbar");
    toolbar->setFixedHeight(40);
    auto* tl = new QHBoxLayout(toolbar);
    tl->setContentsMargins(8, 4, 8, 4);
    tl->setSpacing(4);

    // Zoom controls
    auto* zoomIn  = new QToolButton(toolbar);
    auto* zoomOut = new QToolButton(toolbar);
    auto* reset   = new QToolButton(toolbar);
    zoomIn->setText("+");   zoomIn->setToolTip("Zoom in");
    zoomOut->setText("−");  zoomOut->setToolTip("Zoom out");
    reset->setText("⊡");    reset->setToolTip("Reset view");
    zoomIn->setObjectName("timelineBtn");
    zoomOut->setObjectName("timelineBtn");
    reset->setObjectName("timelineBtn");

    connect(zoomIn,  &QToolButton::clicked, this, &TimelineWidget::onZoomIn);
    connect(zoomOut, &QToolButton::clicked, this, &TimelineWidget::onZoomOut);
    connect(reset,   &QToolButton::clicked, this, &TimelineWidget::onResetView);

    tl->addWidget(zoomIn);
    tl->addWidget(zoomOut);
    tl->addWidget(reset);
    tl->addSpacing(16);

    // Lane visibility toggles
    for (int i = 0; i < LANE_COUNT; ++i) {
        auto* cb = new QCheckBox(LANE_NAMES[i], toolbar);
        cb->setChecked(true);
        cb->setObjectName("laneToggle");
        const int laneIndex = i;
        connect(cb, &QCheckBox::toggled, this, [this, laneIndex](bool v) {
            onLaneToggled(laneIndex, v);
        });
        tl->addWidget(cb);
    }

    tl->addStretch();

    // Window label
    auto* windowLabel = new QLabel(toolbar);
    windowLabel->setObjectName("windowLabel");
    connect(m_viewModel.get(), &TimelineViewModel::windowChanged,
            this, [this, windowLabel]() {
                windowLabel->setText(
                    QString("%1  →  %2")
                    .arg(QString::fromStdString(m_viewModel->windowStart().display()))
                    .arg(QString::fromStdString(m_viewModel->windowEnd().display())));
            });
    tl->addWidget(windowLabel);

    root->addWidget(toolbar);
}

void TimelineWidget::setupCanvas()
{
    // The canvas is a plain QWidget that we paint on via paintEvent
    m_canvas = new QWidget(this);
    m_canvas->setObjectName("timelineCanvas");
    m_canvas->setMouseTracking(true);
    m_canvas->installEventFilter(this);
    m_canvas->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
}

void TimelineWidget::setupScrollBar(QVBoxLayout* root)
{
    m_scrollBar = new QScrollBar(Qt::Horizontal, this);
    m_scrollBar->setObjectName("timelineScrollBar");
    root->addWidget(m_scrollBar);

    connect(m_scrollBar, &QScrollBar::valueChanged,
            this, &TimelineWidget::onScrollBarMoved);
}

void TimelineWidget::setupConnections()
{
    connect(m_viewModel.get(), &TimelineViewModel::eventsChanged,
            this, &TimelineWidget::onEventsChanged);
    connect(m_viewModel.get(), &TimelineViewModel::windowChanged,
            this, &TimelineWidget::onWindowChanged);
}

// ── Slots ─────────────────────────────────────────────────────────────────────

void TimelineWidget::onEventsChanged()
{
    update();
}

void TimelineWidget::onWindowChanged()
{
    updateScrollBar();
    update();
}

void TimelineWidget::onZoomIn()
{
    m_viewModel->zoom(2.0);
}

void TimelineWidget::onZoomOut()
{
    m_viewModel->zoom(0.5);
}

void TimelineWidget::onResetView()
{
    m_viewModel->setWindow(m_viewModel->worldStart(), m_viewModel->worldEnd());
}

void TimelineWidget::onScrollBarMoved(int value)
{
    if (m_scrollBarUpdating) return;

    // Convert scrollbar value (in years) to a pan delta
    const auto ws = m_viewModel->windowStart();
    const int currentYear = ws.era * 100000 + ws.year;
    const int delta = value - currentYear;
    if (delta != 0) {
        m_viewModel->panBy(delta);
    }
}

void TimelineWidget::onLaneToggled(int laneIndex, bool visible)
{
    m_viewModel->setLaneVisible(
        static_cast<Lane>(laneIndex), visible);
}

// ── Paint ─────────────────────────────────────────────────────────────────────

void TimelineWidget::paintEvent(QPaintEvent*)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    paintBackground(p);
    paintLaneSeparators(p);
    paintRuler(p);
    paintLaneLabels(p);
    paintEvents(p);

    if (m_hoveredEvent.has_value()) {
        paintTooltip(p);
    }
}

void TimelineWidget::paintBackground(QPainter& p)
{
    p.fillRect(rect(), QColor("#1e1e2e"));
}

void TimelineWidget::paintLaneSeparators(QPainter& p)
{
    const QRect cr = canvasRect();

    for (int i = 0; i <= LANE_COUNT; ++i) {
        const int y = cr.top() + i * laneHeight();
        p.setPen(QPen(QColor("#313244"), 1));
        p.drawLine(cr.left(), y, cr.right(), y);
    }

    // Alternating lane background tints
    for (int i = 0; i < LANE_COUNT; ++i) {
        const QRect laneRect(cr.left(), cr.top() + i * laneHeight(),
                             cr.width(), laneHeight());
        if (i % 2 == 0) {
            p.fillRect(laneRect, QColor("#181825"));
        } else {
            p.fillRect(laneRect, QColor("#1e1e2e"));
        }
    }
}

void TimelineWidget::paintRuler(QPainter& p)
{
    const QRect cr = canvasRect();

    // Ruler background
    p.fillRect(labelWidth(), 0, cr.width(), rulerHeight(), QColor("#11111b"));

    const auto ticks = computeRulerTicks();
    for (const auto& tick : ticks) {
        const int x = xForNormalisedX(tick.normalisedX);
        if (x < labelWidth() || x > width()) continue;

        if (tick.isMajor) {
            p.setPen(QPen(QColor("#6c7086"), 1));
            p.drawLine(x, 4, x, rulerHeight() - 4);

            p.setPen(QColor("#a6adc8"));
            QFont f = p.font();
            f.setPixelSize(10);
            p.setFont(f);
            p.drawText(x + 3, rulerHeight() - 6, tick.label);
        } else {
            p.setPen(QPen(QColor("#313244"), 1));
            p.drawLine(x, rulerHeight() - 10, x, rulerHeight() - 4);
        }
    }
}

void TimelineWidget::paintLaneLabels(QPainter& p)
{
    const QRect cr = canvasRect();

    p.fillRect(0, rulerHeight(), labelWidth(), cr.height(), QColor("#11111b"));
    p.setPen(QPen(QColor("#313244"), 1));
    p.drawLine(labelWidth(), rulerHeight(), labelWidth(), height());

    QFont f = p.font();
    f.setPixelSize(11);
    f.setBold(true);
    p.setFont(f);

    for (int i = 0; i < LANE_COUNT; ++i) {
        const int y = cr.top() + i * laneHeight();
        const QRect labelRect(0, y, labelWidth(), laneHeight());

        // Lane colour dot
        p.setPen(Qt::NoPen);
        p.setBrush(laneColour(static_cast<Lane>(i)));
        p.drawEllipse(QPoint(12, y + laneHeight() / 2), 5, 5);

        p.setPen(m_viewModel->isLaneVisible(static_cast<Lane>(i))
                 ? QColor("#cdd6f4") : QColor("#45475a"));
        p.drawText(labelRect.adjusted(24, 0, -4, 0),
                   Qt::AlignVCenter | Qt::AlignLeft,
                   LANE_NAMES[i]);
    }
}

void TimelineWidget::paintEvents(QPainter& p)
{
    for (const auto& event : m_viewModel->visibleEvents()) {
        const int laneIdx = static_cast<int>(event.lane);
        paintEvent(p, event, laneY(laneIdx), laneHeight());
    }
}

void TimelineWidget::paintEvent(
    QPainter& p,
    const TimelineViewModel::DisplayEvent& event,
    int laneTop, int laneH)
{
    const int x  = xForNormalisedX(event.normalisedX);
    const int cy = laneTop + laneH / 2;
    const int r  = EVENT_RADIUS;

    const bool isHovered = m_hoveredEvent.has_value()
                        && m_hoveredEvent->id == event.id;

    // Event stem (vertical line from ruler to dot)
    p.setPen(QPen(QColor("#45475a"), 1, Qt::DotLine));
    p.drawLine(x, laneTop, x, cy - r);

    // Event dot
    const QColor baseColor = eventColour(event.type);
    p.setPen(QPen(isHovered ? Qt::white : baseColor.darker(130), 1.5));
    p.setBrush(isHovered ? baseColor.lighter(130) : baseColor);
    p.drawEllipse(QPoint(x, cy), r, r);

    // Short label below dot (only if enough horizontal space)
    if (isHovered) {
        p.setPen(QColor("#cdd6f4"));
        QFont f = p.font();
        f.setPixelSize(10);
        p.setFont(f);
        // Truncate to avoid overlap
        const QFontMetrics fm(f);
        const QString label = fm.elidedText(event.title, Qt::ElideRight, 80);
        p.drawText(x - 40, cy + r + 14, 80, 16,
                   Qt::AlignCenter, label);
    }
}

void TimelineWidget::paintTooltip(QPainter& p)
{
    if (!m_hoveredEvent.has_value()) return;
    const auto& e = *m_hoveredEvent;

    const QString text = QString("%1\n%2\n%3")
        .arg(e.title)
        .arg(e.entityName)
        .arg(QString::fromStdString(e.when.display()));

    QFont f = p.font();
    f.setPixelSize(11);
    p.setFont(f);
    const QFontMetrics fm(f);
    const QRect textRect = fm.boundingRect(
        QRect(), Qt::TextWordWrap, text).adjusted(-8, -6, 8, 6);

    // Position tooltip above the hovered point, clamped to widget
    QPoint pos = m_tooltipPos - QPoint(textRect.width() / 2, textRect.height() + 20);
    pos.setX(std::max(4, std::min(pos.x(), width()  - textRect.width()  - 4)));
    pos.setY(std::max(4, std::min(pos.y(), height() - textRect.height() - 4)));

    const QRect bgRect = textRect.translated(pos);

    // Background
    p.setPen(QPen(QColor("#89b4fa"), 1));
    p.setBrush(QColor("#1e1e2e"));
    p.drawRoundedRect(bgRect, 6, 6);

    // Text
    p.setPen(QColor("#cdd6f4"));
    p.drawText(bgRect.adjusted(8, 6, -8, -6),
               Qt::TextWordWrap | Qt::AlignLeft, text);
}

// ── Mouse events ──────────────────────────────────────────────────────────────

void TimelineWidget::mousePressEvent(QMouseEvent* event)
{
    if (event->button() == Qt::MiddleButton) {
        m_isPanning = true;
        m_panStartPos         = event->pos();
        m_panStartWindowStart = m_viewModel->windowStart();
        setCursor(Qt::ClosedHandCursor);
        return;
    }

    if (event->button() == Qt::LeftButton) {
        const auto hit = hitTest(event->pos());
        if (hit.has_value()) {
            emit eventClicked(hit->id);
        }
    }
}

void TimelineWidget::mouseMoveEvent(QMouseEvent* event)
{
    if (m_isPanning) {
        // Convert pixel delta to year delta and pan
        const int dx      = event->pos().x() - m_panStartPos.x();
        const double span = static_cast<double>(
            m_viewModel->windowEnd().toSortKey()
            - m_viewModel->windowStart().toSortKey());
        const int paintW  = width() - labelWidth();
        if (paintW <= 0) return;

        const int64_t yearDelta = static_cast<int64_t>(
            -dx * span / static_cast<double>(paintW));

        m_viewModel->setWindow(
            TimePoint::fromSortKey(
                m_panStartWindowStart.toSortKey() + yearDelta),
            TimePoint::fromSortKey(
                m_panStartWindowStart.toSortKey() + yearDelta
                + static_cast<int64_t>(span))
        );
        return;
    }

    // Hover detection
    const auto hit = hitTest(event->pos());
    if (hit != m_hoveredEvent) {
        m_hoveredEvent = hit;
        m_tooltipPos   = event->pos();
        setCursor(hit.has_value() ? Qt::PointingHandCursor : Qt::ArrowCursor);
        update();
    }
}

void TimelineWidget::mouseReleaseEvent(QMouseEvent* event)
{
    if (event->button() == Qt::MiddleButton) {
        m_isPanning = false;
        setCursor(Qt::ArrowCursor);
    }
}

void TimelineWidget::wheelEvent(QWheelEvent* event)
{
    const double factor = event->angleDelta().y() > 0 ? 1.5 : 1.0 / 1.5;
    m_viewModel->zoom(factor);
    event->accept();
}

void TimelineWidget::resizeEvent(QResizeEvent*)
{
    update();
}

void TimelineWidget::leaveEvent(QEvent*)
{
    m_hoveredEvent.reset();
    update();
}

// ── Geometry ──────────────────────────────────────────────────────────────────

QRect TimelineWidget::canvasRect() const
{
    return QRect(labelWidth(), rulerHeight(),
                 width() - labelWidth(),
                 height() - rulerHeight() - m_scrollBar->height());
}

int TimelineWidget::laneY(int laneIndex) const
{
    return canvasRect().top() + laneIndex * laneHeight();
}

int TimelineWidget::laneHeight() const
{
    const int available = height() - rulerHeight() - m_scrollBar->height();
    return std::max(MIN_LANE_H, available / LANE_COUNT);
}

int TimelineWidget::xForNormalisedX(double nx) const
{
    const QRect cr = canvasRect();
    return cr.left() + static_cast<int>(nx * cr.width());
}

double TimelineWidget::normalisedXForX(int x) const
{
    const QRect cr = canvasRect();
    if (cr.width() <= 0) return 0.0;
    return static_cast<double>(x - cr.left()) / cr.width();
}

// ── Ruler ticks ───────────────────────────────────────────────────────────────

std::vector<TimelineWidget::RulerTick>
TimelineWidget::computeRulerTicks() const
{
    std::vector<RulerTick> ticks;

    const int64_t winStart = m_viewModel->windowStart().toSortKey();
    const int64_t winEnd   = m_viewModel->windowEnd().toSortKey();
    const int64_t span     = winEnd - winStart;
    if (span <= 0) return ticks;

    // Choose tick interval based on visible span
    int64_t majorInterval = 1;
    const int64_t intervals[] = { 1, 5, 10, 25, 50, 100, 250, 500,
                                   1000, 5000, 10000, 50000 };
    const int targetMajorTicks = 8;
    for (int64_t iv : intervals) {
        if (span / iv <= targetMajorTicks) { majorInterval = iv; break; }
        majorInterval = iv;
    }
    const int64_t minorInterval = majorInterval / 5 > 0
                                ? majorInterval / 5 : 1;

    // Snap to first tick >= winStart
    const int64_t firstMinor = (winStart / minorInterval) * minorInterval;

    for (int64_t t = firstMinor; t <= winEnd; t += minorInterval) {
        if (t < winStart) continue;
        const double nx = static_cast<double>(t - winStart)
                        / static_cast<double>(span);
        const bool isMajor = (t % majorInterval == 0);
        const TimePoint tp = TimePoint::fromSortKey(t);

        RulerTick tick;
        tick.normalisedX = nx;
        tick.isMajor     = isMajor;
        if (isMajor) {
            tick.label = QString::fromStdString(tp.display());
        }
        ticks.push_back(std::move(tick));
    }
    return ticks;
}

// ── Hit testing ───────────────────────────────────────────────────────────────

std::optional<TimelineViewModel::DisplayEvent>
TimelineWidget::hitTest(const QPoint& pos) const
{
    const QRect cr = canvasRect();
    if (!cr.contains(pos)) return std::nullopt;

    const double nx       = normalisedXForX(pos.x());
    const int    yInCanvas = pos.y() - cr.top();
    const int    laneIdx  = yInCanvas / laneHeight();
    if (laneIdx < 0 || laneIdx >= LANE_COUNT) return std::nullopt;

    // Tolerance: EVENT_RADIUS pixels expressed as normalised X
    const double toleranceNx = static_cast<double>(EVENT_RADIUS + 2)
                              / static_cast<double>(cr.width());

    return m_viewModel->eventAtNormalisedX(
        nx, static_cast<Lane>(laneIdx), toleranceNx);
}

// ── Scroll bar ────────────────────────────────────────────────────────────────

void TimelineWidget::updateScrollBar()
{
    m_scrollBarUpdating = true;

    const int64_t worldStart = m_viewModel->worldStart().toSortKey();
    const int64_t worldEnd   = m_viewModel->worldEnd().toSortKey();
    const int64_t winStart   = m_viewModel->windowStart().toSortKey();
    const int64_t winEnd     = m_viewModel->windowEnd().toSortKey();
    const int64_t span       = winEnd - winStart;

    m_scrollBar->setRange(static_cast<int>(worldStart),
                          static_cast<int>(worldEnd - span));
    m_scrollBar->setPageStep(static_cast<int>(span));
    m_scrollBar->setValue(static_cast<int>(winStart));

    m_scrollBarUpdating = false;
}

// ── Colour scheme ─────────────────────────────────────────────────────────────

QColor TimelineWidget::laneColour(Lane lane) const
{
    switch (lane) {
        case Lane::Character: return QColor("#89b4fa");  // blue
        case Lane::Faction:   return QColor("#f38ba8");  // red
        case Lane::Place:     return QColor("#a6e3a1");  // green
        case Lane::Scene:     return QColor("#fab387");  // peach
        case Lane::Custom:    return QColor("#cba6f7");  // mauve
    }
    return QColor("#cdd6f4");
}

QColor TimelineWidget::eventColour(
    Domain::TimelineEvent::EventType type) const
{
    using ET = Domain::TimelineEvent::EventType;
    switch (type) {
        case ET::CharacterBorn:           return QColor("#89dceb");
        case ET::CharacterDied:           return QColor("#6c7086");
        case ET::CharacterEvolved:        return QColor("#89b4fa");
        case ET::CharacterMoved:          return QColor("#74c7ec");
        case ET::CharacterJoinedFaction:  return QColor("#cba6f7");
        case ET::CharacterLeftFaction:    return QColor("#f38ba8");
        case ET::FactionFounded:          return QColor("#a6e3a1");
        case ET::FactionDissolved:        return QColor("#f38ba8");
        case ET::FactionEvolved:          return QColor("#fab387");
        case ET::PlaceEstablished:        return QColor("#a6e3a1");
        case ET::PlaceDestroyed:          return QColor("#f38ba8");
        case ET::PlaceEvolved:            return QColor("#94e2d5");
        case ET::SceneOccurred:           return QColor("#fab387");
        case ET::Custom:
        default:                          return QColor("#cba6f7");
    }
}

} // namespace CF::UI