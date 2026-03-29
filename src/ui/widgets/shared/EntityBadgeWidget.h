#pragma once

#include <QWidget>
#include <QLabel>
#include <QHBoxLayout>
#include <QString>
#include <QColor>

namespace CF::UI {

/**
 * EntityBadgeWidget is a small coloured pill that displays an entity
 * type label and optional name. Used throughout the UI wherever an
 * entity reference needs a compact visual representation.
 *
 * Examples:
 *   [● Character]   [● Place]   [● Faction]
 *   [● Kael · Character]
 *
 * The badge is clickable — emits clicked() when the user presses it,
 * allowing parent widgets to navigate to the referenced entity.
 */
class EntityBadgeWidget : public QWidget {
    Q_OBJECT

public:
    enum class EntityType {
        Character,
        Place,
        Faction,
        Scene,
        Chapter,
        Custom
    };

    // Compact badge: just the type label
    explicit EntityBadgeWidget(EntityType type,
                                QWidget* parent = nullptr);

    // Full badge: name + type label
    EntityBadgeWidget(EntityType type,
                      const QString& name,
                      QWidget* parent = nullptr);

    // Update content after construction
    void setName(const QString& name);
    void setType(EntityType type);
    void setClickable(bool clickable);

    [[nodiscard]] EntityType  entityType() const;
    [[nodiscard]] QString     entityName() const;

    // Returns a colour for a given entity type (used externally too)
    [[nodiscard]] static QColor  colourForType(EntityType type);
    [[nodiscard]] static QString labelForType (EntityType type);

signals:
    void clicked();

protected:
    void mousePressEvent(QMouseEvent* event) override;
    void enterEvent(QEnterEvent* event)      override;
    void leaveEvent(QEvent* event)           override;
    void paintEvent(QPaintEvent* event)      override;

private:
    void build();
    void updateStyle();

    EntityType m_type;
    QString    m_name;
    bool       m_clickable = false;
    bool       m_hovered   = false;

    QLabel* m_dotLabel  = nullptr;
    QLabel* m_textLabel = nullptr;
};

} // namespace CF::UI