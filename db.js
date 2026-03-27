'use strict';
// ═══════════════════════════════════════════════════════════════
// DB.JS — Couche de données SQLite pour Arcanum
// Utilisé exclusivement par main.js (processus Node)
// ═══════════════════════════════════════════════════════════════

const Database = require('better-sqlite3');
const path = require('path');
const fs = require('fs');

let db = null;
let currentFilePath = null;

// ── SCHÉMA ────────────────────────────────────────────────────
const SCHEMA = `
PRAGMA journal_mode=WAL;
PRAGMA foreign_keys=ON;

CREATE TABLE IF NOT EXISTS meta (
  key   TEXT PRIMARY KEY,
  value TEXT
);

CREATE TABLE IF NOT EXISTS personnages (
  id         TEXT PRIMARY KEY,
  data       TEXT NOT NULL,
  created_at INTEGER DEFAULT (strftime('%s','now')),
  updated_at INTEGER DEFAULT (strftime('%s','now'))
);

CREATE TABLE IF NOT EXISTS lieux (
  id         TEXT PRIMARY KEY,
  type       TEXT NOT NULL CHECK(type IN ('planete','ville','vaisseau')),
  nom        TEXT,
  parent_id  TEXT REFERENCES lieux(id) ON DELETE SET NULL,
  data       TEXT NOT NULL,
  created_at INTEGER DEFAULT (strftime('%s','now')),
  updated_at INTEGER DEFAULT (strftime('%s','now'))
);

CREATE TABLE IF NOT EXISTS chapitres (
  id         TEXT PRIMARY KEY,
  numero     INTEGER,
  titre      TEXT,
  data       TEXT NOT NULL,
  created_at INTEGER DEFAULT (strftime('%s','now'))
);

CREATE TABLE IF NOT EXISTS scenes (
  id           TEXT PRIMARY KEY,
  titre        TEXT,
  chapitre_id  TEXT REFERENCES chapitres(id) ON DELETE SET NULL,
  lieu_id      TEXT REFERENCES lieux(id) ON DELETE SET NULL,
  ordre        INTEGER DEFAULT 0,
  data         TEXT NOT NULL,
  created_at   INTEGER DEFAULT (strftime('%s','now')),
  updated_at   INTEGER DEFAULT (strftime('%s','now'))
);

CREATE TABLE IF NOT EXISTS cartes (
  id         TEXT PRIMARY KEY,
  lieu_id    TEXT REFERENCES lieux(id) ON DELETE CASCADE,
  nom        TEXT,
  data       TEXT NOT NULL,
  created_at INTEGER DEFAULT (strftime('%s','now')),
  updated_at INTEGER DEFAULT (strftime('%s','now'))
);

CREATE TABLE IF NOT EXISTS sauvegardes_auto (
  id         INTEGER PRIMARY KEY AUTOINCREMENT,
  snapshot   TEXT NOT NULL,
  created_at INTEGER DEFAULT (strftime('%s','now'))
);

CREATE INDEX IF NOT EXISTS idx_scenes_chapitre ON scenes(chapitre_id);
CREATE INDEX IF NOT EXISTS idx_scenes_lieu ON scenes(lieu_id);
CREATE INDEX IF NOT EXISTS idx_lieux_parent ON lieux(parent_id);
CREATE INDEX IF NOT EXISTS idx_cartes_lieu ON cartes(lieu_id);

CREATE TABLE IF NOT EXISTS textes (
  id         TEXT PRIMARY KEY,
  scene_id   TEXT REFERENCES scenes(id) ON DELETE SET NULL,
  contenu    TEXT NOT NULL DEFAULT '',
  updated_at INTEGER DEFAULT (strftime('%s','now'))
);
CREATE INDEX IF NOT EXISTS idx_textes_scene ON textes(scene_id);
`;

// ── OUVERTURE / CRÉATION ──────────────────────────────────────
function open(filePath) {
  if (db) { db.close(); db = null; }
  currentFilePath = filePath;
  db = new Database(filePath);
  db.exec(SCHEMA);
  // Métadonnées
  const version = db.prepare('SELECT value FROM meta WHERE key=?').get('schema_version');
  if (!version) {
    db.prepare("INSERT INTO meta VALUES ('schema_version','1')").run();
    db.prepare("INSERT INTO meta VALUES ('app_name','Arcanum')").run();
    db.prepare("INSERT INTO meta VALUES ('created_at',?)").run(Date.now().toString());
  }
  return true;
}

function close() {
  if (db) { db.close(); db = null; currentFilePath = null; }
}

function isOpen() { return db !== null; }
function getPath() { return currentFilePath; }

// ── UTILITAIRES ───────────────────────────────────────────────
function uid() {
  return Date.now().toString(36) + Math.random().toString(36).slice(2, 6);
}

function now() { return Math.floor(Date.now() / 1000); }

// ── PERSONNAGES ───────────────────────────────────────────────
const Chars = {
  all() {
    const rows = db.prepare('SELECT id, data FROM personnages ORDER BY json_extract(data,\'$.prenom\'), json_extract(data,\'$.nom\')').all();
    const result = {};
    rows.forEach(r => { result[r.id] = JSON.parse(r.data); });
    return result;
  },
  get(id) {
    const row = db.prepare('SELECT data FROM personnages WHERE id=?').get(id);
    return row ? JSON.parse(row.data) : null;
  },
  save(char) {
    if (!char.id) char.id = uid();
    char.updatedAt = Date.now();
    db.prepare(`INSERT INTO personnages(id,data,updated_at) VALUES(?,?,?)
      ON CONFLICT(id) DO UPDATE SET data=excluded.data, updated_at=excluded.updated_at`)
      .run(char.id, JSON.stringify(char), now());
    return char;
  },
  delete(id) {
    db.prepare('DELETE FROM personnages WHERE id=?').run(id);
  },
};

// ── LIEUX ─────────────────────────────────────────────────────
const Lieux = {
  all() {
    const rows = db.prepare('SELECT id, data FROM lieux').all();
    const result = {};
    rows.forEach(r => { result[r.id] = JSON.parse(r.data); });
    return result;
  },
  get(id) {
    const row = db.prepare('SELECT data FROM lieux WHERE id=?').get(id);
    return row ? JSON.parse(row.data) : null;
  },
  save(lieu) {
    if (!lieu.id) lieu.id = uid();
    lieu.updatedAt = Date.now();
    db.prepare(`INSERT INTO lieux(id,type,nom,parent_id,data,updated_at) VALUES(?,?,?,?,?,?)
      ON CONFLICT(id) DO UPDATE SET type=excluded.type, nom=excluded.nom,
      parent_id=excluded.parent_id, data=excluded.data, updated_at=excluded.updated_at`)
      .run(lieu.id, lieu.type || 'planete', lieu.nom || '', lieu.parentId || null, JSON.stringify(lieu), now());
    return lieu;
  },
  delete(id) {
    db.prepare('DELETE FROM lieux WHERE id=?').run(id);
  },
  fullName(id) {
    const lieu = this.get(id);
    if (!lieu) return '—';
    if (lieu.parentId) {
      const parent = this.get(lieu.parentId);
      if (parent) return `${lieu.nom}, ${parent.nom}`;
    }
    return lieu.nom || '—';
  },
};

// ── CHAPITRES ─────────────────────────────────────────────────
const Chapitres = {
  all() {
    const rows = db.prepare('SELECT id, data FROM chapitres ORDER BY numero').all();
    const result = {};
    rows.forEach(r => { result[r.id] = JSON.parse(r.data); });
    return result;
  },
  list() {
    return db.prepare('SELECT data FROM chapitres ORDER BY numero').all()
      .map(r => JSON.parse(r.data));
  },
  get(id) {
    const row = db.prepare('SELECT data FROM chapitres WHERE id=?').get(id);
    return row ? JSON.parse(row.data) : null;
  },
  save(chap) {
    if (!chap.id) chap.id = uid();
    db.prepare(`INSERT INTO chapitres(id,numero,titre,data) VALUES(?,?,?,?)
      ON CONFLICT(id) DO UPDATE SET numero=excluded.numero, titre=excluded.titre, data=excluded.data`)
      .run(chap.id, chap.numero || 0, chap.titre || '', JSON.stringify(chap));
    return chap;
  },
  delete(id) {
    db.prepare('DELETE FROM chapitres WHERE id=?').run(id);
  },
};

// ── SCÈNES ────────────────────────────────────────────────────
const Scenes = {
  all() {
    const rows = db.prepare('SELECT id, data FROM scenes').all();
    const result = {};
    rows.forEach(r => { result[r.id] = JSON.parse(r.data); });
    return result;
  },
  get(id) {
    const row = db.prepare('SELECT data FROM scenes WHERE id=?').get(id);
    return row ? JSON.parse(row.data) : null;
  },
  save(scene) {
    if (!scene.id) scene.id = uid();
    scene.updatedAt = Date.now();
    db.prepare(`INSERT INTO scenes(id,titre,chapitre_id,lieu_id,ordre,data,updated_at) VALUES(?,?,?,?,?,?,?)
      ON CONFLICT(id) DO UPDATE SET titre=excluded.titre, chapitre_id=excluded.chapitre_id,
      lieu_id=excluded.lieu_id, ordre=excluded.ordre, data=excluded.data, updated_at=excluded.updated_at`)
      .run(scene.id, scene.titre || '', scene.chapitreId || null, scene.lieuId || null,
           scene.ordre || 0, JSON.stringify(scene), now());
    return scene;
  },
  delete(id) {
    db.prepare('DELETE FROM scenes WHERE id=?').run(id);
  },
  byChap(chapId) {
    return db.prepare('SELECT data FROM scenes WHERE chapitre_id=? ORDER BY ordre').all(chapId)
      .map(r => JSON.parse(r.data));
  },
  byLieu(lieuId) {
    return db.prepare('SELECT data FROM scenes WHERE lieu_id=?').all(lieuId)
      .map(r => JSON.parse(r.data));
  },
  matrice() {
    const rows = db.prepare('SELECT chapitre_id, lieu_id, data FROM scenes WHERE chapitre_id IS NOT NULL AND lieu_id IS NOT NULL').all();
    const matrix = {};
    rows.forEach(r => {
      const scene = JSON.parse(r.data);
      if (!matrix[r.chapitre_id]) matrix[r.chapitre_id] = {};
      (scene.personnageIds || []).forEach(cid => {
        matrix[r.chapitre_id][cid] = r.lieu_id;
      });
    });
    return matrix;
  },
};

// ── TEXTES ────────────────────────────────────────────────────
const Textes = {
  all() {
    const rows = db.prepare('SELECT id, data FROM textes').all();
    const result = {};
    rows.forEach(r => {
      const d = JSON.parse(r.data || '{}');
      result[r.id] = d;
    });
    return result;
  },
  byScene(sceneId) {
    const row = db.prepare('SELECT * FROM textes WHERE scene_id=?').get(sceneId);
    if (!row) return null;
    return { id: row.id, sceneId: row.scene_id, contenu: row.contenu, updatedAt: row.updated_at };
  },
  save(texte) {
    if (!texte.id) texte.id = uid();
    texte.updatedAt = Math.floor(Date.now()/1000);
    db.prepare(`INSERT INTO textes(id, scene_id, contenu, updated_at) VALUES(?,?,?,?)
      ON CONFLICT(id) DO UPDATE SET scene_id=excluded.scene_id, contenu=excluded.contenu, updated_at=excluded.updated_at`)
      .run(texte.id, texte.sceneId||null, texte.contenu||'', texte.updatedAt);
    return texte;
  },
  delete(id) { db.prepare('DELETE FROM textes WHERE id=?').run(id); },
};

// ── CARTES ────────────────────────────────────────────────────
const Cartes = {
  all() {
    const rows = db.prepare('SELECT id, lieu_id, data FROM cartes').all();
    const result = {};
    rows.forEach(r => { result[r.id] = JSON.parse(r.data); });
    return result;
  },
  get(id) {
    const row = db.prepare('SELECT data FROM cartes WHERE id=?').get(id);
    return row ? JSON.parse(row.data) : null;
  },
  forLieu(lieuId) {
    const rows = db.prepare('SELECT data FROM cartes WHERE lieu_id=?').all(lieuId);
    return rows.map(r => JSON.parse(r.data));
  },
  save(carte) {
    if (!carte.id) carte.id = uid();
    carte.updatedAt = Date.now();
    db.prepare(`INSERT INTO cartes(id,lieu_id,nom,data,updated_at) VALUES(?,?,?,?,?)
      ON CONFLICT(id) DO UPDATE SET lieu_id=excluded.lieu_id, nom=excluded.nom,
      data=excluded.data, updated_at=excluded.updated_at`)
      .run(carte.id, carte.lieuId || null, carte.nom || '', JSON.stringify(carte), now());
    return carte;
  },
  delete(id) {
    db.prepare('DELETE FROM cartes WHERE id=?').run(id);
  },
};

// ── SAUVEGARDE AUTO ───────────────────────────────────────────
const AutoSave = {
  save() {
    if (!db) return;
    const snapshot = JSON.stringify({
      personnages: Chars.all(),
      lieux:       Lieux.all(),
      chapitres:   Chapitres.all(),
      scenes:      Scenes.all(),
      cartes:      Cartes.all(),
    });
    db.prepare('INSERT INTO sauvegardes_auto(snapshot) VALUES(?)').run(snapshot);
    // Garder seulement les 20 dernières sauvegardes
    db.prepare(`DELETE FROM sauvegardes_auto WHERE id NOT IN
      (SELECT id FROM sauvegardes_auto ORDER BY created_at DESC LIMIT 20)`).run();
  },
  list() {
    return db.prepare('SELECT id, created_at FROM sauvegardes_auto ORDER BY created_at DESC').all();
  },
  restore(id) {
    const row = db.prepare('SELECT snapshot FROM sauvegardes_auto WHERE id=?').get(id);
    if (!row) return false;
    const data = JSON.parse(row.snapshot);
    importData(data);
    return true;
  },
};

// ── IMPORT / EXPORT COMPLET ───────────────────────────────────
function exportAll() {
  return {
    version: 1,
    app: 'Arcanum',
    exportedAt: new Date().toISOString(),
    personnages: Chars.all(),
    lieux:       Lieux.all(),
    chapitres:   Chapitres.all(),
    scenes:      Scenes.all(),
    cartes:      Cartes.all(),
  };
}

function importData(data) {
  const insert = db.transaction(() => {
    if (data.personnages) {
      Object.values(data.personnages).forEach(c => Chars.save(c));
    }
    if (data.lieux) {
      Object.values(data.lieux).forEach(l => Lieux.save(l));
    }
    if (data.chapitres) {
      Object.values(data.chapitres).forEach(c => Chapitres.save(c));
    }
    if (data.scenes) {
      Object.values(data.scenes).forEach(s => Scenes.save(s));
    }
    if (data.cartes) {
      Object.values(data.cartes).forEach(c => Cartes.save(c));
    }
  });
  insert();
}

// Export depuis un ancien JSON arcanum (migration)
function importFromJSON(jsonData) {
  importData(jsonData);
}

module.exports = {
  open, close, isOpen, getPath, uid,
  Chars, Lieux, Chapitres, Scenes, Cartes, Textes, AutoSave,
  exportAll, importData, importFromJSON,
};
