#pragma once

#include <cassert>
#include <string_view>
#include <stdexcept>
#include <iostream>

namespace CF::Core {

/**
 * CF_ASSERT: In debug builds, aborts with a message.
 * In release builds, becomes a no-op (performance safe).
 *
 * CF_REQUIRE: Always-on validation for invariants that must hold in
 * production too (e.g. null pointer guards at API boundaries).
 */

} // namespace CF::Core

#ifdef CF_DEBUG
    #define CF_ASSERT(condition, message)                                        \
        do {                                                                     \
            if (!(condition)) {                                                  \
                std::cerr << "[ASSERT FAILED] " << (message)                    \
                          << "\n  at " << __FILE__ << ":" << __LINE__ << "\n";  \
                std::abort();                                                    \
            }                                                                    \
        } while(false)
#else
    #define CF_ASSERT(condition, message) do { (void)(condition); } while(false)
#endif

// CF_REQUIRE always fires — use for API preconditions
#define CF_REQUIRE(condition, message)                                           \
    do {                                                                         \
        if (!(condition)) {                                                      \
            throw std::logic_error(                                              \
                std::string("[PRECONDITION] ") + (message) +                    \
                " at " + __FILE__ + ":" + std::to_string(__LINE__)              \
            );                                                                   \
        }                                                                        \
    } while(false)