// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <string>
#include "xdata/xblock.h"
#include "xdata/xblocktool.h"
#include "xtxexecutor/xtxexecutor_face.h"

NS_BEG2(top, txexecutor)

bool xblock_maker_t::update_account_state(const xblock_ptr_t & latest_committed_block) {
    m_commit_account = get_store()->query_account(get_account());
    if (m_commit_account == nullptr) {
        m_commit_account = make_object_ptr<xblockchain2_t>(get_account());
    }
    if (m_commit_account->get_last_height() != latest_committed_block->get_height()) {
        xerror("xblock_maker_t::update_account_state fail-load commmit account. account=%s,actual_account_height=%ld,demand_account_height=%ld",
            get_account().c_str(), m_commit_account->get_last_height(), latest_committed_block->get_height());
        return false;
    }
    return true;
}

bool xblock_maker_t::is_latest_blocks_valid(const base::xblock_mptrs & latest_blocks) {
    if (!data::xblocktool_t::is_connect_and_executed_block(latest_blocks.get_latest_committed_block())) {
        xwarn("xblock_maker_t::is_latest_blocks_valid, block flag hehind. commit_block=%s",
            latest_blocks.get_latest_committed_block()->dump().c_str());
        return false;
    }

    // TODO(jimmy) table chain should always has different height of commit/lock/highqc, but unit chain may has same height of commit/lock/highqc
    // because blockstore will always unpack tableblock in committed status
    if (base::enum_vaccount_addr_type_block_contract == get_addr_type()) {
        if (!xblocktool_t::verify_latest_blocks(latest_blocks)) {
            xerror("xblock_maker_t::is_latest_blocks_valid. verify_latest_blocks fail. latest_cert_block=%s", latest_blocks.get_latest_cert_block()->dump().c_str());
            return false;
        }
    } else {
        if (latest_blocks.get_latest_cert_block()->get_height() != latest_blocks.get_latest_locked_block()->get_height()
            || latest_blocks.get_latest_cert_block()->get_height() != latest_blocks.get_latest_committed_block()->get_height()) {
            if (!xblocktool_t::verify_latest_blocks(latest_blocks)) {
                xerror("xblock_maker_t::is_latest_blocks_valid. verify_latest_blocks fail. latest_cert_block=%s", latest_blocks.get_latest_cert_block()->dump().c_str());
                return false;
            }
        }
    }
    return true;
}

bool xblock_maker_t::update_latest_state(const xblock_ptr_t & latest_committed_block) {
    if (m_latest_commit_block != nullptr && m_latest_commit_block->get_height() == latest_committed_block->get_height()) {
        return true;
    }

    if (!update_account_state(latest_committed_block)) {
        return false;
    }
    m_latest_commit_block = latest_committed_block;
    return true;
}

std::vector<xblock_ptr_t> xblock_maker_t::get_uncommit_blocks() const {
    std::vector<xblock_ptr_t> uncommit_blocks;
    for (auto iter = m_latest_blocks.rbegin(); iter != m_latest_blocks.rend(); iter++) {
        if (iter->first <= m_latest_commit_block->get_height()) {
            break;
        }
        uncommit_blocks.push_back(iter->second);
    }
    return uncommit_blocks;
}

bool xblock_maker_t::is_latest_state_unchanged(base::xvblock_t* latest_block) const {
    if ( (!m_latest_blocks.empty())
        && latest_block->get_block_hash() == get_proposal_prev_block()->get_block_hash()) {
        // already latest state
        return true;
    }
    return false;
}

void xblock_maker_t::set_latest_blocks(const base::xblock_mptrs & latest_blocks) {
    base::xvblock_t* commit_block = latest_blocks.get_latest_committed_block();
    base::xvblock_t* lock_block = latest_blocks.get_latest_locked_block();
    base::xvblock_t* cert_block = latest_blocks.get_latest_cert_block();
    // update latest blocks
    commit_block->add_ref();
    m_latest_commit_block.attach((data::xblock_t*)commit_block);
}

xblock_ptr_t xblock_maker_t::set_latest_block(const xblock_ptr_t & block) {
    xassert(block->get_account() == get_account());
    auto iter = m_latest_blocks.find(block->get_height());
    if (iter != m_latest_blocks.end()) {
        if (iter->second->get_block_hash() != block->get_block_hash()) {
            xassert(iter->second->get_viewid() != block->get_viewid());
            xblock_ptr_t delete_block = iter->second;
            m_latest_blocks[block->get_height()] = block;
            xinfo("xblock_maker_t::set_latest_block block_cache_change forked, delete old_block=%s, add new block=%s", delete_block->dump().c_str(), block->dump().c_str());
            return delete_block;
        }
    } else {
        m_latest_blocks[block->get_height()] = block;
        xdbg("xblock_maker_t::set_latest_block block block_cache_change add new block=%s", block->dump().c_str());
    }

    if (m_latest_blocks.size() > m_keep_latest_blocks_max) {
        xblock_ptr_t delete_block = m_latest_blocks.begin()->second;
        xassert(delete_block->get_block_hash() != block->get_block_hash());
        xdbg("xblock_maker_t::set_latest_block block block_cache_change delete old block=%s", delete_block->dump().c_str());
        m_latest_blocks.erase(m_latest_blocks.begin());
        return delete_block;
    }
    return block;
}

bool xblock_maker_t::load_latest_blocks(const xblock_ptr_t & latest_block, std::map<uint64_t, xblock_ptr_t> & latest_blocks) {
    xassert(latest_blocks.empty());
    latest_blocks[latest_block->get_height()] = latest_block;
    xdbg("xblock_maker_t::load_latest_blocks start.account=%s,latest_height=%ld", get_account().c_str(), latest_block->get_height());
    xblock_ptr_t current_block = latest_block;
    uint32_t count = 0;
    while (current_block->get_height() > 0) {
        uint64_t prev_height = current_block->get_height() - 1;
        xblock_ptr_t prev_block = get_latest_block(prev_height);
        if (prev_block != nullptr && prev_block->get_block_hash() == current_block->get_last_block_hash()) {
            xdbg("xblock_maker_t::load_latest_blocks finish prev block in cache.account=%s,latest_height=%ld", get_account().c_str(), latest_block->get_height());
            return true;
        }
        if (count > m_keep_latest_blocks_max) {
            xdbg("xtable_maker_t::load_latest_blocks finish arrive max times.account=%s,latest_height=%ld", get_account().c_str(), latest_block->get_height());
            return true;
        }
        auto _block = get_blockstore()->load_block_object(*this, prev_height);
        if (_block == nullptr) {
            xerror("xtable_maker_t::load_latest_blocks fail-load block.account=%s,height=%ld", get_account().c_str(), prev_height);
            return false;
        }
        if (_block->get_block_hash() != current_block->get_last_block_hash()) {
            xerror("xtable_maker_t::load_latest_blocks fail- not match prev.prev=%s,current=%s", _block->dump().c_str(), current_block->dump().c_str());
            return false;
        }
        prev_block = xblock_t::raw_vblock_to_object_ptr(_block.get());
        latest_blocks[prev_block->get_height()] = prev_block;
        current_block = prev_block;
        count++;
    }
    xdbg("xblock_maker_t::load_latest_blocks finish reach genesis.account=%s,latest_height=%ld,current_height=%ld",
        get_account().c_str(), latest_block->get_height(), current_block->get_height());
    return true;
}

void xblock_maker_t::clear_block(const xblock_ptr_t & block) {
    auto iter = m_latest_blocks.find(block->get_height());
    if (iter != m_latest_blocks.end()) {
        xassert(block->get_block_hash() == iter->second->get_block_hash());
        if (block->get_block_hash() == iter->second->get_block_hash()) {
            xdbg("xblock_maker_t::clear_block block_cache_change clear, block=%s", block->dump().c_str());
            m_latest_blocks.erase(iter);
        }
    }
}

xblock_ptr_t xblock_maker_t::get_latest_block(uint64_t height) const {
    auto iter = m_latest_blocks.find(height);
    if (iter == m_latest_blocks.end()) {
        return nullptr;
    }
    return iter->second;
}

const xblock_ptr_t & xblock_maker_t::get_proposal_prev_block() const {
    xassert(!m_latest_blocks.empty());
    return m_latest_blocks.rbegin()->second;
}

const xblock_ptr_t & xblock_maker_t::get_proposal_prev_prev_block() const {
    xassert(m_latest_blocks.size() >= 2);
    xassert(m_latest_blocks.rbegin()->first == ((++m_latest_blocks.rbegin())->first + 1));
    auto iter = m_latest_blocks.rbegin();
    iter++;
    xassert(iter != m_latest_blocks.rend());
    return iter->second;
}

uint32_t xblock_maker_t::get_latest_consecutive_empty_block_num() const {
    uint32_t num = 0;
    for (auto iter = m_latest_blocks.rbegin(); iter != m_latest_blocks.rend(); iter++) {
        if (iter->second->get_block_class() == base::enum_xvblock_class_nil) {
            num++;
        } else {
            break;
        }
    }
    xassert(num <= 2);
    return num;
}

xblock_ptr_t xblock_maker_t::get_lock_block() const {
    xassert(m_latest_blocks.size() > 0);
    if (m_latest_blocks.size() == 1) {
        return m_latest_blocks.begin()->second;
    } else {
        return (++m_latest_blocks.rbegin())->second;
    }
}

std::string xblock_maker_t::get_lock_block_sign_hash() const {
    xblock_ptr_t lock_block = get_lock_block();
    return lock_block->get_cert()->get_hash_to_sign();

    // if (m_latest_blocks.size() > 1) {
    //     auto & lock_block = get_proposal_prev_prev_block();
    //     return lock_block->get_cert()->get_hash_to_sign();
    // } else {
    //     xassert(get_proposal_prev_block()->get_height() == 0);
    //     return get_proposal_prev_block()->get_cert()->get_hash_to_sign();
    // }
}

std::string xblock_maker_t::get_lock_output_root_hash() const {
    xblock_ptr_t lock_block = get_lock_block();
    return lock_block->get_cert()->get_output_root_hash();
    // if (m_latest_blocks.size() > 1) {
    //     auto & lock_block = get_proposal_prev_prev_block();
    //     return lock_block->get_cert()->get_output_root_hash();
    // } else {
    //     xassert(get_proposal_prev_block()->get_height() == 0);
    //     return get_proposal_prev_block()->get_cert()->get_output_root_hash();
    // }
}

bool xblock_maker_t::has_uncommitted_blocks() const {
    xassert(get_proposal_prev_block()->get_height() >= get_latest_committed_block()->get_height());
    return get_proposal_prev_block()->get_height() > get_latest_committed_block()->get_height();
}

NS_END2
