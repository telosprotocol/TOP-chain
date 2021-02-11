#pragma once

#include "xpbase/base/top_timer.h"

namespace top {
namespace base {

struct TimerItem {
    std::chrono::steady_clock::time_point time_point;
    TimerFunction func;
    uint32_t timer_idx;
};
typedef std::shared_ptr<TimerItem> TimerItemPtr;

}  // namespace base
}  // namespace top
