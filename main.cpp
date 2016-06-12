#include <iostream>
#include <algorithm>
#include <regex>

#define COLON ":"

using namespace std;

static string padDataSize(string dataSize) {
    int nZeroes =  (int) (5 - strlen(dataSize.c_str()));
    if (nZeroes < 0) {      // shouldn't reach here.
        return "";
    }
    string padZeroes = "";
    for (int i = 0; i < nZeroes; ++i) {
        padZeroes += "0";
    }
    return padZeroes + dataSize;
}



int main() {

    string s = "bb324a";

    cout << stoi(s) << endl;
    return 0;
}