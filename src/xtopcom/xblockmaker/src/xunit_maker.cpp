// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <string>
#include <cinttypes>
#include "xblockmaker/xunit_maker.h"
#include "xblockmaker/xblockmaker_error.h"
#include "xblockmaker/xblock_rules.h"
#include "xblockmaker/xunit_builder.h"
#include "xtxexecutor/xtransaction_executor.h"
#include "xdata/xblocktool.h"
#include "xconfig/xpredefined_configurations.h"
#include "xconfig/xconfig_register.h"
#include "xstore/xaccount_context.h"

NS_BEG2(top, blockmaker)

xunit_maker_t::xunit_maker_t(const std::string & account, const xblockmaker_resources_ptr_t & resources, const store::xindexstore_face_ptr_t & indexstore)
: xblock_maker_t(account, resources, m_keep_latest_blocks_max) {
    m_fullunit_contain_of_unit_num_para = XGET_ONCHAIN_GOVERNANCE_PARAMETER(fullunit_contain_of_unit_num);
    m_block_rules = std::make_shared<xblock_rules>(resources);

    m_fullunit_builder = std::make_shared<xfullunit_builder_t>();
    m_lightunit_builder = std::make_shared<xlightunit_builder_t>();
    m_emptyunit_builder = std::make_shared<xemptyunit_builder_t>();
    m_default_builder_para = std::make_shared<xblock_builder_para_face_t>(resources);
    m_indexstore = indexstore;
}

xblock_ptr_t xunit_maker_t::get_latest_block(const base::xaccount_index_t & account_index) {
    base::xblock_vector blocks = get_blockstore()->load_block_object(*this, account_index.get_latest_unit_height());
    for (auto & block : blocks.get_vector()) {
        if (account_index.get_latest_unit_height() == 0 || account_index.is_match_unit_hash(block->get_block_hash())) {
            return xblock_t::raw_vblock_to_object_ptr(block);
        }
    }
    if (!blocks.get_vector().empty()) {
        xerror("xunit_maker_t::get_latest_block fail find match block. account=%s,height=%ld,block_size=%zu", get_account().c_str(), account_index.get_latest_unit_height(), blocks.get_vector().size());
    }
    return nullptr;
}

int32_t    xunit_maker_t::check_latest_state(const base::xaccount_index_t & account_index) {
    if (m_check_state_success && m_latest_account_index == account_index) {
        return xsuccess;
    }

    if (account_index.get_latest_unit_height() < m_latest_account_index.get_latest_unit_height()) {
        xwarn("xunit_maker_t::check_latest_state fail-account index behind, account=%s,cache_height=%ld,index_height=%ld",
            get_account().c_str(), m_latest_account_index.get_latest_unit_height(), account_index.get_latest_unit_height());
        return xblockmaker_error_latest_unit_blocks_invalid;
    }

    // find the latest cert block which matching account_index
    xblock_ptr_t latest_block = get_latest_block(account_index);
    if (nullptr == latest_block) {
        // TODO(jimmy) invoke sync
        xwarn("xunit_maker_t::check_latest_state fail-is_latest_blocks_valid, account=%s", get_account().c_str());
        return xblockmaker_error_latest_unit_blocks_invalid;
    }

    // reinit unit maker
    m_check_state_success = false;
    // cache latest block
    if (!load_and_cache_enough_blocks(latest_block)) {
        xwarn("xunit_maker_t::check_latest_state fail-load_and_cache_enough_blocks.account=%s", get_account().c_str());
        return xblockmaker_error_latest_unit_blocks_invalid;
    }

    xblock_ptr_t latest_committed_block = get_highest_commit_block();
    if (!update_account_state(latest_committed_block)) {
        xwarn("xunit_maker_t::check_latest_state fail-update_account_state.latest_committed_block=%s",
            latest_committed_block->dump().c_str());
        return xblockmaker_error_latest_unit_blocks_invalid;
    }
    set_latest_committed_block(latest_committed_block);

    if (false == check_latest_blocks()) {
        xerror("xunit_maker_t::check_latest_state fail-check_latest_blocks.latest_committed_block=%s",
            latest_committed_block->dump().c_str());
        return xblockmaker_error_latest_unit_blocks_invalid;
    }
    m_latest_account_index = account_index;
    m_check_state_success = true;
    return xsuccess;
}

void xunit_maker_t::find_highest_send_tx(uint64_t & latest_nonce, uint256_t & latest_hash) {
    for (auto iter = m_pending_txs.rbegin(); iter != m_pending_txs.rend(); iter++) {
        auto & tx = *iter;
        if (tx->is_send_tx() || tx->is_self_tx()) {
            latest_nonce = tx->get_transaction()->get_tx_nonce();
            latest_hash = tx->get_transaction()->digest();
            return;
        }
    }
    latest_nonce = get_latest_committed_state()->get_account_mstate().get_latest_send_trans_number();
    latest_hash = get_latest_committed_state()->get_account_mstate().get_latest_send_trans_hash();
}

bool xunit_maker_t::push_tx(const data::xblock_consensus_para_t & cs_para, const xcons_transaction_ptr_t & tx) {
    if (is_account_locked()) {
        xwarn("xunit_maker_t::push_tx fail-tx filtered for account locked.%s,tx=%s", cs_para.dump().c_str(), tx->dump().c_str());
        return false;
    }

    if (is_match_account_fullunit_limit()) {
        // send and self tx is filtered when matching fullunit limit
        if (tx->is_self_tx() || tx->is_send_tx()) {
            xwarn("xunit_maker_t::push_tx fail-tx filtered for fullunit limit.%s,tx=%s", cs_para.dump().c_str(), tx->dump().c_str());
            return false;
        }
    }

    // send tx contious nonce rules
    if (tx->is_send_tx() || tx->is_self_tx()) {
        uint64_t latest_nonce;
        uint256_t latest_hash;
        find_highest_send_tx(latest_nonce, latest_hash);
        if (tx->get_transaction()->get_last_nonce() != latest_nonce || !tx->get_transaction()->check_last_trans_hash(latest_hash)) {
            uint64_t latest_nonce = get_latest_committed_state()->get_account_mstate().get_latest_send_trans_number();
            uint256_t latest_hash = get_latest_committed_state()->get_account_mstate().get_latest_send_trans_hash();
            get_txpool()->updata_latest_nonce(get_account(), latest_nonce, latest_hash);
            xwarn("xunit_maker_t::push_tx fail-tx filtered for send nonce hash not match,%s,latest_nonce=%ld,tx=%s",
                cs_para.dump().c_str(), latest_nonce, tx->dump().c_str());
            return false;
        }
    }

    // TODO(jimmy) same subtype limit
    if (!m_pending_txs.empty()) {
        base::enum_transaction_subtype first_tx_subtype = m_pending_txs[0]->get_tx_subtype();
        if (first_tx_subtype == base::enum_transaction_subtype_self) {
            first_tx_subtype = base::enum_transaction_subtype_send;
        }
        base::enum_transaction_subtype new_tx_subtype = tx->get_tx_subtype();
        if (new_tx_subtype == base::enum_transaction_subtype_self) {
            new_tx_subtype = base::enum_transaction_subtype_send;
        }
        if (new_tx_subtype != first_tx_subtype) {
            xwarn("xunit_maker_t::push_tx fail-tx filtered for not same subtype.%s,tx=%s,tx_count=%zu,first_tx=%s",
                cs_para.dump().c_str(), tx->dump().c_str(), m_pending_txs.size(), m_pending_txs[0]->dump().c_str());
            return false;
        }
    }

    // TODO(jimmy) batch txs limit
    if (!m_pending_txs.empty()) {
        base::enum_transaction_subtype first_tx_subtype = m_pending_txs[0]->get_tx_subtype();
        data::enum_xtransaction_type first_tx_type = (data::enum_xtransaction_type)m_pending_txs[0]->get_transaction()->get_tx_type();
        if ( (first_tx_subtype != enum_transaction_subtype_confirm) &&
            (first_tx_type != xtransaction_type_transfer || (data::enum_xtransaction_type)tx->get_transaction()->get_tx_type() != data::xtransaction_type_transfer) ) {
            xwarn("xunit_maker_t::push_tx fail-tx filtered for batch txs.%s,tx=%s", cs_para.dump().c_str(), tx->dump().c_str());
            return false;
        }
    }

    for (auto & v : m_pending_txs) {
        if (tx->get_transaction()->digest() == v->get_transaction()->digest()) {
            xerror("xunit_maker_t::push_tx repeat tx.%s,tx=%s,pendingtx=%s", cs_para.dump().c_str(), tx->dump().c_str(), v->dump().c_str());
            return false;
        }
    }

    m_pending_txs.push_back(tx);
    xdbg("xunit_maker_t::push_tx succ.%s,total_size=%zu,tx=%s", cs_para.dump().c_str(), m_pending_txs.size(), tx->dump().c_str());
    return true;
}

void xunit_maker_t::clear_tx() {
    m_pending_txs.clear();
}

xblock_ptr_t xunit_maker_t::make_proposal(const xunitmaker_para_t & unit_para, const data::xblock_consensus_para_t & cs_para, xunitmaker_result_t & result) {
    xblock_ptr_t proposal_block = make_next_block(unit_para, cs_para, result);
    clear_tx();
    if (proposal_block == nullptr) {
        if (xblockmaker_error_no_need_make_unit != result.m_make_block_error_code) {
            xwarn("xunit_maker_t::make_proposal fail-make proposal.%s,account=%s,height=%" PRIu64 ",ret=%s",
                cs_para.dump().c_str(), get_account().c_str(), get_lowest_height_block()->get_height(), chainbase::xmodule_error_to_str(result.m_make_block_error_code).c_str());
        } else {
            xwarn("xunit_maker_t::make_proposal no need make proposal.%s,account=%s,height=%" PRIu64 "",
                cs_para.dump().c_str(), get_account().c_str(), get_lowest_height_block()->get_height());
        }
        return nullptr;
    }
    xinfo("xunit_maker_t::make_proposal succ unit.%s,unit=%s,cert=%s,class=%d,unconfirm=%d,prev_confirmed=%d,tx_count=%d,latest_state=%s",
        cs_para.dump().c_str(), proposal_block->dump().c_str(), proposal_block->dump_cert().c_str(), proposal_block->get_block_class(),
        proposal_block->get_unconfirm_sendtx_num(), proposal_block->is_prev_sendtx_confirmed(),
        result.m_success_txs.size(), dump().c_str());
    for (auto & tx : result.m_success_txs) {
        xinfo("xunit_maker_t::make_proposal succ tx.%s,unit=%s,tx=%s",
            cs_para.dump().c_str(), proposal_block->dump().c_str(), tx->dump().c_str());
    }

    return proposal_block;
}

xblock_ptr_t xunit_maker_t::make_next_block(const xunitmaker_para_t & unit_para, const data::xblock_consensus_para_t & cs_para, xunitmaker_result_t & result) {
    xblock_ptr_t proposal_unit = nullptr;

    // reset justify cert hash para
    std::string justify_cert_hash = get_lock_block_sign_hash();
    cs_para.set_justify_cert_hash(justify_cert_hash);
    m_default_builder_para->set_error_code(xsuccess);

    // firstly should process txs and try to make lightunit
    if (can_make_next_light_block()) {
        base::xreceiptid_state_ptr_t receiptid_state = unit_para.m_tablestate->get_receiptid_state();
        xblock_builder_para_ptr_t build_para = std::make_shared<xlightunit_builder_para_t>(m_pending_txs, receiptid_state, get_resources());
        proposal_unit = m_lightunit_builder->build_block(get_highest_height_block(),
                                                        clone_latest_committed_state(),
                                                        cs_para,
                                                        build_para);
        result.m_make_block_error_code = build_para->get_error_code();
        std::shared_ptr<xlightunit_builder_para_t> lightunit_build_para = std::dynamic_pointer_cast<xlightunit_builder_para_t>(build_para);
        result.m_success_txs = lightunit_build_para->get_origin_txs();
        result.m_fail_txs = lightunit_build_para->get_fail_txs();
        for (auto & tx : lightunit_build_para->get_fail_txs()) {
            xassert(tx->is_self_tx() || tx->is_send_tx());
            xwarn("xunit_maker_t::make_next_block fail-pop send tx. account=%s,tx=%s", get_account().c_str(), tx->dump().c_str());
            xtxpool_v2::tx_info_t txinfo(get_account(), tx->get_transaction()->digest(), tx->get_tx_subtype());
            get_txpool()->pop_tx(txinfo);
        }
    }

    // secondly try to make full unit
    if (nullptr == proposal_unit && can_make_next_full_block()) {
        proposal_unit = m_fullunit_builder->build_block(get_highest_height_block(),
                                                        get_latest_committed_state(),
                                                        cs_para,
                                                        m_default_builder_para);
        result.m_make_block_error_code = m_default_builder_para->get_error_code();
    }

    // thirdly try to make full unit
    if (nullptr == proposal_unit && can_make_next_empty_block()) {
        proposal_unit = m_emptyunit_builder->build_block(get_highest_height_block(),
                                                        nullptr,
                                                        cs_para,
                                                        m_default_builder_para);
        result.m_make_block_error_code = m_default_builder_para->get_error_code();
    }
    result.m_block = proposal_unit;
    return proposal_unit;
}

bool xunit_maker_t::can_make_next_block() const {
    if (can_make_next_light_block() || can_make_next_empty_block() || can_make_next_full_block()) {
        return true;
    }
    return false;
}

bool xunit_maker_t::can_make_next_empty_block() const {
    // TODO(jimmy)
    const xblock_ptr_t & current_block = get_highest_height_block();
    if (current_block->get_height() == 0) {
        return false;
    }
    if (current_block->get_block_class() == base::enum_xvblock_class_light) {
        return true;
    }
    xblock_ptr_t prev_block = get_prev_block(current_block);
    if (prev_block == nullptr) {
        return false;
    }
    if (prev_block->get_block_class() == base::enum_xvblock_class_light) {
        return true;
    }
    return false;
}

bool xunit_maker_t::is_account_locked() const {
    return can_make_next_empty_block();
}

bool xunit_maker_t::is_match_account_fullunit_limit() const {
    base::xvblock_t* prev_block = get_highest_height_block().get();
    uint64_t current_height = prev_block->get_height() + 1;
    uint64_t current_fullunit_height = prev_block->get_block_class() == base::enum_xvblock_class_full ? prev_block->get_height() : prev_block->get_last_full_block_height();
    uint64_t current_lightunit_count = current_height - current_fullunit_height;
    uint64_t max_limit_lightunit_count = XGET_ONCHAIN_GOVERNANCE_PARAMETER(fullunit_contain_of_unit_num) * 2; // TODO(jimmy)
    if (current_lightunit_count >= max_limit_lightunit_count) {
        return true;
    }
    return false;
}

bool xunit_maker_t::can_make_next_full_block() const {
    // TODO(jimmy) non contious block make mode. condition:non-empty block is committed status
    if (is_account_locked()) {
        return false;
    }

    base::xvblock_t* prev_block = get_highest_height_block().get();
    uint64_t current_height = prev_block->get_height() + 1;
    uint64_t current_fullunit_height = prev_block->get_block_class() == base::enum_xvblock_class_full ? prev_block->get_height() : prev_block->get_last_full_block_height();
    uint64_t current_lightunit_count = current_height - current_fullunit_height;
    xassert(current_lightunit_count > 0);
    uint64_t max_limit_lightunit_count = XGET_ONCHAIN_GOVERNANCE_PARAMETER(fullunit_contain_of_unit_num);
    if (current_lightunit_count >= max_limit_lightunit_count) {
        if (get_latest_committed_state()->get_unconfirm_sendtx_num() == 0) {
            return true;
        }
        if (current_lightunit_count >= max_limit_lightunit_count * 5) {  // TODO(jimmy)
            xwarn("xunit_maker_t::can_make_next_full_block too many lightunit.current_height=%ld,state_height=%ld,lightunit_count=%ld,unconfirm_sendtx_num=%d",
                current_height,get_latest_committed_state()->get_last_height(), current_lightunit_count, get_latest_committed_state()->get_unconfirm_sendtx_num());
        }
    }
    return false;
}

bool xunit_maker_t::can_make_next_light_block() const {
    if (m_pending_txs.empty()) {
        return false;
    }
    if (is_account_locked())  {
        return false;
    }
    return true;
}

std::string xunit_maker_t::dump() const {
    char local_param_buf[128];
    uint64_t highest_height = get_highest_height_block()->get_height();
    uint64_t lowest_height = get_lowest_height_block()->get_height();
    uint64_t state_height = get_latest_committed_state()->get_chain_height();
    uint64_t commit_height = get_latest_committed_block()->get_height();

    xprintf(local_param_buf,sizeof(local_param_buf),
        "{highest=%" PRIu64 ",lowest=%" PRIu64 ",commit=%" PRIu64 ",state=%" PRIu64 ",nonce=%" PRIu64 ",parent=%" PRIu64 "}",
        highest_height, lowest_height, commit_height, state_height,
        get_latest_committed_state()->get_account_mstate().get_latest_send_trans_number(),
        get_highest_height_block()->get_cert()->get_parent_block_height());
    return std::string(local_param_buf);
}

NS_END2
