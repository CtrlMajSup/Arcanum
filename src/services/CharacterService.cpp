#include "CharacterService.h"
#include "core/Assert.h"

namespace CF::Services {

using namespace CF::Domain;
using namespace CF::Core;

CharacterService::CharacterService(
    std::shared_ptr<Repositories::ICharacterRepository> characterRepo,
    std::shared_ptr<Repositories::ITimelineRepository>  timelineRepo,
    Core::Logger& logger)
    : m_characterRepo(std::move(characterRepo))
    , m_timelineRepo(std::move(timelineRepo))
    , m_logger(logger)
{
    CF_REQUIRE(m_characterRepo != nullptr, "CharacterRepository must not be null");
    CF_REQUIRE(m_timelineRepo  != nullptr, "TimelineRepository must not be null");
}

// ── Queries ───────────────────────────────────────────────────────────────────

Result<Character> CharacterService::getById(CharacterId id) const
{
    CF_ASSERT(id.isValid(), "getById called with invalid CharacterId");
    return m_characterRepo->findById(id);
}

Result<std::vector<Character>> CharacterService::getAll() const
{
    return m_characterRepo->findAll();
}

Result<std::vector<Character>>
CharacterService::search(const std::string& nameFragment) const
{
    if (nameFragment.empty()) return getAll();
    return m_characterRepo->findByNameContaining(nameFragment);
}

Result<std::vector<Character>>
CharacterService::getAtPlace(PlaceId placeId) const
{
    return m_characterRepo->findByPlace(placeId);
}

Result<std::vector<Character>>
CharacterService::getInFaction(FactionId factionId) const
{
    return m_characterRepo->findByFaction(factionId);
}

// ── createCharacter ───────────────────────────────────────────────────────────

Result<Character> CharacterService::createCharacter(
    const std::string& name,
    const std::string& species,
    const TimePoint&   born)
{
    if (name.empty()) {
        return Result<Character>::err(
            AppError::validation("Character name cannot be empty"));
    }

    Character c;
    c.name    = name;
    c.species = species;
    c.born    = born;

    auto saveResult = m_characterRepo->save(c);
    if (saveResult.isErr()) return saveResult;

    const auto& saved = saveResult.value();

    // Automatically emit a Born timeline event
    emitTimelineEvent(
        TimelineEvent::EventType::CharacterBorn,
        saved.id,
        born,
        saved.name + " was born",
        "Era " + std::to_string(born.era) + ", Year " + std::to_string(born.year)
    );

    notifyChange(DomainEvent::Kind::EntityCreated,
                 DomainEvent::EntityType::Character,
                 saved.id.value(),
                 "Character created: " + saved.name);

    m_logger.info("Character created: " + saved.name + " (id="
                  + saved.id.toString() + ")", "CharacterService");

    return saveResult;
}

// ── updateCharacter ───────────────────────────────────────────────────────────

Result<Character> CharacterService::updateCharacter(const Character& updated)
{
    auto validation = validateCharacter(updated);
    if (validation.isErr()) {
        return Result<Character>::err(validation.error());
    }

    if (!m_characterRepo->exists(updated.id)) {
        return Result<Character>::err(
            AppError::notFound("Character not found: " + updated.id.toString()));
    }

    auto result = m_characterRepo->update(updated);
    if (result.isOk()) {
        notifyChange(DomainEvent::Kind::EntityUpdated,
                     DomainEvent::EntityType::Character,
                     updated.id.value(),
                     "Character updated: " + updated.name);
    }
    return result;
}

// ── deleteCharacter ───────────────────────────────────────────────────────────

Result<void> CharacterService::deleteCharacter(CharacterId id)
{
    // Load first so we can log the name and notify with context
    auto existing = m_characterRepo->findById(id);
    if (existing.isErr()) return Result<void>::err(existing.error());

    auto result = m_characterRepo->remove(id);
    if (result.isOk()) {
        notifyChange(DomainEvent::Kind::EntityDeleted,
                     DomainEvent::EntityType::Character,
                     id.value(),
                     "Character deleted: " + existing.value().name);
        m_logger.info("Character deleted: " + existing.value().name,
                      "CharacterService");
    }
    return result;
}

// ── moveCharacter ─────────────────────────────────────────────────────────────

Result<void> CharacterService::moveCharacter(
    CharacterId characterId,
    PlaceId     newPlaceId,
    const TimePoint& when,
    const std::string& note)
{
    auto charResult = m_characterRepo->findById(characterId);
    if (charResult.isErr()) return Result<void>::err(charResult.error());

    const auto& character = charResult.value();

    // Close the current open location stint if one exists
    for (const auto& stint : character.locationHistory) {
        if (stint.isCurrent()) {
            LocationStint closed = stint;
            closed.to = when;
            // Remove old open stint and replace with closed version
            auto rm = m_characterRepo->removeLocationStint(characterId, stint.from);
            if (rm.isErr()) return rm;
            auto save = m_characterRepo->saveLocationStint(characterId, closed);
            if (save.isErr()) return save;
            break;
        }
    }

    // Open new location stint
    LocationStint newStint;
    newStint.placeId = newPlaceId;
    newStint.from    = when;
    newStint.note    = note;

    auto result = m_characterRepo->saveLocationStint(characterId, newStint);
    if (result.isErr()) return result;

    emitTimelineEvent(
        TimelineEvent::EventType::CharacterMoved,
        characterId,
        when,
        character.name + " moved",
        note
    );

    notifyChange(DomainEvent::Kind::EntityUpdated,
                 DomainEvent::EntityType::Character,
                 characterId.value(),
                 character.name + " moved to new location");

    return Result<void>::ok();
}

// ── addEvolution ──────────────────────────────────────────────────────────────

Result<void> CharacterService::addEvolution(
    CharacterId characterId,
    const TimePoint& at,
    const std::string& description)
{
    if (description.empty()) {
        return Result<void>::err(
            AppError::validation("Evolution description cannot be empty"));
    }

    if (!m_characterRepo->exists(characterId)) {
        return Result<void>::err(
            AppError::notFound("Character not found: " + characterId.toString()));
    }

    CharacterEvolution evolution{ at, description };
    auto result = m_characterRepo->saveEvolution(characterId, evolution);

    if (result.isOk()) {
        emitTimelineEvent(
            TimelineEvent::EventType::CharacterEvolved,
            characterId,
            at,
            "Evolution recorded",
            description
        );
        notifyChange(DomainEvent::Kind::EntityUpdated,
                     DomainEvent::EntityType::Character,
                     characterId.value(),
                     "Evolution added at " + at.display());
    }
    return result;
}

// ── removeEvolution ───────────────────────────────────────────────────────────

Result<void> CharacterService::removeEvolution(
    CharacterId characterId, const TimePoint& at)
{
    auto result = m_characterRepo->removeEvolution(characterId, at);
    if (result.isOk()) {
        notifyChange(DomainEvent::Kind::EntityUpdated,
                     DomainEvent::EntityType::Character,
                     characterId.value(),
                     "Evolution removed at " + at.display());
    }
    return result;
}

// ── joinFaction ───────────────────────────────────────────────────────────────

Result<void> CharacterService::joinFaction(
    CharacterId characterId,
    FactionId   factionId,
    const TimePoint& when,
    const std::string& role)
{
    auto charResult = m_characterRepo->findById(characterId);
    if (charResult.isErr()) return Result<void>::err(charResult.error());

    // Guard: check character is not already an active member
    for (const auto& m : charResult.value().factionMemberships) {
        if (m.factionId == factionId && m.isCurrent()) {
            return Result<void>::err(
                AppError::duplicate("Character is already a member of this faction"));
        }
    }

    FactionMembership membership;
    membership.factionId = factionId;
    membership.from      = when;
    membership.role      = role;

    auto result = m_characterRepo->saveFactionMembership(characterId, membership);
    if (result.isOk()) {
        emitTimelineEvent(
            TimelineEvent::EventType::CharacterJoinedFaction,
            characterId,
            when,
            charResult.value().name + " joined faction",
            "Role: " + role
        );
        notifyChange(DomainEvent::Kind::EntityUpdated,
                     DomainEvent::EntityType::Character,
                     characterId.value(),
                     charResult.value().name + " joined a faction");
    }
    return result;
}

// ── leaveFaction ──────────────────────────────────────────────────────────────

Result<void> CharacterService::leaveFaction(
    CharacterId characterId,
    FactionId   factionId,
    const TimePoint& when)
{
    auto charResult = m_characterRepo->findById(characterId);
    if (charResult.isErr()) return Result<void>::err(charResult.error());

    // Find the active membership and close it
    for (const auto& m : charResult.value().factionMemberships) {
        if (m.factionId == factionId && m.isCurrent()) {
            // Remove open membership
            auto rm = m_characterRepo->removeFactionMembership(characterId, factionId);
            if (rm.isErr()) return rm;

            // Re-save with a closed end date
            FactionMembership closed = m;
            closed.to = when;
            auto save = m_characterRepo->saveFactionMembership(characterId, closed);
            if (save.isErr()) return save;

            emitTimelineEvent(
                TimelineEvent::EventType::CharacterLeftFaction,
                characterId,
                when,
                charResult.value().name + " left faction"
            );
            notifyChange(DomainEvent::Kind::EntityUpdated,
                         DomainEvent::EntityType::Character,
                         characterId.value(),
                         charResult.value().name + " left a faction");
            return Result<void>::ok();
        }
    }

    return Result<void>::err(
        AppError::notFound("No active faction membership found to close"));
}

// ── setRelationship ───────────────────────────────────────────────────────────

Result<void> CharacterService::setRelationship(
    CharacterId ownerId, const Relationship& relationship)
{
    if (!relationship.targetId.isValid()) {
        return Result<void>::err(
            AppError::validation("Relationship target ID is invalid"));
    }
    if (ownerId == relationship.targetId) {
        return Result<void>::err(
            AppError::validation("A character cannot have a relationship with itself"));
    }
    if (!m_characterRepo->exists(relationship.targetId)) {
        return Result<void>::err(
            AppError::notFound("Target character not found: "
                               + relationship.targetId.toString()));
    }

    auto result = m_characterRepo->saveRelationship(ownerId, relationship);
    if (result.isOk()) {
        notifyChange(DomainEvent::Kind::EntityUpdated,
                     DomainEvent::EntityType::Character,
                     ownerId.value(),
                     "Relationship updated");
    }
    return result;
}

// ── removeRelationship ────────────────────────────────────────────────────────

Result<void> CharacterService::removeRelationship(
    CharacterId ownerId, CharacterId targetId)
{
    auto result = m_characterRepo->removeRelationship(ownerId, targetId);
    if (result.isOk()) {
        notifyChange(DomainEvent::Kind::EntityUpdated,
                     DomainEvent::EntityType::Character,
                     ownerId.value(),
                     "Relationship removed");
    }
    return result;
}

// ── setOnChangeCallback ───────────────────────────────────────────────────────

void CharacterService::setOnChangeCallback(ChangeCallback callback)
{
    m_onChangeCallback = std::move(callback);
}

// ── Private helpers ───────────────────────────────────────────────────────────

Result<void> CharacterService::validateCharacter(const Character& c) const
{
    if (c.name.empty()) {
        return Result<void>::err(AppError::validation("Name cannot be empty"));
    }
    if (!c.id.isValid()) {
        return Result<void>::err(AppError::validation("Character has invalid ID"));
    }
    if (c.died.has_value() && c.died.value() < c.born) {
        return Result<void>::err(
            AppError::validation("Death date cannot be before birth date"));
    }
    return Result<void>::ok();
}

void CharacterService::emitTimelineEvent(
    TimelineEvent::EventType         type,
    TimelineEvent::Subject           subject,
    const TimePoint&                 when,
    const std::string&               title,
    const std::string&               description)
{
    TimelineEvent event;
    event.when        = when;
    event.type        = type;
    event.subject     = subject;
    event.title       = title;
    event.description = description;

    auto result = m_timelineRepo->save(event);
    if (result.isErr()) {
        // Timeline failure is non-fatal — log but don't propagate
        m_logger.warning("Failed to emit timeline event: "
                         + result.error().message, "CharacterService");
    }
}

void CharacterService::notifyChange(
    DomainEvent::Kind       kind,
    DomainEvent::EntityType type,
    int64_t                 id,
    const std::string&      description)
{
    if (m_onChangeCallback) {
        m_onChangeCallback({ kind, type, id, description });
    }
}

} // namespace CF::Services