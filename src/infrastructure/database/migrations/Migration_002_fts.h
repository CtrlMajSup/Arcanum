#pragma once

#include "../MigrationRunner.h"

namespace CF::Infra::Migrations {

/**
 * Migration 002: SQLite FTS5 full-text search tables.
 * Each FTS table is a content table backed by the real table.
 * Triggers keep them in sync automatically.
 */
inline Migration migration_002_fts()
{
    return {
        .version     = 2,
        .description = "FTS5 full-text search indexes",
        .up = []() -> std::vector<QString> {
            return {

            // FTS virtual table over characters
            R"(CREATE VIRTUAL TABLE fts_characters USING fts5(
                name, biography, species,
                content='characters', content_rowid='id'
            ))",

            // Sync triggers for characters
            R"(CREATE TRIGGER fts_characters_insert AFTER INSERT ON characters BEGIN
                INSERT INTO fts_characters(rowid, name, biography, species)
                VALUES (new.id, new.name, new.biography, new.species);
            END)",
            R"(CREATE TRIGGER fts_characters_update AFTER UPDATE ON characters BEGIN
                INSERT INTO fts_characters(fts_characters, rowid, name, biography, species)
                VALUES ('delete', old.id, old.name, old.biography, old.species);
                INSERT INTO fts_characters(rowid, name, biography, species)
                VALUES (new.id, new.name, new.biography, new.species);
            END)",
            R"(CREATE TRIGGER fts_characters_delete AFTER DELETE ON characters BEGIN
                INSERT INTO fts_characters(fts_characters, rowid, name, biography, species)
                VALUES ('delete', old.id, old.name, old.biography, old.species);
            END)",

            // FTS virtual table over places
            R"(CREATE VIRTUAL TABLE fts_places USING fts5(
                name, description, region,
                content='places', content_rowid='id'
            ))",
            R"(CREATE TRIGGER fts_places_insert AFTER INSERT ON places BEGIN
                INSERT INTO fts_places(rowid, name, description, region)
                VALUES (new.id, new.name, new.description, new.region);
            END)",
            R"(CREATE TRIGGER fts_places_update AFTER UPDATE ON places BEGIN
                INSERT INTO fts_places(fts_places, rowid, name, description, region)
                VALUES ('delete', old.id, old.name, old.description, old.region);
                INSERT INTO fts_places(rowid, name, description, region)
                VALUES (new.id, new.name, new.description, new.region);
            END)",
            R"(CREATE TRIGGER fts_places_delete AFTER DELETE ON places BEGIN
                INSERT INTO fts_places(fts_places, rowid, name, description, region)
                VALUES ('delete', old.id, old.name, old.description, old.region);
            END)",

            // FTS virtual table over factions
            R"(CREATE VIRTUAL TABLE fts_factions USING fts5(
                name, description,
                content='factions', content_rowid='id'
            ))",
            R"(CREATE TRIGGER fts_factions_insert AFTER INSERT ON factions BEGIN
                INSERT INTO fts_factions(rowid, name, description)
                VALUES (new.id, new.name, new.description);
            END)",
            R"(CREATE TRIGGER fts_factions_update AFTER UPDATE ON factions BEGIN
                INSERT INTO fts_factions(fts_factions, rowid, name, description)
                VALUES ('delete', old.id, old.name, old.description);
                INSERT INTO fts_factions(rowid, name, description)
                VALUES (new.id, new.name, new.description);
            END)",
            R"(CREATE TRIGGER fts_factions_delete AFTER DELETE ON factions BEGIN
                INSERT INTO fts_factions(fts_factions, rowid, name, description)
                VALUES ('delete', old.id, old.name, old.description);
            END)",

            // FTS virtual table over chapters
            R"(CREATE VIRTUAL TABLE fts_chapters USING fts5(
                title, markdown_content,
                content='chapters', content_rowid='id'
            ))",
            R"(CREATE TRIGGER fts_chapters_insert AFTER INSERT ON chapters BEGIN
                INSERT INTO fts_chapters(rowid, title, markdown_content)
                VALUES (new.id, new.title, new.markdown_content);
            END)",
            R"(CREATE TRIGGER fts_chapters_update AFTER UPDATE ON chapters BEGIN
                INSERT INTO fts_chapters(fts_chapters, rowid, title, markdown_content)
                VALUES ('delete', old.id, old.title, old.markdown_content);
                INSERT INTO fts_chapters(rowid, title, markdown_content)
                VALUES (new.id, new.title, new.markdown_content);
            END)",
            R"(CREATE TRIGGER fts_chapters_delete AFTER DELETE ON chapters BEGIN
                INSERT INTO fts_chapters(fts_chapters, rowid, title, markdown_content)
                VALUES ('delete', old.id, old.title, old.markdown_content);
            END)",

            }; // end return
        }
    };
}

} // namespace CF::Infra::Migrations