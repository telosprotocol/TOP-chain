#include "xratelimit_data_queue.h"


NS_BEG2(top, xChainRPC)

RatelimitDataQueue::RatelimitDataQueue()
{}

RatelimitDataQueue::~RatelimitDataQueue()
{}

void RatelimitDataQueue::PushRequestIn(RatelimitData* request) {
    in_in_queue_.push(request);
}

bool RatelimitDataQueue::PluckRequest(RatelimitData*& request) {
    return in_in_queue_.pluck(request);
}

void RatelimitDataQueue::PushRequestOut(RatelimitData* request) {
    in_out_queue_.push(request);
}

bool RatelimitDataQueue::PluckRequestOut(RatelimitData*& request) {
    return in_out_queue_.pluck(request);
}

void RatelimitDataQueue::PushResponseIn(RatelimitData* response) {
    out_in_queue_.push(response);
}

bool RatelimitDataQueue::PluckResponse(RatelimitData*& response) {
    return out_in_queue_.pluck(response);
}

void RatelimitDataQueue::PushResponseOut(RatelimitData* response) {
    out_out_queue_.push(response);
}

bool RatelimitDataQueue::PluckResponseOut(RatelimitData*& response) {
    return out_out_queue_.pluck(response);
}

void RatelimitDataQueue::BreakOut() {
    in_in_queue_.break_out();
    in_out_queue_.break_out();
    out_in_queue_.break_out();
    out_out_queue_.break_out();
}

size_t RatelimitDataQueue::RequestInCount() {
    return in_in_queue_.size();
}

size_t RatelimitDataQueue::RequestOutCount() {
    return in_out_queue_.size();
}

size_t RatelimitDataQueue::ResponseInCount() {
    return out_in_queue_.size();
}

size_t RatelimitDataQueue::ResponseOutCount() {
    return out_out_queue_.size();
}

NS_END2
