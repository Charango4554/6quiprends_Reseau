/* common.h */

#ifndef COMMON_H
#define COMMON_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/* ------------------ CONSTANTES ------------------ */

#define NB_CARTES_TOTAL 104
#define MAX_MAIN 10
#define MAX_SERIE 5
#define NB_SERIES 4
#define MAX_JOUEURS 10
#define SCORE_LIMITE 40

/* ------------------ STRUCTURES ------------------ */

/* Une carte du jeu (valeur + nombre de têtes de bœuf) */
typedef struct {
    int valeur;         /* valeur entre 1 et 104 */
    int tetes_boeuf;    /* nombre de têtes de bœuf */
} Carte;

/* Une série (ligne) contenant entre 1 et 5 cartes */
typedef struct {
    Carte cartes[MAX_SERIE];
    int taille;         /* nombre de cartes présentes */
} Serie;

/* Un joueur (humain ou bot) */
typedef struct {
    int id;                             /* identifiant du joueur */
    Carte main[MAX_MAIN];               /* 10 cartes distribuées */
    int nb_cartes_main;

    Carte penalites[NB_CARTES_TOTAL];   /* cartes ramassées */
    int nb_penalites;

    int score;                          /* total de têtes de bœuf */
} Joueur;

/* ------------------ FONCTIONS ------------------ */

/* Gestion du paquet */
void init_paquet(Carte *paquet);
void melanger_paquet(Carte *paquet, int taille);
int get_tetes_boeuf(int valeur);

/* Distribution */
void distribuer_cartes(Carte *paquet, Joueur *joueurs, int nb_joueurs);
void initialiser_series(Carte *paquet, int *index_paquet, Serie series[NB_SERIES]);

/* Logique du placement */
int trouver_ligne_placement(Carte carte, Serie series[NB_SERIES]);
int choisir_ligne_minimale(Serie series[NB_SERIES]);

/*
 * Place une carte dans une série.
 * - remplit "ramassees" si une série doit être prise
 * - nb_ramassees = nombre de cartes ramassées
 * - retourne 1 si ramassage, 0 sinon
 */
int placer_carte(Carte carte, Serie *serie, Carte *ramassees, int *nb_ramassees);

/* Calcul du score */
int calculer_score(const Carte *cartes, int taille);

/* Log */
void log_action(FILE *f, int tour, int joueur_id, Carte carte, int ligne, int score);

#endif /* COMMON_H */
