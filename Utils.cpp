
#define REGISTER "000"
#define CREATE "001"
#define GET_TOP_5 "010"
#define SEND_RSVP "011"
#define GET_RSVPS_LIST "100"
#define UNREGISTER "101"
#define REQUSET_LEN 3

#define FAILURE -1
#define SUCCESS 0

#include <iostream>
#include <string.h>
#include <unistd.h>

using namespace std;
/*
 * Writes data to the given socket.
 * todo: maybe +1 to the strlen
 * todo: should write in loop
 */
int writeData(int socket, string data) {
    const char* cData = data.c_str();
    cerr << "### write ###" << endl;
    cerr << "data: " << cData << endl;
    cerr << "length: " << strlen(cData) << endl;
    if (write(socket, cData, strlen(cData)) < 0) {
        return FAILURE;
    }
    cerr << "### write finished ###" << endl;
    return SUCCESS;
}


/*
 * Reads data from the given socket.
 * todo: get n within the function rather then as arg
 */
int readData(int socket, char* buf, int n) {
    int bCount;
    int br;
    bCount = 0, br = 0;

    while (bCount < n) {
        if ((br = read(socket,buf,n-bCount)) > 0) {
            bCount += br;
            buf += br;
        }

        if (br < 1) {
            return -1;
        }
    }

    return bCount;
}


/*
 * Writes data to the given log.
 * todo: write a prefix of HH:MM:SS\t on every line.
 */
int writeToLog(string logName, string data) {

    return SUCCESS;
}