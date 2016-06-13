#ifndef EX5_UTILS_H
#define EX5_UTILS_H

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

#define REQUEST_SUCCESS "0"
#define REQUEST_FAILURE "1"
#define SPACE " "
#define NEWLINE "\n"
#define COMMA ","
#define MAX_MESSAGE 5

#define DECIMAL 10






/*
 * return the current as a string.
 */
std::string getTime(bool withColons);

/*
 * Returns true iff the string represents a positive int, and assigns the int
 * value to the given reference.
 */
bool isPosInt(char *str, unsigned int &portNum);

/*
 * Returns true iff the given addr represents an address.
 * todo: check if works
 */
bool isAddress(char* addr);

/*
 * Writes data to the given log.
 */
void writeToLog(std::string logName, std::string data);

/*
 * Handles syscall failure in the server side.
 */
void syscallHandler(std::string logName, std::string funcName);

/*
 * Check syscall in the server side.
 */
void checkSyscall(std::string logName, int res, std::string funcName);

/*
 * Writes data to the given socket.
 * todo: maybe +1 to the strlen
 * todo: should write in loop
 */
void writeData(int socket, std::string data, std::string logName);

/*
 * Reads n chars from socket into buf.
 */
void readHelper(int socket, char* buf, int n, std::string logName);

/*
 * Reads data from the given socket.
 */
std::string readData(int socket, std::string logName);

/*
 * Returns the next token in str (according to delim) and
 * erases this token from str.
 */
std::string popNextToken(std::string &str, std::string delim);

/*
 * Peeks token (defined by delim) in str.
 */
std::string peekFirstToken(std::string str, std::string delim);







#endif //EX5_UTILS_H
