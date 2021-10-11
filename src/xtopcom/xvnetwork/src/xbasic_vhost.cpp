// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xvnetwork/xbasic_vhost.h"

#include "xbasic/xcrypto_key.h"
#include "xbasic/xerror/xerror.h"
#include "xbasic/xthreading/xutility.h"
#include "xbasic/xutility.h"
#include "xcodec/xmsgpack_codec.hpp"
#include "xcommon/xsharding_info.h"
#include "xdata/xelection/xelection_cluster_result.h"
#include "xdata/xelection/xelection_group_result.h"
#include "xdata/xelection/xelection_info_bundle.h"
#include "xdata/xelection/xelection_result.h"
#include "xdata/xelection/xelection_result_store.h"
#include "xelection/xdata_accessor_error.h"
#include "xvnetwork/xaddress.h"
#include "xvnetwork/xcodec/xmsgpack/xvnetwork_message_codec.hpp"
#include "xvnetwork/xvnetwork_driver_face.h"
#include "xvnetwork/xvnetwork_error.h"
#include "xvnetwork/xvnetwork_message.h"

#include <cassert>
#include <cinttypes>
#include <functional>
#include <map>

using top::data::election::xelection_cluster_result_t;
using top::data::election::xelection_group_result_t;
using top::data::election::xelection_info_bundle_t;
using top::data::election::xelection_result_store_t;
using top::data::election::xelection_result_t;

NS_BEG2(top, vnetwork)

xtop_basic_vhost::xtop_basic_vhost(common::xnetwork_id_t const & nid,
                                   observer_ptr<time::xchain_time_face_t> const & chain_timer,
                                   observer_ptr<election::cache::xdata_accessor_face_t> const & election_cache_data_accessor)
  : m_network_id{nid}, m_chain_timer{chain_timer}, m_election_cache_data_accessor{election_cache_data_accessor} {
    assert(m_chain_timer != nullptr);
    assert(election_cache_data_accessor != nullptr);
    assert(election_cache_data_accessor->network_id() == nid);
}

common::xnetwork_id_t const & xtop_basic_vhost::network_id() const noexcept {
    return m_network_id;
}

std::map<common::xslot_id_t, data::xnode_info_t> xtop_basic_vhost::members_info_of_group2(xcluster_address_t const & group_addr, common::xelection_round_t const & election_round) const {
    assert(m_election_cache_data_accessor != nullptr);

    std::error_code ec{election::xdata_accessor_errc_t::success};

    return m_election_cache_data_accessor->sharding_nodes(group_addr, election_round, ec);
}

std::map<common::xslot_id_t, data::xnode_info_t> xtop_basic_vhost::members_info_of_group(xcluster_address_t const & group_addr,
                                                                                         common::xelection_round_t const & election_round,
                                                                                         std::error_code & ec) const {
    assert(m_election_cache_data_accessor != nullptr);
    return m_election_cache_data_accessor->sharding_nodes(group_addr, election_round, ec);
}

common::xnode_address_t xtop_basic_vhost::parent_group_address(xvnode_address_t const & child_addr) const {
    std::error_code ec{election::xdata_accessor_errc_t::success};

    return m_election_cache_data_accessor->parent_address(child_addr.sharding_address(), child_addr.election_round(), ec);
}

std::map<xvnode_address_t, xcrypto_key_t<pub>> xtop_basic_vhost::crypto_keys(std::vector<xvnode_address_t> const & nodes) const {
    std::map<xvnode_address_t, xcrypto_key_t<pub>> result;
    XLOCK_GUARD(m_crypto_keys_mutex) {
        for (auto const & node : nodes) {
            try {
                auto key = m_crypto_keys.at(node.node_id());
                result.insert({node, std::move(key)});
            } catch (std::exception const &) {
                // swallow
                xerror("[vnetwork] missing crypto key for node %s", node.to_string().c_str());
            }
        }
    }

    if (result.empty()) {
        std::string s{" "};
        for (auto const & node : nodes) {
            s += node.to_string() + "|";
        }
        top::error::throw_error({ xvnetwork_errc_t::missing_crypto_keys }, "size:" + std::to_string(nodes.size()) + s);
    }

    return result;
}

common::xlogic_time_t xtop_basic_vhost::last_logic_time() const noexcept {
    return m_chain_timer->logic_time();
}

NS_END2
