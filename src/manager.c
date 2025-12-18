/*
 * server.c
 * ----------
 * Serveur TCP pour le jeu "6 qui prend".
 * - Gère les connexions des joueurs
 * - Distribue les cartes
 * - Orchestre les manches et les tours
 * - Applique les règles de placement / ramassage
 * - Calcule les scores
 * - Enregistre des statistiques
 */

#include "common.h"
#include "Statistique.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define PORT 12345
#define MAX_CONNEXIONS 10

/*
 * write_all()
 * -----------
 * Garantit l’envoi complet d’un buffer sur une socket TCP.
 * Évite les écritures partielles dues au comportement de write().
 */
static int write_all(int fd, const void *buf, size_t len) {
    size_t total = 0;
    const char *ptr = buf;
    while (total < len) {
        ssize_t n = write(fd, ptr + total, len - total);
        if (n <= 0) {
            return -1;
        }
        total += (size_t)n;
    }
    return 0;
}

int main(int argc, char *argv[]) {

    // Descripteurs de sockets
    int server_fd, client_fds[MAX_JOUEURS];

    // Structures d’adresses réseau
    struct sockaddr_in server_addr, client_addr;
    socklen_t addr_len = sizeof(struct sockaddr_in);

    // Noms des joueurs (envoyés par les clients à la connexion)
    char noms_joueurs[MAX_JOUEURS][32];

    /* ======================
     * INITIALISATION SERVEUR
     * ====================== */

    // Création de la socket serveur
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    // Préparation de l’adresse du serveur
    bzero((char *)&server_addr, sizeof(server_addr));
    server_addr.sin_family      = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port        = htons(PORT);

    // Association socket <-> port
    if (bind(server_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("bind");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    // Mise en écoute
    if (listen(server_fd, MAX_CONNEXIONS) < 0) {
        perror("listen");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    /* ======================
     * CONFIGURATION PARTIE
     * ====================== */

    // Nombre de joueurs (par défaut 2, sinon via argv)
    int nb_joueurs = 2;
    if (argc > 1) {
        nb_joueurs = atoi(argv[1]);
        if (nb_joueurs < 1 || nb_joueurs > MAX_JOUEURS) {
            fprintf(stderr, "Erreur: nb joueurs invalide.\n");
            exit(EXIT_FAILURE);
        }
    }

    // Structures joueurs et scores cumulés
    Joueur joueurs[nb_joueurs];
    int scores_cumules[MAX_JOUEURS] = {0};

    printf("[Serveur] En attente de %d connexions sur le port %d...\n", nb_joueurs, PORT);

    // Ouverture du fichier de statistiques
    if (!Stat_open_log("../stats/logs.csv")) {
        fprintf(stderr, "[Serveur] Impossible d'ouvrir ../stats/logs.csv.\n");
    }

    // Identifiant unique de la partie
    const int partie_id = (int)time(NULL);

    bzero((char *)&client_addr, sizeof(client_addr));
    bzero(noms_joueurs, sizeof(noms_joueurs));

    /* ======================
     * CONNEXION DES JOUEURS
     * ====================== */

    for (int i = 0; i < nb_joueurs; i++) {

        // Acceptation d’un client
        int client_fd = accept(server_fd, (struct sockaddr*)&client_addr, &addr_len);
        if (client_fd < 0) {
            perror("accept");
            exit(EXIT_FAILURE);
        }
        client_fds[i] = client_fd;

        // Réception du nom du joueur
        read(client_fds[i], noms_joueurs[i], 32);
        printf("[Serveur] Joueur %d identifié comme '%s'\n", i, noms_joueurs[i]);

        // Initialisation du joueur
        joueurs[i].id             = i;
        joueurs[i].nb_penalites   = 0;
        joueurs[i].nb_cartes_main = 0;
        joueurs[i].score          = 0;

        printf("[Serveur] Joueur %d connecté.\n", i);
    }

    /* ======================
     * BOUCLE DE PARTIE
     * ====================== */

    int partie_terminee = 0;
    Serie series[NB_SERIES];

    while (!partie_terminee) {

        // Création et mélange du paquet
        Carte paquet[NB_CARTES_TOTAL];
        init_paquet(paquet);
        melanger_paquet(paquet, NB_CARTES_TOTAL);

        // Distribution des cartes
        distribuer_cartes(paquet, joueurs, nb_joueurs);

        // Restauration des scores cumulés
        for (int i = 0; i < nb_joueurs; i++) {
            joueurs[i].score = scores_cumules[i];
        }

        // Signal "nouvelle manche"
        int nouvelle_manche = 1;
        for (int i = 0; i < nb_joueurs; i++)
            write_all(client_fds[i], &nouvelle_manche, sizeof(int));

        // Envoi des mains
        for (int i = 0; i < nb_joueurs; i++)
            write_all(client_fds[i], joueurs[i].main, sizeof(Carte) * MAX_MAIN);

        // Initialisation des séries
        int index_paquet = nb_joueurs * MAX_MAIN;
        initialiser_series(paquet, &index_paquet, series);

        for (int i = 0; i < nb_joueurs; i++)
            write_all(client_fds[i], series, sizeof(Serie) * NB_SERIES);

        /* ======================
         * TOURS DE JEU
         * ====================== */

        for (int tour = 0; tour < MAX_MAIN; tour++) {

            Carte cartes_jouees[MAX_JOUEURS];
            int ordre[MAX_JOUEURS];

            // Réception des cartes jouées
            for (int i = 0; i < nb_joueurs; i++) {
                read(client_fds[i], &cartes_jouees[i], sizeof(Carte));
                ordre[i] = i;
                printf("[Tour %d] Joueur %d a joué %d\n",
                       tour + 1, i, cartes_jouees[i].valeur);
            }

            // Tri des joueurs par valeur de carte croissante
            for (int i = 0; i < nb_joueurs - 1; i++)
                for (int j = i + 1; j < nb_joueurs; j++)
                    if (cartes_jouees[ordre[i]].valeur >
                        cartes_jouees[ordre[j]].valeur) {
                        int tmp = ordre[i];
                        ordre[i] = ordre[j];
                        ordre[j] = tmp;
                    }

            /* ======================
             * PLACEMENT DES CARTES
             * ====================== */

            for (int k = 0; k < nb_joueurs; k++) {

                int idx = ordre[k];
                Carte carte = cartes_jouees[idx];

                // Envoi des séries au joueur
                write_all(client_fds[idx], series, sizeof(Serie) * NB_SERIES);

                // Réception de la ligne choisie
                int ligne_demande;
                read(client_fds[idx], &ligne_demande, sizeof(int));

                int derniere = series[ligne_demande]
                                   .cartes[series[ligne_demande].taille - 1]
                                   .valeur;

                int ramasse = 0;
                int tetes_ramassees = 0;
                int etait_pleine = (series[ligne_demande].taille == MAX_SERIE);
                int plus_petite = (carte.valeur <= derniere);

                // Règles de placement / ramassage
                if (!plus_petite && !etait_pleine) {
                    series[ligne_demande]
                        .cartes[series[ligne_demande].taille++] = carte;
                } else {
                    ramasse = 1;

                    for (int r = 0; r < series[ligne_demande].taille; r++) {
                        Carte prise = series[ligne_demande].cartes[r];
                        joueurs[idx].penalites[joueurs[idx].nb_penalites++] = prise;
                        tetes_ramassees += prise.tetes_boeuf;
                    }

                    series[ligne_demande].cartes[0] = carte;
                    series[ligne_demande].taille = 1;
                }

                // Mise à jour score
                if (ramasse)
                    scores_cumules[idx] += tetes_ramassees;

                joueurs[idx].score = scores_cumules[idx];

                // Log statistique
                Stat_log_action(
                    partie_id,
                    tour + 1,
                    idx,
                    carte,
                    ligne_demande,
                    ramasse,
                    tetes_ramassees,
                    joueurs[idx].score
                );

                // Envoi des séries et du score
                write_all(client_fds[idx], series, sizeof(Serie) * NB_SERIES);
                write_all(client_fds[idx], &joueurs[idx].score, sizeof(int));
            }
        }

        // Vérification fin de partie
        int score_max = 0;
        for (int i = 0; i < nb_joueurs; i++)
            if (scores_cumules[i] > score_max)
                score_max = scores_cumules[i];

        if (score_max >= SCORE_LIMITE)
            partie_terminee = 1;
    }

    /* ======================
     * FIN DE PARTIE
     * ====================== */

    int fin_manche = 0;
    for (int i = 0; i < nb_joueurs; i++)
        write_all(client_fds[i], &fin_manche, sizeof(int));

    // Calcul du classement final
    int classement[MAX_JOUEURS];
    int scores[MAX_JOUEURS];

    for (int i = 0; i < nb_joueurs; i++) {
        classement[i] = i;
        scores[i] = joueurs[i].score;
    }

    for (int i = 0; i < nb_joueurs - 1; i++)
        for (int j = i + 1; j < nb_joueurs; j++)
            if (scores[classement[i]] > scores[classement[j]]) {
                int tmp = classement[i];
                classement[i] = classement[j];
                classement[j] = tmp;
            }

    // Envoi résultats aux clients
    for (int i = 0; i < nb_joueurs; i++)
        write_all(client_fds[i], &nb_joueurs, sizeof(int));

    for (int i = 0; i < nb_joueurs; i++)
        write_all(client_fds[i], scores, sizeof(int) * nb_joueurs);

    for (int i = 0; i < nb_joueurs; i++)
        write_all(client_fds[i], noms_joueurs, sizeof(noms_joueurs));

    for (int i = 0; i < nb_joueurs; i++)
        write_all(client_fds[i], classement, sizeof(int) * nb_joueurs);

    // Fermeture des sockets
    for (int i = 0; i < nb_joueurs; i++)
        close(client_fds[i]);

    Stat_close_log();
    close(server_fd);
    return 0;
}