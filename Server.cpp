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
#include <unordered_set>
#include <unordered_map>
#include <thread_db.h>


#define MAX_HOST_NAME 40    //todo
#define N_PEDING_CLIENTS 10
#define SERVER_VALID_NARGS 2
#define SERVER_PORT_INDEX 1
#define INITIAL_MESS_SIZE 5
#define EXIT_FROM_KEYBOARD "EXIT"
#define COMMAND_IN_VECTOR_INDEX 0
#define CLIENT_NAME_IN_VECTOR_INDEX 1

#define EVENT_TITLE_IN_VECTOR_INDEX 2
#define EVENT_DATE_IN_VECTOR_INDEX 3
#define STRAT_DESC_IN_VECTOR_INDEX 4

#define EVENT_ID_IN_VECTOR_INDEX 2

#define LOG_FILENAME "logFileName"
#define SUCCESS_STR "0"
#define FAILURE_STR "1"



static void syscallHandler(string funcName);
static void checkSyscallServer(int res, string funcName);
static void* listenToKeyboard(void*);
static int establish(unsigned short portnum);
static void* handleRequest(void* acceptSock);
static void terminateServer();


using namespace std;

class Event;

unordered_set <string>* gClientsSet;
unordered_map <unsigned int, Event*>* gEventsMap;
unordered_set <thread_t> gThreads;
unsigned int gIdCounter;

pthread_mutex_t gClientsMutex;
pthread_mutex_t gEventsMutex;
pthread_mutex_t gThreadsMutex;
pthread_mutex_t gIdCounterMutex;


/*
 * class describing event.
 */
class Event {
    unsigned int id;
    string eventTitle;
    string eventDate;
    string eventDescription;
    string rsvpList;

public:
    Event(unsigned int id, const string &eventTitle, const string &eventDate,
          const string &eventDescription,
          const string &rsvpList) : id(id), eventTitle(eventTitle), eventDate(
            eventDate), eventDescription(eventDescription),
                                    rsvpList(rsvpList) { }

    unsigned int getId() const {
        return id;
    }

    const string &getEventTitle() const {
        return eventTitle;
    }

    const string &getEventDate() const {
        return eventDate;
    }

    const string &getEventDescription() const {
        return eventDescription;
    }

    const string &getRsvpList() const {
        return rsvpList;
    }

    void setRsvpList(const string &rsvpList) {
        Event::rsvpList = rsvpList;
    }
};

/*
 * write the data to the log using utils function.
 */
static void writeToLog(string data) {
    writeToLog(LOG_FILENAME, data);
}

/*
 * split lines by char.
 */
unsigned int split(const string &txt, vector<std::string> &strs, char ch) {
    unsigned int pos = txt.find(ch);
    unsigned int initialPos = 0;
    strs.clear();

    // Decompose statement
    while( pos != std::string::npos) {
        strs.push_back(txt.substr(initialPos, pos - initialPos + 1 ) );
        initialPos = pos + 1;

        pos = txt.find(ch, initialPos);
    }

    // Add the last one
    strs.push_back(txt.substr(initialPos, min(pos, txt.size()) - initialPos + 1));

    return strs.size();
}

/**/
/*
 * Returns true iff the string represents a positive int, and assigns the int
 * value to the given reference.
 */
static bool getPosInt(const char* str, unsigned int &index) {
    char* end  = 0;
    int tmpIndex = strtol(str, &end, DECIMAL);
    index = (unsigned short) tmpIndex;
}

/*
 * Handles syscall failure in the server side.
 */
static void syscallHandler(string funcName) {
    cerr<<"handler" << funcName << endl;
    writeToLog("ERROR\t" + funcName + "\t" + to_string(errno));
    exit(EXIT_FAILURE);
}

/*
 * Check syscall in the server side.
 */
static void checkSyscallServer(int res, string funcName) {
    cerr << "sys call: " << funcName << " res: " << res<< endl;
    if (res < 0) {
        syscallHandler(funcName);
    }
}

/*
 * listen to keyboard until it gets 'EXIT' and terminate the program.
 */
static void* listenToKeyboard(void* serverS) {
    string input;

    bool stillRunning = true;
    while(stillRunning) {
        getline(cin, input);
        if (input == EXIT_FROM_KEYBOARD) {
            stillRunning = false;
        }
    }
        terminateServer((int*) serverS);
}

/*
 * terminates the program.
 */
static void terminateServer(int* serverS) {
    for (thread_t thread : gThreads) {
        checkSyscallServer(pthread_join(thread, NULL), "pthread_join");
    }
    close(*serverS);
    exit(EXIT_SUCCESS);
//    pthread_exit(NULL);
}

/*
 * send a message to client through the socket (first the number of char)
 * and than the message itself.
 */
static void sendToClient(int accSocket, string mess) {
    unsigned int messSize = mess.length();
    writeData(accSocket, string(to_string(messSize)));
    writeData(accSocket, mess);

}

/*
 * check if the current client already registered to the server.
 */
static int checkIfConnectionWasMade(int acceptSocket, vector<string> argFromClient, string& clientName) {

    checkSyscallServer(pthread_mutex_lock(&gClientsMutex), "pthread_mutex_lock");
    if (gClientsSet->find(argFromClient[CLIENT_NAME_IN_VECTOR_INDEX]) == gClientsSet->end()) {
    checkSyscallServer(pthread_mutex_unlock(&gClientsMutex), "pthread_mutex_unlock");

        clientName = argFromClient[CLIENT_NAME_IN_VECTOR_INDEX];
        writeToLog("ERROR:" + clientName + "\tis already exists");
        sendToClient(acceptSocket, FAILURE_STR);
        return FAILURE;
    }
    return SUCCESS;

}

/*
 * handles the situation when where the client asked an event id that isn't
 * exists.
 */
static void handleNotFoundID(int acceptSocket, int curId) {
    writeToLog("event id: " + to_string(curId) +
            " is not on the server events id's list");
    string mess = FAILURE_STR;
    sendToClient(acceptSocket, mess);
}

/*
 * register new client.
 * notice that i don't need to check if i already has this client-
 * it's his responsibility.
 */
static void handleRegister(int acceptSocket, vector<string> argFromClient) {
    string clientName;
    if (checkIfConnectionWasMade(acceptSocket, argFromClient, clientName) == SUCCESS) {
        checkSyscallServer(pthread_mutex_lock(&gClientsMutex),
                           "pthread_mutex_lock");
        gClientsSet->insert(clientName);
        checkSyscallServer(pthread_mutex_unlock(&gClientsMutex),
                           "pthread_mutex_unlock");
        writeToLog(clientName + "\twas registered successfully");
        string mess = SUCCESS_STR;
        sendToClient(acceptSocket, mess);
    }
}

/*
 * creates a new event.
 */
static void handleCreate(int acceptSocket, vector<string> argFromClient) {
    string clientName;
    if (checkIfConnectionWasMade(acceptSocket, argFromClient, clientName) == SUCCESS) {

        checkSyscallServer(pthread_mutex_lock(&gIdCounterMutex),
                           "pthread_mutex_lock");
        unsigned int eventId = gIdCounter++;
        checkSyscallServer(pthread_mutex_unlock(&gIdCounterMutex),
                           "pthread_mutex_unlock");

        string eventTitle = argFromClient[EVENT_TITLE_IN_VECTOR_INDEX];
        string eventDate = argFromClient[EVENT_DATE_IN_VECTOR_INDEX];
        string eventDescription = "";

        for (unsigned int i = STRAT_DESC_IN_VECTOR_INDEX;
             i < argFromClient.size(); ++i) {
            eventDescription += argFromClient[i];
        }

        checkSyscallServer(pthread_mutex_lock(&gEventsMutex),
                           "pthread_mutex_lock");
        gEventsMap->insert(pair<int, Event *>(eventId,
                                              new Event(eventId, eventTitle,
                                                        eventDate,
                                                        eventDescription, "")));
        checkSyscallServer(pthread_mutex_unlock(&gEventsMutex),
                           "pthread_mutex_unlock");

        writeToLog(clientName + "\tevent id " + to_string(eventId)
                                 + "was assigned to the event with title " + eventTitle);
        string mess = SUCCESS_STR;
        mess += " " + to_string(eventId);
        sendToClient(acceptSocket, mess);
    }
}

/*
 * gets the top 5 events back to the client.
 */
static void handleGetTop5(int acceptSocket, vector<string> argFromClient) {
    string clientName;
    if (checkIfConnectionWasMade(acceptSocket, argFromClient, clientName) == SUCCESS) {

        //todo write to log success

        checkSyscallServer(pthread_mutex_lock(&gIdCounterMutex),
                           "pthread_mutex_lock");
        unsigned int curEventId = gIdCounter - 1;
        checkSyscallServer(pthread_mutex_unlock(&gIdCounterMutex),
                           "pthread_mutex_unlock");

        string top5Events = "";
        checkSyscallServer(pthread_mutex_lock(&gEventsMutex),
                           "pthread_mutex_lock");
        int eventAmount = min(5, gEventsMap->size());
        checkSyscallServer(pthread_mutex_unlock(&gEventsMutex),
                           "pthread_mutex_unlock");
        for (int i = 0; i < eventAmount; ++i) {
            checkSyscallServer(pthread_mutex_lock(&gEventsMutex),
                               "pthread_mutex_lock");
            Event *event = gEventsMap->at((curEventId - i));
            checkSyscallServer(pthread_mutex_unlock(&gEventsMutex),
                               "pthread_mutex_unlock");
            top5Events +=
                    to_string(event->getId()) + "\t" + event->getEventTitle() +
                    "\t" +
                    event->getEventDate() + "\t" +
                    event->getEventDescription() +
                    "\n";
        }
        writeToLog(clientName + "\trequests the top 5 newest events");
        string mess = SUCCESS_STR;
        mess += " " + top5Events;
        sendToClient(acceptSocket, mess);
    }
}

/*
 * rsvp the current client for some event id.
 */
static void handleSendRSVP(int acceptSocket, vector<string> argFromClient) {
    string clientName;
    if (checkIfConnectionWasMade(acceptSocket, argFromClient, clientName) == SUCCESS) {

        const char *curIdStr = argFromClient[EVENT_ID_IN_VECTOR_INDEX].c_str();
        unsigned int curId;
        getPosInt(curIdStr, curId);


        checkSyscallServer(pthread_mutex_lock(&gEventsMutex),
                           "pthread_mutex_lock");
        auto eventPair = gEventsMap->find(curId);
        if (eventPair == gEventsMap->end()) {
            checkSyscallServer(pthread_mutex_unlock(&gEventsMutex),
                               "pthread_mutex_unlock");
            handleNotFoundID(acceptSocket, curId);
        }
        else {
            string mess;
            string data = clientName;
            Event *event = eventPair->second;
            string curRsvpList = event->getRsvpList();
            if (curRsvpList.find(clientName) != string::npos) {
                data += "\talready RSVP for this event";
                mess = FAILURE_STR;
            }
            else {
                string newRsvpList;
                if (curRsvpList == "") {
                    newRsvpList = clientName;
                }
                newRsvpList = curRsvpList + "," + clientName;
                event->setRsvpList(newRsvpList);

                data += "\tis RSVP to event with id " + to_string(curId);
                mess = SUCCESS_STR;
            }
            writeToLog(data);
            sendToClient(acceptSocket, mess);
        }
    }
}

/*
 * get the rsvp list back to client for an event id.
 */
static void handleGetRSVPList(int acceptSocket, vector<string> argFromClient) {
    string clientName;
    if (checkIfConnectionWasMade(acceptSocket, argFromClient, clientName) == SUCCESS) {

        const char *curIdStr = argFromClient[EVENT_ID_IN_VECTOR_INDEX].c_str();
        unsigned int curId;
        getPosInt(curIdStr, curId);

        checkSyscallServer(pthread_mutex_lock(&gEventsMutex),
                           "pthread_mutex_lock");
        auto eventPair = gEventsMap->find(curId);
        if (eventPair == gEventsMap->end()) {
            checkSyscallServer(pthread_mutex_unlock(&gEventsMutex),
                               "pthread_mutex_unlock");
            handleNotFoundID(acceptSocket, curId);
        }
        else {
            Event *event = eventPair->second;
            string rsvplist = event->getRsvpList();
            string mess = SUCCESS_STR;
            mess += " " + rsvplist;
            sendToClient(acceptSocket, mess);


            //todo check if there need to be a space after \t - this is not clear from format.
            string data = clientName + "\trequests the RSVP's list for event id " + to_string(curId);
            writeToLog(data);
        }
    }
}

/*
 * unregister a client from the server.
 */
static void handleUnregister(int acceptSocket, vector<string> argFromClient) {
    string clientName;
    if (checkIfConnectionWasMade(acceptSocket, argFromClient, clientName) == SUCCESS) {

        //todo write to log success

        checkSyscallServer(pthread_mutex_lock(&gEventsMutex),
                           "pthread_mutex_lock");
        for (auto eventPair = gEventsMap->begin();
             eventPair != gEventsMap->end(); ++eventPair) {
            Event *curEvent = eventPair->second;
            checkSyscallServer(pthread_mutex_unlock(&gEventsMutex),
                               "pthread_mutex_unlock");
            auto found = curEvent->getRsvpList().find(clientName);
            if (found != string::npos) {
                string oldRsvpList = curEvent->getRsvpList();
                string newRsvpList = oldRsvpList.erase(
                        (found, clientName.size()));
                // if there is more than 1 rsvp
                if (newRsvpList.back() == ',') {
                    newRsvpList.pop_back();
                }
                curEvent->setRsvpList(newRsvpList);
            }
            checkSyscallServer(pthread_mutex_lock(&gEventsMutex),
                               "pthread_mutex_lock");
        }
        sendToClient(acceptSocket, SUCCESS_STR);
        string data = clientName + "\twas unregistered successfully";
        writeToLog(data);
    }
}

/*
 * handle a request from the client.
 */
static void* handleRequest(void* acceptSock) {
    char sizeMessBuff[INITIAL_MESS_SIZE];
    readData((int)acceptSock, sizeMessBuff, INITIAL_MESS_SIZE);

    unsigned int sizeMess;
    getPosInt(sizeMessBuff, sizeMess);

    char requestBuff[sizeMess];
    readData((int)acceptSock, requestBuff, sizeMess);

    const string request = string(requestBuff);

    vector<string> argFromClient;
    split(request, argFromClient, ' ');

    const char* binaryCommandStr = argFromClient[COMMAND_IN_VECTOR_INDEX].c_str();

    if (strcasecmp(binaryCommandStr, REGISTER) == 0) {
        handleRegister((int) acceptSock, argFromClient);
    }
    else if (strcasecmp(binaryCommandStr, CREATE) == 0) {
        handleCreate((int) acceptSock, argFromClient);
    }
    else if (strcasecmp(binaryCommandStr, GET_TOP_5) == 0) {
        handleGetTop5((int) acceptSock, argFromClient);
    }
    else if (strcasecmp(binaryCommandStr, SEND_RSVP) == 0) {
        handleSendRSVP((int) acceptSock, argFromClient);
    }
    else if (strcasecmp(binaryCommandStr, GET_RSVPS_LIST) == 0) {
        handleGetRSVPList((int) acceptSock, argFromClient);
    }
    else {  //case UNREGISTER
        handleUnregister((int) acceptSock, argFromClient);
    }


    checkSyscallServer(pthread_mutex_lock(&gThreadsMutex), "pthread_mutex_lock");
    gThreads.erase(pthread_self());
    checkSyscallServer(pthread_mutex_unlock(&gThreadsMutex), "pthread_mutex_unlock");


    pthread_exit(NULL);
}

/*
 * Establishes server socket.
 */
static int establish(unsigned short portnum) {
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
    unsigned int portNum;
    if ((argc!=SERVER_VALID_NARGS) || (!isPosInt(argv[SERVER_PORT_INDEX],
                                                 portNum))) {
        cout << "Usage: emServer portNum" << endl;
        return 0;
    }

    try {
        gClientsSet = new unordered_set<string>();
        gEventsMap = new unordered_map<unsigned int, Event*>();
    } catch (bad_alloc) {
        cerr << "bad alloc" << endl;
        exit(EXIT_FAILURE);
    }

    int serverS = establish((unsigned short) portNum);

    gIdCounter = 1;

    int acceptSock;
    struct sockaddr client;

    thread_t keyboardThread;
    pthread_create(&keyboardThread, NULL, listenToKeyboard, &serverS);

    // after shutdown we can assume no more connects will be made.
    while ((acceptSock = accept(serverS, (struct sockaddr *) &client,
                               (socklen_t *) &client))) {

        checkSyscallServer(acceptSock, "accept");
        pthread_t requestThread;
        checkSyscallServer(pthread_create(&requestThread, NULL, handleRequest,
                                          NULL), "pthrea_create");
        checkSyscallServer(pthread_mutex_lock(&gThreadsMutex), "pthread_mutex_lock");
        gThreads.insert(requestThread);
        checkSyscallServer(pthread_mutex_unlock(&gThreadsMutex), "pthread_mutex_unlock");

    }

    //should not reach here since it should be terminated already!!!
    close(serverS);
    return 0;
}


