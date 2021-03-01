#pragma once
#ifndef RATELIMIT_SERVER_H_
#define RATELIMIT_SERVER_H_

#include "xratelimit_config.h"
#include "xratelimit_server_stat.h"
#include "xratelimit_data_queue.h"
#include "xratelimit_worker.h"
#include "xratelimit_dispatch.h"
#include "xratelimit_thread.h"
#include "xbasic/xns_macro.h"
#include <atomic>
#include <vector>
#include <memory>
#include <functional>


NS_BEG2(top, xChainRPC)

class RatelimitServer final {
public:
    explicit RatelimitServer(const RatelimitConfig& config);
    ~RatelimitServer();

    void RequestIn(RatelimitData* request);
    void ResponseIn(RatelimitData* response);

    RatelimitData* RequestOut();
    RatelimitData* ResponseOut();

    RatelimitServerStat* GetStat();

    using OutFunc = std::function<void(RatelimitData*)>;
    void RegistRequestOut(OutFunc func);
    void RegistResponseOut(OutFunc func);
    void RequestOutThreadFunc();
    void ResponseOutThreadFunc();

    void BreakOut();

private:
    const RatelimitConfig* config_{ nullptr };
    RatelimitServerStat server_stat_;
    RatelimitDataQueue data_queue_;
    RatelimitCache cache_;
    RatelimitDispatch dispatch_;
    RatelimitWorkerPool in_worker_pool_;
    RatelimitWorkerPool out_worker_pool_;

    std::atomic_bool shut_down_{ false };
    OutFunc request_out_func_;
    OutFunc response_out_func_;
    std::vector<std::shared_ptr<RatelimitThread>> request_out_pool_;
    std::vector<std::shared_ptr<RatelimitThread>> response_out_pool_;
};

class RatelimitServerHelper final {
public:
    static std::string GetAccountAddress(const std::string& str) {
        std::string account{ "target_account_addr=" };
        return GetPostParam(str, account);
    }

    static std::string GetSequenceId(const std::string& str) {
        std::string sequence_id{ "sequence_id=" };
        return GetPostParam(str, sequence_id);
    }

    static std::string GetPostParam(const std::string& body,
        const std::string& param_name) {
        std::string str_ret{ "" };
        size_t pos_beg{ 0 };
        do {
            if (pos_beg != 0) { pos_beg += param_name.length(); }
            pos_beg = body.find(param_name, pos_beg);
        } while (pos_beg != std::string::npos &&
            pos_beg != 0 && body.at(pos_beg - 1) != '&');

        if (pos_beg != std::string::npos) {
            pos_beg += param_name.length();
            auto pos_end = body.find("&", pos_beg);
            if (pos_end == std::string::npos)
                pos_end = body.length();

            str_ret = body.substr(pos_beg, pos_end - pos_beg);
        }
        return str_ret;
    }
};

NS_END2

#endif  // !RATELIMIT_SERVER_H_
