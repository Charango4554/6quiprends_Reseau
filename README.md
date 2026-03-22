# 6 qui prend - Projet Systèmes & Réseaux

Implémentation en C du jeu **6 qui prend** avec architecture **client/serveur TCP**.
Le dépôt contient :
- un serveur de partie (`manager`),
- un client humain terminal (`player_human`),
- un client bot (`player_bot`),
- un pipeline de statistiques (logs CSV + scripts AWK + génération PDF).

## Sommaire
- [Fonctionnalités](#fonctionnalités)
- [Structure du dépôt](#structure-du-dépôt)
- [Prérequis](#prérequis)
- [Compilation](#compilation)
- [Lancement rapide](#lancement-rapide)
- [Règles implémentées](#règles-implémentées)
- [Protocole réseau](#protocole-réseau)
- [Authentification](#authentification)
- [Statistiques](#statistiques)
- [Nettoyage](#nettoyage)
- [Limites connues](#limites-connues)
- [Auteurs](#auteurs)

## Fonctionnalités
- Partie multi-joueurs (jusqu'à `MAX_JOUEURS=10`).
- Joueurs humains et bots dans la même partie.
- Gestion des manches jusqu'à atteinte de `SCORE_LIMITE`.
- Tri des cartes jouées par ordre croissant à chaque tour.
- Journalisation complète des actions dans `stats/logs.csv`.
- Mise à jour des profils joueurs via AWK.
- Génération de rapports PDF de partie.

## Structure du dépôt
```text
.
├── Makefile
├── src/
│   ├── manager.c
│   ├── player_human.c
│   ├── player_bot.c
│   ├── common.c
│   ├── common.h
│   ├── Statistique.c
│   └── Statistique.h
├── build/                 # Objets .o
├── bin/                   # Exécutables compilés
└── stats/
    ├── logs.csv
    ├── USERS/
    ├── PARTIES/
    ├── stats_joueur.awk
    ├── stats_globales.awk
    └── ranking.awk
```

## Prérequis
- `gcc`
- `make`
- `awk`
- `pdflatex` (optionnel, seulement pour générer les PDF)

## Compilation
Depuis la racine du dépôt :

```bash
make
```

Exécutables générés :
- `bin/manager`
- `bin/player_human`
- `bin/player_bot`

## Lancement rapide
### 1) Démarrer le serveur
Dans un premier terminal :

```bash
./bin/manager <nb_joueurs>
```

Exemple :

```bash
./bin/manager 2
```

Si aucun argument n'est fourni, le serveur démarre avec 2 joueurs.

### 2) Lancer les clients
Dans d'autres terminaux (autant que nécessaire) :

```bash
./bin/player_human
```

ou

```bash
./bin/player_bot
```

### 3) Connexion réseau
Par défaut, les clients se connectent à :
- IP : `127.0.0.1`
- Port : `12345`

Pour jouer entre plusieurs machines, adapter `SERVER_IP` dans :
- `src/player_human.c`
- `src/player_bot.c`

puis recompiler (`make`).

## Règles implémentées
- Paquet de 104 cartes.
- 10 cartes distribuées par joueur, 4 séries initiales.
- À chaque tour, tous les joueurs choisissent une carte.
- Le serveur résout les coups par ordre croissant des cartes.
- Placement :
  - carte posée sur la ligne valide la plus proche (écart minimal),
  - si carte plus petite que toutes les fins de lignes, le joueur choisit la ligne à ramasser,
  - si une ligne atteint la 6e carte, le joueur ramasse la ligne.
- La partie se termine lorsqu'un joueur atteint `SCORE_LIMITE` (valeur actuelle : **40** dans `src/common.h`).
- Classement final trié par score croissant (moins de têtes = mieux).

## Protocole réseau
Le protocole est basé sur des `read/write` bloquants avec transmission robuste via helpers :
- `write_all()` côté serveur,
- `recv_all()` côté clients.

Séquence simplifiée :
1. Connexion client.
2. Envoi d'un login sur 32 octets.
3. Début de manche : envoi indicateur, main, séries initiales.
4. Pour chaque tour :
   - client -> serveur : carte jouée,
   - serveur -> client : séries courantes,
   - client -> serveur : ligne choisie,
   - serveur -> client : séries mises à jour + score.
5. Fin de partie : envoi scores + noms + classement.

## Authentification
Côté `player_human` :
- login existant : vérification mot de passe (`stats/USERS/<login>.txt`),
- login inexistant : création de compte possible,
- mode `GUEST` : pas de profil persistant.

Format minimal d'un profil :

```text
PASSWORD=1234
```

Les statistiques sont ensuite ajoutées/actualisées dans le même fichier.

## Statistiques
### Logs bruts
Le serveur enregistre chaque action dans `stats/logs.csv` au format :

```text
partie;tour;joueur;carte;ligne;ramasse;tetes;score
```

### Mise à jour profil joueur
À la fin d'une partie, `player_human` lance :

```bash
awk -v joueur=<id> -v login=<username> -f ../stats/stats_joueur.awk ../stats/logs.csv
```

### Classements/statistiques globales
Scripts disponibles :
- `stats/stats_globales.awk`
- `stats/ranking.awk`

### Génération PDF d'une partie
Script : `stats/PARTIES/gen_pdf.sh`

```bash
cd stats/PARTIES
./gen_pdf.sh <id_partie> [sortie.pdf]
```

Le script filtre les logs de la partie, calcule les stats via AWK puis compile un rapport LaTeX en PDF.

## Nettoyage
```bash
make clean
```
