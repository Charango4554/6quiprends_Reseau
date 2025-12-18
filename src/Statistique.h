#ifndef STATISTIQUE_H
#define STATISTIQUE_H

#include "common.h"
#include <stdio.h>

/*
 * Ouvre le fichier de logs CSV.
 * Retourne 1 si OK, 0 si erreur.
 */
int Stat_open_log(const char *path);

/*
 * Ferme le fichier de logs.
 */
void Stat_close_log();

/*
 * Enregistre une action dans logs.csv
 * Format :
 *  partie;tour;joueur;carte;ligne;ramasse;tetes;score
 */
void Stat_log_action(
    int partie,
    int tour,
    int joueur,
    Carte carte,
    int ligne,
    int ramasse,
    int tetes,
    int score
);

#endif
