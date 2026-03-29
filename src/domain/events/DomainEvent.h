#pragma once

#include "domain/valueobjects/TimePoint.h"
#include "domain/valueobjects/EntityId.h"

#include <string>
#include <chrono>

namespace CF::Domain {

/**
 * DomainEvent is a base record for events that services emit
 * after mutating state. The UI layer subscribes via Qt signals
 * in the ViewModel — services themselves are Qt-free.
 *
 * This is a lightweight observer hook, not a full event bus.
 * Services expose std::function<void(DomainEvent)> callbacks
 * that ViewModels register on.
 */
struct DomainEvent {
    enum class Kind {
        EntityCreated,
        EntityUpdated,
        EntityDeleted
    };

    enum class EntityType {
        Character, Place, Faction, Scene, Chapter, Timeline
    };

    Kind       kind;
    EntityType entityType;
    int64_t    entityId;
    std::string description;  // Human-readable summary for logging
};

} // namespace CF::Domain
