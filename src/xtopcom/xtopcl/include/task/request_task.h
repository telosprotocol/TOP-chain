#pragma once
#ifndef XCHAIN_SDK_REQUEST_TASK
#define XCHAIN_SDK_REQUEST_TASK

#include <string>
#include <memory>
#include <iostream>

#include "trans_base.h"
#include "task_info.h"


namespace xChainSDK
{
class protocol;
class task_info;
class trans_base;
struct RequestTokenResult;

    class request_task
    {
    public:
        using RequestTokenResultPtr = std::shared_ptr<RequestTokenResult>;
        using ProtocolPtr = std::shared_ptr<protocol>;
        using TransPtr = std::shared_ptr<trans_base>;
        using TaskInfoPtr = std::shared_ptr<task_info>;

        request_task(TaskInfoPtr info);
        ~request_task();
        bool is_running();
        int do_task();
        task_info* get_task_info();
        void handle_result(const std::string& result);
        void repeat_task_with_new_server(const std::string& new_server);

    private:
        bool _is_running;
        ProtocolPtr _protocol;
        TransPtr _trans;
        TaskInfoPtr _info;
        RequestTokenResultPtr _result;
        static std::string _req_content;
    };
}

#endif // !XCHAIN_SDK_REQUEST_TASK
