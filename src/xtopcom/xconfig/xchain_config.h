// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once
#include <string>
#include <vector>
#include "xcommon/xnode_type.h"
#include "xsyncbase/xsync_policy.h"
#include "xblockstore/src/xvblockpruner.h"
#include "xvledger/xvaccount.h"

NS_BEG2(top, config)
enum enum_xsync_addr_type {
    enum_xsync_address_table,
    enum_xsync_address_chain,
};
struct xtxpool_config {
    xtxpool_config() {}
    xtxpool_config(bool role, base::enum_xchain_zone_index index, common::xnode_type_t type):m_is_send_receipt_role(role), m_zone_index(index), m_node_type(type) {}
    bool m_is_send_receipt_role;
    base::enum_xchain_zone_index m_zone_index;
    common::xnode_type_t m_node_type;
};
struct xsync_config {
    xsync_config(common::xnode_type_t type, const std::string & addr, sync::enum_chain_sync_policy policy, enum_xsync_addr_type addr_type)
      : m_node_type(type), m_sync_addr(addr), m_sync_policy(policy), m_addr_type(addr_type) {
    }
    common::xnode_type_t m_node_type;
    std::string m_sync_addr;
    sync::enum_chain_sync_policy m_sync_policy;
    enum_xsync_addr_type m_addr_type;
};

class xchain_config {
public:
    xchain_config();
    static xchain_config& instance();
private:
    void init_xsync_config();
    void init_prune_contract();
    void init_prune_table();
    void init_txpool_config();
private:
    std::vector<xsync_config> m_xsync_config;
    std::map<std::string, store::prune_type> m_prune_contract;
    std::map<std::string, store::prune_type> m_prune_table;
    std::map<common::xnode_type_t, xtxpool_config> m_txpool_config;
};

NS_END2