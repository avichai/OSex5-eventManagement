#include <netdb.h>
#include <string.h>
#include <iostream>
#include <unistd.h>
#include <arpa/inet.h>
#include "Client.h"


#define CLIENT_VALID_NARGS 4
#define CLIENT_NAME_INDEX 1
#define SERVER_ADDR_INDEX 2
#define SERVER_PORT_INDEX 3


using namespace std;



int writeData(int s) {

//    char* buf = (char*) malloc(2);
//    buf[0] = 'a';
//    buf[1] = '\0';
//    cout << "buf in read: " << buf << endl;

    string buf = "hello";

    const char* c = buf.c_str();
    cout << "strlen " << strlen(c) << endl;
    write(s, c, strlen(c));

}



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
    struct sockaddr_in sa;
    sa.sin_addr.s_addr = inet_addr("132.65.125.154");
    sa.sin_family = AF_INET;
    sa.sin_port = htons(8888);

    int s = callSocket(sa);

    writeData(s);

    cout << "client finished!!!" << endl;
    close(s);
    return 0;
}