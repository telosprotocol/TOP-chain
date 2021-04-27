#pragma once
#include <xcontract_common/xcontract_state.h>

#include <string>

struct params {
    std::string account_from;
    std::string account_to;
    int value;
    params(std::string _f, std::string _t, int _v) : account_from{_f}, account_to{_t}, value{_v} {
    }
};