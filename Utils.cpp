#include "Utils.h"


#define REGISTER "1"
#define CREATE "2"
#define GET_TOP_5 "3"
#define SEND_RSVP "4"
#define GET_RSVPS_LIST "5"
#define UNREGISTER "6"

#define REQUEST_SUCCESS "0"
#define REQUEST_FAILURE "1"
#define SPACE " "
#define NEWLINE "\n"
#define COMMA ","
#define MAX_MESSAGE 5

#define DECIMAL 10


using namespace std;



/*
 * return the current as a string.
 */
string getTime(bool withColons) {
    time_t currentTime;
    struct tm *localTime;
    char str[9];

    time(&currentTime);
    localTime = localtime(&currentTime);

    string format = withColons ? "%H:%M:%S" : "%H%M%S";
    strftime(str, 9, format.c_str(), localTime);
    return string(str);
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
 */
bool isAddress(char* addr) {
    return regex_match(addr, regex("((\\d)+\\.)+(\\d)+"));
}

/*
 * Writes data to the given log.
 */
void writeToLog(string logName, string data) {
    ofstream gLogFile;

    gLogFile.open(logName, ios::app);
    if (gLogFile.fail()) {
        return;
    }

    gLogFile << getTime(true) << "\t" << data << ".\n";
    cerr << getTime(true) << "\t" << data << ".\n"; //todo: remove
    gLogFile.close();
}

/*
 * Handles syscall failure in the server side.
 */
void syscallHandler(string logName, string funcName) {
    writeToLog(logName, "ERROR\t" + funcName + "\t" + to_string(errno));
    exit(1);
}

/*
 * Check syscall in the server side.
 */
void checkSyscall(string logName, int res, string funcName) {
    if (res < 0) {
        syscallHandler(logName, funcName);
    }
}

/*
 * Pad the data's size to fit the protocol (assuming each message is of
 * length 99,999 chars).
 */
static string padDataSize(string dataSize) {
    int nZeroes =  (int) (MAX_MESSAGE - strlen(dataSize.c_str()));
    if (nZeroes < 0) {      // shouldn't reach here.
        return "";
    }
    string padZeroes = "";
    for (int i = 0; i < nZeroes; ++i) {
        padZeroes += "0";
    }
    return padZeroes + dataSize;
}

/*
 * Writes data to the given socket.
 */
void writeData(int socket, string data, string logName) {
    int dataSize = (int) data.size() + 1;  // for the null char.

    string proData = padDataSize(to_string(dataSize)) + data;
    dataSize = (int) proData.size() + 1;  // for the null char.
    const char* cProData = proData.c_str();

    int charsWritten = 0, tmpChars;
    while (charsWritten < dataSize) {
        if ((tmpChars = (int) write(socket, cProData, 
                                    (size_t) dataSize - charsWritten)) > 0) {
            charsWritten += tmpChars;
            cProData += tmpChars;
        }

        if (tmpChars < 0) {
            syscallHandler(logName,"write");
        }
    }
}

/*
 * Reads n chars from socket into buf.
 */
void readHelper(int socket, char* buf, int n, string logName) {
    int charsRead = 0, tmpChars;
    while (charsRead < n) {
        if ((tmpChars = (int) read(socket, buf, (size_t) n - charsRead)) > 0) {
            charsRead += tmpChars;
            buf += tmpChars;
        }

        if (tmpChars < 0) {
            syscallHandler(logName, "read");
        }
    }
}

/*
 * Reads data from the given socket.
 */
string readData(int socket, string logName) {
    char messageSizeStr[MAX_MESSAGE];
    readHelper(socket,messageSizeStr,MAX_MESSAGE,logName);


    int messageSize = stoi(messageSizeStr);

    char* message = (char*) malloc(messageSize);
    readHelper(socket,message,messageSize,logName);
    string messageStr = string(message);
    free(message);
    return messageStr;
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
