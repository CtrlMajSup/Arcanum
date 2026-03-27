'use strict';
// ═══════════════════════════════════════════════════════════════
// PRELOAD.JS — Pont IPC sécurisé (contextIsolation=true)
// Expose uniquement les fonctions nécessaires au renderer
// via window.arcanum
// ═══════════════════════════════════════════════════════════════

const { contextBridge, ipcRenderer } = require('electron');

contextBridge.exposeInMainWorld('arcanum', {

  // ── FICHIER ────────────────────────────────────────────────
  file: {
    newProject:   ()         => ipcRenderer.invoke('file:new'),
    openProject:  ()         => ipcRenderer.invoke('file:open'),
    saveProject:  ()         => ipcRenderer.invoke('file:save'),
    saveAs:       ()         => ipcRenderer.invoke('file:saveAs'),
    getPath:      ()         => ipcRenderer.invoke('file:getPath'),
    isOpen:       ()         => ipcRenderer.invoke('file:isOpen'),
    exportJSON:   ()         => ipcRenderer.invoke('file:exportJSON'),
    importJSON:   ()         => ipcRenderer.invoke('file:importJSON'),
  },

  // ── PERSONNAGES ────────────────────────────────────────────
  chars: {
    all:    ()    => ipcRenderer.invoke('chars:all'),
    get:    (id)  => ipcRenderer.invoke('chars:get', id),
    save:   (c)   => ipcRenderer.invoke('chars:save', c),
    delete: (id)  => ipcRenderer.invoke('chars:delete', id),
  },

  // ── LIEUX ──────────────────────────────────────────────────
  lieux: {
    all:      ()    => ipcRenderer.invoke('lieux:all'),
    get:      (id)  => ipcRenderer.invoke('lieux:get', id),
    save:     (l)   => ipcRenderer.invoke('lieux:save', l),
    delete:   (id)  => ipcRenderer.invoke('lieux:delete', id),
    fullName: (id)  => ipcRenderer.invoke('lieux:fullName', id),
  },

  // ── CHAPITRES ──────────────────────────────────────────────
  chapitres: {
    all:    ()    => ipcRenderer.invoke('chapitres:all'),
    list:   ()    => ipcRenderer.invoke('chapitres:list'),
    get:    (id)  => ipcRenderer.invoke('chapitres:get', id),
    save:   (c)   => ipcRenderer.invoke('chapitres:save', c),
    delete: (id)  => ipcRenderer.invoke('chapitres:delete', id),
  },

  // ── SCÈNES ─────────────────────────────────────────────────
  scenes: {
    all:      ()        => ipcRenderer.invoke('scenes:all'),
    get:      (id)      => ipcRenderer.invoke('scenes:get', id),
    save:     (s)       => ipcRenderer.invoke('scenes:save', s),
    delete:   (id)      => ipcRenderer.invoke('scenes:delete', id),
    byChap:   (chapId)  => ipcRenderer.invoke('scenes:byChap', chapId),
    byLieu:   (lieuId)  => ipcRenderer.invoke('scenes:byLieu', lieuId),
    matrice:  ()        => ipcRenderer.invoke('scenes:matrice'),
  },

  // ── TEXTES ─────────────────────────────────────────────────
  textes: {
    byScene: (sid) => ipcRenderer.invoke('textes:byScene', sid),
    save:    (t)   => ipcRenderer.invoke('textes:save', t),
    delete:  (id)  => ipcRenderer.invoke('textes:delete', id),
  },

  // ── CARTES ─────────────────────────────────────────────────
  cartes: {
    all:      ()        => ipcRenderer.invoke('cartes:all'),
    get:      (id)      => ipcRenderer.invoke('cartes:get', id),
    forLieu:  (lieuId)  => ipcRenderer.invoke('cartes:forLieu', lieuId),
    save:     (c)       => ipcRenderer.invoke('cartes:save', c),
    delete:   (id)      => ipcRenderer.invoke('cartes:delete', id),
  },

  // ── SAUVEGARDE AUTO ────────────────────────────────────────
  autosave: {
    list:    ()    => ipcRenderer.invoke('autosave:list'),
    restore: (id)  => ipcRenderer.invoke('autosave:restore', id),
  },

  // ── UTILITAIRES ────────────────────────────────────────────
  uid: () => ipcRenderer.invoke('util:uid'),

  // ── ÉVÉNEMENTS ENTRANTS (main → renderer) ─────────────────
  on: (channel, fn) => {
    const allowed = ['app:fileOpened', 'app:fileSaved', 'app:newProject', 'app:autosaved', 'app:titleUpdate', 'app:showAutosaves'];
    if (allowed.includes(channel)) {
      ipcRenderer.on(channel, (_, ...args) => fn(...args));
    }
  },
  off: (channel) => {
    ipcRenderer.removeAllListeners(channel);
  },
});
