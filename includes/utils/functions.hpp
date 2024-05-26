//
// Created by alpha on 26/05/2024.
//

#ifndef PACEY_FUNCTIONS_HPP
#define PACEY_FUNCTIONS_HPP
#include <iostream>
#include <sstream>
using namespace std;

template <typename T>
void println(const T& arg) {
    cout << arg << endl;
}

template <typename T, typename... Args>
void println(const T& first, const Args&... rest) {
    ostringstream oss;
    oss << first;
    (oss << ... << rest);
    cout << oss.str() << endl;
}

template <typename T>
void printerr(const T& arg) {
    cerr << arg << endl;
}

template <typename T, typename... Args>
void printerr(const T& first, const Args&... rest) {
    ostringstream oss;
    oss << first;
    (oss << ... << rest);
    cerr << oss.str() << endl;
}


#endif //PACEY_FUNCTIONS_HPP
