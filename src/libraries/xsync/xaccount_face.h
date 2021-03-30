#pragma once

#include <unordered_map>
#include "xsync/xchain_info.h"
#include "xmbus/xmessage_bus.h"

NS_BEG2(top, sync)

class xaccount_face_t {
public:
    virtual ~xaccount_face_t() {}

public:
    // for account_queue
    virtual std::string get_address() = 0;
    virtual void on_role_changed(const xchain_info_t &chain_info) = 0;
    virtual void on_timer_event(int64_t now) = 0;
    virtual void on_find_block(uint64_t height, uint64_t block_time) = 0;

    virtual void on_response_event(const mbus::xevent_ptr_t &e) = 0;
    virtual void on_behind_event(const mbus::xevent_ptr_t &e) = 0;
    virtual void on_lack_event(const std::set<uint64_t> &set_heights) = 0;
    virtual bool is_idle() = 0;
    virtual int64_t get_next_timeout() = 0;
};

using xaccount_face_ptr_t = std::shared_ptr<xaccount_face_t>;

NS_END2
