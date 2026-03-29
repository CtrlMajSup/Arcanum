#pragma once

#include "domain/valueobjects/EntityId.h"
#include "domain/valueobjects/TimePoint.h"

#include <string>
#include <vector>
#include <optional>

namespace CF::Domain {

/**
 * Scene is a discrete narrative event or moment in the world.
 * It links to:
 *  - Multiple places  (where it happens)
 *  - Multiple factions (which factions are involved)
 *  - Multiple characters (who participates)
 * All via many-to-many join tables in the DB.
 *
 * Scenes are pinned to a TimePoint and optionally to a Chapter.
 */
struct Scene {
    SceneId     id;
    std::string title;
    std::string summary;
    TimePoint   when;

    std::optional<ChapterId> chapterId;   // May exist outside any chapter

    // These are resolved from join tables by the repository
    // and populated by the service layer — not stored directly on Scene
    std::vector<PlaceId>     placeIds;
    std::vector<FactionId>   factionIds;
    std::vector<CharacterId> characterIds;
};

} // namespace CF::Domain