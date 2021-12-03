
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include <sys/socket.h>
#include <arpa/inet.h>
//#include "site.h"

//Sabri the sublime

#define GAGNANT 1
#define PERDANT 0
#define PASSIF 200
#define PARTICIPANT 201
#define CAPTURE 202

struct siteState {
    struct sockaddr_in* pere;
    int puissance;
    int puissance_Pere;
    int etat;
    char* ip;
    int port;
    struct sockaddr_in* liste_IP;
};

struct siteState* initSiteState (char* ip, char* port, struct sockaddr_in* liste_IP) {
    struct siteState* site;
    site->pere = (void*)0;
    site->puissance_Pere = 0;
    site->etat = PARTICIPANT;
    site->ip = ip;
    site->port = atoi(port);
    site->liste_IP = liste_IP;

    return site;
}

char** splitIp(char* ip){
    char** ip_array = (char**)malloc(4 * sizeof(char*));
    const char* delimiter = ":";
    strtok(ip, delimiter);
    ip_array[0] = ip;
    ip_array[1] = strtok(NULL, delimiter);

    return ip_array;
}

struct sockaddr_in* initAddrServer(const char* emplacementFichier, int* y){
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
}

//remove one specified sockaddr_in from addrServer
void removeAddrServer(struct sockaddr_in* addrServerOpponent, char** ip, int* y){
    int i = 0;
    while(((strcmp(inet_ntoa(addrServerOpponent[i].sin_addr), ip[0]) != 0)||(ntohs(addrServerOpponent[i].sin_port) != atoi(ip[1])))&& (i < *y)){
        i++;
    }
    if(i < *y){
        for(int j = i; j < *y - 1; j++){
            addrServerOpponent[j] = addrServerOpponent[j + 1];
        }
        *y = *y - 1;
    }

}

int main(int argc, const char * argv[]){

    //test if there are 3 arguments
    if(argc != 3){
        printf("Utilisation: addresseDuSite : %s EmplacementFichierDesAddressesDeTousLesSites : %s\n", argv[1], argv[2]);
        exit(1);
    }

    //split argv[1] with splitIp
    char** selfIp = splitIp(argv[1]);
    printf("%s", selfIp[0]);
    printf(":%s\n", selfIp[1]);
    struct sockaddr_in* addrServer;

    int nombreElement;
    addrServer = initAddrServer(argv[2], &nombreElement);


    removeAddrServer(addrServer, selfIp, &nombreElement);

    //print all sockaddr_in from addrServer
    for(int i = 0; i < nombreElement; i++){
        printf("Adresse IP %d: %s:%d\n",i, inet_ntoa(addrServer[i].sin_addr),ntohs(addrServer[i].sin_port));
    }

    printf("Initialisation self state\n");

    struct siteState* selfState = initSiteState(selfIp[0], selfIp[1], addrServer);
    printf("%s", selfState->ip);
    printf(":%d\n", selfState->port);
    printf("%d\n", selfState->puissance_Pere);
    printf("Is father null ? %d\n", selfState->pere==(void*)0);



    return 0;
}