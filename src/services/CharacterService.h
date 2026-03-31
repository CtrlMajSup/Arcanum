#pragma once

#include "core/Result.h"
#include "core/Logger.h"
#include "domain/entities/Character.h"
#include "domain/events/DomainEvent.h"
#include "repositories/ICharacterRepository.h"
#include "repositories/ITimelineRepository.h"

#include <memory>
#include <functional>
#include <vector>
#include <string>

namespace CF::Services {

/**
 * CharacterService owns all business rules for characters.
 *
 * Responsibilities:
 *  - Validate character data before persistence
 *  - Emit TimelineEvents when character state changes
 *  - Coordinate between character and timeline repositories
 *  - Notify the UI layer via the onChange callback (no Qt signals here)
 *
 * The service never returns raw repository errors verbatim —
 * it wraps them with context-aware messages.
 */
class CharacterService {
public:
    using ChangeCallback = std::function<void(Domain::DomainEvent)>;

    explicit CharacterService(
        std::shared_ptr<Repositories::ICharacterRepository> characterRepo,
        std::shared_ptr<Repositories::ITimelineRepository>  timelineRepo,
        Core::Logger& logger);

    // ── Queries ───────────────────────────────────────────────────────────────

    [[nodiscard]] Core::Result<Domain::Character>
    getById(Domain::CharacterId id) const;

    [[nodiscard]] Core::Result<std::vector<Domain::Character>>
    getAll() const;

    [[nodiscard]] Core::Result<std::vector<Domain::Character>>
    search(const std::string& nameFragment) const;

    [[nodiscard]] Core::Result<std::vector<Domain::Character>>
    getAtPlace(Domain::PlaceId placeId) const;

    [[nodiscard]] Core::Result<std::vector<Domain::Character>>
    getInFaction(Domain::FactionId factionId) const;

    // ── Mutations ─────────────────────────────────────────────────────────────

    [[nodiscard]] Core::Result<Domain::Character>
    createCharacter(const std::string& name,
                    const std::string& species,
                    const Domain::TimePoint& born);

    [[nodiscard]] Core::Result<Domain::Character>
    updateCharacter(const Domain::Character& updated);

    [[nodiscard]] Core::Result<void>
    deleteCharacter(Domain::CharacterId id);

    // ── Location management ───────────────────────────────────────────────────

    // Moves a character to a new place, closing the previous open stint
    [[nodiscard]] Core::Result<void>
    moveCharacter(Domain::CharacterId characterId,
                  Domain::PlaceId     newPlaceId,
                  const Domain::TimePoint& when,
                  const std::string&  note = "");

    // ── Evolution management ──────────────────────────────────────────────────

    [[nodiscard]] Core::Result<void>
    addEvolution(Domain::CharacterId characterId,
                 const Domain::TimePoint& at,
                 const std::string& description);

    [[nodiscard]] Core::Result<void>
    removeEvolution(Domain::CharacterId characterId,
                    const Domain::TimePoint& at);

    // ── Faction membership ────────────────────────────────────────────────────

    [[nodiscard]] Core::Result<void>
    joinFaction(Domain::CharacterId characterId,
                Domain::FactionId   factionId,
                const Domain::TimePoint& when,
                const std::string& role = "");

    [[nodiscard]] Core::Result<void>
    leaveFaction(Domain::CharacterId characterId,
                 Domain::FactionId   factionId,
                 const Domain::TimePoint& when);

    // ── Relationships ─────────────────────────────────────────────────────────

    [[nodiscard]] Core::Result<void>
    setRelationship(Domain::CharacterId ownerId,
                    const Domain::Relationship& relationship);

    [[nodiscard]] Core::Result<void>
    removeRelationship(Domain::CharacterId ownerId,
                       Domain::CharacterId targetId);

    // ── Observer registration ─────────────────────────────────────────────────

    // The UI layer registers a callback here — no Qt dependency needed
    void setOnChangeCallback(ChangeCallback callback);

private:
    [[nodiscard]] Core::Result<void> validateCharacter(const Domain::Character& c) const;
    [[nodiscard]] static Domain::Relationship::Type
    reciprocalType(Domain::Relationship::Type type);

    void emitTimelineEvent(Domain::TimelineEvent::EventType type,
                           Domain::TimelineEvent::Subject   subject,
                           const Domain::TimePoint&         when,
                           const std::string&               title,
                           const std::string&               description = "");

    void notifyChange(Domain::DomainEvent::Kind      kind,
                      Domain::DomainEvent::EntityType type,
                      int64_t                         id,
                      const std::string&              description);

    std::shared_ptr<Repositories::ICharacterRepository> m_characterRepo;
    std::shared_ptr<Repositories::ITimelineRepository>  m_timelineRepo;
    Core::Logger&  m_logger;
    ChangeCallback m_onChangeCallback;
};

} // namespace CF::Services