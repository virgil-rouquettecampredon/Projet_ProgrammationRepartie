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
#include "site.h"




int main(int argc, const char *argv[]) {

    //Adresse NULL
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

    //Adresse du serveur central
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

    //Création du socket en UDP
    ds = socket(PF_INET, SOCK_DGRAM, 0);
    if (ds < 0) {
        perror("Client : erreur creation socket UDP");
        exit(1);
    }

    //Creation du bind UDP
    if (bind(ds, (struct sockaddr *) &selfAddr, sizeof(selfAddr)) < 0) {
        perror("Client : erreur bind");
        close(ds);
        exit(1);
    }

    //Nombre de site à attaquer
    int nbSiteAttack = nbSites;
    //suppression de nous meme dans la liste des sites à attaquer
    removeAddrServer(addrServer, myIP, myPort, &nbSiteAttack);
    for (int i = 0; i < nbSiteAttack; i++) {
        printf("Adversaire %d : %s:%d\n", i, inet_ntoa(addrServer[i].sin_addr), ntohs(addrServer[i].sin_port));
    }

    //Initialisation de la structure siteState
    //Qui correspond à nos informations en tant que site
    struct siteState *me = (struct siteState *) malloc(sizeof(struct siteState));
    me = initSiteState(myIP, myPort, addrServer, nbSiteAttack);
    me->id = selfPosition;
    printf("Mon port : %d\n", myPort);

    int victoire = false;
    int resRecu = true; // au debut tout le monde peut attaquer on met resRecu a true dans ce cas là

    struct message *message = (struct message *) malloc(sizeof(struct message));
    struct message *res = (struct message *) malloc(sizeof(struct message));

    //Stockage de l'adresse du dernier site ayant envoyé un message
    struct sockaddr_in contact;

    socklen_t lg = sizeof(struct sockaddr_in);

    sleep(1);

    // Test de sendTo
    int snd;

    //Boucle de jeu
    int z = 0;
    //Tant que personne ne gagne l'election continue
    while (me->puissance < (nbSites / 2) + 1 && !victoire) {
        printf("-----------------------------------------------------\n");
        printf("Début du tour : %d\n", z);
        z++;

        if(me->etat == CAPTURE){
            afficherAttributes(me);
        }

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

            snd = sendto(ds, (struct message *) message, sizeof(struct message), 0, (struct sockaddr *) &cible, sizeof(cible));
            if (snd < 0) {
                perror("Client : erreur sendto");
                close(ds);
                exit(1);
            }
        }

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

        //Si le message reçu est un message d'attaque
        if (res->type_message == AT) {
            if (me->puissance > res->puissance) {                      //CAS OU JE SUIS LE PLUS FORT
                message = creer_message(RE, me->puissance, me->id, PERDANT, null_sockaddr_in);
                printf("Envoi du message de résultat au site : %s:%d\n", inet_ntoa(contact.sin_addr),ntohs(contact.sin_port));

                //Envoi du message de résultat PERDANT au site qui a envoyé l'attaque
                snd = sendto(ds, (struct message *) message, sizeof(struct message), 0, (struct sockaddr *) &contact,lg);
                if (snd < 0) {
                    perror("Client : erreur sendto");
                    close(ds);
                    exit(1);
                }

            } else if (me->puissance < res->puissance) {               //CAS OU JE SUIS LE PLUS FAIBLE
                if (me->etat != CAPTURE) {                             //CAS OU JE NE SUIS PAS CAPTURE
                    message = creer_message(RE, me->puissance, me->id, GAGNANT, null_sockaddr_in);
                    printf("Envoi du message de résultat au site : %s:%d\n", inet_ntoa(contact.sin_addr),ntohs(contact.sin_port));

                    //Je me fais capturer
                    me->etat = CAPTURE;
                    memcpy(me->pere, &contact, sizeof(struct sockaddr_in));

                    //Envoi du message de résultat GAGNANT au site qui a envoyé l'attaque
                    snd = sendto(ds, (struct message *) message, sizeof(struct message), 0,(struct sockaddr *) &contact, lg);
                    if (snd < 0) {
                        perror("Client : erreur sendto");
                        close(ds);
                        exit(1);
                    }

                    printf("Affichage du père : %s:%d\n", inet_ntoa(me->pere->sin_addr), ntohs(me->pere->sin_port));
                    me->puissance_pere = (res->puissance) + 1;
                }
                else {                                                          //CAS OU JE SUIS CAPTURE
                    //Si le père à une puissance inférieure à la puissance de l'attaquant lorsque le site s'est fait capturer
                    if (me->puissance_pere <= res->puissance) {
                        //J'envois une demande de puissance, avec la puissance de l'attaquant, son ID, et son adresse
                        message = creer_message(DM, res->puissance, res->id, -1, contact);
                        printf("Envoi du message de demande de père au site : %s:%d\n", inet_ntoa(me->pere->sin_addr),ntohs(me->pere->sin_port));

                        //Envoie d'une demande de puissance au père qui fera sa comparaison avec la puissance de l'attaquant
                        snd = sendto(ds, (struct message *) message, sizeof(struct message), 0,(struct sockaddr *) me->pere, lg);
                        if (snd < 0) {
                            perror("Client : erreur sendto");
                            close(ds);
                            exit(1);
                        }

                    } else {
                        //Si le père à une puissance supérieure à la puissance de l'attaquant lorsque le site s'est fait capturer
                        message = creer_message(RE, me->puissance, me->id, PERDANT, null_sockaddr_in);
                        printf("Envoi du message de résultat au site : %s:%d\n", inet_ntoa(contact.sin_addr),ntohs(contact.sin_port));

                        //Envoi du message de résultat PERDANT au site qui a envoyé l'attaque
                        snd = sendto(ds, (struct message *) message, sizeof(struct message), 0,(struct sockaddr *) &contact, lg);
                        if (snd < 0) {
                            perror("Client : erreur sendto");
                            close(ds);
                            exit(1);
                        }
                    }
                }
            } else if (me->id > res->id) {                      //CAS OU NOTRE PUISSANCE EST EGAL ET QUE J'AI UN ID PLUS GRAND
                message = creer_message(RE, me->puissance, me->id, PERDANT, null_sockaddr_in);
                printf("Envoi du message de résultat au site : %s:%d\n", inet_ntoa(contact.sin_addr),ntohs(contact.sin_port));

                //Envoi du message de résultat PERDANT au site qui a envoyé l'attaque
                snd = sendto(ds, (struct message *) message, sizeof(struct message), 0, (struct sockaddr *) &contact,lg);
                if (snd < 0) {
                    perror("Client : erreur sendto");
                    close(ds);
                    exit(1);
                }

            } else {                                                            //CAS OU NOTRE PUISSANCE EST EGAL ET QUE J'AI UN ID PLUS PETIT
                if (me->etat != CAPTURE) {
                    message = creer_message(RE, me->puissance, me->id, GAGNANT, null_sockaddr_in);
                    printf("Envoi du message de résultat au site : %s:%d\n", inet_ntoa(contact.sin_addr),ntohs(contact.sin_port));

                    //Je me fais capturer
                    me->etat = CAPTURE;
                    memcpy(me->pere, &contact, sizeof(struct sockaddr_in));

                    //Envoi du message de résultat GAGNANT au site qui a envoyé l'attaque
                    snd = sendto(ds, (struct message *) message, sizeof(struct message), 0,(struct sockaddr *) &contact, lg);

                    if (snd < 0) {
                        perror("Client : erreur sendto");
                        close(ds);
                        exit(1);
                    }

                    printf("Affichage du père : %s:%d\n", inet_ntoa(me->pere->sin_addr), ntohs(me->pere->sin_port));
                    me->puissance_pere = (res->puissance) + 1;

                } else {
                    //J'envois une demande de puissance, avec la puissance de l'attaquant, son ID, et son adresse
                    if (me->puissance_pere <= res->puissance) {
                        message = creer_message(DM, res->puissance, res->id, -1, contact);
                        printf("Envoi du message de demande de père au site : %s:%d\n", inet_ntoa(me->pere->sin_addr),ntohs(me->pere->sin_port));

                        //Envoie d'une demande de puissance au père qui fera sa comparaison avec la puissance de l'attaquant
                        snd = sendto(ds, (struct message *) message, sizeof(struct message), 0,(struct sockaddr *) me->pere, lg);
                        if (snd < 0) {
                            perror("Client : erreur sendto");
                            close(ds);
                            exit(1);
                        }

                    } else {
                        message = creer_message(RE, me->puissance, me->id, PERDANT, null_sockaddr_in);
                        printf("Envoi du message de résultat au site : %s:%d\n", inet_ntoa(contact.sin_addr),ntohs(contact.sin_port));

                        //Envoi du message de résultat PERDANT au site qui a envoyé l'attaque
                        snd = sendto(ds, (struct message *) message, sizeof(struct message), 0,(struct sockaddr *) &contact, lg);
                        if (snd < 0) {
                            perror("Client : erreur sendto");
                            close(ds);
                            exit(1);
                        }
                    }
                }
            }
        }

        //Si le message est de type DM, alors on envoie un message de RT au fils
        if (res->type_message == DM) {                                   //MESSAGE DEMANDE DE PUISSANCE RECU
            if (me->puissance > res->puissance) {                      //CAS OU JE SUIS LE PLUS FORT
                message = creer_message(RT, -1, me->id, PERDANT, res->attaquant);
                printf("Envoi du message de retour puissance au site : %s:%d\n", inet_ntoa(contact.sin_addr),
                       ntohs(contact.sin_port));

                //Envoi du message de résultat au fils
                snd = sendto(ds, (struct message *) message, sizeof(struct message), 0, (struct sockaddr *) &contact,lg);
                if (snd < 0) {
                    perror("Client : erreur sendto");
                    close(ds);
                    exit(1);
                }

            } else if (me->puissance < res->puissance) {               //CAS OU JE SUIS LE PLUS FAIBLE
                message = creer_message(RT, res->puissance, me->id, GAGNANT, res->attaquant);
                printf("Envoi du message de retour puissance au site : %s:%d\n", inet_ntoa(contact.sin_addr),
                       ntohs(contact.sin_port));

                //Envoi du message de RT au fils
                snd = sendto(ds, (struct message *) message, sizeof(struct message), 0, (struct sockaddr *) &contact,lg);
                if (snd < 0) {
                    perror("Client : erreur sendto");
                    close(ds);
                    exit(1);
                }

                //Je deviens passif si je ne suis pas capturé
                if(me->etat != CAPTURE){
                    me->etat = PASSIF;
                }
            }
            //estampille dans le cas de puissances egales
            else if (me->id >res->id) {                        //CAS OU NOTRE PUISSANCE EST EGAL ET QUE J'AI UN ID PLUS GRAND
                message = creer_message(RT, -1, me->id, PERDANT, res->attaquant);
                printf("Envoi du message de retour puissance au site : %s:%d\n", inet_ntoa(contact.sin_addr),ntohs(contact.sin_port));

                //Envoi du message de RT au fils
                snd = sendto(ds, (struct message *) message, sizeof(struct message), 0, (struct sockaddr *) &contact,lg);
                if (snd < 0) {
                    perror("Client : erreur sendto");
                    close(ds);
                    exit(1);
                }

            } else {                                                              //CAS OU NOTRE PUISSANCE EST EGAL ET QUE J'AI UN ID PLUS PETIT
                message = creer_message(RT, res->puissance, me->id, GAGNANT, res->attaquant);
                printf("Envoi du message de résultat au site : %s:%d\n", inet_ntoa(contact.sin_addr),ntohs(contact.sin_port));

                //Envoi du message de RT au fils
                snd = sendto(ds, (struct message *) message, sizeof(struct message), 0, (struct sockaddr *) &contact,lg);
                if (snd < 0) {
                    perror("Client : erreur sendto");
                    close(ds);
                    exit(1);
                }

                //Je deviens passif si je ne suis pas capturé
                if(me->etat != CAPTURE){
                    me->etat = PASSIF;
                }
            }

        }

        if (res->type_message == RT) {                                   //MESSAGE REPONSE DEMANDE DE PUISSANCE RECU
            if (res->type_resultat == GAGNANT) {                          //CAS OU LE PERE A PERDU
                memcpy(me->pere, &res->attaquant, sizeof(struct sockaddr_in));
                me->puissance_pere = (res->puissance) + 1;
                message = creer_message(RE, res->puissance, me->id, GAGNANT, null_sockaddr_in);
                printf("Envoi du message de résultat au site : %s:%d\n", inet_ntoa(res->attaquant.sin_addr),ntohs(res->attaquant.sin_port));

                //Envoi du resultat GAGNANT a l'attaquant
                snd = sendto(ds, (struct message *) message, sizeof(struct message), 0,(struct sockaddr *) &res->attaquant, lg);
                if (snd < 0) {
                    perror("Client : erreur sendto");
                    close(ds);
                    exit(1);
                }

            } else {                                                             //CAS OU LE PERE A GAGNE
                message = creer_message(RE, res->puissance, me->id, PERDANT, null_sockaddr_in);
                printf("Envoi du message de résultat au site : %s:%d\n", inet_ntoa(res->attaquant.sin_addr),ntohs(res->attaquant.sin_port));

                //Envoi du resultat PERDANT a l'attaquant
                snd = sendto(ds, (struct message *) message, sizeof(struct message), 0,(struct sockaddr *) &res->attaquant, lg);
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
                removeAddrServer(addrServer, ipCaptured, portCaptured,&nbSiteAttack); // Supprimer le site capturé de la liste des site à attaquer
            } else {                                                              //CAS OU LE SITE A PERDU
                if(me->etat != CAPTURE){
                    me->etat = PASSIF;
                }
                printf("Resultat : PERDANT\n");
            }
        }

        if (me->puissance > (nbSites / 2)) {                                       //CAS OU J'AI GAGNE
            message = creer_message(VI, -1, -1, GAGNANT, null_sockaddr_in);

            //Envoie d'un message de victoire a tous les sites
            for (int j = 0; j < nbSites - 1; ++j) {
                printf("Envoi du message au site : %d\n", j);
                printf("Envoi au site : %s:%d\n", inet_ntoa(me->liste_IP[j].sin_addr),ntohs((me->liste_IP)[j].sin_port));

                snd = sendto(ds, (struct message *) message, sizeof(struct message), 0,(struct sockaddr *) &me->liste_IP[j], lg);
                if (snd < 0) {
                    perror("Client : erreur sendto");
                    close(ds);
                    exit(1);
                }

            }
            printf("Tout les messages de victoires envoyés \n");

        }
        if (res->type_message == VI) {                                   //MESSAGE VICTOIRE RECU
            // AFFICHE QUE LE SITE X A GAGNE
            printf("Reception d'un message de victoire\n");
            victoire = true;
            printf("Victoire du site %s:%d\n", inet_ntoa(contact.sin_addr), ntohs(contact.sin_port));
        }

    }
    // AFFICHE QUE LE SITE X A GAGNE
    if (me->puissance > (nbSites / 2)) {
        printf("Victoire du site %s:%d\n", myIP, myPort);
    }
    printf("La partie est terminée\n");


    char test[20];
    printf("Fin du programme, appuyez sur une touché puis entrée : ");
    scanf("%s", test);

    close(ds);
    free(message);
    free(me);
    free(res);

    return 0;
}