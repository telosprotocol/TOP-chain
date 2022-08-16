// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xstore/xtgas_singleton.h"

#include "xdata/xgenesis_data.h"
#include "xdata/xnative_contract_address.h"
#include "xdata/xsystem_contract/xdata_structures.h"
#include "xmetrics/xmetrics.h"
#include "xvledger/xvledger.h"

NS_BEG2(top, store)

uint64_t xtgas_singleton::get_cache_total_lock_tgas_token() {
    uint64_t total_lock_tgas_token = 0;
    std::string value;
    uint64_t height;
    if (get_latest_property(value, height)) {
        total_lock_tgas_token = base::xstring_utl::touint64(value);
        xdbg("xtgas_singleton::get_cache_total_lock_tgas_token success: %llu", total_lock_tgas_token);
    } else {
        xwarn("xtgas_singleton::get_cache_total_lock_tgas_token failed!");
    }
    return total_lock_tgas_token;
}

bool xtgas_singleton::get_latest_property(std::string & value, uint64_t & height) {
    if (!XGET_CONFIG(enable_sharding_contract))
        return false;

    base::xvaccount_t _zec_workload_vaddress(sys_contract_zec_workload_addr);
    auto bstate = base::xvchain_t::instance().get_xstatestore()->get_blkstate_store()->get_latest_connectted_block_state(_zec_workload_vaddress, metrics::statestore_access_from_store_tgas);
    if (bstate == nullptr) {
        xassert(false);
        return false;
    }
    auto propobj = bstate->load_string_var(data::system_contract::XPORPERTY_CONTRACT_TGAS_KEY);
    if (propobj == nullptr) {
        xassert(false);
        return false;
    }
    value = propobj->query();
    if (value.empty()) { // property is empty after create
        // xassert(bstate->get_block_height() == 0);
        return false;
    }
    height = bstate->get_block_height();
    return true;
}

bool xtgas_singleton::leader_get_total_lock_tgas_token(uint64_t timer_height, uint64_t & total_lock_tgas_token, uint64_t & property_height) {
    std::lock_guard<std::mutex> lock(m_mtx);
    total_lock_tgas_token = m_last_total_lock_tgas_token;
    property_height = m_last_property_height;
    if (timer_height - m_last_update_time <= m_tgas_update_interval) {
        return true;
    }

    std::string value;
    uint64_t height;
    if (get_latest_property(value, height)) {
        auto new_total_lock_tgas_token = base::xstring_utl::touint64(value);
        if ((m_last_property_height < height) && (m_last_total_lock_tgas_token != new_total_lock_tgas_token)) {
            m_last_total_lock_tgas_token = new_total_lock_tgas_token;
            m_last_property_height = height;
            total_lock_tgas_token = m_last_total_lock_tgas_token;
            property_height = m_last_property_height;
            xdbg_info("xtgas_singleton::leader_get_total_lock_tgas_token updated. time_height=%ld,total_tgas=%ld,unit_height=%ld",
                timer_height, m_last_total_lock_tgas_token, m_last_property_height);
        }
    }
    m_last_update_time = timer_height;
    return true;
}

bool xtgas_singleton::backup_get_total_lock_tgas_token(uint64_t timer_height, uint64_t property_height, uint64_t & total_lock_tgas_token) {
    if (!XGET_CONFIG(enable_sharding_contract)) {
        total_lock_tgas_token = 0;
        return true;
    }

    if (property_height == 0) {
        total_lock_tgas_token = 0;
        return true;
    }

    std::lock_guard<std::mutex> lock(m_mtx);
    if (property_height == m_last_property_height) {
        total_lock_tgas_token = m_last_total_lock_tgas_token;
        return true;
    }
    base::xvaccount_t _zec_workload_vaddress(sys_contract_zec_workload_addr);
    auto bstate = base::xvchain_t::instance().get_xstatestore()->get_blkstate_store()->get_committed_block_state(_zec_workload_vaddress, property_height, metrics::statestore_access_from_store_backup_tgas);
    if (bstate == nullptr) {
        xwarn("xtgas_singleton::backup_get_total_lock_tgas_token can't load block. height=%ld", property_height);
        return false;
    }
    auto propobj = bstate->load_string_var(data::system_contract::XPORPERTY_CONTRACT_TGAS_KEY);
    if (propobj == nullptr) {
        xassert(false);
        return false;
    }
    std::string value = propobj->query();
    total_lock_tgas_token = base::xstring_utl::touint64(value);
    if (property_height > m_last_property_height) {
        m_last_total_lock_tgas_token = total_lock_tgas_token;
        m_last_property_height = property_height;
        m_last_update_time = timer_height;
        xdbg_info("xtgas_singleton::backup_get_total_lock_tgas_token updated. time_height=%ld,total_tgas=%ld,unit_height=%ld",
            m_last_update_time, m_last_total_lock_tgas_token, m_last_property_height);
    }
    return true;
}

NS_END2
