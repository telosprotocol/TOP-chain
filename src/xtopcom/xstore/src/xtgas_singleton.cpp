// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xstake/xstake_algorithm.h"
#include "xdata/xgenesis_data.h"
#include "xstore/xtgas_singleton.h"

NS_BEG2(top, store)

uint64_t xtgas_singleton::get_cache_total_lock_tgas_token() {
    std::lock_guard<std::mutex> lock(m_mtx);
    return m_last_total_lock_tgas_token;
}

bool xtgas_singleton::get_latest_property(base::xvblockstore_t* blockstore, std::string & value, uint64_t & height) {
    base::xvaccount_t _zec_workload_vaddress(sys_contract_zec_workload_addr);
    auto latest_block = blockstore->get_latest_executed_block(_zec_workload_vaddress);
    if (latest_block->is_genesis_block()) {
        return false;
    }

    xblock_t* zec_workload_block_ptr = dynamic_cast<xblock_t*>(latest_block.get());
    int32_t ret = zec_workload_block_ptr->get_native_property().native_string_get(xstake::XPORPERTY_CONTRACT_TGAS_KEY, value);
    if (ret == xsuccess) {
        height = zec_workload_block_ptr->get_height();
        xdbg_info("xtgas_singleton::get_latest_property find property in latest_height=%ld", latest_block->get_height());
        return true;
    }
    uint64_t next_height = latest_block->get_height() - 1;
    while (next_height >= 1) {
        auto next_block = blockstore->load_block_object(_zec_workload_vaddress, next_height, base::enum_xvblock_flag_committed, true);
        if (next_block == nullptr) {
            xerror("xtgas_singleton::get_latest_property block not exist. height=%ld", next_height);
            return false;
        }
        zec_workload_block_ptr = dynamic_cast<xblock_t*>(next_block.get());
        ret = zec_workload_block_ptr->get_native_property().native_string_get(xstake::XPORPERTY_CONTRACT_TGAS_KEY, value);
        if (ret == xsuccess) {
            height = zec_workload_block_ptr->get_height();
            xdbg_info("xtgas_singleton::get_latest_property find property in height=%ld, latest_height:%ld",
                next_height, latest_block->get_height());
            return true;
        }
        next_height = next_block->get_height() - 1;
    }
    return false;
}

bool xtgas_singleton::leader_get_total_lock_tgas_token(base::xvblockstore_t* blockstore, uint64_t timer_height, uint64_t & total_lock_tgas_token, uint64_t & property_height) {
    std::lock_guard<std::mutex> lock(m_mtx);
    total_lock_tgas_token = m_last_total_lock_tgas_token;
    property_height = m_last_property_height;
    if (timer_height - m_last_update_time <= m_tgas_update_interval) {
        return true;
    }

    std::string value;
    uint64_t height;
    if (get_latest_property(blockstore, value, height)) {
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

bool xtgas_singleton::backup_get_total_lock_tgas_token(base::xvblockstore_t* blockstore, uint64_t timer_height, uint64_t property_height, uint64_t & total_lock_tgas_token) {
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
    auto zec_workload_block = blockstore->load_block_object(_zec_workload_vaddress, property_height, base::enum_xvblock_flag_committed, true);
    if (zec_workload_block == nullptr) {
        xwarn("xtgas_singleton::backup_get_total_lock_tgas_token can't load block. height=%ld", property_height);
        return false;
    }

    xblock_t* zec_workload_block_ptr = dynamic_cast<xblock_t*>(zec_workload_block.get());
    std::string value;
    int32_t ret = zec_workload_block_ptr->get_native_property().native_string_get(xstake::XPORPERTY_CONTRACT_TGAS_KEY, value);
    if (ret) {
        xerror("xtable_blockmaker_t::backup_get_total_lock_tgas_token can't read property. height=%ld", zec_workload_block->get_height());
        return false;
    }
    total_lock_tgas_token = base::xstring_utl::touint64(value);
    property_height = zec_workload_block->get_height();

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
