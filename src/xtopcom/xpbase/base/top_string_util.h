//
//  top_string_util.h
//
//  Created by @author on 01/15/2019.
//  Copyright (c) 2017-2019 Telos Foundation & contributors
//

#pragma once

#include <stdio.h>
#include <string>
#include <sstream>
#include <vector>
#include <limits>
#include <memory>

#include "xpbase/base/check_cast.h"

namespace top {

namespace base {

namespace StringUtil {
template<typename T>
std::string T2String(T num) {
    std::stringstream ss;
    ss << num;
    std::string result;
    ss >> result;
    return result;
}

template<typename T>
T String2T(const ::std::string& str_num) {
    std::stringstream ss;
    ss << str_num;
    T result;
    ss >> result;
    return result;
}

template<typename ... Args>
std::string str_fmt(const std::string& format, Args ... args) {
    size_t size = ::snprintf(nullptr, 0, format.c_str(), args ...) + 1;  // Extra space for '\0'
    std::unique_ptr<char[]> buf(new char[size]);
    ::snprintf(buf.get(), size, format.c_str(), args ...);
    return std::string(buf.get(), buf.get() + size - 1);  // We don't want the '\0' inside
}

inline std::vector<std::string> String2Array(
        const std::string& in_source,
        const std::string in_split,
        int32_t skip_num = 0) {
    std::vector<std::string> vt_str;
    std::vector<std::string>::size_type s_pos = skip_num;
    std::vector<std::string>::size_type e_pos = in_source.find(in_split, s_pos);

    while (e_pos != std::string::npos) {
        if (s_pos != e_pos) {
            vt_str.push_back(in_source.substr(s_pos, e_pos - s_pos));
        }
        s_pos = e_pos + in_split.size();
        e_pos = in_source.find(in_split, s_pos);
    }

    if (s_pos < in_source.size()) {
        vt_str.push_back(in_source.substr(s_pos, in_source.size() - s_pos));
    }

    return vt_str;
}

}  // namespace StringUtil

}  // namespace base

}  // namespace top
