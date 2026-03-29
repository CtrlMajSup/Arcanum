#pragma once

#include <QWidget>
#include <QListView>
#include <QPushButton>
#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <memory>

#include "ui/viewmodels/ChapterViewModel.h"

namespace CF::UI {

/**
 * ChapterListWidget shows the ordered chapter list.
 * Chapters can be reordered by drag-and-drop.
 * Selecting a chapter notifies the ViewModel to set it as active.
 */
class ChapterListWidget : public QWidget {
    Q_OBJECT

public:
    explicit ChapterListWidget(
        std::shared_ptr<ChapterViewModel> viewModel,
        QWidget* parent = nullptr);

    void refresh();

signals:
    void chapterSelected(int row);

private slots:
    void onSelectionChanged(const QModelIndex& current,
                            const QModelIndex& previous);
    void onCreateClicked();
    void onDeleteClicked();
    void onError(const QString& message);

private:
    void setupUi();
    void setupConnections();

    std::shared_ptr<ChapterViewModel> m_viewModel;
    QListView*   m_listView     = nullptr;
    QPushButton* m_createButton = nullptr;
    QPushButton* m_deleteButton = nullptr;
    QLabel*      m_countLabel   = nullptr;
};

} // namespace CF::UI