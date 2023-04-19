// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <string>
#include "xbasic/xmemory.hpp"
#include "xstatestore/xunitstate_cache.h"
#include "xstatestore/xerror.h"

NS_BEG2(top, statestore)

xaccount_state_cache_t::xaccount_state_cache_t(std::string const& unit_hash, xobject_ptr_t<base::xvbstate_t> const& bstate)
: m_unit_hash(unit_hash),m_bstate(bstate)  {
    xdbg("xaccount_state_cache_t::xaccount_state_cache_t new %s,height=%ld,hash=%s",bstate->get_account().c_str(),bstate->get_block_height(),base::xstring_utl::to_hex(unit_hash).c_str());
}
xaccount_state_cache_t::~xaccount_state_cache_t() {
    if (nullptr != m_bstate) {
        xdbg("xaccount_state_cache_t::~xaccount_state_cache_t %s,height=%ld,hash=%s",m_bstate->get_account().c_str(),m_bstate->get_block_height(),base::xstring_utl::to_hex(m_unit_hash).c_str());
        m_bstate->close();
        m_bstate = nullptr;
    }
}

xobject_ptr_t<base::xvbstate_t> xaccount_state_cache_t::get_bstate(uint64_t height, std::string const& unit_hash) const {
    if (m_unit_hash == unit_hash && height == m_bstate->get_block_height()) {        
        return m_bstate;
    }    
    return nullptr;
}

void xaccount_state_cache_t::set_bstate(std::string const& unit_hash, xobject_ptr_t<base::xvbstate_t> const& bstate) {
    xdbg("xaccount_state_cache_t::set_bstate erase %s,height=%ld,hash=%s",m_bstate->get_account().c_str(),m_bstate->get_block_height(),base::xstring_utl::to_hex(m_unit_hash).c_str());
    // close old one
    m_bstate->close();
    m_bstate = nullptr;    
    m_unit_hash = unit_hash;
    m_bstate = bstate;
    xdbg("xaccount_state_cache_t::set_bstate new %s,height=%ld,hash=%s",bstate->get_account().c_str(),bstate->get_block_height(),base::xstring_utl::to_hex(unit_hash).c_str());
}

xcontract_state_cache_t::xcontract_state_cache_t(std::string const& unit_hash, xobject_ptr_t<base::xvbstate_t> const& bstate) {
    xassert(!unit_hash.empty());
    xdbg("xcontract_state_cache_t::xcontract_state_cache_t new %s,height=%ld,hash=%s",bstate->get_account().c_str(),bstate->get_block_height(),base::xstring_utl::to_hex(unit_hash).c_str());
    m_states[bstate->get_block_height()] = std::make_pair(unit_hash, bstate);
}
xcontract_state_cache_t::~xcontract_state_cache_t() {
    for (auto & v : m_states) {
        xdbg("xcontract_state_cache_t::~xcontract_state_cache_t %s,height=%ld,hash=%s",v.second.second->get_account().c_str(),v.second.second->get_block_height(),base::xstring_utl::to_hex(v.second.first).c_str());
        v.second.second->close();
    }
    m_states.clear();
}

xobject_ptr_t<base::xvbstate_t> xcontract_state_cache_t::get_bstate(uint64_t height, std::string const& unit_hash) const {
    auto iter = m_states.find(height);
    if (iter != m_states.end()) {
        if (iter->second.first == unit_hash) {            
            return iter->second.second;
        }
    }
    return nullptr;
}

void xcontract_state_cache_t::set_bstate(std::string const& unit_hash, xobject_ptr_t<base::xvbstate_t> const& bstate) {
    xassert(!unit_hash.empty());
    auto iter = m_states.find(bstate->get_block_height());
    if (iter != m_states.end()) {
        if (iter->second.first == unit_hash) {            
            return;
        }
        xdbg("xcontract_state_cache_t::set_bstate erase %s,height=%ld,hash=%s",iter->second.second->get_account().c_str(),iter->second.second->get_block_height(),base::xstring_utl::to_hex(iter->second.first).c_str());
        iter->second.second->close();
        m_states.erase(iter);
    }
    xdbg("xcontract_state_cache_t::set_bstate new %s,height=%ld,hash=%s",bstate->get_account().c_str(),bstate->get_block_height(),base::xstring_utl::to_hex(unit_hash).c_str());
    m_states[bstate->get_block_height()] = std::make_pair(unit_hash, bstate);

    if (m_states.size() > enum_max_state_count) {
        auto iter = m_states.begin();
        xdbg("xcontract_state_cache_t::set_bstate erase %s,height=%ld,hash=%s",iter->second.second->get_account().c_str(),iter->second.second->get_block_height(),base::xstring_utl::to_hex(iter->second.first).c_str());
        iter->second.second->close();
        m_states.erase(iter);
        assert(m_states.size() == enum_max_state_count);
    }
}

xunitstate_cache_t::xunitstate_cache_t(uint32_t cache_size)
: m_unitstate_cache(cache_size) {

}

xobject_ptr_t<base::xvbstate_t> xunitstate_cache_t::get_unitstate(std::string const& address, uint64_t height, std::string const& unit_hash) const {
    xaccount_state_cache_face_ptr_t account_cache = nullptr;
    {
        std::lock_guard<std::mutex> _l(m_mutex);
        m_unitstate_cache.get(address, account_cache);  
    }
    if (nullptr != account_cache) {
        xobject_ptr_t<base::xvbstate_t> bstate = account_cache->get_bstate(height, unit_hash);
        if (nullptr != bstate) {
            XMETRICS_GAUGE(metrics::statestore_get_unit_state_from_cache, 1);
            xdbg("xunitstate_cache_t::get_unitstate succ.%s,height=%ld,hash=%s",address.c_str(), height, base::xstring_utl::to_hex(unit_hash).c_str());
            return bstate;
        }
    }
    XMETRICS_GAUGE(metrics::statestore_get_unit_state_from_cache, 0);
    xdbg_info("xunitstate_cache_t::get_unitstate fail.%s,height=%ld,hash=%s",address.c_str(), height, base::xstring_utl::to_hex(unit_hash).c_str());
    return nullptr;
}

void xunitstate_cache_t::set_unitstate(std::string const& unit_hash, xobject_ptr_t<base::xvbstate_t> const& bstate) {
    xaccount_state_cache_face_ptr_t account_cache = nullptr;
    std::lock_guard<std::mutex> _l(m_mutex);
    m_unitstate_cache.get(bstate->get_account(), account_cache);
    if (nullptr != account_cache) {
        account_cache->set_bstate(unit_hash, bstate);
        xdbg("xunitstate_cache_t::set_unitstate update.%s,hash=%s",bstate->dump().c_str(), base::xstring_utl::to_hex(unit_hash).c_str());
        return;
    }
    xaccount_state_cache_face_ptr_t new_account_cache;
    base::xvaccount_t vaccount(bstate->get_account()); // TODO(jimmy)
    if (vaccount.is_contract_address()) {
        new_account_cache = std::make_shared<xcontract_state_cache_t>(unit_hash, bstate);
    } else {
        new_account_cache = std::make_shared<xaccount_state_cache_t>(unit_hash, bstate);
    }
    m_unitstate_cache.put(bstate->get_account(), new_account_cache);
    xdbg("xunitstate_cache_t::set_unitstate new.%s,hash=%s",bstate->dump().c_str(), base::xstring_utl::to_hex(unit_hash).c_str());
}

void xunitstate_cache_t::clear() {
    std::lock_guard<std::mutex> _l(m_mutex);
    m_unitstate_cache.clear();
}

size_t xunitstate_cache_t::size() const {
    std::lock_guard<std::mutex> _l(m_mutex);
    return m_unitstate_cache.size();
}

NS_END2