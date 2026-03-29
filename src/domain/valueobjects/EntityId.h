#pragma once

#include <cstdint>
#include <string>
#include <functional>

namespace CF::Domain {

/**
 * EntityId is a strong typedef over int64 — prevents mixing up
 * CharacterId with PlaceId at compile time.
 *
 * Usage:
 *   using CharacterId = EntityId<struct CharacterTag>;
 *   using PlaceId     = EntityId<struct PlaceTag>;
 *
 * CharacterId and PlaceId are now distinct types — assigning one to
 * the other is a compile error.
 */
template<typename Tag>
class EntityId {
public:
    static constexpr int64_t INVALID = -1;

    EntityId() : m_value(INVALID) {}
    explicit EntityId(int64_t value) : m_value(value) {}

    [[nodiscard]] int64_t value()   const { return m_value; }
    [[nodiscard]] bool    isValid() const { return m_value != INVALID; }

    bool operator==(const EntityId& other) const { return m_value == other.m_value; }
    bool operator!=(const EntityId& other) const { return !(*this == other); }
    bool operator< (const EntityId& other) const { return m_value < other.m_value; }

    [[nodiscard]] std::string toString() const { return std::to_string(m_value); }

private:
    int64_t m_value;
};

// ─── Concrete ID types (defined once, used everywhere) ────────────────────────
using CharacterId  = EntityId<struct CharacterTag>;
using PlaceId      = EntityId<struct PlaceTag>;
using FactionId    = EntityId<struct FactionTag>;
using SceneId      = EntityId<struct SceneTag>;
using ChapterId    = EntityId<struct ChapterTag>;
using TimelineId   = EntityId<struct TimelineTag>;

} // namespace CF::Domain


// ─── std::hash specialisations (needed for unordered_map/set) ─────────────────
namespace std {
    template<typename Tag>
    struct hash<CF::Domain::EntityId<Tag>> {
        size_t operator()(const CF::Domain::EntityId<Tag>& id) const noexcept {
            return std::hash<int64_t>{}(id.value());
        }
    };
}