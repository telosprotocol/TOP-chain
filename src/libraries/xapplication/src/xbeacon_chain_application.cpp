// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xapplication/xbeacon_chain_application.h"

#include "xapplication/xapplication.h"
#include "xapplication/xerror/xerror.h"
#include "xbasic/xerror/xerror.h"
#include "xbasic/xscope_executer.h"
#include "xcodec/xmsgpack_codec.hpp"
#include "xcommon/xip.h"
#include "xdata/xblocktool.h"
#include "xdata/xcodec/xmsgpack/xelection/xelection_result_store_codec.hpp"
#include "xdata/xelection/xelection_result_property.h"
#include "xdata/xelection/xelection_result_store.h"
#include "xdata/xnative_contract_address.h"
#include "xstatestore/xstatestore_face.h"

#include <cinttypes>

NS_BEG2(top, application)

xtop_beacon_chain_application::xtop_beacon_chain_application(observer_ptr<xapplication_t> const & application,
                                                             xobject_ptr_t<base::xvblockstore_t> & blockstore,
                                                             xobject_ptr_t<base::xvcertauth_t> & cert_ptr,
                                                             observer_ptr<base::xiothread_t> const & grpc_thread,
                                                             observer_ptr<base::xiothread_t> const & sync_thread,
                                                             std::vector<observer_ptr<base::xiothread_t>> const & sync_account_thread_pool,
                                                             std::vector<observer_ptr<base::xiothread_t>> const & sync_handler_thread_pool)
  : xbase_t{application,
            common::xnetwork_id_t{top::config::to_chainid(XGET_CONFIG(chain_name))},
            blockstore,
            cert_ptr,
            grpc_thread,
            sync_thread,
            sync_account_thread_pool,
            sync_handler_thread_pool} {
}

void xtop_beacon_chain_application::load_last_election_data() {
    std::vector<common::xaccount_address_t> sys_addr{rec_elect_rec_contract_address,
                                                     rec_elect_archive_contract_address,
                                                     rec_elect_exchange_contract_address,
                                                     rec_elect_fullnode_contract_address,
                                                     rec_elect_edge_contract_address,
                                                     rec_elect_zec_contract_address,
                                                     zec_elect_consensus_contract_address,
                                                     zec_elect_eth_contract_address,
                                                     relay_make_block_contract_address};

    std::map<common::xaccount_address_t, common::xzone_id_t> addr_to_zone_id{{rec_elect_rec_contract_address, common::xcommittee_zone_id},
                                                                             {rec_elect_zec_contract_address, common::xzec_zone_id},
                                                                             {rec_elect_archive_contract_address, common::xstorage_zone_id},
                                                                             {rec_elect_exchange_contract_address, common::xstorage_zone_id},
                                                                             {rec_elect_fullnode_contract_address, common::xfullnode_zone_id},
                                                                             {rec_elect_edge_contract_address, common::xedge_zone_id},
                                                                             {zec_elect_consensus_contract_address, common::xdefault_zone_id},
                                                                             {zec_elect_eth_contract_address, common::xevm_zone_id},
                                                                             {relay_make_block_contract_address, common::xrelay_zone_id}};
    for (const auto & addr : sys_addr) {
        for (auto const & property : data::election::get_property_name_by_addr(addr)) {
            common::xzone_id_t zone_id = addr_to_zone_id[addr];
            using top::data::election::xelection_result_store_t;
            std::error_code ec;

            xwarn("xbeacon_chain_application::load_last_election_data begin. contract %s; property %s", addr.c_str(), property.c_str());
            xscope_executer_t loading_result_logger{ [&ec, &addr, property] {
                xwarn("xbeacon_chain_application::load_last_election_data end. contract %s; property %s", addr.c_str(), property.c_str());
            } };


            data::xunitstate_ptr_t unitstate = statestore::xstatestore_hub_t::instance()->get_unit_latest_connectted_change_state(addr);
            if (unitstate == nullptr) {
                xerror("xtop_application::load_last_election_data fail-get state.");
                top::error::throw_error(ec);
                continue;
            }
            std::string result;
            if (xsuccess != unitstate->string_get(property, result)) {
                xerror("xtop_application::load_last_election_data fail-get property.");
                top::error::throw_error(ec);
                continue;
            }

            uint64_t block_height = unitstate->height();
            auto const & last_election_result_store = codec::msgpack_decode<data::election::xelection_result_store_t>({ std::begin(result), std::end(result) });
            xinfo("xbeacon_chain_application::load_last_election_data load block.addr=%s,height=%ld", addr.c_str(), block_height);

            if ((addr == rec_elect_rec_contract_address || addr == rec_elect_zec_contract_address || addr == zec_elect_consensus_contract_address ||
                 addr == zec_elect_eth_contract_address || addr == relay_make_block_contract_address) &&
                block_height != 0) {
                uint64_t prev_block_height = block_height - 1;
                data::xunitstate_ptr_t unitstate2 = statestore::xstatestore_hub_t::instance()->get_unit_committed_changed_state(addr, prev_block_height);
                if (unitstate == nullptr) {
                    xwarn("xtop_application::load_last_election_data fail-get state.");
                } else {
                    if (xsuccess != unitstate2->string_get(property, result)) {
                        xerror("xtop_application::load_last_election_data fail-get property.");
                        top::error::throw_error(ec);
                        continue;
                    }                   
                    auto const & before_last_election_result_store = codec::msgpack_decode<data::election::xelection_result_store_t>({ std::begin(result), std::end(result) });
                    on_election_data_updated(before_last_election_result_store, zone_id, prev_block_height);  // TODO(jimmy) use state->get_block_height() ?
                }
            }
            on_election_data_updated(last_election_result_store, zone_id, block_height);
        }
    }
}

NS_END2
