#pragma once

#include <QAbstractListModel>
#include <QObject>
#include <QString>
#include <memory>
#include <vector>

#include "services/CharacterService.h"
#include "services/PlaceService.h"
#include "services/FactionService.h"
#include "domain/entities/Character.h"
#include "core/Logger.h"

namespace CF::UI {

/**
 * CharacterViewModel bridges CharacterService and Qt's model/view system.
 *
 * It owns a local cache of characters and exposes it as a QAbstractListModel
 * so views can bind to it directly. All mutations go through the service —
 * the ViewModel only updates its cache on success.
 *
 * Qt signals are emitted after service calls succeed, never speculatively.
 */
class CharacterViewModel : public QAbstractListModel {
    Q_OBJECT

public:
    // Custom roles for QML / delegate access
    enum Roles {
        IdRole       = Qt::UserRole + 1,
        NameRole,
        SpeciesRole,
        BiographyRole,
        BornEraRole,
        BornYearRole,
        IsAliveRole
    };

    explicit CharacterViewModel(
        std::shared_ptr<Services::CharacterService> characterService,
        std::shared_ptr<Services::PlaceService>     placeService,
        std::shared_ptr<Services::FactionService>   factionService,
        Core::Logger& logger,
        QObject* parent = nullptr);

    // ── QAbstractListModel interface ──────────────────────────────────────────
    int      rowCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    // ── Commands (called by widgets) ──────────────────────────────────────────
    void loadAll();
    void search(const QString& nameFragment);

    // Returns the Character at a given list row — used by the editor widget
    [[nodiscard]] std::optional<Domain::Character>
    characterAt(int row) const;

    // Returns a character by ID — used when navigating from other widgets
    [[nodiscard]] std::optional<Domain::Character>
    characterById(Domain::CharacterId id) const;

    // CRUD
    void createCharacter(const QString& name,
                         const QString& species,
                         int bornEra, int bornYear);

    void updateCharacter(const Domain::Character& updated);
    void deleteCharacter(Domain::CharacterId id);

    // Location
    void moveCharacter(Domain::CharacterId characterId,
                       Domain::PlaceId     newPlaceId,
                       int era, int year,
                       const QString& note);

    // Evolution
    void addEvolution(Domain::CharacterId id,
                      int era, int year,
                      const QString& description);
    void removeEvolution(Domain::CharacterId id, int era, int year);

    // Faction membership
    void joinFaction(Domain::CharacterId characterId,
                     Domain::FactionId   factionId,
                     int era, int year,
                     const QString& role);
    void leaveFaction(Domain::CharacterId characterId,
                      Domain::FactionId   factionId,
                      int era, int year);

    // Relationship
    void setRelationship(Domain::CharacterId ownerId,
                         const Domain::Relationship& rel);
    void removeRelationship(Domain::CharacterId ownerId,
                            Domain::CharacterId targetId);

signals:
    void errorOccurred(const QString& message);
    void characterCreated(qint64 characterId);
    void characterUpdated(qint64 characterId);
    void characterDeleted(qint64 characterId);

private:
    void onServiceChange(Domain::DomainEvent event);
    void refreshCharacter(Domain::CharacterId id);

    // Finds the list index of a character by ID — returns -1 if not found
    [[nodiscard]] int indexOfId(Domain::CharacterId id) const;

    std::shared_ptr<Services::CharacterService> m_characterService;
    std::shared_ptr<Services::PlaceService>     m_placeService;
    std::shared_ptr<Services::FactionService>   m_factionService;
    Core::Logger& m_logger;

    std::vector<Domain::Character> m_characters;
    QString                        m_currentFilter;
};

} // namespace CF::UI