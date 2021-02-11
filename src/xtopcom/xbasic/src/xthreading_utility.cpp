#include "xbasic/xthreading/xutility.h"

NS_BEG2(top, threading)

template
struct xtop_lock_guard_helper<std::mutex>;

NS_END2
