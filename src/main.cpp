#include <iostream>
#include <string>

using std::cerr;
using std::cout;
using std::endl;
using std::getline;

int main() {
    // Flush after every std::cout / std:cerr

    while (true) {
        cout << std::unitbuf;
        cerr << std::unitbuf;
        cout << "$ ";

        std::string input;
        getline(std::cin, input);

        if (input == "exit 0") {
            break;
        } else {
            cout << input << ": command not found" << endl;
        }
    }
}
