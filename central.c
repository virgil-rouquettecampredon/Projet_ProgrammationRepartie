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

//84.103.70.123 33333

#define MAX_BUFFER_SIZE 16000 // taille du buffer qui me permet de récupérer le contenu du fichier à recevoir bloc par bloc. Vous pouvez changer cette valeur.

int sendTCP(int socket, const char * buffer, size_t length, unsigned int *nbBytesSent, unsigned int * nbCallSend){

    ssize_t sent;

    while (length > 0){
        sent = send(socket, buffer, length, 0);

        if(sent <= 0){
            return sent;
        }

        buffer += sent;
        length -= sent;

        (*nbBytesSent) += sent;
        (*nbCallSend)++;
    }
    return 1;
}

int recvTCP(int socket, char *buffer, size_t length, unsigned int *nbBytesReceved, unsigned int * nbCallRecv){
    ssize_t received;

    while (length > 0){
        received = recv(socket, buffer, length, 0);

        if(received <= 0){
            return received;
        }

        buffer += received;
        length -= received;

        (*nbBytesReceved) += received;
        (*nbCallRecv)++;
    }
    return 1;
}

int main(int argc, char *argv[])
{

    if (argc<2){
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

    struct sockaddr_in liste_client[nombreDeSite];

    /* boucle pour le traitement itératif des clients */
    while(nombreDeSite != 0){
        printf("Serveur : j'attends la demande d'un client (accept) \n");

        struct sockaddr_in adc;
        socklen_t lgc = sizeof (struct sockaddr_in);

        int dsc = accept(ds, (struct sockaddr *)& adc, &lgc);
        if (dsc < 0){
            perror("Serveur : erreur accept");
            exit(1);
        }

        printf("Serveur : le client %s:%d est connecté\n", inet_ntoa(adc.sin_addr) , ntohs(adc.sin_port));

        liste_client[nombreDeSite-1] = adc;

        nombreDeSite--;
        //je passe au client suivant.
        close(dsc);
    }
    int lgAdr = sizeof(struct sockaddr_in);

    char* allClient;
    allClient = malloc(lgAdr*nombreDeSite);
    for (int i = 0; i < nombreDeSite; ++i) {
        strcat_s(allClient, lgAdr, inet_ntoa(liste_client[i].sin_addr));
        strcat_s(allClient, lgAdr, ":");
        strcat_s(allClient, lgAdr, ntohs(addrServer[i].sin_port));
        strcat_s(allClient, lgAdr, "\n");
    }

    //send to all clients in list_client the number of clients and list_client
    for (int i = 0; i < nombreDeSite; ++i) {
        int conn = connect(ds, (struct sockaddr *)&liste_client[i], lgAdr);
        if (conn < 0){
            perror("Serveur : erreur connect");
            exit(1);
        }
        snd = sendTCP(ds, allClient, sizeof(allClient), &nbTotalOctetsEnvoyes, &nbAppelSend);
        if(snd<0){
            perror("Client : erreur lors du send:");
            free(filepath);
            close(ds);
            exit(1);
        }

        if(snd==0){
            printf("Client : serveur deconnecte\n");
            free(filepath);
            close(ds);
            exit(1);
        }

    }

    close (ds); // atteignable si on sort de la boucle infinie.
    printf("Serveur : je termine\n");
}