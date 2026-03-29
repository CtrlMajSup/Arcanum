#include "FactionService.h"
#include "core/Assert.h"

namespace CF::Services {

using namespace CF::Domain;
using namespace CF::Core;

FactionService::FactionService(
    std::shared_ptr<Repositories::IFactionRepository>  factionRepo,
    std::shared_ptr<Repositories::ITimelineRepository> timelineRepo,
    Core::Logger& logger)
    : m_factionRepo(std::move(factionRepo))
    , m_timelineRepo(std::move(timelineRepo))
    , m_logger(logger)
{
    CF_REQUIRE(m_factionRepo  != nullptr, "FactionRepository must not be null");
    CF_REQUIRE(m_timelineRepo != nullptr, "TimelineRepository must not be null");
}

Result<Faction> FactionService::getById(FactionId id) const
{
    return m_factionRepo->findById(id);
}

Result<std::vector<Faction>> FactionService::getAll() const
{
    return m_factionRepo->findAll();
}

Result<std::vector<Faction>>
FactionService::search(const std::string& nameFragment) const
{
    if (nameFragment.empty()) return getAll();
    return m_factionRepo->findByNameContaining(nameFragment);
}

Result<std::vector<Faction>>
FactionService::getActiveAt(const TimePoint& when) const
{
    return m_factionRepo->findActiveAt(when);
}

Result<Faction> FactionService::createFaction(
    const std::string& name,
    const std::string& type,
    const TimePoint& founded)
{
    if (name.empty()) {
        return Result<Faction>::err(
            AppError::validation("Faction name cannot be empty"));
    }

    Faction f;
    f.name    = name;
    f.type    = type;
    f.founded = founded;

    auto result = m_factionRepo->save(f);
    if (result.isOk()) {
        emitTimelineEvent(
            TimelineEvent::EventType::FactionFounded,
            result.value().id,
            founded,
            name + " founded",
            "Type: " + type
        );
        notifyChange(DomainEvent::Kind::EntityCreated,
                     result.value().id.value(), "Faction created: " + name);
        m_logger.info("Faction created: " + name, "FactionService");
    }
    return result;
}

Result<Faction> FactionService::updateFaction(const Faction& updated)
{
    if (updated.name.empty()) {
        return Result<Faction>::err(
            AppError::validation("Faction name cannot be empty"));
    }
    if (!m_factionRepo->exists(updated.id)) {
        return Result<Faction>::err(
            AppError::notFound("Faction not found: " + updated.id.toString()));
    }

    auto result = m_factionRepo->update(updated);
    if (result.isOk()) {
        notifyChange(DomainEvent::Kind::EntityUpdated,
                     updated.id.value(), "Faction updated: " + updated.name);
    }
    return result;
}

Result<void> FactionService::dissolveFaction(FactionId id, const TimePoint& when)
{
    auto existing = m_factionRepo->findById(id);
    if (existing.isErr()) return Result<void>::err(existing.error());

    if (existing.value().dissolved.has_value()) {
        return Result<void>::err(
            AppError::validation("Faction is already dissolved"));
    }
    if (when < existing.value().founded) {
        return Result<void>::err(
            AppError::validation("Dissolution date cannot be before founding date"));
    }

    Faction dissolved    = existing.value();
    dissolved.dissolved  = when;

    auto result = m_factionRepo->update(dissolved);
    if (result.isOk()) {
        emitTimelineEvent(
            TimelineEvent::EventType::FactionDissolved,
            id, when,
            existing.value().name + " dissolved"
        );
        notifyChange(DomainEvent::Kind::EntityUpdated,
                     id.value(), existing.value().name + " dissolved");
    }
    return result.isOk() ? Result<void>::ok()
                         : Result<void>::err(result.error());
}

Result<void> FactionService::deleteFaction(FactionId id)
{
    auto existing = m_factionRepo->findById(id);
    if (existing.isErr()) return Result<void>::err(existing.error());

    auto result = m_factionRepo->remove(id);
    if (result.isOk()) {
        notifyChange(DomainEvent::Kind::EntityDeleted,
                     id.value(), "Faction deleted: " + existing.value().name);
    }
    return result;
}

Result<void> FactionService::addEvolution(
    FactionId factionId, const TimePoint& at, const std::string& description)
{
    if (description.empty()) {
        return Result<void>::err(
            AppError::validation("Evolution description cannot be empty"));
    }

    FactionEvolution evolution{ at, description };
    auto result = m_factionRepo->saveEvolution(factionId, evolution);
    if (result.isOk()) {
        emitTimelineEvent(
            TimelineEvent::EventType::FactionEvolved,
            factionId, at,
            "Faction evolution recorded", description
        );
        notifyChange(DomainEvent::Kind::EntityUpdated,
                     factionId.value(), "Faction evolution added");
    }
    return result;
}

Result<void> FactionService::removeEvolution(
    FactionId factionId, const TimePoint& at)
{
    auto result = m_factionRepo->removeEvolution(factionId, at);
    if (result.isOk()) {
        notifyChange(DomainEvent::Kind::EntityUpdated,
                     factionId.value(), "Faction evolution removed");
    }
    return result;
}

void FactionService::setOnChangeCallback(ChangeCallback callback)
{
    m_onChangeCallback = std::move(callback);
}

void FactionService::emitTimelineEvent(
    TimelineEvent::EventType type, FactionId subject,
    const TimePoint& when, const std::string& title, const std::string& desc)
{
    TimelineEvent event;
    event.when = when; event.type = type;
    event.subject = subject; event.title = title; event.description = desc;
    auto r = m_timelineRepo->save(event);
    if (r.isErr()) {
        m_logger.warning("Failed to emit faction timeline event: "
                         + r.error().message, "FactionService");
    }
}

void FactionService::notifyChange(DomainEvent::Kind kind,
                                   int64_t id, const std::string& desc)
{
    if (m_onChangeCallback) {
        m_onChangeCallback({ kind, DomainEvent::EntityType::Faction, id, desc });
    }
}

} // namespace CF::Services