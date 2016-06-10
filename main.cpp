#include <iostream>

using namespace std;

int main() {

    string input, command;
    size_t pos = 0;
    bool stillRunning = true;
    while (stillRunning) {
        getline(cin, input);
        pos = input.find(" ");
        command = input.substr(0,pos);
        cout << "!!! " << command << " !!!" << endl;

        stillRunning = false;
    }


    return 0;
}