#pragma once

#include <memory>
#include "xsync/xrole_chains.h"
#include "xmbus/xmessage_bus.h"

NS_BEG2(top, sync)

using xsync_roles_t = std::unordered_map<vnetwork::xvnode_address_t, std::shared_ptr<xrole_chains_t>>;

class xrole_chains_mgr_t {
public:
    xrole_chains_mgr_t(std::string vnode_id);
    void add_role(std::shared_ptr<xrole_chains_t> &role_chains);
    void remove_role(std::shared_ptr<xrole_chains_t> &role_chains);
    map_chain_info_t get_all_chains();

    bool exists(const std::string &address);

    xsync_roles_t get_roles();
    std::string get_roles_string();
    std::shared_ptr<xrole_chains_t> get_role(const vnetwork::xvnode_address_t &self_address);

private:
    map_chain_info_t calc_union_set();
    map_chain_info_t calc_add_diff(map_chain_info_t &old_chains, map_chain_info_t &new_chains);

private:
    std::string m_vnode_id;

    std::mutex m_lock;
    xsync_roles_t m_roles;

protected:
    map_chain_info_t m_chains;
};

NS_END2