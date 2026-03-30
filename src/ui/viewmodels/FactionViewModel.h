#pragma once

#include <QAbstractListModel>
#include <QObject>
#include <memory>
#include <vector>
#include <optional>

#include "services/FactionService.h"
#include "domain/entities/Faction.h"
#include "core/Logger.h"

namespace CF::UI {

class FactionViewModel : public QAbstractListModel {
    Q_OBJECT

public:
    enum Roles {
        IdRole          = Qt::UserRole + 1,
        NameRole,
        TypeRole,
        IsActiveRole
    };

    explicit FactionViewModel(
        std::shared_ptr<Services::FactionService> factionService,
        Core::Logger& logger,
        QObject* parent = nullptr);

    // ── QAbstractListModel ────────────────────────────────────────────────────
    int      rowCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    // ── Commands ──────────────────────────────────────────────────────────────
    void loadAll();
    void search(const QString& nameFragment);

    [[nodiscard]] std::optional<Domain::Faction>
    factionAt(int row) const;

    [[nodiscard]] std::optional<Domain::Faction>
    factionById(Domain::FactionId id) const;

    void createFaction(const QString& name,
                       const QString& type,
                       int foundedEra, int foundedYear);

    void updateFaction(const Domain::Faction& updated);

    void dissolveFaction(Domain::FactionId id,
                         int era, int year);

    void deleteFaction(Domain::FactionId id);

    void addEvolution(Domain::FactionId id,
                      int era, int year,
                      const QString& description);

    void removeEvolution(Domain::FactionId id, int era, int year);

    // Shared service access — for FactionPickerDialog
    [[nodiscard]] std::shared_ptr<Services::FactionService> factionService() const
    {
        return m_factionService;
    }

signals:
    void errorOccurred(const QString& message);
    void factionCreated(qint64 factionId);
    void factionUpdated(qint64 factionId);
    void factionDeleted(qint64 factionId);

private:
    void onServiceChange(Domain::DomainEvent event);
    void refreshFaction(Domain::FactionId id);
    [[nodiscard]] int indexOfId(Domain::FactionId id) const;

    std::shared_ptr<Services::FactionService> m_factionService;
    Core::Logger& m_logger;

    std::vector<Domain::Faction> m_factions;
    QString                      m_currentFilter;
};

} // namespace CF::UI