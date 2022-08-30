// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#include "xelection/xstore/xelection_data_accessor.h"

#include "xbasic/xutility.h"
#include "xcodec/xmsgpack_codec.hpp"
#include "xconfig/xconfig_register.h"
#include "xconfig/xpredefined_configurations.h"
#include "xdata/xcodec/xmsgpack/xelection/xelection_result_store_codec.hpp"
#include "xdata/xelection/xelection_cluster_result.h"
#include "xdata/xelection/xelection_group_result.h"
#include "xdata/xelection/xelection_network_result.h"
#include "xdata/xelection/xelection_result.h"
#include "xdata/xnative_contract_address.h"
#include "xelection/xdata_accessor_error.h"

#include <inttypes.h>
NS_BEG3(top, election, store)

xtop_election_data_accessor::xtop_election_data_accessor() : m_cache(64) {}

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
