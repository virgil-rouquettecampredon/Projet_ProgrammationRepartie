//
// Created by vivap on 26/11/2021.
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, const char * argv[]) {
    char ip1[] = "192.168.1.1:8800";
    char ip2[] = "192.168.168.1.2:2077";

    //Split the ip into ip and port
    const char* delimiter = ":";
    strtok(ip1, delimiter);
    char* port = strtok(NULL, delimiter);
    printf("IP : %s", ip1);
    printf("\nPort : %s\n\n", port);

    strtok(ip2, delimiter);
    port = strtok(NULL, delimiter);
    printf("IP : %s", ip2);
    printf("\nPort : %s\n", port);

    return 0;
}