//
// Created by Virgil LAPTOP 2 on 03/12/2021.
//

#include <stdlib.h>
#include <stdio.h>

int main(int argc, char *argv[]) {
    if (argc!=2){
        printf("Utilisation : %s nombreDeSites\n", argv[0]);
        exit(1);
    }

    int exit_status;
    int nbProcess = atoi(argv[1]);

    for(int i = 0; i<nbProcess; i++) {
        exit_status = system("glxgears");
        if(exit_status==-1) {
            perror("Failed opening terminal\n");
            exit(1);
        }
    }
}
