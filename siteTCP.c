
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


#define true 1
#define false 0


struct siteState {
    struct sockaddr_in *pere;
    int id;
    int puissance;
    int puissance_pere;
    int etat;
    char *ip;
    int port;
    struct sockaddr_in *liste_IP;
};

struct siteState *initSiteState(char *ip, char *port, struct sockaddr_in *liste_IP) {
    struct siteState *site;
    site->id = -1;
    site->pere = (void *) 0;
    site->puissance_pere = 0;
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
}*/

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
        //on parcourt les lignes et on split sur le delimiterIp pour avooir l'ip et le port de l'adversaire repr??sent?? dans la ligne parcourue
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
void removeAddrServer(struct sockaddr_in *addrServerOpponent, char *ip, char* port,  int *y) {
    int i = 0;
    while (((strcmp(inet_ntoa(addrServerOpponent[i].sin_addr), ip) != 0) ||
            (ntohs(addrServerOpponent[i].sin_port) != atoi(port))) && (i < *y)) {
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

//Return a list of opened socket to all the opponents
int* connectToOpponent(struct sockaddr_in *addrServer, int position, int numberOpponents) {
    printf("Connecting to opponents...\n");
    int* dsOpponents = (int *) malloc(numberOpponents * sizeof(int));
    int beginningPos = position;
    int lgAdr = sizeof(struct sockaddr_in);

    int opponentId = 0;

    //If I'm not the first site, I await connections
    if (position != 0) {
        int ds = socket(PF_INET, SOCK_STREAM, 0);
        if (ds < 0) {
            perror("Client : erreur creation socket");
        }

        if (bind(ds, (struct sockaddr *) &addrServer[position], sizeof(struct sockaddr)) < 0) {
            perror("Serveur : erreur bind");
            close(ds);
            exit(1);
        }

        int ecoute = listen(ds, numberOpponents);
        if (ecoute < 0) {
            perror("Serveur : erreur ecoute");
            close(ds);
            exit(1);
        }

        while (position != 0) {
            printf("Serveur : j'attends la demande d'un client (accept) \n");

            struct sockaddr_in adc;
            socklen_t lgc = sizeof(struct sockaddr_in);

            int dsc = accept(ds, (struct sockaddr *) &adc, &lgc);
            if (dsc < 0) {
                perror("Serveur : erreur accept");
                exit(1);
            }

            dsOpponents[opponentId] = dsc;
            printf("Ajout de l'opposant : %d\n", opponentId);
            opponentId++;


            printf("Serveur : le client %s:%d est connect??\n", inet_ntoa(adc.sin_addr), ntohs(adc.sin_port));
            position--;
        }
    }

    printf("Self : %d | Next to be added : %d\n", beginningPos, opponentId);
    opponentId++;

    int dsOpponent;

    for (int i = beginningPos + 1; i < numberOpponents; i++) {
        printf("Je me connecte ?? mes opposants\n");
        //Connexion au serveur central
        dsOpponent = socket(PF_INET, SOCK_STREAM, 0);
        if (dsOpponent < 0) {
            perror("Client : erreur creation socket");
        }
        int conn = connect(dsOpponent, (struct sockaddr *) &addrServer[i], lgAdr);
        if (conn < 0) {
            perror("Client : erreur connect");
            close(dsOpponent);
            exit(1);
        }
        printf("Ajout de l'opposant : %d\n", opponentId);
        dsOpponents[opponentId] = dsOpponent;
        opponentId++;
    }

    return dsOpponents;
}

int main(int argc, const char *argv[]) {

    //test if there are 2 arguments
    if (argc != 3) {
        printf("Utilisation: %s addresse_IP_du_serveur_central  numero_de_port_du_serveur_central\n",
               argv[0]);
        exit(1);
    }
    //Connexion au serveur central
    int ds = socket(PF_INET, SOCK_STREAM, 0);
    if (ds < 0) {
        perror("Client : erreur creation socket");
    }

    //Addresse du serveur central
    struct sockaddr_in adrServ;
    adrServ.sin_addr.s_addr = inet_addr(argv[1]);
    adrServ.sin_family = AF_INET;
    adrServ.sin_port = htons((short) atoi(argv[2]));

    int lgAdr = sizeof(struct sockaddr_in);

    //Connexion au serveur central
    int conn = connect(ds, (struct sockaddr *) &adrServ, lgAdr);
    if (conn < 0) {
        perror("Client : erreur connect");
        close(ds);
        exit(1);
    }

    printf("Attente du nombre de sites...\n");

    //R??ception nbSites
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
    printf("Attente de l'identit?? des autres sites...\n");

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

    // Cr??ation d'un tableau de socketAdversaire
    struct sockaddr_in *addrServer = (struct sockaddr_in *) malloc(nbSites * sizeof(struct sockaddr_in));
    // Initialisation du tableau des adversaires
    addrServer = initAddrServer(allClient, nbSites);

    struct sockaddr_in selfAddr;
    char myIP[16];
    unsigned int myPort;

    // Get my ip address and port
    bzero(&selfAddr, sizeof(selfAddr));
    socklen_t len = sizeof(selfAddr);
    getsockname(ds, (struct sockaddr *) &selfAddr, &len);
    inet_ntop(AF_INET, &selfAddr.sin_addr, myIP, sizeof(myIP));
    myPort = ntohs(selfAddr.sin_port);

    printf("Local ip address: %s\n", myIP);
    printf("Local port : %u\n", myPort);

    //Find position of selfAddr in addrServer
    int selfPosition = -1;
    int i = 0;
    while (i < nbSites && selfPosition == -1) {
        printf("Comparing %s:%d with %s:%d\n", myIP, myPort, inet_ntoa(addrServer[i].sin_addr),
               ntohs(addrServer[i].sin_port));
        if (strcmp(myIP, inet_ntoa(addrServer[i].sin_addr)) == 0 && myPort == ntohs(addrServer[i].sin_port)) {
            selfPosition = i;
        }
        i++;
    }
    printf("selfPosition : %d\n", selfPosition);

    close(ds);

    int* tabOpponents = (int*) malloc(nbSites * sizeof(int));

    tabOpponents = connectToOpponent(addrServer, selfPosition, nbSites);

    /*
    printf("Cr??ation de la socket UDP...\n");
    ds = socket(PF_INET, SOCK_DGRAM, 0);
    if (ds < 0) {
        perror("Client : erreur creation socket UDP");
    }

    printf("Bind de la socket UDP...\n");
    if (bind(ds, (struct sockaddr *) &selfAddr, sizeof(selfAddr)) < 0) {
        perror("Client : erreur bind");
        close(ds);
        exit(1);
    }
     */

    /* Initialisation du multiplexage */
    fd_set set;
    FD_ZERO(&set);
    for(int i = 0; i < nbSites; i++) {
        if(i!=selfPosition) {
            FD_SET(tabOpponents[i], &set);
        }
    }

    int BUFFER_SIZE = 16000;

    char *buffer = (char *) malloc(BUFFER_SIZE);
    sprintf(buffer, "Je suis le site -> %s:%d\n", myIP, myPort);


    printf("Envoi de son identit?? au prochain site : %d\n", (selfPosition + 1) % nbSites);
    printf("Prochain site  : %d\n", tabOpponents[(selfPosition + 1) % nbSites]);
    int snd = sendTCP(tabOpponents[(selfPosition + 1) % nbSites], buffer, strlen(buffer));
    if (snd < 0) {
        perror("Client : erreur sendto");
        close(ds);
        exit(1);
    }

    printf("R??ception d'un message...\n");
    char buffer2[BUFFER_SIZE];

    int messageReceived = 0;

    while(!messageReceived) {
        int sel = select(nbSites, tabOpponents, NULL, NULL, NULL);
        if (sel < 0) {
            perror("Client : erreur select");
            close(ds);
            exit(1);
        }

        for(int i=0; i<nbSites; i++) {
            if(FD_ISSET(tabOpponents[i], &set)) {
                int rcv = recvTCP(tabOpponents[i], buffer2, BUFFER_SIZE);
                if (rcv < 0) {
                    perror("Client : probleme recv :");
                    close(ds);
                    exit(1);
                }
                if (rcv == 0) {
                    perror("Client : serveur out of reach :");
                    close(ds);
                    exit(1);
                }
                messageReceived = 1;
                printf("Message re??u : %s\n", buffer2);
            }
        }
    }

    close(ds);

    /*

    struct siteState *me = initSiteState(myIP, myPort, &addrServer);
    me->id = selfPosition;

    int victoire = false;
    int nbSiteAttack = nbSites;
    int resRecu = true; // au debut tout le monde peut attaquer on met resRecu a true dans ce cas l??

    struct sockaddr_in *attacker;
    int attacker_puissance;
    socklen_t lg = sizeof(struct sockaddr_in);
    while (me->puissance < (nbSites / 2) && !victoire) {

//me est attaquant dans cette conditon ( le cas ou x envoie un message d'attaque)
        if (me->etat == PARTICIPANT && resRecu) {
            resRecu = false;
            int y = rand() % nbSiteAttack;
            char puissance[3];
            sprintf(puissance, "%d", me->puissance);
            char attaque[10] = "AT:";
            strcat(attaque, puissance);
            char id[3];
            sprintf(id, "%d", me->id);
            strcat(attaque, id);
            struct sockaddr_in cible = addrServer[y];
            //envoi du message d'attaque ?? la cible
            sendto(ds, attaque, 10, 0, (struct sockaddr *) &cible, sizeof(cible));
        }

        struct sockaddr_in contact;
        char message[10];
        recvfrom(ds, message, 10, 0, (struct sockaddr *) &contact, &lg);

        char delimiter = ":";
        char **contenuMsg = split(message, delimiter, 4);

        if (strcmp(contenuMsg[1], "AT") == 0) {                                    //MESSAGE ATTAQUE RECU
            if (me->puissance > atoi(contenuMsg[2])) {                      //CAS OU JE SUIS LE PLUS FORT
                char res[3];
                sprintf(res, "%d", PERDANT);
                char resultat[10] = "RE:";
                strcat(resultat, res);
                sendto(ds, resultat, 10, 0, (struct sockaddr *) &contact, &lg);
            } else if (me->puissance < atoi(contenuMsg[2])) {               //CAS OU JE SUIS LE PLUS FAIBLE
                if (me->etat != CAPTURE) {                                        //CAS OU JE NE SUIS PAS CAPTURE
                    char res[3];
                    sprintf(res, "%d", GAGNANT);
                    char resultat[10] = "RE:";
                    strcat(resultat, res);
                    sendto(ds, resultat, 10, 0, (struct sockaddr *) &contact, &lg);
                    me->pere = &contact;
                    me->puissance_pere = atoi(contenuMsg[2]) + 1;
                } else {                                                          //CAS OU JE SUIS CAPTURE
                    attacker = &contact;
                    attacker_puissance = atoi(contenuMsg[2]);
                    char *puissance_adversaire = contenuMsg[2];
                    char demande[10] = "DM:";
                    strcat(demande, puissance_adversaire);
                    char *id = contenuMsg[3];
                    strcat(demande, puissance_adversaire);
                    strcat(demande, id);
                    sendto(ds, demande, 10, 0, (struct sockaddr *) me->pere, &lg);
                }
            } else if (me->id >
                       atoi(contenuMsg[3])) {                      //CAS OU NOTRE PUISSANCE EST EGAL ET QUE J'AI UN ID PLUS GRAND
                char res[3];
                sprintf(res, "%d", PERDANT);
                char resultat[10] = "RE:";
                strcat(resultat, res);
                sendto(ds, resultat, 10, 0, (struct sockaddr *) &contact, &lg);
            } else {                                                             //CAS OU NOTRE PUISSANCE EST EGAL ET QUE J'AI UN ID PLUS PETIT
                char res[3];
                sprintf(res, "%d", GAGNANT);
                char resultat[10] = "RE:";
                strcat(resultat, res);
                sendto(ds, resultat, 10, 0, (struct sockaddr *) &contact, &lg);
                me->pere = &contact;
                me->puissance_pere = atoi(contenuMsg[2]) + 1;
            }
        }


        if (strcmp(contenuMsg[1], "DM") == 0) {                                   //MESSAGE DEMANDE DE PUISSANCE RECU
            if (me->puissance > atoi(contenuMsg[2])) {                      //CAS OU JE SUIS LE PLUS FORT
                char res[3];
                sprintf(res, "%d", PERDANT);
                char resultat[10] = "RT:";
                strcat(resultat, res);
                sendto(ds, resultat, 10, 0, (struct sockaddr *) &contact, &lg);
            } else if (me->puissance < atoi(contenuMsg[2])) {               //CAS OU JE SUIS LE PLUS FAIBLE
                char res[3];
                sprintf(res, "%d", GAGNANT);
                char resultat[10] = "RE:";
                strcat(resultat, res);
                sendto(ds, resultat, 10, 0, (struct sockaddr *) &contact, &lg);
                me->pere = &contact;
                me->puissance_pere = atoi(contenuMsg[2]) + 1;

            }
                //estampille dans le cas de puissances egales
            else if (me->id > atoi(contenuMsg[3])) {                        //CAS OU NOTRE PUISSANCE EST EGAL ET QUE J'AI UN ID PLUS GRAND
                char res[3];
                sprintf(res, "%d", PERDANT);
                char resultat[10] = "RE:";
                strcat(resultat, res);
                sendto(ds, resultat, 10, 0, (struct sockaddr *) &contact, &lg);
            } else {                                                              //CAS OU NOTRE PUISSANCE EST EGAL ET QUE J'AI UN ID PLUS PETIT
                char res[3];
                sprintf(res, "%d", GAGNANT);
                char resultat[10] = "RE:";
                strcat(resultat, res);
                sendto(ds, resultat, 10, 0, (struct sockaddr *) &contact, &lg);
                me->pere = &contact;
                me->puissance_pere = atoi(contenuMsg[2]) + 1;
            }

        }
        if (strcmp(contenuMsg[1], "RT") == 0) {                                   //MESSAGE REPONSE DEMANDE DE PUISSANCE RECU
            if (atoi(contenuMsg[2]) == GAGNANT) {                          //CAS OU LE PERE A PERDU
                me->pere = attacker;
                me->puissance_pere = attacker_puissance;
                char res[3];
                sprintf(res, "%d", GAGNANT);
                char resultat[10] = "RE:";
                strcat(resultat, res);
                sendto(ds, resultat, 10, 0, (struct sockaddr *) &attacker, &lg);
            } else {                                                             //CAS OU LE PERE A GAGNE
                char res[3];
                sprintf(res, "%d", PERDANT);
                char resultat[10] = "RE:";
                strcat(resultat, res);
                sendto(ds, resultat, 10, 0, (struct sockaddr *) &attacker, &lg);
            }

        }

        if (strcmp(contenuMsg[1], "RE") == 0) {                                   //MESSAGE RESULTAT RECU
            resRecu = true;
            if (atoi(contenuMsg[2]) == GAGNANT) {                           //CAS OU LE SITE A GAGNE
                me->puissance = me->puissance + 1;
                nbSiteAttack--;
                //concatenation ip and port of contact in a char[][]
                char ipCaptured[15];
                strcpy(ipCaptured, inet_ntoa(contact.sin_addr));;
                char portCaptured[5];
                strcpy(portCaptured,ntohs(contact.sin_port));
                removeAddrServer(addrServer,ipCaptured,portCaptured, nbSiteAttack); // Supprimer le site captur?? de la liste des site ?? attaquer
            } else {                                                              //CAS OU LE SITE A PERDU
                me->etat = PASSIF;
            }

        }

        if (me->puissance > (nbSites / 2)) {                                       //CAS OU J'AI GAGNE
            char res[3];
            sprintf(res, "%d", GAGNANT);
            char resultat[10] = "VI:";
            strcat(resultat, res);
            for (int j = 0; j < nbSites; ++j) {
                sendto(ds, resultat, 10, 0, (struct sockaddr *) &me->liste_IP[j], &lg);
            }
        }
        if (strcmp(contenuMsg[1], "VI") == 0) {                                   //MESSAGE VICTOIRE RECU
            victoire = true;
        }

    }

     */


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