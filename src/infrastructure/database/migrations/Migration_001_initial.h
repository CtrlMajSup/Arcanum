#pragma once

#include "../MigrationRunner.h"
#include <QString>

namespace CF::Infra::Migrations {

inline Migration migration_001_initial()
{
    return {
        .version     = 1,
        .description = "Initial schema: all core tables",
        .up = []() -> std::vector<QString> {
            return {

            // ── Places ───────────────────────────────────────────────────────
            R"(CREATE TABLE places (
                id          INTEGER PRIMARY KEY AUTOINCREMENT,
                name        TEXT    NOT NULL,
                type        TEXT    NOT NULL DEFAULT 'Place',
                region      TEXT    NOT NULL DEFAULT '',
                description TEXT    NOT NULL DEFAULT '',
                is_mobile   INTEGER NOT NULL DEFAULT 0,
                map_x       REAL    NOT NULL DEFAULT 0.5,
                map_y       REAL    NOT NULL DEFAULT 0.5,
                parent_id   INTEGER REFERENCES places(id) ON DELETE SET NULL
            ))",

            R"(CREATE TABLE place_evolutions (
                id           INTEGER PRIMARY KEY AUTOINCREMENT,
                place_id     INTEGER NOT NULL REFERENCES places(id) ON DELETE CASCADE,
                era          INTEGER NOT NULL,
                year         INTEGER NOT NULL,
                description  TEXT    NOT NULL,
                UNIQUE(place_id, era, year)
            ))",

            // ── Factions ─────────────────────────────────────────────────────
            R"(CREATE TABLE factions (
                id              INTEGER PRIMARY KEY AUTOINCREMENT,
                name            TEXT    NOT NULL,
                type            TEXT    NOT NULL DEFAULT 'Faction',
                description     TEXT    NOT NULL DEFAULT '',
                icon_ref        TEXT    NOT NULL DEFAULT '',
                founded_era     INTEGER NOT NULL DEFAULT 1,
                founded_year    INTEGER NOT NULL DEFAULT 1,
                dissolved_era   INTEGER,
                dissolved_year  INTEGER
            ))",

            R"(CREATE TABLE faction_evolutions (
                id          INTEGER PRIMARY KEY AUTOINCREMENT,
                faction_id  INTEGER NOT NULL REFERENCES factions(id) ON DELETE CASCADE,
                era         INTEGER NOT NULL,
                year        INTEGER NOT NULL,
                description TEXT    NOT NULL,
                UNIQUE(faction_id, era, year)
            ))",

            // ── Characters ───────────────────────────────────────────────────
            R"(CREATE TABLE characters (
                id          INTEGER PRIMARY KEY AUTOINCREMENT,
                name        TEXT    NOT NULL,
                species     TEXT    NOT NULL DEFAULT '',
                biography   TEXT    NOT NULL DEFAULT '',
                image_ref   TEXT    NOT NULL DEFAULT '',
                born_era    INTEGER NOT NULL DEFAULT 1,
                born_year   INTEGER NOT NULL DEFAULT 1,
                died_era    INTEGER,
                died_year   INTEGER
            ))",

            R"(CREATE TABLE character_evolutions (
                id           INTEGER PRIMARY KEY AUTOINCREMENT,
                character_id INTEGER NOT NULL REFERENCES characters(id) ON DELETE CASCADE,
                era          INTEGER NOT NULL,
                year         INTEGER NOT NULL,
                description  TEXT    NOT NULL,
                UNIQUE(character_id, era, year)
            ))",

            R"(CREATE TABLE character_locations (
                id           INTEGER PRIMARY KEY AUTOINCREMENT,
                character_id INTEGER NOT NULL REFERENCES characters(id) ON DELETE CASCADE,
                place_id     INTEGER NOT NULL REFERENCES places(id)     ON DELETE CASCADE,
                from_era     INTEGER NOT NULL,
                from_year    INTEGER NOT NULL,
                to_era       INTEGER,
                to_year      INTEGER,
                note         TEXT    NOT NULL DEFAULT ''
            ))",

            R"(CREATE TABLE character_faction_memberships (
                id           INTEGER PRIMARY KEY AUTOINCREMENT,
                character_id INTEGER NOT NULL REFERENCES characters(id)  ON DELETE CASCADE,
                faction_id   INTEGER NOT NULL REFERENCES factions(id)    ON DELETE CASCADE,
                from_era     INTEGER NOT NULL,
                from_year    INTEGER NOT NULL,
                to_era       INTEGER,
                to_year      INTEGER,
                role         TEXT    NOT NULL DEFAULT ''
            ))",

            R"(CREATE TABLE character_relationships (
                id            INTEGER PRIMARY KEY AUTOINCREMENT,
                owner_id      INTEGER NOT NULL REFERENCES characters(id) ON DELETE CASCADE,
                target_id     INTEGER NOT NULL REFERENCES characters(id) ON DELETE CASCADE,
                type          TEXT    NOT NULL,
                note          TEXT    NOT NULL DEFAULT '',
                since_era     INTEGER NOT NULL,
                since_year    INTEGER NOT NULL,
                until_era     INTEGER,
                until_year    INTEGER,
                UNIQUE(owner_id, target_id)
            ))",

            // ── Scenes ───────────────────────────────────────────────────────
            R"(CREATE TABLE scenes (
                id          INTEGER PRIMARY KEY AUTOINCREMENT,
                title       TEXT    NOT NULL,
                summary     TEXT    NOT NULL DEFAULT '',
                era         INTEGER NOT NULL,
                year        INTEGER NOT NULL,
                chapter_id  INTEGER REFERENCES chapters(id) ON DELETE SET NULL
            ))",

            R"(CREATE TABLE scene_places (
                scene_id    INTEGER NOT NULL REFERENCES scenes(id)  ON DELETE CASCADE,
                place_id    INTEGER NOT NULL REFERENCES places(id)  ON DELETE CASCADE,
                PRIMARY KEY (scene_id, place_id)
            ))",

            R"(CREATE TABLE scene_factions (
                scene_id    INTEGER NOT NULL REFERENCES scenes(id)   ON DELETE CASCADE,
                faction_id  INTEGER NOT NULL REFERENCES factions(id) ON DELETE CASCADE,
                PRIMARY KEY (scene_id, faction_id)
            ))",

            R"(CREATE TABLE scene_characters (
                scene_id     INTEGER NOT NULL REFERENCES scenes(id)     ON DELETE CASCADE,
                character_id INTEGER NOT NULL REFERENCES characters(id) ON DELETE CASCADE,
                PRIMARY KEY (scene_id, character_id)
            ))",

            // ── Chapters ─────────────────────────────────────────────────────
            R"(CREATE TABLE chapters (
                id               INTEGER PRIMARY KEY AUTOINCREMENT,
                title            TEXT    NOT NULL,
                markdown_content TEXT    NOT NULL DEFAULT '',
                sort_order       INTEGER NOT NULL DEFAULT 0,
                time_start_era   INTEGER,
                time_start_year  INTEGER,
                time_end_era     INTEGER,
                time_end_year    INTEGER,
                notes            TEXT    NOT NULL DEFAULT ''
            ))",

            // ── Timeline events ───────────────────────────────────────────────
            R"(CREATE TABLE timeline_events (
                id           INTEGER PRIMARY KEY AUTOINCREMENT,
                era          INTEGER NOT NULL,
                year         INTEGER NOT NULL,
                event_type   TEXT    NOT NULL,
                subject_id   INTEGER NOT NULL,
                subject_type TEXT    NOT NULL,
                title        TEXT    NOT NULL,
                description  TEXT    NOT NULL DEFAULT ''
            ))",

            // ── Indexes ───────────────────────────────────────────────────────
            "CREATE INDEX idx_characters_name       ON characters(name)",
            "CREATE INDEX idx_places_region         ON places(region)",
            "CREATE INDEX idx_timeline_sort         ON timeline_events(era, year)",
            "CREATE INDEX idx_timeline_subject      ON timeline_events(subject_id)",
            "CREATE INDEX idx_char_locations_char   ON character_locations(character_id)",
            "CREATE INDEX idx_char_locations_place  ON character_locations(place_id)",

            }; // end return
        }
    };
}

} // namespace CF::Infra::Migrations