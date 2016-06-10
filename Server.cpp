#include <netinet/in.h>
#include <string.h>
#include <netdb.h>
#include <unistd.h>
#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "Server.h"
#include "Utils.cpp"

#define MAX_HOST_NAME 40    //todo
#define N_PEDING_CLIENTS 10
#define SERVER_VALID_NARGS 2
#define DECIMAL 10
#define PORT_NUM_INDEX 1


using namespace std;


/*
 * Returns true iff the string represents a positive int, and assigns the int
 * value to the given reference.
 */
static bool isPosInt(char* str, unsigned short &portNum)
{
    char* end  = 0;
    int tmpCacheSize = strtol(str, &end, DECIMAL);
    portNum = (unsigned short) tmpCacheSize;

    return (*end == '\0') && (end != str) && (tmpCacheSize > 0);
}

/*
 * Handles syscall failure in the server side.
 */
static void syscallHandler(string funcName) {
    // todo write to log
    exit(1);
}

/*
 * Check syscall in the server side.
 */
static void checkSyscallServer(int res, string funcName) {
    if (res < 0) {
        syscallHandler(funcName);
    }
}


/*
 * Establishes server socket.
 */
int establish(unsigned short portnum)
{
    char myName[MAX_HOST_NAME+1];
    int serverS;
    struct sockaddr_in sa;
    struct hostent* hp;

    memset(&sa,0,sizeof(struct sockaddr_in));
    checkSyscallServer(gethostname(myName, MAX_HOST_NAME), "gethostname");
    hp = gethostbyname(myName);
    if (hp == NULL) {
        syscallHandler("gethostbyname");
    }

    sa.sin_family = hp->h_addrtype;
    memcpy(&sa.sin_addr,hp->h_addr,hp->h_length);
    sa.sin_port = htons(portnum);
//    cout << "addr = " << inet_ntoa(sa.sin_addr) << endl;

    serverS = socket(AF_INET,SOCK_STREAM,0);
    checkSyscallServer(serverS, "socket");

    checkSyscallServer(bind(serverS, (struct sockaddr*)&sa, sizeof(sa)), "bind");
    checkSyscallServer(listen(serverS, N_PEDING_CLIENTS), "listen");

    return serverS;
}






/*
 * Server main function.
 */
int main(int argc, char *argv[]) {
    unsigned short portNum;
    if ((argc!=SERVER_VALID_NARGS)&&(isPosInt(argv[PORT_NUM_INDEX], portNum))) {
        cout << "Usage: emServer portNum" << endl;
    }

    int serverS = establish(portNum);
    struct sockaddr client;
    int a = accept(serverS, (struct sockaddr*)&client, (socklen_t*)&client), "accept");

    char buf[10];
    readData(a, buf, 5);

    cout << "buf = " << buf << endl;
    cout << "server finished!!!" << endl;

    close(serverS);
    return 0;
}


