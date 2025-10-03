#pragma once
#include <string>
#include <vector>

namespace proto {

inline std::vector<std::string> split_ws(const std::string &s) {
    std::vector<std::string> out;
    std::string cur;
    for (char c : s) {
        if (c == ' ' || c == '\t' || c == '\r' || c == '\n') {
            if (!cur.empty()) {
                out.push_back(cur);
                cur.clear();
            }
        } else {
            cur.push_back(c);
        }
    }
    if (!cur.empty()) out.push_back(cur);
    return out;
}

inline std::string join_space(const std::vector<std::string> &parts) {
    std::string s;
    for (size_t i = 0; i < parts.size(); ++i) {
        if (i) s.push_back(' ');
        s += parts[i];
    }
    return s;
}

} // namespace proto
