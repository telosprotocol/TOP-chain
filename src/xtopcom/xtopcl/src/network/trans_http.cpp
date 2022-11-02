
#include "xtopcl/include/network/trans_http.h"

#include "xtopcl/include/task/task_dispatcher.h"

#include <algorithm>
#include <iostream>
#include <string>

namespace xChainSDK {
trans_http_client::trans_http_client(std::string const & ip_port) : base_t{ip_port} {
}

void trans_http_client::do_request(std::string const & content) {
    request_post_string("/", content, [](std::string const & resp_str) {
        ResponseContent * cont = new ResponseContent();
        if (resp_str.empty()) {
            cont->content = SERVER_ERROR;
        } else {
            cont->content = resp_str;
        }
        task_dispatcher::get_instance()->post_message(msgResponse, (uint32_t *)cont, 0);
    });
    run_io_service();
}

trans_http::trans_http(const std::string & host)
  : trans_base(host)
  , async_client(_host)
// , thread_(&trans_http::thread_func, this)
{
}

trans_http::~trans_http() {
    shut_down_ = true;
    // cv_.notify_all();
    // thread_.join();
    //        std::cout << "delete trans_http" << std::endl;
}

int trans_http::do_trans(const std::string & content) {
    async_client.do_request(content);
    return 1;
}

// void trans_http::thread_func() {
//     // std::cout << "trans_http::thread_func" << std::endl;
//     while (!shut_down_) {
//         if (notify_count_ != 0) {
//             notify_count_--;
//             if (client.io_service != nullptr) {
//                 client.io_service->run();
//             }
//             else {
//                 std::cout << "client.io_service == nullptr" << std::endl;
//             }
//             continue;
//         }
//         std::unique_lock<std::mutex> lock(mutex_);
//         cv_.wait(lock);
//     }
//     return;
// }
}  // namespace xChainSDK
