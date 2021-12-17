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
#include <time.h>
#include <stdlib.h>
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
    unsigned int port;
    struct sockaddr_in *liste_IP;
};

struct siteState *initSiteState(char *ip, unsigned int port, struct sockaddr_in *liste_IP, int nbAdversaire) {
    struct siteState *site;
    site->id = -1;
    site->pere = (void *) 0;
    site->puissance_pere = 0;
    site->puissance = 1;
    site->etat = PARTICIPANT;
    site->ip = ip;
    site->port = port;
    site->liste_IP = (struct sockaddr_in*) malloc(sizeof(struct sockaddr_in) * nbAdversaire);
    for(int i = 0; i<nbAdversaire; i++) {
        site->liste_IP[i] = liste_IP[i];
    }

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
int *connectToOpponent(struct sockaddr_in *addrServer, int position, int numberOpponents) {
    printf("Connecting to opponents...\n");
    int dsOpponents[numberOpponents - 1];
    int beginningPos = position;
    int lgAdr = sizeof(struct sockaddr_in);
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

        int ecoute = listen(ds, numberOpponents - position - 1);
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

            printf("Serveur : le client %s:%d est connecté\n", inet_ntoa(adc.sin_addr), ntohs(adc.sin_port));
            position--;
        }
    }

    int dsOpponent;

    for (int i = beginningPos + 1; i < numberOpponents; i++) {
        printf("Je me connecte à mes opposants\n");
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
        dsOpponents[i - 1] = dsOpponent;
    }
}

int main(int argc, const char *argv[]) {

    //test if there are 2 arguments
    if (argc != 3) {
        printf("Utilisation: %s addresse_IP_du_serveur_central  numero_de_port_du_serveur_central\n",
               argv[0]);
        exit(1);
    }
    srand(time(NULL));
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

    // Création d'un tableau de socketAdversaire
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

    //Find position of selfAddr in addrServer
    int selfPosition = -1;
    int i = 0;
    while (i < nbSites && selfPosition == -1) {
        if (strcmp(myIP, inet_ntoa(addrServer[i].sin_addr)) == 0 && myPort == ntohs(addrServer[i].sin_port)) {
            selfPosition = i;
        }
        i++;
    }

    close(ds);

    ds = socket(PF_INET, SOCK_DGRAM, 0);
    if (ds < 0) {
        perror("Client : erreur creation socket UDP");
    }

    if (bind(ds, (struct sockaddr *) &selfAddr, sizeof(selfAddr)) < 0) {
        perror("Client : erreur bind");
        close(ds);
        exit(1);
    }

    /* int BUFFER_SIZE = 16000;

     char *buffer = (char *) malloc(BUFFER_SIZE);
     sprintf(buffer, "Je suis le site -> %s:%d\n", myIP, myPort);


     printf("Envoi de son identité au prochain site : %d\n", (selfPosition + 1) % nbSites);
     int snd = sendto(ds, buffer, strlen(buffer), 0, (struct sockaddr *) &addrServer[(selfPosition + 1) % nbSites],
                      sizeof(addrServer[(selfPosition + 1) % nbSites]));
     if (snd < 0) {
         perror("Client : erreur sendto");
         close(ds);
         exit(1);
     }

     printf("Réception d'un message...\n");
     char buffer2[BUFFER_SIZE];
     rcv = recv(ds, buffer2, BUFFER_SIZE, 0);
     if (rcv < 0) {
         perror("Client : erreur recv");
         close(ds);
         exit(1);
     }

     printf("Message reçu : %s", buffer2);

     close(ds);
 */

    int nbSiteAttack = nbSites;
    removeAddrServer(addrServer, myIP, myPort, &nbSiteAttack);
    for(int i=0; i<nbSiteAttack; i++){
        printf("Adversaire %d : %s:%d\n", i, inet_ntoa(addrServer[i].sin_addr), ntohs(addrServer[i].sin_port));
    }

    struct siteState *me = (struct siteState *) malloc(sizeof(struct siteState));
    me = initSiteState(myIP, myPort, addrServer, nbSiteAttack);
    me->id = selfPosition;
    printf("Mon port : %d\n", myPort);

    int victoire = false;
    int resRecu = true; // au debut tout le monde peut attaquer on met resRecu a true dans ce cas là

    struct sockaddr_in *attacker;
    int attacker_puissance;
    socklen_t lg = sizeof(struct sockaddr_in);

    int z = 0;
    while (me->puissance < (nbSites / 2) + 1 && !victoire) {
        printf("-----------------------------------------------------\n");
        printf("Début du tour : %d\n", z);
        z++;

//me est attaquant dans cette conditon ( le cas ou x envoie un message d'attaque)
        if (me->etat == PARTICIPANT && resRecu) {
            resRecu = false;
            int y = rand() % nbSiteAttack;
            char puissance[3];
            sprintf(puissance, "%d", me->puissance);
            //printf("Puissance me : %i\n", me->puissance);
            //printf("Puissance String: %s\n", puissance);
            char attaque[10] = "AT:";
            strcat(attaque, puissance);
            char id[3];
            sprintf(id, "%d", me->id);
            strcat(attaque, ":");
            strcat(attaque, id);
            struct sockaddr_in cible = addrServer[y];
            //envoi du message d'attaque à la cible
            printf("Envoi de l'attaque au site %s:%d\n", inet_ntoa(addrServer[y].sin_addr),
                   ntohs(addrServer[y].sin_port));
            sendto(ds, attaque, 10, 0, (struct sockaddr *) &cible, sizeof(cible));
        }

        struct sockaddr_in contact;
        char message[10];
        printf("Attente d'un message...\n");
        int receive = recvfrom(ds, message, 10, 0, (struct sockaddr *) &contact, &lg);
        if (receive < 0) {
            perror("Client : erreur recvfrom");
            close(ds);
            exit(1);
        }
        printf("Message reçu : %s\n", message);
        char *delimiter = ":";
        char **contenuMsg = split(message, delimiter, 4);

        if (strcmp(contenuMsg[0], "AT") == 0) {
            printf("Reception d'une attaque\n");
            if (me->puissance > atoi(contenuMsg[1])) {                      //CAS OU JE SUIS LE PLUS FORT
                printf("Je suis le plus fort\n");
                char res[3];
                sprintf(res, "%d", PERDANT);
                char resultat[10] = "RE:";
                strcat(resultat, res);
                printf("Envoi du message de résultat au site : %s:%d\n", inet_ntoa(contact.sin_addr),
                       ntohs(contact.sin_port));
                sendto(ds, resultat, 10, 0, (struct sockaddr *) &contact, lg);
            } else if (me->puissance < atoi(contenuMsg[1])) {               //CAS OU JE SUIS LE PLUS FAIBLE
                printf("Je suis le plus faible\n");
                if (me->etat != CAPTURE) {                                        //CAS OU JE NE SUIS PAS CAPTURE
                    printf("Je n ai pas de pere\n");
                    char res[3];
                    sprintf(res, "%d", GAGNANT);
                    char resultat[10] = "RE:";
                    strcat(resultat, res);
                    printf("Envoi du message de résultat au site : %s:%d\n", inet_ntoa(contact.sin_addr),
                           ntohs(contact.sin_port));
                    sendto(ds, resultat, 10, 0, (struct sockaddr *) &contact, lg);
                    me->etat = CAPTURE;
                    me->pere = &contact;
                    me->puissance_pere = atoi(contenuMsg[1]) + 1;
                } else {                                                          //CAS OU JE SUIS CAPTURE
                    printf("J ai un pere\n");
                    attacker = &contact;
                    attacker_puissance = atoi(contenuMsg[1]);
                    char *puissance_adversaire = contenuMsg[1];
                    char demande[10] = "DM:";
                    strcat(demande, puissance_adversaire);
                    char *id = contenuMsg[2];
                    strcat(demande, ":");
                    strcat(demande, id);
                    printf("Envoi du message de demande de père au site : %s:%d\n", inet_ntoa(me->pere->sin_addr),
                           ntohs(me->pere->sin_port));
                    sendto(ds, demande, 10, 0, (struct sockaddr *) me->pere, lg);
                }
            } else if (me->id >
                       atoi(contenuMsg[2])) {                      //CAS OU NOTRE PUISSANCE EST EGAL ET QUE J'AI UN ID PLUS GRAND
                printf("J ai la meme puissance, mais mon Id est plus grand\n");
                char res[3];
                sprintf(res, "%d", PERDANT);
                char resultat[10] = "RE:";
                strcat(resultat, res);
                printf("Envoi du message de résultat au site : %s:%d\n", inet_ntoa(contact.sin_addr),
                       ntohs(contact.sin_port));
                sendto(ds, resultat, 10, 0, (struct sockaddr *) &contact, lg);
            } else {                                                            //CAS OU NOTRE PUISSANCE EST EGAL ET QUE J'AI UN ID PLUS PETIT
                printf("J ai la meme puissance, mais mon Id est plus petit\n");
                if (me->etat != CAPTURE) {
                    printf("Je n ai pas de pere\n");
                    char res[3];
                    sprintf(res, "%d", GAGNANT);
                    char resultat[10] = "RE:";
                    strcat(resultat, res);
                    printf("Envoi du message de résultat au site : %s:%d\n", inet_ntoa(contact.sin_addr),
                           ntohs(contact.sin_port));
                    sendto(ds, resultat, 10, 0, (struct sockaddr *) &contact, lg);
                    me->etat = CAPTURE;
                    me->pere = &contact;
                    me->puissance_pere = atoi(contenuMsg[2]) + 1;
                } else {
                    printf("J ai un pere\n");
                    attacker = &contact;
                    attacker_puissance = atoi(contenuMsg[1]);
                    char *puissance_adversaire = contenuMsg[1];
                    char demande[10] = "DM:";
                    char *id = contenuMsg[2];
                    strcat(demande, puissance_adversaire);
                    strcat(demande, ":");
                    strcat(demande, id);
                    printf("Envoi du message de demande de père au site : %s:%d\n", inet_ntoa(me->pere->sin_addr),
                           ntohs(me->pere->sin_port));
                    sendto(ds, demande, 10, 0, (struct sockaddr *) me->pere, lg);
                }
            }
        }


        if (strcmp(contenuMsg[0], "DM") == 0) {                                   //MESSAGE DEMANDE DE PUISSANCE RECU
            printf("Reception d'une demande de puissance\n");
            if (me->puissance > atoi(contenuMsg[1])) {                      //CAS OU JE SUIS LE PLUS FORT
                char res[3];
                sprintf(res, "%d", PERDANT);
                char resultat[10] = "RT:";
                strcat(resultat, res);
                printf("Envoi du message de retour puissance au site : %s:%d\n", inet_ntoa(contact.sin_addr),
                       ntohs(contact.sin_port));
                sendto(ds, resultat, 10, 0, (struct sockaddr *) &contact, lg);
            } else if (me->puissance < atoi(contenuMsg[1])) {               //CAS OU JE SUIS LE PLUS FAIBLE
                char res[3];
                sprintf(res, "%d", GAGNANT);
                char resultat[10] = "RT:";
                strcat(resultat, res);
                printf("Envoi du message de retour puissance au site : %s:%d\n", inet_ntoa(contact.sin_addr),
                       ntohs(contact.sin_port));
                sendto(ds, resultat, 10, 0, (struct sockaddr *) &contact, lg);
                me->etat = PASSIF;

            }
                //estampille dans le cas de puissances egales
            else if (me->id >
                     atoi(contenuMsg[2])) {                        //CAS OU NOTRE PUISSANCE EST EGAL ET QUE J'AI UN ID PLUS GRAND
                char res[3];
                sprintf(res, "%d", PERDANT);
                char resultat[10] = "RT:";
                strcat(resultat, res);
                printf("Envoi du message de retour puissance au site : %s:%d\n", inet_ntoa(contact.sin_addr),
                       ntohs(contact.sin_port));
                sendto(ds, resultat, 10, 0, (struct sockaddr *) &contact, lg);
            } else {                                                              //CAS OU NOTRE PUISSANCE EST EGAL ET QUE J'AI UN ID PLUS PETIT
                char res[3];
                sprintf(res, "%d", GAGNANT);
                char resultat[10] = "RT:";
                strcat(resultat, res);
                printf("Envoi du message de résultat au site : %s:%d\n", inet_ntoa(contact.sin_addr),
                       ntohs(contact.sin_port));
                sendto(ds, resultat, 10, 0, (struct sockaddr *) &contact, lg);
                me->etat = PASSIF;
            }

        }
        if (strcmp(contenuMsg[0], "RT") ==
            0) {                                   //MESSAGE REPONSE DEMANDE DE PUISSANCE RECU
            printf("Reception d'un retour de demande de puissance\n");
            if (atoi(contenuMsg[1]) == GAGNANT) {                          //CAS OU LE PERE A PERDU
                me->pere = attacker;
                me->puissance_pere = attacker_puissance + 1;
                char res[3];
                sprintf(res, "%d", GAGNANT);
                char resultat[10] = "RE:";
                strcat(resultat, res);
                printf("Envoi du message de résultat au site : %s:%d\n", inet_ntoa(me->pere->sin_addr),
                       ntohs(me->pere->sin_port));
                sendto(ds, resultat, 10, 0, (struct sockaddr *) &attacker, lg);
            } else {                                                             //CAS OU LE PERE A GAGNE
                char res[3];
                sprintf(res, "%d", PERDANT);
                char resultat[10] = "RE:";
                strcat(resultat, res);
                printf("Envoi du message de résultat au site : %s:%d\n", inet_ntoa(contact.sin_addr),
                       ntohs(contact.sin_port));
                sendto(ds, resultat, 10, 0, (struct sockaddr *) &attacker, lg);
            }

        }

        if (strcmp(contenuMsg[0], "RE") == 0) {                                   //MESSAGE RESULTAT RECU
            resRecu = true;
            printf("Reception d'un resultat\n");
            if (atoi(contenuMsg[1]) == GAGNANT) {                           //CAS OU LE SITE A GAGNE
                printf("Resultat : GAGNANT\n");
                me->puissance = me->puissance + 1;
                char ipCaptured[15];
                strcpy(ipCaptured, inet_ntoa(contact.sin_addr));;
                int portCaptured = ntohs(contact.sin_port);
                removeAddrServer(addrServer, ipCaptured, portCaptured,
                                 &nbSiteAttack); // Supprimer le site capturé de la liste des site à attaquer
            } else {                                                              //CAS OU LE SITE A PERDU
                printf("Resultat : PERDANT\n");
                me->etat = PASSIF;
            }

        }

        printf("Puissance : %d\n", me->puissance);
        if (me->puissance > (nbSites / 2)) {                                       //CAS OU J'AI GAGNE
            printf("Nombre de sites : %d\n", nbSites);
            printf("Nombre de sites à attaquer: %d\n", nbSiteAttack);
            char res[3];
            sprintf(res, "%d", GAGNANT);
            char resultat[10] = "VI:";
            strcat(resultat, res);

            for (int j = 0; j < nbSites-1; ++j) {
                printf("Envoi du message au site : %d\n", j);
                printf("Envoi au site : %s:%d\n", inet_ntoa(me->liste_IP[j].sin_addr),
                       ntohs((me->liste_IP)[j].sin_port));
                sendto(ds, resultat, 10, 0, (struct sockaddr *) &me->liste_IP[j], lg);
            }
            printf("Tout les messages de victoires envoyés \n");

        }
        if (strcmp(contenuMsg[0], "VI") == 0) {                                   //MESSAGE VICTOIRE RECU
            printf("Reception d'un message de victoire\n");
            victoire = true;
            printf("Victoire du site %s:%d\n", inet_ntoa(contact.sin_addr), ntohs(contact.sin_port));
        }

    }

    if (me->puissance > (nbSites / 2)) {
        printf("Victoire du site %s:%d\n", myIP, myPort);
    }
    printf("La partie est terminée\n");


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