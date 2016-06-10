#include <netdb.h>
#include <iostream>
#include <unistd.h>
#include <arpa/inet.h>
#include <assert.h>
#include "Client.h"
#include "Utils.cpp"


#define CLIENT_VALID_NARGS 4
#define CLIENT_NAME_INDEX 1
#define SERVER_ADDR_INDEX 2
#define SERVER_PORT_INDEX 3

#define REGISTER_CMD "REGISTER"
#define CREATE_CMD "CREATE"
#define GET_TOP_5_CMD "GET_TOP_5"
#define SEND_RSVP_CMD "SEND_RSVP"
#define GET_RSVPS_LIST_CMD "GET_RSVPS_LIST"
#define UNREGISTER_CMD "UNREGISTER"

#define MAX_RESPONSE_LEN 99999


using namespace std;

// forward declarations
static int callServer(struct sockaddr_in sa);
static void clientRun(string clientName, struct sockaddr_in serverAddr);
static void handleRespone(string response, string cmd);


// global logName
string logName;


/*
 * Gets the log's name.
 */
void getLogName(string clientName) {
    //todo
}

/*
 * Calls the server.
 */
static int callServer(struct sockaddr_in sa) {
    int s;

    s = socket(AF_INET, SOCK_STREAM, 0);// todo

    int n = connect(s, (struct sockaddr*)&sa, sizeof(sa)); //todo

    if (n < 0) {
        cout << "connect" << endl;
    }

    return s;
}

/*
 * Runs the client.
 */
static void clientRun(string clientName, struct sockaddr_in serverAddr) {
    bool isRegistered = false;


    string input, cmd, delim = SPACE, message, args;
    const char* cmdC;
    size_t pos;
    int serverS;
    bool stillRunning = true, sendReq;
    while (stillRunning) {
        sendReq = false;
        args = "";

        getline(cin, input);
        cmdC = getNextToken(input, SPACE).c_str();
        if (strcasecmp(cmdC, REGISTER_CMD) == 0) {
            if (isRegistered) {

            }
            isRegistered = true;
            sendReq = true;
            cmd = REGISTER;
        }
        else if (strcasecmp(cmdC, CREATE_CMD) == 0) {
            if (!isRegistered) {

            }
        }
        else if (strcasecmp(cmdC, GET_TOP_5_CMD) == 0) {
            if (!isRegistered) {

            }

        }
        else if (strcasecmp(cmdC, SEND_RSVP_CMD) == 0) {
            if (!isRegistered) {

            }

        }
        else if (strcasecmp(cmdC, GET_RSVPS_LIST_CMD) == 0) {
            if (!isRegistered) {

            }

        }
        else if (strcasecmp(cmdC, UNREGISTER_CMD) == 0) {
            stillRunning = false;
        }
        else {

        };


        if (sendReq) {
            message = cmd + SPACE + clientName + SPACE + args;
            serverS = callServer(serverAddr);
            writeData(serverS, message);
            char response[MAX_RESPONSE_LEN];
            readData(serverS, response, MAX_RESPONSE_LEN);
            handleRespone(string(response), cmd);
        }
    }
}

/*
 * Handles the response received by the server.
 */
static void handleRespone(string response, string cmd) {
    bool requestSucceed = getNextToken(response, SPACE) == REQUSET_SUCCESS;

    if (cmd == REGISTER) {
        if (requestSucceed) {

        }
        else {

        }
    }
    else if (cmd == CREATE) {
        if (requestSucceed) {

        }
        else {

        }
    }
    else if (cmd == GET_TOP_5) {
        if (requestSucceed) {

        }
        else {

        }
    }
    else if (cmd == SEND_RSVP) {
        if (requestSucceed) {

        }
        else {

        }
    }
    else if (cmd == GET_RSVPS_LIST) {
        if (requestSucceed) {

        }
        else {

        }
    }
    else if (cmd == UNREGISTER) {
        if (requestSucceed) {

        }
        else {

        }
    }

    assert(0); //todo: remove
}


/*
 * Client main function.
 */
int main(int argc, char *argv[]) {
    unsigned short portNum;
    if ((argc != CLIENT_VALID_NARGS) ||
        (!isAddress(argv[SERVER_ADDR_INDEX])) ||
        (!isPosInt(argv[SERVER_PORT_INDEX], portNum))) {
        cout << "Usage: emClient clientName serverAddress serverPort" << endl;
        return 0;
    }

    struct sockaddr_in serverAddr;
    serverAddr.sin_addr.s_addr = inet_addr(argv[SERVER_ADDR_INDEX]);
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(portNum);

    string clientName = string(argv[CLIENT_NAME_INDEX]);
    getLogName(clientName);

    // runs the client
    clientRun(clientName, serverAddr);

    cout << "client - " << clientName << " exit!!" << endl;
    return 0;
}