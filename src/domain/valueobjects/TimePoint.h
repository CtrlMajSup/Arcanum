#pragma once

#include <cstdint>
#include <string>
#include <compare>

namespace CF::Domain {

/**
 * TimePoint represents a moment in your world's history.
 * Format: Era (e.g. 2) + Year (e.g. 347) → "Era 2, Year 347"
 *
 * TimePoints are fully ordered and comparable.
 * Era 3 Year 1 > Era 2 Year 999 (era takes priority).
 */
struct TimePoint {
    int32_t era  = 1;
    int32_t year = 1;

    // Three-way comparison (C++20 spaceship, but we support C++17 too)
    bool operator==(const TimePoint& o) const { return era == o.era && year == o.year; }
    bool operator!=(const TimePoint& o) const { return !(*this == o); }
    bool operator< (const TimePoint& o) const {
        if (era != o.era) return era < o.era;
        return year < o.year;
    }
    bool operator<=(const TimePoint& o) const { return !(o < *this); }
    bool operator> (const TimePoint& o) const { return o < *this; }
    bool operator>=(const TimePoint& o) const { return !(*this < o); }

    [[nodiscard]] std::string display() const {
        return "Era " + std::to_string(era) + ", Year " + std::to_string(year);
    }

    // Compact storage key for SQLite (single sortable integer)
    // Assumes max 100,000 years per era — adjust if needed
    [[nodiscard]] int64_t toSortKey() const {
        return static_cast<int64_t>(era) * 100'000LL + year;
    }

    static TimePoint fromSortKey(int64_t key) {
        return { static_cast<int32_t>(key / 100'000LL),
                 static_cast<int32_t>(key % 100'000LL) };
    }
};

} // namespace CF::Domain