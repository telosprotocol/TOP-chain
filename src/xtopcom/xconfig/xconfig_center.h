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
#include "xcommon/xaccount_address.h"

NS_BEG2(top, config)
enum enum_addr_type {
    enum_address_table,
    enum_address_chain,
    enum_address_contract,
};
struct xsync_config {
    xsync_config(common::xnode_type_t type, const std::string & addr, sync::enum_chain_sync_policy policy, enum_addr_type addr_type)
      : m_node_type(type), m_sync_addr(addr), m_sync_policy(policy), m_addr_type(addr_type) {
    }
    common::xnode_type_t m_node_type;
    std::string m_sync_addr;
    sync::enum_chain_sync_policy m_sync_policy;
    enum_addr_type m_addr_type;
};
struct xtxpool_config {
    xtxpool_config() {}
    xtxpool_config(bool role, base::enum_xchain_zone_index index, common::xnode_type_t type):m_is_send_receipt_role(role), m_zone_index(index), m_node_type(type) {}
    bool m_is_send_receipt_role;
    base::enum_xchain_zone_index m_zone_index;
    common::xnode_type_t m_node_type;
};

struct xchain_config {
    xchain_config() {}
    xchain_config(store::prune_type prune_type, enum_addr_type addr_type, uint32_t used_number): m_prune_type(prune_type), m_addr_type(addr_type), m_used_number(used_number) {}
    std::vector<xsync_config> m_sync_config;
    store::prune_type m_prune_type;
    enum_addr_type m_addr_type;
    std::map<common::xnode_type_t, xtxpool_config> m_txpool_config;
    uint32_t m_used_number;
};

class xconfig_center {
public:
    xconfig_center();
    static xconfig_center& instance();

    std::vector<std::string> get_table_accounts();
    std::vector<std::string> get_system_contract_accounts();
private:
    void init_xchain_config();
    void init_prune_table();
private:
    std::map<std::string, xchain_config> m_chain_config;
    std::map<std::string, xchain_config> m_contract_config;
};


NS_END2