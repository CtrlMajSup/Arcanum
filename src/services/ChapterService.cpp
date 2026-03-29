#include "ChapterService.h"
#include "core/Assert.h"

namespace CF::Services {

using namespace CF::Domain;
using namespace CF::Core;

ChapterService::ChapterService(
    std::shared_ptr<Repositories::IChapterRepository> chapterRepo,
    Core::Logger& logger)
    : m_chapterRepo(std::move(chapterRepo))
    , m_logger(logger)
{
    CF_REQUIRE(m_chapterRepo != nullptr, "ChapterRepository must not be null");
}

Result<Chapter> ChapterService::getById(ChapterId id) const
{
    return m_chapterRepo->findById(id);
}

Result<std::vector<Chapter>> ChapterService::getAllOrdered() const
{
    return m_chapterRepo->findAllOrdered();
}

Result<Chapter> ChapterService::createChapter(const std::string& title)
{
    if (title.empty()) {
        return Result<Chapter>::err(
            AppError::validation("Chapter title cannot be empty"));
    }

    // Compute next sort order (place at end of list)
    auto allResult = m_chapterRepo->findAllOrdered();
    int32_t nextOrder = 0;
    if (allResult.isOk() && !allResult.value().empty()) {
        nextOrder = allResult.value().back().sortOrder + 1;
    }

    Chapter c;
    c.title     = title;
    c.sortOrder = nextOrder;

    auto result = m_chapterRepo->save(c);
    if (result.isOk()) {
        notifyChange(DomainEvent::Kind::EntityCreated,
                     result.value().id.value(), "Chapter created: " + title);
        m_logger.info("Chapter created: " + title, "ChapterService");
    }
    return result;
}

Result<Chapter> ChapterService::updateChapter(const Chapter& updated)
{
    if (updated.title.empty()) {
        return Result<Chapter>::err(
            AppError::validation("Chapter title cannot be empty"));
    }
    if (!m_chapterRepo->exists(updated.id)) {
        return Result<Chapter>::err(
            AppError::notFound("Chapter not found: " + updated.id.toString()));
    }

    auto result = m_chapterRepo->update(updated);
    if (result.isOk()) {
        notifyChange(DomainEvent::Kind::EntityUpdated,
                     updated.id.value(), "Chapter updated: " + updated.title);
    }
    return result;
}

Result<void> ChapterService::saveContent(
    ChapterId id, const std::string& markdownContent)
{
    // Partial update: touch only the content column, not the whole row
    auto result = m_chapterRepo->updateContent(id, markdownContent);
    if (result.isErr()) {
        m_logger.warning("Autosave failed for chapter "
                         + id.toString() + ": " + result.error().message,
                         "ChapterService");
    }
    return result;
}

Result<void> ChapterService::reorder(const std::vector<ChapterId>& orderedIds)
{
    // Assign sort_order = index position in the provided list
    for (int32_t i = 0; i < static_cast<int32_t>(orderedIds.size()); ++i) {
        auto result = m_chapterRepo->updateSortOrder(orderedIds[i], i);
        if (result.isErr()) {
            m_logger.warning("Failed to update sort order for chapter "
                             + orderedIds[i].toString(), "ChapterService");
            return result;
        }
    }
    notifyChange(DomainEvent::Kind::EntityUpdated, -1, "Chapters reordered");
    return Result<void>::ok();
}

Result<void> ChapterService::deleteChapter(ChapterId id)
{
    auto existing = m_chapterRepo->findById(id);
    if (existing.isErr()) return Result<void>::err(existing.error());

    auto result = m_chapterRepo->remove(id);
    if (result.isOk()) {
        notifyChange(DomainEvent::Kind::EntityDeleted,
                     id.value(), "Chapter deleted: " + existing.value().title);
    }
    return result;
}

void ChapterService::setOnChangeCallback(ChangeCallback callback)
{
    m_onChangeCallback = std::move(callback);
}

void ChapterService::notifyChange(DomainEvent::Kind kind,
                                   int64_t id, const std::string& desc)
{
    if (m_onChangeCallback) {
        m_onChangeCallback({ kind, DomainEvent::EntityType::Chapter, id, desc });
    }
}

} // namespace CF::Services