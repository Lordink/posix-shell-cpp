#include <iostream>
#include <sstream>
#include <string>
#include <vector>

using std::cerr;
using std::cout;
using std::endl;
using std::getline;
using std::string;

int main() {
    // Flush after every std::cout / std:cerr

    while (true) {
        cout << std::unitbuf;
        cerr << std::unitbuf;
        cout << "$ ";

        string input;
        getline(std::cin, input);

        // Break into words:
        string word;
        std::istringstream iss(input, std::istringstream::in);
        std::vector<string> words;

        while (iss >> word) {
            words.push_back(word);
        }

        if (input == "exit 0") {
            break;
        } else if (words.size() > 1 && words[0] == "echo") {
            for (size_t i = 1; i < words.size(); i++) {
                cout << words[i] << ' ';
            }
            cout << endl;
        } else {
            cout << input << ": command not found" << endl;
        }
    }
}
