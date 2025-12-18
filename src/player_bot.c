/* player_bot.c - Client automatique pour "6 qui prend" avec choix de la carte ET de la ligne */

#include "common.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>

#define SERVER_IP "127.0.0.1"
#define SERVER_PORT 12345

// Choisit la plus petite carte disponible dans la main
int choisir_carte(const Carte *main, int nb_cartes) {
    int index_min = -1;
    for (int i = 0; i < nb_cartes; i++) {
        if (main[i].valeur != -1) { // Ignorer les cartes déjà jouées
            if (index_min == -1 || main[i].valeur < main[index_min].valeur) {
                index_min = i;
            }
        }
    }
    return index_min;
}

int choisir_ligne(const Carte carte, Serie series[NB_SERIES]) {
    
    // 1) Essaye de trouver une ligne valide pour placer la carte
    int ligne_choisie = trouver_ligne_placement(carte, series);

    // 2) Si aucune ligne ne convient -> ramassage obligatoire
    if (ligne_choisie == -1) {
        ligne_choisie = choisir_ligne_minimale(series);
    }

    return ligne_choisie;
}

/*
 * recv_all()
 * ----------
 * Comme TCP ne garantit pas qu'un read() retourne toute la structure envoyée,
 * on boucle jusqu'à avoir lu exactement "len" octets pour rester synchronisé.
 */
static int recv_all(int fd, void *buf, size_t len) {
    size_t total = 0;
    char *ptr = buf;
    while (total < len) {
        ssize_t n = read(fd, ptr + total, len - total);
        if (n <= 0) return -1;
        total += (size_t)n;
    }
    return 0;
}

static int write_all(int fd, const void *buf, size_t len) {
    size_t total = 0;
    const char *ptr = buf;
    while (total < len) {
        ssize_t n = write(fd, ptr + total, len - total);
        if (n <= 0) return -1;
        total += (size_t)n;
    }
    return 0;
}

int main() {
    //socketClient.c-------- Création Socket
    int sockfd;
    struct sockaddr_in serv_addr; 

    sockfd = socket(AF_INET, SOCK_STREAM, 0); 
    if (sockfd < 0) {
        perror("socket");
        return 1;
    }

    //tcpServerFork.c--------- Préparation adresse serveur
    serv_addr.sin_family = AF_INET; 
    serv_addr.sin_port = htons(SERVER_PORT); 
    struct hostent *server = gethostbyname(SERVER_IP);
    if (!server) {
        fprintf(stderr, "Erreur : hôte inconnu\n");
        return 1;
    }

    //socketClient.c-----Copie de l’adresse IP dans la structure sockaddr_in
    bcopy((char*)server->h_addr, 
          (char*)&serv_addr.sin_addr.s_addr,
          server->h_length);

          //Connexion TCP au serveur
    if (connect(sockfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("connect");
        return 1;
    }

    // --- IDENTIFICATION DU BOT ---
    char login[32];
    int bot_id = getpid() % 10000;  // Un identifiant basé sur le PID du processus
    snprintf(login, sizeof(login), "BOT%d", bot_id);

    // Envoi du login au serveur
    if (write_all(sockfd, login, sizeof(login)) < 0) {
        perror("Creation login du bot");
        return 1;
    }

    //-- Structure stockage main et serie du bot --
    Carte main[MAX_MAIN];
    Serie series[NB_SERIES];


    //--Boucle manche --
    while (1) {

        //Gestion pour savoir si le serveur a dit si il y a une nouvelle manche ou non
        int nouvelle_manche;
        if (recv_all(sockfd, &nouvelle_manche, sizeof(int)) < 0) {
            perror("recv manche");
            close(sockfd);
            return 1;
        }

        if (!nouvelle_manche) //Si 0-> Pas de nouvelle manche -> On arrête
            break;

        //--

        if (recv_all(sockfd, main, sizeof(Carte) * MAX_MAIN) < 0) { //Reception main
            perror("recv main");
            return 1;
        }

        if (recv_all(sockfd, series, sizeof(Serie) * NB_SERIES) < 0) {  //Reception série
            perror("recv series");
            return 1;
        }

        
        // Debut de jeu sur la manche :
        for (int tour = 0; tour < MAX_MAIN; tour++) {
            int index_carte = choisir_carte(main, MAX_MAIN); //Choisi la carte à jouer selon choisir_carte()
            if (index_carte == -1) {
                fprintf(stderr, "Erreur : aucune carte valide à jouer.\n");
                break;
            }

            Carte carte_jouee = main[index_carte]; //Carte choisie
            main[index_carte].valeur = -1; //Marqué comme jouée

            //Envoie la carte choisi au serveur
            if (write_all(sockfd, &carte_jouee, sizeof(Carte)) < 0) {
                perror("write carte");
                return 1;
            }

            //Le serveur envoie les series actualisée
            if (recv_all(sockfd, series, sizeof(Serie) * NB_SERIES) < 0) {
                perror("recv series pour choix de ligne");
                return 1;
            }

            int ligne_choisie = choisir_ligne(carte_jouee, series); //Choisi la ligne grace à choisir_ligne()

            //Envoie ligne choisie au serveur
            if (write_all(sockfd, &ligne_choisie, sizeof(int)) < 0) {
                perror("write ligne");
                return 1;
            }

            //Reception des series mise à jours
            if (recv_all(sockfd, series, sizeof(Serie) * NB_SERIES) < 0) {
                perror("recv maj series");
                return 1;
            }

            //Mise à jour du score gérer par le serveur
            int score;
            if (recv_all(sockfd, &score, sizeof(int)) < 0) {
                perror("recv score");
                return 1;
            }

            //Info terminal
            printf("[Bot] Tour %d - Carte jouée: %d - Ligne choisie: %d - Score actuel: %d\n",
                   tour + 1, carte_jouee.valeur, ligne_choisie, score);
        }
    }

    //Ferme la socket puis le programme
    close(sockfd);
    return 0;
}
