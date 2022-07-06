// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xblockmaker/xrelay_proposal_maker.h"

#include "xbasic/xhex.h"
#include "xbasic/xmemory.hpp"
#include "xblockmaker/xblock_maker_para.h"
#include "xblockmaker/xblockmaker_error.h"
#include "xblockmaker/xrelay_proposal_maker.h"
#include "xcrypto/xckey.h"
#include "xdata/xblocktool.h"
#include "xdata/xblockbuild.h"
#include "xevm_common/common_data.h"
#include "xevm_common/xevm_transaction_result.h"
#include "xmbus/xevent_behind.h"
#include "xpbase/base/top_utils.h"
#include "xstore/xtgas_singleton.h"
//#include "xdata/xrelay_block_store.h"



#include <fstream>
#include <string>

NS_BEG2(top, blockmaker)

#define RELAY_BLOCK_TXS_PACK_NUM_MAX    (32)

// REG_XMODULE_LOG(chainbase::enum_xmodule_type::xmodule_type_xblockmaker, xblockmaker_error_to_string, xblockmaker_error_base+1, xblockmaker_error_max);

xrelay_proposal_maker_t::xrelay_proposal_maker_t(const std::string & account,
                                                 const xblockmaker_resources_ptr_t & resources,
                                                 const observer_ptr<xrelay_chain::xrelay_chain_mgr_t> & relay_chain_mgr)
  : m_resources(resources), m_relay_chain_mgr(relay_chain_mgr), m_relay_maker(make_object_ptr<xrelay_maker_t>(account, resources)) {
    xdbg("xrelay_proposal_maker_t::xrelay_proposal_maker_t create,this=%p,account=%s ", this, account.c_str());
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

void xrelay_proposal_maker_t::convert_to_xrelay_tx_and_receipts(const std::map<uint64_t, xrelay_chain::xcross_txs_t> & cross_tx_map,
                                                                std::vector<data::xeth_transaction_t> & transactions,
                                                                std::vector<data::xeth_receipt_t> & receipts) {
    for (auto & cross_tx_map_pair : cross_tx_map) {
        auto & cross_txs = cross_tx_map_pair.second;
        for (auto & tx : cross_txs.m_txs) {
            std::error_code ec;
            data::xeth_transaction_t relay_tx = tx->to_eth_tx(ec);
            xinfo("xrelay_proposal_maker_t::convert_to_xrelay_tx_and_receipts tx_info: %s  relay_tx: %ss ", tx->dump().c_str(), relay_tx.dump().c_str());
            transactions.push_back(relay_tx);
        }

        for (auto & tx_result : cross_txs.m_tx_results) {
            data::xeth_receipt_t receipt;
            receipt.set_tx_status(tx_result.get_tx_status());
            receipt.set_cumulative_gas_used(tx_result.get_cumulative_gas_used());
            receipt.set_logs(tx_result.get_logs());
            receipt.create_bloom();
            receipts.push_back(receipt);
        }
    }
}

data::xrelay_block xrelay_proposal_maker_t::build_relay_block(evm_common::h256 prev_hash,
                                                              uint64_t block_height,
                                                              uint64_t timestamp,
                                                              const std::vector<data::xeth_transaction_t> & transactions,
                                                              const std::vector<data::xeth_receipt_t> & receipts,
                                                              const data::xrelay_election_group_t & reley_election_group,
                                                              uint64_t epochid) {
    data::xrelay_block relay_block(prev_hash, block_height, epochid, timestamp);
    relay_block.set_transactions(transactions);
    relay_block.set_receipts(receipts);
    relay_block.set_elections_next(reley_election_group);
    relay_block.build_finish();
#ifdef DEBUG
    reley_election_group.dump();
#endif
    return relay_block;
}


bool xrelay_proposal_maker_t::build_relay_block_data_leader(const data::xblock_ptr_t & latest_wrap_block,
                                                            uint64_t timestamp,
                                                            uint64_t &cur_election_height,
                                                            uint64_t &cur_evm_table_height,
                                                            uint64_t epochid,
                                                            std::string & relay_block_data) {
    data::xrelay_election_group_t reley_election_group;

    //check election change
    auto ret = m_relay_chain_mgr->get_elect_cache(cur_election_height + 1, reley_election_group.elections_vector);
    if (ret) {
        cur_election_height++;
        reley_election_group.election_epochID = cur_election_height;
    }

    //get last block info
    data::xrelay_block last_relay_block;
    if (latest_wrap_block->get_height() == 0) {
        last_relay_block = data::xrootblock_t::get_genesis_relay_block();
    } else {
        std::error_code ec;
        data::xblockextract_t::unpack_relayblock(latest_wrap_block.get(), false, last_relay_block, ec);
        if (ec) {
            xwarn("xrelay_proposal_maker_t:build_relay_block_data_leader decodeBytes error %s; err msg %s", ec.category().name(), ec.message().c_str());
            return false;
        }
    }
     
    data::xrelay_block new_relay_block;
    //create election block
    if (!reley_election_group.empty()) {
        m_last_poly_timestamp = 0;
        new_relay_block = data::xrelay_block(last_relay_block.get_block_hash(), last_relay_block.get_block_height() + 1,
                                             epochid, timestamp, reley_election_group);

    } else {
        if (m_last_poly_timestamp == 0) {
            m_last_poly_timestamp = timestamp + XGET_ONCHAIN_GOVERNANCE_PARAMETER(max_relay_poly_interval) * 10;
        } 

        if((timestamp > m_last_poly_timestamp) && last_relay_block.get_all_receipts().size() > 0) {

            xinfo("xrelay_proposal_maker_t::build_relay_block_data_leader time last_poly_timestamp(%ld), timestamp(%ld)",
                m_last_poly_timestamp, timestamp);

            m_last_poly_timestamp +=  XGET_ONCHAIN_GOVERNANCE_PARAMETER(max_relay_poly_interval) * 10;
            new_relay_block = data::xrelay_block(last_relay_block.get_block_hash(), last_relay_block.get_block_height() + 1,
                                                epochid, timestamp);
        } else {
            std::map<uint64_t, xrelay_chain::xcross_txs_t> cross_tx_map;
            std::vector<data::xeth_transaction_t> transactions;
            std::vector<data::xeth_receipt_t> receipts;

            auto result = m_relay_chain_mgr->get_tx_cache_leader(cur_evm_table_height, cur_evm_table_height, cross_tx_map, RELAY_BLOCK_TXS_PACK_NUM_MAX);
            if (!result || cross_tx_map.empty()) {
                return false;
            }
            convert_to_xrelay_tx_and_receipts(cross_tx_map, transactions, receipts);
            new_relay_block = data::xrelay_block(last_relay_block.get_block_hash(),  last_relay_block.get_block_height() + 1, 
                                                epochid, timestamp, transactions, receipts);
        }
    }

    new_relay_block.build_finish();
    xdbg("xrelay_proposal_maker_t::build_relay_block_data_leader relay_block:%s, new evm height:%llu", new_relay_block.dump().c_str(), cur_evm_table_height);
    xbytes_t rlp_stream = new_relay_block.encodeBytes();
    relay_block_data = from_bytes<std::string>((xbytes_t)(rlp_stream));
    return true;
}

bool xrelay_proposal_maker_t::build_relay_block_data_backup(evm_common::h256 prev_hash,
                                                            uint64_t block_height,
                                                            uint64_t last_evm_table_height,
                                                            uint64_t new_evm_table_height,
                                                            uint64_t timestamp,
                                                            uint64_t new_election_height,
                                                            uint64_t epochid,
                                                            std::string & relay_block_data,
                                                            bool tx_empty) {
    std::vector<data::xeth_transaction_t> transactions;
    std::vector<data::xeth_receipt_t> receipts;

    if (last_evm_table_height < new_evm_table_height) {
        if (!tx_empty) {
            std::map<uint64_t, xrelay_chain::xcross_txs_t> cross_tx_map;
            auto ret = m_relay_chain_mgr->get_tx_cache_backup(last_evm_table_height, new_evm_table_height, cross_tx_map);
            if (!ret) {
                return false;
            }
            convert_to_xrelay_tx_and_receipts(cross_tx_map, transactions, receipts);
        } else {
            last_evm_table_height = new_evm_table_height;
        }
    } else {
        if (last_evm_table_height != new_evm_table_height) {
            xerror("xrelay_proposal_maker_t::build_relay_block_data_backup evm height wrong:%llu:%llu.", last_evm_table_height, new_evm_table_height);
            return false;
        }
        xinfo("xrelay_proposal_maker_t::build_relay_block_data_backup not pack tx.");
    }

    std::vector<data::xrelay_election_node_t> reley_election;

    if (new_election_height != 0) {
        auto ret = m_relay_chain_mgr->get_elect_cache(new_election_height, reley_election);
        if (!ret) {
            xinfo("xrelay_proposal_maker_t::build_relay_block_data_backup. get elect cache fail. height:%llu", new_election_height);
            return false;
        }
    }

    data::xrelay_election_group_t reley_election_group;
    reley_election_group.election_epochID = new_election_height;
    reley_election_group.elections_vector = reley_election;

    data::xrelay_block relay_block = build_relay_block(prev_hash, block_height, timestamp, transactions, receipts, reley_election_group, epochid);

    xdbg("xrelay_proposal_maker_t::build_relay_block_data_backup relay_block:%s,last evm height:%llu,new evm height:%llu", relay_block.dump().c_str(), last_evm_table_height, new_evm_table_height);

    xbytes_t rlp_stream = relay_block.encodeBytes();
    relay_block_data = from_bytes<std::string>((xbytes_t)(rlp_stream));
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

    // wrap phase, evm table height, elect height store into wrap block header comments.
    uint8_t last_wrap_phase;
    uint64_t last_evm_height;
    uint64_t last_elect_height;
    data::xtableheader_extra_t last_header_extra;
    if (latest_cert_block->is_genesis_block()) {
        last_wrap_phase = 2;
        last_evm_height = 0;
        last_elect_height = 0;
    } else {
        auto extra_str = latest_cert_block->get_header()->get_extra_data();
        auto ret = last_header_extra.deserialize_from_string(extra_str);
        if (ret <= 0) {
            xerror("xrelay_proposal_maker_t::make_proposal header extra data deserialize fail.");
            return nullptr;
        }
        auto last_wrap_data = last_header_extra.get_relay_wrap_info();
        if (last_wrap_data.empty()) {
            xerror("xrelay_proposal_maker_t::make_proposal wrap data should not empty.");
            return nullptr;
        }

        data::xrelay_wrap_info_t last_wrap_info;
        last_wrap_info.serialize_from_string(last_wrap_data);
        last_wrap_phase = last_wrap_info.get_wrap_phase();
        last_evm_height = last_wrap_info.get_evm_height();
        last_elect_height = last_wrap_info.get_elect_height();
    }

    base::xstream_t _stream(base::xcontext_t::instance());
    // wrap phase: 0, 1, 2
    uint8_t wrap_phase = last_wrap_phase + 1;

    std::string relay_block_data;
    if (wrap_phase > 2) {
        wrap_phase = 0;  
        auto ret = build_relay_block_data_leader(  
            latest_cert_block, proposal_para.get_timestamp(), last_elect_height, last_evm_height,  proposal_para.get_election_round(), relay_block_data);
        if (!ret) {
            xinfo("xrelay_proposal_maker_t::make_proposal fail-no tx for pack.%s", proposal_para.dump().c_str());
            return nullptr;
        }
    } else {
      //  evm_height = last_evm_height;
       // elect_height = last_elect_height;
        auto last_relay_block_data = last_header_extra.get_relay_block_data();
        xassert(!last_relay_block_data.empty());

        std::error_code ec;
        data::xrelay_block last_relay_block;
        last_relay_block.decodeBytes(to_bytes(last_relay_block_data), ec);
        if (ec) {
            xwarn("xrelay_proposal_maker_t:make_proposal last_relay_block decodeBytes error %s; err msg %s", ec.category().name(), ec.message().c_str());
            return nullptr;
        }

        if (proposal_para.get_election_round() < last_relay_block.get_inner_header().get_epochID()) {
            xwarn("xrelay_proposal_maker_t:make_proposal proposal epoch id fallbehind.relay_block:%s,proposal elect round:%lu", last_relay_block.dump().c_str(), proposal_para.get_election_round());
            return nullptr;
        }

        if (last_relay_block.get_inner_header().get_epochID() != proposal_para.get_election_round()) {
            if (last_relay_block.get_inner_header().get_epochID() + 1 != proposal_para.get_election_round()) {
                xerror("xrelay_proposal_maker_t:make_proposal epochid exception.relay_block:%s,new round:%lu", last_relay_block.dump().c_str(), proposal_para.get_election_round());
                return nullptr;
            }
            xwarn("xrelay_proposal_maker_t:make_proposal epochid changed, should consensus from phase 0 again.relay_block:%s,new round:%lu",
                  last_relay_block.dump().c_str(),
                  proposal_para.get_election_round());
            last_relay_block.get_header().set_epochid(proposal_para.get_election_round());
            last_relay_block.build_finish();
            xbytes_t rlp_stream = last_relay_block.encodeBytes();
            relay_block_data = from_bytes<std::string>((xbytes_t)(rlp_stream));
            // wrap_phase = 0;
        } else {
            relay_block_data = last_relay_block_data;
        }
    }

    std::string wrap_data = data::xrelay_wrap_info_t::build_relay_wrap_info_string(wrap_phase, last_evm_height, last_elect_height);

    data::xtableheader_extra_t header_extra;
    header_extra.set_relay_wrap_info(wrap_data);
    header_extra.set_relay_block_data(relay_block_data);
    std::string extra_data;
    header_extra.serialize_to_string(extra_data);
    table_para.set_relay_extra_data(extra_data);
    table_para.set_need_relay_prove(wrap_phase == 2);

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

    if (wrap_phase == 2) {
        std::error_code ec;
        data::xrelay_block proposal_relay_block;
        proposal_relay_block.decodeBytes(to_bytes(relay_block_data), ec);
        if (ec) {
            xerror("xrelay_proposal_maker_t:make_proposal proposal_relay_block decodeBytes error %s; err msg %s", ec.category().name(), ec.message().c_str());
            return nullptr;
        }
        auto hash = proposal_relay_block.get_block_hash();  // todo(nathan):inner header may not enough.
        uint256_t hash256 = from_bytes<uint256_t>(hash.to_bytes());
        proposal_block->set_vote_extend_hash(hash256);
    }

    bool bret = proposal_block->reset_prev_block(latest_cert_block.get());
    xassert(bret);

    xinfo("xrelay_proposal_maker_t::make_proposal succ.proposal_block=%s", proposal_block->dump().c_str());
    return proposal_block;
}

bool xrelay_proposal_maker_t::check_wrap_proposal(const xblock_ptr_t & latest_cert_block, base::xvblock_t * proposal_block, uint8_t & wrap_phase, uint256_t & relay_hash) {
    uint8_t last_wrap_phase;
    uint64_t last_evm_height;
    uint64_t last_elect_height;
    std::string last_relay_block_data;
    data::xtableheader_extra_t last_header_extra;

    if (latest_cert_block->is_genesis_block()) {
        last_wrap_phase = 2;
        last_evm_height = 0;
        last_elect_height = 0;
    } else {
        auto extra_str = latest_cert_block->get_header()->get_extra_data();
        auto ret = last_header_extra.deserialize_from_string(extra_str);
        if (ret <= 0) {
            xerror("xrelay_proposal_maker_t::check_wrap_proposal header extra data deserialize fail.");
            return false;
        }
        auto last_wrap_data = last_header_extra.get_relay_wrap_info();
        if (last_wrap_data.empty()) {
            xerror("xrelay_proposal_maker_t::check_wrap_proposal wrap data should not empty.proposal_block=%s", proposal_block->dump().c_str());
            return false;
        }

        last_relay_block_data = last_header_extra.get_relay_block_data();

        data::xrelay_wrap_info_t last_wrap_info;
        last_wrap_info.serialize_from_string(last_wrap_data);
        last_wrap_phase = last_wrap_info.get_wrap_phase();
        last_evm_height = last_wrap_info.get_evm_height();
        last_elect_height = last_wrap_info.get_elect_height();
    }

    auto proposal_extra_str = proposal_block->get_header()->get_extra_data();
    data::xtableheader_extra_t proposal_header_extra;
    auto ret = proposal_header_extra.deserialize_from_string(proposal_extra_str);
    if (ret <= 0) {
        xerror("xrelay_proposal_maker_t::check_wrap_proposal header extra data deserialize fail.");
        return false;
    }

    auto proposal_wrap_data = proposal_header_extra.get_relay_wrap_info();
    if (proposal_wrap_data.empty()) {
        xerror("xrelay_proposal_maker_t::check_wrap_proposal wrap data should not empty.proposal_block=%s", proposal_block->dump().c_str());
        return false;
    }

    data::xrelay_wrap_info_t proposal_wrap_info;
    proposal_wrap_info.serialize_from_string(proposal_wrap_data);
    uint8_t wrap_phase_tmp = proposal_wrap_info.get_wrap_phase();
    uint64_t evm_height = proposal_wrap_info.get_evm_height();
    uint64_t elect_height = proposal_wrap_info.get_elect_height();


    auto proposal_relay_block_data = proposal_header_extra.get_relay_block_data();

    std::string relay_block_data;
    bool epochid_changed = false;
    if (wrap_phase_tmp == 0) {
        std::error_code ec;
        data::xrelay_block proposal_relay_block;
        proposal_relay_block.decodeBytes(to_bytes(proposal_relay_block_data), ec);
        if (ec) {
            xwarn("xrelay_proposal_maker_t:check_wrap_proposal proposal_relay_block decodeBytes error %s; err msg %s", ec.category().name(), ec.message().c_str());
            return false;
        }
        //todo,rank  
        if (!proposal_relay_block.get_header().get_elections_sets().empty() && !proposal_relay_block.get_all_receipts().empty()) {
             xerror("xrelay_proposal_maker_t:check_wrap_proposal proposal_relay_block election set(%d) and receipts(%d) both empty or not empty",
                   proposal_relay_block.get_header().get_elections_sets().size(),  proposal_relay_block.get_all_receipts().size());
            return false;
        }
    
        evm_common::h256 local_prev_hash;
        uint64_t local_block_height = 0;
        if (proposal_block->get_height() == 1) {
            auto genesis_block = data::xrootblock_t::get_genesis_relay_block();
            local_prev_hash = genesis_block.get_block_hash();
            local_block_height = genesis_block.get_block_height() + 1;
        } else {
            data::xrelay_block last_relay_block;
            last_relay_block.decodeBytes(to_bytes(last_relay_block_data), ec);
            if (ec) {
                xwarn("xrelay_proposal_maker_t:check_wrap_proposal last_relay_block decodeBytes error %s; err msg %s", ec.category().name(), ec.message().c_str());
                return false;
            }
            local_prev_hash = last_relay_block.get_block_hash();
            local_block_height = last_relay_block.get_block_height() + 1;
            uint64_t proposal_epoch_id = proposal_relay_block.get_header().get_epochid();
            uint64_t last_epoch_id = last_relay_block.get_header().get_epochid();
            epochid_changed = (proposal_epoch_id != last_epoch_id);
        }

        if (proposal_relay_block.get_header().get_prev_block_hash() != local_prev_hash || proposal_relay_block.get_block_height() != local_block_height) {
            xerror("xrelay_proposal_maker_t::check_wrap_proposal relay block pre hash or height not match.pre hash:%s:%s,height:%llu:%llu.proposal_block=%s",
                   proposal_relay_block.get_header().get_prev_block_hash().hex().c_str(),
                   local_prev_hash.hex().c_str(),
                   proposal_relay_block.get_block_height(),
                   local_block_height,
                   proposal_block->dump().c_str());
            return false;
        }

        bool election_empty = proposal_relay_block.get_header().get_elections_sets().empty();
        bool tx_empty = proposal_relay_block.get_all_transactions().empty();
        if (!election_empty && !tx_empty) {
            xerror("xrelay_proposal_maker_t::check_wrap_proposal both election and tx are empty or not empty.relay_block:%s", proposal_relay_block.dump().c_str());
            return false;
        }
        
        xdbg("xrelay_proposal_maker_t::check_wrap_proposal relay_block:%s", proposal_relay_block.dump().c_str());

        auto ret = build_relay_block_data_backup(local_prev_hash,
                                                 local_block_height,
                                                 last_evm_height,
                                                 evm_height,
                                                 proposal_relay_block.get_timestamp(),
                                                 (proposal_relay_block.get_header().get_elections_sets().size() == 0) ? 0 : elect_height,
                                                 proposal_relay_block.get_inner_header().get_epochID(),
                                                 relay_block_data, tx_empty);
        if (!ret) {
            xwarn("xrelay_proposal_maker_t::check_wrap_proposal build relay block data fail.proposal_block=%s", proposal_block->dump().c_str());
            return false;
        }
    } else {
        // todo(nathan):check if epoch id is changed.
        relay_block_data = proposal_relay_block_data;
        // relay_block_data = last_relay_block_data;
    }

    if ((epochid_changed && wrap_phase_tmp != 0) || (!epochid_changed && wrap_phase_tmp != ((last_wrap_phase + 1) % 3)) ||
        (wrap_phase_tmp != 0 && (evm_height != last_evm_height || elect_height != last_elect_height))) {
        xerror(
            "xrelay_proposal_maker_t::check_wrap_proposal wrap data not "
            "match.wrap_phase:%d:%d,evm_height:%llu:%llu,elect_height:%llu:%llu,proposal_block=%s",
            wrap_phase_tmp,
            last_wrap_phase,
            evm_height,
            last_evm_height,
            elect_height,
            last_elect_height,
            proposal_block->dump().c_str());
        return false;
    }

    // check relay_block_data
    if (relay_block_data != proposal_relay_block_data) {
        xerror("xrelay_proposal_maker_t::check_wrap_proposal relay block data not match.proposal_block=%s", proposal_block->dump().c_str());
        return false;
    }

    wrap_phase = wrap_phase_tmp;
    if (wrap_phase == 2) {
        std::error_code ec;
        data::xrelay_block proposal_relay_block;
        proposal_relay_block.decodeBytes(to_bytes(relay_block_data), ec);
        if (ec) {
            xerror("xrelay_proposal_maker_t:check_wrap_proposal proposal_relay_block decodeBytes error %s; err msg %s", ec.category().name(), ec.message().c_str());
            return false;
        }
        auto hash = proposal_relay_block.get_block_hash();  // todo(nathan):inner header may not enough.
        relay_hash = from_bytes<uint256_t>(hash.to_bytes());
    }

    return true;
}

int xrelay_proposal_maker_t::verify_proposal(base::xvblock_t * proposal_block, base::xvqcert_t * bind_clock_cert) {
    xdbg("xrelay_proposal_maker_t::verify_proposal enter. proposal=%s", proposal_block->dump().c_str());
    // uint64_t gmtime = proposal_block->get_second_level_gmtime();
    xblock_consensus_para_t cs_para(get_account(), proposal_block->get_clock(), proposal_block->get_viewid(), proposal_block->get_viewtoken(), proposal_block->get_height(), 0);

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

    uint8_t wrap_phase;
    uint256_t relay_hash;
    if (false == check_wrap_proposal(proposal_prev_block, proposal_block, wrap_phase, relay_hash)) {
        return xblockmaker_error_proposal_bad_input;
    }

    table_para.set_relay_extra_data(proposal_block->get_header()->get_extra_data());
    table_para.set_need_relay_prove(wrap_phase == 2);

    if (false == backup_set_consensus_para(proposal_prev_block.get(), proposal_block, nullptr, cs_para)) {
        xwarn("xproposal_maker_t::verify_proposal fail-backup_set_consensus_para. proposal=%s", proposal_block->dump().c_str());
        XMETRICS_GAUGE(metrics::cons_fail_verify_proposal_consensus_para_get, 1);
        XMETRICS_GAUGE(metrics::cons_table_backup_verify_proposal_succ, 0);
        return xblockmaker_error_proposal_bad_consensus_para;
    }

    if (wrap_phase == 2) {
        proposal_block->set_vote_extend_hash(relay_hash);
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

std::string xrelay_proposal_maker_t::calc_random_seed(base::xvblock_t * latest_cert_block, uint64_t viewtoken) {
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
