#include "EntityBadgeWidget.h"

#include <QPainter>
#include <QMouseEvent>
#include <QEnterEvent>

namespace CF::UI {

EntityBadgeWidget::EntityBadgeWidget(EntityType type, QWidget* parent)
    : QWidget(parent)
    , m_type(type)
{
    build();
}

EntityBadgeWidget::EntityBadgeWidget(
    EntityType type, const QString& name, QWidget* parent)
    : QWidget(parent)
    , m_type(type)
    , m_name(name)
{
    build();
}

void EntityBadgeWidget::build()
{
    setObjectName("entityBadge");
    setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

    auto* layout = new QHBoxLayout(this);
    layout->setContentsMargins(6, 2, 8, 2);
    layout->setSpacing(5);

    // Coloured dot
    m_dotLabel = new QLabel(this);
    m_dotLabel->setFixedSize(7, 7);

    // Text label
    m_textLabel = new QLabel(this);
    m_textLabel->setObjectName("countLabel");

    layout->addWidget(m_dotLabel);
    layout->addWidget(m_textLabel);

    updateStyle();
}

void EntityBadgeWidget::setName(const QString& name)
{
    m_name = name;
    updateStyle();
}

void EntityBadgeWidget::setType(EntityType type)
{
    m_type = type;
    updateStyle();
}

void EntityBadgeWidget::setClickable(bool clickable)
{
    m_clickable = clickable;
    setCursor(clickable ? Qt::PointingHandCursor : Qt::ArrowCursor);
}

EntityBadgeWidget::EntityType EntityBadgeWidget::entityType() const
{
    return m_type;
}

QString EntityBadgeWidget::entityName() const
{
    return m_name;
}

// ── Static helpers ────────────────────────────────────────────────────────────

QColor EntityBadgeWidget::colourForType(EntityType type)
{
    switch (type) {
        case EntityType::Character: return QColor("#89b4fa");
        case EntityType::Place:     return QColor("#a6e3a1");
        case EntityType::Faction:   return QColor("#f38ba8");
        case EntityType::Scene:     return QColor("#fab387");
        case EntityType::Chapter:   return QColor("#cba6f7");
        case EntityType::Custom:    return QColor("#94e2d5");
    }
    return QColor("#cdd6f4");
}

QString EntityBadgeWidget::labelForType(EntityType type)
{
    switch (type) {
        case EntityType::Character: return "Character";
        case EntityType::Place:     return "Place";
        case EntityType::Faction:   return "Faction";
        case EntityType::Scene:     return "Scene";
        case EntityType::Chapter:   return "Chapter";
        case EntityType::Custom:    return "Custom";
    }
    return "Entity";
}

// ── Events ────────────────────────────────────────────────────────────────────

void EntityBadgeWidget::mousePressEvent(QMouseEvent* event)
{
    if (m_clickable && event->button() == Qt::LeftButton) {
        emit clicked();
    }
}

void EntityBadgeWidget::enterEvent(QEnterEvent*)
{
    if (m_clickable) {
        m_hovered = true;
        update();
    }
}

void EntityBadgeWidget::leaveEvent(QEvent*)
{
    m_hovered = false;
    update();
}

void EntityBadgeWidget::paintEvent(QPaintEvent* event)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    const QColor colour = colourForType(m_type);

    // Background pill
    QColor bg = colour;
    bg.setAlpha(m_hovered ? 40 : 20);
    QColor border = colour;
    border.setAlpha(m_hovered ? 120 : 60);

    p.setPen(QPen(border, 1));
    p.setBrush(bg);
    p.drawRoundedRect(rect().adjusted(0, 0, -1, -1), 4, 4);

    // Draw the coloured dot manually (QLabel doesn't support
    // circular backgrounds cleanly at small sizes)
    if (m_dotLabel) {
        const QRect dotRect = m_dotLabel->geometry();
        const QPoint centre = dotRect.center();
        p.setPen(Qt::NoPen);
        p.setBrush(colour);
        p.drawEllipse(centre, 3, 3);
    }

    QWidget::paintEvent(event);
}

void EntityBadgeWidget::updateStyle()
{
    const QColor colour = colourForType(m_type);
    const QString label = labelForType(m_type);

    // Build text: "Name · Type" or just "Type"
    const QString text = m_name.isEmpty()
        ? label
        : QString("%1 · %2").arg(m_name, label);

    if (m_textLabel) {
        m_textLabel->setText(text);
        m_textLabel->setStyleSheet(
            QString("color: %1; font-size: 11px; font-weight: bold;")
            .arg(colour.name()));
    }

    update();
}

} // namespace CF::UI