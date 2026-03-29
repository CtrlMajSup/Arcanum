# 🖋️ Arcanum : Architecture Technique

Bienvenue dans le cœur de **Arcanum**. Ce projet utilise une **Architecture Propre (Clean Architecture)** adaptée au C++20 et Qt6. L'objectif est de séparer strictement la logique métier (ton univers, tes personnages) des détails techniques (la base de données SQLite, l'interface graphique).

---

## 🏗️ Structure des Dossiers

### 1. `src/core/` (Noyau Technique)
C'est le socle technique du projet. On y trouve les outils transversaux qui ne concernent pas le métier mais le fonctionnement de l'application.
* **Result.h** : Gestion des erreurs sans exceptions (`Result<T, E>`).
* **Logger.h** : Système de log centralisé.
* **Assert.h** : Macros de validation pour le mode debug.

### 2. `src/domain/` (Cœur Métier - Pur C++)
**C'est la partie la plus importante.** Elle ne doit dépendre de rien (ni Qt, ni SQLite). Si on change de framework UI, ce dossier ne bouge pas, ça c'est plus pour me chalenger
* **entities/** : Les objets principaux (Personnage, Lieu, Scène).
* **valueobjects/** : Des types simples mais typés (ID unique, format de date fictive, c'est surtout pour la logique des nouvveaux format de date en fait).
* **events/** : Système de notifications internes quand une donnée change.

### 3. `src/repositories/` (Interfaces de Données)
Ce dossier contient uniquement des **classes abstraites** (interfaces). Elles définissent *ce que l'on peut faire* avec les données (ex: `ICharacterRepository::save()`), sans dire *comment* (SQL, JSON, Cloud).

### 4. `src/infrastructure/` (Détails Techniques)
C'est ici que l'on implémente les interfaces du dessus.
* **database/** : Gestion de la connexion SQLite et des migrations (mise à jour du schéma de la base).
* **repositories/** : Code SQL concret pour transformer des lignes de base de données en objets C++.

### 5. `src/services/` (Logique de Contrôle)
Les services orchestrent le travail. Ils font le pont entre les dépôts (repositories) et l'UI. 
* *Exemple* : `CharacterService` vérifie si un nom de personnage est unique avant de demander au repository de l'enregistrer.

### 6. `src/ui/` (Interface Utilisateur - Qt6)
La couche de présentation.
* **viewmodels/** : Adaptent les données brutes du domaine pour les rendre affichables par Qt (gestion des `QString`, signaux et slots).
* **widgets/** : Composants graphiques personnalisés (Timeline peinte à la main, éditeurs).

---

## 🛠️ Flux de Données (Data Flow)

Pour maintenir un code propre, nous suivons une direction unique :

1.  **L'utilisateur** clique sur "Enregistrer" dans un `Widget`.
2.  Le `Widget` appelle une méthode du `ViewModel`.
3.  Le `ViewModel` invoque un `Service`.
4.  Le `Service` valide la logique et appelle le `Repository` (Interface).
5.  L'implémentation dans `Infrastructure` exécute la requête **SQL**.

---

## 🚀 Principes Clés

* **Pas d'Exceptions** : On utilise le type `Result` pour forcer la gestion des erreurs proprement.
* **Zéro Logique dans l'UI** : Un widget ne doit jamais savoir qu'une base de données existe. Il ne connaît que son ViewModel.
* **Injection de Dépendances** : Les objets sont créés dans `App.cpp` et passés aux autres par référence/pointeur pour faciliter les tests unitaires.

---
