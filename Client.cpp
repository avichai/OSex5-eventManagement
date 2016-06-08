#include <netdb.h>
#include <string.h>
#include "Client.h"


// can give sa as arg?

int callSocket(struct sockaddr sa) {
    struct sockaddr_in sa;
    int s;

    s = socket(AF_INET, SOCK_STREAM, 0);

    connect(s, (struct sockaddr*)&sa, sizeof(sa)); //todo

    return s;
}

