'use strict';
// ═══════════════════════════════════════════════════════════════
// MAIN.JS — Processus principal Electron
// Gère : fenêtre, menus natifs, IPC, autosave, SQLite via db.js
// ═══════════════════════════════════════════════════════════════

const { app, BrowserWindow, ipcMain, dialog, Menu, shell } = require('electron');
const path = require('path');
const fs   = require('fs');
const db   = require('./db');

const isDev = process.argv.includes('--dev');

let mainWindow = null;
let autosaveTimer = null;
const AUTOSAVE_INTERVAL = 2 * 60 * 1000; // 2 minutes

// ── FENÊTRE ───────────────────────────────────────────────────
function createWindow() {
  mainWindow = new BrowserWindow({
    width:  1280,
    height: 800,
    minWidth:  900,
    minHeight: 600,
    title: 'Arcanum',
    backgroundColor: '#f7f6f3',
    webPreferences: {
      preload: path.join(__dirname, 'preload.js'),
      contextIsolation: true,
      nodeIntegration:  false,
      sandbox: false,
    },
  });

  mainWindow.loadFile(path.join(__dirname, 'renderer', 'index.html'));
  if (isDev) mainWindow.webContents.openDevTools();

  mainWindow.on('close', (e) => {
    // Sauvegarde automatique à la fermeture si un projet est ouvert
    if (db.isOpen()) {
      db.AutoSave.save();
    }
  });
}

// ── AUTOSAVE ──────────────────────────────────────────────────
function startAutosave() {
  stopAutosave();
  autosaveTimer = setInterval(() => {
    if (db.isOpen()) {
      db.AutoSave.save();
      mainWindow?.webContents.send('app:autosaved', new Date().toLocaleTimeString('fr-FR'));
    }
  }, AUTOSAVE_INTERVAL);
}

function stopAutosave() {
  if (autosaveTimer) { clearInterval(autosaveTimer); autosaveTimer = null; }
}

// ── TITRE FENÊTRE ─────────────────────────────────────────────
function updateTitle() {
  const filePath = db.getPath();
  const name = filePath ? path.basename(filePath, '.arcanum') : 'Sans titre';
  mainWindow?.setTitle(`Arcanum — ${name}`);
  mainWindow?.webContents.send('app:titleUpdate', { name, filePath });
}

// ── MENUS ─────────────────────────────────────────────────────
function buildMenu() {
  const template = [
    {
      label: 'Fichier',
      submenu: [
        {
          label: 'Nouveau projet',
          accelerator: 'CmdOrCtrl+N',
          click: () => handleNewProject(),
        },
        {
          label: 'Ouvrir…',
          accelerator: 'CmdOrCtrl+O',
          click: () => handleOpenProject(),
        },
        { type: 'separator' },
        {
          label: 'Enregistrer',
          accelerator: 'CmdOrCtrl+S',
          click: () => handleSave(),
        },
        {
          label: 'Enregistrer sous…',
          accelerator: 'CmdOrCtrl+Shift+S',
          click: () => handleSaveAs(),
        },
        { type: 'separator' },
        {
          label: 'Exporter en JSON…',
          click: () => handleExportJSON(),
        },
        {
          label: 'Importer depuis JSON…',
          click: () => handleImportJSON(),
        },
        { type: 'separator' },
        {
          label: 'Historique de sauvegardes',
          click: () => mainWindow?.webContents.send('app:showAutosaves'),
        },
        { type: 'separator' },
        { role: 'quit', label: 'Quitter' },
      ],
    },
    {
      label: 'Édition',
      submenu: [
        { role: 'undo',      label: 'Annuler' },
        { role: 'redo',      label: 'Rétablir' },
        { type: 'separator' },
        { role: 'cut',       label: 'Couper' },
        { role: 'copy',      label: 'Copier' },
        { role: 'paste',     label: 'Coller' },
        { role: 'selectAll', label: 'Tout sélectionner' },
      ],
    },
    {
      label: 'Affichage',
      submenu: [
        { role: 'reload',          label: 'Recharger' },
        { role: 'forceReload',     label: 'Forcer le rechargement' },
        { type: 'separator' },
        { role: 'resetZoom',       label: 'Zoom normal' },
        { role: 'zoomIn',          label: 'Zoom +' },
        { role: 'zoomOut',         label: 'Zoom −' },
        { type: 'separator' },
        { role: 'togglefullscreen', label: 'Plein écran' },
        isDev ? { role: 'toggleDevTools', label: 'Outils développeur' } : null,
      ].filter(Boolean),
    },
    {
      label: 'Aide',
      submenu: [
        {
          label: 'À propos d\'Arcanum',
          click: () => {
            dialog.showMessageBox(mainWindow, {
              type: 'info',
              title: 'Arcanum',
              message: 'Arcanum v1.0',
              detail: 'Gestionnaire de roman — personnages, lieux, scènes, timeline.\n\nFichiers .arcanum : base de données SQLite locale, 100% privée.',
            });
          },
        },
      ],
    },
  ];

  Menu.setApplicationMenu(Menu.buildFromTemplate(template));
}

// ── HANDLERS FICHIERS ─────────────────────────────────────────
async function handleNewProject() {
  // Sur Linux, forcer le focus avant d'ouvrir un dialogue natif
  if (mainWindow) { mainWindow.focus(); mainWindow.moveTop(); }

  // Dialogue de confirmation simplifié (évite un double dialogue sur Linux)
  const { filePath } = await dialog.showSaveDialog(mainWindow, {
    title:       'Nouveau projet — choisir l\'emplacement',
    defaultPath: 'MonRoman.arcanum',
    filters:     [{ name: 'Projet Arcanum', extensions: ['arcanum'] }],
  });
  if (!filePath) return;

  stopAutosave();
  db.close();
  if (fs.existsSync(filePath)) fs.unlinkSync(filePath);
  db.open(filePath);
  startAutosave();
  updateTitle();
  mainWindow?.webContents.send('app:newProject');
}

async function handleOpenProject() {
  if (mainWindow) { mainWindow.focus(); mainWindow.moveTop(); }

  const { filePaths } = await dialog.showOpenDialog(mainWindow, {
    title:       'Ouvrir un projet Arcanum',
    filters:     [{ name: 'Projet Arcanum', extensions: ['arcanum'] }],
    properties:  ['openFile'],
  });
  if (!filePaths?.length) return;

  stopAutosave();
  db.close();
  db.open(filePaths[0]);
  startAutosave();
  updateTitle();
  mainWindow?.webContents.send('app:fileOpened', filePaths[0]);
}

async function handleSave() {
  if (!db.isOpen()) return handleSaveAs();
  db.AutoSave.save();
  mainWindow?.webContents.send('app:fileSaved', db.getPath());
}

async function handleSaveAs() {
  const { filePath } = await dialog.showSaveDialog(mainWindow, {
    title:       'Enregistrer sous…',
    defaultPath: 'MonRoman.arcanum',
    filters:     [{ name: 'Projet Arcanum', extensions: ['arcanum'] }],
  });
  if (!filePath) return;

  // Copier le fichier actuel vers la nouvelle destination
  if (db.isOpen() && db.getPath() !== filePath) {
    db.close();
    if (fs.existsSync(db.getPath())) fs.copyFileSync(db.getPath(), filePath);
  }
  db.open(filePath);
  startAutosave();
  updateTitle();
  mainWindow?.webContents.send('app:fileSaved', filePath);
}

async function handleExportJSON() {
  if (!db.isOpen()) { dialog.showErrorBox('Arcanum', 'Aucun projet ouvert.'); return; }
  const { filePath } = await dialog.showSaveDialog(mainWindow, {
    title:       'Exporter en JSON',
    defaultPath: 'arcanum_export.json',
    filters:     [{ name: 'JSON', extensions: ['json'] }],
  });
  if (!filePath) return;
  fs.writeFileSync(filePath, JSON.stringify(db.exportAll(), null, 2), 'utf-8');
  dialog.showMessageBox(mainWindow, { type: 'info', message: 'Export JSON réussi.', detail: filePath });
}

async function handleImportJSON() {
  const { filePaths } = await dialog.showOpenDialog(mainWindow, {
    title:   'Importer depuis JSON',
    filters: [{ name: 'JSON', extensions: ['json'] }],
    properties: ['openFile'],
  });
  if (!filePaths?.length) return;
  if (!db.isOpen()) {
    dialog.showErrorBox('Arcanum', 'Ouvrez ou créez un projet avant d\'importer.');
    return;
  }
  const data = JSON.parse(fs.readFileSync(filePaths[0], 'utf-8'));
  db.importFromJSON(data);
  mainWindow?.webContents.send('app:fileOpened', db.getPath());
  dialog.showMessageBox(mainWindow, { type: 'info', message: 'Import réussi !' });
}

// ── IPC HANDLERS ──────────────────────────────────────────────
function registerIPC() {

  // Fichier
  ipcMain.handle('file:new',       () => handleNewProject());
  ipcMain.handle('file:open',      () => handleOpenProject());
  ipcMain.handle('file:save',      () => handleSave());
  ipcMain.handle('file:saveAs',    () => handleSaveAs());
  ipcMain.handle('file:getPath',   () => db.getPath());
  ipcMain.handle('file:isOpen',    () => db.isOpen());
  ipcMain.handle('file:exportJSON',() => handleExportJSON());
  ipcMain.handle('file:importJSON',() => handleImportJSON());

  // Personnages
  ipcMain.handle('chars:all',    ()     => db.Chars.all());
  ipcMain.handle('chars:get',    (_, id)=> db.Chars.get(id));
  ipcMain.handle('chars:save',   (_, c) => db.Chars.save(c));
  ipcMain.handle('chars:delete', (_, id)=> { db.Chars.delete(id); return true; });

  // Lieux
  ipcMain.handle('lieux:all',      ()     => db.Lieux.all());
  ipcMain.handle('lieux:get',      (_, id)=> db.Lieux.get(id));
  ipcMain.handle('lieux:save',     (_, l) => db.Lieux.save(l));
  ipcMain.handle('lieux:delete',   (_, id)=> { db.Lieux.delete(id); return true; });
  ipcMain.handle('lieux:fullName', (_, id)=> db.Lieux.fullName(id));

  // Chapitres
  ipcMain.handle('chapitres:all',    ()     => db.Chapitres.all());
  ipcMain.handle('chapitres:list',   ()     => db.Chapitres.list());
  ipcMain.handle('chapitres:get',    (_, id)=> db.Chapitres.get(id));
  ipcMain.handle('chapitres:save',   (_, c) => db.Chapitres.save(c));
  ipcMain.handle('chapitres:delete', (_, id)=> { db.Chapitres.delete(id); return true; });

  // Scènes
  ipcMain.handle('scenes:all',     ()        => db.Scenes.all());
  ipcMain.handle('scenes:get',     (_, id)   => db.Scenes.get(id));
  ipcMain.handle('scenes:save',    (_, s)    => db.Scenes.save(s));
  ipcMain.handle('scenes:delete',  (_, id)   => { db.Scenes.delete(id); return true; });
  ipcMain.handle('scenes:byChap',  (_, cid)  => db.Scenes.byChap(cid));
  ipcMain.handle('scenes:byLieu',  (_, lid)  => db.Scenes.byLieu(lid));
  ipcMain.handle('scenes:matrice', ()        => db.Scenes.matrice());

  // Textes
  ipcMain.handle('textes:byScene', (_, sid) => db.Textes.byScene(sid));
  ipcMain.handle('textes:save',    (_, t)   => db.Textes.save(t));
  ipcMain.handle('textes:delete',  (_, id)  => { db.Textes.delete(id); return true; });

  // Cartes
  ipcMain.handle('cartes:all',     ()       => db.Cartes.all());
  ipcMain.handle('cartes:get',     (_, id)  => db.Cartes.get(id));
  ipcMain.handle('cartes:forLieu', (_, lid) => db.Cartes.forLieu(lid));
  ipcMain.handle('cartes:save',    (_, c)   => db.Cartes.save(c));
  ipcMain.handle('cartes:delete',  (_, id)  => { db.Cartes.delete(id); return true; });

  // Autosave
  ipcMain.handle('autosave:list',    ()     => db.AutoSave.list());
  ipcMain.handle('autosave:restore', (_, id)=> db.AutoSave.restore(id));

  // Utilitaires
  ipcMain.handle('util:uid', () => db.uid());
}

// ── APP LIFECYCLE ─────────────────────────────────────────────
app.whenReady().then(() => {
  // Override CSP pour le renderer local
  const { session } = require('electron');
  session.defaultSession.webRequest.onHeadersReceived((details, callback) => {
    callback({
      responseHeaders: {
        ...details.responseHeaders,
        'Content-Security-Policy': [
          "default-src 'self' 'unsafe-inline' 'unsafe-eval' data: blob:; " +
          "font-src 'self' https://fonts.gstatic.com; " +
          "style-src 'self' 'unsafe-inline' https://fonts.googleapis.com; " +
          "img-src 'self' data: blob:;"
        ],
      },
    });
  });

  buildMenu();
  registerIPC();
  createWindow();

  app.on('activate', () => {
    if (BrowserWindow.getAllWindows().length === 0) createWindow();
  });
});

app.on('window-all-closed', () => {
  stopAutosave();
  db.close();
  if (process.platform !== 'darwin') app.quit();
});

// Ouvrir un fichier .arcanum depuis le gestionnaire de fichiers
app.on('open-file', (event, filePath) => {
  event.preventDefault();
  if (filePath.endsWith('.arcanum')) {
    if (mainWindow) {
      db.open(filePath);
      startAutosave();
      updateTitle();
      mainWindow.webContents.send('app:fileOpened', filePath);
    }
  }
});
