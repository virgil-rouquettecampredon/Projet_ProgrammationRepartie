//
// Created by vivap on 26/11/2021.
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include <sys/socket.h>
#include <arpa/inet.h>


/*struct sockaddr_in server;
server.sin_family = AF_INET;
server.sin_addr.s_addr = INADDR_ANY;
server.sin_port = htons(atoi(argv[1]));*/

char** splitIp(char* ip){
    char** ip_array = (char**)malloc(4 * sizeof(char*));
    const char* delimiter = ":";
    strtok(ip, delimiter);
    ip_array[0] = ip;
    ip_array[1] = strtok(NULL, delimiter);

    return ip_array;
}

struct sockaddr_in* initAddrServer(const char* emplacementFichier){
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
    return addrServer;
}


int main(int argc, const char * argv[]) {

    //test if there are 3 arguments
    if(argc != 3){
        printf("Error: wrong number of arguments\n");
        exit(1);
    }

    struct sockaddr_in* addrServer;

    addrServer = initAddrServer(argv[2]);

    printf("Adresse IP 0 : %s\n", inet_ntoa(addrServer[0].sin_addr));
    printf("Adresse IP 1 : %s\n", inet_ntoa(addrServer[1].sin_addr));
    printf("Port IP 0 : %d\n", ntohs(addrServer[0].sin_port));
    printf("Port IP 1 : %d\n", ntohs(addrServer[1].sin_port));

    return 0;
}