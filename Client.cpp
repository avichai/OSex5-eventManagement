#include <netdb.h>
#include <string.h>
#include <iostream>
#include <unistd.h>
#include <arpa/inet.h>
#include "Client.h"
#include "Utils.cpp"


#define CLIENT_VALID_NARGS 4
#define CLIENT_NAME_INDEX 1
#define SERVER_ADDR_INDEX 2
#define SERVER_PORT_INDEX 3


using namespace std;

// can give sa as arg?
int callSocket(struct sockaddr_in sa) {
    int s;

    s = socket(AF_INET, SOCK_STREAM, 0);

    int n = connect(s, (struct sockaddr*)&sa, sizeof(sa)); //todo

    if (n < 0) {
        cout << "connect" << endl;
    }

    return s;
}



int main(int argc, char *argv[]) {
    unsigned short portNum;
    if ((argc != CLIENT_VALID_NARGS) ||
        (!isAddress(argv[SERVER_ADDR_INDEX])) ||
        (!isPosInt(argv[SERVER_PORT_INDEX], portNum))) {
        cout << "Usage: emClient clientName serverAddress serverPort" << endl;
        return 0;
    }

    struct sockaddr_in sa;
    sa.sin_addr.s_addr = inet_addr(argv[SERVER_ADDR_INDEX]);
    sa.sin_family = AF_INET;
    sa.sin_port = htons(portNum);

    int s = callSocket(sa);//todo

    writeData(s, "blabla");

    close(s); //todo
    cout << "client finished!!!" << endl;
    return 0;
}