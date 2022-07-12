#pragma once

#include <iostream>
#include <sstream>
#include <string>

template <typename T>
void handle_log(std::ostringstream & os, T val) {
    os << val;
}

template <typename T, typename... Args>
void handle_log(std::ostringstream & os, T val, Args... args) {
    os << val;
    handle_log(os, args...);
}

template <typename... Args>
void log_console(Args... args) {
    std::ostringstream out;
    handle_log(out, args...);
    std::string out_str = out.str();
    std::cout << out_str.c_str() << std::endl;
}

#define CONSOLE_ERROR(...) log_console("Error: " ,__VA_ARGS__)
#define CONSOLE_INFO(...) log_console(__VA_ARGS__)