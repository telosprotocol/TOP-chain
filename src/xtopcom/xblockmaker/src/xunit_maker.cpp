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
#include "xmbus/xevent_behind.h"
#include "xchain_fork/xchain_upgrade_center.h"

NS_BEG2(top, blockmaker)

xunit_maker_t::xunit_maker_t(const std::string & account, const xblockmaker_resources_ptr_t & resources)
: xblock_maker_t(account, resources, m_keep_latest_blocks_max) {
    xdbg("xunit_maker_t::xunit_maker_t create,this=%p,account=%s", this, account.c_str());
    m_fullunit_contain_of_unit_num_para = XGET_ONCHAIN_GOVERNANCE_PARAMETER(fullunit_contain_of_unit_num);

    m_fullunit_builder = std::make_shared<xfullunit_builder_t>();
    m_lightunit_builder = std::make_shared<xlightunit_builder_t>();
    m_emptyunit_builder = std::make_shared<xemptyunit_builder_t>();
    m_default_builder_para = std::make_shared<xblock_builder_para_face_t>(resources);
}

xunit_maker_t::~xunit_maker_t() {
    xdbg("xunit_maker_t::xunit_maker_t destroy,this=%p", this);
}

xblock_ptr_t xunit_maker_t::get_latest_block(const base::xaccount_index_t & account_index) {
    base::xblock_vector blocks = get_blockstore()->load_block_object(*this, account_index.get_latest_unit_height(), metrics::blockstore_access_from_blk_mk_unit_ld_last_blk);
    for (auto & block : blocks.get_vector()) {
        if (account_index.is_match_unit(block)) {
            return xblock_t::raw_vblock_to_object_ptr(block);
        }
    }
    xwarn("xunit_maker_t::get_latest_block fail find match block. account=%s,index=%s,block_size=%zu",
        get_account().c_str(), account_index.dump().c_str(), blocks.get_vector().size());
    return nullptr;
}

void xunit_maker_t::try_sync_lacked_blocks(uint64_t from_height, uint64_t to_height, const std::string & reason, bool is_consensus, bool need_proof) {
    xinfo("xunit_maker_t::try_sync_lacked_blocks check_latest_state %s, account=%s,try sync unit from:%llu,end:%llu,proof:%d", reason.c_str(), get_account().c_str(), from_height, to_height, need_proof);
    if (to_height >= from_height) {
        uint32_t sync_num = (uint32_t)(to_height + 1 - from_height);
        mbus::xevent_behind_ptr_t ev = make_object_ptr<mbus::xevent_behind_on_demand_t>(
            get_address(), from_height, sync_num, is_consensus, "account_state_fall_behind", need_proof);
        get_bus()->push_event(ev);
    }
}

int32_t xunit_maker_t::check_latest_state(const data::xblock_consensus_para_t & cs_para,
                                          const base::xaccount_index_t & account_index,
                                          const base::xaccount_index_t & commit_account_index) {
    if (m_check_state_success && m_latest_account_index == account_index) {
        return xsuccess;
    }

    XMETRICS_TIMER(metrics::cons_unitmaker_check_state_tick);
    do {
        m_check_state_success = false;
        // firstly, load connected block, always sync unit from latest connected block
        if (!xblocktool_t::check_lacking_unit_and_try_sync(*this, commit_account_index, get_blockstore(), "unit_maker")) {
            break;
        }

        auto _latest_cert_block = get_blockstore()->load_block_object(
            *this, account_index.get_latest_unit_height(), account_index.get_latest_unit_viewid(), false, metrics::blockstore_access_from_blk_mk_unit_chk_last_state);
        if (_latest_cert_block == nullptr) {
            xwarn("xunit_maker_t::check_latest_state fail-load unit cert block.%s, account=%s,index=%s,missing_height=%ld",
                  cs_para.dump().c_str(),
                  get_account().c_str(),
                  account_index.dump().c_str(),
                  account_index.get_latest_unit_height());
            break;
        }
        xblock_ptr_t latest_block = xblock_t::raw_vblock_to_object_ptr(_latest_cert_block.get());
    
        uint64_t lacked_height_from = 0;
        uint64_t lacked_height_to = 0;
        auto & fork_config = top::chain_fork::xtop_chain_fork_config_center::chain_fork_config();
        bool is_forked = chain_fork::xtop_chain_fork_config_center::is_forked(fork_config.block_fork_point, cs_para.get_clock());
        if (is_forked) {
            set_keep_latest_blocks_max(1);
        }
        if (!load_and_cache_enough_blocks(latest_block, lacked_height_from, lacked_height_to)) {
            xwarn("xunit_maker_t::check_latest_state fail-load unit block.%s, account=%s,index=%s,missing_height=%ld:%ld",
                cs_para.dump().c_str(), get_account().c_str(), account_index.dump().c_str(), lacked_height_from, lacked_height_to);
            try_sync_lacked_blocks(lacked_height_from, lacked_height_to, "missing_unit_lock_commit", false, true);
            break;
        }

        if (!update_account_state(latest_block)) {
            xerror("xunit_maker_t::check_latest_state fail-make unit state.%s,account=%s,index=%s,missing_height=%ld:%ld",
                cs_para.dump().c_str(), get_account().c_str(), account_index.dump().c_str(), lacked_height_from, lacked_height_to);
            break;
        }

        if (false == check_latest_blocks(latest_block)) {
            xerror("xunit_maker_t::check_latest_state fail-check_latest_blocks.%s,account=%s,index=%s,latest_block=%s",
                cs_para.dump().c_str(), get_account().c_str(), account_index.dump().c_str(), latest_block->dump().c_str());
            break;
        }
        m_latest_account_index = account_index;
        m_check_state_success = true;
        XMETRICS_GAUGE(metrics::cons_fail_make_proposal_unit_check_state, 1);
        return xsuccess;
    } while(0);

    XMETRICS_GAUGE(metrics::cons_fail_make_proposal_unit_check_state, 0);
    return xblockmaker_error_latest_unit_blocks_invalid;
}

void xunit_maker_t::find_highest_send_tx(uint64_t & latest_nonce, uint256_t & latest_hash) {
    uint64_t max_tx_nonce = 0;
    uint256_t tx_hash;
    for (auto iter = m_pending_txs.rbegin(); iter != m_pending_txs.rend(); iter++) {
        auto & tx = *iter;
        if (tx->is_send_tx() || tx->is_self_tx()) {
            if (tx->get_transaction()->get_tx_nonce() > max_tx_nonce) {
                max_tx_nonce = tx->get_transaction()->get_tx_nonce();
                tx_hash = tx->get_tx_hash_256();
            }
        }
    }
    if (max_tx_nonce > 0) {
        latest_nonce = max_tx_nonce;
        latest_hash = tx_hash;
    } else {
        latest_nonce = get_latest_bstate()->get_latest_send_trans_number();
        latest_hash = get_latest_bstate()->account_send_trans_hash();
    }
}

bool xunit_maker_t::push_tx(const data::xblock_consensus_para_t & cs_para, const xcons_transaction_ptr_t & tx) {
    if (is_account_locked()) {
        xwarn("xunit_maker_t::push_tx fail-tx filtered for account locked.%s,tx=%s", cs_para.dump().c_str(), tx->dump().c_str());
        return false;
    }

    uint64_t current_lightunit_count = get_current_lightunit_count_from_full();

    auto fork_config = top::chain_fork::xtop_chain_fork_config_center::chain_fork_config();
    bool is_forked = chain_fork::xtop_chain_fork_config_center::is_forked(fork_config.block_fork_point, cs_para.get_clock());
    // send and self tx is filtered when matching fullunit limit
    if (tx->is_self_tx() || tx->is_send_tx()) {
        if (is_match_account_fullunit_send_tx_limit(current_lightunit_count)) {
            XMETRICS_GAUGE(metrics::cons_packtx_fail_fullunit_limit, 1);
            xwarn("xunit_maker_t::push_tx fail-tx filtered for fullunit limit.%s,account=%s,lightunit_count=%ld,tx=%s,forked:%d",
                cs_para.dump().c_str(), get_account().c_str(), current_lightunit_count, tx->dump().c_str(), is_forked);
            if (!is_forked)
                return false;
        }
    }

    // recv tx should also limit for fullunit
    if (tx->is_recv_tx()) {
        if (is_match_account_fullunit_recv_tx_limit(current_lightunit_count)) {
            XMETRICS_GAUGE(metrics::cons_packtx_fail_fullunit_limit, 1);
            xwarn("xunit_maker_t::push_tx fail-tx filtered for fullunit limit.%s,account=%s,lightunit_count=%ld,tx=%s,forked:%d",
                cs_para.dump().c_str(), get_account().c_str(), current_lightunit_count, tx->dump().c_str(),is_forked);
            if (!is_forked)
                return false;
        }
    }

    // on demand load origin tx for execute
    if (tx->get_transaction() == nullptr) {
        xassert(tx->is_confirm_tx()); // only confirmtx support on demand load
        // TODO(jimmy) only load origin tx
        base::xvtransaction_store_ptr_t tx_store = get_blockstore()->query_tx(tx->get_tx_hash(), base::enum_transaction_subtype_send);
        if (tx_store == nullptr || tx_store->get_raw_tx() == nullptr) {
            XMETRICS_GAUGE(metrics::cons_packtx_fail_load_origintx, 1);
            xwarn("xunit_maker_t::push_tx fail-load origin tx.%s tx=%s", cs_para.dump().c_str(), tx->dump().c_str());
            return false;
        }
        xtransaction_t * raw_tx = dynamic_cast<xtransaction_t *>(tx_store->get_raw_tx());
        if (false == tx->set_raw_tx(raw_tx)) {
            xerror("xunit_maker_t::push_tx fail-set origin tx.%s tx=%s", cs_para.dump().c_str(), tx->dump().c_str());
            return false;
        }
        xdbg_info("xunit_maker_t::push_tx load and set raw tx succ. %s tx=%s", cs_para.dump().c_str(), tx->dump().c_str());
        xassert(tx->get_transaction()->get_source_addr() == get_account());
    }

    if (tx->is_confirm_tx()) {
        auto latest_nonce = get_latest_bstate()->get_latest_send_trans_number();
        if (tx->get_transaction()->get_tx_nonce() > latest_nonce) {
            XMETRICS_GAUGE(metrics::cons_packtx_fail_nonce_contious, 1);
            xwarn("xunit_maker_t::push_tx fail-tx filtered for nonce is overstepped. %s latest_nonce=%llu, tx=%s",
                cs_para.dump().c_str(), latest_nonce, tx->dump(true).c_str());
            return false;
        }
    }

    // send tx contious nonce rules
    if (tx->is_send_tx() || tx->is_self_tx()) {
        uint64_t latest_nonce;
        uint256_t latest_hash;
        find_highest_send_tx(latest_nonce, latest_hash);
        if (tx->get_transaction()->get_last_nonce() != latest_nonce) {
            // auto commit_bstate = base::xvchain_t::instance().get_xstatestore()->get_blkstate_store()->get_latest_connectted_block_state(*this,metrics::statestore_access_from_blkmaker_unit_connect_state);
            // if (commit_bstate != nullptr) {
            //     xunit_bstate_t committed_state(commit_bstate.get());
            //     uint64_t account_latest_nonce = committed_state.get_latest_send_trans_number();
            //     get_txpool()->updata_latest_nonce(get_account(), account_latest_nonce);
            // }
            XMETRICS_GAUGE(metrics::cons_packtx_fail_nonce_contious, 1);
            xwarn("xunit_maker_t::push_tx fail-tx filtered for send nonce hash not match,%s,bstate=%s,latest_nonce=%ld,tx=%s",
                cs_para.dump().c_str(), get_latest_bstate()->get_bstate()->dump().c_str(), latest_nonce, tx->dump().c_str());
            return false;
        }
    }

    // TODO(jimmy) same subtype limit
    // if (!m_pending_txs.empty()) {
    //     base::enum_transaction_subtype first_tx_subtype = m_pending_txs[0]->get_tx_subtype();
    //     if (first_tx_subtype == base::enum_transaction_subtype_self) {
    //         first_tx_subtype = base::enum_transaction_subtype_send;
    //     }
    //     base::enum_transaction_subtype new_tx_subtype = tx->get_tx_subtype();
    //     if (new_tx_subtype == base::enum_transaction_subtype_self) {
    //         new_tx_subtype = base::enum_transaction_subtype_send;
    //     }
    //     if (new_tx_subtype != first_tx_subtype) {
    //         xwarn("xunit_maker_t::push_tx fail-tx filtered for not same subtype.%s,tx=%s,tx_count=%zu,first_tx=%s",
    //             cs_para.dump().c_str(), tx->dump().c_str(), m_pending_txs.size(), m_pending_txs[0]->dump().c_str());
    //         return false;
    //     }
    // }

    // TODO(jimmy) non-transfer tx only include one tx limit
    if (!m_pending_txs.empty()) {
        data::enum_xtransaction_type first_tx_type = (data::enum_xtransaction_type)m_pending_txs[0]->get_transaction()->get_tx_type();
        if ( (first_tx_type != xtransaction_type_transfer) || ((data::enum_xtransaction_type)tx->get_transaction()->get_tx_type() != data::xtransaction_type_transfer) ) {
            XMETRICS_GAUGE(metrics::cons_packtx_fail_transfer_limit, 1);
            xwarn("xunit_maker_t::push_tx fail-tx filtered for non-transfer txs.%s,tx=%s", cs_para.dump().c_str(), tx->dump(true).c_str());
            return false;
        }
    }

    for (auto & v : m_pending_txs) {
        if (tx->get_tx_hash_256() == v->get_tx_hash_256()) {
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
    XMETRICS_TIMER(metrics::cons_make_unit_tick);
    xblock_ptr_t proposal_block = make_next_block(unit_para, cs_para, result);
    clear_tx();
    if (proposal_block == nullptr) {
        xassert(result.m_make_block_error_code != xsuccess);
        if (xblockmaker_error_no_need_make_unit != result.m_make_block_error_code) {
            xwarn("xunit_maker_t::make_proposal fail-make proposal.%s,account=%s,height=%" PRIu64 ",ret=%s",
                cs_para.dump().c_str(), get_account().c_str(), get_highest_height_block()->get_height(), chainbase::xmodule_error_to_str(result.m_make_block_error_code).c_str());
        } else {
            xwarn("xunit_maker_t::make_proposal no need make proposal.%s,account=%s,height=%" PRIu64 "",
                cs_para.dump().c_str(), get_account().c_str(), get_highest_height_block()->get_height());
        }
        return nullptr;
    }
    xinfo("xunit_maker_t::make_proposal succ unit.is_leader=%d,%s,unit=%s,ir=%s,or=%s,state=%s,txs_info={ufm=%d,total=%d,self=%d,send=%d,recv=%d,confirm=%d}",
        unit_para.m_is_leader, cs_para.dump().c_str(), proposal_block->dump().c_str(),
        base::xstring_utl::to_hex(proposal_block->get_input_root_hash()).c_str(), base::xstring_utl::to_hex(proposal_block->get_output_root_hash()).c_str(),
        dump().c_str(),
        get_latest_bstate()->get_unconfirm_sendtx_num(),
        (uint32_t)result.m_pack_txs.size(), result.m_self_tx_num, result.m_send_tx_num, result.m_recv_tx_num, result.m_confirm_tx_num);

    uint64_t now = cs_para.get_gettimeofday_s();
    for (auto & tx : result.m_pack_txs) {
        uint64_t delay = now - tx->get_push_pool_timestamp();
        xinfo("xunit_maker_t::make_proposal succ tx.is_leader=%d,%s,unit=%s,tx=%s,delay=%llu",
            unit_para.m_is_leader, cs_para.dump().c_str(), proposal_block->dump().c_str(), tx->dump().c_str(), now - tx->get_push_pool_timestamp());
        if (unit_para.m_is_leader) {
            if (tx->is_recv_tx()) {
                XMETRICS_GAUGE(metrics::txpool_tx_delay_from_push_to_pack_recv, delay);
            } else if (tx->is_confirm_tx()) {
                XMETRICS_GAUGE(metrics::txpool_tx_delay_from_push_to_pack_confirm, delay);
            } else {
                XMETRICS_GAUGE(metrics::txpool_tx_delay_from_push_to_pack_send, delay);
            }
        }
    }

    return proposal_block;
}

// same for light unit & full unit, full unit now 
void xunit_maker_t::make_light_block(xblock_ptr_t & proposal_unit, xblock_builder_face_ptr_t & block_builder, const xunitmaker_para_t & unit_para, const data::xblock_consensus_para_t & cs_para, xunitmaker_result_t & result) {
    const xblock_ptr_t & cert_block = get_highest_height_block();
    base::xreceiptid_state_ptr_t receiptid_state = unit_para.m_tablestate->get_receiptid_state();
    xblock_builder_para_ptr_t build_para = std::make_shared<xlightunit_builder_para_t>(m_pending_txs, receiptid_state, get_resources());
    proposal_unit = block_builder->build_block(cert_block,
                                                    get_latest_bstate()->get_bstate(),
                                                    cs_para,
                                                    build_para);
    result.m_make_block_error_code = build_para->get_error_code();
    std::shared_ptr<xlightunit_builder_para_t> lightunit_build_para = std::dynamic_pointer_cast<xlightunit_builder_para_t>(build_para);
    result.add_pack_txs(lightunit_build_para->get_pack_txs());
    result.m_fail_txs = lightunit_build_para->get_fail_txs();
    result.m_tgas_balance_change = lightunit_build_para->get_tgas_balance_change();
    result.m_unchange_txs = lightunit_build_para->get_unchange_txs();
    if (lightunit_build_para->get_pack_txs().empty() && !lightunit_build_para->get_unchange_txs().empty()) {
        result.m_make_block_error_code = xblockmaker_error_no_need_make_unit;
    }
    for (auto & tx : lightunit_build_para->get_fail_txs()) {
        xassert(tx->is_self_tx() || tx->is_send_tx());
        xwarn("xunit_maker_t::make_next_block fail-pop send tx. account=%s,tx=%s", get_account().c_str(), tx->dump().c_str());
        xtxpool_v2::tx_info_t txinfo(get_account(), tx->get_tx_hash_256(), tx->get_tx_subtype());
        get_txpool()->pop_tx(txinfo);
    }
}

xblock_ptr_t xunit_maker_t::make_next_block(const xunitmaker_para_t & unit_para, const data::xblock_consensus_para_t & cs_para, xunitmaker_result_t & result) {
    xblock_ptr_t proposal_unit = nullptr;

    const xblock_ptr_t & cert_block = get_highest_height_block();
    // TODO(jimmy) check cache blocks again
    base::xaccount_index_t accountindex;
    unit_para.m_tablestate->get_account_index(get_account(), accountindex);
    if (cert_block->get_height() != accountindex.get_latest_unit_height()
        || cert_block->get_viewid() != accountindex.get_latest_unit_viewid()) {
        xerror("xunit_maker_t::make_next_block,fail-check highest cert block. %s,account=%s", cs_para.dump().c_str(), get_account().c_str());
        return nullptr;
    }

    auto & fork_config = top::chain_fork::xtop_chain_fork_config_center::chain_fork_config();
    bool is_forked = chain_fork::xtop_chain_fork_config_center::is_forked(fork_config.block_fork_point, cs_para.get_clock());
    if (!is_forked) {
        xblock_ptr_t lock_block = get_prev_block_from_cache(cert_block);
        if (lock_block == nullptr) {
            xerror("xunit_maker_t::make_next_block,fail-get lock block. %s,cert_block=%s", cs_para.dump().c_str(), cert_block->dump().c_str());
            return nullptr;
        }
        // reset justify cert hash para
        cs_para.set_justify_cert_hash(lock_block->get_input_root_hash());
        xassert(cs_para.get_proposal_height() != 0);
        cs_para.set_parent_height(cs_para.get_proposal_height());
        m_default_builder_para->set_error_code(xsuccess);
    }

    XMETRICS_GAUGE(metrics::cons_table_total_process_unit_count, 1);
    XMETRICS_GAUGE(metrics::cons_table_total_process_tx_count, m_pending_txs.size());        
    if (is_forked) {
        // firstly try to make full unit and process txs
        if (can_make_next_full_block(is_forked)) {
            xwarn("xunit_maker_t::make_next_block full block. account=%s,pending_txs:%zu,cs_para:%s", get_account().c_str(), m_pending_txs.size(), cs_para.dump().c_str());
            make_light_block(proposal_unit, m_fullunit_builder, unit_para, cs_para, result);
        }

        // secondly try to make lightunit and process txs
        if (nullptr == proposal_unit && result.m_make_block_error_code != xblockmaker_error_no_need_make_unit && can_make_next_light_block()) {
            make_light_block(proposal_unit, m_lightunit_builder, unit_para, cs_para, result);
        }
    } else {
        // firstly should process txs and try to make lightunit
        if (can_make_next_light_block()) {
            make_light_block(proposal_unit, m_lightunit_builder, unit_para, cs_para, result);
        }

        // secondly try to make full unit
        if (nullptr == proposal_unit && can_make_next_full_block(is_forked)) {
            proposal_unit = m_fullunit_builder->build_block(cert_block,
                                                            get_latest_bstate()->get_bstate(),
                                                            cs_para,
                                                            m_default_builder_para);
            result.m_make_block_error_code = m_default_builder_para->get_error_code();
        }
    }

    if (!is_forked) {
        // thirdly try to make empty unit
        if (nullptr == proposal_unit && can_make_next_empty_block()) {
            XMETRICS_GAUGE(metrics::cons_table_total_process_unit_count, 1);
            proposal_unit = m_emptyunit_builder->build_block(cert_block,
                                                            nullptr,
                                                            cs_para,
                                                            m_default_builder_para);
            result.m_make_block_error_code = m_default_builder_para->get_error_code();
        }
    }

    if (is_forked) {
        if ((nullptr == proposal_unit) && (result.m_make_block_error_code == 0)) {
            xdbg("wish null unit account:%s,cs_para:%s,make_block_error_code:%d", get_account().c_str(),cs_para.dump().c_str(),result.m_make_block_error_code);
            result.m_make_block_error_code = xblockmaker_error_null_unit;
        }
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

bool xunit_maker_t::can_make_next_block_v2() const {
    if (can_make_next_light_block() || can_make_next_full_block(true)) {
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
    xblock_ptr_t prev_block = get_prev_block_from_cache(current_block);
    if (prev_block == nullptr) {
        return false;
    }
    if (prev_block->get_block_class() == base::enum_xvblock_class_light) {
        return true;
    }
    return false;
}

bool xunit_maker_t::is_account_locked() const {
    // return can_make_next_empty_block();
    return false;  // TODO(jimmy)
}

uint64_t xunit_maker_t::get_current_lightunit_count_from_full() const {
    base::xvblock_t* prev_block = get_highest_height_block().get();
    uint64_t current_height = prev_block->get_height() + 1;
    uint64_t current_fullunit_height = prev_block->get_block_class() == base::enum_xvblock_class_full ? prev_block->get_height() : prev_block->get_last_full_block_height();
    uint64_t current_lightunit_count = current_height - current_fullunit_height;
    return current_lightunit_count;
}
bool xunit_maker_t::is_match_account_fullunit_send_tx_limit(uint64_t current_lightunit_count) const {
    uint64_t max_limit_lightunit_count = XGET_ONCHAIN_GOVERNANCE_PARAMETER(fullunit_contain_of_unit_num); // TODO(jimmy)
    if (current_lightunit_count >= max_limit_lightunit_count) {
        return true;
    }
    return false;
}
bool xunit_maker_t::is_match_account_fullunit_recv_tx_limit(uint64_t current_lightunit_count) const {
    uint64_t max_limit_lightunit_count = XGET_ONCHAIN_GOVERNANCE_PARAMETER(fullunit_contain_of_unit_num)*2; // TODO(jimmy)
    if (current_lightunit_count >= max_limit_lightunit_count) {
        return true;
    }
    return false;
}

bool xunit_maker_t::must_make_next_full_block() const {
    uint64_t current_lightunit_count = get_current_lightunit_count_from_full();
    xassert(current_lightunit_count > 0);
    uint64_t max_limit_lightunit_count = XGET_ONCHAIN_GOVERNANCE_PARAMETER(fullunit_contain_of_unit_num);
    if (current_lightunit_count >= max_limit_lightunit_count * 2) {
        return true;
    }
    return false;
}

bool xunit_maker_t::can_make_next_full_block(bool is_forked_unit_opt) const {
    // TODO(jimmy) non contious block make mode. condition:non-empty block is committed status
    xwarn("xunit_maker_t::can_make_next_full_block lightunit.account=%s,current_height=%ld,pending_txs=%zu,locked=%d,forked%d",
        get_account().c_str(), get_latest_bstate()->get_block_height(), m_pending_txs.size(), is_account_locked(), is_forked_unit_opt);
    if (is_forked_unit_opt) {
        if (m_pending_txs.empty()) {
            return false;
        }
    }
    
    if (is_account_locked()) {
        return false;
    }

    uint64_t current_lightunit_count = get_current_lightunit_count_from_full();
    xassert(current_lightunit_count > 0);
    uint64_t max_limit_lightunit_count = XGET_ONCHAIN_GOVERNANCE_PARAMETER(fullunit_contain_of_unit_num);
    xwarn("xunit_maker_t::can_make_next_full_block lightunit.account=%s,current_height=%ld,current_lightunit_count=%llu,max_limit_lightunit_count:%llu",
        get_account().c_str(), get_latest_bstate()->get_block_height(), current_lightunit_count, max_limit_lightunit_count);
    if (current_lightunit_count >= max_limit_lightunit_count) {
#if 0  // remove unconfirm limit
        if (get_latest_bstate()->get_unconfirm_sendtx_num() == 0) {  
            return true;
        }
        if (current_lightunit_count >= max_limit_lightunit_count * 2) {  // TODO(jimmy)
            xwarn("xunit_maker_t::can_make_next_full_block too many lightunit.account=%s,current_height=%ld,lightunit_count=%ld,unconfirm_sendtx_num=%d",
                get_account().c_str(), get_latest_bstate()->get_block_height(), current_lightunit_count, get_latest_bstate()->get_unconfirm_sendtx_num());
        }
#endif
        return true;
    }
    return false;
}

bool xunit_maker_t::can_make_next_light_block() const {
    xwarn("xunit_maker_t::can_make_next_light_block lightunit.account=%s,current_height=%ld,pending_txs=%zu,lock=%d",
        get_account().c_str(), get_latest_bstate()->get_block_height(), m_pending_txs.size(), is_account_locked());
    if (m_pending_txs.empty()) {
        return false;
    }
    if (is_account_locked())  {
        return false;
    }
    return true;
}

std::string xunit_maker_t::dump() const {
#ifdef DEBUG
    char local_param_buf[128];
    uint64_t highest_height = get_highest_height_block()->get_height();
    uint64_t state_height = get_latest_bstate()->get_block_height();

    xprintf(local_param_buf,sizeof(local_param_buf),
        "{highest=%" PRIu64 ",index=%s,state=%" PRIu64 ",nonce=%" PRIu64 ",parent=%" PRIu64 "}",
        highest_height, m_latest_account_index.dump().c_str(),state_height,
        get_latest_bstate()->get_latest_send_trans_number(),
        get_highest_height_block()->get_cert()->get_parent_block_height());
    return std::string(local_param_buf);
#else 
    return std::string();
#endif
}

NS_END2
