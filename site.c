#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include "calcul.h"
#include <netinet/in.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
//#include "site.h"

//Sabri the sublime

#define GAGNANT 1
#define PERDANT 0
#define PASSIF 200
#define PARTICIPANT 201
#define CAPTURE 202

struct siteState {
    struct sockaddr_in *pere;
    int puissance;
    int puissance_Pere;
    int etat;
    char *ip;
    int port;
    struct sockaddr_in *liste_IP;
};

struct siteState *initSiteState(char *ip, char *port, struct sockaddr_in *liste_IP) {
    struct siteState *site;
    site->pere = (void *) 0;
    site->puissance_Pere = 0;
    site->etat = PARTICIPANT;
    site->ip = ip;
    site->port = atoi(port);
    site->liste_IP = liste_IP;

    return site;
}

/*char** splitIp(char* ip){
    char** ip_array = (char**)malloc(4 * sizeof(char*));
    const char* delimiter = ":";
    strtok(ip, delimiter);
    ip_array[0] = ip;
    ip_array[1] = strtok(NULL, delimiter);

    return ip_array;
}
*/
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

/*struct sockaddr_in* initAddrServer(const char* emplacementFichier, int* y){
    //load file
    FILE* fichier = fopen(emplacementFichier, "r");
    //return error if file doesn't exist
    if(fichier == NULL){
        printf("Error: file doesn't exist\n");
        exit(1);
    }
    struct sockaddr_in* addrServer;
    char* ipTemp;
    ssize_t nbLus = 0;
    int i = 0;
    while ((getline(&ipTemp, &nbLus, fichier)) != -1) {
        i++;
    }
    addrServer = (struct sockaddr_in*)malloc(i * sizeof(struct sockaddr_in));
    rewind(fichier);

    i = 0;
    int tailleSockAddr_in = sizeof(struct sockaddr_in);
    while ((getline(&ipTemp, &nbLus, fichier)) != -1){
        char** ip_array = splitIp(ipTemp);
        addrServer[i].sin_family = AF_INET;
        addrServer[i].sin_addr.s_addr = inet_addr(ip_array[0]);
        addrServer[i].sin_port = htons(atoi(ip_array[1]));
        i++;
    }
    *y = i;
    return addrServer;
}*/
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
        //printf(" %s  ", lines[i]);
        //printf(" %s  ", ipPort[0]);
        //printf(" %s  \n", ipPort[1]);
        addrServer[i].sin_family = AF_INET;
        addrServer[i].sin_addr.s_addr = inet_addr(ipPort[0]);
        addrServer[i].sin_port = htons(atoi(ipPort[1]));
        i++;
    }

    return addrServer;
}

//remove one specified sockaddr_in from addrServer
void removeAddrServer(struct sockaddr_in *addrServerOpponent, char **ip, int *y) {
    int i = 0;
    while (((strcmp(inet_ntoa(addrServerOpponent[i].sin_addr), ip[0]) != 0) ||
            (ntohs(addrServerOpponent[i].sin_port) != atoi(ip[1]))) && (i < *y)) {
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

        if (sent <= 0) {
            return sent;
        }

        buffer += sent;
        length -= sent;
    }
    return 1;
}

int recvTCP(int socket, char *buffer, size_t length) {
    ssize_t received;

    while (length > 0) {
        received = recv(socket, buffer, length, 0);

        if (received <= 0) {
            return received;
        }

        buffer += received;
        length -= received;
    }
    return 1;
}

int main(int argc, const char *argv[]) {

    //test if there are 3 arguments
    if (argc != 4) {
        printf("Utilisation: %s addresse_IP_du_erveur_central  numero_de_port_du_serveur_central le_numero_de_port_du_site\n",
               argv[0]);
        exit(1);
    }
    //Connexion au serveur central
    int ds = socket(PF_INET, SOCK_STREAM, 0);
    if (ds < 0) {
        perror("Client : erreur creation socket");
    }

    struct sockaddr_in adrServ;
    adrServ.sin_addr.s_addr = inet_addr(argv[1]);
    adrServ.sin_family = AF_INET;
    adrServ.sin_port = htons((short) atoi(argv[2]));

    int lgAdr = sizeof(struct sockaddr_in);

    int conn = connect(ds, (struct sockaddr *) &adrServ, lgAdr);
    if (conn < 0) {
        perror("Client : erreur connect");
        close(ds);
        exit(1);
    }

    printf("Attente du nombre de sites...\n");

    //Réception nbSites
    int nbSites;
    int rcv = recvTCP(ds, &nbSites, sizeof(int));
    /* Traiter TOUTES les valeurs de retour (voir le cours ou la documentation). */
    if (rcv < 0) {
        perror("Client: probleme recv :");
        close(ds);
        exit(1);
    }
    if (rcv == 0) {
        perror("Client: serveur out of reach :");
        close(ds);
        exit(1);
    }
    printf("Nombre de sites : %d\n", nbSites);
    printf("Attente de l'identité des autres sites...\n");

    //Reception des ips et ports des autres sites sous forme d'une chaine de char avec delimit
    //Taille message ip/port
    int tailleSite = 22 * nbSites;
    char allClient[tailleSite];

    rcv = recvTCP(ds, allClient, tailleSite);
    if (rcv < 0) {
        perror("Client: probleme recv :");
        close(ds);
        exit(1);
    }
    if (rcv == 0) {
        perror("Client: serveur out of reach :");
        close(ds);
        exit(1);
    }

    printf("Liste des adversaires : %s\n", allClient);

    // Création d'un tableau de socketAdversaire
    struct sockaddr_in* addrServer = (struct sockaddr_in*) malloc(nbSites * sizeof(struct sockaddr_in));
    // Initialisation du tableau des adversaires
    addrServer = initAddrServer(allClient, nbSites);

//    //split argv[1] with splitIp
//    char** selfIp = splitIp(argv[1]);
//    printf("Mon IP : %s", selfIp[0]);
//    printf(":%s\n", selfIp[1]);
//
//    int nombreElement;
//    addrServer = initAddrServer(argv[2], &nombreElement);
//
//
//    removeAddrServer(addrServer, selfIp, &nombreElement);
//
//    //print all sockaddr_in from addrServer
//    for(int i = 0; i < nombreElement; i++){
//        printf("Adresse IP %d: %s:%d\n",i, inet_ntoa(addrServer[i].sin_addr),ntohs(addrServer[i].sin_port));
//    }
//
//    printf("Initialisation self state\n");
//
//    struct siteState* selfState = initSiteState(selfIp[0], selfIp[1], addrServer);
//    printf("%s", selfState->ip);
//    printf(":%d\n", selfState->port);
//    printf("%d\n", selfState->puissance_Pere);
//    printf("Is father null ? %d\n", selfState->pere==(void*)0);


    close(ds);


    return 0;
}