/* common.c */

#include "common.h"

/* Initialise le paquet de 104 cartes avec leurs valeurs et têtes de boeuf */
void init_paquet(Carte *paquet) {
    for (int i = 0; i < NB_CARTES_TOTAL; i++) {
        int val = i + 1;
        int tb;
        if (val == 55) tb = 7;
        else if (val % 11 == 0) tb = 5;
        else if (val % 10 == 0) tb = 3;
        else if (val % 5 == 0) tb = 2;
        else tb = 1;

        paquet[i].valeur = val;
        paquet[i].tetes_boeuf = tb;
    }
}

/* Mélange un paquet de cartes (Fisher-Yates) */
void melanger_paquet(Carte *paquet, int taille) {
    srand(time(NULL)); //Initialise rand() avec une seed basé sur time
    for (int i = taille - 1; i > 0; i--) { //Du dernier index on monte jusqu'à 1
        int j = rand() % (i + 1); //Génére un nombre aléatoire basé sur srand et s'assure que j est dans l'interval [Ø,i]
        Carte temp = paquet[i];
        paquet[i] = paquet[j];
        paquet[j] = temp;
    }
}

/* Calcule le nombre de têtes de boeuf pour une valeur donnée */
int get_tetes_boeuf(int valeur) {
    if (valeur == 55) return 7;
    int tb = 1;
    if (valeur % 11 == 0) tb = 5;
    else if (valeur % 10 == 0) tb = 3;
    else if (valeur % 5 == 0) tb = 2;
    return tb;
}

/* Distribue 10 cartes à chaque joueur */
void distribuer_cartes(Carte *paquet, Joueur *joueurs, int nb_joueurs) {
    for (int i = 0; i < nb_joueurs; i++) {
        for (int j = 0; j < MAX_MAIN; j++) {
            joueurs[i].main[j] = paquet[i * MAX_MAIN + j];
        }
        joueurs[i].nb_cartes_main = MAX_MAIN;
        joueurs[i].nb_penalites = 0;
        joueurs[i].score = 0;
    }
}

/* Initialise les 4 séries de départ avec les 4 cartes suivantes du paquet */
void initialiser_series(Carte *paquet, int *index_paquet, Serie series[NB_SERIES]) {
    for (int i = 0; i < NB_SERIES; i++) {
        series[i].cartes[0] = paquet[*index_paquet];// Place dans la série i la carte du paquet située à l’index actuel
        series[i].taille = 1;
        (*index_paquet)++;// Avance l’index de paquet pour utiliser la carte suivante lors du prochain tour
    }
}

/* Trouve la ligne où placer une carte (retourne -1 si aucune ne convient) */
int trouver_ligne_placement(Carte carte, Serie series[NB_SERIES]) {
    int min_diff = 999;
    int ligne_choisie = -1;

    for (int i = 0; i < NB_SERIES; i++) {
        int fin = series[i].cartes[series[i].taille - 1].valeur; // Récupère la valeur de la dernière carte présente dans cette série
        if (carte.valeur > fin) { //Si carte plus grande
            int diff = carte.valeur - fin; // Calcule la différence entre la carte et la dernière carte de la série
            if (diff < min_diff) {
                min_diff = diff;
                ligne_choisie = i;
            }
        }
    }
    return ligne_choisie;
}

/* Choisit la ligne avec le moins de têtes de boeuf (en cas de ramassage) */
int choisir_ligne_minimale(Serie series[NB_SERIES]) {
    int min_tetes = 999;
    int index = 0;
    for (int i = 0; i < NB_SERIES; i++) {
        int total = 0;
        for (int j = 0; j < series[i].taille; j++) {
            total += series[i].cartes[j].tetes_boeuf;
        }
        if (total < min_tetes) {
            min_tetes = total;
            index = i;
        }
    }
    return index;
}

/* Place une carte dans une série (ou ramasse si 6e carte) */
int placer_carte(Carte carte, Serie *serie, Carte *ramassees, int *nb_ramassees) {
    if (serie->taille == MAX_SERIE) {
        for (int i = 0; i < MAX_SERIE; i++) {
            ramassees[i] = serie->cartes[i];
        }
        *nb_ramassees = MAX_SERIE;
        serie->cartes[0] = carte;
        serie->taille = 1;
        return 1; // a ramassé
    } else {
        serie->cartes[serie->taille++] = carte;
        *nb_ramassees = 0;
        return 0; // pas de ramassage
    }
}

/* Calcule le score total à partir d'une pile de cartes */
int calculer_score(const Carte *cartes, int taille) {
    int score = 0;
    for (int i = 0; i < taille; i++) {
        score += cartes[i].tetes_boeuf;
    }
    return score;
}

/* Écrit une action dans le fichier log */
void log_action(FILE *f, int tour, int joueur_id, Carte carte, int ligne, int score) {
    fprintf(f, "%d;%d;%d;%d;%d\n", tour, joueur_id, carte.valeur, ligne, score);
    fflush(f);
}
