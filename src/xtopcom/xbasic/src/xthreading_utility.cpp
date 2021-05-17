#include <mutex>

#include "xbasic/xthreading/xutility.h"
#include "xbase/xns_macro.h"

NS_BEG2(top, threading)

template
struct xtop_lock_guard_helper<std::mutex>;

NS_END2
