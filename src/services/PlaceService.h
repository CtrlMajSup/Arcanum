#pragma once

#include "core/Result.h"
#include "core/Logger.h"
#include "domain/entities/Place.h"
#include "domain/events/DomainEvent.h"
#include "repositories/IPlaceRepository.h"
#include "repositories/ITimelineRepository.h"

#include <memory>
#include <functional>
#include <string>
#include <vector>

namespace CF::Services {

class PlaceService {
public:
    using ChangeCallback = std::function<void(Domain::DomainEvent)>;

    explicit PlaceService(
        std::shared_ptr<Repositories::IPlaceRepository>   placeRepo,
        std::shared_ptr<Repositories::ITimelineRepository> timelineRepo,
        Core::Logger& logger);

    // ── Queries ───────────────────────────────────────────────────────────────
    [[nodiscard]] Core::Result<Domain::Place>
    getById(Domain::PlaceId id) const;

    [[nodiscard]] Core::Result<std::vector<Domain::Place>>
    getAll() const;

    [[nodiscard]] Core::Result<std::vector<Domain::Place>>
    search(const std::string& nameFragment) const;

    [[nodiscard]] Core::Result<std::vector<Domain::Place>>
    getByRegion(const std::string& region) const;

    [[nodiscard]] Core::Result<std::vector<std::string>>
    getAllRegions() const;

    [[nodiscard]] Core::Result<std::vector<Domain::Place>>
    getMobilePlaces() const;

    [[nodiscard]] Core::Result<std::vector<Domain::Place>>
    getChildren(Domain::PlaceId parentId) const;

    // ── Mutations ─────────────────────────────────────────────────────────────
    [[nodiscard]] Core::Result<Domain::Place>
    createPlace(const std::string& name,
                const std::string& type,
                const std::string& region,
                bool isMobile = false);

    [[nodiscard]] Core::Result<Domain::Place>
    updatePlace(const Domain::Place& updated);

    [[nodiscard]] Core::Result<void>
    deletePlace(Domain::PlaceId id);

    // Updates the schematic map position (called on drag-drop in MapWidget)
    [[nodiscard]] Core::Result<void>
    updateMapPosition(Domain::PlaceId id, double x, double y);

    [[nodiscard]] Core::Result<void>
    addEvolution(Domain::PlaceId placeId,
                 const Domain::TimePoint& at,
                 const std::string& description);

    [[nodiscard]] Core::Result<void>
    removeEvolution(Domain::PlaceId placeId,
                    const Domain::TimePoint& at);

    void setOnChangeCallback(ChangeCallback callback);

private:
    [[nodiscard]] Core::Result<void> validatePlace(const Domain::Place& p) const;

    void emitTimelineEvent(Domain::TimelineEvent::EventType type,
                           Domain::PlaceId subject,
                           const Domain::TimePoint& when,
                           const std::string& title,
                           const std::string& description = "");

    void notifyChange(Domain::DomainEvent::Kind kind,
                      int64_t id,
                      const std::string& description);

    std::shared_ptr<Repositories::IPlaceRepository>    m_placeRepo;
    std::shared_ptr<Repositories::ITimelineRepository> m_timelineRepo;
    Core::Logger&  m_logger;
    ChangeCallback m_onChangeCallback;
};

} // namespace CF::Services