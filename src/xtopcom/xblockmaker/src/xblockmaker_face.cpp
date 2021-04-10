// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <string>
#include "xdata/xblock.h"
#include "xdata/xblocktool.h"
#include "xdata/xcons_transaction.h"
#include "xblockmaker/xblockmaker_face.h"

NS_BEG2(top, blockmaker)

bool xblock_maker_t::update_account_state(const xblock_ptr_t & latest_committed_block) {
    if (m_commit_account != nullptr && m_commit_account->get_last_height() == latest_committed_block->get_height()) {
        return true;
    }

    m_commit_account = get_store()->query_account(get_account());
    if (m_commit_account == nullptr) {
        m_commit_account = make_object_ptr<xblockchain2_t>(get_account());
    }
    if (m_commit_account->get_last_height() != latest_committed_block->get_height()) {
        xwarn("xblock_maker_t::update_account_state fail-load commmit account. account=%s,actual_account_height=%ld,demand_account_height=%ld",
            get_account().c_str(), m_commit_account->get_last_height(), latest_committed_block->get_height());
        return false;
    }
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

void xblock_maker_t::set_latest_committed_block(const xblock_ptr_t & latest_committed_block) {
    xassert(latest_committed_block->check_block_flag(base::enum_xvblock_flag_committed));
    if (m_latest_commit_block == nullptr || m_latest_commit_block->get_height() != latest_committed_block->get_height()) {
        m_latest_commit_block = latest_committed_block;
    }
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
        xdbg("xblock_maker_t::set_latest_block already exist, block=%s", block->dump().c_str());
        return block;
    }

    if (m_latest_blocks.empty()) {
        m_latest_blocks[block->get_height()] = block;
        xdbg("xblock_maker_t::set_latest_block block_cache_change add new block=%s", block->dump().c_str());
        return block;
    }

    if (block->get_height() + m_keep_latest_blocks_max <= m_latest_blocks.rbegin()->first) {
        xdbg("xblock_maker_t::set_latest_block too old block,block=%s", block->dump().c_str());
        return block;
    }
    m_latest_blocks[block->get_height()] = block;

    uint64_t highest_height = m_latest_blocks.rbegin()->first;
    for (auto iter = m_latest_blocks.begin(); iter != m_latest_blocks.end();) {
        if (iter->first + m_keep_latest_blocks_max <= highest_height) {
            xdbg("xblock_maker_t::set_latest_block block_cache_change delete old block=%s", iter->second->dump().c_str());
            iter = m_latest_blocks.erase(iter);
        } else {
            break;
        }
    }
    return block;
}

bool xblock_maker_t::load_latest_blocks(const xblock_ptr_t & latest_block, std::map<uint64_t, xblock_ptr_t> & latest_blocks) {
    xassert(latest_blocks.empty());
    latest_blocks[latest_block->get_height()] = latest_block;
    xdbg("xblock_maker_t::load_latest_blocks start.account=%s,latest_height=%ld", get_account().c_str(), latest_block->get_height());
    xblock_ptr_t current_block = latest_block;
    uint32_t count = 1;
    while (current_block->get_height() > 0) {
        uint64_t prev_height = current_block->get_height() - 1;
        xblock_ptr_t prev_block = get_latest_block(prev_height);
        if (prev_block != nullptr && prev_block->get_block_hash() == current_block->get_last_block_hash()) {
            xdbg("xblock_maker_t::load_latest_blocks finish prev block in cache.account=%s,latest_height=%ld", get_account().c_str(), latest_block->get_height());
            return true;
        }
        auto _block = get_blockstore()->load_block_object(*this, prev_height, 0, true);
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
        if (count >= m_keep_latest_blocks_max) {
            xdbg("xtable_maker_t::load_latest_blocks finish arrive max times.account=%s,latest_height=%ld", get_account().c_str(), latest_block->get_height());
            return true;
        }
    }
    xdbg("xblock_maker_t::load_latest_blocks finish reach genesis.account=%s,latest_height=%ld,current_height=%ld",
        get_account().c_str(), latest_block->get_height(), current_block->get_height());
    return true;
}

bool xblock_maker_t::load_and_cache_enough_blocks() {
    xassert(m_latest_blocks.size() > 0);

    xblock_ptr_t current_block = get_highest_height_block();
    uint32_t count = 1;
    while (current_block->get_height() > 0) {
        uint64_t prev_height = current_block->get_height() - 1;
        xblock_ptr_t prev_block = get_latest_block(prev_height);
        if (prev_block == nullptr) {
            auto _block = get_blockstore()->load_block_object(*this, prev_height, 0, true);
            if (_block == nullptr) {
                xerror("xblock_maker_t::load_and_cache_enough_blocks fail-load block.account=%s,height=%ld", get_account().c_str(), prev_height);
                return false;
            }
            if (_block->get_block_hash() != current_block->get_last_block_hash()) {
                xerror("xblock_maker_t::load_and_cache_enough_blocks fail- not match prev.prev=%s,current=%s", _block->dump().c_str(), current_block->dump().c_str());
                return false;
            }
            prev_block = xblock_t::raw_vblock_to_object_ptr(_block.get());
            set_latest_block(prev_block);
        }
        count++;
        if (count >= m_keep_latest_blocks_max) {
            break;
        }
        current_block = prev_block;
    }
    xdbg("xblock_maker_t::load_and_cache_enough_blocks succ.account=%s,blocks_size=%d,highest=%ld,lowest=%ld",
        get_account().c_str(), m_latest_blocks.size(), get_highest_height_block()->get_height(), get_lowest_height_block()->get_height());
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

xblock_ptr_t xblock_maker_t::get_prev_block(const xblock_ptr_t & current) const {
    xassert(current->get_height() > 0);
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

const xblock_ptr_t & xblock_maker_t::get_lowest_height_block() const {
    xassert(!m_latest_blocks.empty());
    return m_latest_blocks.begin()->second;
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

bool xblock_maker_t::check_latest_blocks() const {
    uint32_t count = m_latest_blocks.size();
    if (count == 0) {
        xassert(0);
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

    if (get_lowest_height_block()->get_height() > get_latest_committed_block()->get_height()) {
        xassert(0);
        return false;
    }

    uint64_t distance_height = get_highest_height_block()->get_height() - get_lowest_height_block()->get_height() + 1;
    if (distance_height != m_latest_blocks.size()) {
        xassert(0);
        return false;
    }

    if (distance_height < m_latest_blocks.size()) {
        if (get_highest_height_block()->get_height() != distance_height - 1) {
            xassert(0);
            return false;
        }

        if (get_highest_height_block()->get_height() < 2) {
            xassert(0);
            return false;
        }
    }

    return true;
}

xblock_ptr_t xblock_maker_t::get_highest_lock_block() const {
    uint32_t count = 0;
    for (auto iter = m_latest_blocks.rbegin(); iter != m_latest_blocks.rend(); iter++) {
        count++;
        if (iter->first == 0 || count == 2) {
            return iter->second;
        }
    }
    xassert(false);
    return nullptr;
}

xblock_ptr_t xblock_maker_t::get_highest_commit_block() const {
    uint32_t count = 0;
    for (auto iter = m_latest_blocks.rbegin(); iter != m_latest_blocks.rend(); iter++) {
        count++;
        if (iter->first == 0 || count == 3) {
            return iter->second;
        }
    }
    xassert(false);
    return nullptr;
}

std::string xblock_maker_t::get_lock_block_sign_hash() const {
    xblock_ptr_t lock_block = get_highest_lock_block();
    return lock_block->get_cert()->get_hash_to_sign();
}

std::string xblock_maker_t::get_lock_output_root_hash() const {
    xblock_ptr_t lock_block = get_highest_lock_block();
    return lock_block->get_cert()->get_output_root_hash();
}

xblock_ptr_t xblock_maker_t::get_highest_non_empty_block() const {
    for (auto iter = m_latest_blocks.rbegin(); iter != m_latest_blocks.rend(); iter++) {
        if (iter->second->get_block_class() != base::enum_xvblock_class_nil) {
            return iter->second;
        }
    }
    return nullptr;
}

bool xblock_maker_t::has_uncommitted_blocks() const {
    xassert(get_highest_height_block()->get_height() >= get_latest_committed_block()->get_height());
    return get_highest_height_block()->get_height() > get_latest_committed_block()->get_height();
}

xaccount_ptr_t xblock_maker_t::clone_latest_committed_state() const {
    std::string stream_str;
    get_latest_committed_state()->serialize_to_string(stream_str);
    xaccount_ptr_t blockchain = make_object_ptr<xblockchain2_t>(get_account());
    blockchain->serialize_from_string(stream_str);
    return blockchain;
}

xblock_ptr_t        xblock_builder_face_t::build_empty_block(const xblock_ptr_t & prev_block,
                                                    const data::xblock_consensus_para_t & cs_para) {
    base::xvblock_t* _proposal_block = data::xemptyblock_t::create_next_emptyblock(prev_block.get());
    xblock_ptr_t proposal_unit;
    proposal_unit.attach((data::xblock_t*)_proposal_block);
    proposal_unit->set_consensus_para(cs_para);
    return proposal_unit;
}


xblock_ptr_t    xblock_builder_face_t::build_genesis_block(const std::string & account, int64_t top_balance) {
    base::enum_vaccount_addr_type addrtype = base::xvaccount_t::get_addrtype_from_account(account);
    base::xvblock_t* _proposal_block = nullptr;
    if (addrtype == base::enum_vaccount_addr_type_block_contract) {
        _proposal_block = xemptyblock_t::create_genesis_emptyblock(account, base::enum_xvblock_level_table);
    } else if (addrtype == base::enum_vaccount_addr_type_timer || addrtype == base::enum_vaccount_addr_type_drand) {
        _proposal_block = xemptyblock_t::create_genesis_emptyblock(account, base::enum_xvblock_level_root);
    } else if (addrtype == base::enum_vaccount_addr_type_secp256k1_user_account
                || addrtype == base::enum_vaccount_addr_type_secp256k1_user_sub_account
                || addrtype == base::enum_vaccount_addr_type_native_contract
                || addrtype == base::enum_vaccount_addr_type_custom_contract) {
        _proposal_block = build_genesis_unit(account, top_balance);
    } else {
        xassert(false);
        return nullptr;
    }
    xblock_ptr_t _block;
    _block.attach((data::xblock_t*)_proposal_block);
    xassert(_block->get_refcount() == 1);
    return _block;
}

base::xvblock_t*    xblock_builder_face_t::build_genesis_unit(const std::string & account, int64_t top_balance) {
    base::xvblock_t* _proposal_block;
    if (top_balance == 0) {
        _proposal_block = xemptyblock_t::create_genesis_emptyblock(account, base::enum_xvblock_level_unit);
    } else {
        xtransaction_ptr_t tx = make_object_ptr<xtransaction_t>();
        data::xproperty_asset asset(top_balance);
        tx->make_tx_transfer(asset);
        // genesis transfer tx is a special transaction
        tx->set_same_source_target_address(account);
        tx->set_fire_timestamp(0);
        tx->set_expire_duration(0);
        tx->set_deposit(0);
        tx->set_digest();
        tx->set_len();
        xtransaction_result_t result;
        result.m_balance_change = top_balance;
        _proposal_block = xlightunit_block_t::create_genesis_lightunit(account, tx, result);
    }
    return _proposal_block;
}

NS_END2
