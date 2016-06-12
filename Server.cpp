#include <netinet/in.h>
#include <string.h>
#include <netdb.h>
#include <unistd.h>
#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "Server.h"
#include "Utils.h"
#include <unordered_set>
#include <unordered_map>
#include <thread_db.h>


#define MAX_HOST_NAME 40    //todo
#define N_PEDING_CLIENTS 10
#define SERVER_VALID_NARGS 2
#define SERVER_PORT_INDEX 1
#define EXIT_FROM_KEYBOARD "EXIT"
#define COMMAND_IN_VECTOR_INDEX 0
#define CLIENT_NAME_IN_VECTOR_INDEX 1

#define EVENT_TITLE_IN_VECTOR_INDEX 2
#define EVENT_DATE_IN_VECTOR_INDEX 3
#define START_DESC_IN_VECTOR_INDEX 4

#define EVENT_ID_IN_VECTOR_INDEX 2

#define LOG_FILENAME "emServer.log"


static void* listenToKeyboard(void*);
static int establish(unsigned short portnum);
static void* handleRequest(void* acceptSock);
static void terminateServer(int* serverS);


using namespace std;

class Event;

//unordered_set <string>* gClientsSet;
unordered_map <unsigned int, Event*>* gEventsMap;
unordered_set <thread_t> gThreads;
unsigned int gIdCounter;

//pthread_mutex_t gClientsMutex;
pthread_mutex_t gEventsMutex;
pthread_mutex_t gThreadsMutex;
pthread_mutex_t gIdCounterMutex;
pthread_mutex_t gLogFileMutex;



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
    checkSyscall(LOG_FILENAME, pthread_mutex_lock(&gLogFileMutex), "pthread_mutex_lock");
    writeToLog(LOG_FILENAME, data);
    checkSyscall(LOG_FILENAME, pthread_mutex_unlock(&gLogFileMutex), "pthread_mutex_unlock");
}

/*
 * split lines by char.
 */
void split(const string &txt, vector<std::string> &strs, char ch) {
    size_t pos = txt.find(ch), initialPos = 0;
    strs.clear();

    // Decompose statement
    while( pos != std::string::npos) {
        strs.push_back(txt.substr(initialPos, pos - initialPos + 1 ) );
        initialPos = pos + 1;

        pos = txt.find(ch, initialPos);
    }

    // Add the last one
    strs.push_back(txt.substr(initialPos, min(pos, txt.size()) - initialPos + 1));
}


///*
// * Handles syscall failure in the server side.
// */
//static void syscallHandler(string funcName) {
//    writeToLog("ERROR\t" + funcName + "\t" + to_string(errno));
//    exit(EXIT_FAILURE);
//}
//
///*
// * Check syscall in the server side.
// */
//static void checkSyscall(int res, string funcName) {
//    if (res < 0) {
//        syscallHandler(funcName);
//    }
//}

/*
 * listen to keyboard until it gets 'EXIT' and terminate the program.
 */
static void* listenToKeyboard(void* serverS) {
    string input;

    bool stillRunning = true;
    while(stillRunning) {
        getline(cin, input);
        if(strcasecmp(input.c_str(), EXIT_FROM_KEYBOARD) == 0) {
            stillRunning = false;
        }
    }
    terminateServer((int*) serverS);
    return NULL;
}

/*
 * terminates the program.
 */
static void terminateServer(int* serverS) {
    for (thread_t thread : gThreads) {
        checkSyscall(LOG_FILENAME, pthread_join(thread, NULL), "pthread_join");
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
    writeData(accSocket,mess,LOG_FILENAME);
}

/*
 * check if the current client already registered to the server.
 */
//static int checkIfConnectionWasMade(int acceptSocket, vector<string> argFromClient, string& clientName) {
//
//    checkSyscall(pthread_mutex_lock(&gClientsMutex), "pthread_mutex_lock");
//    if (gClientsSet->find(argFromClient[CLIENT_NAME_IN_VECTOR_INDEX]) == gClientsSet->end()) {
//    checkSyscall(pthread_mutex_unlock(&gClientsMutex), "pthread_mutex_unlock");
//
//        clientName = argFromClient[CLIENT_NAME_IN_VECTOR_INDEX];
//        writeToLog("ERROR:" + clientName + "\tis already exists");
//        sendToClient(acceptSocket, );
//        return FAILURE;
//    }
//    return SUCCESS;
//
//}

/*
 * handles the situation when where the client asked an event id that isn't
 * exists.
 */
static void handleNotFoundID(int acceptSocket, int curId) {
    writeToLog("event id: " + to_string(curId) +
            " is not on the server events id's list");
    string mess = REQUEST_FAILURE;
    sendToClient(acceptSocket, mess);
}

/*
 * register new client.
 * notice that i don't need to check if i already has this client-
 * it's his responsibility.
 */
static void handleRegister(int acceptSocket, vector<string> argFromClient) {
    string clientName = argFromClient[CLIENT_NAME_IN_VECTOR_INDEX];
    writeToLog(clientName + "\twas registered successfully");
    string mess = REQUEST_SUCCESS;
    sendToClient(acceptSocket, mess);
}

/*
 * creates a new event.
 */
static void handleCreate(int acceptSocket, vector<string> argFromClient) {
    string clientName = argFromClient[CLIENT_NAME_IN_VECTOR_INDEX];

    checkSyscall(LOG_FILENAME, pthread_mutex_lock(&gIdCounterMutex),
                       "pthread_mutex_lock");
    unsigned int eventId = gIdCounter++;
    checkSyscall(LOG_FILENAME, pthread_mutex_unlock(&gIdCounterMutex),
                       "pthread_mutex_unlock");

    string eventTitle = argFromClient[EVENT_TITLE_IN_VECTOR_INDEX];
    string eventDate = argFromClient[EVENT_DATE_IN_VECTOR_INDEX];
    string eventDescription = "";

    for (unsigned int i = START_DESC_IN_VECTOR_INDEX;
         i < argFromClient.size(); ++i) {
        eventDescription += argFromClient[i];
    }

    checkSyscall(LOG_FILENAME, pthread_mutex_lock(&gEventsMutex),
                       "pthread_mutex_lock");
    gEventsMap->insert(pair<int, Event *>(eventId,
                                          new Event(eventId, eventTitle,
                                                    eventDate,
                                                    eventDescription, "")));
    checkSyscall(LOG_FILENAME, pthread_mutex_unlock(&gEventsMutex),
                       "pthread_mutex_unlock");

    writeToLog(clientName + "\tevent id " + to_string(eventId)
                             + "was assigned to the event with title " + eventTitle);
    string mess = REQUEST_SUCCESS;
    mess += " " + to_string(eventId);
    sendToClient(acceptSocket, mess);
}

/*
 * gets the top 5 events back to the client.
 */
static void handleGetTop5(int acceptSocket, vector<string> argFromClient) {
    string clientName = argFromClient[CLIENT_NAME_IN_VECTOR_INDEX];

    //todo write to log success

    checkSyscall(LOG_FILENAME, pthread_mutex_lock(&gIdCounterMutex),
                       "pthread_mutex_lock");
    unsigned int curEventId = gIdCounter - 1;
    checkSyscall(LOG_FILENAME, pthread_mutex_unlock(&gIdCounterMutex),
                       "pthread_mutex_unlock");

    string top5Events = "";
    checkSyscall(LOG_FILENAME, pthread_mutex_lock(&gEventsMutex),
                       "pthread_mutex_lock");
    int eventAmount = min(5, (int) gEventsMap->size());
    checkSyscall(LOG_FILENAME, pthread_mutex_unlock(&gEventsMutex),
                       "pthread_mutex_unlock");
    for (int i = 0; i < eventAmount; ++i) {
        checkSyscall(LOG_FILENAME, pthread_mutex_lock(&gEventsMutex),
                           "pthread_mutex_lock");
        Event *event = gEventsMap->at((curEventId - i));
        checkSyscall(LOG_FILENAME, pthread_mutex_unlock(&gEventsMutex),
                           "pthread_mutex_unlock");
        top5Events +=
                to_string(event->getId()) + "\t" + event->getEventTitle() +
                "\t" +
                event->getEventDate() + "\t" +
                event->getEventDescription() +
                "\n";
    }
    writeToLog(clientName + "\trequests the top 5 newest events");
    string mess = REQUEST_SUCCESS;
    mess += " " + top5Events;
    sendToClient(acceptSocket, mess);
}

/*
 * rsvp the current client for some event id.
 */
static void handleSendRSVP(int acceptSocket, vector<string> argFromClient) {
    string clientName = argFromClient[CLIENT_NAME_IN_VECTOR_INDEX];

    const char *curIdStr = argFromClient[EVENT_ID_IN_VECTOR_INDEX].c_str();
    unsigned int curId = (unsigned int) stoi(curIdStr);


    checkSyscall(LOG_FILENAME, pthread_mutex_lock(&gEventsMutex),
                       "pthread_mutex_lock");
    auto eventPair = gEventsMap->find(curId);
    if (eventPair == gEventsMap->end()) {
        checkSyscall(LOG_FILENAME, pthread_mutex_unlock(&gEventsMutex),
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
            mess = REQUEST_FAILURE;
        }
        else {
            string newRsvpList;
            if (strcasecmp(newRsvpList.c_str(), "") == 0) {
                newRsvpList = clientName;
            }
            newRsvpList = curRsvpList + "," + clientName;
            event->setRsvpList(newRsvpList);

            data += "\tis RSVP to event with id " + to_string(curId);
            mess = REQUEST_SUCCESS;
        }
        writeToLog(data);
        sendToClient(acceptSocket, mess);
    }
}

/*
 * get the rsvp list back to client for an event id.
 */
static void handleGetRSVPList(int acceptSocket, vector<string> argFromClient) {
    string clientName = argFromClient[CLIENT_NAME_IN_VECTOR_INDEX];

    const char *curIdStr = argFromClient[EVENT_ID_IN_VECTOR_INDEX].c_str();
    unsigned int curId = (unsigned int) stoi(curIdStr);

    checkSyscall(LOG_FILENAME, pthread_mutex_lock(&gEventsMutex),
                       "pthread_mutex_lock");
    auto eventPair = gEventsMap->find(curId);
    if (eventPair == gEventsMap->end()) {
        checkSyscall(LOG_FILENAME, pthread_mutex_unlock(&gEventsMutex),
                           "pthread_mutex_unlock");
        handleNotFoundID(acceptSocket, curId);
    }
    else {
        Event *event = eventPair->second;
        string rsvplist = event->getRsvpList();
        string mess = REQUEST_SUCCESS;
        mess += " " + rsvplist;
        sendToClient(acceptSocket, mess);


        //todo check if there need to be a space after \t - this is not clear from format.
        string data = clientName + "\trequests the RSVP's list for event id " + to_string(curId);
        writeToLog(data);
    }
}

/*
 * unregister a client from the server.
 */
static void handleUnregister(int acceptSocket, vector<string> argFromClient) {
    string clientName = argFromClient[CLIENT_NAME_IN_VECTOR_INDEX];

    //todo write to log success

    checkSyscall(LOG_FILENAME, pthread_mutex_lock(&gEventsMutex), "pthread_mutex_lock");

    for (auto eventPair = gEventsMap->begin();
         eventPair != gEventsMap->end(); ++eventPair) {
        Event *curEvent = eventPair->second;
        auto found = curEvent->getRsvpList().find(clientName);
        if (found != string::npos) {
            string oldRsvpList = curEvent->getRsvpList();
            string newRsvpList = oldRsvpList.erase(found, clientName.size());
            // if there is more than 1 rsvp
            if (newRsvpList.back() == ',') {
                newRsvpList.pop_back();
            }
            curEvent->setRsvpList(newRsvpList);
        }
    }

    checkSyscall(LOG_FILENAME, pthread_mutex_unlock(&gEventsMutex), "pthread_mutex_unlock");
    sendToClient(acceptSocket, REQUEST_SUCCESS);
    string data = clientName + "\twas unregistered successfully";
    writeToLog(data);
}

/*
 * handle a request from the client.
 */
static void* handleRequest(void* acceptSock) {
//    char sizeMessBuff[INITIAL_MESS_SIZE];
//    readData((int)acceptSock, sizeMessBuff, INITIAL_MESS_SIZE);
//
//    unsigned int sizeMess;
//    getPosInt(sizeMessBuff, sizeMess);
//
//    char requestBuff[sizeMess];

    string request = readData(*((int*)acceptSock), LOG_FILENAME);

    vector<string> argFromClient;
    split(request, argFromClient, ' ');

    const char* binaryCommandStr = argFromClient[COMMAND_IN_VECTOR_INDEX].c_str();

    if (strcasecmp(binaryCommandStr, REGISTER) == 0) {
        handleRegister(*((int*)acceptSock), argFromClient);
    }
    else if (strcasecmp(binaryCommandStr, CREATE) == 0) {
        handleCreate(*((int*)acceptSock), argFromClient);
    }
    else if (strcasecmp(binaryCommandStr, GET_TOP_5) == 0) {
        handleGetTop5(*((int*)acceptSock), argFromClient);
    }
    else if (strcasecmp(binaryCommandStr, SEND_RSVP) == 0) {
        handleSendRSVP(*((int*)acceptSock), argFromClient);
    }
    else if (strcasecmp(binaryCommandStr, GET_RSVPS_LIST) == 0) {
        handleGetRSVPList(*((int*)acceptSock), argFromClient);
    }
    else {  //case UNREGISTER
        handleUnregister(*((int*)acceptSock), argFromClient);
    }


    checkSyscall(LOG_FILENAME, pthread_mutex_lock(&gThreadsMutex), "pthread_mutex_lock");
    gThreads.erase(pthread_self());
    checkSyscall(LOG_FILENAME, pthread_mutex_unlock(&gThreadsMutex), "pthread_mutex_unlock");


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
    checkSyscall(LOG_FILENAME, gethostname(myName, MAX_HOST_NAME), "gethostname");
    hp = gethostbyname(myName);
    if (hp == NULL) {
        syscallHandler(LOG_FILENAME, "gethostbyname");
    }

    sa.sin_family = hp->h_addrtype;
    memcpy(&sa.sin_addr,hp->h_addr,hp->h_length);
    sa.sin_port = htons(portnum);

    serverS = socket(AF_INET,SOCK_STREAM,0);
    checkSyscall(LOG_FILENAME, serverS, "socket");

    checkSyscall(LOG_FILENAME, bind(serverS, (struct sockaddr*)&sa, sizeof(sa)), "bind");
    checkSyscall(LOG_FILENAME, listen(serverS, N_PEDING_CLIENTS), "listen");

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
//        gClientsSet = new unordered_set<string>();
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

        checkSyscall(LOG_FILENAME, acceptSock, "accept");
        pthread_t requestThread;
        checkSyscall(LOG_FILENAME, pthread_create(&requestThread, NULL, handleRequest, &acceptSock), "pthrea_create");
        checkSyscall(LOG_FILENAME, pthread_mutex_lock(&gThreadsMutex), "pthread_mutex_lock");
        gThreads.insert(requestThread);
        checkSyscall(LOG_FILENAME, pthread_mutex_unlock(&gThreadsMutex), "pthread_mutex_unlock");

    }

    //should not reach here since it should be terminated already!!!
    close(serverS);
    return 0;
}


