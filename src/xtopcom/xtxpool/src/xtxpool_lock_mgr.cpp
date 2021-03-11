// Copyright (c) 2017-2020 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xtxpool/xtxpool_lock_mgr.h"

#include "xdata/xtableblock.h"

NS_BEG2(top, xtxpool)

xtxpool_lock_mgr::~xtxpool_lock_mgr() {

}

void xtxpool_lock_mgr::update_blocks(const base::xblock_mptrs & latest_blocks) {
    m_lock_accounts.clear();
    base::xvblock_t* cert_block = latest_blocks.get_latest_cert_block();
    if (false == cert_block->check_block_flag(base::enum_xvblock_flag_committed)
        && cert_block->get_block_class() == base::enum_xvblock_class_light) {
        data::xtable_block_t* tableblock = dynamic_cast<data::xtable_block_t*>(cert_block);
        xassert(tableblock != nullptr);
        const auto & units = tableblock->get_tableblock_units();
        for (auto & v : units) {
            m_lock_accounts[v->get_account()] = cert_block->get_height();
        }
    }

    base::xvblock_t* lock_block = latest_blocks.get_latest_locked_block();
    if (false == lock_block->check_block_flag(base::enum_xvblock_flag_committed)
        && lock_block->get_block_class() == base::enum_xvblock_class_light) {
        data::xtable_block_t* tableblock = dynamic_cast<data::xtable_block_t*>(lock_block);
        xassert(tableblock != nullptr);
        const auto & units = tableblock->get_tableblock_units();
        for (auto & v : units) {
            m_lock_accounts[v->get_account()] = lock_block->get_height();
        }
    }
}

bool xtxpool_lock_mgr::is_account_lock(const std::string & account) {
    auto iter = m_lock_accounts.find(account);
    if (iter == m_lock_accounts.end()) {
        return false;
    }
    return true;
}

NS_END2
