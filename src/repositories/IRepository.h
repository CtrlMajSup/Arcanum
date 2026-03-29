#pragma once

#include "core/Result.h"
#include "domain/valueobjects/EntityId.h"

#include <vector>
#include <optional>
#include <cstdint>

namespace CF::Repositories {

/**
 * IRepository<T, ID> is the generic base for all repositories.
 *
 * Every concrete repository inherits from this and may extend it
 * with domain-specific query methods.
 *
 * All methods return Result<T> — no exceptions cross the repository
 * boundary. Callers always check isOk() before using the value.
 */
template<typename T, typename ID>
class IRepository {
public:
    virtual ~IRepository() = default;

    // Returns the entity, or AppError::NotFound
    [[nodiscard]] virtual Core::Result<T>
    findById(ID id) const = 0;

    // Returns all entities (may be large — prefer filtered queries)
    [[nodiscard]] virtual Core::Result<std::vector<T>>
    findAll() const = 0;

    // Persists a new entity. The ID on the returned T is the DB-assigned one.
    [[nodiscard]] virtual Core::Result<T>
    save(const T& entity) = 0;

    // Updates an existing entity. Entity must have a valid ID.
    [[nodiscard]] virtual Core::Result<T>
    update(const T& entity) = 0;

    // Removes an entity by ID.
    [[nodiscard]] virtual Core::Result<void>
    remove(ID id) = 0;

    // Returns true if an entity with this ID exists.
    [[nodiscard]] virtual bool
    exists(ID id) const = 0;
};

} // namespace CF::Repositories