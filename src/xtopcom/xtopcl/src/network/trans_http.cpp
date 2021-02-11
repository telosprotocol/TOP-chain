
#include "trans_http.h"
#include <string>
#include <iostream>
#include <algorithm>
#include "task/task_dispatcher.h"

namespace xChainSDK
{

    trans_http::trans_http(const std::string& host)
        : trans_base(host)
        , client(_host)
        // , thread_(&trans_http::thread_func, this)
    {
    }

    trans_http::~trans_http()
    {
        shut_down_ = true;
        // cv_.notify_all();
        // thread_.join();
//        std::cout << "delete trans_http" << std::endl;
    }

    int trans_http::do_trans(const std::string& content)
    {
        client.request("POST", "/", content, [](std::shared_ptr<HttpClient::Response> response, const SimpleWeb::error_code & ec) {
            ResponseContent* cont = new ResponseContent();
            if (!ec) {
                cont->content = response->content.string();
                // std::cout << cont->content.c_str() << std::endl;
                task_dispatcher::get_instance()->post_message(msgResponse, (uint32_t*)cont, 0);
            } else {
                cont->content = SERVER_ERROR;
                task_dispatcher::get_instance()->post_message(msgResponse, (uint32_t*)cont, 0);
            }
        });
        client.io_service->run();
        // notify_count_++;
        // cv_.notify_one();
        return 1; // client.io_service->run();
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
}
