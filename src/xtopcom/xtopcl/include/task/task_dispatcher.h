#pragma once

#ifndef XCHAIN_SDK_TASK_DISPATCHER
#define XCHAIN_SDK_TASK_DISPATCHER

#include "global_definition.h"
#include "request_result_definition.h"
#include "request_task.h"
#include "stat.h"
#include "thread_queue.h"
#include "trans_http.h"
#include "trans_ws.h"
#include "xelect_net/include/http_client.h"
#include "xtopcl/include/global_definition.h"

#include <map>
#include <memory>
#include <mutex>
#include <string>

#define MsgBase 100
#define msgResponse (MsgBase + 1)
#define msgAddTask (MsgBase + 2)

namespace xChainSDK {
struct Msg {
    uint32_t id;
    uint32_t * param1;
    uint32_t * param2;
};

struct ResponseContent {
    std::string content;
};

class task_dispatcher {
public:
    using RequestTaskPtr = std::shared_ptr<request_task>;
    using TaskQueue = std::map<uint32_t, RequestTaskPtr>;
    using HttpTransMap = std::map<std::string, std::shared_ptr<trans_http>>;
    using MessageQueue = ThreadQueue<Msg>;
    using ResultQueue = ThreadQueue<std::string>;

    static task_dispatcher * get_instance();
    static void destory_instance();

    void init();
    void init_thread();

    // task
    int add_task(request_task::TaskInfoPtr info);

    void dispatch_task();

    void handle_response(const std::string & result);
    void push_result_queue(std::string const & result, RequestTaskPtr & task);
    std::string get_result();

    void handle_vote_details_result(VoteDetailsResultPtr result);
    void handle_candidate_details_result(CandidateDetailsResultPtr result);
    void handle_dividend_details_result(DividendDetailsResultPtr result);

    void handle_request_token_result(RequestTokenResultPtr result);
    void handle_create_account_result(CreateAccountResultPtr result);
    void handle_account_info_result(AccountInfoResultPtr result);
    void handle_account_transaction_result(AccountTransactionResultPtr result);
    void handle_transfer_result(TransferResultPtr result);
    void handle_get_property_result(GetPropertyResultPtr result);

    std::shared_ptr<trans_ws> get_ws_trans(const std::string & host);

    void thread_func();

    void post_message(uint32_t id, uint32_t * param1, uint32_t * param2);
    void on_message(uint32_t id, uint32_t * param1, uint32_t * param2);

    request_stat stat_;

private:
    task_dispatcher();
    ~task_dispatcher();

    void regist_transmode();

    static task_dispatcher * s_instance;

    TaskQueue task_queue;
    std::mutex mutex_;
    MessageQueue msg_queue_;
    ResultQueue result_queue_;
    std::thread thread_;

    std::shared_ptr<trans_ws> ws_trans_{nullptr};

    top::elect::SeedHttpClient seed_client;
    static std::vector<std::string> seeds;
    static std::uint16_t index;
    static std::once_flag m_once_init_flag;
};
}  // namespace xChainSDK

#endif  // !XCHAIN_SDK_TASK_DISPATCH
