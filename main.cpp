#include <iostream>
#include <algorithm>
#include <regex>

#define COLON ":"

using namespace std;

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

int main() {
    cout << "with: " <<getTime(0) << endl;
    cout << "without: " <<getTime(1) << endl;
    return 0;
}