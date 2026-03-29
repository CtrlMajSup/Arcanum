#pragma once

#include "core/Result.h"
#include "core/Logger.h"
#include "repositories/ICharacterRepository.h"
#include "repositories/IPlaceRepository.h"
#include "repositories/IFactionRepository.h"
#include "repositories/ISceneRepository.h"

#include <memory>
#include <string>
#include <vector>

namespace CF::Services {

/**
 * AutoCompleteSuggestion is a lightweight descriptor for the popup.
 * It carries just enough to render and insert the entity reference.
 */
struct AutoCompleteSuggestion {
    enum class EntityType { Character, Place, Faction, Scene };

    int64_t     id;
    std::string name;
    std::string detail;    // e.g. species, region, type — shown as a subtitle
    EntityType  type;

    // The text inserted into the editor when this suggestion is accepted
    // e.g. "@[Kael|character:42]"
    [[nodiscard]] std::string insertionText() const {
        std::string typeStr;
        switch (type) {
            case EntityType::Character: typeStr = "character"; break;
            case EntityType::Place:     typeStr = "place";     break;
            case EntityType::Faction:   typeStr = "faction";   break;
            case EntityType::Scene:     typeStr = "scene";     break;
        }
        return "@[" + name + "|" + typeStr + ":" + std::to_string(id) + "]";
    }
};

/**
 * AutoCompleteService provides entity name suggestions for the chapter editor.
 * It is triggered when the user types '@' followed by characters.
 *
 * It queries all four entity repositories with a prefix match and merges
 * results ranked by name similarity to the typed fragment.
 */
class AutoCompleteService {
public:
    explicit AutoCompleteService(
        std::shared_ptr<Repositories::ICharacterRepository> characterRepo,
        std::shared_ptr<Repositories::IPlaceRepository>     placeRepo,
        std::shared_ptr<Repositories::IFactionRepository>   factionRepo,
        std::shared_ptr<Repositories::ISceneRepository>     sceneRepo,
        Core::Logger& logger);

    // Returns suggestions for the fragment typed after '@'
    // e.g. fragment="Kae" → [{id:1, name:"Kael", type:Character}, ...]
    [[nodiscard]] Core::Result<std::vector<AutoCompleteSuggestion>>
    suggest(const std::string& fragment, int maxResults = 10) const;

private:
    std::shared_ptr<Repositories::ICharacterRepository> m_characterRepo;
    std::shared_ptr<Repositories::IPlaceRepository>     m_placeRepo;
    std::shared_ptr<Repositories::IFactionRepository>   m_factionRepo;
    std::shared_ptr<Repositories::ISceneRepository>     m_sceneRepo;
    Core::Logger& m_logger;
};

} // namespace CF::Services