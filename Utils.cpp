#include <iostream>
#include <string.h>
#include <unistd.h>
#include <regex>
#include <fstream>


#define REGISTER "1"
#define CREATE "2"
#define GET_TOP_5 "3"
#define SEND_RSVP "4"
#define GET_RSVPS_LIST "5"
#define UNREGISTER "6"

#define REQUSET_SUCCESS "0"
#define REQUSET_FAILURE "1"
#define SPACE " "
#define NEWLINE "\n"
#define COMMA ","
#define COLON ":"


#define FAILURE -1
#define SUCCESS 0
#define DECIMAL 10

#define TIME_FAILURE -1


using namespace std;

//todo: server log should be protected by a mutex.
//todo: in getTime should add a 0 in case one section is just one digit?

/*
 * return the current as a string.
 */
string getTime(bool withColons) {
    time_t currentTime;
    struct tm *localTime;

    time_t seconds = time(&currentTime);                   // Get the current time
    if (seconds == (time_t) TIME_FAILURE) {
        return "couldn't get the time"; //todo what to do in this case?
    }
    localTime = localtime(&currentTime);  // Convert the current time to the local time

    int hour   = localTime->tm_hour;
    int min    = localTime->tm_min;
    int sec    = localTime->tm_sec;

    string time = to_string(hour);
    if (withColons) {
        time += COLON;
    }
    time += to_string(min);
    if (withColons) {
        time += COLON;
    }
    time += to_string(sec);
    return time;
}



/*
 * Returns true iff the string represents a positive int, and assigns the int
 * value to the given reference.
 */
bool isPosInt(char *str, unsigned int &portNum)
{
    char* end  = 0;
    int tmpCacheSize = strtol(str, &end, DECIMAL);
    portNum = (unsigned short) tmpCacheSize;

    return (*end == '\0') && (end != str) && (tmpCacheSize > 0);
}

/*
 * Returns true iff the given addr represents an address.
 * todo: check if works
 */
bool isAddress(char* addr) {
    return regex_match(addr, regex("((\\d)+\\.)+(\\d)+"));
}


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
    cerr << "socket: " << socket << endl;
    if (write(socket, cData, strlen(cData)) < 0) {
        cerr<<"failure" << endl;
        return FAILURE;
    }
    cerr << "### write finished ###" << endl;
    return SUCCESS;
}

//todo: change ret val of readData to string (the function would first read the num of chars
//todo: to read, malloc a char* buf of that size and then returns a string representaion
//todo: of that char (remember to free the buf before return).
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
    ofstream gLogFile;

    gLogFile.open(logName, ios::app);
    if (gLogFile.fail()) {
        return -errno; // todo: how handlethis case
    }

    string time = getTime(true);
    gLogFile << time << "\t" << data; //todo: without -> << "." << endl;

    // closing the log file
    gLogFile.close();

    return SUCCESS;
}

/*
 * Returns the next token in str (according to delim) and
 * erases this token from str.
 */
string popNextToken(string &str, string delim) {
    size_t pos = str.find(delim);
    if (pos == string::npos) {
        pos = str.length();
    }
    string token = str.substr(0, pos);
    str.erase(0, pos + delim.length());
    return token;
}

/*
 * Peeks token (defined by delim) in str.
 */
string peekFirstToken(string str, string delim) {
    return str.substr(0, str.find(delim));
}