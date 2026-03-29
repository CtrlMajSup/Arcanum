#pragma once

#include <variant>
#include <string>
#include <stdexcept>
#include <functional>

namespace CF::Core {

/**
 * Result<T, E> is a discriminated union representing either a success value
 * or an error. This eliminates exceptions from the data/service layers.
 *
 * Usage:
 *   Result<Character, AppError> r = repo.findById(id);
 *   if (r.isOk()) { use(r.value()); }
 *   else          { log(r.error().message); }
 */

struct AppError {
    enum class Code {
        NotFound,
        DatabaseError,
        ValidationError,
        Duplicate,
        Unknown
    };

    Code        code    = Code::Unknown;
    std::string message;

    static AppError notFound(std::string msg)     { return {Code::NotFound,       std::move(msg)}; }
    static AppError dbError(std::string msg)       { return {Code::DatabaseError,  std::move(msg)}; }
    static AppError validation(std::string msg)    { return {Code::ValidationError,std::move(msg)}; }
    static AppError duplicate(std::string msg)     { return {Code::Duplicate,      std::move(msg)}; }
};

template<typename T>
class Result {
public:
    // Success factory
    static Result ok(T value) {
        Result r;
        r.m_data = std::move(value);
        return r;
    }

    // Failure factory
    static Result err(AppError error) {
        Result r;
        r.m_data = std::move(error);
        return r;
    }

    [[nodiscard]] bool isOk()  const { return std::holds_alternative<T>(m_data); }
    [[nodiscard]] bool isErr() const { return !isOk(); }

    // Throws std::logic_error if called on error — use isOk() first
    [[nodiscard]] const T& value() const {
        if (isErr()) throw std::logic_error("Result::value() called on error state");
        return std::get<T>(m_data);
    }

    [[nodiscard]] T& value() {
        if (isErr()) throw std::logic_error("Result::value() called on error state");
        return std::get<T>(m_data);
    }

    [[nodiscard]] const AppError& error() const {
        if (isOk()) throw std::logic_error("Result::error() called on ok state");
        return std::get<AppError>(m_data);
    }

    // Monadic helpers
    template<typename F>
    auto map(F&& fn) const -> Result<decltype(fn(std::declval<T>()))> {
        using U = decltype(fn(std::declval<T>()));
        if (isOk()) return Result<U>::ok(fn(value()));
        return Result<U>::err(error());
    }

    // Provide a fallback value on error
    [[nodiscard]] T valueOr(T fallback) const {
        return isOk() ? value() : std::move(fallback);
    }

private:
    std::variant<T, AppError> m_data;
};

// Specialisation for void success (operations with no return value)
template<>
class Result<void> {
public:
    static Result ok()              { Result r; r.m_ok = true;  return r; }
    static Result err(AppError e)   { Result r; r.m_ok = false; r.m_error = std::move(e); return r; }

    [[nodiscard]] bool isOk()  const { return m_ok; }
    [[nodiscard]] bool isErr() const { return !m_ok; }

    [[nodiscard]] const AppError& error() const { return m_error; }

private:
    bool     m_ok = false;
    AppError m_error;
};

} // namespace CF::Core