#pragma once

#include "xdata/xdata_common.h"
#include "xsync/xchain_info.h"
#include "xdata/xchain_param.h"
#include "xconfig/xconfig_register.h"
#include "xvnetwork/xaddress.h"

NS_BEG2(top, sync)

using map_chain_info_t = std::unordered_map<std::string, xchain_info_t>;

class xchains_wrapper_t {
public:
    void add(const std::string &id, const xchain_info_t &info);
    void merge(const xchains_wrapper_t &other);
    const map_chain_info_t & get_chains() const {return m_chains;}

private:
    map_chain_info_t m_chains;
};

class xrole_chains_t {
public:
    xrole_chains_t(const vnetwork::xvnode_address_t &role, const std::set<uint16_t> &table_ids);
    xchains_wrapper_t& get_chains_wrapper() {return m_chains_wrapper;}
    vnetwork::xvnode_address_t get_role() {return m_role;}

private:
    void init_chains();
    void add_chain(common::xnode_type_t allow_types, const std::string& address, enum_chain_sync_policy sync_policy);
    void add_tables(common::xnode_type_t allow_types, const std::string& address, enum_chain_sync_policy sync_policy);
    void add_rec_or_zec(common::xnode_type_t allow_types, const std::string &address, enum_chain_sync_policy sync_policy);

private:
    vnetwork::xvnode_address_t m_role;
    common::xnode_type_t m_type;
    std::set<uint16_t> m_table_ids;
    xchains_wrapper_t m_chains_wrapper;
};


NS_END2
