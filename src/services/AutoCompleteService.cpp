#include "AutoCompleteService.h"
#include "core/Assert.h"

#include <algorithm>

namespace CF::Services {

using namespace CF::Core;

AutoCompleteService::AutoCompleteService(
    std::shared_ptr<Repositories::ICharacterRepository> characterRepo,
    std::shared_ptr<Repositories::IPlaceRepository>     placeRepo,
    std::shared_ptr<Repositories::IFactionRepository>   factionRepo,
    std::shared_ptr<Repositories::ISceneRepository>     sceneRepo,
    Core::Logger& logger)
    : m_characterRepo(std::move(characterRepo))
    , m_placeRepo(std::move(placeRepo))
    , m_factionRepo(std::move(factionRepo))
    , m_sceneRepo(std::move(sceneRepo))
    , m_logger(logger)
{
    CF_REQUIRE(m_characterRepo != nullptr, "CharacterRepo must not be null");
    CF_REQUIRE(m_placeRepo     != nullptr, "PlaceRepo must not be null");
    CF_REQUIRE(m_factionRepo   != nullptr, "FactionRepo must not be null");
    CF_REQUIRE(m_sceneRepo     != nullptr, "SceneRepo must not be null");
}

Result<std::vector<AutoCompleteSuggestion>>
AutoCompleteService::suggest(const std::string& fragment, int maxResults) const
{
    if (fragment.empty()) {
        return Result<std::vector<AutoCompleteSuggestion>>::ok({});
    }

    std::vector<AutoCompleteSuggestion> suggestions;

    // Gather from all four entity types
    if (auto r = m_characterRepo->findByNameContaining(fragment); r.isOk()) {
        for (const auto& c : r.value()) {
            suggestions.push_back({
                c.id.value(), c.name, c.species,
                AutoCompleteSuggestion::EntityType::Character
            });
        }
    }

    if (auto r = m_placeRepo->findByNameContaining(fragment); r.isOk()) {
        for (const auto& p : r.value()) {
            suggestions.push_back({
                p.id.value(), p.name, p.region,
                AutoCompleteSuggestion::EntityType::Place
            });
        }
    }

    if (auto r = m_factionRepo->findByNameContaining(fragment); r.isOk()) {
        for (const auto& f : r.value()) {
            suggestions.push_back({
                f.id.value(), f.name, f.type,
                AutoCompleteSuggestion::EntityType::Faction
            });
        }
    }

    if (auto r = m_sceneRepo->findByNameContaining(fragment); r.isOk()) {
        for (const auto& s : r.value()) {
            suggestions.push_back({
                s.id.value(), s.title, s.when.display(),
                AutoCompleteSuggestion::EntityType::Scene
            });
        }
    }

    // Sort: exact prefix matches first, then alphabetical
    const std::string lowerFragment = [&]{
        std::string s = fragment;
        std::transform(s.begin(), s.end(), s.begin(), ::tolower);
        return s;
    }();

    std::sort(suggestions.begin(), suggestions.end(),
        [&](const AutoCompleteSuggestion& a, const AutoCompleteSuggestion& b) {
            std::string la = a.name, lb = b.name;
            std::transform(la.begin(), la.end(), la.begin(), ::tolower);
            std::transform(lb.begin(), lb.end(), lb.begin(), ::tolower);

            const bool aPrefix = la.rfind(lowerFragment, 0) == 0;
            const bool bPrefix = lb.rfind(lowerFragment, 0) == 0;

            if (aPrefix != bPrefix) return aPrefix > bPrefix;
            return la < lb;
        });

    // Trim to maxResults
    if (static_cast<int>(suggestions.size()) > maxResults) {
        suggestions.resize(maxResults);
    }

    return Result<std::vector<AutoCompleteSuggestion>>::ok(std::move(suggestions));
}

} // namespace CF::Services