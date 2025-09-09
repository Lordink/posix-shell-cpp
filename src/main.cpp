#include <iostream>
#include <sstream>
#include <string>
#include <unordered_set>
#include <vector>

using std::cerr;
using std::cout;
using std::endl;
using std::getline;
using std::string;

// Used for "type" command only
const std::unordered_set<string> builtins = {"echo", "exit", "type"};

std::vector<string> into_words(const string &input) {
    string word;
    std::istringstream iss(input, std::istringstream::in);
    std::vector<string> words;

    while (iss >> word) {
        words.push_back(word);
    }

    return words;
}

int main() {
    // Flush after every std::cout / std:cerr

    while (true) {
        cout << std::unitbuf;
        cerr << std::unitbuf;
        cout << "$ ";

        string input;
        getline(std::cin, input);

        // Break into words:
        const auto words = into_words(input);

        if (input == "exit 0") {
            break;
        } else if (words.size() > 1) {
            if (words[0] == "echo") {
                for (size_t i = 1; i < words.size(); i++) {
                    cout << words[i] << ' ';
                }
                cout << endl;
            } else if (words[0] == "type") {
                string all_args = words[1];

                // add the rest
                if (words.size() > 2) {
                    for (size_t i = 2; i < words.size(); i++) {
                        all_args.append(" " + words[i]);
                    }
                }

                // Could be nicer if using cpp20, but this is fine for now
                if (builtins.find(all_args) != builtins.end()) {
                    cout << all_args << " is a shell builtin" << endl;
                } else {
                    cout << all_args << ": not found" << endl;
                }
            }
        } else {
            cout << input << ": command not found" << endl;
        }
    }
}
