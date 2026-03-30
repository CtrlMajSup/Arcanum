#pragma once

#include <QAbstractListModel>
#include <QObject>
#include <memory>
#include <vector>
#include <optional>

#include "services/PlaceService.h"
#include "domain/entities/Place.h"
#include "core/Logger.h"

namespace CF::UI {

class PlaceViewModel : public QAbstractListModel {
    Q_OBJECT

public:
    enum Roles {
        IdRole      = Qt::UserRole + 1,
        NameRole,
        TypeRole,
        RegionRole,
        IsMobileRole
    };

    explicit PlaceViewModel(
        std::shared_ptr<Services::PlaceService> placeService,
        Core::Logger& logger,
        QObject* parent = nullptr);

    // ── QAbstractListModel ────────────────────────────────────────────────────
    int      rowCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    // ── Commands ──────────────────────────────────────────────────────────────
    void loadAll();
    void search(const QString& nameFragment);

    [[nodiscard]] std::optional<Domain::Place> placeAt(int row)          const;
    [[nodiscard]] std::optional<Domain::Place> placeById(Domain::PlaceId id) const;

    void createPlace(const QString& name,
                     const QString& type,
                     const QString& region,
                     bool isMobile);

    void updatePlace(const Domain::Place& updated);
    void deletePlace(Domain::PlaceId id);

    void addEvolution(Domain::PlaceId id,
                      int era, int year,
                      const QString& description);
    void removeEvolution(Domain::PlaceId id, int era, int year);

    // Shared service access — for PlacePickerDialog
    [[nodiscard]] std::shared_ptr<Services::PlaceService> placeService() const
    {
        return m_placeService;
    }

signals:
    void errorOccurred(const QString& message);
    void placeCreated(qint64 placeId);
    void placeUpdated(qint64 placeId);
    void placeDeleted(qint64 placeId);

private:
    void onServiceChange(Domain::DomainEvent event);
    void refreshPlace(Domain::PlaceId id);
    [[nodiscard]] int indexOfId(Domain::PlaceId id) const;

    std::shared_ptr<Services::PlaceService> m_placeService;
    Core::Logger& m_logger;

    std::vector<Domain::Place> m_places;
    QString                    m_currentFilter;
};

} // namespace CF::UI