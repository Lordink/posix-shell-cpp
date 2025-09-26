#pragma once

#include <cstdlib>
#include <filesystem>
#include <format>
#include <iostream>
#include <stdexcept>
#include <string>
#include <unordered_set>
#include <utility>
#include <vector>

#include "util.hpp"

using std::cerr;
using std::cout;
using std::endl;
using std::format;
using std::getline;
using std::string;
using std::vector;

struct ShellState final {
    ExecMap path;
    // Reqs mention preserving the order of dirs; using extra vec for that.
    std::vector<string> dir_order;
    Path cwd = std::filesystem::current_path().string();

    ShellState();
    ~ShellState() {}

    bool find_executable_dir(string const &executable,
                             string &out_found_dir) const;

    // TODO not handling empty path rn
    Path sanitize(Path const &path) const;
};
