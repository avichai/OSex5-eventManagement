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

pthread_mutex_t gClientsMutex;
pthread_mutex_t gEventsMutex;
pthread_mutex_t gThreadsMutex;


bool gShouldExit;



class Event {
    unsigned int id;
    string eventTitle;
    string eventDate;
    string eventDescription;

public:
    Event(unsigned int id, const string &eventTitle, const string &eventDate,
          const string &eventDescription) : id(id), eventTitle(eventTitle),
                                            eventDate(eventDate),
                                            eventDescription(
                                                    eventDescription) { }

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


/**/
/*
 * Returns true iff the string represents a positive int, and assigns the int
 * value to the given reference.
 */
static bool getPosInt(char* str, unsigned int &sizeMess)
{
    char* end  = 0;
    int tmpSizeMess = strtol(str, &end, DECIMAL);
    sizeMess = (unsigned short) tmpSizeMess;
}

/*
 * Handles syscall failure in the server side.
 */
static void syscallHandler(string funcName) {
    cerr<<"handler" << funcName << endl;
    // todo write to log
    exit(1);
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

static void* listenToKeyboard(void*) {
    string input, command;
    size_t pos;

    bool stillRunning = true;
    while(stillRunning) {
        getline(cin, input);
        if (input == "EXIT") {
            stillRunning = false;
        }
    }
        terminateServer();
}

static void terminateServer() {
    for (thread_t thread : gThreads) {
        checkSyscallServer(pthread_join(thread, NULL), "pthread_join");
    }
    pthread_exit(NULL);
}

static void* handleRequest(void* acceptSock) {
    char sizeMessBuff[5];
    readData((int)acceptSock, sizeMessBuff, 5);

    unsigned int sizeMess;
    getPosInt(sizeMessBuff, sizeMess);
//    checkSyscallServer(pthread_mutex_lock(&gThreadsMutex), "pthread_mutex_lock");
//    gThreads.erase(pthread_self());
//    checkSyscallServer(pthread_mutex_unlock(&gThreadsMutex), "pthread_mutex_unlock");
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
    unsigned short portNum;
    if ((argc!=SERVER_VALID_NARGS) || (!isPosInt(argv[SERVER_PORT_INDEX], portNum))) {
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

    int serverS = establish(portNum);

    gShouldExit = false;

    int acceptSock;
    struct sockaddr client;

    thread_t keyboardThread;
    pthread_create(&keyboardThread, NULL, listenToKeyboard, NULL);

    // after shutdown we can assume no more connects will be made.
    while (acceptSock = accept(serverS, (struct sockaddr *) &client,
                               (socklen_t *) &client)) {

        checkSyscallServer(acceptSock, "accept");
        pthread_t requestThread;
        checkSyscallServer(pthread_create(&requestThread, NULL, handleRequest,
                                          NULL), "pthrea_create");
        gThreads.insert(requestThread);

    }


    close(serverS);
    return 0;
}


