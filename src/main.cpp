#include <cctype>
#include <cstdlib>
#include <filesystem>
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

#ifdef _WIN32
const std::unordered_set<string> windows_exec_exts = {".exe", ".bat", ".cmd"};
#endif

// Also checks whether it has the right perm
bool is_executable(std::filesystem::path const &path) {
#ifdef _WIN32
    auto ext = path.extension().string();
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
    return windows_exec_exts.contains(ext);
#else
    // TODO
#endif
}

void get_executables_in_dir(
    const string &abs_path,
    // TODO this should be unordered_map<string, vec<string>>
    std::unordered_set<string> &out_executables) {

    try {
        std::filesystem::directory_iterator it(abs_path);

        for (const auto &entry : it) {
            if (entry.is_regular_file() && is_executable(entry.path())) {
                out_executables.insert(entry.path().filename().string());
            }
        }
    } catch (const std::filesystem::filesystem_error &e) {
        cerr << "Failed to read from directory: " << e.what() << endl;
    }
}

std::vector<string> into_words(const string &input) {
    string word;
    std::istringstream iss(input, std::istringstream::in);
    std::vector<string> words;

    while (iss >> word) {
        words.push_back(word);
    }

    return words;
}

std::vector<string> get_path_dirs() {
    const char *path = std::getenv("PATH");

#ifdef _WIN32
    constexpr char sep = ';';
#else
    constexpr char sep = ':';
#endif

    string dir;
    std::istringstream iss(path);
    std::vector<string> dirs;

    while (getline(iss, dir, sep)) {
        if (!dir.empty()) {
            dirs.push_back(dir);
        }
    }

    return dirs;
}

int main() {
    // Flush after every std::cout / std:cerr
    cout << std::unitbuf;
    cerr << std::unitbuf;

    std::unordered_set<string> executables;
    for (const string &dir : get_path_dirs()) {
        // cout << dir << ": " << endl;
        get_executables_in_dir(dir, executables);
    }
    /*
    cout << "Num executables found: " << executables.size();
    for (const auto &fname : executables) {
        cout << fname << "; ";
    }
    cout << endl;
    */
    // TODO executive permissions check (on unix)

    while (true) {
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
                if (builtins.contains(all_args) ||
                    executables.contains(all_args)) {
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
