// Copyright (c) 2017-2022 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xsync/xsync_session_manager.h"
#include "xsync/xsync_log.h"
#include "xmetrics/xmetrics.h"

NS_BEG2(top, sync)



bool  xsync_session_manager_t::sync_block_request_insert(const xsync_msg_block_request_ptr_t& request_ptr)
{
    if (m_request_ptr_cache.size() == m_session_num) {
        xwarn("xsync_session_manager_t::sync_block_request_insert sessionID(%lx) failed, session count(%ld).", request_ptr->get_sessionID(), m_request_ptr_cache.size());
        return false;
    }
    
    std::unique_lock<std::mutex> lock(m_session_mutex);
    if (m_request_ptr_cache.find(request_ptr->get_sessionID()) == m_request_ptr_cache.end()) {
        m_request_ptr_cache[request_ptr->get_sessionID()] = request_ptr;
        XMETRICS_GAUGE(metrics::xsync_block_send_request, 1);
        return true;
    } else {
        xwarn("xsync_session_manager_t::sync_block_request_insert sessionID(%lx) is exist.", request_ptr->get_sessionID());
    }
    return false;
}

bool xsync_session_manager_t::sync_block_session_check(const xsync_msg_block_response_ptr_t& response_ptr, xsync_msg_block_request_ptr_t& reuqest_ptr)
{
    std::unique_lock<std::mutex> lock(m_session_mutex);
    //enum_sync_block_by_height_lists maybe one request, but multi response 
    if (response_ptr->get_requeset_param_type() == enum_sync_block_by_height_lists) {
        return true;
    }
    
    if (m_request_ptr_cache.find(response_ptr->get_sessionID()) != m_request_ptr_cache.end()) {
        auto& request = m_request_ptr_cache[response_ptr->get_sessionID()];
        if (request->get_address() == response_ptr->get_address() && request->get_option() == response_ptr->get_option()) {
            reuqest_ptr = request;
            m_request_ptr_cache.erase(response_ptr->get_sessionID());
            return true;
        }
    }
    xwarn("xsync_session_manager_t::sync_block_session_check  session(%lx) is not exist.", response_ptr->get_sessionID());
    return false;
}

void xsync_session_manager_t::sync_block_request_timeout_clear()
{
    std::unique_lock<std::mutex> lock(m_session_mutex);
    int64_t now = base::xtime_utl::gmttime_ms();
    XMETRICS_GAUGE_SET_VALUE(metrics::xsync_block_request_count, m_request_ptr_cache.size());
    for (auto it = m_request_ptr_cache.begin(); it!=m_request_ptr_cache.end();) {
        if ((now - it->second->get_time()) > m_timeout) {
            xinfo("sync_block_request_timeout_clear remove timeout sessionID %lx %s", it->first, it->second->dump().c_str());
            m_request_ptr_cache.erase(it++);
        } else {
            ++it;
        }
    }
}

bool xsync_session_manager_t::sync_block_resopnse_valid_check(const xsync_msg_block_response_ptr_t& response_ptr, xsync_msg_block_request_ptr_t& reuqest_ptr)
{
    if (!sync_block_msg_valid_check(response_ptr)) {
        return false;
    }

    if (response_ptr->get_request_type() != enum_sync_block_request_push) {
        if (!sync_block_session_check(response_ptr, reuqest_ptr)) {
            xwarn("xsync_session_manager_t::sync_block_resopnse_valid_check  session(%lx) is not exist.", response_ptr->get_sessionID());
            return false;
        }
    }

    XMETRICS_GAUGE(metrics::xsync_block_recv_response, 1);
    if (response_ptr->get_block_version() != 0) {
        xwarn("xsync_session_manager_t::sync_block_resopnse_valid_check  block version (%d) is not support.", response_ptr->get_block_version());
        return false;
    }

    auto& block_datas = response_ptr->get_blocks_data();
    if (block_datas.size() == 0) {
        xwarn("xsync_session_manager_t::sync_block_resopnse_valid_check  block size is 0.");
        return false;
    }

    return true;
}

bool xsync_session_manager_t::sync_block_request_valid_check(const xsync_msg_block_request_ptr_t& msg)
{
    if (!sync_block_msg_valid_check(msg)) {
        return false;
    }

    if ((msg->get_requeset_param_type() == enum_sync_block_by_hash ||
         msg->get_requeset_param_type() == enum_sync_block_by_txhash) && 
         msg->get_requeset_param_str().empty()) {
        xwarn("xsync_session_manager_t::sync_block_request_valid_check  param is empty");
        return false;
    }

    if (msg->get_request_start_height() == 0) {
        xwarn("xsync_session_manager_t::sync_block_request_valid_check  start_height 0");
        return false;
    }

    return true;
}


bool xsync_session_manager_t::sync_block_push_valid_check(const xsync_msg_block_push_ptr_t& msg_push_ptr)
{
    if (!sync_block_msg_valid_check(msg_push_ptr)) {
        return false;
    }

    if (msg_push_ptr->get_block_version() != 0) {
        xwarn("xsync_session_manager_t::sync_block_push_valid_check  block version (%d) is not support.", msg_push_ptr->get_block_version());
        return false;
    }

    auto& block_datas = msg_push_ptr->get_blocks_data();
    if (block_datas.size() == 0) {
        xwarn("xsync_session_manager_t::sync_block_push_valid_check  block size is 0.");
        return false;
    }

    return true;
}

bool xsync_session_manager_t::sync_block_msg_valid_check(const xobject_ptr_t<xsync_msg_block_t> msg)
{
    if (msg->get_sessionID() == 0) {
        xwarn("xsync_session_manager_t::sync_block_msg_valid_check  msg sessionid is 0.");
        return false;
    }

    if (msg->get_address().empty()) {
        xwarn("xsync_session_manager_t::sync_block_msg_valid_check  msg address is empty.");
        return false;
    }

    if ((msg->get_request_type() < enum_sync_block_request_push) || (msg->get_request_type() > enum_sync_block_request_ontime)) {
        xwarn("xsync_session_manager_t::sync_block_msg_valid_check  msg type(%d) is error ", msg->get_request_type());
        return false;
    }

    if (msg->get_request_type() != enum_sync_block_request_push) {
        if (msg->get_requeset_param_type() < enum_sync_block_by_height) {
            xwarn("xsync_session_manager_t::sync_block_msg_valid_check  msg param(%d) is error ", msg->get_requeset_param_type());
            return false;
        }
    }

    if ((msg->get_requeset_param_type() >= enum_sync_block_by_max)) {
        xwarn("xsync_session_manager_t::sync_block_msg_valid_check  msg request_param(%d) is error ", msg->get_requeset_param_type());
        return false;
    }

    if (msg->get_data_type() == 0) {
        xwarn("xsync_session_manager_t::sync_block_msg_valid_check  msg data type(%d) is error ", msg->get_data_type());
        return false;
    }

    if (msg->get_block_object_type() != 0) {
        xwarn("xsync_session_manager_t::sync_block_msg_valid_check  msg block type(%d) is not support ", msg->get_block_object_type());
        return false;
    }

    return true;
}


NS_END2