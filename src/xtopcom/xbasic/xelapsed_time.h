// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once
#include <string>
#include <chrono>
#include "xbase/xns_macro.h"

NS_BEG1(top)
// from https://stackoverflow.com/questions/1861294/how-to-calculate-execution-time-of-a-code-snippet-in-c?noredirect=1&lq=1
class xelapsed_time {
public:
    xelapsed_time() : beg_(clock_::now()) {}
    void reset() { beg_ = clock_::now(); }
    double elapsed() const {
        return std::chrono::duration_cast<second_>(clock_::now() - beg_).count(); 
    }
private:
    typedef std::chrono::high_resolution_clock clock_;
    typedef std::chrono::duration<double, std::ratio<1> > second_;
    std::chrono::time_point<clock_> beg_;
};
NS_END1