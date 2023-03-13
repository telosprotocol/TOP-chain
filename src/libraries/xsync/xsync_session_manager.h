// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <mutex>
#include "xsync/xsync_message.h"

NS_BEG2(top, sync)

const uint32_t SYNC_SESSION_TIMEOUT = 5000;

class xsync_session_manager_t {

public:
    xsync_session_manager_t(uint32_t _max_session_num, uint32_t _timeout)
    : m_session_num(_max_session_num)
    , m_timeout(_timeout) {

    }
    virtual ~xsync_session_manager_t(){};

public:
    void on_timer() { sync_block_request_timeout_clear();}
    bool sync_block_request_insert(const xsync_msg_block_request_ptr_t& request_ptr);
    bool sync_block_request_valid_check(const xsync_msg_block_request_ptr_t& request_ptr);
    bool sync_block_resopnse_valid_check(const xsync_msg_block_response_ptr_t& response_ptr, xsync_msg_block_request_ptr_t& reuqest_ptr);
    bool sync_block_push_valid_check(const xsync_msg_block_push_ptr_t& msg_push_ptr);

private:
    void sync_block_request_timeout_clear();
    bool sync_block_msg_valid_check(const xobject_ptr_t<xsync_msg_block_t> msg);
    bool sync_block_session_check(const xsync_msg_block_response_ptr_t& response_ptr, xsync_msg_block_request_ptr_t& reuqest_ptr);
private:
    uint32_t m_session_num;
    uint32_t m_timeout;
    std::mutex m_session_mutex;
    std::unordered_map<uint64_t, xsync_msg_block_request_ptr_t> m_request_ptr_cache;
};

NS_END2