#pragma once

#include <QWidget>
#include <QListView>
#include <QLineEdit>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QMenu>
#include <memory>

#include "ui/viewmodels/CharacterViewModel.h"

namespace CF::UI {

/**
 * CharacterListWidget shows the character roster in a searchable list.
 * It is a pure view — it holds a ViewModel but no business logic.
 *
 * Selecting a character emits characterSelected(id) which the
 * MainWindow uses to open the editor panel.
 */
class CharacterListWidget : public QWidget {
    Q_OBJECT

public:
    explicit CharacterListWidget(
        std::shared_ptr<CharacterViewModel> viewModel,
        QWidget* parent = nullptr);

signals:
    void characterSelected(Domain::CharacterId id);
    void createRequested();

public slots:
    void refresh();

private slots:
    void onSearchTextChanged(const QString& text);
    void onSelectionChanged(const QModelIndex& current,
                            const QModelIndex& previous);
    void onCreateClicked();
    void onContextMenuRequested(const QPoint& pos);
    void onDeleteSelected();
    void onError(const QString& message);

private:
    void setupUi();
    void setupConnections();

    std::shared_ptr<CharacterViewModel> m_viewModel;

    QLineEdit*   m_searchBox    = nullptr;
    QListView*   m_listView     = nullptr;
    QPushButton* m_createButton = nullptr;
    QLabel*      m_countLabel   = nullptr;
};

} // namespace CF::UI