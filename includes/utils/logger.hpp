#ifndef PACEY_LOGGER_HPP
#define PACEY_LOGGER_HPP

#include <iostream>
#include <string>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <utility>
#include "functions.hpp"

using namespace std;

class Logger {
public:
    enum class Level {
        LOG,
        WARN,
        ERR
    };

    Logger(const string& projectName) : projectName(projectName) {}

    template<typename... Args>
    void log(Args... args) {
        logMessage(Level::LOG, args...);
    }

    template<typename... Args>
    void warn(Args... args) {
        logMessage(Level::WARN, args...);
    }

    template<typename... Args>
    void error(Args... args) {
        logErr(Level::ERR, args...);
    }

private:
    string projectName;

    static string getCurrentTime() {
        auto now = chrono::system_clock::now();
        auto in_time_t = chrono::system_clock::to_time_t(now);
        stringstream ss;
        ss << put_time(localtime(&in_time_t), "%Y-%m-%d %X");
        return ss.str();
    }

    static string levelToString(Level level) {
        switch (level) {
            case Level::LOG: return "LOG";
            case Level::WARN: return "WARN";
            case Level::ERR: return "ERROR";
            default: return "unknown";
        }
    }

    template<typename T>
    void buildMessage(stringstream& oss, const T& arg) {
        oss << arg;
    }

    template<typename T, typename... Args>
    void buildMessage(stringstream& oss, const T& first, const Args&... rest) {
        oss << first;
        buildMessage(oss, rest...);
    }

    template<typename... Args>
    void logMessage(Level level, const Args&... args) {
        stringstream oss;
        oss << "[" << getCurrentTime() << "] "
            << "[" << levelToString(level) << "] "
            << "(" << projectName << ") : ";
        buildMessage(oss, args...);
        cout << oss.str() << endl;
    }

    template<typename... Args>
    void logErr(Level level, const Args&... args) {
        stringstream oss;
        oss << "[" << getCurrentTime() << "] "
            << "[" << levelToString(level) << "] "
            << "(" << projectName << ") : ";
        buildMessage(oss, args...);
        cerr << oss.str() << endl;
    }
};

#endif //PACEY_LOGGER_HPP
