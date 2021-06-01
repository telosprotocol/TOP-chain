#include "../xdb_tool.h"

#include "xbase/xbase.h"
#include "xcodec/xmsgpack_codec.hpp"
#include "xcommon/xip.h"
#include "xdb/xdb_factory.h"
#include "xdata/xnative_contract_address.h"
#include "xdata/xproperty.h"
#include "xdata/xelection/xelection_result_store.h"
#include "xdata/xcodec/xmsgpack/xelection_result_store_codec.hpp"


#include <fstream>
#include <sstream>
#include <string>


#define CONS_AUDITOR_COUNT 2
#define CONS_VALIDATOR_COUNT 4

void xdb_tool::init_xdb_tool(std::string const& db_path) {
    db_path_ = db_path;
    #ifdef DEBUG
        std::cout << "[xdb_tool::init_xdb_tool] init db_path: " << db_path_ << "\n";
    #endif
    store_ = top::store::xstore_factory::create_store_with_kvdb(db_path_);
}

uint64_t xdb_tool::get_blockheight(std::string const& tableblock_addr) const {
    #ifdef DEBUG
        std::cout << "[xdb_tool::get_blockheight] current db_path: " << db_path_ << "\n";
        std::cout << "[xdb_tool::get_blockheight] tableblock address: " << tableblock_addr << "\n";
    #endif
    auto height = store_->get_blockchain_height(tableblock_addr);
    std::cout << height << "\n";
    return height;
}

void xdb_tool::get_voteinfo_from_block(std::string const& tableblock_addr, uint64_t start, uint64_t end) {
    #ifdef DEBUG
        std::cout << "[xdb_tool::get_voteinfo_from_block] current db_path: " << db_path_ << "\n";
        std::cout << "[xdb_tool::get_voteinfo_from_block] tableblock address: " << tableblock_addr << ", start height: " << start << ", end height: " << end << "\n";
    #endif

    std::string filename = tableblock_addr + "-" + std::to_string(start) + "-" + std::to_string(end);
    std::fstream file_out(filename, std::ios_base::out | std::ios_base::trunc );
    for (uint64_t i = start; i <= end; ++i) {
        top::base::xauto_ptr<xblock_t> block = store_->get_block_by_height(tableblock_addr, i);
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

    }
}


std::string xdb_tool::cons_electinfo_by_height(uint64_t height) const {
    std::stringstream outstr;
    outstr << height;

    top::base::xauto_ptr<xblock_t> block = store_->get_block_by_height(top::sys_contract_zec_elect_consensus_addr, height);
    xnative_property_t const& native_property = block->get_native_property();
    // process auditor&validator
    for (auto i = 0; i < CONS_AUDITOR_COUNT; ++i) {
        uint8_t auditor_group_id = top::common::xauditor_group_id_value_begin + i;
        outstr << " auditor_" << (uint16_t)auditor_group_id << ":";
        std::string result;
        std::string property_name = std::string(top::data::XPROPERTY_CONTRACT_ELECTION_RESULT_KEY) + "_" + std::to_string(auditor_group_id);

        if (native_property.native_string_get(property_name, result) || result.empty()) {
            std::cout << "[ xdb_tool::cons_electinfo_by_height] auditor groupid " << std::dec << (uint16_t)auditor_group_id << " cannot get native property at height: " << height << "," "\n";
            continue;
        }
        using top::data::election::xelection_result_store_t;
        auto const & election_result_store = top::codec::msgpack_decode<xelection_result_store_t>({std::begin(result), std::end(result)});

        // auditor
        auto & auditor_current_group_nodes = election_result_store.result_of(top::common::xnetwork_id_t{0})
                                         .result_of(top::common::xnode_type_t::consensus_auditor)
                                         .result_of(top::common::xdefault_cluster_id)
                                         .result_of(top::common::xgroup_id_t{auditor_group_id});

        for (auto const& node_by_slotid: auditor_current_group_nodes.results()) {
            outstr << " " << node_by_slotid.second.node_id().value();
        }

        // validator
        for (auto count = 0; count < 2; ++count) {
            uint8_t validator_group_id ;
            if (auditor_group_id == 1) {
                validator_group_id = top::common::xvalidator_group_id_value_begin;
            } else if (auditor_group_id == 2) {
                validator_group_id = top::common::xvalidator_group_id_value_begin + 2;
            }

            validator_group_id += count;

            outstr << " validator_" << (uint16_t)validator_group_id << ":";
            auto & validator_current_group_nodes = election_result_store.result_of(top::common::xnetwork_id_t{0})
                                                .result_of(top::common::xnode_type_t::consensus_validator)
                                                .result_of(top::common::xdefault_cluster_id)
                                                .result_of(top::common::xgroup_id_t{validator_group_id});

            for (auto const& node_by_slotid: validator_current_group_nodes.results()) {
                outstr << " " << node_by_slotid.second.node_id().value();
            }
        }




    }


    // std::cout << outstr.str() << "\n";
    return outstr.str();


}

// void xdb_tool::get_electinfo_by_height(std::string const& elect_addr, uint64_t height) const {


// }


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
            top::base::xauto_ptr<xblock_t> block = store_->get_block_by_height(specific_tableblock_addr, i);
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
