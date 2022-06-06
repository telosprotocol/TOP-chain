// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xblockmaker/xrelay_proposal_maker.h"

#include "xbasic/xmemory.hpp"
#include "xblockmaker/xblock_maker_para.h"
#include "xblockmaker/xblockmaker_error.h"
#include "xblockmaker/xrelay_proposal_maker.h"
#include "xdata/xblocktool.h"
#include "xmbus/xevent_behind.h"
#include "xstore/xtgas_singleton.h"

#include <string>

NS_BEG2(top, blockmaker)

// REG_XMODULE_LOG(chainbase::enum_xmodule_type::xmodule_type_xblockmaker, xblockmaker_error_to_string, xblockmaker_error_base+1, xblockmaker_error_max);

xrelay_proposal_maker_t::xrelay_proposal_maker_t(const std::string & account,
                                                 const xblockmaker_resources_ptr_t & resources,
                                                 const observer_ptr<xrelay_chain::xrelay_chain_mgr_t> & relay_chain_mgr)
  : m_resources(resources), m_relay_chain_mgr(relay_chain_mgr), m_relay_maker(make_object_ptr<xrelay_maker_t>(account, resources)) {
    xdbg("xrelay_proposal_maker_t::xrelay_proposal_maker_t create,this=%p,account=%s", this, account.c_str());
}

xrelay_proposal_maker_t::~xrelay_proposal_maker_t() {
    xdbg("xrelay_proposal_maker_t::xrelay_proposal_maker_t destroy,this=%p", this);
}

bool xrelay_proposal_maker_t::can_make_proposal(data::xblock_consensus_para_t & proposal_para) {
    if (proposal_para.get_viewid() <= proposal_para.get_latest_cert_block()->get_viewid()) {
        xwarn("xrelay_proposal_maker_t::can_make_proposal fail-behind viewid. %s,latest_viewid=%" PRIu64 "",
              proposal_para.dump().c_str(),
              proposal_para.get_latest_cert_block()->get_viewid());
        return false;
    }

    if (!data::xblocktool_t::verify_latest_blocks(
            proposal_para.get_latest_cert_block().get(), proposal_para.get_latest_locked_block().get(), proposal_para.get_latest_committed_block().get())) {
        xwarn("xrelay_proposal_maker_t::can_make_proposal. fail-verify_latest_blocks fail.%s", proposal_para.dump().c_str());
        return false;
    }
    return true;
}

void xrelay_proposal_maker_t::convert_to_xrelay_receipts(const std::map<uint64_t, xrelay_chain::xcross_txs_t> & cross_tx_map, std::vector<data::xrelay_receipt> receipts) {
    for (auto & cross_tx_map_pair : cross_tx_map) {
        auto & cross_txs = cross_tx_map_pair.second;
        for (auto & tx_result : cross_txs.m_tx_results) {
            // todo(nathan): use real data for m_type, logs_bloom etc.
            evm_common::h2048 logs_bloom(tx_result.bloom().get_data());
            std::vector<data::xrelay_receipt_log> relay_receipt_logs;
            for (auto & log : tx_result.get_logs()) {
                data::xrelay_receipt_log relay_receipt_log;
                relay_receipt_log.m_contract_address = evm_common::h160(to_bytes(log.address));
                relay_receipt_log.m_data = to_bytes(log.data);
                for (auto & topic : log.topics) {
                    relay_receipt_log.m_topics.push_back(evm_common::h256(to_bytes(topic)));
                }
                relay_receipt_logs.push_back(relay_receipt_log);
            }
            data::xrelay_receipt receipt(tx_result.get_version(), (uint8_t)tx_result.get_tx_status(), (evm_common::u256)tx_result.get_cumulative_gas_used(), logs_bloom, relay_receipt_logs);
            receipts.push_back(receipt);
        }
    }
}

data::xrelay_block xrelay_proposal_maker_t::build_relay_block(evm_common::h256 prev_hash, uint64_t block_height, uint64_t cur_evm_table_height, uint64_t timestamp, std::vector<data::xrelay_receipt> receipts) {
    uint64_t block_version = 0;
    evm_common::u256 chain_bits = 0;
    uint64_t epochID = 0;
    data::xrelay_block relay_block(block_version, prev_hash, chain_bits, cur_evm_table_height, block_height, epochID, timestamp);
    xdbg("xrelay_proposal_maker_t::build_relay_block, %s,%llu,%llu", prev_hash.hex().c_str(), block_height, timestamp);
    relay_block.set_receipts(receipts);
    return relay_block;
}

bool xrelay_proposal_maker_t::build_relay_block_data_leader(const data::xblock_ptr_t & latest_wrap_block, uint64_t timestamp, std::string & relay_block_data) {
    evm_common::h256 prev_hash;
    uint64_t block_height = 0;

    uint64_t last_evm_table_height = 0;
    auto last_relay_block_data = latest_wrap_block->get_header()->get_extra_data();
    if (!last_relay_block_data.empty()) {
        data::xrelay_block last_relay_block;
        last_relay_block.decodeRLP(evm_common::RLP(last_relay_block_data));
        last_relay_block.build_finish();
        prev_hash = last_relay_block.get_block_hash();
        block_height = last_relay_block.get_inner_header().m_height + 1;
        last_evm_table_height = last_relay_block.get_header().get_table_height();
    } else {
        uint64_t block_version = 0;
        evm_common::u256 chain_bits = 0;
        uint64_t epochID = 0;
        evm_common::h256 prev_hash;
        data::xrelay_block genesis_block(block_version, prev_hash, chain_bits, 0, 0, epochID, 0);
        genesis_block.build_finish();

        prev_hash = genesis_block.get_block_hash();
        block_height = genesis_block.get_inner_header().m_height + 1;
        last_evm_table_height = genesis_block.get_header().get_table_height();
    }

    std::map<uint64_t, xrelay_chain::xcross_txs_t> cross_tx_map;
    uint64_t cur_evm_table_height = 0;
    auto ret = m_relay_chain_mgr->get_tx_cache_leader(last_evm_table_height, cur_evm_table_height, cross_tx_map);
    if (!ret) {
        return {};
    }
    if (cross_tx_map.empty()) {
        xinfo("xrelay_proposal_maker_t::build_relay_block_data_leader no cross txs.");
        return false;
    }

    std::vector<data::xrelay_receipt> receipts;
    convert_to_xrelay_receipts(cross_tx_map, receipts);

    data::xrelay_block relay_block = build_relay_block(prev_hash, block_height, cur_evm_table_height, timestamp, receipts);

    evm_common::RLPStream rlp_stream;
    relay_block.streamRLP(rlp_stream);
    relay_block_data = from_bytes<std::string>((xbytes_t)(rlp_stream.out()));
    return true;
}

bool xrelay_proposal_maker_t::build_relay_block_data_backup(evm_common::h256 prev_hash, uint64_t block_height, uint64_t last_evm_table_height, uint64_t cur_evm_table_height, uint64_t timestamp, std::string & relay_block_data) {
    std::map<uint64_t, xrelay_chain::xcross_txs_t> cross_tx_map;
    auto ret = m_relay_chain_mgr->get_tx_cache_backup(last_evm_table_height, cur_evm_table_height, cross_tx_map);
    if (!ret) {
        return {};
    }
    std::vector<data::xrelay_receipt> receipts;
    convert_to_xrelay_receipts(cross_tx_map, receipts);

    data::xrelay_block relay_block = build_relay_block(prev_hash, block_height, cur_evm_table_height, timestamp, receipts);

    evm_common::RLPStream rlp_stream;
    relay_block.streamRLP(rlp_stream);
    relay_block_data = from_bytes<std::string>((xbytes_t)(rlp_stream.out()));
    return true;
}


data::xtablestate_ptr_t xrelay_proposal_maker_t::get_target_tablestate(base::xvblock_t * block) {
    base::xauto_ptr<base::xvbstate_t> bstate = m_resources->get_xblkstatestore()->get_block_state(block, metrics::statestore_access_from_blkmaker_get_target_tablestate);
    if (bstate == nullptr) {
        xwarn("xrelay_proposal_maker_t::get_target_tablestate fail-get target state.block=%s", block->dump().c_str());
        return nullptr;
    }
    data::xtablestate_ptr_t tablestate = std::make_shared<data::xtable_bstate_t>(bstate.get());
    return tablestate;
}

xblock_ptr_t xrelay_proposal_maker_t::make_proposal(data::xblock_consensus_para_t & proposal_para, uint32_t min_tx_num) {
    auto & latest_cert_block = proposal_para.get_latest_cert_block();
    data::xtablestate_ptr_t tablestate = get_target_tablestate(latest_cert_block.get());
    if (nullptr == tablestate) {
        xwarn("xrelay_proposal_maker_t::make_proposal fail clone tablestate. %s,cert_height=%" PRIu64 "", proposal_para.dump().c_str(), latest_cert_block->get_height());
        return nullptr;
    }

    data::xtablestate_ptr_t tablestate_commit = get_target_tablestate(proposal_para.get_latest_committed_block().get());
    if (tablestate_commit == nullptr) {
        xwarn("xrelay_proposal_maker_t::make_proposal fail clone tablestate. %s,commit_height=%" PRIu64 "",
              proposal_para.dump().c_str(),
              proposal_para.get_latest_committed_block()->get_height());
        return nullptr;
    }

    xtablemaker_para_t table_para(tablestate, tablestate_commit);

    // todo(nathan): set relay block data, evm height, rec height to wrap block.
    uint8_t last_wrap_phase;
    uint64_t last_evm_height;
    uint64_t last_rec_height;
    if (latest_cert_block->is_genesis_block()) {
        last_wrap_phase = 2;
        last_evm_height = 0;
        last_rec_height = 0;
    } else {
        auto & last_wrap_data = latest_cert_block->get_header()->get_comments();
        if (last_wrap_data.empty()) {
            xerror("xrelay_proposal_maker_t::make_proposal wrap data should not empty.");
            return nullptr;
        } else {
            base::xstream_t stream_wrap_data{base::xcontext_t::instance(), (uint8_t*)last_wrap_data.data(), static_cast<uint32_t>(last_wrap_data.size())};
            stream_wrap_data >> last_wrap_phase;
            stream_wrap_data >> last_evm_height;
            stream_wrap_data >> last_rec_height;
        }
    }

    base::xstream_t _stream(base::xcontext_t::instance());
    // wrap phase: 0, 1, 2
    uint8_t wrap_phase = last_wrap_phase + 1;
    uint64_t evm_height;
    uint64_t rec_height;
    std::string relay_block_data;
    if (wrap_phase > 2) {
        wrap_phase = 0;
        // use m_relay_chain_mgr build relay block data.
        // std::map<uint64_t, xrelay_chain::xcross_txs_t> cross_tx_map;
        // uint64_t upper_height;
        // uint32_t tx_num;
        // bool ret = m_relay_chain_mgr->get_tx_cache(last_evm_height, cross_tx_map, evm_height, tx_num);
        // if (!ret) {
        //     xinfo("xrelay_proposal_maker_t::make_proposal no cross tx, not pack relay block.");
        //     return nullptr;
        // }
        // todo(nathan): build real relay block data.
        bool ret = build_relay_block_data_leader(latest_cert_block, proposal_para.get_gmtime(), relay_block_data);
        if (!ret) {
            xinfo("xrelay_proposal_maker_t::make_proposal fail-no tx for pack.%s", proposal_para.dump().c_str());
            return nullptr;
        }
        // todo(nathan): use real rec height.
        evm_height = last_evm_height + 1;
        rec_height = last_rec_height + 1;
    }
    else {
        evm_height = last_evm_height;
        rec_height = last_rec_height;
        relay_block_data = latest_cert_block->get_header()->get_extra_data();
    }
    _stream << wrap_phase;
    _stream << evm_height;
    _stream << rec_height;
    std::string wrap_data = std::string((char *)_stream.data(), _stream.size());

    table_para.set_relay_block_data(relay_block_data);
    table_para.set_relay_wrap_data(wrap_data);

    if (false == leader_set_consensus_para(latest_cert_block.get(), proposal_para)) {
        xwarn("xrelay_proposal_maker_t::make_proposal fail-leader_set_consensus_para.%s", proposal_para.dump().c_str());
        return nullptr;
    }

    xtablemaker_result_t table_result;
    xblock_ptr_t proposal_block = m_relay_maker->make_proposal(table_para, proposal_para, table_result);
    if (proposal_block == nullptr) {
        if (xblockmaker_error_no_need_make_table != table_result.m_make_block_error_code) {
            XMETRICS_GAUGE(metrics::cons_table_leader_make_proposal_succ, 0);
            xwarn("xrelay_proposal_maker_t::make_proposal fail-make_proposal.%s error_code=%s",
                  proposal_para.dump().c_str(),
                  chainbase::xmodule_error_to_str(table_result.m_make_block_error_code).c_str());
        } else {
            xinfo("xrelay_proposal_maker_t::make_proposal no need make table.%s", proposal_para.dump().c_str());
        }
        return nullptr;
    }
    auto & proposal_input = table_para.get_proposal();
    std::string proposal_input_str;
    proposal_input->serialize_to_string(proposal_input_str);
    proposal_block->get_input()->set_proposal(proposal_input_str);
    bool bret = proposal_block->reset_prev_block(latest_cert_block.get());
    xassert(bret);

    // add metrics of tx counts / table counts ratio
    xinfo("xrelay_proposal_maker_t::make_proposal succ.proposal_block=%s,proposal_input={size=%zu,txs=%zu,accounts=%zu}",
          proposal_block->dump().c_str(),
          proposal_input_str.size(),
          proposal_input->get_input_txs().size(),
          proposal_input->get_other_accounts().size());
    return proposal_block;
}

bool xrelay_proposal_maker_t::check_wrap_proposal(const xblock_ptr_t & latest_cert_block, base::xvblock_t * proposal_block) {
    uint8_t last_wrap_phase;
    uint64_t last_evm_height;
    uint64_t last_rec_height;
    std::string last_relay_block_data = latest_cert_block->get_relay_block_data();
    
    if (latest_cert_block->is_genesis_block()) {
        last_wrap_phase = 2;
        last_evm_height = 0;
        last_rec_height = 0;
    } else {
        auto & last_wrap_data = latest_cert_block->get_header()->get_comments();
        if (last_wrap_data.empty()) {
            xerror("xrelay_proposal_maker_t::check_wrap_proposal wrap data should not empty.proposal_block=%s", proposal_block->dump().c_str());
            return false;
        } else {
            base::xstream_t stream_wrap_data{base::xcontext_t::instance(), (uint8_t*)last_wrap_data.data(), static_cast<uint32_t>(last_wrap_data.size())};
            stream_wrap_data >> last_wrap_phase;
            stream_wrap_data >> last_evm_height;
            stream_wrap_data >> last_rec_height;
        }
    }

    uint8_t wrap_phase;
    uint64_t evm_height;
    uint64_t rec_height;
    auto & proposal_wrap_data = proposal_block->get_header()->get_comments();
    if (proposal_wrap_data.empty()) {
        xerror("xrelay_proposal_maker_t::check_wrap_proposal wrap data should not empty.");
        return false;
    } else {
        base::xstream_t stream_wrap_data{base::xcontext_t::instance(), (uint8_t*)proposal_wrap_data.data(), static_cast<uint32_t>(proposal_wrap_data.size())};
        stream_wrap_data >> wrap_phase;
        stream_wrap_data >> evm_height;
        stream_wrap_data >> rec_height;
    }

    std::string relay_block_data;
    if (wrap_phase == 0) {
        // relay_block_data = base::xstring_utl::tostring(latest_cert_block->get_height());

        data::xrelay_block proposal_relay_block;
        proposal_relay_block.decodeRLP(evm_common::RLP(proposal_block->get_relay_block_data()));

        evm_common::h256 local_prev_hash;
        uint64_t local_block_height = 0;
        uint64_t local_last_evm_table_height = 0;
        auto last_relay_block_data = latest_cert_block->get_header()->get_extra_data();
        if (!last_relay_block_data.empty()) {
            data::xrelay_block last_relay_block;
            last_relay_block.decodeRLP(evm_common::RLP(last_relay_block_data));
            last_relay_block.build_finish();
            local_prev_hash = last_relay_block.get_block_hash();
            local_block_height = last_relay_block.get_inner_header().m_height + 1;
            local_last_evm_table_height = last_relay_block.get_header().get_table_height();
        } else {
            uint64_t block_version = 0;
            evm_common::u256 chain_bits = 0;
            uint64_t epochID = 0;
            evm_common::h256 prev_hash;
            data::xrelay_block genesis_block(block_version, prev_hash, chain_bits, 0, 0, epochID, 0);
            genesis_block.build_finish();

            prev_hash = genesis_block.get_block_hash();
            local_block_height = genesis_block.get_inner_header().m_height + 1;
            local_last_evm_table_height = genesis_block.get_header().get_table_height();
        }

        if (proposal_relay_block.get_header().get_prev_block_hash() != local_prev_hash || proposal_relay_block.get_inner_header().m_height != local_block_height) {
            xerror("xrelay_proposal_maker_t::check_wrap_proposal relay block pre hash or height not match.pre hash:%s:%s,height:%llu:%llu.proposal_block=%s",
               proposal_relay_block.get_header().get_prev_block_hash().hex().c_str(),
               local_prev_hash.hex().c_str(),
               proposal_relay_block.get_inner_header().m_height,
               local_block_height,
               proposal_block->dump().c_str());
            return false;
        }

        auto ret = build_relay_block_data_backup(local_prev_hash,
                                                 local_block_height,
                                                 local_last_evm_table_height,
                                                 proposal_relay_block.get_header().get_table_height(),
                                                 proposal_relay_block.get_inner_header().m_timestamp,
                                                 relay_block_data);
        if (!ret) {
            xerror("xrelay_proposal_maker_t::check_wrap_proposal build relay block data fail.proposal_block=%s", proposal_block->dump().c_str());
            return false;
        }
    } else {
        relay_block_data = last_relay_block_data;
    }

    if (wrap_phase != ((last_wrap_phase + 1) % 3) || (wrap_phase != 0 && (evm_height != last_evm_height || rec_height != last_rec_height)) ||
        (wrap_phase == 0 && (evm_height != last_evm_height + 1 || rec_height != last_rec_height + 1))) {
        xerror("xrelay_proposal_maker_t::check_wrap_proposal wrap data not match.wrap_phase:%d:%d,evm_height:%llu:%llu,rec_height:%llu:%llu,relay_block_data:%s:%s.proposal_block=%s",
               wrap_phase,
               last_wrap_phase,
               evm_height,
               last_evm_height,
               rec_height,
               last_rec_height,
               relay_block_data.c_str(),
               proposal_block->get_relay_block_data().c_str(),
               proposal_block->dump().c_str());
        return false;
    }

    // check relay_block_data
    if (relay_block_data != proposal_block->get_relay_block_data()) {
        xerror("xrelay_proposal_maker_t::check_wrap_proposal relay block data not match.relay_block_data:%s:%s.proposal_block=%s",
               relay_block_data.c_str(),
               proposal_block->get_relay_block_data().c_str(),
               proposal_block->dump().c_str());
        return false;
    }

    return true;
}

int xrelay_proposal_maker_t::verify_proposal(base::xvblock_t * proposal_block, base::xvqcert_t * bind_clock_cert) {
    xdbg("xrelay_proposal_maker_t::verify_proposal enter. proposal=%s", proposal_block->dump().c_str());
    // uint64_t gmtime = proposal_block->get_second_level_gmtime();
    xblock_consensus_para_t cs_para(
        get_account(), proposal_block->get_clock(), proposal_block->get_viewid(), proposal_block->get_viewtoken(), proposal_block->get_height(), 0);

    // verify gmtime valid
    // uint64_t now = (uint64_t)base::xtime_utl::gettimeofday();
    // if ((gmtime > (now + 60)) || (gmtime < (now - 60))) {  // the gmtime of leader should in +-60s with backup node
    //     xwarn("xrelay_proposal_maker_t::verify_proposal fail-gmtime not match. proposal=%s,leader_gmtime=%ld,backup_gmtime=%ld", proposal_block->dump().c_str(), gmtime, now);
    //     XMETRICS_GAUGE(metrics::cons_fail_verify_proposal_blocks_invalid, 1);
    //     XMETRICS_GAUGE(metrics::cons_table_backup_verify_proposal_succ, 0);
    //     return xblockmaker_error_proposal_outofdate;
    // }

    auto cert_block = m_resources->get_blockstore()->get_latest_cert_block(*m_relay_maker);
    if (proposal_block->get_height() < cert_block->get_height()) {
        xwarn(
            "xrelay_proposal_maker_t::verify_proposal fail-proposal height less than cert block. proposal=%s,cert=%s", proposal_block->dump().c_str(), cert_block->dump().c_str());
        return xblockmaker_error_proposal_cannot_connect_to_cert;
    }

    // TODO(jimmy) xbft callback and pass cert/lock/commit to us for performance
    // find matched cert block
    xblock_ptr_t proposal_prev_block = nullptr;
    if (proposal_block->get_last_block_hash() == cert_block->get_block_hash() && proposal_block->get_height() == cert_block->get_height() + 1) {
        proposal_prev_block = xblock_t::raw_vblock_to_object_ptr(cert_block.get());
    } else {
        auto _demand_cert_block = m_resources->get_blockstore()->load_block_object(
            *m_relay_maker, proposal_block->get_height() - 1, proposal_block->get_last_block_hash(), false, metrics::blockstore_access_from_blk_mk_proposer_verify_proposal);
        if (_demand_cert_block == nullptr) {
            xwarn("xrelay_proposal_maker_t::verify_proposal fail-find cert block. proposal=%s", proposal_block->dump().c_str());
            XMETRICS_GAUGE(metrics::cons_fail_verify_proposal_blocks_invalid, 1);
            XMETRICS_GAUGE(metrics::cons_table_backup_verify_proposal_succ, 0);
            return xblockmaker_error_proposal_cannot_connect_to_cert;
        }
        proposal_prev_block = xblock_t::raw_vblock_to_object_ptr(_demand_cert_block.get());
    }
    cs_para.update_latest_cert_block(proposal_prev_block);  // prev table block is key info

    // find matched lock block
    if (proposal_prev_block->get_height() > 0) {
        auto lock_block = m_resources->get_blockstore()->load_block_object(*m_relay_maker,
                                                              proposal_prev_block->get_height() - 1,
                                                              proposal_prev_block->get_last_block_hash(),
                                                              false,
                                                              metrics::blockstore_access_from_blk_mk_proposer_verify_proposal);
        if (lock_block == nullptr) {
            xwarn("xrelay_proposal_maker_t::verify_proposal fail-find lock block. proposal=%s", proposal_block->dump().c_str());
            return xblockmaker_error_proposal_cannot_connect_to_cert;
        }
        xblock_ptr_t prev_lock_block = xblock_t::raw_vblock_to_object_ptr(lock_block.get());
        cs_para.update_latest_lock_block(prev_lock_block);
    } else {
        cs_para.update_latest_lock_block(proposal_prev_block);
    }
    // find matched commit block
    if (cs_para.get_latest_locked_block()->get_height() > 0) {
        // XTODO get latest connected block which can also load commit block, and it will invoke to update latest connect height.
        auto connect_block = m_resources->get_blockstore()->get_latest_connected_block(*m_relay_maker);
        if (connect_block == nullptr) {
            xerror("xrelay_proposal_maker_t::verify_proposal fail-find connected block. proposal=%s", proposal_block->dump().c_str());
            return xblockmaker_error_proposal_cannot_connect_to_cert;
        }
        if (connect_block->get_height() != cs_para.get_latest_locked_block()->get_height() - 1) {
            xwarn("xrelay_proposal_maker_t::verify_proposal fail-connect not match commit block. proposal=%s,connect_height=%ld",
                  proposal_block->dump().c_str(),
                  connect_block->get_height());
            return xblockmaker_error_proposal_cannot_connect_to_cert;
        }
        xblock_ptr_t prev_commit_block = xblock_t::raw_vblock_to_object_ptr(connect_block.get());
        cs_para.update_latest_commit_block(prev_commit_block);
    } else {
        cs_para.update_latest_commit_block(cs_para.get_latest_locked_block());
    }

    xdbg_info(
        "xrelay_proposal_maker_t::verify_proposal. set latest_cert_block.proposal=%s, latest_cert_block=%s", proposal_block->dump().c_str(), proposal_prev_block->dump().c_str());

    // update txpool receiptid state
    const xblock_ptr_t & commit_block = cs_para.get_latest_committed_block();
    data::xtablestate_ptr_t commit_tablestate = get_target_tablestate(commit_block.get());
    if (commit_tablestate == nullptr) {
        xwarn("xrelay_proposal_maker_t::verify_proposal fail clone tablestate. %s,cert=%s", cs_para.dump().c_str(), proposal_prev_block->dump().c_str());
        return xblockmaker_error_proposal_table_state_clone;
    }

    // get tablestate related to latest cert block
    data::xtablestate_ptr_t tablestate = get_target_tablestate(proposal_prev_block.get());
    if (nullptr == tablestate) {
        xwarn("xrelay_proposal_maker_t::verify_proposal fail clone tablestate. %s,cert=%s", cs_para.dump().c_str(), proposal_prev_block->dump().c_str());
        return xblockmaker_error_proposal_table_state_clone;
    }

    xtablemaker_para_t table_para(tablestate, commit_tablestate);
    if (false == verify_proposal_input(proposal_block, table_para)) {
        xwarn("xrelay_proposal_maker_t::verify_proposal fail-proposal input invalid. proposal=%s", proposal_block->dump().c_str());
        return xblockmaker_error_proposal_bad_input;
    }

    if (false == check_wrap_proposal(proposal_prev_block, proposal_block)) {
        return xblockmaker_error_proposal_bad_input;
    }

    table_para.set_relay_wrap_data(proposal_block->get_header()->get_comments());
    table_para.set_relay_block_data(proposal_block->get_relay_block_data());

    if (false == backup_set_consensus_para(proposal_prev_block.get(), proposal_block, nullptr, cs_para)) {
        xwarn("xproposal_maker_t::verify_proposal fail-backup_set_consensus_para. proposal=%s",
            proposal_block->dump().c_str());
        XMETRICS_GAUGE(metrics::cons_fail_verify_proposal_consensus_para_get, 1);
        XMETRICS_GAUGE(metrics::cons_table_backup_verify_proposal_succ, 0);
        return xblockmaker_error_proposal_bad_consensus_para;
    }

    int32_t verify_ret = m_relay_maker->verify_proposal(proposal_block, table_para, cs_para);
    if (verify_ret != xsuccess) {
        xwarn("xrelay_proposal_maker_t::verify_proposal fail-verify_proposal. proposal=%s,error_code=%s",
              proposal_block->dump().c_str(),
              chainbase::xmodule_error_to_str(verify_ret).c_str());
        XMETRICS_GAUGE(metrics::cons_table_backup_verify_proposal_succ, 0);
        return verify_ret;
    }
    XMETRICS_GAUGE(metrics::cons_table_backup_verify_proposal_succ, 1);
    xdbg_info("xrelay_proposal_maker_t::verify_proposal succ. proposal=%s,latest_cert_block=%s", proposal_block->dump().c_str(), proposal_prev_block->dump().c_str());
    return xsuccess;
}

bool xrelay_proposal_maker_t::verify_proposal_input(base::xvblock_t * proposal_block, xtablemaker_para_t & table_para) {
    return true;
}

std::string xrelay_proposal_maker_t::calc_random_seed(base::xvblock_t* latest_cert_block,uint64_t viewtoken) {
    std::string random_str;
    uint64_t last_block_nonce = latest_cert_block->get_cert()->get_nonce();
    random_str = base::xstring_utl::tostring(last_block_nonce);
    random_str += base::xstring_utl::tostring(viewtoken);
    uint64_t seed = base::xhash64_t::digest(random_str);
    return base::xstring_utl::tostring(seed);
}

bool xrelay_proposal_maker_t::leader_set_consensus_para(base::xvblock_t * latest_cert_block, xblock_consensus_para_t & cs_para) {
    uint64_t now = (uint64_t)base::xtime_utl::gettimeofday();
    cs_para.set_timeofday_s(now);

    uint64_t total_lock_tgas_token = 0;
    uint64_t property_height = 0;
    bool ret = store::xtgas_singleton::get_instance().leader_get_total_lock_tgas_token(cs_para.get_clock(), total_lock_tgas_token, property_height);
    if (!ret) {
        xwarn("xrelay_proposal_maker_t::leader_set_consensus_para fail-leader_get_total_lock_tgas_token. %s", cs_para.dump().c_str());
        return ret;
    }
    std::string random_seed = calc_random_seed(latest_cert_block, cs_para.get_viewtoken());
    cs_para.set_parent_height(latest_cert_block->get_height() + 1);
    cs_para.set_tableblock_consensus_para(0, random_seed, total_lock_tgas_token, property_height);
    xdbg_info("xrelay_proposal_maker_t::leader_set_consensus_para %s random_seed=%s,tgas_token=%" PRIu64 ",tgas_height=%" PRIu64 " leader",
              cs_para.dump().c_str(),
              random_seed.c_str(),
              total_lock_tgas_token,
              property_height);
    return true;
}

bool xrelay_proposal_maker_t::backup_set_consensus_para(base::xvblock_t * latest_cert_block,
                                                        base::xvblock_t * proposal,
                                                        base::xvqcert_t * bind_drand_cert,
                                                        xblock_consensus_para_t & cs_para) {
    uint64_t now = (uint64_t)base::xtime_utl::gettimeofday();
    cs_para.set_timeofday_s(now);

    cs_para.set_parent_height(latest_cert_block->get_height() + 1);
    cs_para.set_common_consensus_para(proposal->get_cert()->get_clock(),
                                      proposal->get_cert()->get_validator(),
                                      proposal->get_cert()->get_auditor(),
                                      proposal->get_cert()->get_viewid(),
                                      proposal->get_cert()->get_viewtoken(),
                                      proposal->get_cert()->get_drand_height());
    return true;
}

NS_END2
