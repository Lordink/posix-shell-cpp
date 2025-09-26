#include <format>
#include <iostream>
#include <string>
#include <vector>

#include "commands.hpp"
#include "state.h"
#include "util.hpp"

using std::cerr;
using std::cout;
using std::endl;
using std::getline;
using std::string;

// #define _DEBUG_LOG_EXECUTABLES true

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

        // if (state.handle_builtin(words)) {
        if (commands::dispatch_cmd<commands::Builtins>(state, words)) {
            // code inside will call exit() if it comes to that
            continue;
        }

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
