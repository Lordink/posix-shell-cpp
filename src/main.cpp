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
        const auto tokens = util::tokenize(input);

        // if (state.handle_builtin(words)) {
        if (commands::dispatch_builtin<commands::Builtins>(state, tokens)) {
            continue;
        }

        auto cmd = tokens[0];
#ifdef _WIN32
        // On windows, by default things end with .exe (there's other exec
        // extensions but keeping it simple) So if cmd has no .exe already,
        // we'll add it Not bulletproof solution for multiple reasons, but
        // speeds up local testing
        util::adjust_exec_file_ext(cmd);
#endif
        if (string exec_dir; state.find_executable_dir(cmd, exec_dir)) {
            util::exec(input);
            continue;
        }

        cout << input << ": command not found" << endl;
    }
}
