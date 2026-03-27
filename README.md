# Arcanum — Gestionnaire de roman

Application desktop locale pour gérer personnages, lieux, scènes, timeline et cartes de votre roman.

## Installation

### Prérequis
- Node.js 18+ : https://nodejs.org
- Git (optionnel)

### Démarrage

```bash
# 1. Aller dans le dossier du projet
cd arcanum-app

# 2. Installer les dépendances (une seule fois)
npm install

# 3. Lancer l'application
npm start
```

### Première utilisation

Au lancement, cliquez **Nouveau projet** et choisissez où enregistrer votre fichier `.arcanum`.

Ce fichier est une base de données SQLite — vous pouvez le copier, sauvegarder sur une clé USB, ou l'envoyer à quelqu'un d'autre.

## Fichiers `.arcanum`

- **Extension** : `.arcanum`
- **Format** : SQLite (lisible avec DB Browser for SQLite)
- **Sauvegarde auto** : toutes les 2 minutes dans le fichier ouvert
- **Historique** : 20 dernières sauvegardes automatiques conservées (Menu Fichier > Historique)

## Raccourcis clavier

| Touche | Action |
|--------|--------|
| `Ctrl+N` | Nouveau projet |
| `Ctrl+O` | Ouvrir un projet |
| `Ctrl+S` | Enregistrer |
| `Ctrl+Shift+S` | Enregistrer sous |
| `/` | Recherche globale |

### Éditeur de cartes
| Touche | Outil |
|--------|-------|
| `V` | Sélection |
| `H` | Déplacer la vue |
| `P` | Tracé libre |
| `L` | Ligne |
| `R` | Rectangle |
| `E` | Ellipse |
| `G` | Polygone (double-clic pour fermer) |
| `T` | Texte |
| `M` | Marqueur de lieu |
| `Suppr` | Supprimer la sélection |
| `Échap` | Annuler l'outil en cours |
| `+` / `-` | Zoom |
| `0` | Vue par défaut |

## Distribution

Pour créer un installeur Linux :

```bash
npm run build:linux
# → dist/Arcanum-1.0.0.AppImage
# → dist/arcanum_1.0.0_amd64.deb
```

## Structure du projet

```
arcanum-app/
  main.js        ← Processus principal Electron
  preload.js     ← Pont IPC sécurisé
  db.js          ← Couche SQLite (better-sqlite3)
  package.json
  renderer/
    arcanum.js   ← API renderer
    arcanum.css  ← Design system partagé
    index.html   ← Hub principal
    personnages.html
    lieux.html
    scenes.html
    timeline.html
    search.html
    carte.html   ← Éditeur de cartes vectoriel
```
