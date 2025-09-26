#include "state.h"

ShellState::ShellState() {
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

bool ShellState::find_executable_dir(string const &executable,
                                     string &out_found_dir) const {
    for (const auto &dir : this->dir_order) {
        auto it = this->path.find(dir);
        if (it != this->path.end() && it->second.contains(executable)) {
            out_found_dir = dir;
            return true;
        }
    }

    return false;
}

// TODO not handling empty path rn
Path ShellState::sanitize(Path const &path) const {
    auto it = path.begin();
    auto start = it->string();
    Path out;
    if (start == ".") {
        out = this->cwd;
    } else if (start == "..") {
        out = this->cwd.parent_path();
    } else if (start == "~") {
        const char *home_path = std::getenv("HOME");
        out = Path(home_path);
    } else {
        out = start;
    }
    // Pushing it forward before loop
    ++it;

    while (it != path.end()) {
        auto p = it->string();
        if (p == ".") {
            throw std::invalid_argument("Found . in the middle of cd path");
        } else if (p == "..") {
            out = out.parent_path();
        } else {
            out /= p;
        }
        ++it;
    }

    return out;
}
