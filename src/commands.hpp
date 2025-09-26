#pragma once

#include <cstdlib>
#include <filesystem>
#include <format>
#include <iostream>
#include <string>
#include <utility>
#include <vector>

#include "state.h"
#include "util.hpp"

using std::cerr;
using std::cout;
using std::endl;
using std::format;
using std::getline;
using std::string;
using std::vector;

namespace commands {
using std::size_t;

// Command should at least have 3 methods, matches(), min_args() and execute()
// TODO check min_args() in runtime
template <typename T>
concept Command = requires(vector<string> const &args, ShellState &state) {
    // In runtime, we match each of these against the input's first word
    { T::matches(args) } -> std::same_as<bool>;
    // Actually run the command. Args are words separated by whitespace
    // (will evolve once we introduce single quoates)
    { T::execute(state, args) } -> std::same_as<void>;
    // Num args that should be passed to this command, excluding the command
    // itself (0th arg)
    { T::min_args() } -> std::same_as<size_t>;
};

// Forward decl so that the commands themselves could use this,
// e.g. TypeCmd
struct TypeCmd;
struct EchoCmd;
struct ExitCmd;
struct PwdCmd;
struct CdCmd;
using Builtins = std::tuple<EchoCmd, ExitCmd, PwdCmd, TypeCmd, CdCmd>;

struct EchoCmd {
    static size_t min_args() { return 1; }
    static bool matches(vector<string> const &args) {
        return args[0] == "echo";
    }
    static void execute(ShellState &state, vector<string> const &args) {
        for (size_t i = 1; i < args.size(); i++) {
            cout << args[i] << ' ';
        }
        cout << endl;
    }
};

struct ExitCmd {
    static size_t min_args() { return 1; }
    static bool matches(vector<string> const &args) {
        return args[0] == "exit";
    }
    static void execute(ShellState &state, vector<string> const &args) {
        // May throw; ignoring that fact for now
        exit(std::stoi(args[1]));
    }
};
struct PwdCmd {
    static size_t min_args() { return 0; }
    static bool matches(vector<string> const &args) { return args[0] == "pwd"; }
    static void execute(ShellState &state, vector<string> const &args) {
        auto path_str = state.cwd.string();
        if (path_str.back() == '/') {
            path_str.pop_back();
        }
        cout << path_str << endl;
    }
};
struct CdCmd {
    static size_t min_args() { return 1; }
    static bool matches(vector<string> const &args) { return args[0] == "cd"; }
    static void execute(ShellState &state, vector<string> const &args) {
        const auto &new_path_str = args[1];
        Path new_path;
        try {
            new_path = state.sanitize(Path(new_path_str));
        } catch (std::exception &e) {
            cerr << format("Error parsing path: {}", e.what());
            exit(1);
        }

        if (!std::filesystem::exists(new_path)) {
            cout << format("{}: No such file or directory\n", new_path_str);
        } else {
            state.cwd = new_path;
        }
    }
};

struct TypeCmd {

    // Internal fn used by is_in_tuple, allows for folding
    template <typename... Commands>
    static bool exec_is_in_tuple(string const &cmd) {
        const vector<string> v = {cmd};
        return (Commands::matches(v) || ...);
    }

    // Does this arg chain match any of the tuple's type members?
    template <typename CommandsTuple>
    static bool is_in_tuple(string const &cmd) {
        return std::apply(
            [&](auto... cmds) {
                return exec_is_in_tuple<decltype(cmds)...>(cmd);
            },
            CommandsTuple{});
    }

    static size_t min_args() { return 1; }
    static bool matches(vector<string> const &args) {
        return args[0] == "type";
    }
    static void execute(ShellState &state, vector<string> const &args) {
        string queried_exec = args[1];

        if (is_in_tuple<Builtins>(queried_exec)) {
            cout << queried_exec << " is a shell builtin" << endl;
        } else {
            util::adjust_exec_file_ext(queried_exec);
            string queried_exec_dir;
            const bool found_queried_in_path =
                state.find_executable_dir(queried_exec, queried_exec_dir);
            if (found_queried_in_path) {
                cout << format("{} is {}/{}\n", queried_exec, queried_exec_dir,
                               queried_exec);
            } else {
                cout << queried_exec << ": not found" << endl;
            }
        }
    }
};

template <typename Tuple, size_t... Is>
constexpr bool ith_member_satisfies_concept(std::index_sequence<Is...>) {
    return (Command<std::tuple_element_t<Is, Tuple>> && ...);
}

template <typename Tuple>
constexpr bool all_commands_valid = ith_member_satisfies_concept<Tuple>(
    std::make_index_sequence<std::tuple_size_v<Tuple>>());

static_assert(all_commands_valid<Builtins>,
              "Not all builtins satisfy Command concept");

// Returns true if command executed, i.e. it was a valid builtin
// (doesn't return the result of command executing tho)
template <typename... Commands>
bool exec_if_in_tuple(ShellState &state, vector<string> const &args) {
    return ((Commands::matches(args) ? (Commands::execute(state, args), true)
                                     : false) ||
            ...);
}

template <typename CommandsTuple>
bool dispatch_cmd(ShellState &state, vector<string> const &args) {
    return std::apply(
        [&](auto... cmds) {
            return exec_if_in_tuple<decltype(cmds)...>(state, args);
        },
        CommandsTuple{});
}
} // namespace commands
