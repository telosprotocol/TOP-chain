#include "xpbase/base/sem.h"
#include <chrono>

#include "xpbase/base/top_utils.h"

namespace top {
namespace base {

Sem::Sem(long count) : count_(count) {
}

void Sem::Pend() {
    std::unique_lock<std::mutex> lock(mutex_);
    if (count_ > 0) {
        count_ -= 1;
        return;
    }

    condv_.wait(lock, [this] { return count_ > 0; });
    count_ -= 1;
    return;
}

void Sem::Post() {
    std::unique_lock<std::mutex> lock(mutex_);
    count_ += 1;
    if (count_ > 0) {
        condv_.notify_one();
    }
}

bool Sem::PendFor(long ms) {
    std::unique_lock<std::mutex> lock(mutex_);
    if (count_ > 0) {
        count_ -= 1;
        return true;
    }

    bool waited = condv_.wait_for(lock, std::chrono::milliseconds(ms), [this] { return count_ > 0; });
    if (!waited) {
        return false;
    }

    count_ -= 1;
    return true;
}

}  // namespace base
}  // namespace top
