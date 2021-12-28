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

struct message* creer_message(int type_message, int puissance, int id, int type_resultat, struct sockaddr_in attaquant) {
    //Wait for 100ms-200ms
    usleep(rand() % 100001 + 100000);

    struct message *msg = malloc(sizeof (struct message));
    msg->type_message = type_message;
    msg->puissance = puissance;
    msg->id = id;
    msg->type_resultat = type_resultat;
    msg->attaquant = attaquant;
    return msg;
}

void afficher_message(struct message *msg) {
    if(msg->type_message == AT) {
        printf("|ATTAQUE\n");
    } else if(msg->type_message == RE) {
        printf("|RESULTAT\n");
    } else if(msg->type_message == DM) {
        printf("|DEMANDE PUISSANCE PERE\n");
    } else if(msg->type_message == RT) {
        printf("|RETOUR PUISSANCE PERE\n");
    } else if(msg->type_message == VI) {
        printf("|VICTOIRE\n");
    }
    printf("|Puissance : %d\n", msg->puissance);
    printf("|ID : %d\n", msg->id);
    if(msg->type_resultat == GAGNANT) {
        printf("|Résultat : GAGNANT\n");
    } else if(msg->type_resultat == PERDANT) {
        printf("|Résultat : PERDANT\n");
    }
    //Check if msg->attaquant is not equal to sockaddr_in null
    if(msg->attaquant.sin_addr.s_addr != null_sockaddr_in.sin_addr.s_addr) {
        printf("|Attaquant : %s:%d\n", inet_ntoa(msg->attaquant.sin_addr), ntohs(msg->attaquant.sin_port));
    }
}

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

int main(int argc, const char *argv[]) {
    null_sockaddr_in.sin_addr.s_addr = inet_addr("255.255.255.255");
    null_sockaddr_in.sin_family = AF_INET;
    null_sockaddr_in.sin_port = htons((short) 65535);

    // Test if there are 2 arguments
    if (argc != 3) {
        printf("Utilisation: %s addresse_IP_du_serveur_central  numero_de_port_du_serveur_central\n", argv[0]);
        exit(1);
    }
    srand(time(NULL));

    //Connexion au serveur central
    int ds = socket(PF_INET, SOCK_STREAM, 0);
    if (ds < 0) {
        perror("Client : erreur creation socket");
        exit(1);
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

    //Find position of selfAddr in addrServer (own position)
    int selfPosition = -1;
    int i = 0;
    while (i < nbSites && selfPosition == -1) {
        if (strcmp(myIP, inet_ntoa(addrServer[i].sin_addr)) == 0 && myPort == ntohs(addrServer[i].sin_port)) {
            selfPosition = i;
        }
        i++;
    }

    //Fermeture du socket en TCP pour le transformer en UDP
    close(ds);

    ds = socket(PF_INET, SOCK_DGRAM, 0);
    if (ds < 0) {
        perror("Client : erreur creation socket UDP");
        exit(1);
    }

    if (bind(ds, (struct sockaddr *) &selfAddr, sizeof(selfAddr)) < 0) {
        perror("Client : erreur bind");
        close(ds);
        exit(1);
    }

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

    struct message* message = (struct message *) malloc(sizeof(struct message));
    struct message* res = (struct message*) malloc(sizeof(struct message));

    //Stockage de l'adresse du dernier site ayant envoyé un message
    struct sockaddr_in contact;

    socklen_t lg = sizeof(struct sockaddr_in);

    sleep(1);

    int snd;

    int z = 0;
    while (me->puissance < (nbSites / 2) + 1 && !victoire) {
        printf("-----------------------------------------------------\n");
        printf("Début du tour : %d\n", z);
        z++;

        //me est attaquant dans cette conditon ( le cas ou x envoie un message d'attaque)
        if (me->etat == PARTICIPANT && resRecu) {
            resRecu = false;
            //Sélection d'une cible aléatoire
            int y = rand() % nbSiteAttack;
            printf("Le numéro random sélectionné est : %d\n", y);
            struct sockaddr_in cible = addrServer[y];

            //Création du message d'attaque
            message = creer_message(AT, me->puissance, me->id, -1, null_sockaddr_in);

            //Envoi du message d'attaque à la cible
            printf("Envoi de l'attaque au site %s:%d\n", inet_ntoa(addrServer[y].sin_addr), ntohs(addrServer[y].sin_port));
            //calcul(1);
            snd = sendto(ds, (struct message*) message, sizeof(struct message), 0, (struct sockaddr *) &cible, sizeof(cible));
            //calcul(1);
            if (snd < 0) {
                perror("Client : erreur sendto");
                close(ds);
                exit(1);
            }
        }

        printf("MON ID EST LE SUIVANT: %i\n", me->id);

        //On attends de recevoir un message d'un autre site
        printf("Attente d'un message...\n");
        int receive = recvfrom(ds, res, sizeof(struct message), 0, (struct sockaddr *) &contact, &lg);
        if (receive < 0) {
            perror("Client : erreur recvfrom");
            close(ds);
            exit(1);
        }
        printf("Message reçu de : %s:%d\n", inet_ntoa(contact.sin_addr), ntohs(contact.sin_port));
        afficher_message(res);

        if (res->type_message==AT) {
            if (me->puissance > res->puissance) {                      //CAS OU JE SUIS LE PLUS FORT
                message = creer_message(RE, me->puissance, me->id, PERDANT, null_sockaddr_in);
                printf("Envoi du message de résultat au site : %s:%d\n", inet_ntoa(contact.sin_addr), ntohs(contact.sin_port));
                //calcul(1);
                printf("CAS OU JE SUIS LE PLUS FORT\n");
                snd = sendto(ds, (struct message*)message, sizeof(struct message), 0, (struct sockaddr *) &contact, lg);
                if (snd < 0) {
                    perror("Client : erreur sendto");
                    close(ds);
                    exit(1);
                }
            } else if (me->puissance < res->puissance) {               //CAS OU JE SUIS LE PLUS FAIBLE
                if (me->etat != CAPTURE) {                                        //CAS OU JE NE SUIS PAS CAPTURE
                    printf("CAS OU JE SUIS LE PLUS FAIBLE ET PAS CAPTURE\n");
                    message = creer_message(RE, me->puissance, me->id, GAGNANT, null_sockaddr_in);
                    printf("Envoi du message de résultat au site : %s:%d\n", inet_ntoa(contact.sin_addr), ntohs(contact.sin_port));
                    //calcul(1);
                    me->etat = CAPTURE;
                    me->pere = &contact;
                    snd = sendto(ds, (struct message*)message, sizeof(struct message), 0, (struct sockaddr *) &contact, lg);
                    if (snd < 0) {
                        perror("Client : erreur sendto");
                        close(ds);
                        exit(1);
                    }
                    printf("Affichage du père : %s:%d\n", inet_ntoa(me->pere->sin_addr), ntohs(me->pere->sin_port));
                    me->puissance_pere = (res->puissance) + 1;
                } else {                                                          //CAS OU JE SUIS CAPTURE
                    //J'envois une demande de puissance, avec la puissance de l'attaquant, son ID, et son adresse
                    printf("CAS OU JE SUIS LE PLUS FAIBLE ET CAPTURE \n");
                    if (me->puissance_pere < res->puissance){
                        printf("ID PERE < \n");
                        message = creer_message(DM, res->puissance, res->id, -1, contact);
                        printf("Envoi du message de demande de père au site : %s:%d\n", inet_ntoa(me->pere->sin_addr), ntohs(me->pere->sin_port));

                        //calcul(1);
                        snd = sendto(ds, (struct message*)message, sizeof(struct message), 0, (struct sockaddr *) me->pere, lg);
                        if (snd < 0) {
                            perror("Client : erreur sendto");
                            close(ds);
                            exit(1);
                        }
                    }
                    else{
                        printf("ID PERE > \n");
                        message = creer_message(RE, me->puissance, me->id, PERDANT, null_sockaddr_in);
                        printf("Envoi du message de résultat au site : %s:%d\n", inet_ntoa(contact.sin_addr), ntohs(contact.sin_port));
                        //calcul(1);
                        snd = sendto(ds, (struct message*)message, sizeof(struct message), 0, (struct sockaddr *) &contact, lg);
                        if (snd < 0) {
                            perror("Client : erreur sendto");
                            close(ds);
                            exit(1);
                        }
                    }
                    
                    
                    
                }
            } else if (me->id > res->id) {                      //CAS OU NOTRE PUISSANCE EST EGAL ET QUE J'AI UN ID PLUS GRAND
                printf("CAS OU NOTRE PUISSANCE EST EGAL ET QUE J'AI UN ID PLUS GRAND\n");
                message = creer_message(RE, me->puissance, me->id, PERDANT, null_sockaddr_in);
                printf("Envoi du message de résultat au site : %s:%d\n", inet_ntoa(contact.sin_addr), ntohs(contact.sin_port));
                //calcul(1);
                snd = sendto(ds, (struct message*)message, sizeof(struct message), 0, (struct sockaddr *) &contact, lg);
                if (snd < 0) {
                    perror("Client : erreur sendto");
                    close(ds);
                    exit(1);
                }
            } else {                                                            //CAS OU NOTRE PUISSANCE EST EGAL ET QUE J'AI UN ID PLUS PETIT
                if (me->etat != CAPTURE) {
                    printf("CAS OU NOTRE PUISSANCE EST EGAL ET QUE J'AI UN ID PLUS PETIT ET PAS CAPTURE\n");
                    printf("ME->ID :%i\n", me->id);
                    message = creer_message(RE, me->puissance, me->id, GAGNANT, null_sockaddr_in);
                    printf("Envoi du message de résultat au site : %s:%d\n", inet_ntoa(contact.sin_addr), ntohs(contact.sin_port));
                    //calcul(1);
                    me->etat = CAPTURE;
                    me->pere = &contact;
                    snd = sendto(ds, (struct message*)message, sizeof(struct message), 0, (struct sockaddr *) &contact, lg);
                    if (snd < 0) {
                        perror("Client : erreur sendto");
                        close(ds);
                        exit(1);
                    }
                    printf("Affichage du père : %s:%d\n", inet_ntoa(me->pere->sin_addr), ntohs(me->pere->sin_port));
                    me->puissance_pere = (res->puissance) + 1;
                } 
                else {
                    //J'envois une demande de puissance, avec la puissance de l'attaquant, son ID, et son adresse
                    printf("CAS OU NOTRE PUISSANCE EST EGAL ET QUE J'AI UN ID PLUS PETIT ET CAPTURE\n");
                    if (me->puissance_pere < res->puissance){
                        message = creer_message(DM, res->puissance, res->id, -1, contact);
                        printf("Envoi du message de demande de père au site : %s:%d\n", inet_ntoa(me->pere->sin_addr), ntohs(me->pere->sin_port));

                        //calcul(1);
                        snd = sendto(ds, (struct message*)message, sizeof(struct message), 0, (struct sockaddr *) me->pere, lg);
                        if (snd < 0) {
                            perror("Client : erreur sendto");
                            close(ds);
                            exit(1);
                        }
                    }
                    else{
                        message = creer_message(RE, me->puissance, me->id, PERDANT, null_sockaddr_in);
                        printf("Envoi du message de résultat au site : %s:%d\n", inet_ntoa(contact.sin_addr), ntohs(contact.sin_port));
                        //calcul(1);
                        snd = sendto(ds, (struct message*)message, sizeof(struct message), 0, (struct sockaddr *) &contact, lg);
                        if (snd < 0) {
                            perror("Client : erreur sendto");
                            close(ds);
                            exit(1);
                        }
                    }
                }
            }
        }


        if (res->type_message==DM) {                                   //MESSAGE DEMANDE DE PUISSANCE RECU
            if (me->puissance > res->puissance) {                      //CAS OU JE SUIS LE PLUS FORT
                message = creer_message(RT, -1, me->id, PERDANT, res->attaquant);
                printf("Envoi du message de retour puissance au site : %s:%d\n", inet_ntoa(contact.sin_addr), ntohs(contact.sin_port));

                //calcul(1);
                snd = sendto(ds, (struct message*)message, sizeof(struct message), 0, (struct sockaddr *) &contact, lg);
                if (snd < 0) {
                    perror("Client : erreur sendto");
                    close(ds);
                    exit(1);
                }
            } else if (me->puissance < res->puissance) {               //CAS OU JE SUIS LE PLUS FAIBLE
                message = creer_message(RT, res->puissance, me->id, GAGNANT, res->attaquant);
                printf("Envoi du message de retour puissance au site : %s:%d\n", inet_ntoa(contact.sin_addr), ntohs(contact.sin_port));

                //calcul(1);
                snd = sendto(ds, (struct message*)message, sizeof(struct message), 0, (struct sockaddr *) &contact, lg);
                if (snd < 0) {
                    perror("Client : erreur sendto");
                    close(ds);
                    exit(1);
                }
                me->etat = PASSIF;

            }
            //estampille dans le cas de puissances egales
            else if (me->id > res->id) {                        //CAS OU NOTRE PUISSANCE EST EGAL ET QUE J'AI UN ID PLUS GRAND
                message = creer_message(RT, -1, me->id, PERDANT, res->attaquant);
                printf("Envoi du message de retour puissance au site : %s:%d\n", inet_ntoa(contact.sin_addr), ntohs(contact.sin_port));

                //calcul(1);
                snd = sendto(ds, (struct message*)message, sizeof(struct message), 0, (struct sockaddr *) &contact, lg);
                if (snd < 0) {
                    perror("Client : erreur sendto");
                    close(ds);
                    exit(1);
                }
            } else {                                                              //CAS OU NOTRE PUISSANCE EST EGAL ET QUE J'AI UN ID PLUS PETIT
                message = creer_message(RT, res->puissance, me->id, GAGNANT, res->attaquant);
                printf("Envoi du message de résultat au site : %s:%d\n", inet_ntoa(contact.sin_addr), ntohs(contact.sin_port));

                //calcul(1);
                snd = sendto(ds, (struct message*)message, sizeof(struct message), 0, (struct sockaddr *) &contact, lg);
                if (snd < 0) {
                    perror("Client : erreur sendto");
                    close(ds);
                    exit(1);
                }
                me->etat = PASSIF;
            }

        }

        if (res->type_message==RT) {                                   //MESSAGE REPONSE DEMANDE DE PUISSANCE RECU

            if (res->type_resultat == GAGNANT) {                          //CAS OU LE PERE A PERDU
                me->pere = &res->attaquant;
                me->puissance_pere = (res->puissance) + 1;
                message = creer_message(RE, res->puissance, me->id, GAGNANT, null_sockaddr_in);
                printf("Envoi du message de résultat au site : %s:%d\n", inet_ntoa(res->attaquant.sin_addr), ntohs(res->attaquant.sin_port));

                //calcul(1);
                snd = sendto(ds, (struct message*)message, sizeof(struct message), 0, (struct sockaddr *) &res->attaquant, lg);
                if (snd < 0) {
                    perror("Client : erreur sendto");
                    close(ds);
                    exit(1);
                }
            } else {                                                             //CAS OU LE PERE A GAGNE
                message = creer_message(RE, res->puissance, me->id, PERDANT, null_sockaddr_in);
                printf("Envoi du message de résultat au site : %s:%d\n", inet_ntoa(res->attaquant.sin_addr), ntohs(res->attaquant.sin_port));

                //calcul(1);
                snd = sendto(ds, (struct message*)message, sizeof(struct message), 0, (struct sockaddr *) &res->attaquant, lg);
                if (snd < 0) {
                    perror("Client : erreur sendto");
                    close(ds);
                    exit(1);
                }
            }
        }

        if (res->type_message == RE) {                                   //MESSAGE RESULTAT RECU
            resRecu = true;
            if (res->type_resultat == GAGNANT) {                           //CAS OU LE SITE A GAGNE
                me->puissance = me->puissance + 1;
                char ipCaptured[15];
                strcpy(ipCaptured, inet_ntoa(contact.sin_addr));;
                int portCaptured = ntohs(contact.sin_port);
                removeAddrServer(addrServer, ipCaptured, portCaptured, &nbSiteAttack); // Supprimer le site capturé de la liste des site à attaquer
            } else {                                                              //CAS OU LE SITE A PERDU
                printf("Resultat : PERDANT\n");
                me->etat = PASSIF;
            }

        }

        if (me->puissance > (nbSites / 2)) {                                       //CAS OU J'AI GAGNE
            printf("Nombre de sites : %d\n", nbSites);
            printf("Nombre de sites à attaquer: %d\n", nbSiteAttack);
            message = creer_message(VI, -1, -1, GAGNANT, null_sockaddr_in);

            for (int j = 0; j < nbSites-1; ++j) {
                printf("Envoi du message au site : %d\n", j);
                printf("Envoi au site : %s:%d\n", inet_ntoa(me->liste_IP[j].sin_addr), ntohs((me->liste_IP)[j].sin_port));
                //calcul(1);
                snd = sendto(ds, (struct message*)message, sizeof(struct message), 0, (struct sockaddr *) &me->liste_IP[j], lg);
                if (snd < 0) {
                    perror("Client : erreur sendto");
                    close(ds);
                    exit(1);
                }
            }
            printf("Tout les messages de victoires envoyés \n");

        }
        if (res->type_message == VI) {                                   //MESSAGE VICTOIRE RECU
            printf("Reception d'un message de victoire\n");
            victoire = true;
            printf("Victoire du site %s:%d\n", inet_ntoa(contact.sin_addr), ntohs(contact.sin_port));
        }

    }

    if (me->puissance > (nbSites / 2)) {
        printf("Victoire du site %s:%d\n", myIP, myPort);
    }
    printf("La partie est terminée\n");


    char test[20];
    printf("Fin du programme, appuyez sur une touché puis entrée : ");
    scanf("%s", test);
    calcul(1);
    close(ds);

    return 0;
}