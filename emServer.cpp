#include <netinet/in.h>
#include <string.h>
#include <netdb.h>
#include <unistd.h>
#include <iostream>
#include <arpa/inet.h>
#include "Utils.h"
#include <unordered_set>
#include <unordered_map>
#include <thread_db.h>
#include <set>



#define MAX_HOST_NAME 40
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
unordered_map <unsigned int, Event*> gEventsMap;
unordered_set <thread_t> gThreads;
unordered_set <string> gClientNames;
unsigned int gIdCounter;

//pthread_mutex_t gClientsMutex;
pthread_mutex_t gEventsMutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t gThreadsMutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t gIdCounterMutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t gLogFileMutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t gClientNamesMutex = PTHREAD_MUTEX_INITIALIZER;



/*
 * class describing event.
 */
class Event {
    unsigned int id;
    string eventTitle;
    string eventDate;
    string eventDescription;

public:
    Event(unsigned int id, const string &eventTitle, const string &eventDate,
          const string &eventDescription,
          const string &rsvpList) : id(id), eventTitle(eventTitle), eventDate(
            eventDate), eventDescription(eventDescription) {}

    set<string> rsvpList;

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

};

/*
 * write the data to the log using utils function.
 */
static void writeToLog(string data) {
    checkSyscall(LOG_FILENAME, pthread_mutex_lock(&gLogFileMutex),
                 "pthread_mutex_lock");
    writeToLog(LOG_FILENAME, data);
    checkSyscall(LOG_FILENAME, pthread_mutex_unlock(&gLogFileMutex),
                 "pthread_mutex_unlock");
}

/*
 * split lines by char.
 */
void split(const string &txt, vector<std::string> &strs, char ch) {
    size_t pos = txt.find(ch), initialPos = 0;
    strs.clear();

    // Decompose statement
    while( pos != std::string::npos) {
        strs.push_back(txt.substr(initialPos, pos - initialPos ) );
        initialPos = pos + 1;

        pos = txt.find(ch, initialPos);
    }

    if (txt.substr(initialPos, min(pos, txt.size()) - initialPos) == "") {
        return;
    }
    // Add the last one
    strs.push_back(txt.substr(initialPos, min(pos, txt.size()) - initialPos));
}

/*
 * listen to keyboard until it gets 'EXIT' and terminate the program.
 */
static void* listenToKeyboard(void* serverS) {
    string input;

    while (true) {
        getline(cin, input);
        if(strcasecmp(input.c_str(), EXIT_FROM_KEYBOARD) == 0) {
            break;
        }
    }
    terminateServer((int*) serverS);
    return NULL;
}

/*
 * terminates the program.
 */
static void terminateServer(int* serverS) {
    checkSyscall(LOG_FILENAME, pthread_mutex_lock(&gThreadsMutex),
                 "pthread_mutex_lock");
    // finish handling requests
    for (thread_t thread : gThreads) {
        checkSyscall(LOG_FILENAME, pthread_join(thread, NULL), "pthread_join");
    }

    checkSyscall(LOG_FILENAME, pthread_mutex_unlock(&gThreadsMutex),
                 "pthread_mutex_unlock");

    close(*serverS);

    // deleting events
    checkSyscall(LOG_FILENAME, pthread_mutex_lock(&gEventsMutex),
                 "pthread_mutex_lock");
    for (auto it = gEventsMap.begin(); it != gEventsMap.end(); ++it) {
        delete it->second;
    }

    checkSyscall(LOG_FILENAME, pthread_mutex_unlock(&gEventsMutex),
                 "pthread_mutex_unlock");
    writeToLog("EXIT command is typed: server is shutdown");

    exit(EXIT_SUCCESS);
}

/*
 * send a message to client through the socket (first the number of char)
 * and than the message itself.
 */
static void sendToClient(int accSocket, string mess) {
    writeData(accSocket,mess,LOG_FILENAME);
}

/*
 * handles the situation when where the client asked an event id that isn't
 * exists.
 */
static void handleNotFoundID(int acceptSocket, int curId) {
    writeToLog("ERROR: event id: " + to_string(curId) +
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
    string mess, data;
    checkSyscall(LOG_FILENAME, pthread_mutex_lock(&gClientNamesMutex),
                 "pthread_mutex_lock");
    if (gClientNames.find(clientName) != gClientNames.end()) {
        mess = REQUEST_FAILURE;
        data = "ERROR: " + clientName + "\tis already exists";
    }
    else {
        gClientNames.insert(clientName);
        mess = REQUEST_SUCCESS;
        data = clientName + "\twas registered successfully";
    }
    checkSyscall(LOG_FILENAME, pthread_mutex_unlock(&gClientNamesMutex),
                 "pthread_mutex_unlock");
    writeToLog(data);
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
        eventDescription += argFromClient[i] + SPACE;
    }
    if (eventDescription != "") {
        eventDescription.pop_back();
    }


    checkSyscall(LOG_FILENAME, pthread_mutex_lock(&gEventsMutex),
                       "pthread_mutex_lock");
    gEventsMap.insert(pair<int, Event *>(eventId,
                                          new Event(eventId, eventTitle,
                                                    eventDate,
                                                    eventDescription, "")));
    checkSyscall(LOG_FILENAME, pthread_mutex_unlock(&gEventsMutex),
                       "pthread_mutex_unlock");

    writeToLog(clientName + "\tevent id " + to_string(eventId)
                             + " was assigned to the event with title " +
                       eventTitle);
    string mess = REQUEST_SUCCESS;
    mess += SPACE + to_string(eventId);
    sendToClient(acceptSocket, mess);
}

/*
 * gets the top 5 events back to the client.
 */
static void handleGetTop5(int acceptSocket, vector<string> argFromClient) {
    string clientName = argFromClient[CLIENT_NAME_IN_VECTOR_INDEX];

    checkSyscall(LOG_FILENAME, pthread_mutex_lock(&gIdCounterMutex),
                       "pthread_mutex_lock");
    unsigned int curEventId = gIdCounter - 1;
    checkSyscall(LOG_FILENAME, pthread_mutex_unlock(&gIdCounterMutex),
                       "pthread_mutex_unlock");

    string top5Events = "";
    checkSyscall(LOG_FILENAME, pthread_mutex_lock(&gEventsMutex),
                       "pthread_mutex_lock");
    int eventAmount = min(5, (int) gEventsMap.size());
    checkSyscall(LOG_FILENAME, pthread_mutex_unlock(&gEventsMutex),
                       "pthread_mutex_unlock");
    for (int i = 0; i < eventAmount; ++i) {
        checkSyscall(LOG_FILENAME, pthread_mutex_lock(&gEventsMutex),
                           "pthread_mutex_lock");
        Event *event = gEventsMap.at((curEventId - i));
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
    mess += SPACE + top5Events;
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
    auto eventPair = gEventsMap.find(curId);
    if (eventPair == gEventsMap.end()) {
        handleNotFoundID(acceptSocket, curId);
    }
    else {
        string mess;
        string data = clientName;
        Event *event = eventPair->second;
        event->rsvpList.insert(clientName);

        data += "\tis RSVP to event with id " + to_string(curId);
        mess = REQUEST_SUCCESS;

        writeToLog(data);
        sendToClient(acceptSocket, mess);
    }
    checkSyscall(LOG_FILENAME, pthread_mutex_unlock(&gEventsMutex),
                       "pthread_mutex_unlock");
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


    auto eventPair = gEventsMap.find(curId);



    if (eventPair == gEventsMap.end()) {
        handleNotFoundID(acceptSocket, curId);
    }
    else {
        Event *event = eventPair->second;

        string rsvplist = "";
        if (event->rsvpList.size() > 0) {
            for (string rsvp : event->rsvpList) {
                rsvplist += rsvp + COMMA;
            }
                rsvplist.pop_back();
        }

        string mess = REQUEST_SUCCESS;
        mess += " " + rsvplist;
        sendToClient(acceptSocket, mess);

        string data = clientName + "\trequests the RSVP's list for event id " +
                to_string(curId);
        writeToLog(data);
    }
    checkSyscall(LOG_FILENAME, pthread_mutex_unlock(&gEventsMutex),
                       "pthread_mutex_unlock");
}

/*
 * unregister a client from the server.
 */
static void handleUnregister(int acceptSocket, vector<string> argFromClient) {
    string clientName = argFromClient[CLIENT_NAME_IN_VECTOR_INDEX];

    checkSyscall(LOG_FILENAME, pthread_mutex_lock(&gClientNamesMutex),
                 "pthread_mutex_lock");
    gClientNames.erase(clientName);
    checkSyscall(LOG_FILENAME, pthread_mutex_unlock(&gClientNamesMutex),
                 "pthread_mutex_unlock");

    checkSyscall(LOG_FILENAME, pthread_mutex_lock(&gEventsMutex),
                 "pthread_mutex_lock");


    for (auto eventPair = gEventsMap.begin();
         eventPair != gEventsMap.end(); ++eventPair) {
        Event *curEvent = eventPair->second;
        curEvent->rsvpList.erase(clientName);
    }

    checkSyscall(LOG_FILENAME, pthread_mutex_unlock(&gEventsMutex),
                 "pthread_mutex_unlock");
    sendToClient(acceptSocket, REQUEST_SUCCESS);
    string data = clientName + "\twas unregistered successfully";
    writeToLog(data);
}

/*
 * handle a request from the client.
 */
static void* handleRequest(void* acceptSock) {
    string request = readData(*((int*)acceptSock), LOG_FILENAME);

    vector<string> argFromClient;
    split(request, argFromClient, ' ');
    const char* binaryCommandStr =
            argFromClient[COMMAND_IN_VECTOR_INDEX].c_str();

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
    else if (strcasecmp(binaryCommandStr, UNREGISTER) == 0) {
        handleUnregister(*((int*)acceptSock), argFromClient);
    }


    checkSyscall(LOG_FILENAME, pthread_mutex_lock(&gThreadsMutex),
                 "pthread_mutex_lock");
    gThreads.erase(pthread_self());
    checkSyscall(LOG_FILENAME, pthread_mutex_unlock(&gThreadsMutex),
                 "pthread_mutex_unlock");

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
    checkSyscall(LOG_FILENAME, gethostname(myName, MAX_HOST_NAME),
                 "gethostname");
    hp = gethostbyname(myName);
    if (hp == NULL) {
        syscallHandler(LOG_FILENAME, "gethostbyname");
    }

    sa.sin_family = hp->h_addrtype;
    memcpy(&sa.sin_addr,hp->h_addr,hp->h_length);
    sa.sin_port = htons(portnum);

    cerr << "addr: " << inet_ntoa(sa.sin_addr) << endl;//todo

    serverS = socket(AF_INET,SOCK_STREAM,0);
    checkSyscall(LOG_FILENAME, serverS, "socket");

    checkSyscall(LOG_FILENAME, bind(serverS, (struct sockaddr*)&sa,
                                    sizeof(sa)), "bind");
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

    int serverS = establish((unsigned short) portNum);

    gIdCounter = 1;

    int acceptSock;
    struct sockaddr client;

    thread_t keyboardThread;
    checkSyscall(LOG_FILENAME,pthread_create(&keyboardThread, NULL,
                 listenToKeyboard, &serverS),"pthread_create");

    // after shutdown we can assume no more connects will be made.
    bool run = true;
    while (run) {
        acceptSock = accept(serverS, (struct sockaddr*) &client,
                            (socklen_t *) &client) ;
        checkSyscall(LOG_FILENAME, acceptSock, "accept");
        pthread_t requestThread;
        checkSyscall(LOG_FILENAME, pthread_mutex_lock(&gThreadsMutex),
                     "pthread_mutex_lock");
        checkSyscall(LOG_FILENAME, pthread_create(&requestThread, NULL,
                     handleRequest, &acceptSock), "pthread_create");
        gThreads.insert(requestThread);
        checkSyscall(LOG_FILENAME, pthread_mutex_unlock(&gThreadsMutex),
                     "pthread_mutex_unlock");
    }
}


