// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xmbus/xevent_account.h"
#include "xsync/xchain_info.h"
#include "xdata/xblockchain.h"
#include "xsync/xaccount_face.h"
#include "xsync/xsync_range_mgr.h"
#include "xsync/xsync_mgr.h"
#include "xsync/xsync_store.h"
#include "xsync/xsync_sender.h"
#include "xsync/xsync_ratelimit.h"

NS_BEG2(top, sync)

enum enum_result_code {
    success,
    failed,
    pending_auth,
    exist,
    forked,
};

class elect_item_t {
public:
    elect_item_t(const std::string &_address, uint64_t _height):
    address(_address),
    height(_height) {
    }

    std::string address;
    uint64_t height{};
};

class dependency_info_t {
public:
    std::list<elect_item_t> m_list;

    std::string to_string() {
        std::string str;

        for (auto &it: m_list) {
            str += "(";
            str += it.address;
            str += ",";
            str += std::to_string(it.height);
            str += ")";
        }

        return str;
    }
};

class xaccount_base_t : public xaccount_face_t {
public:
    xaccount_base_t(std::string vnode_id,
        xsync_store_face_t *sync_store, const observer_ptr<mbus::xmessage_bus_face_t> &mbus,
        const observer_ptr<base::xvcertauth_t> &certauth,
        xsync_sender_t *sync_sender, xsync_ratelimit_face_t *ratelimit,
        const xchain_info_t &chain_info);

    virtual ~xaccount_base_t();

    std::string get_address() override;

    int64_t get_next_timeout() override;
    void on_timer_event(int64_t now) override;
    void on_response_event(const mbus::xevent_ptr_t &e) override;
    void on_behind_event(const mbus::xevent_ptr_t &e) override;

    bool is_idle() override;

protected:
    enum_result_code handle_block(const base::xauto_ptr<base::xvblock_t> &current_block, xblock_ptr_t &block, bool is_elect_chain);

    void handle_next(const data::xblock_ptr_t &current_block, bool head_forked = false);

    void clear();

    dependency_info_t get_depend_elect_info(const data::xblock_ptr_t &block);
    int64_t get_time();

    bool check_behind(uint64_t height, const char *elect_address);

    int64_t get_next_send_time();

    void send_request(int64_t now);

protected:
    std::string m_vnode_id;
    xsync_store_face_t *m_sync_store;
    observer_ptr<mbus::xmessage_bus_face_t> m_mbus;
    observer_ptr<base::xvcertauth_t> m_certauth;
    xsync_sender_t *m_sync_sender;
    xsync_ratelimit_face_t *m_ratelimit;
    std::string m_address{};
    xsync_range_mgr_t m_sync_range_mgr;
    xsync_mgr_t m_sync_mgr;
    xentire_block_request_ptr_t m_request{nullptr};
};

class xaccount_general_t : public xaccount_base_t {
public:
    xaccount_general_t(std::string vnode_id,
        xsync_store_face_t *sync_store, const observer_ptr<mbus::xmessage_bus_face_t> &mbus,
        const observer_ptr<base::xvcertauth_t> &certauth,
        xsync_sender_t *sync_sender, xsync_ratelimit_face_t *ratelimit,
        const xchain_info_t &chain_info);
    virtual ~xaccount_general_t();

    void on_role_changed(const xchain_info_t &chain_info) override;

    void on_find_block(uint64_t height, uint64_t block_time) override;

    void on_lack_event(const std::set<uint64_t> &set_heights) override;
};

class xaccount_sequence_t : public xaccount_base_t {
public:
    xaccount_sequence_t(std::string vnode_id,
        xsync_store_face_t *sync_store, const observer_ptr<mbus::xmessage_bus_face_t> &mbus,
        const observer_ptr<base::xvcertauth_t> &certauth,
        xsync_sender_t *sync_sender, xsync_ratelimit_face_t *ratelimit,
        const xchain_info_t &chain_info);

    virtual ~xaccount_sequence_t();

    // do nothing
    void on_role_changed(const xchain_info_t &chain_info) override;

    void on_find_block(uint64_t height, uint64_t block_time) override;

    void on_lack_event(const std::set<uint64_t> &set_heights) override;
};

NS_END2
