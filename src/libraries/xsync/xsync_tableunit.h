#pragma once

#include <unordered_map>
#include "xmbus/xevent_account.h"
#include "xsync/xsync_range_mgr.h"

NS_BEG2(top, sync)

class xaccount_pool_t {
public:
    bool update(const std::string &owner, uint64_t height) {
        auto it = m_map_account.find(owner);
        if (it == m_map_account.end()) {
            m_map_account[owner] = height;
            return true;
        }

        if (height > it->second) {
            it->second = height;
            return true;
        }

        return false;
    }

    void clear() {
        m_map_account.clear();
    }

public:
    std::unordered_map<std::string, uint64_t> m_map_account;
};

class xaccount_info_t {
public:
    xaccount_info_t(const std::string &_address,
                uint64_t _height, uint64_t _block_time):
    address(_address),
    height(_height),
    block_time(_block_time) {
    }

    std::string address;
    uint64_t height;
    uint64_t block_time;
};

class xsync_tableunit_t {
public:

    xsync_tableunit_t(std::string vnode_id, const std::string &address,
            mbus::xmessage_bus_face_t *mbus);
#if 0
    void on_history_unit(const data::xblock_ptr_t &block, const syncbase::xdata_mgr_ptr_t &data_mgr);
    bool on_receive_history_unit(const std::string &address, uint64_t height, const syncbase::xdata_mgr_ptr_t &data_mgr);
    void on_active_tableblock(const data::xblock_ptr_t &block);
    void on_history_tableblock(const data::xblock_ptr_t &block);

    bool is_idle();
    void clear();

private:
    void table_find_unit(std::vector<xaccount_info_t> &account_list);
    void clear_block();
#endif

protected:
    std::string m_vnode_id;
    std::string m_address;
    mbus::xmessage_bus_face_t *m_mbus{};
    uint64_t m_history_waiting_height{};
    std::unordered_map<std::string, uint64_t> m_waiting_accounts;
    // sub_account_pool is used to decrease table-find-unit-account event
    xaccount_pool_t m_sub_account_pool;

};

NS_END2
