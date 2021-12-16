#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include <sys/socket.h>
#include <arpa/inet.h>

char** splitIp(char* ip){
    char** ip_array = (char**)malloc(4 * sizeof(char*));
    const char* delimiter = ":";
    char* token;
    token = strtok(ip, delimiter);
    ip_array[0] = token;
    ip_array[1] = strtok(NULL, delimiter);

    return ip_array;
}
char** split(char s[],char* delimiteur,int n){

int i=0;
char* p = strtok (s, delimiteur);
char** tab = (char**)malloc(n * sizeof(char*));
   while (p != NULL)
  {
        tab[i]=p;
        p = strtok (NULL, delimiteur);
        i++;
  }
return tab;
}
char** splitLigne(char* chaine){
    char** ip_array = (char**)malloc(4* sizeof(char*));
    const char* delimiteur = "#";
    char* token;
    token = strtok(chaine, delimiteur);
    int i = 0;
   while( token != NULL ) {
         printf( " %s\n", token );
       ip_array[i] = token;
       i++;
         token = strtok(NULL, delimiteur);
      }

    return ip_array;
}

/*char** splitLine(char* chaine,int nbLine){

    char** lines = (char**)malloc(nbLine* sizeof(char*));
    const char* delimiter = "§";
    int j = 0;
    while(j <= nbLine){
        for(int i = 0;i < sizeof(chaine);i++){
            if(strcmp((char)chaine[i],delimiter)==0){
            lines[j] += chaine[i];
            }
            else{
                j++;
                i++;
            }
        }
    }

    return lines;
}
*/

struct sockaddr_in* initAddrServer(char* str, int n){

    struct sockaddr_in* addrServer = (struct sockaddr_in*)malloc(n * sizeof(struct sockaddr_in));
    char* delimiterLigne = "#";
    char* delimiterIp = ":";
    char** lines = split(str,delimiterLigne,n);
    char** ipPort;
    int i =0;
    while(i<n){
        ipPort = split(lines[i],delimiterIp,2);
        //printf(" %s  ", lines[i]);
        printf(" %s  ", ipPort[0]);
        printf(" %s  \n", ipPort[1]);
        addrServer[i].sin_family = AF_INET;
        addrServer[i].sin_addr.s_addr = inet_addr(ipPort[0]);
        addrServer[i].sin_port = htons(atoi(ipPort[1]));
        i++;
        }

    return addrServer;
}

int main(int argc, char *argv[]){
    /* char str[80] = "ip1:port1#ip2:port2#ip3:port3#ip4:port4#";
    int n = 4;
    char* delimiterLigne = "#";
    char* delimiterIp = ":";
    char** lines = split(str,delimiterLigne,4);
    char** ipPort;
    int i =0;
    while(i<n){
         ipPort = split(lines[i],delimiterIp,2);
         //printf(" %s  ", lines[i]);
         printf(" %s  ", ipPort[0]);
         printf(" %s  \n", ipPort[1]);
         i++;
    }
    initAddrServer(str,n); */
    char puissance[3];
    sprintf(puissance, "%d", 0);
    char attaque[10] ="ATTACK:";
    strcat(attaque,puissance);

    printf("attaque = %s \n",attaque);





return 0;
}








































struct siteState* me = initSiteState(myIP,myPort, &addrServer);
me->id = selfPosition;

bool victoire = false;
int nbSiteAttack = nbSites;
bool resRecu = true; // au debut tout le monde peut attaquer on met resRecu a true dans ce cas là


while(me->puissance < (nbSites/2) && !victoire){

//me est attaquant dans cette conditon ( le cas ou x envoie un message d'attaque)
    if(me->etat == PARTICIPANT && resRecu ){
    resRecu = false;
    int y = rand()%nbSiteAttack;
    char puissance[3];
    sprintf(puissance, "%d", me->puissance);
    char attaque[10] ="AT:";
    strcat(attaque,puissance);
    char id[3];
    sprintf(id, "%d", me->id);
    strcat(attaque,id);
    sockaddr_in cible = addrServer[y];
    //envoi du message d'attaque à la cible
    sendto(ds,attaque,10,0,(struct sockaddr *)&cible,sizeof(cible));

    struct sockaddr_in contact;
    recvfrom(ds,message,10,0,(struct sockaddr *)&contact,sizeof(contact));

    delimiter = ":";
    char** contenuMsg = split(message,delimiter,4);

    if(strcmp(contenuMsg[1],"AT")==0){
        if(me->puissance > atoi(contenuMsg[2])){
            char res[3];
            sprintf(res, "%d", PERDANT);
            char resultat[10] ="RE:";
            strcat(resultat,res);
            sendto(ds,resultat,10,0,(struct sockaddr *)&cible,sizeof(contact));
        }
        else{
            if(me->puissance < atoi(contenuMsg[2])){
                if(me->etat != CAPTURE){
                    char res[3];
                    sprintf(res, "%d", GAGNANT);
                    char resultat[10] ="RE:";
                    strcat(resultat,res);
                    sendto(ds,resultat,10,0,(struct sockaddr *)&cible,sizeof(contact));
                    me->pere = &contact;
                    me->puissance_pere = atoi(contenuMsg[2])+1;
                }
                else{
                    char puissance_adversaire[3] = contenuMsg[2];
                    char demande[10] ="DM:";
                    strcat(demande,puissance_adversaire);
                    char id[3] = contenuMsg[3];
                    strcat(demande,puissance_adversaire);
                    strcat(demande,id);
                    sendto(ds,resultat,10,0,(struct sockaddr *)me->pere,sizeof(me->pere));
                }
            }
            else{
                if(me->id > atoi(contenuMsg[3])){
                    char res[3];
                    sprintf(res, "%d", PERDANT);
                    char resultat[10] ="RE:";
                    strcat(resultat,res);
                    sendto(ds,resultat,10,0,(struct sockaddr *)&cible,sizeof(contact));
                }
                else{
                    char res[3];
                    sprintf(res, "%d", GAGNANT);
                    char resultat[10] ="RE:";
                    strcat(resultat,res);
                    sendto(ds,resultat,10,0,(struct sockaddr *)&cible,sizeof(contact));
                    me->pere = &contact;
                    me->puissance_pere = atoi(contenuMsg[2])+1;
                }
            }
        }
    }
    if(strcmp(contenuMsg[1],"DM")==0){
        if(me->puissance > atoi(contenuMsg[2])){
                    char res[3];
                    sprintf(res, "%d", PERDANT);
                    char resultat[10] ="RT:";
                    strcat(resultat,res);
                    sendto(ds,resultat,10,0,(struct sockaddr *)&cible,sizeof(contact));
                }
                else{

                }

        }



    }

}