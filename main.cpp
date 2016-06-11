#include <iostream>
#include <algorithm>
#include <regex>

#define COLON ":"

using namespace std;

bool isAddress(char* addr) {
    return regex_match(addr, regex("((\\d)+\\.)+(\\d)+"));
}

int main() {
    char s[] = "23.4.2.-2";
    cout << isAddress(s) << endl;
    return 0;
}