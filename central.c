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

//84.103.70.123 33333

#define MAX_BUFFER_SIZE 16000 // taille du buffer qui me permet de récupérer le contenu du fichier à recevoir bloc par bloc. Vous pouvez changer cette valeur.

void fermerSocket(int *listeSocket, int nombreSocket) {
    for (int i = 0; i < nombreSocket; ++i) {
        close(listeSocket[i]);
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

int main(int argc, char *argv[]) {

    if (argc < 3) {
        printf("Utilisation : %s numero_port nombreDeSite\n", argv[0]);
        exit(1);
    }

    int nombreDeSite = atoi(argv[2]);
    int ds = socket(PF_INET, SOCK_STREAM, 0);

    if (ds < 0) {
        perror("Serveur : erreur creation socket");
        exit(1);
    }

    int option = 1;
    setsockopt(ds, SOL_SOCKET, SO_REUSEADDR, &option, sizeof(option));

    struct sockaddr_in server;
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_port = htons((short) atoi(argv[1]));

    if (bind(ds, (struct sockaddr *) &server, sizeof(server)) < 0) {
        perror("Serveur : erreur bind");
        close(ds);
        exit(1);
    }

    int ecoute = listen(ds, nombreDeSite);
    if (ecoute < 0) {
        perror("Serveur : erreur ecoute");
        close(ds);
        exit(1);
    }

    int liste_client[nombreDeSite];
    struct sockaddr_in clients[nombreDeSite];

    /* boucle pour le traitement itératif des clients */
    int i = nombreDeSite;


    //xxx.xxx.xxx.xxx:yyyyy$ taille ip = 21 char
    int sizeClient = 22 * nombreDeSite;
    char* allClient = malloc(sizeof(char) * sizeClient);

    while (i != 0) {
        printf("Serveur : j'attends la demande d'un client (accept) \n");

        struct sockaddr_in adc;
        socklen_t lgc = sizeof(struct sockaddr_in);

        int dsc = accept(ds, (struct sockaddr *) &adc, &lgc);
        if (dsc < 0) {
            perror("Serveur : erreur accept");
            exit(1);
        }

        printf("Serveur : le client %s:%d est connecté\n", inet_ntoa(adc.sin_addr), ntohs(adc.sin_port));


        liste_client[i - 1] = dsc;

        //convert short unsigned int to string
        char port[6];

        strcat(allClient, inet_ntoa(adc.sin_addr));
        strcat(allClient, ":");
        sprintf(port, "%d", ntohs(adc.sin_port));
        strcat(allClient, port);
        if(i!=1) strcat(allClient, "$");

        i--;
        //je passe au client suivant.
    }



    //send to all clients in list_client the number of clients and list_client
    for (int i = 0; i < nombreDeSite; ++i) {
        int snd = sendTCP(liste_client[i], &nombreDeSite, sizeof(int));
        if (snd < 0) {
            perror("Client : erreur lors du send:");
            fermerSocket(liste_client, nombreDeSite);
            close(ds);
            exit(1);
        }

        if (snd == 0) {
            printf("Client : serveur deconnecte\n");
            fermerSocket(liste_client, nombreDeSite);
            close(ds);
            exit(1);
        }

        snd = sendTCP(liste_client[i], (char *) allClient, sizeClient);
        if (snd < 0) {
            perror("Client : erreur lors du send:");
            fermerSocket(liste_client, nombreDeSite);
            close(ds);
            exit(1);
        }

        if (snd == 0) {
            printf("Client : serveur deconnecte\n");
            fermerSocket(liste_client, nombreDeSite);
            close(ds);
            exit(1);
        }
        close(liste_client[i]);
    }

    shutdown(ds, SHUT_RDWR);
//    int len=MAX_BUFFER_SIZE;
//    char buffer[MAX_BUFFER_SIZE];
//    while(len!=0) {
//        len = read(ds, buffer, MAX_BUFFER_SIZE);
//        printf("%s\n", buffer);
//    }
    printf("Close return value : %d\n", close(ds));
    printf("Serveur : je termine\n");
}