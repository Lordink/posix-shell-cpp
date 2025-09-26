#pragma once

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
using std::vector;

using Path = std::filesystem::path;

// Key is dir; val is executables inside that dir
using ExecMap = std::unordered_map<string, std::unordered_set<string>>;

#ifdef _WIN32
const std::unordered_set<string> windows_exec_exts = {".exe", ".bat", ".cmd"};
#endif

// #define _DEBUG_LOG_INTO_WORDS true

namespace util {

static std::vector<string> into_words(const string &input) {
    string word;
    vector<string> words;
    // TODO bitfield this? (for fun)
    bool is_single_quoting = false;
    bool is_double_quoting = false;

    static const auto dbg = [](auto text) {
#ifdef _DEBUG_LOG_INTO_WORDS
        cout << text << endl;
#endif
    };

    for (char const &c : input) {
        dbg(format("Checking {}", c));
        switch (c) {
        case '"':
            if (!is_single_quoting) {
                is_double_quoting = !is_double_quoting;
            } else {
                // Within single quotes, double quote is literal
                word.push_back(c);
            }
            break;
        case '\'':
            if (!is_double_quoting) {
                is_single_quoting = !is_single_quoting;
            } else {
                // Within double quotes, single quote is literal
                word.push_back(c);
            }
            break;
        case ' ':
            if (is_single_quoting || is_double_quoting) {
                word.push_back(c);
            } else if (!word.empty()) {
                dbg(format("  Pushed back word {}", word));
                words.push_back(word);
                word = "";
            }
            break;
        default:
            dbg(format("  Pushed back char {}", c));
            word.push_back(c);
            break;
        }
    }

    if (!word.empty()) {
        words.push_back(word);
    }

    return words;
}

// @returns list of strings representing abs directories, found in $PATH
static std::vector<string> get_path_dirs() {
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
static int exec(string const &command) { return std::system(command.c_str()); }

// Also checks whether it has the right perm
static bool is_executable(Path const &path) {
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
static void adjust_exec_file_ext(string &cmd) {
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

static void get_executables_in_dir(const string &abs_path,
                                   ExecMap &out_executables) {
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
