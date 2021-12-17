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

int main(int argc, const char *argv[]) {
    //Ask the user to input a server ip and port
    printf("Please enter the server ip : ");
    char server_ip[20];
    char server_port[20];
    scanf("%s", server_ip);
    printf("\nPlease enter the server port : ");
    scanf("%s", server_port);
    printf("\nYou entered: %s:", server_ip);
    printf("%s\n", server_port);


    //Create UDP socket
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        perror("socket");
        exit(1);
    }

    //Create server address
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(atoi(server_port));
    server_addr.sin_addr.s_addr = inet_addr(server_ip);

    //Send a message to the server
    char message[10];
    printf("Please enter a message: ");
    scanf("%s", message);
    printf("Sending message: %s\n", message);
    int snd = sendto(sockfd, message, strlen(message), 0, (struct sockaddr *)&server_addr, sizeof(server_addr));
    if (snd < 0) {
        perror("sendto");
        exit(1);
    }


    return 0;
}