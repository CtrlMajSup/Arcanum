#pragma once

#include "EntityId.h"
#include "TimePoint.h"
#include <string>
#include <optional>

namespace CF::Domain {

/**
 * Relationship captures a directed link between two characters.
 * e.g. "Kael is the MENTOR of Sira, established in Era 1 Year 12,
 *        ended in Era 2 Year 4 (Sira left the order)"
 *
 * Relationships are owned by the Character aggregate — they are not
 * a separate top-level entity. They evolve on the timeline via
 * the 'since' / 'until' fields.
 */
struct Relationship {
    enum class Type {
        Ally,
        Enemy,
        Mentor,
        Student,
        Family,
        Romantic,
        Neutral,
        Unknown
    };

    CharacterId            targetId;    // The other character
    Type                   type;
    std::string            note;        // Optional description
    TimePoint              since;       // When this relationship started
    std::optional<TimePoint> until;     // Empty = still active

    [[nodiscard]] bool isActive() const { return !until.has_value(); }

    static std::string typeToString(Type t) {
        switch (t) {
            case Type::Ally:     return "Ally";
            case Type::Enemy:    return "Enemy";
            case Type::Mentor:   return "Mentor";
            case Type::Student:  return "Student";
            case Type::Family:   return "Family";
            case Type::Romantic: return "Romantic";
            case Type::Neutral:  return "Neutral";
            default:             return "Unknown";
        }
    }

    static Type typeFromString(const std::string& s) {
        if (s == "Ally")     return Type::Ally;
        if (s == "Enemy")    return Type::Enemy;
        if (s == "Mentor")   return Type::Mentor;
        if (s == "Student")  return Type::Student;
        if (s == "Family")   return Type::Family;
        if (s == "Romantic") return Type::Romantic;
        if (s == "Neutral")  return Type::Neutral;
        return Type::Unknown;
    }
};

} // namespace CF::Domain