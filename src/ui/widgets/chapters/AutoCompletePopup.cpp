#include "AutoCompletePopup.h"

#include <QKeyEvent>
#include <QListWidgetItem>
#include <QLabel>
#include <QHBoxLayout>

namespace CF::UI {

using namespace CF::Services;

AutoCompletePopup::AutoCompletePopup(QWidget* parent)
    : QFrame(parent, Qt::ToolTip | Qt::FramelessWindowHint)
{
    setObjectName("autoCompletePopup");
    setFixedWidth(320);
    setMaximumHeight(280);

    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(4, 4, 4, 4);
    layout->setSpacing(0);

    m_list = new QListWidget(this);
    m_list->setObjectName("autoCompleteList");
    m_list->setFrameShape(QFrame::NoFrame);
    m_list->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_list->setSpacing(1);
    layout->addWidget(m_list);

    connect(m_list, &QListWidget::itemActivated,
            this, &AutoCompletePopup::onItemActivated);
    connect(m_list, &QListWidget::itemClicked,
            this, &AutoCompletePopup::onItemActivated);

    setStyleSheet(R"(
        #autoCompletePopup {
            background: #313244;
            border: 1px solid #45475a;
            border-radius: 6px;
        }
        #autoCompleteList {
            background: transparent;
            color: #cdd6f4;
        }
        #autoCompleteList::item {
            padding: 5px 8px;
            border-radius: 4px;
        }
        #autoCompleteList::item:selected {
            background: #45475a;
        }
        #autoCompleteList::item:hover {
            background: #3d3f52;
        }
    )");
}

void AutoCompletePopup::setSuggestions(
    const std::vector<AutoCompleteSuggestion>& suggestions)
{
    m_suggestions = suggestions;
    m_list->clear();

    for (const auto& s : suggestions) {
        auto* item = new QListWidgetItem(m_list);

        // Build a two-line display: name (bold) + detail (dim)
        const QString typeTag  = typeLabel(s.type);
        const QString display  = QString("%1\n%2  ·  %3")
            .arg(QString::fromStdString(s.name))
            .arg(typeTag)
            .arg(QString::fromStdString(s.detail));

        item->setText(display);
        item->setForeground(typeColour(s.type));
        item->setData(Qt::UserRole, QString::fromStdString(s.insertionText()));
        m_list->addItem(item);
    }

    if (m_list->count() > 0) {
        m_list->setCurrentRow(0);
    }

    // Resize to content
    const int rowH   = 46;
    const int maxRows = 5;
    const int rows   = std::min(m_list->count(), maxRows);
    setFixedHeight(rows * rowH + 8);
}

void AutoCompletePopup::setAcceptCallback(AcceptCallback cb)
{
    m_callback = std::move(cb);
}

void AutoCompletePopup::navigateUp()
{
    const int row = m_list->currentRow();
    if (row > 0) m_list->setCurrentRow(row - 1);
}

void AutoCompletePopup::navigateDown()
{
    const int row = m_list->currentRow();
    if (row < m_list->count() - 1) m_list->setCurrentRow(row + 1);
}

void AutoCompletePopup::acceptCurrent()
{
    auto* item = m_list->currentItem();
    if (item) onItemActivated(item);
}

bool AutoCompletePopup::hasSuggestions() const
{
    return m_list->count() > 0;
}

void AutoCompletePopup::keyPressEvent(QKeyEvent* event)
{
    // Forward navigation keys to the list
    switch (event->key()) {
        case Qt::Key_Up:     navigateUp();    break;
        case Qt::Key_Down:   navigateDown();  break;
        case Qt::Key_Return:
        case Qt::Key_Tab:    acceptCurrent(); break;
        case Qt::Key_Escape: hide();          break;
        default: QFrame::keyPressEvent(event);
    }
}

void AutoCompletePopup::onItemActivated(QListWidgetItem* item)
{
    if (!item || !m_callback) return;
    m_callback(item->data(Qt::UserRole).toString());
    hide();
}

QString AutoCompletePopup::typeLabel(AutoCompleteSuggestion::EntityType t)
{
    switch (t) {
        case AutoCompleteSuggestion::EntityType::Character: return "Character";
        case AutoCompleteSuggestion::EntityType::Place:     return "Place";
        case AutoCompleteSuggestion::EntityType::Faction:   return "Faction";
        case AutoCompleteSuggestion::EntityType::Scene:     return "Scene";
    }
    return "Entity";
}

QColor AutoCompletePopup::typeColour(AutoCompleteSuggestion::EntityType t)
{
    switch (t) {
        case AutoCompleteSuggestion::EntityType::Character: return QColor("#89b4fa");
        case AutoCompleteSuggestion::EntityType::Place:     return QColor("#a6e3a1");
        case AutoCompleteSuggestion::EntityType::Faction:   return QColor("#f38ba8");
        case AutoCompleteSuggestion::EntityType::Scene:     return QColor("#fab387");
    }
    return QColor("#cdd6f4");
}

} // namespace CF::UI