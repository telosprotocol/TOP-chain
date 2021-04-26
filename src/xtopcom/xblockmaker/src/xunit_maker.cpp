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

void xunit_maker_t::init_unit_blocks(const base::xblock_mptrs & latest_blocks) {
    // only cache unit with matching account index
    xblock_ptr_t cert_block = xblock_t::raw_vblock_to_object_ptr(latest_blocks.get_latest_cert_block());
    set_latest_block(cert_block);
    if (latest_blocks.get_latest_locked_block()->get_block_hash() != get_highest_height_block()->get_block_hash()) {
        xblock_ptr_t lock_block = xblock_t::raw_vblock_to_object_ptr(latest_blocks.get_latest_locked_block());
        set_latest_block(lock_block);
    }
    if (latest_blocks.get_latest_committed_block()->get_block_hash() != get_highest_height_block()->get_block_hash()) {
        xblock_ptr_t commit_block = xblock_t::raw_vblock_to_object_ptr(latest_blocks.get_latest_committed_block());
        set_latest_block(commit_block);
    }
}

int32_t    xunit_maker_t::check_latest_state(const xblock_ptr_t & committed_table_block) {
    // TODO(jimmy) only empty unit maker should init latest blocks
    base::xblock_mptrs latest_blocks = get_blockstore()->get_latest_blocks(*this);
    if (get_latest_blocks().empty()) {
        if (!is_latest_blocks_valid(latest_blocks)) {
            xwarn("xunit_maker_t::check_latest_state fail-is_latest_blocks_valid, account=%s", get_account().c_str());
            return xblockmaker_error_latest_unit_blocks_invalid;
        }

        if (committed_table_block != nullptr) {
            if (false == check_index_state(committed_table_block, latest_blocks)) {
                xwarn("xunit_maker_t::check_latest_state fail-check_index_state.account=%s", get_account().c_str());
                return xblockmaker_error_latest_unit_blocks_invalid;
            }
        } else {
            init_unit_blocks(latest_blocks);
        }
    }

    if (!load_and_cache_enough_blocks()) {
        xwarn("xunit_maker_t::check_latest_state fail-load_and_cache_enough_blocks.account=%s", get_account().c_str());
        return xblockmaker_error_latest_unit_blocks_invalid;
    }

    xblock_ptr_t latest_committed_block = xblock_t::raw_vblock_to_object_ptr(latest_blocks.get_latest_committed_block());
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
    return xsuccess;
}

bool    xunit_maker_t::check_index_state(const xblock_ptr_t & committed_table_block, const base::xblock_mptrs & latest_blocks) {
    base::xaccount_index_t account_index;
    bool ret = m_indexstore->get_account_index(committed_table_block, get_account(), account_index);
    if (!ret) {
        xwarn("xunit_maker_t::check_index_state fail-get_account_index.account=%s,table_commit_height=%ld",
            get_account().c_str(), committed_table_block->get_height());
        return false;
    }
    // only cache unit with matching account index
    xblock_ptr_t cert_block = xblock_t::raw_vblock_to_object_ptr(latest_blocks.get_latest_cert_block());
    xblock_ptr_t lock_block = xblock_t::raw_vblock_to_object_ptr(latest_blocks.get_latest_locked_block());
    xblock_ptr_t commit_block = xblock_t::raw_vblock_to_object_ptr(latest_blocks.get_latest_committed_block());
    if (cert_block->get_height() == account_index.get_latest_unit_height()
        && (account_index.get_latest_unit_height() == 0 || account_index.is_match_unit_hash(cert_block->get_block_hash())) ) {
        set_latest_block(cert_block);
        set_latest_block(lock_block);
        set_latest_block(commit_block);
        return true;
    }

    if (lock_block->get_height() == account_index.get_latest_unit_height()
        && (account_index.get_latest_unit_height() == 0 || account_index.is_match_unit_hash(cert_block->get_block_hash())) ) {
        set_latest_block(lock_block);
        set_latest_block(commit_block);
        return true;
    }

    if (commit_block->get_height() == account_index.get_latest_unit_height()
        && (account_index.get_latest_unit_height() == 0 || account_index.is_match_unit_hash(cert_block->get_block_hash())) ) {
        set_latest_block(commit_block);
        return true;
    }

    // TODO(jimmy) get unit from db should according to account index, then it will always match with table
    xwarn("xunit_maker_t::check_index_state fail-unit index unmatch.account=%s,index_height=%ld,actual_height=%ld,parent=%ld,current=%ld",
        get_account().c_str(), account_index.get_latest_unit_height(), cert_block->get_height(), cert_block->get_cert()->get_parent_block_height(), committed_table_block->get_height());
    return false;
}

bool xunit_maker_t::unit_rules_filter(const std::vector<xcons_transaction_ptr_t> & origin_txs,
                                            std::vector<xcons_transaction_ptr_t> & valid_txs,
                                            std::vector<xcons_transaction_ptr_t> & pop_txs) {
    xassert(!origin_txs.empty());
    xassert(valid_txs.empty());
    xassert(pop_txs.empty());

    m_block_rules->unit_rules_filter(get_latest_committed_block(), get_latest_committed_state(), origin_txs, valid_txs, pop_txs);
    return true;
}

bool xunit_maker_t::leader_push_txs(const std::vector<xcons_transaction_ptr_t> & txs) {
    if (is_account_locked()) {
        // TODO(jimmy)
        xerror("xunit_maker_t::leader_push_txs fail-account locked.account=%s,hightest_height=%ld",
            get_account().c_str(), get_highest_height_block()->get_height());
        return false;
    }

    uint64_t latest_nonce = get_latest_committed_state()->get_account_mstate().get_latest_send_trans_number();
    uint256_t latest_hash = get_latest_committed_state()->get_account_mstate().get_latest_send_trans_hash();
    get_txpool()->updata_latest_nonce(get_account(), latest_nonce, latest_hash);

    std::vector<xcons_transaction_ptr_t> valid_txs;
    std::vector<xcons_transaction_ptr_t> pop_txs;
    if (false == unit_rules_filter(txs, valid_txs, pop_txs)) {
        xerror("xunit_maker_t::leader_push_txs fail-unit_rules_filter.account=%s", get_account().c_str());
        return false;
    }

    for (auto & tx : pop_txs) {
        xinfo("xunit_maker_t::leader_push_txs pop tx. account=%s,tx=%s", get_account().c_str(), tx->dump().c_str());
        xtxpool_v2::tx_info_t txinfo(get_account(), tx->get_transaction()->digest(), tx->get_tx_subtype());
        get_txpool()->pop_tx(txinfo);
    }

    if (valid_txs.empty()) {
        xwarn("xunit_maker_t::leader_push_txs final fail. account=%s,origin_tx_count=%d,pop_tx_count=%d",
            get_account().c_str(), txs.size(), pop_txs.size());
        return false;
    }

    m_pending_txs = valid_txs;
    xdbg("xunit_maker_t::leader_push_txs final succ. account=%s,latest_nonce:%llu,origin_tx_count=%d,valid_tx_count=%d,pop_tx_count=%d,latest_committed_block:%s",
        get_account().c_str(), latest_nonce, txs.size(), valid_txs.size(), pop_txs.size(), get_latest_committed_block()->dump().c_str());
    return true;
}

bool xunit_maker_t::backup_push_txs(const std::vector<xcons_transaction_ptr_t> & txs) {
    xassert(!txs.empty());
    if (is_account_locked()) {
        // TODO(jimmy)
        xwarn("xunit_maker_t::backup_push_txs fail-account locked.account=%s,hightest_height=ld",
            get_account().c_str(), get_highest_height_block()->get_height());
        return false;
    }

    get_txpool()->on_block_confirmed(get_latest_committed_block().get());
    uint64_t latest_nonce = get_latest_committed_state()->get_account_mstate().get_latest_send_trans_number();
    uint256_t latest_hash = get_latest_committed_state()->get_account_mstate().get_latest_send_trans_hash();
    get_txpool()->updata_latest_nonce(get_account(), latest_nonce, latest_hash);

    std::vector<xcons_transaction_ptr_t> valid_txs;
    std::vector<xcons_transaction_ptr_t> pop_txs;
    if (false == unit_rules_filter(txs, valid_txs, pop_txs) || valid_txs.size() != txs.size()) {
        xwarn("xunit_maker_t::backup_push_txs fail-unit_rules_filter.account=%s,origin_tx_count=%d,valid_tx_count=%d,pop_tx_count=%d",
            get_account().c_str(), txs.size(), valid_txs.size(), pop_txs.size());
        return false;
    }
    m_pending_txs = txs;
    xdbg("xunit_maker_t::backup_push_txs succ.account=%s,tx_count=%d", get_account().c_str(), txs.size());
    return true;
}

void xunit_maker_t::clear_tx() {
    m_pending_txs.clear();
}

xblock_ptr_t xunit_maker_t::make_proposal(const xunit_proposal_input_t & proposal_input, const data::xblock_consensus_para_t & cs_para, xunitmaker_result_t & result) {
    if (!proposal_input.get_input_txs().empty()) {
        leader_push_txs(proposal_input.get_input_txs());
    }

    xblock_ptr_t proposal_block = make_next_block(cs_para, result);
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

xblock_ptr_t xunit_maker_t::verify_proposal(const xunit_proposal_input_t & proposal_input, const data::xblock_consensus_para_t & cs_para, xunitmaker_result_t & result) {
    xdbg("xunit_maker_t::verify_proposal start. %s,account=%s,latest_state=%s",
        cs_para.dump().c_str(), get_account().c_str(), dump().c_str());

    auto & highest_block = get_highest_height_block();
    if (proposal_input.get_last_block_height() != highest_block->get_height()
        || proposal_input.get_last_block_hash() != highest_block->get_block_hash()) {
        result.m_make_block_error_code = xblockmaker_error_proposal_unit_not_match_prev_block;
        xwarn("xunit_maker_t::verify_proposal fail-check_last_height_hash.%s,account=%s,proposal_height=%" PRIu64 ",highest_height=%" PRIu64 "",
            cs_para.dump().c_str(), get_account().c_str(), proposal_input.get_last_block_height(), highest_block->get_height());
        return nullptr;
    }

    if (!proposal_input.get_input_txs().empty()) {
        if (false == backup_push_txs(proposal_input.get_input_txs())) {
            result.m_make_block_error_code = xblockmaker_error_proposal_bad_input;
            xwarn("xunit_maker_t::verify_proposal fail-push txs.%s,account=%s,proposal_height=%" PRIu64 ",txs_count=%d",
                cs_para.dump().c_str(), get_account().c_str(), proposal_input.get_last_block_height(), proposal_input.get_input_txs().size());
            return nullptr;
        }
    }

    xblock_ptr_t proposal_block = make_next_block(cs_para, result);
    clear_tx();
    if (proposal_block == nullptr) {
        xwarn("xunit_maker_t::verify_proposal fail-make proposal.%s,account=%s,ret=%s",
            cs_para.dump().c_str(), get_account().c_str(), chainbase::xmodule_error_to_str(result.m_make_block_error_code).c_str());
        return nullptr;
    }
    xdbg("xunit_maker_t::verify_proposal succ unit. %s,unit=%s,cert=%s,class=%d,unconfirm=%d,prev_confirmed=%d,tx_count=%d,latest_state=%s",
        cs_para.dump().c_str(), proposal_block->dump().c_str(), proposal_block->dump_cert().c_str(), proposal_block->get_block_class(),
        proposal_block->get_unconfirm_sendtx_num(), proposal_block->is_prev_sendtx_confirmed(),
        result.m_success_txs.size(), dump().c_str());
    return proposal_block;
}


xblock_ptr_t xunit_maker_t::make_next_block(const data::xblock_consensus_para_t & cs_para, xunitmaker_result_t & result) {
    xblock_ptr_t proposal_unit = nullptr;

    // reset justify cert hash para
    std::string justify_cert_hash = get_lock_block_sign_hash();
    cs_para.set_justify_cert_hash(justify_cert_hash);
    m_default_builder_para->set_error_code(xsuccess);

    // firstly should process txs and try to make lightunit
    if (can_make_next_light_block()) {
        base::xreceiptid_state_ptr_t receiptid_state = result.m_tablestate->get_receiptid_state();
        xblock_builder_para_ptr_t build_para = std::make_shared<xlightunit_builder_para_t>(m_pending_txs, receiptid_state, get_resources());
        proposal_unit = m_lightunit_builder->build_block(get_highest_height_block(),
                                                        clone_latest_committed_state(),
                                                        cs_para,
                                                        build_para);
        result.m_make_block_error_code = build_para->get_error_code();
        std::shared_ptr<xlightunit_builder_para_t> lightunit_build_para = std::dynamic_pointer_cast<xlightunit_builder_para_t>(build_para);
        result.m_success_txs = lightunit_build_para->get_origin_txs();
        for (auto & tx : lightunit_build_para->get_fail_txs()) {
            xwarn("xunit_maker_t::make_next_block fail-pop send tx. account=%s,tx=%s", get_account().c_str(), tx->dump().c_str());
            xtxpool_v2::tx_info_t txinfo(get_account(), tx->get_transaction()->digest(), tx->get_tx_subtype());
            get_txpool()->pop_tx(txinfo);
        }
    } else if (can_make_next_full_block()) {
        proposal_unit = m_fullunit_builder->build_block(get_highest_height_block(),
                                                        get_latest_committed_state(),
                                                        cs_para,
                                                        m_default_builder_para);
        result.m_make_block_error_code = m_default_builder_para->get_error_code();
    } else if (can_make_next_empty_block()) {
        proposal_unit = m_emptyunit_builder->build_block(get_highest_height_block(),
                                                        nullptr,
                                                        cs_para,
                                                        m_default_builder_para);
        result.m_make_block_error_code = m_default_builder_para->get_error_code();
    } else {
        result.m_make_block_error_code = xblockmaker_error_no_need_make_unit;
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
    if (current_lightunit_count >= m_fullunit_contain_of_unit_num_para) {
        if (get_latest_committed_state()->get_unconfirm_sendtx_num() == 0) {
            return true;
        }
        xwarn("xunit_maker_t::can_make_next_full_block state_height=%ld, unconfirm_sendtx_num=%d,prev_block=%s",
            get_latest_committed_state()->get_last_height(), get_latest_committed_state()->get_unconfirm_sendtx_num(), prev_block->dump().c_str());
    }
    return false;
}

bool xunit_maker_t::can_make_next_light_block() const {
    if (m_pending_txs.empty()) {
        return false;
    }
    // TODO(jimmy) non contious block make mode. condition:non-empty block is committed status
    if (is_account_locked())  {
        xerror("xunit_maker_t::can_make_next_light_block account locked. account=%s", get_account().c_str());
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
