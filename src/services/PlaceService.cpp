#include "PlaceService.h"
#include "core/Assert.h"

namespace CF::Services {

using namespace CF::Domain;
using namespace CF::Core;

PlaceService::PlaceService(
    std::shared_ptr<Repositories::IPlaceRepository>    placeRepo,
    std::shared_ptr<Repositories::ITimelineRepository> timelineRepo,
    Core::Logger& logger)
    : m_placeRepo(std::move(placeRepo))
    , m_timelineRepo(std::move(timelineRepo))
    , m_logger(logger)
{
    CF_REQUIRE(m_placeRepo    != nullptr, "PlaceRepository must not be null");
    CF_REQUIRE(m_timelineRepo != nullptr, "TimelineRepository must not be null");
}

Result<Place> PlaceService::getById(PlaceId id) const
{
    return m_placeRepo->findById(id);
}

Result<std::vector<Place>> PlaceService::getAll() const
{
    return m_placeRepo->findAll();
}

Result<std::vector<Place>>
PlaceService::search(const std::string& nameFragment) const
{
    if (nameFragment.empty()) return getAll();
    return m_placeRepo->findByNameContaining(nameFragment);
}

Result<std::vector<Place>>
PlaceService::getByRegion(const std::string& region) const
{
    return m_placeRepo->findByRegion(region);
}

Result<std::vector<std::string>> PlaceService::getAllRegions() const
{
    return m_placeRepo->allRegions();
}

Result<std::vector<Place>> PlaceService::getMobilePlaces() const
{
    return m_placeRepo->findMobilePlaces();
}

Result<std::vector<Place>> PlaceService::getChildren(PlaceId parentId) const
{
    return m_placeRepo->findChildren(parentId);
}

Result<Place> PlaceService::createPlace(
    const std::string& name,
    const std::string& type,
    const std::string& region,
    bool isMobile)
{
    if (name.empty()) {
        return Result<Place>::err(AppError::validation("Place name cannot be empty"));
    }

    Place p;
    p.name     = name;
    p.type     = type;
    p.region   = region;
    p.isMobile = isMobile;

    auto result = m_placeRepo->save(p);
    if (result.isOk()) {
        // Establish event uses the beginning of time (Era 1, Year 1)
        // if no explicit founding time — refine if needed per your world
        emitTimelineEvent(
            TimelineEvent::EventType::PlaceEstablished,
            result.value().id,
            { 1, 1 },
            name + " established"
        );
        notifyChange(DomainEvent::Kind::EntityCreated,
                     result.value().id.value(),
                     "Place created: " + name);
        m_logger.info("Place created: " + name, "PlaceService");
    }
    return result;
}

Result<Place> PlaceService::updatePlace(const Place& updated)
{
    auto validation = validatePlace(updated);
    if (validation.isErr()) return Result<Place>::err(validation.error());

    if (!m_placeRepo->exists(updated.id)) {
        return Result<Place>::err(
            AppError::notFound("Place not found: " + updated.id.toString()));
    }

    auto result = m_placeRepo->update(updated);
    if (result.isOk()) {
        notifyChange(DomainEvent::Kind::EntityUpdated,
                     updated.id.value(), "Place updated: " + updated.name);
    }
    return result;
}

Result<void> PlaceService::deletePlace(PlaceId id)
{
    auto existing = m_placeRepo->findById(id);
    if (existing.isErr()) return Result<void>::err(existing.error());

    auto result = m_placeRepo->remove(id);
    if (result.isOk()) {
        notifyChange(DomainEvent::Kind::EntityDeleted,
                     id.value(), "Place deleted: " + existing.value().name);
    }
    return result;
}

Result<void> PlaceService::updateMapPosition(PlaceId id, double x, double y)
{
    // Clamp coordinates to valid range
    x = std::max(0.0, std::min(1.0, x));
    y = std::max(0.0, std::min(1.0, y));

    auto existing = m_placeRepo->findById(id);
    if (existing.isErr()) return Result<void>::err(existing.error());

    Place updated     = existing.value();
    updated.mapX      = x;
    updated.mapY      = y;

    auto result = m_placeRepo->update(updated);
    if (result.isOk()) {
        // Map position change is a minor update — notify UI without timeline event
        notifyChange(DomainEvent::Kind::EntityUpdated,
                     id.value(), "Map position updated");
    }
    return result.isOk() ? Result<void>::ok()
                         : Result<void>::err(result.error());
}

Result<void> PlaceService::addEvolution(
    PlaceId placeId, const TimePoint& at, const std::string& description)
{
    if (description.empty()) {
        return Result<void>::err(
            AppError::validation("Evolution description cannot be empty"));
    }

    PlaceEvolution evolution{ at, description };
    auto result = m_placeRepo->saveEvolution(placeId, evolution);

    if (result.isOk()) {
        emitTimelineEvent(
            TimelineEvent::EventType::PlaceEvolved,
            placeId, at,
            "Place evolution recorded",
            description
        );
        notifyChange(DomainEvent::Kind::EntityUpdated,
                     placeId.value(), "Place evolution added at " + at.display());
    }
    return result;
}

Result<void> PlaceService::removeEvolution(PlaceId placeId, const TimePoint& at)
{
    auto result = m_placeRepo->removeEvolution(placeId, at);
    if (result.isOk()) {
        notifyChange(DomainEvent::Kind::EntityUpdated,
                     placeId.value(), "Place evolution removed");
    }
    return result;
}

void PlaceService::setOnChangeCallback(ChangeCallback callback)
{
    m_onChangeCallback = std::move(callback);
}

Result<void> PlaceService::validatePlace(const Place& p) const
{
    if (p.name.empty())
        return Result<void>::err(AppError::validation("Place name cannot be empty"));
    if (!p.id.isValid())
        return Result<void>::err(AppError::validation("Place has invalid ID"));
    return Result<void>::ok();
}

void PlaceService::emitTimelineEvent(
    TimelineEvent::EventType type, PlaceId subject,
    const TimePoint& when, const std::string& title, const std::string& desc)
{
    TimelineEvent event;
    event.when        = when;
    event.type        = type;
    event.subject     = subject;
    event.title       = title;
    event.description = desc;
    auto r = m_timelineRepo->save(event);
    if (r.isErr()) {
        m_logger.warning("Failed to emit place timeline event: "
                         + r.error().message, "PlaceService");
    }
}

void PlaceService::notifyChange(DomainEvent::Kind kind,
                                int64_t id,
                                const std::string& description)
{
    if (m_onChangeCallback) {
        m_onChangeCallback({
            kind,
            DomainEvent::EntityType::Place,
            id,
            description
        });
    }
}

} // namespace CF::Services