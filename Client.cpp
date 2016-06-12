#include <netdb.h>
#include <iostream>
#include <unistd.h>
#include <arpa/inet.h>
#include <assert.h>
#include <set>
#include "Client.h"
#include "Utils.h"


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

#define CREATE_NARGS 3

using namespace std;


enum ERROR_TYPE {ILLEGAL_CMD, MISSING_ARG, INVALID_ARG,
                 FIRST_CMD_REG, ALREADY_REG, ALREADY_RSVP,
                 INVALID_CMD, FAILED};


// forward declarations
static bool validCreateNArgs(string data);
static bool validCreateArgs(string data, string & invalidArg);
static string getSortedEvents(string events);
static void assignLogName(string clientName);
static void writeToClientLog(string message);
static int callServer(struct sockaddr_in sa);
static void clientRun(string clientName, struct sockaddr_in serverAddr);
static void handleResponse(string response, string cmd, string input, bool &stillRunning);


// globalS
string logName;
string clientName;
bool isRegistered;
set<string> eventsSet;


/*
 * Validates the create number of arguments.
 */
static bool validCreateNArgs(string data) {
    for (int i = 0; i < CREATE_NARGS; ++i) {
        if (popNextToken(data, SPACE) == "") {
            return false;
        }
    }
    return true;
}

/*
 * Validate the create arguments. If not valid, assigns the invalid argument
 * to the given invalidArg string.
 */
static bool validCreateArgs(string data, string & invalidArg) {
    string eventTitle = popNextToken(data, SPACE);
    string eventDate = popNextToken(data, SPACE);
    string eventDesc = popNextToken(data, SPACE);

    //todo: how to validate these args?

    return true;
}

/*
 * Events sorter (by id).
 */
static bool eventsSorter(const string e1, const string e2) {
    return stoi(peekFirstToken(e1, SPACE)) > stoi(peekFirstToken(e2, SPACE));
}

/*
 * Returns a string representing the top 5 events (sorted by id).
 * Sorted from newest to oldest.
 */
static string getSortedEvents(string events) {
    if (events == "") {
        return "";
    }
    vector<string> vec;
    string event;
    while ((event = popNextToken(events,NEWLINE)) != "") {
        vec.push_back(event);
    }
    sort(vec.begin(), vec.end(), eventsSorter);

    string sortedEvents = "";
    for (auto it = vec.begin(); it != vec.end(); ++it) {
        sortedEvents += *it + ".\n";
    }
    return sortedEvents;
}

/*
 * Returns a string representing the RSVPs for an event (sorted alphabetically).
 */
static string getSortedRSVPs(string names) {
    if (names == "") {
        return "";
    }
    vector <string> vec;
    string name;
    while ((name = popNextToken(names,COMMA)) != "") {
        vec.push_back(name);
    }
    sort(vec.begin(), vec.end());

    string sortedNames = "";
    for (auto it = vec.begin(); it != vec.end(); ++it) {
        sortedNames += *it + COMMA;
    }
    sortedNames.pop_back(); //removes last ','
    return sortedNames;
}

/*
 * Gets the log's name.
 */
static void assignLogName(string clientName) {
    logName = clientName + "_" + getTime(false) + ".log";
}

/*
 * Writes error to the client's log.
 */
static void writeErrToClientLog(ERROR_TYPE errType, string s1, string s2) {
    string errMessage = "ERROR: ";
    switch (errType) {
        case ILLEGAL_CMD:
            errMessage += "illegal command";
            break;
        case MISSING_ARG:
            errMessage += "missing argument in command " + s1;
            break;
        case INVALID_ARG:
            errMessage += "invalid argument " + s1 + " in command " + s2;
            break;
        case FIRST_CMD_REG:
            errMessage += "first command must be REGISTER.";
            break;
        case ALREADY_REG:
            errMessage += "the client" + clientName + " was already registered";
            break;
        case ALREADY_RSVP:
            errMessage += "RSVP to event id " + s1 + " was already sent";
            break;
        case INVALID_CMD:
            errMessage += "Invalid command";
            break;
        case FAILED:
            errMessage += "failed to " + s1;
            break;
    }
    writeToLog(logName, errMessage);
}

/*
 * Writes message to the client's log.
 */
static void writeToClientLog(string message) {
    writeToLog(logName, message);
}

/*
 * Calls the server.
 */
static int callServer(struct sockaddr_in sa) {

    int s;

    s = socket(AF_INET, SOCK_STREAM, 0);
    checkSyscall(logName, s, "socket");

    int c = connect(s, (struct sockaddr*)&sa, sizeof(sa));
    checkSyscall(logName, c, "connect");

    return s;
}

/*
 * Runs the client.
 */
static void clientRun(string clientName, struct sockaddr_in serverAddr) {
    isRegistered = false;

    string input, cmd, message;
    const char* cmdC;
    int serverS;
    bool stillRunning = true;
    while (stillRunning) {
        getline(cin, input);
        cmdC = popNextToken(input, SPACE).c_str();

        if (strcasecmp(cmdC, REGISTER_CMD) == 0) {
            if (isRegistered) {
                writeErrToClientLog(INVALID_CMD,"","");
                continue;
            }
            cmd = REGISTER;
        }
        else if (strcasecmp(cmdC, CREATE_CMD) == 0) {
            string invalidArg;
            if (!isRegistered) {
                writeErrToClientLog(FIRST_CMD_REG,"","");
                continue;
            }
            if (validCreateNArgs(input)) {
                writeErrToClientLog(MISSING_ARG,CREATE_CMD,"");
                continue;
            }
            if (validCreateArgs(input,invalidArg)) {//todo: how to check args?
                writeErrToClientLog(INVALID_ARG,invalidArg,CREATE_CMD);
                continue;
            }
            cmd = CREATE;
        }
        else if (strcasecmp(cmdC, GET_TOP_5_CMD) == 0) {
            if (!isRegistered) {
                writeErrToClientLog(FIRST_CMD_REG,"","");
                continue;
            }
            cmd = GET_TOP_5;
        }
        else if (strcasecmp(cmdC, SEND_RSVP_CMD) == 0) {
            unsigned int ignored;
            if (!isRegistered) {
                writeErrToClientLog(FIRST_CMD_REG,"","");
                continue;
            }
            if (input == "") {
                writeErrToClientLog(MISSING_ARG,SEND_RSVP_CMD,"");
                continue;
            }
            if (!isPosInt((char*) input.c_str(),ignored)) {
                writeErrToClientLog(INVALID_ARG,input,SEND_RSVP_CMD);
                continue;
            }
            if (eventsSet.find(input) != eventsSet.end()) {
                writeErrToClientLog(ALREADY_RSVP,input,"");
                continue;
            }
            cmd = SEND_RSVP;
        }
        else if (strcasecmp(cmdC, GET_RSVPS_LIST_CMD) == 0) {
            unsigned int ignored;
            if (!isRegistered) {
                writeErrToClientLog(FIRST_CMD_REG,"","");
                continue;
            }
            if (input == "") {
                writeErrToClientLog(MISSING_ARG,GET_RSVPS_LIST,"");
                continue;
            }
            if (!isPosInt((char*) input.c_str(),ignored)) {
                writeErrToClientLog(INVALID_ARG,input,GET_RSVPS_LIST);
                continue;
            }
            cmd = GET_RSVPS_LIST;
        }
        else if (strcasecmp(cmdC, UNREGISTER_CMD) == 0) {
            if (!isRegistered) {
                writeErrToClientLog(FIRST_CMD_REG,"","");
                continue;
            }
            cmd = UNREGISTER;
        }
        else {
            writeErrToClientLog(ILLEGAL_CMD,"","");
            continue;
        };


        message = cmd + SPACE + clientName + SPACE + input;
        serverS = callServer(serverAddr);
        writeData(serverS, message, logName);
        string response = readData(serverS, logName);
        handleResponse(response, cmd, input, stillRunning);
    }
}

/*
 * Handles the response received by the server.
 */
static void handleResponse(string response, string cmd, string input, bool &stillRunning) {
    bool requestSucceed = popNextToken(response, SPACE) == REQUEST_SUCCESS;

    if (cmd == REGISTER) {
        if (requestSucceed) {
            isRegistered = true;
            writeToClientLog("Client " + clientName + " was registered successfully");
        }
        else {
            writeErrToClientLog(ALREADY_REG,"","");
            stillRunning = false;
            exit(1);
        }
    }
    else if (cmd == CREATE) {
        if (requestSucceed) {
            writeToClientLog("Event id " + response + " was created successfully");
        }
        else {
            writeErrToClientLog(FAILED,"create the event: " + response,"");
        }
    }
    else if (cmd == GET_TOP_5) {
        if (requestSucceed) {
            string sortedEvents = getSortedEvents(response);
            writeToClientLog("Top 5 newest events are:\n" + sortedEvents);  //todo: maybe with ".\n"?
        }
        else {
            writeErrToClientLog(FAILED,"receive top 5 newest events " + response,"");
        }
    }
    else if (cmd == SEND_RSVP) {
        if (requestSucceed) {
            eventsSet.insert(input);
            writeToClientLog("RSVP to event id " + input + " was received successfully");
        }
        else {
            writeErrToClientLog(FAILED,"send RSVP to event id " + input + ":" + response,"");
        }
    }
    else if (cmd == GET_RSVPS_LIST) {
        if (requestSucceed) {
            string sortedRSVPs = getSortedRSVPs(response);  //todo (without\n at the end)
            writeToClientLog("The RSVP's list for event id " + input + " is: " + sortedRSVPs);
        }
        else {
            // todo: check if can fail by the server
        }
    }
    else if (cmd == UNREGISTER) {
        if (requestSucceed) {
            writeToClientLog("Client " + clientName + " was unregistered successfully");
            stillRunning = false;
            exit(0);
        }
        else {
            // todo: check if can fail by the server
        }
    }
    else {
        assert(0); //todo: remove
    }
}


/*
 * Client main function.
 */
int main(int argc, char *argv[]) {
    unsigned int portNum;
    if ((argc != CLIENT_VALID_NARGS) ||
        (!isAddress(argv[SERVER_ADDR_INDEX])) ||
        (!isPosInt(argv[SERVER_PORT_INDEX], portNum))) {
        cout << "Usage: emClient clientName serverAddress serverPort" << endl;
        return 0;
    }

    struct sockaddr_in serverAddr;
    serverAddr.sin_addr.s_addr = inet_addr(argv[SERVER_ADDR_INDEX]);
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons((unsigned short) portNum);

    clientName = string(argv[CLIENT_NAME_INDEX]);
    assignLogName(clientName);

    // runs the client
    clientRun(clientName, serverAddr);

    cout << "client - " << clientName << " exit!!" << endl;  //todo: remove
    return 0;
}