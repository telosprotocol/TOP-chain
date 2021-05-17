#pragma once

#include <stdint.h>
#include <memory>
#include <string>
#include <condition_variable>
#include <iosfwd>
#include <mutex>

#include "cpp_redis/core/client.hpp"
#include "xpbase/base/sem.h"

namespace top {

namespace base {

class RedisClient {
public:
    static RedisClient* Instance();
    bool Start(const std::string& ip, uint16_t port);
    void Stop();
    std::shared_ptr<cpp_redis::client> redis_cli() { return redis_cli_; }

private:
    RedisClient();
    ~RedisClient();
    void ConnectCallback(
            const std::string& host,
            std::size_t port,
            cpp_redis::client::connect_state status);

    std::shared_ptr<cpp_redis::client> redis_cli_{ nullptr };
    std::mutex start_mutex_;
    base::Sem sem;
};

}
}
