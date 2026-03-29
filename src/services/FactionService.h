#pragma once

#include "core/Result.h"
#include "core/Logger.h"
#include "domain/entities/Faction.h"
#include "domain/events/DomainEvent.h"
#include "repositories/IFactionRepository.h"
#include "repositories/ITimelineRepository.h"

#include <memory>
#include <functional>
#include <string>
#include <vector>

namespace CF::Services {

class FactionService {
public:
    using ChangeCallback = std::function<void(Domain::DomainEvent)>;

    explicit FactionService(
        std::shared_ptr<Repositories::IFactionRepository>  factionRepo,
        std::shared_ptr<Repositories::ITimelineRepository> timelineRepo,
        Core::Logger& logger);

    [[nodiscard]] Core::Result<Domain::Faction>
    getById(Domain::FactionId id) const;

    [[nodiscard]] Core::Result<std::vector<Domain::Faction>>
    getAll() const;

    [[nodiscard]] Core::Result<std::vector<Domain::Faction>>
    search(const std::string& nameFragment) const;

    [[nodiscard]] Core::Result<std::vector<Domain::Faction>>
    getActiveAt(const Domain::TimePoint& when) const;

    [[nodiscard]] Core::Result<Domain::Faction>
    createFaction(const std::string& name,
                  const std::string& type,
                  const Domain::TimePoint& founded);

    [[nodiscard]] Core::Result<Domain::Faction>
    updateFaction(const Domain::Faction& updated);

    [[nodiscard]] Core::Result<void>
    dissolveFaction(Domain::FactionId id, const Domain::TimePoint& when);

    [[nodiscard]] Core::Result<void>
    deleteFaction(Domain::FactionId id);

    [[nodiscard]] Core::Result<void>
    addEvolution(Domain::FactionId factionId,
                 const Domain::TimePoint& at,
                 const std::string& description);

    [[nodiscard]] Core::Result<void>
    removeEvolution(Domain::FactionId factionId,
                    const Domain::TimePoint& at);

    void setOnChangeCallback(ChangeCallback callback);

private:
    void emitTimelineEvent(Domain::TimelineEvent::EventType type,
                           Domain::FactionId subject,
                           const Domain::TimePoint& when,
                           const std::string& title,
                           const std::string& desc = "");

    void notifyChange(Domain::DomainEvent::Kind kind,
                      int64_t id, const std::string& desc);

    std::shared_ptr<Repositories::IFactionRepository>  m_factionRepo;
    std::shared_ptr<Repositories::ITimelineRepository> m_timelineRepo;
    Core::Logger&  m_logger;
    ChangeCallback m_onChangeCallback;
};

} // namespace CF::Services