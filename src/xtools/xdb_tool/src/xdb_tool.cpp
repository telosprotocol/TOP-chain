#include "../xdb_tool.h"

#include "xbase/xbase.h"
#include "xblockstore/xblockstore_face.h"
#include "xcodec/xmsgpack_codec.hpp"
#include "xcommon/xip.h"
#include "xdb/xdb_factory.h"
#include "xdata/xnative_contract_address.h"
#include "xdata/xproperty.h"
#include "xdata/xelection/xelection_result_store.h"
#include "xdata/xcodec/xmsgpack/xelection_result_store_codec.hpp"

#include "xvledger/xvledger.h"
#include "xvledger/xvblock.h"


#include <fstream>
#include <sstream>
#include <string>


#define CONS_AUDITOR_COUNT 1
#define CONS_VALIDATOR_COUNT 1
#define NETWORK_ID 0

void xdb_tool::init_xdb_tool(std::string const& db_path) {
    db_path_ = db_path;
    #ifdef DEBUG
        std::cout << "[xdb_tool::init_xdb_tool] init db_path: " << db_path_ << "\n";
    #endif
    store_ = top::store::xstore_factory::create_store_with_kvdb(db_path_);
    top::base::xvchain_t::instance().set_xdbstore(store_.get());
    blockstore_.attach(top::store::get_vblockstore());
}

uint64_t xdb_tool::get_blockheight(std::string const& tableblock_addr) const {
    #ifdef DEBUG
        std::cout << "[xdb_tool::get_blockheight] current db_path: " << db_path_ << "\n";
        std::cout << "[xdb_tool::get_blockheight] tableblock address: " << tableblock_addr << "\n";
    #endif
    // auto height = store_->get_blockchain_height(tableblock_addr);
    auto block = blockstore_->get_latest_committed_block(tableblock_addr);
    auto height = block->get_height();
    std::cout << height << "\n";
    return height;
}

void xdb_tool::get_voteinfo_from_block(std::string const& tableblock_addr, uint64_t start, uint64_t end) {
    #ifdef DEBUG
        std::cout << "[xdb_tool::get_voteinfo_from_block] current db_path: " << db_path_ << "\n";
        std::cout << "[xdb_tool::get_voteinfo_from_block] tableblock address: " << tableblock_addr << ", start height: " << start << ", end height: " << end << "\n";
    #endif

    // this table has no block
    if (end < start) return;

    std::string filename = tableblock_addr + "-" + std::to_string(start) + "-" + std::to_string(end);
    std::fstream file_out(filename, std::ios_base::out | std::ios_base::trunc );
    for (uint64_t i = start; i <= end; ++i) {
        auto vblock = blockstore_->load_block_object(tableblock_addr, i, 0, false);
        xblock_t* block = dynamic_cast<xblock_t*>(vblock.get());
        assert(block != nullptr);
        std::stringstream outstr;
        outstr << block->get_height();

        auto auditor_xip2 = block->get_cert()->get_auditor();
        auto validator_xip2 = block->get_cert()->get_validator();
        bool auditor_leader = true; // default auditor is leader

        if (!is_xip2_empty(auditor_xip2)) {
            if (get_node_id_from_xip2(auditor_xip2) != 0x3ff) {
                auditor_leader = true;
            }

            top::auth::xmutisigdata_t aggregated_sig_obj;
            xassert(aggregated_sig_obj.serialize_from_string(block->get_cert()->get_audit_signature()) > 0);
            outstr << " auditor_vote:" << get_multisig_votestr(aggregated_sig_obj);
            outstr << " " << std::dec << get_network_height_from_xip2(auditor_xip2) << "|"
                << get_group_id_from_xip2(auditor_xip2) << "|"
                << get_group_nodes_count_from_xip2(auditor_xip2);
        }
        if (!is_xip2_empty(validator_xip2)) {
            if (get_node_id_from_xip2(validator_xip2) != 0x3ff) {
                auditor_leader = false;
            }

            top::auth::xmutisigdata_t aggregated_sig_obj;
            xassert(aggregated_sig_obj.serialize_from_string(block->get_cert()->get_verify_signature()) > 0);
            outstr << " validator_vote:" << get_multisig_votestr(aggregated_sig_obj);
            outstr << " " << std::dec << get_network_height_from_xip2(validator_xip2) << "|"
                << get_group_id_from_xip2(validator_xip2) << "|"
                << get_group_nodes_count_from_xip2(validator_xip2);
        }

        if (auditor_leader) {
            outstr << " " << std::hex << "leader_auditor=" << auditor_xip2.high_addr << ":" << auditor_xip2.low_addr;
        } else {
            outstr << " " << std::hex << "leader_validator=" << validator_xip2.high_addr << ":" << validator_xip2.low_addr;
        }

        file_out << outstr.str() << "\n";
        file_out.flush();

    }
}


std::string xdb_tool::cons_electinfo_by_height(uint64_t height, bool print) const {
    std::stringstream outstr;
    outstr << height;

    auto vblock = blockstore_->load_block_object((std::string)top::sys_contract_zec_elect_consensus_addr, height, 0, false);
    outstr << " " << vblock->get_clock();
    top::base::xauto_ptr<top::base::xvbstate_t> bstate = top::base::xvchain_t::instance().get_xstatestore()->get_blkstate_store()->get_block_state(vblock.get());
    if (bstate == nullptr) {
        std::cout << "[ xdb_tool::cons_electinfo_by_height]" << " cannot get cons elect info at height: " << height << ", state is null. \n";
        return "";
    }
    xunit_bstate_t unitstate(bstate.get());

    // process auditor&validator
    for (auto i = 0; i < CONS_AUDITOR_COUNT; ++i) {
        uint8_t auditor_group_id = top::common::xauditor_group_id_value_begin + i;
        outstr << " auditor_" << std::dec << (uint16_t)auditor_group_id << ":";

        std::string property_name = std::string(top::data::XPROPERTY_CONTRACT_ELECTION_RESULT_KEY) + "_" + std::to_string(auditor_group_id);
        std::string result = unitstate.native_string_get(property_name);

        if (result.empty()) {
            std::cout << "[ xdb_tool::cons_electinfo_by_height] auditor groupid " << std::dec << (uint16_t)auditor_group_id << " cannot get native property at height: " << height << "," "\n";
            continue;
        }

        using top::data::election::xelection_result_store_t;
        auto const & election_result_store = top::codec::msgpack_decode<xelection_result_store_t>({std::begin(result), std::end(result)});

        // auditor
        auto & auditor_current_group_nodes = election_result_store.result_of(top::common::xnetwork_id_t{NETWORK_ID})
                                         .result_of(top::common::xnode_type_t::consensus_auditor)
                                         .result_of(top::common::xdefault_cluster_id)
                                         .result_of(top::common::xgroup_id_t{auditor_group_id});

        for (auto const& node_by_slotid: auditor_current_group_nodes.results()) {
            outstr << " " << node_by_slotid.second.node_id().value();
            top::common::xip2_t xip{
                top::common::xnetwork_id_t{NETWORK_ID},
                top::common::xconsensus_zone_id,
                top::common::xdefault_cluster_id,
                top::common::xgroup_id_t{auditor_group_id},
                node_by_slotid.first,
                (uint16_t)auditor_current_group_nodes.size(),
                height
            };
            outstr << "(" << std::hex << xip.raw_high_part() << ":" << xip.raw_low_part() << ")";
        }

        // validator
        for (auto count = 0; count < CONS_VALIDATOR_COUNT; ++count) {
            uint8_t validator_group_id ;
            if (auditor_group_id == 1) {
                validator_group_id = top::common::xvalidator_group_id_value_begin;
            } else if (auditor_group_id == 2) {
                validator_group_id = top::common::xvalidator_group_id_value_begin + 2;
            }

            validator_group_id += count;

            outstr << " validator_" << std::dec << (uint16_t)validator_group_id << ":";
            auto & validator_current_group_nodes = election_result_store.result_of(top::common::xnetwork_id_t{NETWORK_ID})
                                                .result_of(top::common::xnode_type_t::consensus_validator)
                                                .result_of(top::common::xdefault_cluster_id)
                                                .result_of(top::common::xgroup_id_t{validator_group_id});

            for (auto const& node_by_slotid: validator_current_group_nodes.results()) {
                outstr << " " << node_by_slotid.second.node_id().value();
                top::common::xip2_t xip{
                    top::common::xnetwork_id_t{NETWORK_ID},
                    top::common::xconsensus_zone_id,
                    top::common::xdefault_cluster_id,
                    top::common::xgroup_id_t{validator_group_id},
                    node_by_slotid.first,
                    (uint16_t)validator_current_group_nodes.size(),
                    height
                };
                outstr << "(" << std::hex << xip.raw_high_part() << ":" << xip.raw_low_part() << ")";
                }
        }




    }


    if (print)  std::cout << outstr.str() << "\n";
    return outstr.str();


}

void xdb_tool::all_cons_electinfo() const {
    std::string filename = "all_consensus_elect_info";
    std::fstream file_out(filename, std::ios_base::out | std::ios_base::trunc );
    auto block_height = get_blockheight(top::sys_contract_zec_elect_consensus_addr);
    for (uint64_t i = 1; i <= block_height; ++i) {
        auto res = cons_electinfo_by_height(i);
        file_out << res << "\n";
    }
}


std::string xdb_tool::get_multisig_votestr(top::auth::xmutisigdata_t const& aggregated_sig_obj) const {
    std::string res;
    top::auth::xnodebitset& nodebits = aggregated_sig_obj.get_nodebitset();
    for (int i = 0; i < nodebits.get_alloc_bits(); ++i) {
        if (nodebits.is_set(i)) {
            res += "1";
        } else {
            res += "0";
        }
    }

    return res;
}

/**
 * @brief normal credit_data(table vote info and elect info)
 *
 */
void xdb_tool::credit_data(uint64_t table_id) {
    if (table_id != 0) { // specific table by table id
        std::string specific_tableblock_addr =  top::sys_contract_sharding_table_block_addr;
        specific_tableblock_addr += "@" +  std::to_string(table_id);
        auto block_height = get_blockheight(specific_tableblock_addr);
        get_voteinfo_from_block(specific_tableblock_addr, 1, block_height);

    } else { // all table
        for (auto i = 0; i < enum_vbucket_has_tables_count; ++i) {
            std::string specific_tableblock_addr =  top::sys_contract_sharding_table_block_addr;
            specific_tableblock_addr += "@" +  std::to_string(i);
            auto block_height = get_blockheight(specific_tableblock_addr);
            get_voteinfo_from_block(specific_tableblock_addr, 1, block_height);

        }

    }

    std::string filename = "consensus_elect_info";
    std::fstream file_out(filename, std::ios_base::out | std::ios_base::trunc );
    auto block_height = get_blockheight(top::sys_contract_zec_elect_consensus_addr);
    for (uint64_t i = 1; i <= block_height; ++i) {
        auto res = cons_electinfo_by_height(i);
        file_out << res << "\n";
    }

}

void xdb_tool::specific_clockheight(uint64_t start_gmttime, uint64_t end_gmttime) {
    #ifdef DEBUG
        std::cout << "[xdb_tool::specific_clockheight] current db_path: " << db_path_ << "\n";
        std::cout << "[xdb_tool::specific_clockheight] start clock height: " << start_gmttime << ", end clock height: " << end_gmttime << "\n";
    #endif

    for (auto i = 0; i < enum_vbucket_has_tables_count; ++i) {
        std::string specific_tableblock_addr =  top::sys_contract_sharding_table_block_addr;
        specific_tableblock_addr += "@" +  std::to_string(i);
        auto block_height = get_blockheight(specific_tableblock_addr);



        std::string filename = specific_tableblock_addr + "-" + std::to_string(start_gmttime) + "-" + std::to_string(end_gmttime);
        std::fstream file_out(filename, std::ios_base::out | std::ios_base::trunc );
        for (uint64_t i = 1; i <= block_height; ++i) {
            auto vblock = blockstore_->load_block_object((std::string)top::sys_contract_zec_elect_consensus_addr, i, 0, false);
            xblock_t* block = dynamic_cast<xblock_t*>(vblock.get());
            std::stringstream outstr;
            auto gmt_time = block->get_timestamp();
            if (gmt_time >= start_gmttime && gmt_time <= end_gmttime) {
                outstr << gmt_time;

                auto auditor_xip2 = block->get_cert()->get_auditor();
                auto validator_xip2 = block->get_cert()->get_validator();
                bool auditor_leader = true; // default auditor is leader

                if (!is_xip2_empty(auditor_xip2)) {
                    if (get_node_id_from_xip2(auditor_xip2) != 0x3ff) {
                        auditor_leader = true;
                    }

                    top::auth::xmutisigdata_t aggregated_sig_obj;
                    xassert(aggregated_sig_obj.serialize_from_string(block->get_cert()->get_audit_signature()) > 0);
                    outstr << " auditor_vote:" << get_multisig_votestr(aggregated_sig_obj);
                    outstr << " " << std::dec << get_network_height_from_xip2(auditor_xip2) << "|"
                        << get_group_id_from_xip2(auditor_xip2) << "|"
                        << get_group_nodes_count_from_xip2(auditor_xip2);
                }
                if (!is_xip2_empty(validator_xip2)) {
                    if (get_node_id_from_xip2(validator_xip2) != 0x3ff) {
                        auditor_leader = false;
                    }

                    top::auth::xmutisigdata_t aggregated_sig_obj;
                    xassert(aggregated_sig_obj.serialize_from_string(block->get_cert()->get_verify_signature()) > 0);
                    outstr << " validator_vote:" << get_multisig_votestr(aggregated_sig_obj);
                    outstr << " " << std::dec << get_network_height_from_xip2(validator_xip2) << "|"
                        << get_group_id_from_xip2(validator_xip2) << "|"
                        << get_group_nodes_count_from_xip2(validator_xip2);
                }

                if (auditor_leader) {
                    outstr << " " << std::hex << "leader_auditor=" << auditor_xip2.high_addr << ":" << auditor_xip2.low_addr;
                } else {
                    outstr << " " << std::hex << "leader_validator=" << validator_xip2.high_addr << ":" << validator_xip2.low_addr;
                }

                file_out << outstr.str() << "\n";
            }
        }




    }


}

top::data::xstatistics_data_t xdb_tool::get_fulltable_statistic(std::string const& tableblock_addr, uint64_t height) {
        auto vblock = blockstore_->load_block_object(tableblock_addr, height, 0, true);
        xblock_t* block = dynamic_cast<xblock_t*>(vblock.get());

        assert(block->is_fullblock());
        xfull_tableblock_t* full_tableblock = dynamic_cast<xfull_tableblock_t*>(block);

        auto statistic = full_tableblock->get_table_statistics();
        top::base::xstream_t base_stream{top::base::xcontext_t::instance()};
        base_stream << statistic;
        std::cout << "size is: " << base_stream.size() << "\n";
        return statistic;

}

