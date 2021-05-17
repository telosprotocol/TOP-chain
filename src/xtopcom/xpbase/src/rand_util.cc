//
//  rand_util.cc
//
//  Created by @author on 01/15/2019.
//  Copyright (c) 2017-2019 Telos Foundation & contributors
//

#include "xpbase/base/rand_util.h"

#include <mutex>
#include <random>
#include <cstdint>

namespace top {
namespace base {

int64_t GetRandomInt64() {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    static std::uniform_int_distribution<int64_t> dis;
    static std::mutex mutex;
    std::lock_guard<std::mutex> lock(mutex);
    return dis(gen);
}

}  // namespace base
}  // namespace top
