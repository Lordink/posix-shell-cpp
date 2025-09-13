#include <cctype>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <sstream>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

using std::cerr;
using std::cout;
using std::endl;
using std::getline;
using std::string;

// #define _DEBUG_LOG_EXECUTABLES true

// Used for "type" command only
const std::unordered_set<string> builtins = {"echo", "exit", "type"};

#ifdef _WIN32
const std::unordered_set<string> windows_exec_exts = {".exe", ".bat", ".cmd"};
#endif

// Key is dir; val is executables inside that dir
using ExecMap = std::unordered_map<string, std::unordered_set<string>>;

// Also checks whether it has the right perm
bool is_executable(std::filesystem::path const &path) {
#ifdef _WIN32
    auto ext = path.extension().string();
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
    return windows_exec_exts.contains(ext);
#else
    using std::filesystem::perms;
    const auto prms = std::filesystem::status(path).permissions();
    return (prms & perms::owner_exec) != perms::none ||
           (prms & perms::group_exec) != perms::none ||
           (prms & perms::others_exec) != perms::none;
#endif
}

void get_executables_in_dir(const string &abs_path, ExecMap &out_executables) {
    try {
        std::filesystem::directory_iterator it(abs_path);

        for (const auto &entry : it) {
            if (entry.is_regular_file() && is_executable(entry.path())) {
                const auto exec_str = entry.path().filename().string();
                if (out_executables.contains(abs_path)) {
                    out_executables[abs_path].insert(exec_str);
                } else {
                    out_executables.insert({abs_path, {exec_str}});
                }
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

bool find_executable_dir(ExecMap const &execs, string const &executable,
                         string &out_found_dir) {

    for (const auto &[dir, execs_inside] : execs) {
        if (execs_inside.contains(executable)) {
            out_found_dir = dir;
            return true;
        }
    }

    return false;
}

int main() {
    // Flush after every std::cout / std:cerr
    cout << std::unitbuf;
    cerr << std::unitbuf;

    ExecMap executables;
    for (const string &dir : get_path_dirs()) {
        get_executables_in_dir(dir, executables);
    }
#ifdef _DEBUG_LOG_EXECUTABLES
    cout << "Num executables found: " << executables.size();
    for (const auto &[dir, execs_inside] : executables) {
        cout << "Directory " << dir << ":" << endl;
        cout << "    ";
        for (const auto &exec : execs_inside) {
            cout << exec << " ";
        }
        cout << endl;
    }
#endif

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
                if (builtins.contains(all_args)) {
                    cout << all_args << " is a shell builtin" << endl;
                } else {
                    string exec_dir;
                    const bool found_in_path =
                        find_executable_dir(executables, all_args, exec_dir);

                    if (found_in_path) {
                        cout << all_args << " is " << exec_dir << endl;
                    } else {
                        cout << all_args << ": not found" << endl;
                    }
                }
            }
        } else {
            cout << input << ": command not found" << endl;
        }
    }
}
