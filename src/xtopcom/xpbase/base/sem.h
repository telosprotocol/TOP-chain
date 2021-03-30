#pragma once

#include <mutex>
#include <condition_variable>

namespace top {
namespace base {

class Sem {
public:
    explicit Sem(long count = 0);
    void Pend();
    void Post();
    bool PendFor(long ms);

private:
    std::mutex mutex_;
    std::condition_variable condv_;
    long count_;
};

}  // namespace base
}  // namespace top
