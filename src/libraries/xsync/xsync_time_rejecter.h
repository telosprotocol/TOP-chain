#include "xbase/xns_macro.h"
#include <cstdint>

#pragma once
NS_BEG2(top, sync)
class xsync_time_rejecter_t {
public:
    xsync_time_rejecter_t(int64_t min_time_interval_ms) : m_min_time_interval_ms(min_time_interval_ms){};
    bool reject();

private:
    int64_t m_time_ms{0};
    int64_t m_min_time_interval_ms;
};

NS_END2