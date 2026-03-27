// ═══════════════════════════════════════════════════════════════
// ARCANUM.JS (renderer) — API côté interface
// Enveloppe window.arcanum (IPC) avec des helpers synchrones-like
// Compatible avec le code existant des modules HTML
// ═══════════════════════════════════════════════════════════════

// Toutes les méthodes retournent des Promises via IPC.
// On expose un objet ARCANUM qui ressemble à l'ancien API synchrone
// mais dont les méthodes sont async.

const ARCANUM = {

  // ── ID ──────────────────────────────────────────────────────
  uid: () => window.arcanum.uid(),

  // ── PERSONNAGES ─────────────────────────────────────────────
  Chars: {
    all:      ()    => window.arcanum.chars.all(),
    get:      (id)  => window.arcanum.chars.get(id),
    save:     (c)   => window.arcanum.chars.save(c),
    delete:   (id)  => window.arcanum.chars.delete(id),
    async name(id) {
      const c = await window.arcanum.chars.get(id);
      if (!c) return '—';
      return [c.prenom, c.nom].filter(Boolean).join(' ') || 'Sans nom';
    },
    async initials(id) {
      const c = await window.arcanum.chars.get(id);
      if (!c) return '?';
      return ((c.prenom?.[0] || '') + (c.nom?.[0] || '')).toUpperCase() || '?';
    },
    async list() {
      const all = await window.arcanum.chars.all();
      return Object.values(all).sort((a, b) =>
        [a.prenom, a.nom].filter(Boolean).join(' ')
          .localeCompare([b.prenom, b.nom].filter(Boolean).join(' '), 'fr')
      );
    },
  },

  // ── LIEUX ───────────────────────────────────────────────────
  Lieux: {
    all:      ()    => window.arcanum.lieux.all(),
    get:      (id)  => window.arcanum.lieux.get(id),
    save:     (l)   => window.arcanum.lieux.save(l),
    delete:   (id)  => window.arcanum.lieux.delete(id),
    fullName: (id)  => window.arcanum.lieux.fullName(id),
    async planetes() {
      const all = await window.arcanum.lieux.all();
      return Object.values(all).filter(l => l.type === 'planete');
    },
    async vaisseaux() {
      const all = await window.arcanum.lieux.all();
      return Object.values(all).filter(l => l.type === 'vaisseau');
    },
    async enfants(parentId) {
      const all = await window.arcanum.lieux.all();
      return Object.values(all).filter(l => l.parentId === parentId);
    },
    defaultNew(type) {
      return {
        type, nom: '', description: '', image: '',
        couleur: '#b0afa9', tags: [], parentId: null, createdAt: Date.now(),
      };
    },
  },

  // ── CHAPITRES ───────────────────────────────────────────────
  Chapitres: {
    all:    ()    => window.arcanum.chapitres.all(),
    list:   ()    => window.arcanum.chapitres.list(),
    get:    (id)  => window.arcanum.chapitres.get(id),
    save:   (c)   => window.arcanum.chapitres.save(c),
    delete: (id)  => window.arcanum.chapitres.delete(id),
    async defaultNew() {
      const existing = await window.arcanum.chapitres.list();
      const maxNum = existing.reduce((m, c) => Math.max(m, c.numero || 0), 0);
      return { numero: maxNum + 1, titre: '', acte: '', dateFiction: '', description: '', createdAt: Date.now() };
    },
  },

  // ── SCÈNES ──────────────────────────────────────────────────
  Scenes: {
    all:     ()        => window.arcanum.scenes.all(),
    get:     (id)      => window.arcanum.scenes.get(id),
    save:    (s)       => window.arcanum.scenes.save(s),
    delete:  (id)      => window.arcanum.scenes.delete(id),
    byChap:  (chapId)  => window.arcanum.scenes.byChap(chapId),
    byLieu:  (lieuId)  => window.arcanum.scenes.byLieu(lieuId),
    matrice: ()        => window.arcanum.scenes.matrice(),
    async defaultNew(chapitreId) {
      const existing = await window.arcanum.scenes.byChap(chapitreId || '');
      const maxOrdre = existing.reduce((m, s) => Math.max(m, s.ordre || 0), 0);
      return {
        titre: '', chapitreId: chapitreId || null, lieuId: null,
        personnageIds: [], dateFiction: '', heureFiction: '',
        note: '', ordre: maxOrdre + 1, createdAt: Date.now(),
      };
    },
  },

  // ── CARTES ──────────────────────────────────────────────────
  Cartes: {
    all:     ()        => window.arcanum.cartes.all(),
    get:     (id)      => window.arcanum.cartes.get(id),
    forLieu: (lieuId)  => window.arcanum.cartes.forLieu(lieuId),
    save:    (c)       => window.arcanum.cartes.save(c),
    delete:  (id)      => window.arcanum.cartes.delete(id),
    defaultNew(lieuId) {
      return {
        lieuId: lieuId || null, nom: 'Nouvelle carte',
        elements: [], layers: [{ id: 'base', nom: 'Fond', visible: true }],
        viewport: { x: 0, y: 0, zoom: 1 },
        width: 1600, height: 1200,
        createdAt: Date.now(),
      };
    },
  },

  // ── AUTOSAVE ────────────────────────────────────────────────
  AutoSave: {
    list:    ()    => window.arcanum.autosave.list(),
    restore: (id)  => window.arcanum.autosave.restore(id),
  },

  // ── IO ──────────────────────────────────────────────────────
  IO: {
    newProject:  () => window.arcanum.file.newProject(),
    openProject: () => window.arcanum.file.openProject(),
    save:        () => window.arcanum.file.saveProject(),
    saveAs:      () => window.arcanum.file.saveAs(),
    getPath:     () => window.arcanum.file.getPath(),
    isOpen:      () => window.arcanum.file.isOpen(),
    exportJSON:  () => window.arcanum.file.exportJSON(),
    importJSON:  () => window.arcanum.file.importJSON(),
  },

  // ── COULEURS PAR LIEU ───────────────────────────────────────
  _colorCache: {},
  _palette: [
    '#5B8DB8','#7BAF7B','#C17D6F','#9B7BB5','#C4A35A',
    '#6AABAA','#B5727C','#7A9E7E','#8C6E9E','#B08050',
  ],
  lieuColor(lieuId) {
    if (!lieuId) return '#d0cfc9';
    if (this._colorCache[lieuId]) return this._colorCache[lieuId];
    const idx = Object.keys(this._colorCache).length % this._palette.length;
    this._colorCache[lieuId] = this._palette[idx];
    return this._colorCache[lieuId];
  },
  resetColorCache() { this._colorCache = {}; },
};

// ── HELPERS UI COMMUNS ─────────────────────────────────────────

function esc(s) {
  return String(s || '').replace(/&/g, '&amp;').replace(/"/g, '&quot;').replace(/</g, '&lt;');
}

function charName(c) {
  return [c?.prenom, c?.nom].filter(Boolean).join(' ') || 'Sans nom';
}

function charInitials(c) {
  return ((c?.prenom?.[0] || '') + (c?.nom?.[0] || '')).toUpperCase() || '?';
}

function toast(msg, type = 'info') {
  const el = document.createElement('div');
  el.className = 'toast' + (type === 'error' ? ' toast-error' : '');
  el.textContent = msg;
  document.body.appendChild(el);
  setTimeout(() => el.remove(), 2500);
}

function modal(title, msg, onOk, okLabel = 'Confirmer') {
  const m = document.createElement('div');
  m.className = 'modal-overlay';
  m.innerHTML = `<div class="modal">
    <h3>${esc(title)}</h3><p>${esc(msg)}</p>
    <div class="modal-actions">
      <button class="btn btn-outline" onclick="this.closest('.modal-overlay').remove()">Annuler</button>
      <button class="btn btn-dark" id="mOk">${esc(okLabel)}</button>
    </div>
  </div>`;
  document.body.appendChild(m);
  m.querySelector('#mOk').onclick = () => { m.remove(); onOk(); };
}
