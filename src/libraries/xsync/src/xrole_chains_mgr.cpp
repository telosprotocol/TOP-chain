#include "xsync/xrole_chains_mgr.h"
#include "xdata/xgenesis_data.h"
#include "xsync/xsync_log.h"
#include "xsync/xsync_util.h"

NS_BEG2(top, sync)

using namespace data;

xrole_chains_mgr_t::xrole_chains_mgr_t(std::string vnode_id):
m_vnode_id(vnode_id) {
}

void xrole_chains_mgr_t::add_role(std::shared_ptr<xrole_chains_t> &role_chains) {

    vnetwork::xvnode_address_t role = role_chains->get_role();

    std::unique_lock<std::mutex> lock(m_lock);

    for (auto& pair : m_roles) {
        if (pair.first.cluster_address() == role.cluster_address()) {
            if (pair.first.logical_version() >= role.logical_version()) {
                xwarn("version error %s %s", pair.first.to_string().c_str(), role.to_string().c_str());
                return;
            }
            m_roles.erase(pair.first);
            break;
        }
    }

    m_roles[role] = role_chains;

    map_chain_info_t new_chains = calc_union_set();
    map_chain_info_t diff = calc_add_diff(m_chains, new_chains);

    for (auto &it: diff) {
        auto it2 = m_chains.find(it.first);
        if (it2 == m_chains.end()) {
            m_chains[it.first] = it.second;
        } else {
            if (it.second != it2->second) {
                it2->second = it.second;
            } else {
                assert(0);
            }
        }
    }
}

void xrole_chains_mgr_t::remove_role(std::shared_ptr<xrole_chains_t> &role_chains) {

    vnetwork::xvnode_address_t role = role_chains->get_role();

    std::unique_lock<std::mutex> lock(m_lock);

    m_roles.erase(role);
    auto new_chains = calc_union_set();

    map_chain_info_t::iterator it = m_chains.begin();
    for (; it!=m_chains.end(); ) {
        auto it2 = new_chains.find(it->first);
        if (it2 == new_chains.end()) {
            // not exist, remove
            m_chains.erase(it++);
        } else {
            // if exist, check if is equal
            if (it->second != it2->second) {
                it->second = it2->second;
            }
            ++it;
        }
    }
}

map_chain_info_t xrole_chains_mgr_t::get_all_chains() {
    std::unique_lock<std::mutex> lock(m_lock);
    map_chain_info_t chains = m_chains;;
    return chains;
}

map_chain_info_t xrole_chains_mgr_t::calc_union_set() {

    xchains_wrapper_t chains_wrapper;

    for (auto &it: m_roles) {
        const xchains_wrapper_t &other_chains = it.second->get_chains_wrapper();
        chains_wrapper.merge(other_chains);
    }

    map_chain_info_t new_chains = chains_wrapper.get_chains();

    return new_chains;
}

map_chain_info_t xrole_chains_mgr_t::calc_add_diff(map_chain_info_t &old_chains, map_chain_info_t &new_chains) {

    map_chain_info_t diff;

    for (auto &it: new_chains) {
        auto it2 = old_chains.find(it.first);

        if (it2 != old_chains.end()) {
            if (it.second != it2->second) {
                diff[it.first] = it.second;
            }
        } else {
            diff[it.first] = it.second;
        }
    }
    return diff;
}

bool xrole_chains_mgr_t::exists(const std::string &address) {
    std::unique_lock<std::mutex> lock(m_lock);
    {
        auto it = m_chains.find(address);
        if (it != m_chains.end()) {
            return true;
        }
    }

    return false;
}

xsync_roles_t xrole_chains_mgr_t::get_roles() {

    xsync_roles_t roles;

    std::unique_lock<std::mutex> lock(m_lock);

    roles = m_roles;
    return roles;
}

std::string xrole_chains_mgr_t::get_roles_string() {
    std::unique_lock<std::mutex> lock(m_lock);

    std::string str = "(";

    int count = 0;
    for (auto &it: m_roles) {
        if (count > 0)
            str += ",";
        str += it.first.to_string();
        count++;
    }

    str += ")";

    return str;
}

std::shared_ptr<xrole_chains_t> xrole_chains_mgr_t::get_role(const vnetwork::xvnode_address_t &self_address) {
    std::unique_lock<std::mutex> lock(m_lock);

    auto it = m_roles.find(self_address);
    if (it == m_roles.end())
        return nullptr;

    return it->second;
}

NS_END2
