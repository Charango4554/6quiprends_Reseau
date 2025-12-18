#include "Statistique.h"

static FILE *logfile = NULL;

/*
 * Ouvre le fichier logs.csv en mode append.
 * Si le fichier n'existe pas, il est créé.
 */
int Stat_open_log(const char *path) {
    logfile = fopen(path, "a");
    if (!logfile) return 0;

    return 1;
}

/*
 * Ferme proprement le fichier.
 */
void Stat_close_log() {
    if (logfile) {
        fclose(logfile);
        logfile = NULL;
    }
}

/*
 * Ajoute une ligne dans logs.csv
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
) {
    if (!logfile) return; // sécurité en cas de problème d'ouverture

    fprintf(logfile, "%d;%d;%d;%d;%d;%d;%d;%d\n",
            partie,
            tour,
            joueur,
            carte.valeur,
            ligne,
            ramasse,
            tetes,
            score);

    fflush(logfile);
}
