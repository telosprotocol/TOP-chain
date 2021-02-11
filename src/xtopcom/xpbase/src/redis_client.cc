#include "xpbase/base/redis_client.h"

#include <cassert>

namespace top {

namespace base {

RedisClient* RedisClient::Instance() {
    static RedisClient ins;
    return &ins;
}

bool RedisClient::Start(const std::string& ip, uint16_t port) {
    std::unique_lock<std::mutex> lock(start_mutex_);
    assert(!redis_cli_);
    if (redis_cli_) {
        return false;
    }
    redis_cli_ = std::make_shared<cpp_redis::client>();
    using namespace std::placeholders;
    redis_cli_->connect(ip, port, std::bind(&RedisClient::ConnectCallback, this, _1, _2, _3));    
    redis_cli_->auth("top@com");
    std::cout << "redis connect: " << ip << ":" << port << std::endl;
    sem.Pend();
    if (!redis_cli_->is_connected()) {
        redis_cli_.reset();
        return false;
    }
    return true;
}

void RedisClient::Stop() {
    std::unique_lock<std::mutex> lock(start_mutex_);
    redis_cli_.reset();
}

void RedisClient::ConnectCallback(
        const std::string& host,
        std::size_t port,
        cpp_redis::client::connect_state status) {
    std::cout << "redis connect callback: " << host << ":" << port << std::endl;
    sem.Post();
}

RedisClient::RedisClient() {}

RedisClient::~RedisClient() {}

}  // namespace base

}  // namespace top
