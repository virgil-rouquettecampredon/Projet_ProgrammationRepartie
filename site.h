#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#ifndef PROJET_PROG_REP_SITE_H
#define PROJET_PROG_REP_SITE_H

#define GAGNANT 401
#define PERDANT 402
#define PASSIF 200
#define PARTICIPANT 201
#define CAPTURE 202

#define AT 300
#define RE 301
#define DM 302
#define RT 303
#define VI 304


#define true 1
#define false 0

struct sockaddr_in null_sockaddr_in;

struct siteState {
    struct sockaddr_in *pere;
    int id;
    int puissance;
    int puissance_pere;
    int etat;
    char *ip;
    unsigned int port;
    struct sockaddr_in *liste_IP;
};

struct message {
    int type_message; // AT : Attaque / RE : Résultat / DM : Demande Puissance Pere / RT : Retour Puissance Pere / VI : Victoire
    int puissance; // Puissance du site
    int id; // ID du site utile pour casser la symétrie en cas de puissance égale
    int type_resultat; // GAGNANT ou PERDANT (ou NULL)
    struct sockaddr_in attaquant; // Adresse de l'attaquant pour Retour de puissance
};

struct message * creer_message(int type_message, int puissance, int id, int type_resultat, struct sockaddr_in attaquant) {
    //Wait for 100ms-200ms
    usleep(rand() % 100001 + 100000);

    struct message *msg = malloc(sizeof(struct message));
    msg->type_message = type_message;
    msg->puissance = puissance;
    msg->id = id;
    msg->type_resultat = type_resultat;
    msg->attaquant = attaquant;
    return msg;
}

void afficher_message(struct message *msg) {
    if (msg->type_message == AT) {
        printf("|ATTAQUE\n");
    } else if (msg->type_message == RE) {
        printf("|RESULTAT\n");
    } else if (msg->type_message == DM) {
        printf("|DEMANDE PUISSANCE PERE\n");
    } else if (msg->type_message == RT) {
        printf("|RETOUR PUISSANCE PERE\n");
    } else if (msg->type_message == VI) {
        printf("|VICTOIRE\n");
    }
    printf("|Puissance : %d\n", msg->puissance);
    printf("|ID : %d\n", msg->id);
    if (msg->type_resultat == GAGNANT) {
        printf("|Résultat : GAGNANT\n");
    } else if (msg->type_resultat == PERDANT) {
        printf("|Résultat : PERDANT\n");
    }
    //Check if msg->attaquant is not equal to sockaddr_in null
    if (msg->attaquant.sin_addr.s_addr != null_sockaddr_in.sin_addr.s_addr) {
        printf("|Attaquant : %s:%d\n", inet_ntoa(msg->attaquant.sin_addr), ntohs(msg->attaquant.sin_port));
    }
}

struct siteState *initSiteState(char *ip, unsigned int port, struct sockaddr_in *liste_IP, int nbAdversaire) {
    struct siteState *site;
    site->id = -1;
    site->pere = malloc(sizeof(struct sockaddr_in*));
    site->pere = memset(site->pere, 0, sizeof(struct sockaddr_in));
    site->puissance_pere = 0;
    site->puissance = 1;
    site->etat = PARTICIPANT;
    site->ip = ip;
    site->port = port;
    site->liste_IP = (struct sockaddr_in *) malloc(sizeof(struct sockaddr_in) * nbAdversaire);
    for (int i = 0; i < nbAdversaire; i++) {
        site->liste_IP[i] = liste_IP[i];
    }

    return site;
}

char **split(char s[], char *delimiteur, int n) {

    int i = 0;
    char *p = strtok(s, delimiteur);
    char **tab = (char **) malloc(n * sizeof(char *));
    while (p != NULL) {
        tab[i] = p;
        p = strtok(NULL, delimiteur);
        i++;
    }
    return tab;
}

//fonction qui initialise le tableau des adversiars
struct sockaddr_in *initAddrServer(char *str, int n) {
    struct sockaddr_in *addrServer = (struct sockaddr_in *) malloc(n * sizeof(struct sockaddr_in));
    char *delimiterLigne = "$";
    char *delimiterIp = ":";
    //on decoupe les lignes avec le delimiterLigne
    char **lines = split(str, delimiterLigne, n);
    char **ipPort;
    int i = 0;
    while (i < n) {
        //on parcourt les lignes et on split sur le delimiterIp pour avooir l'ip et le port de l'adversaire représenté dans la ligne parcourue
        ipPort = split(lines[i], delimiterIp, 2);
        addrServer[i].sin_family = AF_INET;
        addrServer[i].sin_addr.s_addr = inet_addr(ipPort[0]);
        addrServer[i].sin_port = htons(atoi(ipPort[1]));
        i++;
    }

    return addrServer;
}

//remove one specified sockaddr_in from addrServer
void removeAddrServer(struct sockaddr_in *addrServerOpponent, char *ip, unsigned int port, int *y) {
    int i = 0;
    while (((strcmp(inet_ntoa(addrServerOpponent[i].sin_addr), ip) != 0) ||
            (ntohs(addrServerOpponent[i].sin_port) != port)) && (i < *y)) {
        i++;
    }

    if (i < *y) {
        for (int j = i; j < *y - 1; j++) {
            addrServerOpponent[j] = addrServerOpponent[j + 1];
        }
        *y = *y - 1;
    }
}

int sendTCP(int socket, const char *buffer, size_t length) {
    ssize_t sent;

    while (length > 0) {
        sent = send(socket, buffer, length, 0);

        if (sent <= 0) return sent;

        buffer += sent;
        length -= sent;
    }
    return 1;
}

int recvTCP(int socket, char *buffer, size_t length) {
    ssize_t received;

    while (length > 0) {
        received = recv(socket, buffer, length, 0);

        if (received <= 0) return received;

        buffer += received;
        length -= received;
    }
    return 1;
}

#endif //PROJET_PROG_REP_SITE_H
