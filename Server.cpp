#include <netinet/in.h>
#include <string.h>
#include <netdb.h>
#include <unistd.h>
#include "Server.h"

#define MAX_HOST_NAME 40    //todo
#define N_PEDING_CLIENTS 10



int establish(unsigned short portnum)
{
    char myName[MAX_HOST_NAME+1];
    int s;
    struct sockaddr_in sa;
    struct hostent* hp;

    memset(&sa,0,sizeof(struct sockaddr_in));

    gethostname(myName, MAX_HOST_NAME); //todo
    hp = gethostbyname(myName); //todo

    sa.sin_family = hp->h_addrtype;

    memcpy(&sa.sin_addr,hp->h_addr,hp->h_length);

    sa.sin_port = htons(portnum);

    s = socket(AF_INET,SOCK_STREAM,0); //todo

    bind(s, (struct sockaddr*)&sa,sizeof(struct sockaddr_in));  // todo

    listen(s, N_PEDING_CLIENTS); //todo

    return s;
}





int main(int argc, char *argv[]) {

    int s = establish( );

}




























