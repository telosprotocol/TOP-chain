#pragma once

#include "xbasic/xns_macro.h"
#include "xsync/xchain_info.h"
#include "xstore/xstore_face.h"
#include "xmbus/xevent_behind.h"
#include "xdata/xblock.h"

NS_BEG2(top, sync)

enum enum_role_changed_result {
    enum_role_changed_result_none,
    enum_role_changed_result_add_history,
    enum_role_changed_result_remove_history
};

class xsync_range_mgr_t {
public:
    xsync_range_mgr_t(std::string vnode_id, const std::string &address, const observer_ptr<mbus::xmessage_bus_face_t> &mbus);
    virtual ~xsync_range_mgr_t() {
    }

    enum_role_changed_result on_role_changed(const xchain_info_t &chain_info);

    int update_progress(const data::xblock_ptr_t &current_block, bool head_forked);
    bool get_next_behind(const data::xblock_ptr_t &current_block, bool forked, uint32_t count_limit, uint64_t &start_height, uint32_t &count, vnetwork::xvnode_address_t &self_addr, vnetwork::xvnode_address_t &target_addr);

    int set_behind_info(const data::xblock_ptr_t &current_block, const data::xblock_ptr_t &successor_block,
                const vnetwork::xvnode_address_t &self_addr, const vnetwork::xvnode_address_t &target_addr);
    
    void clear_behind_info();
    void get_try_sync_info(uint8_t &try_count, int64_t &try_time);

    uint64_t get_behind_height() const;

private:
    int64_t get_time();

private:
    std::string m_vnode_id;
    std::string m_address;
    xchain_info_t m_chain_info;
    observer_ptr<mbus::xmessage_bus_face_t> m_mbus{};

    // for behind
    uint64_t m_behind_height{0};
    std::string m_behind_hash;

    uint64_t m_successor_height{0};
    uint64_t m_successor_view_id{0};

    uint8_t m_behind_try_count{0};
    int64_t m_behind_try_sync_time{0};
    vnetwork::xvnode_address_t m_behind_self_addr;
    vnetwork::xvnode_address_t m_behind_target_addr;
    int64_t m_behind_update_time{0};
};

NS_END2
