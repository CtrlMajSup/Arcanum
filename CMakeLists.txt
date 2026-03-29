cmake_minimum_required(VERSION 3.22)
project(Arcanum VERSION 1.0.0 LANGUAGES CXX)

# ─── C++ Standard ─────────────────────────────────────────────────────────────
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ─── Build type default ───────────────────────────────────────────────────────
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Debug" CACHE STRING "Build type" FORCE)
endif()

# ─── Debug mode flag (passed to source via compile definition) ────────────────
option(CF_DEBUG_MODE "Enable verbose debug logging" ON)

# ─── Qt6 ──────────────────────────────────────────────────────────────────────
find_package(Qt6 REQUIRED COMPONENTS
    Core
    Widgets
    Gui
    Sql          # Qt's SQLite driver
)
qt_standard_project_setup()

# ─── SQLite (bundled via Qt's QSQLITE driver — no extra dep needed) ───────────
# If you later want raw sqlite3.h access, add: find_package(SQLite3 REQUIRED)

# ─── Collect sources by layer ─────────────────────────────────────────────────
file(GLOB_RECURSE CORE_SOURCES         src/core/*.cpp)
file(GLOB_RECURSE DOMAIN_SOURCES       src/domain/*.cpp)
file(GLOB_RECURSE REPO_SOURCES         src/repositories/*.cpp)
file(GLOB_RECURSE INFRA_SOURCES        src/infrastructure/*.cpp)
file(GLOB_RECURSE SERVICE_SOURCES      src/services/*.cpp)
file(GLOB_RECURSE UI_SOURCES           src/ui/*.cpp)

# ─── Main executable ──────────────────────────────────────────────────────────
qt_add_executable(Arcanum
    src/main.cpp
    src/App.cpp
    ${CORE_SOURCES}
    ${DOMAIN_SOURCES}
    ${REPO_SOURCES}
    ${INFRA_SOURCES}
    ${SERVICE_SOURCES}
    ${UI_SOURCES}
)

# ─── Resources ────────────────────────────────────────────────────────────────
qt_add_resources(Arcanum "resources"
    PREFIX "/"
    FILES
        resources/styles/main.qss
)

# ─── Include paths ────────────────────────────────────────────────────────────
target_include_directories(Arcanum PRIVATE
    src/
    src/core/
    src/domain/
    src/repositories/
    src/infrastructure/
    src/services/
    src/ui/
)

# ─── Link libraries ───────────────────────────────────────────────────────────
target_link_libraries(Arcanum PRIVATE
    Qt6::Core
    Qt6::Widgets
    Qt6::Gui
    Qt6::Sql
)

# ─── Compile definitions ──────────────────────────────────────────────────────
if(CF_DEBUG_MODE)
    target_compile_definitions(Arcanum PRIVATE CF_DEBUG=1)
endif()

target_compile_definitions(Arcanum PRIVATE
    CF_APP_VERSION="${PROJECT_VERSION}"
)

# ─── Compiler warnings ────────────────────────────────────────────────────────
if(MSVC)
    target_compile_options(Arcanum PRIVATE /W4 /WX)
else()
    target_compile_options(Arcanum PRIVATE -Wall -Wextra -Wpedantic)
endif()

# ─── Tests subdirectory ───────────────────────────────────────────────────────
option(CF_BUILD_TESTS "Build unit tests" OFF)
if(CF_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()