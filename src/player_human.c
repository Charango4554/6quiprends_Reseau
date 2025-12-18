/** player_human.c - Client humain pour jouer au jeu "6 qui prend" en terminal */

#include "common.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <stdlib.h>

#define SERVER_IP "127.0.0.1"
#define SERVER_PORT 12345


/* Lecture robuste d'un bloc réseau : boucle jusqu'à recevoir exactement len octets. */
static int recv_all(int fd, void *buf, size_t len) {
    size_t total = 0;
    char *ptr = buf;
    while (total < len) {
        ssize_t n = read(fd, ptr + total, len - total);
        if (n <= 0) {
            return -1;
        }
        total += (size_t)n;
    }
    return 0;
}

/* ========================================================= */
/* ===================  SYSTEME DE LOGIN  ================== */
/* ========================================================= */

/* Vérifie qu'un fichier profil existe pour ce login (utilisé par l'authentification). */
int user_exists(const char *username) {
    char path[256];
    sprintf(path, "../stats/USERS/%s.txt", username);

    struct stat buffer;
    return (stat(path, &buffer) == 0);
}

/* Compare le mot de passe saisi avec celui stocké dans stats/USER. */
int check_password(const char *username, const char *password) {
    char path[256];
    sprintf(path, "../stats/USERS/%s.txt", username);

    FILE *f = fopen(path, "r");
    if (!f) return 0;

    char stored[32];
    if (fscanf(f, "PASSWORD=%s", stored) != 1) {
        fclose(f);
        return 0;
    }

    fclose(f);
    return strcmp(stored, password) == 0;
}

/* Crée un fichier utilisateur avec le mot de passe choisi. */
int create_user(const char *username, const char *password) {
    char path[256];
    sprintf(path, "../stats/USERS/%s.txt", username); // Enregistre le chemin dans "path"

    FILE *f = fopen(path, "w");
    if (!f) return 0;

    fprintf(f, "PASSWORD=%s\n", password);
    fclose(f);
    return 1;
}

/* Gestion du flux d'authentification : invite, vérifie ou crée l'utilisateur. */
void authentification(char *username_out) {
    char username[64];
    char password[16];

    printf("Entrez votre nom d'utilisateur (ou GUEST) : ");
    scanf("%s", username);

    if (strcasecmp(username, "GUEST") == 0) {
        strcpy(username_out, "GUEST");
        printf("Mode invité activé.\n");
        return;
    }

    if (user_exists(username)) {
        printf("Utilisateur trouvé. Entrez votre mot de passe (4 chiffres) : ");
        scanf("%s", password);

        if (!check_password(username, password)) {
            printf("Mot de passe incorrect.\n");
            exit(1);
        }

        printf("Connexion réussie.\n");
        strcpy(username_out, username);
        return;
    }

    printf("Ce login n'existe pas. Voulez-vous le créer ? (o/n) : ");
    char rep;
    scanf(" %c", &rep);

    if (rep != 'o' && rep != 'O') {
        printf("Annulation.\n");
        exit(1);
    }

    printf("Choisissez un mot de passe (4 chiffres) : ");
    scanf("%s", password);

    if (!create_user(username, password)) {
        printf("Erreur lors de la création du compte.\n");
        exit(1);
    }

    printf("Compte créé avec succès.\n");
    strcpy(username_out, username);
}

/* ========================================================= */
/* ======================  AFFICHAGE  ====================== */
/* ========================================================= */

/* Affiche la main du joueur avec indices et têtes de boeuf. */
void afficher_main(Carte *main) {
    printf("\nVotre main :\n");
    for (int i = 0; i < MAX_MAIN; i++) {
        if (main[i].valeur != -1) {
            printf("[%d] %d (têtes: %d)\n", i, main[i].valeur, main[i].tetes_boeuf);
        }
    }
}

/* Calcule le total de têtes d'une série pour l'affichage. */
int total_tetes_serie(Serie *s) {
    int total = 0;
    for (int i = 0; i < s->taille; i++) {
        total += s->cartes[i].tetes_boeuf;
    }
    return total;
}

/* Affiche l'état des 4 séries avec leur somme de têtes. */
void afficher_series(Serie *series) {
    printf("\nSéries sur la table :\n");

    for (int i = 0; i < NB_SERIES; i++) {
        int total = total_tetes_serie(&series[i]);

        printf("Ligne %d : ", i);

        for (int j = 0; j < series[i].taille; j++) {
            printf("%d ", series[i].cartes[j].valeur);
        }

        printf(" (total : %d têtes)\n", total);
    }
}

/* ========================================================= */
/* ====================  STATISTIQUES  ===================== */
/* ========================================================= */

/* Lance stats_joueur.awk pour rafraîchir le profil de l'utilisateur courant. */
static void mettre_a_jour_stats(const char *username, int joueur_id) {
    if (!username || username[0] == '\0' || joueur_id < 0)
        return;

    if (strcasecmp(username, "GUEST") == 0)
        return;

    if (access("../stats/stats_joueur.awk", R_OK) != 0 ||
        access("../stats/logs.csv", R_OK) != 0) // Vérifie si le fichier existe et qu'on a les permissions
        return;

    pid_t pid = fork();
    if (pid == 0) {
        char joueur_arg[32];
        char login_arg[128];

        snprintf(joueur_arg, sizeof(joueur_arg), "joueur=%d", joueur_id);
        snprintf(login_arg, sizeof(login_arg), "login=%s", username);

        execlp("awk", "awk",
               "-v", joueur_arg,
               "-v", login_arg,
               "-f", "../stats/stats_joueur.awk",
               "../stats/logs.csv",
               (char *)NULL);
        perror("awk");
        _exit(1);
    } else if (pid > 0) {
        waitpid(pid, NULL, 0);
    }
}

/* ========================================================= */
/* =========================  MAIN  ======================== */
/* ========================================================= */

/* Point d'entrée du client humain : s'authentifie, joue les 10 tours et met à jour son profil. */
int main() {

    /* ====== AUTHENTIFICATION ====== */
    char username[64];
    authentification(username);

    printf("\nBienvenue %s ! Connexion au serveur...\n", username);

    /* ====== CONNEXION RESEAU (syntaxe TP) ====== */
    int sockfd;
    struct sockaddr_in serv_addr;
    struct hostent *server;

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("socket");
        return 1;
    }

    server = gethostbyname(SERVER_IP);
    if (server == NULL) {
        fprintf(stderr, "Erreur : hôte inconnu\n");
        close(sockfd);
        return 1;
    }

    bzero((char *)&serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(SERVER_PORT);

    bcopy((char *)server->h_addr_list[0], //SocketClient.c
          (char *)&serv_addr.sin_addr.s_addr,
          server->h_length);

    if (connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("connect");
        close(sockfd);
        return 1;
    }

    /* Envoi du nom au serveur (sur 32 octets) */
    char login_buf[32];
    memset(login_buf, 0, sizeof(login_buf));
    strncpy(login_buf, username, 31);

    if (write(sockfd, login_buf, sizeof(login_buf)) < 0) {
        perror("write login");
        close(sockfd);
        return 1;
    }

    Carte main[MAX_MAIN];
    Serie series[NB_SERIES];

    /* ====================== PARTIES SUCCESSIVES ===================== */

    while (1) {
        int nouvelle_manche;
        if (recv_all(sockfd, &nouvelle_manche, sizeof(int)) < 0) {
            perror("recv manche");
            close(sockfd);
            return 1;
        }

        if (!nouvelle_manche)
            break;

        if (recv_all(sockfd, main, sizeof(Carte) * MAX_MAIN) < 0) {
            perror("recv main");
            close(sockfd);
            return 1;
        }

        if (recv_all(sockfd, series, sizeof(Serie) * NB_SERIES) < 0) {
            perror("recv series");
            close(sockfd);
            return 1;
        }

        for (int tour = 0; tour < MAX_MAIN; tour++) {
            afficher_main(main);
            afficher_series(series);

            int index;
            do {
                printf("\nEntrez l'index de la carte à jouer : ");
                scanf("%d", &index);
            } while (index < 0 || index >= MAX_MAIN || main[index].valeur == -1);

            Carte jouee = main[index];
            main[index].valeur = -1;

            printf("\nVous avez choisi la carte : %d (têtes : %d)\n",
                   jouee.valeur, jouee.tetes_boeuf);

            if (write(sockfd, &jouee, sizeof(Carte)) < 0) {
                perror("write carte");
                close(sockfd);
                return 1;
            }

            if (recv_all(sockfd, series, sizeof(Serie) * NB_SERIES) < 0) {
                perror("recv series (choix ligne)");
                close(sockfd);
                return 1;
            }

            afficher_series(series);
            int ligne_auto = trouver_ligne_placement(jouee, series);
            int ligne_choisie;

            if (ligne_auto == -1) {
                do {
                    printf("Votre carte est plus petite que toutes les cartes visibles.\n");
                    printf("Choisissez la ligne à ramasser (0-3) : ");
                    scanf("%d", &ligne_choisie);
                } while (ligne_choisie < 0 || ligne_choisie >= NB_SERIES);
            } 
            else {
                ligne_choisie = ligne_auto;
                printf("La carte %d se place automatiquement sur la ligne %d.\n",
                       jouee.valeur, ligne_choisie);
            }

            //Envoie ligne choisie
            if (write(sockfd, &ligne_choisie, sizeof(int)) < 0) {
                perror("write ligne");
                close(sockfd);
                return 1;
            }

            //Reception des series mise à jours
            if (recv_all(sockfd, series, sizeof(Serie) * NB_SERIES) < 0) {
                perror("recv series (maj)");
                close(sockfd);
                return 1;
            }

            //Mise à jour du score gérer par le serveur
            int score;
            if (recv_all(sockfd, &score, sizeof(int)) < 0) {
                perror("recv score");
                close(sockfd);
                return 1;
            }

            printf("\n[Tour %d] Carte jouée : %d | Ligne : %d | Score : %d\n",
                   tour + 1, jouee.valeur, ligne_choisie, score);
        }
    }

    /* ================= CLASSEMENT FINAL ================= */

    int nb_joueurs;
    if (recv_all(sockfd, &nb_joueurs, sizeof(int)) < 0) {
        perror("recv nb joueurs");
        close(sockfd);
        return 1;
    }

    int scores[MAX_JOUEURS];
    if (recv_all(sockfd, scores, sizeof(int) * nb_joueurs) < 0) {
        perror("recv scores");
        close(sockfd);
        return 1;
    }

    char noms[MAX_JOUEURS][32];
    if (recv_all(sockfd, noms, sizeof(noms)) < 0) {
        perror("recv noms");
        close(sockfd);
        return 1;
    }

    int classement[MAX_JOUEURS];
    if (recv_all(sockfd, classement, sizeof(int) * nb_joueurs) < 0) {
        perror("recv classement");
        close(sockfd);
        return 1;
    }

    printf("\n===== CLASSEMENT FINAL =====\n");
    for (int i = 0; i < nb_joueurs; i++) {
        int id = classement[i];
        char *nom = noms[id];

        if (strcmp(nom, "BOT") == 0)
            printf("%d. Joueur %d — %d points\n", i + 1, id, scores[id]);
        else
            printf("%d. %s — %d points\n", i + 1, nom, scores[id]);
    }
    printf("============================\n");

    int self_id = -1;
    for (int i = 0; i < nb_joueurs; i++) {
        if (strcmp(noms[i], username) == 0) {
            self_id = i;
            break;
        }
    }
    mettre_a_jour_stats(username, self_id);

    close(sockfd);
    return 0;
}
