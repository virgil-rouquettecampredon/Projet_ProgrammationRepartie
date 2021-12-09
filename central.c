#include <stdio.h>
#include <sys/types.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <sys/time.h>
#include <sys/select.h>
#include "calcul.h"

//84.103.70.123 33333

#define MAX_BUFFER_SIZE 16000 // taille du buffer qui me permet de récupérer le contenu du fichier à recevoir bloc par bloc. Vous pouvez changer cette valeur.

int sendTCP(int socket, const char * buffer, size_t length){

    ssize_t sent;

    while (length > 0){
        sent = send(socket, buffer, length, 0);

        if(sent <= 0){
            return sent;
        }

        buffer += sent;
        length -= sent;
    }
    return 1;
}

int recvTCP(int socket, char *buffer, size_t length){
    ssize_t received;

    while (length > 0){
        received = recv(socket, buffer, length, 0);

        if(received <= 0){
            return received;
        }

        buffer += received;
        length -= received;
    }
    return 1;
}

int main(int argc, char *argv[])
{

    if (argc<3){
        printf("Utilisation : %s numero_port nombreDeSite\n", argv[0]);
        exit(1);
    }

    int nombreDeSite = atoi(argv[2]);
    int ds = socket(PF_INET, SOCK_STREAM, 0);

    if (ds < 0) {
        perror("Serveur : erreur creation socket");
        exit(1);
    }

    struct sockaddr_in server;
    server.sin_family = AF_INET;
    server.sin_addr.s_addr =INADDR_ANY;
    server.sin_port = htons( (short) atoi (argv[1]));

    if(bind(ds, (struct sockaddr *) &server, sizeof(server)) < 0){
        perror("Serveur : erreur bind");
        close(ds);
        exit(1);
    }

    int ecoute = listen(ds, nombreDeSite);
    if (ecoute < 0){
        perror("Serveur : erreur ecoute");
        close(ds);
        exit(1);
    }

    int liste_client[nombreDeSite];
    struct sockaddr_in clients[nombreDeSite];

    /* boucle pour le traitement itératif des clients */
    int i = nombreDeSite;


    //xxx.xxx.xxx.xxx:yyyyy$ taille ip = 21 char
    int sizeClient = 22*nombreDeSite-1;
    char allClient[sizeClient];

    while(i != 0){
        printf("Serveur : j'attends la demande d'un client (accept) \n");

        struct sockaddr_in adc;
        socklen_t lgc = sizeof (struct sockaddr_in);

        int dsc = accept(ds, (struct sockaddr *)& adc, &lgc);
        if (dsc < 0){
            perror("Serveur : erreur accept");
            exit(1);
        }

        printf("Serveur : le client %s:%d est connecté\n", inet_ntoa(adc.sin_addr) , ntohs(adc.sin_port));


        liste_client[i-1] = dsc;

        //convert short unsigned int to string
        char port[6];

        strcat(allClient, inet_ntoa(adc.sin_addr));
        strcat(allClient, ":");
        sprintf(port, "%d", ntohs(adc.sin_port));
        strcat(allClient, port);
        strcat(allClient, "$");

        i--;
        //je passe au client suivant.
    }



    //send to all clients in list_client the number of clients and list_client
    for (int i = 0; i < nombreDeSite; ++i) {
        int dsic = liste_client[i];
        printf("Etat socket client %d avant premier send : %d\n", i, liste_client[i]);
        int snd = send(liste_client[i],(char*) &nombreDeSite, sizeof(int), 0);
        if(snd<0){
            perror("Client : erreur lors du send:");
            //TODO : FAUDRA FERMER TOUT LES SOCKETS
            close(ds);
            close(liste_client[i]);
            exit(1);
        }

        if(snd==0){
            printf("Client : serveur deconnecte\n");
            close(liste_client[i]);
            close(ds);
            exit(1);
        }

        printf("Etat socket client %d après premier send : %d\n", i, liste_client[i]);

        printf("Taille du buffer : %d\n", sizeClient);
        printf("Contenu du buffer : %s\n", allClient);
        snd = send(liste_client[i], allClient, sizeClient, 0);
        printf("Etat socket client %d avant second send : %d\n", i, liste_client[i]);
        if(snd<0){
            perror("Client : erreur lors du send:");
            close(ds);
            close(liste_client[i]);
            exit(1);
        }

        if(snd==0){
            printf("Client : serveur deconnecte\n");
            close(ds);
            close(liste_client[i]);
            exit(1);
        }
        //close(liste_client[i]);
        calcul(5);

    }

    //close (ds); // atteignable si on sort de la boucle infinie.
    printf("Serveur : je termine\n");
}