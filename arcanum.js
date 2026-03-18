// ═══════════════════════════════════════════════════════════════
// ARCANUM.JS — Couche de données partagée entre tous les modules
// Stockage : localStorage, clé par type d'entité
// ═══════════════════════════════════════════════════════════════

const ARCANUM = (() => {

  // ── CLÉS DE STOCKAGE ──────────────────────────────────────────
  const KEYS = {
    chars:    'arcanum_v3',       // migration depuis v3
    lieux:    'arcanum_lieux',
    scenes:   'arcanum_scenes',
    chapitres:'arcanum_chapitres',
  };

  // ── UTILITAIRES ───────────────────────────────────────────────
  function uid() {
    return Date.now().toString(36) + Math.random().toString(36).slice(2, 6);
  }

  function load(key) {
    try { return JSON.parse(localStorage.getItem(key)) || {}; }
    catch(e) { return {}; }
  }

  function save(key, data) {
    localStorage.setItem(key, JSON.stringify(data));
  }

  // ── PERSONNAGES ───────────────────────────────────────────────
  const Chars = {
    all()      { return load(KEYS.chars); },
    get(id)    { return load(KEYS.chars)[id]; },
    name(id)   {
      const c = load(KEYS.chars)[id];
      if (!c) return '—';
      return [c.prenom, c.nom].filter(Boolean).join(' ') || 'Sans nom';
    },
    initials(id) {
      const c = load(KEYS.chars)[id];
      if (!c) return '?';
      return ((c.prenom?.[0] || '') + (c.nom?.[0] || '')).toUpperCase() || '?';
    },
    list() {
      return Object.values(load(KEYS.chars)).sort((a, b) =>
        [a.prenom,a.nom].filter(Boolean).join(' ')
          .localeCompare([b.prenom,b.nom].filter(Boolean).join(' '), 'fr')
      );
    },
  };

  // ── LIEUX ─────────────────────────────────────────────────────
  // Structure : { id, type:'planete'|'ville'|'vaisseau', nom, parentId?,
  //               description, image, couleur, tags:[] }
  const Lieux = {
    all()   { return load(KEYS.lieux); },
    get(id) { return load(KEYS.lieux)[id]; },
    save(lieu) {
      const data = load(KEYS.lieux);
      if (!lieu.id) lieu.id = uid();
      lieu.updatedAt = Date.now();
      data[lieu.id] = lieu;
      save(KEYS.lieux, data);
      return lieu;
    },
    delete(id) {
      const data = load(KEYS.lieux);
      delete data[id];
      // orpheline les enfants
      Object.values(data).forEach(l => { if (l.parentId === id) l.parentId = null; });
      save(KEYS.lieux, data);
    },
    // Retourne les planètes (top-level)
    planetes() {
      return Object.values(load(KEYS.lieux)).filter(l => l.type === 'planete');
    },
    // Retourne les enfants directs d'un lieu
    enfants(parentId) {
      return Object.values(load(KEYS.lieux)).filter(l => l.parentId === parentId);
    },
    // Retourne vaisseaux (position dynamique gérée via scènes)
    vaisseaux() {
      return Object.values(load(KEYS.lieux)).filter(l => l.type === 'vaisseau');
    },
    // Nom complet avec hiérarchie : "Ville, Planète"
    fullName(id) {
      const data = load(KEYS.lieux);
      const lieu = data[id];
      if (!lieu) return '—';
      if (lieu.parentId && data[lieu.parentId]) {
        return `${lieu.nom}, ${data[lieu.parentId].nom}`;
      }
      return lieu.nom;
    },
    defaultNew(type) {
      return { id: uid(), type, nom: '', description: '', image: '',
               couleur: '#b0afa9', tags: [], parentId: null, createdAt: Date.now() };
    },
  };

  // ── CHAPITRES ─────────────────────────────────────────────────
  // Structure : { id, numero, titre, acte, dateFiction, description }
  const Chapitres = {
    all()   { return load(KEYS.chapitres); },
    get(id) { return load(KEYS.chapitres)[id]; },
    save(chap) {
      const data = load(KEYS.chapitres);
      if (!chap.id) chap.id = uid();
      data[chap.id] = chap;
      save(KEYS.chapitres, data);
      return chap;
    },
    delete(id) {
      const data = load(KEYS.chapitres);
      delete data[id];
      save(KEYS.chapitres, data);
    },
    list() {
      return Object.values(load(KEYS.chapitres))
        .sort((a, b) => (a.numero || 0) - (b.numero || 0));
    },
    defaultNew() {
      const existing = Object.values(load(KEYS.chapitres));
      const maxNum = existing.reduce((m, c) => Math.max(m, c.numero || 0), 0);
      return { id: uid(), numero: maxNum + 1, titre: '', acte: '',
               dateFiction: '', description: '', createdAt: Date.now() };
    },
  };

  // ── SCÈNES ────────────────────────────────────────────────────
  // Structure : { id, titre, chapitreId, lieuId, personnageIds:[],
  //               dateFiction, heureFiction, note, ordre }
  const Scenes = {
    all()   { return load(KEYS.scenes); },
    get(id) { return load(KEYS.scenes)[id]; },
    save(scene) {
      const data = load(KEYS.scenes);
      if (!scene.id) scene.id = uid();
      scene.updatedAt = Date.now();
      data[scene.id] = scene;
      save(KEYS.scenes, data);
      return scene;
    },
    delete(id) {
      const data = load(KEYS.scenes);
      delete data[id];
      save(KEYS.scenes, data);
    },
    // Toutes les scènes d'un chapitre, triées par ordre
    byChap(chapId) {
      return Object.values(load(KEYS.scenes))
        .filter(s => s.chapitreId === chapId)
        .sort((a, b) => (a.ordre || 0) - (b.ordre || 0));
    },
    // Toutes les scènes d'un personnage
    byChar(charId) {
      return Object.values(load(KEYS.scenes))
        .filter(s => (s.personnageIds || []).includes(charId))
        .sort((a, b) => {
          const ca = Chapitres.get(a.chapitreId), cb = Chapitres.get(b.chapitreId);
          return ((ca?.numero || 0) - (cb?.numero || 0)) || ((a.ordre || 0) - (b.ordre || 0));
        });
    },
    // Toutes les scènes dans un lieu
    byLieu(lieuId) {
      return Object.values(load(KEYS.scenes))
        .filter(s => s.lieuId === lieuId);
    },
    // Pour la timeline : où est chaque perso à chaque chapitre
    // Retourne { chapId: { charId: lieuId } }
    matrice() {
      const scenes = Object.values(load(KEYS.scenes));
      const matrix = {};
      scenes.forEach(s => {
        if (!s.chapitreId || !s.lieuId) return;
        if (!matrix[s.chapitreId]) matrix[s.chapitreId] = {};
        (s.personnageIds || []).forEach(cid => {
          matrix[s.chapitreId][cid] = s.lieuId;
        });
      });
      return matrix;
    },
    // Position d'un vaisseau à un chapitre donné (dernière scène connue)
    positionVaisseau(vaisseauId, chapitreId) {
      const chaps = Chapitres.list();
      const chapIdx = chaps.findIndex(c => c.id === chapitreId);
      // cherche en remontant
      for (let i = chapIdx; i >= 0; i--) {
        const scenes = Scenes.byChap(chaps[i].id);
        const found = scenes.find(s => s.lieuId === vaisseauId);
        if (found) return found.lieuId;
        // ou amarré quelque part
        const amarrage = scenes.find(s => s.vaisseauAmarreId === vaisseauId);
        if (amarrage) return amarrage.lieuId;
      }
      return null;
    },
    defaultNew(chapitreId) {
      const existing = Object.values(load(KEYS.scenes)).filter(s => s.chapitreId === chapitreId);
      const maxOrdre = existing.reduce((m, s) => Math.max(m, s.ordre || 0), 0);
      return { id: uid(), titre: '', chapitreId: chapitreId || null,
               lieuId: null, personnageIds: [], dateFiction: '',
               heureFiction: '', note: '', ordre: maxOrdre + 1, createdAt: Date.now() };
    },
  };

  // ── IMPORT / EXPORT GLOBAL ────────────────────────────────────
  const IO = {
    exportAll() {
      return {
        version: 3,
        exportedAt: new Date().toISOString(),
        chars:     load(KEYS.chars),
        lieux:     load(KEYS.lieux),
        scenes:    load(KEYS.scenes),
        chapitres: load(KEYS.chapitres),
      };
    },
    importAll(data) {
      if (data.chars)     save(KEYS.chars,     data.chars);
      if (data.lieux)     save(KEYS.lieux,     data.lieux);
      if (data.scenes)    save(KEYS.scenes,    data.scenes);
      if (data.chapitres) save(KEYS.chapitres, data.chapitres);
    },
    downloadJSON(obj, filename) {
      const b = new Blob([JSON.stringify(obj, null, 2)], { type: 'application/json' });
      const a = document.createElement('a');
      a.href = URL.createObjectURL(b); a.download = filename; a.click();
    },
  };

  // ── COULEURS PAR LIEU (pour timeline) ─────────────────────────
  const PALETTE = [
    '#5B8DB8','#7BAF7B','#C17D6F','#9B7BB5','#C4A35A',
    '#6AABAA','#B5727C','#7A9E7E','#8C6E9E','#B08050',
  ];
  let _colorCache = {};
  function lieuColor(lieuId) {
    if (!lieuId) return '#d0cfc9';
    if (_colorCache[lieuId]) return _colorCache[lieuId];
    const idx = Object.keys(_colorCache).length % PALETTE.length;
    _colorCache[lieuId] = PALETTE[idx];
    return _colorCache[lieuId];
  }
  // Réinitialise le cache (appeler si on recharge les lieux)
  function resetColorCache() { _colorCache = {}; }

  // API publique
  return { uid, Chars, Lieux, Chapitres, Scenes, IO, lieuColor, resetColorCache };

})();
