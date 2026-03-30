#include "FactionViewModel.h"
#include "core/Assert.h"

namespace CF::UI {

using namespace CF::Domain;
using namespace CF::Services;
using namespace CF::Core;

FactionViewModel::FactionViewModel(
    std::shared_ptr<FactionService> factionService,
    Core::Logger& logger,
    QObject* parent)
    : QAbstractListModel(parent)
    , m_factionService(std::move(factionService))
    , m_logger(logger)
{
    CF_REQUIRE(m_factionService != nullptr, "FactionService must not be null");

    m_factionService->setOnChangeCallback(
        [this](Domain::DomainEvent event) {
            QMetaObject::invokeMethod(this, [this, event]() {
                onServiceChange(event);
            }, Qt::QueuedConnection);
        });
}

// ── QAbstractListModel ────────────────────────────────────────────────────────

int FactionViewModel::rowCount(const QModelIndex& parent) const
{
    if (parent.isValid()) return 0;
    return static_cast<int>(m_factions.size());
}

QVariant FactionViewModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid() || index.row() >= static_cast<int>(m_factions.size()))
        return {};

    const auto& f = m_factions[index.row()];
    switch (role) {
        case Qt::DisplayRole:
        case NameRole:     return QString::fromStdString(f.name);
        case IdRole:       return static_cast<qint64>(f.id.value());
        case TypeRole:     return QString::fromStdString(f.type);
        case IsActiveRole: return !f.dissolved.has_value();
        default:           return {};
    }
}

QHash<int, QByteArray> FactionViewModel::roleNames() const
{
    return {
        { IdRole,       "factionId" },
        { NameRole,     "name"      },
        { TypeRole,     "type"      },
        { IsActiveRole, "isActive"  }
    };
}

// ── Commands ──────────────────────────────────────────────────────────────────

void FactionViewModel::loadAll()
{
    m_currentFilter.clear();
    auto result = m_factionService->getAll();
    if (result.isErr()) {
        emit errorOccurred(QString::fromStdString(result.error().message));
        return;
    }
    beginResetModel();
    m_factions = std::move(result.value());
    endResetModel();
}

void FactionViewModel::search(const QString& nameFragment)
{
    m_currentFilter = nameFragment;
    auto result = m_factionService->search(nameFragment.toStdString());
    if (result.isErr()) {
        emit errorOccurred(QString::fromStdString(result.error().message));
        return;
    }
    beginResetModel();
    m_factions = std::move(result.value());
    endResetModel();
}

std::optional<Faction> FactionViewModel::factionAt(int row) const
{
    if (row < 0 || row >= static_cast<int>(m_factions.size())) return std::nullopt;
    return m_factions[row];
}

std::optional<Faction> FactionViewModel::factionById(FactionId id) const
{
    for (const auto& f : m_factions)
        if (f.id == id) return f;
    return std::nullopt;
}

void FactionViewModel::createFaction(
    const QString& name, const QString& type,
    int foundedEra, int foundedYear)
{
    auto result = m_factionService->createFaction(
        name.toStdString(), type.toStdString(),
        { foundedEra, foundedYear });

    if (result.isErr()) {
        emit errorOccurred(QString::fromStdString(result.error().message));
        return;
    }

    const int newRow = static_cast<int>(m_factions.size());
    beginInsertRows(QModelIndex(), newRow, newRow);
    m_factions.push_back(result.value());
    endInsertRows();

    emit factionCreated(static_cast<qint64>(result.value().id.value()));
}

void FactionViewModel::updateFaction(const Faction& updated)
{
    auto result = m_factionService->updateFaction(updated);
    if (result.isErr()) {
        emit errorOccurred(QString::fromStdString(result.error().message));
        return;
    }

    const int row = indexOfId(updated.id);
    if (row >= 0) {
        m_factions[row] = result.value();
        const auto idx  = index(row);
        emit dataChanged(idx, idx);
    }
    emit factionUpdated(static_cast<qint64>(updated.id.value()));
}

void FactionViewModel::dissolveFaction(FactionId id, int era, int year)
{
    auto result = m_factionService->dissolveFaction(id, { era, year });
    if (result.isErr()) {
        emit errorOccurred(QString::fromStdString(result.error().message));
        return;
    }
    refreshFaction(id);
}

void FactionViewModel::deleteFaction(FactionId id)
{
    auto result = m_factionService->deleteFaction(id);
    if (result.isErr()) {
        emit errorOccurred(QString::fromStdString(result.error().message));
        return;
    }

    const int row = indexOfId(id);
    if (row >= 0) {
        beginRemoveRows(QModelIndex(), row, row);
        m_factions.erase(m_factions.begin() + row);
        endRemoveRows();
    }
    emit factionDeleted(static_cast<qint64>(id.value()));
}

void FactionViewModel::addEvolution(
    FactionId id, int era, int year, const QString& description)
{
    auto result = m_factionService->addEvolution(
        id, { era, year }, description.toStdString());
    if (result.isErr()) {
        emit errorOccurred(QString::fromStdString(result.error().message));
        return;
    }
    refreshFaction(id);
}

void FactionViewModel::removeEvolution(FactionId id, int era, int year)
{
    auto result = m_factionService->removeEvolution(id, { era, year });
    if (result.isErr()) {
        emit errorOccurred(QString::fromStdString(result.error().message));
        return;
    }
    refreshFaction(id);
}

// ── Private ───────────────────────────────────────────────────────────────────

void FactionViewModel::onServiceChange(Domain::DomainEvent event)
{
    using Kind = Domain::DomainEvent::Kind;
    switch (event.kind) {
        case Kind::EntityCreated:
        case Kind::EntityUpdated:
            refreshFaction(FactionId(event.entityId));
            break;
        case Kind::EntityDeleted: {
            const int row = indexOfId(FactionId(event.entityId));
            if (row >= 0) {
                beginRemoveRows(QModelIndex(), row, row);
                m_factions.erase(m_factions.begin() + row);
                endRemoveRows();
            }
            break;
        }
    }
}

void FactionViewModel::refreshFaction(FactionId id)
{
    auto result = m_factionService->getById(id);
    if (result.isErr()) return;

    const int row = indexOfId(id);
    if (row >= 0) {
        m_factions[row] = result.value();
        const auto idx  = index(row);
        emit dataChanged(idx, idx);
        emit factionUpdated(static_cast<qint64>(id.value()));
    } else if (m_currentFilter.isEmpty()) {
        beginInsertRows(QModelIndex(),
                        static_cast<int>(m_factions.size()),
                        static_cast<int>(m_factions.size()));
        m_factions.push_back(result.value());
        endInsertRows();
    }
}

int FactionViewModel::indexOfId(FactionId id) const
{
    for (int i = 0; i < static_cast<int>(m_factions.size()); ++i)
        if (m_factions[i].id == id) return i;
    return -1;
}

} // namespace CF::UI