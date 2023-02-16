// Copyright (c) 2017-2019 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <thread>
#include <memory>
#include <map>
#include <atomic>
#include <mutex>
#include <condition_variable>

#include "xpbase/base/top_timer.h"
#include "xkad/routing_table/routing_utils.h"
#if defined(XCXX20)
#include "xtransport/proto/ubuntu/transport.pb.h"
#include "xkad/proto/ubuntu/kadmlia.pb.h"
#else
#include "xtransport/proto/centos/transport.pb.h"
#include "xkad/proto/centos/kadmlia.pb.h"
#endif

namespace top {

namespace base {
class xpacket_t;
}

namespace kadmlia {

typedef std::function<void(
        int,
        transport::protobuf::RoutingMessage&,
        base::xpacket_t&)> ResponseFunctor;
typedef std::function<void(
        int,
        transport::protobuf::RoutingMessage&,
        base::xpacket_t&,
        std::shared_ptr<std::mutex>,
        std::shared_ptr<std::condition_variable>)> CallbackResponseFunctor;

struct CallbackItem {
    uint32_t message_id;
    ResponseFunctor callback;
    CallbackResponseFunctor mutex_callback;
    int32_t timeout_sec;
    int32_t expect_count;
    std::shared_ptr<std::mutex> wait_mutex;
    std::shared_ptr<std::condition_variable> wait_condition;

    ~CallbackItem();
};

typedef std::shared_ptr<CallbackItem> CallbackItemPtr;

class CallbackManager :public std::enable_shared_from_this<CallbackManager>{
public:
    static CallbackManager* Instance();
    static uint32_t MessageId();
    void Join();
    void Add(
            uint32_t message_id,
            int32_t timeout_sec,
            ResponseFunctor callback,
            int32_t expect_count);
    void Add(CallbackItemPtr callback_ptr);
    void Callback(
            uint32_t message_id,
            transport::protobuf::RoutingMessage& message,
            base::xpacket_t& packet);
    void Timeout(uint32_t message_id);
    void Cancel(uint32_t message_id, uint32_t no_callback);
    void Remove(uint32_t message_id);

private:
    CallbackManager();
    ~CallbackManager();

    void TimeoutCheck();

    static std::atomic<uint32_t> msg_id_;
    std::map<uint32_t, CallbackItemPtr> callback_map_;
    std::mutex callback_map_mutex_;
//     base::SingleThreadTimer     timer_;
    std::shared_ptr<base::TimerRepeated> timer_;
    // std::map<uint32_t,std::shared_ptr<base::TimerRepeated>> timer_map_;
    base::TimerManager* timer_manager_{base::TimerManager::Instance()};

    DISALLOW_COPY_AND_ASSIGN(CallbackManager);
};

typedef std::shared_ptr<CallbackManager> CallbackManagerPtr;

}  // namespace kadmlia

}  // namespace top
