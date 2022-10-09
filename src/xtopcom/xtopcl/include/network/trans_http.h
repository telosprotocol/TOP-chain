#pragma once

#include "xhttp/xhttp_client_base.h"
#include "xtopcl/include/network/trans_base.h"

#include <memory>
#include <thread>

namespace xChainSDK {

class trans_http_client : public top::xhttp::xhttp_client_async_base_t {
private:
    using base_t = top::xhttp::xhttp_client_async_base_t;

public:
    trans_http_client(std::string const & ip_port);

    void do_request(std::string const & content);
};

class trans_http : public trans_base {
public:
    trans_http(const std::string & host);
    virtual ~trans_http();

    virtual int do_trans(const std::string & content);

    // void thread_func();

private:
    trans_http_client async_client;
    // std::thread thread_;
    // std::mutex mutex_;
    // std::condition_variable cv_;
    bool shut_down_{false};
    // std::atomic_int notify_count_{ 0 };
};
}  // namespace xChainSDK
