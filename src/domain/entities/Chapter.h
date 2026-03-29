#pragma once

#include "domain/valueobjects/EntityId.h"
#include "domain/valueobjects/TimePoint.h"

#include <string>
#include <vector>
#include <cstdint>
#include <optional>

namespace CF::Domain {

/**
 * Chapter is the writing unit — a section of your book/script.
 * It stores Markdown source text and metadata.
 *
 * Chapters are ordered by 'sortOrder' (user-defined drag order),
 * not by TimePoint, because narrative order ≠ in-world chronology.
 */
struct Chapter {
    ChapterId   id;
    std::string title;
    std::string markdownContent;   // Raw Markdown — rendered in the UI
    int32_t     sortOrder = 0;     // User-defined ordering in the chapter list

    // In-world time range this chapter covers (optional, for timeline linkage)
    std::optional<TimePoint> timeStart;
    std::optional<TimePoint> timeEnd;

    std::string notes;             // Author's private notes, not rendered
};

} // namespace CF::Domain