#pragma once
#ifndef RATELIMIT_DATA_QUEUE_H_
#define RATELIMIT_DATA_QUEUE_H_

#include <stddef.h>
#include <stdint.h>
#include <string>

#include "xratelimit_thread_queue.h"
#include "xbase/xns_macro.h"


NS_BEG2(top, xChainRPC)

class RatelimitData {
public:
    virtual ~RatelimitData() {}
    enum class TypeInOut {
        kIn,
        kOut
    } type_;
    uint32_t ip_;
    std::string account_;
    int err_;
};

class RatelimitDataQueue final {
    using DataQueue = ThreadQueue<RatelimitData*>;

public:
    RatelimitDataQueue();
    ~RatelimitDataQueue();

    void PushRequestIn(RatelimitData* request);
    bool PluckRequest(RatelimitData*& request);
    void PushRequestOut(RatelimitData* request);
    bool PluckRequestOut(RatelimitData*& request);

    void PushResponseIn(RatelimitData* response);
    bool PluckResponse(RatelimitData*& response);
    void PushResponseOut(RatelimitData* response);
    bool PluckResponseOut(RatelimitData*& response);

    void BreakOut();

    size_t RequestInCount();
    size_t RequestOutCount();
    size_t ResponseInCount();
    size_t ResponseOutCount();

private:
    DataQueue in_in_queue_;     // request in
    DataQueue in_out_queue_;    // request passed
    DataQueue out_in_queue_;    // response in
    DataQueue out_out_queue_;   // response passed (always) or request resused
};

NS_END2

#endif  // !RATELIMIT_DATA_QUEUE_H_
