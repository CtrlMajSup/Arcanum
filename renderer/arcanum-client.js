// ═══════════════════════════════════════════════════════════════
// ARCANUM-CLIENT.JS — Couche client avec cache local
// Wraps window.arcanum (IPC async) avec un cache en mémoire
// Utilisé par tous les modules renderer
// ═══════════════════════════════════════════════════════════════

const AC = (() => {

  // ── CACHE LOCAL ─────────────────────────────────────────────
  let _chars    = null;
  let _lieux    = null;
  let _chaps    = null;
  let _scenes   = null;
  let _cartes   = null;

  // ── HELPERS INTERNES ────────────────────────────────────────
  function charNameStr(c) {
    return [c?.prenom, c?.nom].filter(Boolean).join(' ') || 'Sans nom';
  }
  function charInitialsStr(c) {
    return ((c?.prenom?.[0] || '') + (c?.nom?.[0] || '')).toUpperCase() || '?';
  }
  function lieuFullName(id) {
    const l = _lieux?.[id];
    if (!l) return '—';
    if (l.parentId && _lieux[l.parentId]) return `${l.nom}, ${_lieux[l.parentId].nom}`;
    return l.nom || '—';
  }

  // ── COULEURS LIEUX ──────────────────────────────────────────
  const PALETTE = ['#5B8DB8','#7BAF7B','#C17D6F','#9B7BB5','#C4A35A','#6AABAA','#B5727C','#7A9E7E','#8C6E9E','#B08050'];
  let _colorCache = {};
  function lieuColor(id) {
    if (!id) return '#d0cfc9';
    if (_colorCache[id]) return _colorCache[id];
    const idx = Object.keys(_colorCache).length % PALETTE.length;
    _colorCache[id] = PALETTE[idx];
    return _colorCache[id];
  }
  function resetColorCache() { _colorCache = {}; }

  // ── LOAD ALL (recharge tout depuis IPC) ─────────────────────
  async function loadAll() {
    const [chars, lieux, chaps, scenes] = await Promise.all([
      window.arcanum.chars.all(),
      window.arcanum.lieux.all(),
      window.arcanum.chapitres.all(),
      window.arcanum.scenes.all(),
    ]);
    _chars  = chars;
    _lieux  = lieux;
    _chaps  = chaps;
    _scenes = scenes;
  }

  async function loadChars()  { _chars  = await window.arcanum.chars.all();     return _chars;  }
  async function loadLieux()  { _lieux  = await window.arcanum.lieux.all();     return _lieux;  }
  async function loadChaps()  { _chaps  = await window.arcanum.chapitres.all(); return _chaps;  }
  async function loadScenes() { _scenes = await window.arcanum.scenes.all();    return _scenes; }
  async function loadCartes() { _cartes = await window.arcanum.cartes.all();    return _cartes; }

  // ── PERSONNAGES ─────────────────────────────────────────────
  const Chars = {
    async all()   { if (!_chars)  await loadChars();  return _chars;  },
    async list()  {
      if (!_chars) await loadChars();
      return Object.values(_chars).sort((a, b) =>
        charNameStr(a).localeCompare(charNameStr(b), 'fr')
      );
    },
    get(id)       { return _chars?.[id] || null; },
    name(id)      { return charNameStr(_chars?.[id]); },
    initials(id)  { return charInitialsStr(_chars?.[id]); },
    async save(c) {
      const saved = await window.arcanum.chars.save(c);
      if (!_chars) _chars = {};
      _chars[saved.id] = saved;
      return saved;
    },
    async delete(id) {
      await window.arcanum.chars.delete(id);
      if (_chars) delete _chars[id];
    },
    _getAll()     { return _chars; },  // accès direct au cache (sync)
    avatarHtml(id, size = 30) {
      const c = _chars?.[id];
      const s = `width:${size}px;height:${size}px;font-size:${Math.round(size*0.4)}px`;
      if (c?.image) return `<div class="cav" style="${s}"><img src="${c.image}" alt=""></div>`;
      return `<div class="cav" style="${s}">${charInitialsStr(c)}</div>`;
    },
  };

  // ── LIEUX ───────────────────────────────────────────────────
  const Lieux = {
    async all()   { if (!_lieux) await loadLieux(); return _lieux; },
    get(id)       { return _lieux?.[id] || null; },
    fullName(id)  { return lieuFullName(id); },
    _lieux_raw()   { return _lieux; },  // accès direct cache lieux (sync)
    planetes()    { return Object.values(_lieux || {}).filter(l => l.type === 'planete'); },
    vaisseaux()   { return Object.values(_lieux || {}).filter(l => l.type === 'vaisseau'); },
    enfants(pid)  { return Object.values(_lieux || {}).filter(l => l.parentId === pid); },
    async save(l) {
      const saved = await window.arcanum.lieux.save(l);
      if (!_lieux) _lieux = {};
      _lieux[saved.id] = saved;
      return saved;
    },
    async delete(id) {
      await window.arcanum.lieux.delete(id);
      if (_lieux) {
        delete _lieux[id];
        Object.values(_lieux).forEach(l => { if (l.parentId === id) l.parentId = null; });
      }
    },
    defaultNew(type) {
      return { type, nom: '', description: '', image: '', couleur: '#b0afa9', tags: [], parentId: null, createdAt: Date.now() };
    },
    color: lieuColor,
    resetColorCache,
  };

  // ── CHAPITRES ───────────────────────────────────────────────
  const Chapitres = {
    async all()   { if (!_chaps) await loadChaps(); return _chaps; },
    async list()  {
      if (!_chaps) await loadChaps();
      return Object.values(_chaps).sort((a, b) => (a.numero || 0) - (b.numero || 0));
    },
    get(id)       { return _chaps?.[id] || null; },
    async save(c) {
      const saved = await window.arcanum.chapitres.save(c);
      if (!_chaps) _chaps = {};
      _chaps[saved.id] = saved;
      return saved;
    },
    async delete(id) {
      await window.arcanum.chapitres.delete(id);
      if (_chaps) delete _chaps[id];
    },
    async defaultNew() {
      const list = await this.list();
      const maxNum = list.reduce((m, c) => Math.max(m, c.numero || 0), 0);
      return { numero: maxNum + 1, titre: '', acte: '', dateFiction: '', description: '', createdAt: Date.now() };
    },
  };

  // ── SCÈNES ──────────────────────────────────────────────────
  const Scenes = {
    async all()   { if (!_scenes) await loadScenes(); return _scenes; },
    get(id)       { return _scenes?.[id] || null; },
    byChap(chapId) {
      return Object.values(_scenes || {})
        .filter(s => s.chapitreId === chapId)
        .sort((a, b) => (a.ordre || 0) - (b.ordre || 0));
    },
    byLieu(lieuId) {
      return Object.values(_scenes || {}).filter(s => s.lieuId === lieuId);
    },
    matrice() {
      const matrix = {};
      Object.values(_scenes || {}).forEach(s => {
        if (!s.chapitreId || !s.lieuId) return;
        if (!matrix[s.chapitreId]) matrix[s.chapitreId] = {};
        (s.personnageIds || []).forEach(cid => { matrix[s.chapitreId][cid] = s.lieuId; });
      });
      return matrix;
    },
    async save(s) {
      const saved = await window.arcanum.scenes.save(s);
      if (!_scenes) _scenes = {};
      _scenes[saved.id] = saved;
      return saved;
    },
    async delete(id) {
      await window.arcanum.scenes.delete(id);
      if (_scenes) delete _scenes[id];
    },
    async defaultNew(chapitreId) {
      const existing = this.byChap(chapitreId || '');
      const maxOrdre = existing.reduce((m, s) => Math.max(m, s.ordre || 0), 0);
      return { titre: '', chapitreId: chapitreId || null, lieuId: null, personnageIds: [], dateFiction: '', heureFiction: '', note: '', ordre: maxOrdre + 1, createdAt: Date.now() };
    },
  };

  // ── CARTES ──────────────────────────────────────────────────
  const Cartes = {
    async all()        { if (!_cartes) await loadCartes(); return _cartes; },
    get(id)            { return _cartes?.[id] || null; },
    forLieu(lieuId)    { return Object.values(_cartes || {}).filter(c => c.lieuId === lieuId); },
    defaultNew(lieuId) {
      return { lieuId: lieuId||null, nom:'Nouvelle carte', elements:[], layers:[{id:'base',nom:'Fond',visible:true}], viewport:{x:0,y:0,zoom:1}, width:1600, height:1200, createdAt:Date.now() };
    },
    async save(c) {
      const saved = await window.arcanum.cartes.save(c);
      if (!_cartes) _cartes = {};
      _cartes[saved.id] = saved;
      return saved;
    },
    async delete(id) {
      await window.arcanum.cartes.delete(id);
      if (_cartes) delete _cartes[id];
    },
  };

  // ── IO ──────────────────────────────────────────────────────
  const IO = {
    newProject:  () => window.arcanum.file.newProject(),
    openProject: () => window.arcanum.file.openProject(),
    save:        () => window.arcanum.file.saveProject(),
    saveAs:      () => window.arcanum.file.saveAs(),
    getPath:     () => window.arcanum.file.getPath(),
    isOpen:      () => window.arcanum.file.isOpen(),
    exportJSON:  () => window.arcanum.file.exportJSON(),
    importJSON:  () => window.arcanum.file.importJSON(),
  };

  // ── UTILS UI ────────────────────────────────────────────────
  function esc(s) {
    return String(s || '').replace(/&/g, '&amp;').replace(/"/g, '&quot;').replace(/</g, '&lt;');
  }

  function toast(msg, type = 'ok') {
    const t = document.createElement('div');
    t.className = 'toast';
    t.textContent = msg;
    document.body.appendChild(t);
    setTimeout(() => t.remove(), 2500);
  }

  function modal(title, msg, onOk, okLabel = 'Confirmer') {
    const m = document.createElement('div');
    m.className = 'modal-overlay';
    m.innerHTML = `<div class="modal">
      <h3>${esc(title)}</h3><p>${esc(msg)}</p>
      <div class="modal-actions">
        <button class="btn btn-outline" onclick="this.closest('.modal-overlay').remove()">Annuler</button>
        <button class="btn btn-dark" id="_mOk">${esc(okLabel)}</button>
      </div>
    </div>`;
    document.body.appendChild(m);
    m.querySelector('#_mOk').onclick = () => { m.remove(); onOk(); };
  }

  // ── INVALIDATE (appelé après import/reload) ──────────────────
  function invalidate() {
    _chars = null; _lieux = null; _chaps = null; _scenes = null; _cartes = null;
    resetColorCache();
  }

  async function getUid() { return await window.arcanum.uid(); }
  // API publique
  return { loadAll, Chars, Lieux, Chapitres, Scenes, Cartes, IO, lieuColor, resetColorCache, esc, toast, modal, invalidate, uid: getUid };

})();

// Écoute les événements de rechargement depuis main
if (window.arcanum?.on) {
  window.arcanum.on('app:fileOpened', () => { AC.invalidate(); location.reload(); });
  window.arcanum.on('app:newProject', () => { AC.invalidate(); location.reload(); });
}
