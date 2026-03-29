#pragma once

#include "IRepository.h"
#include "domain/entities/Scene.h"
#include "domain/valueobjects/TimePoint.h"

#include <string>
#include <vector>

namespace CF::Repositories {

/**
 * ISceneRepository handles the Scene aggregate and its three
 * many-to-many join tables:
 *   scene_places     (scene ↔ place)
 *   scene_factions   (scene ↔ faction)
 *   scene_characters (scene ↔ character)
 */
class ISceneRepository
    : public IRepository<Domain::Scene, Domain::SceneId>
{
public:
    ~ISceneRepository() override = default;

    [[nodiscard]] virtual Core::Result<std::vector<Domain::Scene>>
    findByNameContaining(const std::string& fragment) const = 0;

    // All scenes that occur within a TimePoint range (inclusive)
    [[nodiscard]] virtual Core::Result<std::vector<Domain::Scene>>
    findInTimeRange(const Domain::TimePoint& from,
                    const Domain::TimePoint& to) const = 0;

    // Scenes linked to a specific place
    [[nodiscard]] virtual Core::Result<std::vector<Domain::Scene>>
    findByPlace(Domain::PlaceId placeId) const = 0;

    // Scenes linked to a specific faction
    [[nodiscard]] virtual Core::Result<std::vector<Domain::Scene>>
    findByFaction(Domain::FactionId factionId) const = 0;

    // Scenes a character participates in
    [[nodiscard]] virtual Core::Result<std::vector<Domain::Scene>>
    findByCharacter(Domain::CharacterId characterId) const = 0;

    // Scenes belonging to a chapter
    [[nodiscard]] virtual Core::Result<std::vector<Domain::Scene>>
    findByChapter(Domain::ChapterId chapterId) const = 0;

    // ── Many-to-many link management ─────────────────────────────────────────

    [[nodiscard]] virtual Core::Result<void>
    linkPlace(Domain::SceneId sceneId, Domain::PlaceId placeId) = 0;

    [[nodiscard]] virtual Core::Result<void>
    unlinkPlace(Domain::SceneId sceneId, Domain::PlaceId placeId) = 0;

    [[nodiscard]] virtual Core::Result<void>
    linkFaction(Domain::SceneId sceneId, Domain::FactionId factionId) = 0;

    [[nodiscard]] virtual Core::Result<void>
    unlinkFaction(Domain::SceneId sceneId, Domain::FactionId factionId) = 0;

    [[nodiscard]] virtual Core::Result<void>
    linkCharacter(Domain::SceneId sceneId, Domain::CharacterId characterId) = 0;

    [[nodiscard]] virtual Core::Result<void>
    unlinkCharacter(Domain::SceneId sceneId, Domain::CharacterId characterId) = 0;
};

} // namespace CF::Repositories