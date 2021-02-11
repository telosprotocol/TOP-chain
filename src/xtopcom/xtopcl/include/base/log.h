#pragma once
#include <string>
#include <iostream>
#include <sstream>



namespace xChainSDK {

    template <typename ...Args>
    void print_log(std::ostringstream& os, Args...args) {
    }

    template <typename T, typename ...Args>
    void print_log(std::ostringstream& os, T val, Args...args) {
        os << val;
        print_log(os, args...);
    }

    template <typename ...Args>
    void log_console(Args...args) {
        std::ostringstream out;
        print_log(out, args...);
        std::string out_str = out.str();
        std::cout << out_str.c_str() << std::endl;
    }
}


#ifdef LOG_CONSOLE
#   define LOG(...) xChainSDK::log_console(__VA_ARGS__)
#else
#   define LOG(...)
#endif

