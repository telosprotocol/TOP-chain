// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#include "xelection/xstore/xelection_data_accessor.h"

#include "xbasic/xutility.h"
#include "xcodec/xmsgpack_codec.hpp"
#include "xconfig/xconfig_register.h"
#include "xconfig/xpredefined_configurations.h"
#include "xdata/xcodec/xmsgpack/xelection_result_store_codec.hpp"
#include "xdata/xnative_contract_address.h"
#include "xelection/xdata_accessor_error.h"

#include <inttypes.h>
NS_BEG3(top, election, store)
using top::data::xblock_ptr_t;

xtop_election_data_accessor::xtop_election_data_accessor(const observer_ptr<top::store::xstore_face_t> & store) : m_store(store), m_cache(64) {}

#if 0
// todo charles
// need refactor
data::election::xelection_result_store_t xtop_election_data_accessor::get_election_result_store(const std::string & elect_addr, const uint64_t block_height, std::error_code & ec) {
    if (!block_height) {
        ec = xdata_accessor_errc_t::block_height_error;
        xwarn("block height error. block owner: %s, %" PRIu64, elect_addr.c_str(), block_height);
        return {};
    }
    base::xauto_ptr<xblock_t> block(m_store->get_block_by_height(elect_addr, block_height));
    if (block == nullptr) {
        ec = xdata_accessor_errc_t::block_is_empty;
        xwarn("block is empty. block owner: %s, height: %" PRIu64, elect_addr.c_str(), block_height);
        return {};
    }
    std::string result;
    if (block->get_native_property().native_string_get(data::XPROPERTY_CONTRACT_ELECTION_RESULT_KEY, result) || result.empty()) {
        ec = xdata_accessor_errc_t::election_result_is_empty;
        xwarn("election_result is empty. block owner: %s, height: %" PRIu64, elect_addr.c_str(), block_height);
        return {};
    }
    auto const & election_result_store = codec::msgpack_decode<data::election::xelection_result_store_t>({std::begin(result), std::end(result)});
    return election_result_store;
}
#endif
std::pair<common::xcluster_id_t, common::xgroup_id_t> xtop_election_data_accessor::get_cid_gid_from_xip(const common::xip2_t & xip) {
    return {xip.cluster_id(), xip.group_id()};
}

elect_result_t xtop_election_data_accessor::get_elect_result(const data::election::xelection_result_store_t & election_result_store,
                                                             const common::xip2_t & xip,
                                                             const common::xip2_t & cache_xip) {
    auto cid_gid = get_cid_gid_from_xip(xip);
    elect_result_t results;
    for (auto const & election_result_info : election_result_store) {
        auto const & election_type_results = top::get<data::election::xelection_network_result_t>(election_result_info);
        for (auto const & election_type_result : election_type_results) {
            auto const & election_result = top::get<data::election::xelection_result_t>(election_type_result);
            for (auto const & cluster_result_info : election_result) {
                if (top::get<common::xcluster_id_t const>(cluster_result_info) != cid_gid.first) {
                    continue;
                }
                auto const & cluster_result = top::get<data::election::xelection_cluster_result_t>(cluster_result_info);
                for (auto const & group_result_info : cluster_result) {
                    if (top::get<common::xgroup_id_t const>(group_result_info) != cid_gid.second) {
                        continue;
                    }
                    auto const & group_result = top::get<data::election::xelection_group_result_t>(group_result_info);
                    for (auto const & node_info : group_result) {
                        auto const slot_id = top::get<common::xslot_id_t const>(node_info);
                        results[slot_id] = top::get<data::election::xelection_info_bundle_t>(node_info);
                    }
                }
            }
        }
    }
    m_cache.put(cache_xip, results);
    return results;
}

std::string xtop_election_data_accessor::get_elec_blockchain_addr(std::string const & owner) {
    if (owner.find(sys_contract_beacon_table_block_addr) == 0) {
        return sys_contract_rec_elect_rec_addr;
    }
    if (owner.find(sys_contract_zec_table_block_addr) == 0) {
        return sys_contract_rec_elect_zec_addr;
    }
    return sys_contract_zec_elect_consensus_addr;
}

NS_END3
