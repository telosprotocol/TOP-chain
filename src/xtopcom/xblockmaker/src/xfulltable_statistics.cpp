#include "xblockmaker/xfulltable_statistics.h"
#include "xcertauth/xcertauth_face.h"
#include "xcertauth/src/xsigndata.h"
#include "xvm/manager/xcontract_manager.h"

#include "xvledger/xvblock.h"
// TODO(jimmy) #include "xbase/xvledger.h"
#include "xbase/xobject_ptr.h"
#include "xdata/xblocktool.h"
#include "xdata/xdatautil.h"

namespace top {
namespace blockmaker {

using top::auth::xmutisigdata_t;
using top::auth::xnodebitset;

static void calc_workload_data(const xvip2_t leader_xip, const uint32_t txs_count, data::xstatistics_data_t & data) {
    // height
    uint64_t block_height = get_network_height_from_xip2(leader_xip);
    auto it_height = data.detail.find(block_height);
    if (it_height == data.detail.end()) {
        data::xelection_related_statistics_data_t election_related_data;
        std::pair<std::map<uint64_t, data::xelection_related_statistics_data_t>::iterator, bool> ret = data.detail.insert(std::make_pair(block_height, election_related_data));
        it_height = ret.first;
    }
    // gid
    auto group_addr = common::xgroup_address_t{ common::xip_t{leader_xip.low_addr} };
    // common::xgroup_id_t group_id = common::xgroup_id_t{group_idx};
    auto it_group = it_height->second.group_statistics_data.find(group_addr);
    if (it_group == it_height->second.group_statistics_data.end()) {
        data::xgroup_related_statistics_data_t group_related_data;
        auto ret = it_height->second.group_statistics_data.insert(std::make_pair(group_addr, group_related_data));
        it_group = ret.first;
    }
    // nid
    uint16_t slot_idx = uint16_t(get_node_id_from_xip2(leader_xip));
    // common::xslot_id_t slot_id = common::xslot_id_t{slot_idx};
    if(it_group->second.account_statistics_data.size() < size_t(slot_idx+1)){
        it_group->second.account_statistics_data.resize(slot_idx+1);
    }
    // workload
    it_group->second.account_statistics_data[slot_idx].block_data.block_count++;
    it_group->second.account_statistics_data[slot_idx].block_data.transaction_count += txs_count;
}

static void calc_consensus_vote_data(xvip2_t const & vote_xip,
                                     xobject_ptr_t<data::xblock_t> const & block,
                                     std::string const & aggregated_signatures_bin,
                                     data::xstatistics_data_t & data) {
    // height
    uint64_t block_height = get_network_height_from_xip2(vote_xip);
    auto it_height = data.detail.find(block_height);
    if (it_height == data.detail.end()) {
        data::xelection_related_statistics_data_t election_related_data;
        std::pair<std::map<uint64_t, data::xelection_related_statistics_data_t>::iterator, bool> ret = data.detail.insert(std::make_pair(block_height, election_related_data));
        it_height = ret.first;
    }

    // gid
    auto group_addr = common::xgroup_address_t{ common::xip_t{vote_xip.low_addr} };
    // common::xgroup_id_t group_id = common::xgroup_id_t{group_idx};
    auto it_group = it_height->second.group_statistics_data.find(group_addr);
    if (it_group == it_height->second.group_statistics_data.end()) {
        data::xgroup_related_statistics_data_t group_related_data;
        auto ret = it_height->second.group_statistics_data.insert(std::make_pair(group_addr, group_related_data));
        it_group = ret.first;
    }

    xassert(!aggregated_signatures_bin.empty());

    xmutisigdata_t aggregated_sig_obj;
    xassert(aggregated_sig_obj.serialize_from_string(aggregated_signatures_bin) > 0);

    xnodebitset & nodebits = aggregated_sig_obj.get_nodebitset();
    if(it_group->second.account_statistics_data.size() < (uint32_t)nodebits.get_alloc_bits()){
        it_group->second.account_statistics_data.resize(nodebits.get_alloc_bits());
    }
    for (int i = 0; i < nodebits.get_alloc_bits(); ++i) {
        it_group->second.account_statistics_data[i].vote_data.block_count++;
        if (nodebits.is_set(i)) {
            it_group->second.account_statistics_data[i].vote_data.vote_count++;
        }
    }
}

static void process_vote_info(xobject_ptr_t<data::xblock_t> const & block, data::xstatistics_data_t & data) {
    auto auditor_xip = block->get_cert()->get_auditor();
    auto validator_xip = block->get_cert()->get_validator();

    if (!is_xip2_empty(auditor_xip)) {//block has auditor info
        calc_consensus_vote_data(auditor_xip, block, block->get_cert()->get_audit_signature(), data);
    }

    if (!is_xip2_empty(validator_xip)) {
        calc_consensus_vote_data(validator_xip, block, block->get_cert()->get_verify_signature(), data);
    }
}

data::xstatistics_data_t tableblock_statistics(std::vector<xobject_ptr_t<data::xblock_t>> const & blks) {
    data::xstatistics_data_t data;
    xdbg("[tableblock_statistics] blks size: %u", blks.size());
    for (size_t i = 0; i < blks.size(); i++) {
        if (nullptr == blks[i]) {
            xerror("[tableblock_statistics] blks[%u] null", i);
            continue;
        }

        uint32_t txs_count = data::xblockextract_t::get_txactions_count(blks[i].get());

        auto leader_xip = blks[i]->get_cert()->get_validator();
        if (get_node_id_from_xip2(leader_xip) == 0x3FF) {
            leader_xip = blks[i]->get_cert()->get_auditor();
            xassert(!blks[i]->get_cert()->get_audit_signature().empty());
        }

        calc_workload_data(leader_xip, txs_count, data);
        process_vote_info(blks[i], data);
    }

    return data;
}

static void calc_consortium_data(const xvip2_t leader_xip, const uint64_t burn_gas, data::xstatistics_cons_data_t & data) {
    // height
    uint64_t block_height = get_network_height_from_xip2(leader_xip);
    auto it_height = data.detail.find(block_height);
    if (it_height == data.detail.end()) {
        data::xelection_statistics_cons_data_t election_related_data;
        std::pair<std::map<uint64_t, data::xelection_statistics_cons_data_t>::iterator, bool> ret = data.detail.insert(std::make_pair(block_height, election_related_data));
        it_height = ret.first;
    }
    // gid
    auto group_addr = common::xgroup_address_t{ common::xip_t{leader_xip.low_addr} };
    auto it_group = it_height->second.group_statistics_data.find(group_addr);
    if (it_group == it_height->second.group_statistics_data.end()) {
        data::xgroup_statistics_cons_data_t group_related_data;
        auto ret = it_height->second.group_statistics_data.insert(std::make_pair(group_addr, group_related_data));
        it_group = ret.first;
    }
    // nid
    uint16_t slot_idx = uint16_t(get_node_id_from_xip2(leader_xip));
    // common::xslot_id_t slot_id = common::xslot_id_t{slot_idx};
    if(it_group->second.account_statistics_data.size() < size_t(slot_idx+1)){
        it_group->second.account_statistics_data.resize(slot_idx+1);
    }
    // reward
    it_group->second.account_statistics_data[slot_idx].burn_gas_value += burn_gas;
    xdbg("[calc_consortium_data] blks height %u xip %s burn_gas %lu total_gas %lu ", block_height, 
      data::xdatautil::xip_to_hex(leader_xip).c_str(),  burn_gas, it_group->second.account_statistics_data[slot_idx].burn_gas_value );
}

data::xstatistics_cons_data_t tableblock_statistics_consortium(std::vector<xobject_ptr_t<data::xblock_t>> const& blks)
{
    data::xstatistics_cons_data_t data;
    xdbg("[tableblock_statistics_consortium] blks size: %u", blks.size());
    for (size_t i = 0; i < blks.size(); i++) {
        if (nullptr == blks[i]) {
            xerror("[tableblock_statistics] blks[%u] null", i);
            continue;
        }
        uint64_t blk_burn_gas = 0;

        auto leader_xip = blks[i]->get_cert()->get_validator();
        if (get_node_id_from_xip2(leader_xip) == 0x3FF) {
            leader_xip = blks[i]->get_cert()->get_auditor();
            xassert(!blks[i]->get_cert()->get_audit_signature().empty());
        }

        if (!blks[i]->get_header()->get_extra_data().empty()) {
            const std::string& extra_data = blks[i]->get_header()->get_extra_data();
            data::xtableheader_extra_t blockheader_extradata;
            int32_t ret = blockheader_extradata.deserialize_from_string(extra_data);
            if (ret <= 0) {
                xerror("tableblock_statistics::get_table_header fail-extra data invalid");
            }

            blk_burn_gas = blockheader_extradata.get_total_burn_gas();
        }
        if (blk_burn_gas == 0) {
            continue;
        }

        calc_consortium_data(leader_xip, blk_burn_gas, data);
    }

    return data;
}
}
}
