#pragma once

#include <QWidget>
#include <QListView>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <memory>

#include "ui/viewmodels/FactionViewModel.h"

namespace CF::UI {

class FactionListWidget : public QWidget {
    Q_OBJECT

public:
    explicit FactionListWidget(
        std::shared_ptr<FactionViewModel> viewModel,
        QWidget* parent = nullptr);

    void refresh();

signals:
    void factionSelected(Domain::FactionId id);
    void createRequested();

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

    std::shared_ptr<FactionViewModel> m_viewModel;

    QLineEdit*   m_searchBox    = nullptr;
    QListView*   m_listView     = nullptr;
    QPushButton* m_createButton = nullptr;
    QLabel*      m_countLabel   = nullptr;
};

} // namespace CF::UI