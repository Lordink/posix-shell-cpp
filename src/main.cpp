#include <cctype>
#include <cstdlib>
#include <filesystem>
#include <format>
#include <iostream>
#include <sstream>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

using std::cerr;
using std::cout;
using std::endl;
using std::format;
using std::getline;
using std::string;

// #define _DEBUG_LOG_EXECUTABLES true

// Used for "type" command only
// TODO map these to their functionality to avoid duplication and potential
//      mistakes if a new builtin is added only to one of two places
const std::unordered_set<string> builtins = {"echo", "exit", "type", "pwd"};

#ifdef _WIN32
const std::unordered_set<string> windows_exec_exts = {".exe", ".bat", ".cmd"};
#endif

// Key is dir; val is executables inside that dir
using ExecMap = std::unordered_map<string, std::unordered_set<string>>;

namespace util {

std::vector<string> into_words(const string &input) {
    string word;
    std::istringstream iss(input, std::istringstream::in);
    std::vector<string> words;

    while (iss >> word) {
        words.push_back(word);
    }

    return words;
}

// @returns list of strings representing abs directories, found in $PATH
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

// Run executable
int exec(string const &command) { return std::system(command.c_str()); }

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

// On win: adds .exe, returns resulting string
// Anywhere else: returns Nothing
void adjust_exec_file_ext(string &cmd) {
#ifdef _WIN32
    // On windows, by default things end with .exe (there's other exec
    // extensions but keeping it simple) So if cmd has no .exe already,
    // we'll add it Not bulletproof solution for multiple reasons, but
    // speeds up local testing
    if (cmd.find('.') == std::string::npos) {
        cmd = cmd + ".exe";
    }
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

} // namespace util

struct ShellState final {
    ExecMap path;
    // Reqs mention preserving the order of dirs; using extra vec for that.
    std::vector<string> dir_order;

    ShellState() {
        for (const string &dir : util::get_path_dirs()) {
            util::get_executables_in_dir(dir, this->path);
            dir_order.push_back(dir);
        }
#ifdef _DEBUG_LOG_EXECUTABLES
        for (const auto &[dir, execs_inside] : this->path) {
            cout << "Directory " << dir << ":" << endl;
            cout << "    ";
            for (const auto &exec : execs_inside) {
                cout << exec << " ";
            }
            cout << endl;
        }
        cout << "Num executables found: " << this->path.size() << endl;
#endif
    }
    ~ShellState() {}

    bool find_executable_dir(string const &executable, string &out_found_dir) {
        for (const auto &dir : this->dir_order) {
            auto it = this->path.find(dir);
            if (it != this->path.end() && it->second.contains(executable)) {
                out_found_dir = dir;
                return true;
            }
        }

        return false;
    }

    // If it's a builtin - will handle it and return true
    bool handle_builtin(const std::vector<string> &cmd_words) {
        const auto &cmd = cmd_words[0];
        // Most builtins require at least 1 arg
        const auto has_args = [cmd_words] { return cmd_words.size() > 1; };

        if (cmd == "exit" && has_args()) {
            // May throw; ignoring that fact for now
            exit(std::stoi(cmd_words[1]));
        } else if (cmd == "echo" && has_args()) {
            for (size_t i = 1; i < cmd_words.size(); i++) {
                cout << cmd_words[i] << ' ';
            }
            cout << endl;
        } else if (cmd == "type" && has_args()) {
            string queried_exec = cmd_words[1];

            if (builtins.contains(queried_exec)) {
                cout << queried_exec << " is a shell builtin" << endl;
            } else {
                util::adjust_exec_file_ext(queried_exec);
                string queried_exec_dir;
                const bool found_queried_in_path =
                    this->find_executable_dir(queried_exec, queried_exec_dir);
                if (found_queried_in_path) {
                    cout << format("{} is {}/{}\n", queried_exec,
                                   queried_exec_dir, queried_exec);
                } else {
                    cout << queried_exec << ": not found" << endl;
                }
            }
        } else if (cmd == "pwd") {
            const auto current_dir = std::filesystem::current_path().string();
            cout << current_dir << endl;
        } else {
            return false;
        }
        return true;
    }
};

int main() {
    // Flush after every std::cout / std:cerr
    cout << std::unitbuf;
    cerr << std::unitbuf;

    ShellState state;

    while (true) {
        cout << "$ ";

        string input;
        getline(std::cin, input);

        // Break into words:
        const auto words = util::into_words(input);

        if (state.handle_builtin(words)) {
            // code inside will call exit() if it comes to that
            continue;
        };

        auto cmd = words[0];
#ifdef _WIN32
        // On windows, by default things end with .exe (there's other exec
        // extensions but keeping it simple) So if cmd has no .exe already,
        // we'll add it Not bulletproof solution for multiple reasons, but
        // speeds up local testing
        util::adjust_exec_file_ext(cmd);
#endif
        string exec_dir;
        const bool found_in_path = state.find_executable_dir(cmd, exec_dir);
        if (found_in_path) {
            util::exec(input);
            continue;
        }

        cout << input << ": command not found" << endl;
    }
}
