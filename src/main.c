/* main.c - Programme de test pour common.c */

#include "common.h"

int main() {
    Carte paquet[NB_CARTES_TOTAL];
    Joueur joueurs[2];
    Serie series[NB_SERIES];
    int index_paquet = 0;

    // Initialisation
    init_paquet(paquet);
    melanger_paquet(paquet, NB_CARTES_TOTAL);
    distribuer_cartes(paquet, joueurs, 2);
    index_paquet = 2 * MAX_MAIN;
    initialiser_series(paquet, &index_paquet, series);

    // Affichage des mains
    printf("\n--- Mains des joueurs ---\n");
    for (int i = 0; i < 2; i++) {
        printf("Joueur %d : ", i);
        for (int j = 0; j < MAX_MAIN; j++) {
            printf("%d ", joueurs[i].main[j].valeur);
        }
        printf("\n");
    }

    // Affichage des séries
    printf("\n--- Séries de départ ---\n");
    for (int i = 0; i < NB_SERIES; i++) {
        printf("Ligne %d : %d\n", i, series[i].cartes[0].valeur);
    }

    // Simulation du placement d'une carte
    Carte test = joueurs[0].main[0];
    printf("\nCarte à placer : %d\n", test.valeur);

    int ligne = trouver_ligne_placement(test, series);
    if (ligne == -1) {
        printf("Aucune ligne possible. Le joueur doit ramasser une ligne.\n");
        ligne = choisir_ligne_minimale(series);
        printf("Ligne avec le moins de têtes de boeuf : %d\n", ligne);
    } else {
        printf("Carte placée dans la ligne %d\n", ligne);
    }

    Carte ramassees[MAX_SERIE];
    int nb_ramassees = 0;
    int ramassage = placer_carte(test, &series[ligne], ramassees, &nb_ramassees);

    if (ramassage) {
        printf("Le joueur a ramassé %d cartes : ", nb_ramassees);
        for (int i = 0; i < nb_ramassees; i++) {
            printf("%d ", ramassees[i].valeur);
        }
        printf("\n");
    }

    // Affichage final de la ligne modifiée
    printf("\nContenu de la ligne %d : ", ligne);
    for (int j = 0; j < series[ligne].taille; j++) {
        printf("%d ", series[ligne].cartes[j].valeur);
    }
    printf("\n\n");

    return 0;
}
