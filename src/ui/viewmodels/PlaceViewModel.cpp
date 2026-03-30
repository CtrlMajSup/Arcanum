#include "PlaceViewModel.h"
#include "core/Assert.h"

namespace CF::UI {

using namespace CF::Domain;
using namespace CF::Services;
using namespace CF::Core;

PlaceViewModel::PlaceViewModel(
    std::shared_ptr<PlaceService> placeService,
    Core::Logger& logger,
    QObject* parent)
    : QAbstractListModel(parent)
    , m_placeService(std::move(placeService))
    , m_logger(logger)
{
    CF_REQUIRE(m_placeService != nullptr, "PlaceService must not be null");

    m_placeService->setOnChangeCallback(
        [this](Domain::DomainEvent event) {
            QMetaObject::invokeMethod(this, [this, event]() {
                onServiceChange(event);
            }, Qt::QueuedConnection);
        });
}

// ── QAbstractListModel ────────────────────────────────────────────────────────

int PlaceViewModel::rowCount(const QModelIndex& parent) const
{
    if (parent.isValid()) return 0;
    return static_cast<int>(m_places.size());
}

QVariant PlaceViewModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid() || index.row() >= static_cast<int>(m_places.size()))
        return {};

    const auto& p = m_places[index.row()];
    switch (role) {
        case Qt::DisplayRole:
        case NameRole:     return QString::fromStdString(p.name);
        case IdRole:       return static_cast<qint64>(p.id.value());
        case TypeRole:     return QString::fromStdString(p.type);
        case RegionRole:   return QString::fromStdString(p.region);
        case IsMobileRole: return p.isMobile;
        default:           return {};
    }
}

QHash<int, QByteArray> PlaceViewModel::roleNames() const
{
    return {
        { IdRole,       "placeId"  },
        { NameRole,     "name"     },
        { TypeRole,     "type"     },
        { RegionRole,   "region"   },
        { IsMobileRole, "isMobile" }
    };
}

// ── Commands ──────────────────────────────────────────────────────────────────

void PlaceViewModel::loadAll()
{
    m_currentFilter.clear();
    auto result = m_placeService->getAll();
    if (result.isErr()) {
        emit errorOccurred(QString::fromStdString(result.error().message));
        return;
    }
    beginResetModel();
    m_places = std::move(result.value());
    endResetModel();
}

void PlaceViewModel::search(const QString& nameFragment)
{
    m_currentFilter = nameFragment;
    auto result = m_placeService->search(nameFragment.toStdString());
    if (result.isErr()) {
        emit errorOccurred(QString::fromStdString(result.error().message));
        return;
    }
    beginResetModel();
    m_places = std::move(result.value());
    endResetModel();
}

std::optional<Place> PlaceViewModel::placeAt(int row) const
{
    if (row < 0 || row >= static_cast<int>(m_places.size())) return std::nullopt;
    return m_places[row];
}

std::optional<Place> PlaceViewModel::placeById(PlaceId id) const
{
    for (const auto& p : m_places)
        if (p.id == id) return p;
    return std::nullopt;
}

void PlaceViewModel::createPlace(
    const QString& name, const QString& type,
    const QString& region, bool isMobile)
{
    auto result = m_placeService->createPlace(
        name.toStdString(), type.toStdString(),
        region.toStdString(), isMobile);

    if (result.isErr()) {
        emit errorOccurred(QString::fromStdString(result.error().message));
        return;
    }

    const int newRow = static_cast<int>(m_places.size());
    beginInsertRows(QModelIndex(), newRow, newRow);
    m_places.push_back(result.value());
    endInsertRows();

    emit placeCreated(static_cast<qint64>(result.value().id.value()));
}

void PlaceViewModel::updatePlace(const Place& updated)
{
    auto result = m_placeService->updatePlace(updated);
    if (result.isErr()) {
        emit errorOccurred(QString::fromStdString(result.error().message));
        return;
    }

    const int row = indexOfId(updated.id);
    if (row >= 0) {
        m_places[row] = result.value();
        const auto idx = index(row);
        emit dataChanged(idx, idx);
    }
    emit placeUpdated(static_cast<qint64>(updated.id.value()));
}

void PlaceViewModel::deletePlace(PlaceId id)
{
    auto result = m_placeService->deletePlace(id);
    if (result.isErr()) {
        emit errorOccurred(QString::fromStdString(result.error().message));
        return;
    }

    const int row = indexOfId(id);
    if (row >= 0) {
        beginRemoveRows(QModelIndex(), row, row);
        m_places.erase(m_places.begin() + row);
        endRemoveRows();
    }
    emit placeDeleted(static_cast<qint64>(id.value()));
}

void PlaceViewModel::addEvolution(
    PlaceId id, int era, int year, const QString& description)
{
    auto result = m_placeService->addEvolution(
        id, { era, year }, description.toStdString());
    if (result.isErr()) {
        emit errorOccurred(QString::fromStdString(result.error().message));
        return;
    }
    refreshPlace(id);
}

void PlaceViewModel::removeEvolution(PlaceId id, int era, int year)
{
    auto result = m_placeService->removeEvolution(id, { era, year });
    if (result.isErr()) {
        emit errorOccurred(QString::fromStdString(result.error().message));
        return;
    }
    refreshPlace(id);
}

// ── Private ───────────────────────────────────────────────────────────────────

void PlaceViewModel::onServiceChange(Domain::DomainEvent event)
{
    using Kind = Domain::DomainEvent::Kind;
    switch (event.kind) {
        case Kind::EntityCreated:
        case Kind::EntityUpdated:
            refreshPlace(PlaceId(event.entityId));
            break;
        case Kind::EntityDeleted: {
            const int row = indexOfId(PlaceId(event.entityId));
            if (row >= 0) {
                beginRemoveRows(QModelIndex(), row, row);
                m_places.erase(m_places.begin() + row);
                endRemoveRows();
            }
            break;
        }
    }
}

void PlaceViewModel::refreshPlace(PlaceId id)
{
    auto result = m_placeService->getById(id);
    if (result.isErr()) return;

    const int row = indexOfId(id);
    if (row >= 0) {
        m_places[row] = result.value();
        const auto idx = index(row);
        emit dataChanged(idx, idx);
        emit placeUpdated(static_cast<qint64>(id.value()));
    } else if (m_currentFilter.isEmpty()) {
        beginInsertRows(QModelIndex(),
                        static_cast<int>(m_places.size()),
                        static_cast<int>(m_places.size()));
        m_places.push_back(result.value());
        endInsertRows();
    }
}

int PlaceViewModel::indexOfId(PlaceId id) const
{
    for (int i = 0; i < static_cast<int>(m_places.size()); ++i)
        if (m_places[i].id == id) return i;
    return -1;
}

} // namespace CF::UI