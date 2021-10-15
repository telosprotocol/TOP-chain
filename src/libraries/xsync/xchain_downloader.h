// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <set>
#include "xmbus/xevent_account.h"
#include "xsync/xchain_info.h"
#include "xsync/xsync_range_mgr.h"
#include "xsync/xsync_store.h"
#include "xsync/xsync_sender.h"
#include "xsync/xsync_ratelimit.h"
#include "xsync/xrequest.h"
#include "xsync/xsync_task.h"

NS_BEG2(top, sync)

#define GET_TOKEN_RETRY_INTERVAL 6000

class xchain_downloader_face_t {
public:
    virtual ~xchain_downloader_face_t() {}

public:
    virtual const std::string& get_address() const = 0;
    virtual void destroy() = 0;
    virtual void on_role_changed(const xchain_info_t &chain_info) = 0;
    virtual bool on_timer(int64_t now) = 0;
    virtual bool downloading(int64_t now) = 0;

    virtual void on_response(std::vector<data::xblock_ptr_t> &blocks, const vnetwork::xvnode_address_t &self_addr, const vnetwork::xvnode_address_t &from_addr) = 0;
    virtual void on_archive_blocks(std::vector<data::xblock_ptr_t> &blocks, const vnetwork::xvnode_address_t &self_addr, const vnetwork::xvnode_address_t &from_addr) = 0;
    virtual void on_behind(uint64_t start_height, uint64_t end_height, enum_chain_sync_policy sync_policy, const vnetwork::xvnode_address_t &self_addr, const vnetwork::xvnode_address_t &target_addr, const std::string &reason) = 0;
    virtual void on_chain_snapshot_response(const std::string & chain_snapshot, uint64_t height, const vnetwork::xvnode_address_t &self_addr, const vnetwork::xvnode_address_t &from_addr) = 0;
    virtual void on_block_committed_event(uint64_t height) = 0;
};

using xchain_downloader_face_ptr_t = std::shared_ptr<xchain_downloader_face_t>;

enum enum_result_code {
    success,
    failed,
    auth_failed,
    exist,
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

class xchain_object_t {
    public:
        xchain_object_t(){
        };

        xchain_object_t(const uint64_t start_height, const uint64_t end_height, const vnetwork::xvnode_address_t &self_addr, const vnetwork::xvnode_address_t &target_addr):
            m_start_height(start_height), m_end_height(end_height), m_self_addr(self_addr), m_target_addr(target_addr){
        };

        bool pick(std::pair<uint64_t, uint64_t> &interval, vnetwork::xvnode_address_t &self_addr, vnetwork::xvnode_address_t &target_addr);
        uint64_t height();
        void set_height(uint64_t height);
        void set_picked_height(uint64_t height);
        uint64_t picked_height();
        void clear();
    private:
        uint64_t m_picked_height{0};
        uint64_t m_current_height{1};
        uint64_t m_start_height{0};
        uint64_t m_end_height{0};
        vnetwork::xvnode_address_t m_self_addr;
        vnetwork::xvnode_address_t m_target_addr;
};

class xchain_downloader_t : public xchain_downloader_face_t {
public:
    xchain_downloader_t(std::string vnode_id,
        xsync_store_face_t *sync_store, const observer_ptr<mbus::xmessage_bus_face_t> &mbus,
        const observer_ptr<base::xvcertauth_t> &certauth,
        xsync_sender_t *sync_sender, xsync_ratelimit_face_t *ratelimit,
        const std::string &address);

    virtual ~xchain_downloader_t();

    const std::string& get_address() const override;
    void destroy() override;

    void on_role_changed(const xchain_info_t &chain_info) override;
    bool on_timer(int64_t now) override;
    bool downloading(int64_t now) override;
    void on_response(std::vector<data::xblock_ptr_t> &blocks, const vnetwork::xvnode_address_t &self_addr, const vnetwork::xvnode_address_t &from_addr) override;
    void on_archive_blocks(std::vector<data::xblock_ptr_t> &blocks, const vnetwork::xvnode_address_t &self_addr, const vnetwork::xvnode_address_t &from_addr) override;
    void on_behind(uint64_t start_height, uint64_t end_height, enum_chain_sync_policy sync_policy, const vnetwork::xvnode_address_t &self_addr, const vnetwork::xvnode_address_t &target_addr, const std::string &reason) override;
    void on_chain_snapshot_response(const std::string & chain_snapshot, uint64_t height, const vnetwork::xvnode_address_t &self_addr, const vnetwork::xvnode_address_t &from_addr) override;
    void on_block_committed_event(uint64_t height) override;
    xsync_command_execute_result execute_next_download(const std::string & chain_snapshot, uint64_t height, const vnetwork::xvnode_address_t &self_addr, const vnetwork::xvnode_address_t &from_addr);
    xsync_command_execute_result execute_next_download(std::vector<data::xblock_ptr_t> &blocks, const vnetwork::xvnode_address_t &self_addr, const vnetwork::xvnode_address_t &from_addr);
    xsync_command_execute_result execute_next_download(uint64_t height);
    xsync_command_execute_result execute_download(uint64_t start_height, uint64_t end_height, enum_chain_sync_policy sync_policy, const vnetwork::xvnode_address_t &self_addr, const vnetwork::xvnode_address_t &target_addr, const std::string &reason);
protected:
    enum_result_code handle_block(xblock_ptr_t &block, bool is_elect_chain, uint64_t quota_height);
    enum_result_code pre_handle_block(std::vector<data::xblock_ptr_t> &blocks, bool is_elect_chain, uint64_t quota_height, std::vector<base::xvblock_t*> &processed_blocks);

    xsync_command_execute_result handle_next(uint64_t current_height);
    bool handle_fulltable(uint64_t fulltable_height_of_tablechain, const vnetwork::xvnode_address_t self_addr, const vnetwork::xvnode_address_t target_addr);
    void clear();

    dependency_info_t get_depend_elect_info(const data::xblock_ptr_t &block);
    int64_t get_time();

    bool check_behind(uint64_t height, const char *elect_address);

    bool send_request(int64_t now);
    bool send_request(int64_t now, const xsync_message_chain_snapshot_meta_t &chain_snapshot_meta);
    xentire_block_request_ptr_t create_request(uint64_t start_height, uint32_t count);

private:
    void init_committed_event_group();
    void wait_committed_event_group(uint64_t height, uint64_t quota_height);
    bool notify_committed_event_group(uint64_t height);
    bool notified_committed_event_group();
    enum_result_code handle_archive_block(xblock_ptr_t &block, bool is_elect_chain, uint64_t quota_height);
protected:
    std::string m_vnode_id;
    xsync_store_face_t *m_sync_store;
    observer_ptr<mbus::xmessage_bus_face_t> m_mbus;
    observer_ptr<base::xvcertauth_t> m_certauth;
    xsync_sender_t *m_sync_sender;
    xsync_ratelimit_face_t *m_ratelimit;
    std::string m_address{};
    xsync_range_mgr_t m_sync_range_mgr;
    xentire_block_request_ptr_t m_request{nullptr};
private:
    xchain_object_t m_chain_objects[enum_chain_sync_policy_max];
    uint32_t m_current_object_index{enum_chain_sync_policy_fast};
    xsync_task_t m_task{this};
    uint32_t m_continuous_times{0};
    std::set<uint64_t> m_wait_committed_event_group;
    uint64_t m_refresh_time;
};

NS_END2
