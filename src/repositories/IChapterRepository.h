#pragma once

#include "IRepository.h"
#include "domain/entities/Chapter.h"

#include <string>
#include <vector>

namespace CF::Repositories {

class IChapterRepository
    : public IRepository<Domain::Chapter, Domain::ChapterId>
{
public:
    ~IChapterRepository() override = default;

    [[nodiscard]] virtual Core::Result<std::vector<Domain::Chapter>>
    findByNameContaining(const std::string& fragment) const = 0;

    // Returns all chapters ordered by sortOrder ascending
    [[nodiscard]] virtual Core::Result<std::vector<Domain::Chapter>>
    findAllOrdered() const = 0;

    // Persist only the content field (avoids full round-trip on every keystroke)
    [[nodiscard]] virtual Core::Result<void>
    updateContent(Domain::ChapterId id,
                  const std::string& markdownContent) = 0;

    // Persist only the sort order (called after drag-reorder in UI)
    [[nodiscard]] virtual Core::Result<void>
    updateSortOrder(Domain::ChapterId id, int32_t sortOrder) = 0;
};

} // namespace CF::Repositories