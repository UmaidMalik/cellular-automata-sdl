#pragma once

#include <string>
#include <vector>
#include <regex>

namespace util {

    void ltrim(std::string &s)
    {
        s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](unsigned char ch)
        {
            return !std::isspace(ch);
        }));
    }

    void rtrim(std::string &s)
    {
        s.erase(std::find_if(s.rbegin(), s.rend(),
            [](unsigned char ch) { return !std::isspace(ch); }).base(), s.end());
    }

    void trim(std::string &s)
    {
        ltrim(s);
        rtrim(s);
    }

    std::vector<std::string> split_and_trim(const std::string &str, char delimiter)
    {
        std::vector<std::string> tokens;
        std::istringstream iss(str);
        std::string token;

        while (std::getline(iss, token, delimiter)) {
            trim(token);
            if (!token.empty()) { // skip empty results if you want
                tokens.push_back(token);
            }
        }
        return tokens;
    }

    void remove_spaces(std::string &s) {
        s.erase(std::remove_if(s.begin(), s.end(),
            [](unsigned char ch) { return std::isspace(ch); }),
            s.end());
    }
}