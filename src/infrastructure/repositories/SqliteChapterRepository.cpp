#include "SqliteChapterRepository.h"
#include "core/Assert.h"

#include <QSqlQuery>
#include <QSqlError>
#include <QVariant>

namespace CF::Infra {

using namespace CF::Domain;
using namespace CF::Core;

SqliteChapterRepository::SqliteChapterRepository(DatabaseManager& db, Core::Logger& logger)
    : m_db(db), m_logger(logger)
{}

Result<Chapter> SqliteChapterRepository::findById(ChapterId id) const
{
    auto q = m_db.makeQuery();
    q.prepare("SELECT * FROM chapters WHERE id = ?");
    q.addBindValue(static_cast<qlonglong>(id.value()));
    if (!q.exec() || !q.next()) {
        return Result<Chapter>::err(
            AppError::notFound("Chapter not found: " + id.toString()));
    }
    return Result<Chapter>::ok(hydrateChapter(q));
}

Result<std::vector<Chapter>> SqliteChapterRepository::findAll() const
{
    auto q = m_db.makeQuery();
    q.exec("SELECT * FROM chapters ORDER BY sort_order");
    std::vector<Chapter> result;
    while (q.next()) result.push_back(hydrateChapter(q));
    return Result<std::vector<Chapter>>::ok(std::move(result));
}

Result<std::vector<Chapter>> SqliteChapterRepository::findAllOrdered() const
{
    return findAll(); // already sorted by sort_order
}

Result<Chapter> SqliteChapterRepository::save(const Chapter& c)
{
    auto q = m_db.makeQuery();
    q.prepare(R"(
        INSERT INTO chapters
            (title, markdown_content, sort_order,
             time_start_era, time_start_year,
             time_end_era,   time_end_year, notes)
        VALUES (?, ?, ?, ?, ?, ?, ?, ?)
    )");
    q.addBindValue(QString::fromStdString(c.title));
    q.addBindValue(QString::fromStdString(c.markdownContent));
    q.addBindValue(c.sortOrder);
    q.addBindValue(c.timeStart ? QVariant(c.timeStart->era)  : QVariant());
    q.addBindValue(c.timeStart ? QVariant(c.timeStart->year) : QVariant());
    q.addBindValue(c.timeEnd   ? QVariant(c.timeEnd->era)    : QVariant());
    q.addBindValue(c.timeEnd   ? QVariant(c.timeEnd->year)   : QVariant());
    q.addBindValue(QString::fromStdString(c.notes));

    if (!q.exec()) {
        return Result<Chapter>::err(AppError::dbError(
            "Insert chapter failed: " + q.lastError().text().toStdString()));
    }
    Chapter saved = c;
    saved.id = ChapterId(q.lastInsertId().toLongLong());
    return Result<Chapter>::ok(std::move(saved));
}

Result<Chapter> SqliteChapterRepository::update(const Chapter& c)
{
    CF_REQUIRE(c.id.isValid(), "Cannot update chapter with invalid ID");

    auto q = m_db.makeQuery();
    q.prepare(R"(
        UPDATE chapters SET
            title=?, markdown_content=?, sort_order=?,
            time_start_era=?, time_start_year=?,
            time_end_era=?,   time_end_year=?, notes=?
        WHERE id=?
    )");
    q.addBindValue(QString::fromStdString(c.title));
    q.addBindValue(QString::fromStdString(c.markdownContent));
    q.addBindValue(c.sortOrder);
    q.addBindValue(c.timeStart ? QVariant(c.timeStart->era)  : QVariant());
    q.addBindValue(c.timeStart ? QVariant(c.timeStart->year) : QVariant());
    q.addBindValue(c.timeEnd   ? QVariant(c.timeEnd->era)    : QVariant());
    q.addBindValue(c.timeEnd   ? QVariant(c.timeEnd->year)   : QVariant());
    q.addBindValue(QString::fromStdString(c.notes));
    q.addBindValue(static_cast<qlonglong>(c.id.value()));

    if (!q.exec()) {
        return Result<Chapter>::err(AppError::dbError(
            "Update chapter failed: " + q.lastError().text().toStdString()));
    }
    return Result<Chapter>::ok(c);
}

Result<void> SqliteChapterRepository::remove(ChapterId id)
{
    auto q = m_db.makeQuery();
    q.prepare("DELETE FROM chapters WHERE id = ?");
    q.addBindValue(static_cast<qlonglong>(id.value()));
    if (!q.exec()) {
        return Result<void>::err(AppError::dbError(
            "Delete chapter failed: " + q.lastError().text().toStdString()));
    }
    return Result<void>::ok();
}

bool SqliteChapterRepository::exists(ChapterId id) const
{
    auto q = m_db.makeQuery();
    q.prepare("SELECT 1 FROM chapters WHERE id = ?");
    q.addBindValue(static_cast<qlonglong>(id.value()));
    return q.exec() && q.next();
}

Result<std::vector<Chapter>>
SqliteChapterRepository::findByNameContaining(const std::string& fragment) const
{
    auto q = m_db.makeQuery();
    q.prepare("SELECT * FROM chapters WHERE title LIKE ? ORDER BY sort_order");
    q.addBindValue(QString::fromStdString("%" + fragment + "%"));
    std::vector<Chapter> result;
    if (q.exec()) while (q.next()) result.push_back(hydrateChapter(q));
    return Result<std::vector<Chapter>>::ok(std::move(result));
}

// Partial update: only touches markdown_content — avoids full reload on each save
Result<void> SqliteChapterRepository::updateContent(
    ChapterId id, const std::string& markdown)
{
    auto q = m_db.makeQuery();
    q.prepare("UPDATE chapters SET markdown_content=? WHERE id=?");
    q.addBindValue(QString::fromStdString(markdown));
    q.addBindValue(static_cast<qlonglong>(id.value()));
    if (!q.exec()) {
        return Result<void>::err(AppError::dbError(
            "Update chapter content failed: " + q.lastError().text().toStdString()));
    }
    return Result<void>::ok();
}

// Partial update: only touches sort_order — called after drag-reorder
Result<void> SqliteChapterRepository::updateSortOrder(
    ChapterId id, int32_t sortOrder)
{
    auto q = m_db.makeQuery();
    q.prepare("UPDATE chapters SET sort_order=? WHERE id=?");
    q.addBindValue(sortOrder);
    q.addBindValue(static_cast<qlonglong>(id.value()));
    if (!q.exec()) {
        return Result<void>::err(AppError::dbError(
            "Update chapter sort order failed: " + q.lastError().text().toStdString()));
    }
    return Result<void>::ok();
}

Chapter SqliteChapterRepository::hydrateChapter(QSqlQuery& q) const
{
    Chapter c;
    c.id              = ChapterId(q.value("id").toLongLong());
    c.title           = q.value("title").toString().toStdString();
    c.markdownContent = q.value("markdown_content").toString().toStdString();
    c.sortOrder       = q.value("sort_order").toInt();
    c.notes           = q.value("notes").toString().toStdString();

    const auto sEra  = q.value("time_start_era");
    const auto sYear = q.value("time_start_year");
    if (!sEra.isNull()) c.timeStart = TimePoint{ sEra.toInt(), sYear.toInt() };

    const auto eEra  = q.value("time_end_era");
    const auto eYear = q.value("time_end_year");
    if (!eEra.isNull()) c.timeEnd = TimePoint{ eEra.toInt(), eYear.toInt() };

    return c;
}

} // namespace CF::Infra