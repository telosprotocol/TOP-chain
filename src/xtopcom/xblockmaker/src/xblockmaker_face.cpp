// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <string>
#include "xdata/xblock.h"
#include "xdata/xblocktool.h"
#include "xdata/xcons_transaction.h"
#include "xblockmaker/xblockmaker_face.h"
#include "xmetrics/xmetrics.h"

NS_BEG2(top, blockmaker)
xblock_maker_t::xblock_maker_t(const std::string & account, const xblockmaker_resources_ptr_t & resources, uint32_t latest_blocks_max)
    : base::xvaccount_t(account), m_resources(resources), m_keep_latest_blocks_max(latest_blocks_max) {
    XMETRICS_GAUGE(metrics::dataobject_xblock_maker_t, 1);
}
xblock_maker_t::~xblock_maker_t() {
    XMETRICS_GAUGE(metrics::dataobject_xblock_maker_t, -1);
}
// create the state matching latest block and cache it
bool xblock_maker_t::update_account_state(const xblock_ptr_t & latest_block, uint64_t & lacked_block_height) {
    if (m_latest_bstate != nullptr
        && m_latest_bstate->get_block_viewid() == latest_block->get_viewid()
        && m_latest_bstate->get_block_height() == latest_block->get_height()) {
        xdbg("xblock_maker_t::update_account_state find cache state. account=%s,height=%ld",
            get_account().c_str(), latest_block->get_height());
        return true;
    }

    base::xauto_ptr<base::xvbstate_t> bstate = base::xvchain_t::instance().get_xstatestore()->get_blkstate_store()->get_block_state(latest_block.get(),metrics::statestore_access_from_blkmaker_update_account_state);
    if (bstate == nullptr) {
        lacked_block_height = latest_block->get_height() - 1;
        xwarn("xblock_maker_t::update_account_state fail-get target state. account=%s,height=%ld,viewid=%ld",
            get_account().c_str(), latest_block->get_height(), latest_block->get_viewid());
        return false;
    }
    xaccount_ptr_t target_state = std::make_shared<xunit_bstate_t>(bstate.get());
    m_latest_bstate = target_state;
    xassert(m_latest_bstate->get_block_height() == latest_block->get_height() && m_latest_bstate->get_block_viewid() == latest_block->get_viewid());
    xdbg("xblock_maker_t::update_account_state succ cache new state. account=%s,height=%ld,viewid=%ld",
        get_account().c_str(), latest_block->get_height(), latest_block->get_viewid());
    return true;
}

bool xblock_maker_t::is_latest_blocks_valid(const base::xblock_mptrs & latest_blocks) {
    return verify_latest_blocks(latest_blocks.get_latest_cert_block(), latest_blocks.get_latest_locked_block(), latest_blocks.get_latest_committed_block());
}

bool xblock_maker_t::verify_latest_blocks(base::xvblock_t* latest_cert_block, base::xvblock_t* lock_block, base::xvblock_t* commited_block) {
    // TODO(jimmy) table chain should always has different height of commit/lock/highqc, but unit chain may has same height of commit/lock/highqc
    // because blockstore will always unpack tableblock in committed status
    if (base::enum_vaccount_addr_type_block_contract == get_addr_type()) {
        if (!xblocktool_t::verify_latest_blocks(latest_cert_block, lock_block, commited_block)) {
            xwarn("xblock_maker_t::verify_latest_blocks,fail-table latests check. latest_cert_block=%s", latest_cert_block->dump().c_str());
            return false;
        }
    } else {
        if (latest_cert_block->get_height() != lock_block->get_height()
            || latest_cert_block->get_height() != commited_block->get_height()) {
            if (!xblocktool_t::verify_latest_blocks(latest_cert_block, lock_block, commited_block)) {
                xwarn("xblock_maker_t::verify_latest_blocks,fail-unit latests check. latest_cert_block=%s", latest_cert_block->dump().c_str());
                return false;
            }
        } else {
            if (!data::xblocktool_t::is_connect_and_executed_block(commited_block)) {
                xwarn("xblock_maker_t::verify_latest_blocks,fail-committed not executed. commit_block=%s",
                    commited_block->dump().c_str());
                return false;
            }
        }
    }
    return true;
}

void xblock_maker_t::clear_old_blocks() {
    if (m_latest_blocks.empty()) {
        return;
    }
    uint64_t highest_height = m_latest_blocks.rbegin()->first;
    for (auto iter = m_latest_blocks.begin(); iter != m_latest_blocks.end();) {
        if (iter->first + m_keep_latest_blocks_max <= highest_height) {
            iter = m_latest_blocks.erase(iter);
        } else {
            break;
        }
    }
}

void xblock_maker_t::set_latest_block(const xblock_ptr_t & block) {
    // always replace input block
    m_latest_blocks[block->get_height()] = block;
}

void xblock_maker_t::reset_latest_cert_block(const xblock_ptr_t & block) {
    // reset new latest cert block and delete any higher block
    uint64_t highest_height = block->get_height();
    for (auto iter = m_latest_blocks.begin(); iter != m_latest_blocks.end();) {
        if (iter->first > highest_height) {
            iter = m_latest_blocks.erase(iter);
        } else {
            iter++;
        }
    }
    set_latest_block(block);
}


bool xblock_maker_t::load_and_cache_enough_blocks(const xblock_ptr_t & latest_block, uint64_t & lacked_block_height) {
    XMETRICS_TIME_RECORD("cons_tableblock_verfiy_proposal_load_and_cache_enough_blocks");
    xblock_ptr_t current_block = latest_block;
    reset_latest_cert_block(latest_block);
    uint32_t count = 1;
    while (current_block->get_height() > 0) {
        if (count >= m_keep_latest_blocks_max) {
            break;
        }
        xblock_ptr_t prev_block = get_prev_block_from_cache(current_block);
        if (prev_block == nullptr) {
            // only mini-block is enough
            auto _block = get_blockstore()->load_block_object(*this, current_block->get_height() - 1, current_block->get_last_block_hash(), false, metrics::blockstore_access_from_blk_mk_ld_and_cache);
            if (_block == nullptr) {
                xwarn("xblock_maker_t::load_and_cache_enough_blocks fail-load block.account=%s,height=%ld", get_account().c_str(), current_block->get_height() - 1);
                lacked_block_height = current_block->get_height() - 1;
                return false;
            }
            prev_block = xblock_t::raw_vblock_to_object_ptr(_block.get());
            set_latest_block(prev_block);
        }
        current_block = prev_block;
        count++;
    }
    clear_old_blocks();
    xdbg("xblock_maker_t::load_and_cache_enough_blocks succ.account=%s,blocks_size=%d,highest=%ld",
        get_account().c_str(), m_latest_blocks.size(), get_highest_height_block()->get_height());
    return true;
}

xblock_ptr_t xblock_maker_t::get_prev_block_from_cache(const xblock_ptr_t & current) const {
    if (current->is_genesis_block()) {
        return current;
    }

    auto iter = m_latest_blocks.find(current->get_height() - 1);
    if (iter != m_latest_blocks.end() && current->get_last_block_hash() == iter->second->get_block_hash()) {
        return iter->second;
    }
    return nullptr;
}

const xblock_ptr_t & xblock_maker_t::get_highest_height_block() const {
    xassert(!m_latest_blocks.empty());
    return m_latest_blocks.rbegin()->second;
}

bool xblock_maker_t::check_latest_blocks(const xblock_ptr_t & latest_block) const {
    uint32_t count = m_latest_blocks.size();
    if (count == 0) {
        xassert(0);
        return false;
    }

    if (get_highest_height_block()->get_block_hash() != latest_block->get_block_hash()
        || get_highest_height_block()->get_height() != latest_block->get_height()) {
        xerror("xblock_maker_t::check_latest_blocks fail-check latest cert block.latest_block=%s,highest=%s",
            latest_block->dump().c_str(), get_highest_height_block()->dump().c_str());
        return false;
    }

    xblock_ptr_t highest_block = get_highest_height_block();
    if (m_latest_blocks.size() == 1) {
        if (highest_block->get_height() == 0) {
            return true;
        }
        xassert(0);
        return false;
    }

    if (m_latest_blocks.size() > m_keep_latest_blocks_max) {
        xassert(0);
        return false;
    }

    // must be connection
    xblock_ptr_t current_block = get_highest_height_block();
    for (auto iter = m_latest_blocks.rbegin(); iter != m_latest_blocks.rend(); iter++) {
        if (iter != m_latest_blocks.rbegin()) {
            auto & prev_block = iter->second;
            if (current_block->get_last_block_hash() != prev_block->get_block_hash()) {
                xassert(0);
                return false;
            }
            current_block = prev_block;
        }
    }

    return true;
}

NS_END2
