#pragma once

#include "core/Result.h"
#include "core/Logger.h"
#include "domain/entities/Chapter.h"
#include "domain/events/DomainEvent.h"
#include "repositories/IChapterRepository.h"

#include <memory>
#include <functional>
#include <string>
#include <vector>

namespace CF::Services {

class ChapterService {
public:
    using ChangeCallback = std::function<void(Domain::DomainEvent)>;

    explicit ChapterService(
        std::shared_ptr<Repositories::IChapterRepository> chapterRepo,
        Core::Logger& logger);

    [[nodiscard]] Core::Result<Domain::Chapter>
    getById(Domain::ChapterId id) const;

    [[nodiscard]] Core::Result<std::vector<Domain::Chapter>>
    getAllOrdered() const;

    [[nodiscard]] Core::Result<Domain::Chapter>
    createChapter(const std::string& title);

    [[nodiscard]] Core::Result<Domain::Chapter>
    updateChapter(const Domain::Chapter& updated);

    // Partial save: only flushes content (called on autosave timer)
    [[nodiscard]] Core::Result<void>
    saveContent(Domain::ChapterId id, const std::string& markdownContent);

    // Reorder all chapters — called after drag-reorder in the UI
    // 'orderedIds' is the full list of chapter IDs in new order
    [[nodiscard]] Core::Result<void>
    reorder(const std::vector<Domain::ChapterId>& orderedIds);

    [[nodiscard]] Core::Result<void>
    deleteChapter(Domain::ChapterId id);

    void setOnChangeCallback(ChangeCallback callback);

private:
    void notifyChange(Domain::DomainEvent::Kind kind,
                      int64_t id, const std::string& desc);

    std::shared_ptr<Repositories::IChapterRepository> m_chapterRepo;
    Core::Logger&  m_logger;
    ChangeCallback m_onChangeCallback;
};

} // namespace CF::Services