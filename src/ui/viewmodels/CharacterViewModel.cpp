#include "CharacterViewModel.h"
#include "core/Assert.h"

namespace CF::UI {

using namespace CF::Domain;
using namespace CF::Services;
using namespace CF::Core;

CharacterViewModel::CharacterViewModel(
    std::shared_ptr<CharacterService> characterService,
    std::shared_ptr<PlaceService>     placeService,
    std::shared_ptr<FactionService>   factionService,
    Core::Logger& logger,
    QObject* parent)
    : QAbstractListModel(parent)
    , m_characterService(std::move(characterService))
    , m_placeService(std::move(placeService))
    , m_factionService(std::move(factionService))
    , m_logger(logger)
{
    CF_REQUIRE(m_characterService != nullptr, "CharacterService must not be null");

    // Wire service change notifications back to this ViewModel
    m_characterService->setOnChangeCallback(
        [this](DomainEvent event) {
            // Service callbacks may come from any context — use Qt's
            // queued connection pattern via QMetaObject to be thread-safe
            QMetaObject::invokeMethod(this, [this, event]() {
                onServiceChange(event);
            }, Qt::QueuedConnection);
        }
    );
}

// ── QAbstractListModel ────────────────────────────────────────────────────────

int CharacterViewModel::rowCount(const QModelIndex& parent) const
{
    if (parent.isValid()) return 0;
    return static_cast<int>(m_characters.size());
}

QVariant CharacterViewModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid() || index.row() >= static_cast<int>(m_characters.size()))
        return {};

    const auto& c = m_characters[index.row()];

    switch (role) {
        case Qt::DisplayRole:
        case NameRole:       return QString::fromStdString(c.name);
        case IdRole:         return static_cast<qint64>(c.id.value());
        case SpeciesRole:    return QString::fromStdString(c.species);
        case BiographyRole:  return QString::fromStdString(c.biography);
        case BornEraRole:    return c.born.era;
        case BornYearRole:   return c.born.year;
        case IsAliveRole:    return c.isAlive();
        default:             return {};
    }
}

QHash<int, QByteArray> CharacterViewModel::roleNames() const
{
    return {
        { IdRole,        "characterId"  },
        { NameRole,      "name"         },
        { SpeciesRole,   "species"      },
        { BiographyRole, "biography"    },
        { BornEraRole,   "bornEra"      },
        { BornYearRole,  "bornYear"     },
        { IsAliveRole,   "isAlive"      }
    };
}

// ── Commands ──────────────────────────────────────────────────────────────────

void CharacterViewModel::loadAll()
{
    m_currentFilter.clear();
    auto result = m_characterService->getAll();
    if (result.isErr()) {
        emit errorOccurred(QString::fromStdString(result.error().message));
        return;
    }

    beginResetModel();
    m_characters = std::move(result.value());
    endResetModel();
}

void CharacterViewModel::search(const QString& nameFragment)
{
    m_currentFilter = nameFragment;
    auto result = m_characterService->search(nameFragment.toStdString());
    if (result.isErr()) {
        emit errorOccurred(QString::fromStdString(result.error().message));
        return;
    }

    beginResetModel();
    m_characters = std::move(result.value());
    endResetModel();
}

std::optional<Character>
CharacterViewModel::characterAt(int row) const
{
    if (row < 0 || row >= static_cast<int>(m_characters.size()))
        return std::nullopt;
    return m_characters[row];
}

std::optional<Character>
CharacterViewModel::characterById(CharacterId id) const
{
    for (const auto& c : m_characters) {
        if (c.id == id) return c;
    }
    return std::nullopt;
}

void CharacterViewModel::createCharacter(
    const QString& name, const QString& species, int bornEra, int bornYear)
{
    auto result = m_characterService->createCharacter(
        name.toStdString(),
        species.toStdString(),
        { bornEra, bornYear }
    );

    if (result.isErr()) {
        emit errorOccurred(QString::fromStdString(result.error().message));
        return;
    }

    // Add to local cache and notify views
    const auto& saved = result.value();
    beginInsertRows(QModelIndex(),
                    static_cast<int>(m_characters.size()),
                    static_cast<int>(m_characters.size()));
    m_characters.push_back(saved);
    endInsertRows();

    emit characterCreated(static_cast<qint64>(saved.id.value()));
    m_logger.debug("CharacterViewModel: character created in cache", "CharacterVM");
}

void CharacterViewModel::updateCharacter(const Character& updated)
{
    auto result = m_characterService->updateCharacter(updated);
    if (result.isErr()) {
        emit errorOccurred(QString::fromStdString(result.error().message));
        return;
    }

    const int row = indexOfId(updated.id);
    if (row >= 0) {
        m_characters[row] = result.value();
        const auto idx = index(row);
        emit dataChanged(idx, idx);
    }
    emit characterUpdated(static_cast<qint64>(updated.id.value()));
}

void CharacterViewModel::deleteCharacter(CharacterId id)
{
    auto result = m_characterService->deleteCharacter(id);
    if (result.isErr()) {
        emit errorOccurred(QString::fromStdString(result.error().message));
        return;
    }

    const int row = indexOfId(id);
    if (row >= 0) {
        beginRemoveRows(QModelIndex(), row, row);
        m_characters.erase(m_characters.begin() + row);
        endRemoveRows();
    }
    emit characterDeleted(static_cast<qint64>(id.value()));
}

void CharacterViewModel::moveCharacter(
    CharacterId characterId, PlaceId newPlaceId,
    int era, int year, const QString& note)
{
    auto result = m_characterService->moveCharacter(
        characterId, newPlaceId, { era, year }, note.toStdString());

    if (result.isErr()) {
        emit errorOccurred(QString::fromStdString(result.error().message));
        return;
    }
    refreshCharacter(characterId);
}

void CharacterViewModel::addEvolution(
    CharacterId id, int era, int year, const QString& description)
{
    auto result = m_characterService->addEvolution(
        id, { era, year }, description.toStdString());

    if (result.isErr()) {
        emit errorOccurred(QString::fromStdString(result.error().message));
        return;
    }
    refreshCharacter(id);
}

void CharacterViewModel::removeEvolution(CharacterId id, int era, int year)
{
    auto result = m_characterService->removeEvolution(id, { era, year });
    if (result.isErr()) {
        emit errorOccurred(QString::fromStdString(result.error().message));
        return;
    }
    refreshCharacter(id);
}

void CharacterViewModel::joinFaction(
    CharacterId characterId, FactionId factionId,
    int era, int year, const QString& role)
{
    auto result = m_characterService->joinFaction(
        characterId, factionId, { era, year }, role.toStdString());

    if (result.isErr()) {
        emit errorOccurred(QString::fromStdString(result.error().message));
        return;
    }
    refreshCharacter(characterId);
}

void CharacterViewModel::leaveFaction(
    CharacterId characterId, FactionId factionId, int era, int year)
{
    auto result = m_characterService->leaveFaction(
        characterId, factionId, { era, year });

    if (result.isErr()) {
        emit errorOccurred(QString::fromStdString(result.error().message));
        return;
    }
    refreshCharacter(characterId);
}

void CharacterViewModel::setRelationship(
    CharacterId ownerId, const Relationship& rel)
{
    auto result = m_characterService->setRelationship(ownerId, rel);
    if (result.isErr()) {
        emit errorOccurred(QString::fromStdString(result.error().message));
        return;
    }
    refreshCharacter(ownerId);
}

void CharacterViewModel::removeRelationship(
    CharacterId ownerId, CharacterId targetId)
{
    auto result = m_characterService->removeRelationship(ownerId, targetId);
    if (result.isErr()) {
        emit errorOccurred(QString::fromStdString(result.error().message));
        return;
    }
    refreshCharacter(ownerId);
}

// ── Private ───────────────────────────────────────────────────────────────────

void CharacterViewModel::onServiceChange(DomainEvent event)
{
    using Kind = DomainEvent::Kind;

    // When a change arrives from outside this ViewModel (e.g. another feature
    // moves a character), refresh the affected row rather than reloading all
    switch (event.kind) {
        case Kind::EntityCreated:
        case Kind::EntityUpdated:
            refreshCharacter(CharacterId(event.entityId));
            break;
        case Kind::EntityDeleted: {
            const int row = indexOfId(CharacterId(event.entityId));
            if (row >= 0) {
                beginRemoveRows(QModelIndex(), row, row);
                m_characters.erase(m_characters.begin() + row);
                endRemoveRows();
            }
            break;
        }
    }
}

void CharacterViewModel::refreshCharacter(CharacterId id)
{
    auto result = m_characterService->getById(id);
    if (result.isErr()) return;

    const int row = indexOfId(id);
    if (row >= 0) {
        m_characters[row] = result.value();
        const auto idx = index(row);
        emit dataChanged(idx, idx, { IdRole, NameRole, SpeciesRole,
                                     BiographyRole, IsAliveRole });
        emit characterUpdated(static_cast<qint64>(id.value()));
    } else {
        // Character not in current filtered list — may need adding
        if (m_currentFilter.isEmpty()) {
            beginInsertRows(QModelIndex(),
                            static_cast<int>(m_characters.size()),
                            static_cast<int>(m_characters.size()));
            m_characters.push_back(result.value());
            endInsertRows();
        }
    }
}

int CharacterViewModel::indexOfId(CharacterId id) const
{
    for (int i = 0; i < static_cast<int>(m_characters.size()); ++i) {
        if (m_characters[i].id == id) return i;
    }
    return -1;
}

} // namespace CF::UI